/*-----------------------------------------------------------------------------
--
--	tek.lib.string
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		String management supporting two 31 bit values for each codepoint
--		(intended for character strings and their metadata). Strings are
--		stored internally as UTF-8 encoded snippets which are automatically
--		packed and unpacked on demand. Forward iteration, insertion and
--		deletion is considered fast, even for strings of arbitrary length
--		and at arbitrary positions.
--
--	FUNCTIONS::
--		- String:attachdata() - Attach a Lua value to a string
--		- String.encodeutf8() - Encode codepoints to UTF-8
--		- String:erase() - Erases a range of a string
--		- String:find() - Finds the UTF-8 string in a string object
--		- String.foreachutf8() - Iterate codepoints of an UTF-8 encoded string
--		- String:free() - Reset string object, free associated data
--		- String:get() - Returns string
--		- String:getchar() - Returns character UTF-8 encoded
--		- String:getdata() - Retrieves Lua value associated with a string
--		- String:getval() - Returns character and metadata
--		- String:insert() - Appends or inserts string
--		- String:len() - Returns string length
--		- String.new() - Creates a new string object
--		- String:overwrite() - Overwrites range of a string
--		- String:set() - Initializes string object
--		- String:setmetadata() - Set metadata in a string object
--		- String:sub() - Returns substring
--
-------------------------------------------------------------------------------

module "tek.lib.string"
_VERSION = "String 2.0"
local String = _M

******************************************************************************/

/*#define NDEBUG*/
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <tek/lib/tek_lua.h>
#include <tek/teklib.h>
#include <tek/lib/utf8.h>
#include <tek/lib/tekui.h>

#define TEK_LIB_STRING_VERSION	"String Library 2.0"
#define TEK_LIB_STRING_NAME		"tek.lib.string"

/*****************************************************************************/

#define COMPACTION
/*#define SANITY_CHECKS*/
/*#define DUMP_STRING*/
/*#define STATS*/

#define MAXOVERSIZE	64	/* max node length that receives an overhang */
#define OVERSIZE	8
#define COMPACT_THRESHOLD	30

#define NUMFIELDS	2
#define METAIDX		1

/*****************************************************************************/

typedef int32_t tek_char;

typedef intptr_t tek_size;

typedef struct 
{ 
	tek_char cdata[NUMFIELDS];
} tek_entity;

typedef struct 
{
	struct TNode tsn_Node;
	tek_size tsn_Length;
	tek_size tsn_AllocLength;
	tek_entity tsn_Data[0];
} tek_node;

typedef struct 
{ 
	tek_size pos; 
	tek_node *node; 
#if defined(COMPACTION)
	tek_size utf8pos[NUMFIELDS];
#endif
} tek_hint;

typedef struct TEKString
{
	struct TList ts_List;
	tek_size ts_Length;
	tek_hint ts_Hint;
#if defined(COMPACTION)
	unsigned char *ts_UTF8[NUMFIELDS];
	tek_size ts_UTF8Len[NUMFIELDS];
#endif
	int ts_NumReadAccess;
	int ts_RefData;
} tek_string;

/*****************************************************************************/

static int getfirstlast(int len, tek_size *pp0, tek_size *pp1)
{
	if (len > 0)
	{
		tek_size p0 = *pp0;
		tek_size p1 = *pp1;
		if (p0 == 0)
			p0 = 1;
		if (p0 < 0)
		{
			p0 = len + 1 + p0;
			p0 = TMAX(p0, 1);
		}
		if (p1 < 0)
		{
			p1 = len + 1 + p1;
			p1 = TMIN(p1, len);
		}
		if (p0 <= len && p1 >= p0)
		{
			if (p1 > len)
				p1 = len;
			*pp0 = p0;
			*pp1 = p1;
			return 1;
		}
	}
	return 0;
}

static int getfirst(int len, tek_size *pp0, tek_size min, tek_size max)
{
	tek_size p0 = *pp0;
	if (len == 0 && p0 == 1)
		return 1;
	if (p0 < 0)
		p0 = len + 1 + p0;
	if (p0 >= min && p0 <= max)
	{
		*pp0 = p0;
		return 1;
	}
	return 0;
}

/*****************************************************************************/

#if defined(STATS)
static int s_numstr = 0;
static int s_unpacked = 0;
static int s_count = 0;
#if defined(COMPACTION)
static int s_reads = 0;
static int s_hints = 0;
#endif
static int s_alloccount = 0;
static int s_allocnum = 0;
static tek_size s_allocbytes = 0;
static tek_size s_maxbytes = 0;
static tek_size s_maxnum = 0;

static void dostats(void)
{
	if (++s_count % 500 > 0) return;
	fprintf(stderr, "stat strings=%d unpacked=%d\n", s_numstr, s_unpacked);
}
#endif

#if defined(DUMP_STRING)
static void tek_string_dump(tek_string *s)
{
	struct TNode *next, *node = s->ts_List.tlh_Head;
	tek_size pos = 1;
#if defined(SANITY_CHECKS)
	tek_size elen = 0;
#endif
	for (; (next = node->tln_Succ); node = next)
	{
		tek_node *sn = (tek_node *) node;
		int i;
#if defined(SANITY_CHECKS)
		elen += sn->tsn_Length;
#endif
		for (i = 0; i < sn->tsn_Length; ++i)
		{
			int c = sn->tsn_Data[i].cdata[0];
			if (c == 9) c = ' ';
			if (s->ts_Hint.pos == pos && s->ts_Hint.node == sn) c = '*';
			fprintf(stderr, "%c", c);
			pos++;
		}
		for (i = sn->tsn_Length; i < sn->tsn_AllocLength; ++i)
			fprintf(stderr, ".");
		fprintf(stderr, " ");
	}
	fprintf(stderr, "\n");
#if defined(SANITY_CHECKS)
	if (elen != s->ts_Length)
	{
		fprintf(stderr, "*** string illegal len=%d expected=%d\n", elen, s->ts_Length);
		abort();
	}
#endif
}
#endif

#if defined(SANITY_CHECKS)
static void tek_string_check(tek_string *s)
{
	tek_size hintpos = s->ts_Hint.pos;
	if (hintpos > 0 && hintpos <= s->ts_Length)
	{
		struct TNode *next, *node = s->ts_List.tlh_Head;
		tek_size p0 = 1;
		int i = 1;
		assert(hintpos != 1);
		assert(hintpos <= s->ts_Length);
		for (; (next = node->tln_Succ); node = next, i++)
		{
			tek_node *sn = (tek_node *) node;
			if (hintpos == p0)
			{
				if (sn != s->ts_Hint.node)
				{
					fprintf(stderr, "hintpos=%ld, p0=%ld, node=%d\n",
						hintpos, p0, i);
					assert(sn == s->ts_Hint.node);
				}
				return;	/* check passed */
			}
			if (hintpos < p0 + sn->tsn_Length)
			{
				fprintf(stderr, "hpos=%ld, p0=%ld, p0+len=%ld, node=%d\n",
					hintpos, p0, p0 + sn->tsn_Length, i);
				assert(hintpos >= p0 + sn->tsn_Length);
			}
			p0 += sn->tsn_Length;
		}
		assert(0);
	}
}
#endif

/*****************************************************************************/

static void *tek_string_alloc(tek_size size)
{
	void *mem = malloc(size);
#if defined(STATS)
	if (mem)
	{
		s_allocbytes += size;
		s_maxbytes = TMAX(s_maxbytes, s_allocbytes);
		s_allocnum++;
		s_maxnum = TMAX(s_maxnum, s_allocnum);
		if (++s_alloccount % 1000 == 0)
			fprintf(stderr, "allocs=%d,max=%ld bytes=%ld,max=%ld\n", 
				s_allocnum, s_maxnum, s_allocbytes, s_maxbytes);
	}
#endif
	return mem;
}

static void tek_string_free(void *mem, tek_size size)
{
#if defined(STATS)
	s_allocbytes -= size;
	s_allocnum--;
#endif
	free(mem);
}

static tek_node *tek_string_allocnode(lua_State *L, tek_size num)
{
	/* so larger blocks tend to fragment into smaller ones: */
	tek_size oversize = num < MAXOVERSIZE ? OVERSIZE : 0;
	size_t size = sizeof(tek_node) + (num + oversize) * sizeof(tek_entity);
	tek_node *s = tek_string_alloc(size);
	if (s == NULL) luaL_error(L, "Out of memory");
	s->tsn_Length = num;
	s->tsn_AllocLength = num + oversize;
	return s;
}

static void tek_string_freelist(lua_State *L, tek_string *s)
{
	tek_node *sn;
#if defined(STATS)
	if (!TISLISTEMPTY(&s->ts_List))
		s_unpacked--;
	dostats();
#endif
	while ((sn = (tek_node *) TRemHead(&s->ts_List)))
		tek_string_free(sn, 
			sizeof(tek_node) + sn->tsn_AllocLength * sizeof(tek_entity));
	s->ts_Length = 0;
	s->ts_Hint.pos = 0;
	s->ts_Hint.node = TNULL;
}

/*****************************************************************************/

static void tek_string_hintnode(tek_string *s, tek_size pos, tek_node *node)
{
	if (pos > 1) /* no point in hinting at position 1 */
	{
		s->ts_Hint.pos = pos;
		s->ts_Hint.node = node;
	}
#if defined(SANITY_CHECKS)
	tek_string_check(s);
#endif
}

/*
**	invalidate hints beginning with the given position
*/

static void tek_string_unhint(tek_string *s, tek_size pos)
{
	if (s->ts_Hint.pos > pos) /* invalidate */
		s->ts_Hint.pos = 0;
#if defined(SANITY_CHECKS)
	tek_string_check(s);
#endif
}

/*****************************************************************************/

#if defined(COMPACTION)

static void tek_string_freeutf8(lua_State *L, tek_string *s)
{
	int i;
	for (i = 0; i < NUMFIELDS; ++i)
	{
		if (s->ts_UTF8[i])
		{
			tek_string_free(s->ts_UTF8[i], s->ts_UTF8Len[i]);
			s->ts_UTF8[i] = TNULL;
			s->ts_UTF8Len[i] = 0;
		}
	}
}

static unsigned char *tek_string_allocutf8n(lua_State *L, tek_string *s,
	int idx, tek_size lenc)
{
	unsigned char *c = tek_string_alloc(lenc);
	if (!c)
		luaL_error(L, "Out of memory");
	assert(!s->ts_UTF8[idx]);
	s->ts_UTF8[idx] = c;
	s->ts_UTF8Len[idx] = lenc;
	return c;
}

static void tek_string_unpack(lua_State *L, tek_string *s)
{
	struct utf8reader rd[NUMFIELDS];
	tek_node *sn;
	tek_entity *chars;
	int i;
	
	if (s->ts_Length == 0) return;
	if (!TISLISTEMPTY(&s->ts_List))
	{
		assert(!s->ts_UTF8[0]);
		return;
	}
	assert(s->ts_UTF8[0]);
	
	utf8initreader(&rd[0], s->ts_UTF8[0], s->ts_UTF8Len[0]);
	for (i = 1; i < NUMFIELDS; ++i)
	{
		if (s->ts_UTF8[i])
			utf8initreader(&rd[i], s->ts_UTF8[i], s->ts_UTF8Len[i]);
	}
	
	sn = tek_string_allocnode(L, s->ts_Length);
	chars = sn->tsn_Data;
	for (;;)
	{
		tek_char c = utf8read(&rd[0]);
		if (c < 0) break;
		for (i = 1; i < NUMFIELDS; ++i)
			chars->cdata[i] = s->ts_UTF8[i] ? utf8read(&rd[i]) : 0;
		chars++->cdata[0] = c;
	}
	TAddTail(&s->ts_List, &sn->tsn_Node);
	tek_string_freeutf8(L, s);
	tek_string_unhint(s, 0);
	
#if defined(STATS)
	s_unpacked++;
	dostats();
#endif
}

static void tek_string_compact_int(lua_State *L, tek_string *s)
{
	unsigned char utf8buf[6];
	tek_size i, j;
	struct TNode *next, *node = s->ts_List.tlh_Head.tln_Succ;
	tek_size elen[NUMFIELDS];
	int have_data[NUMFIELDS];
	unsigned char *utf8[NUMFIELDS];
	
	if (s->ts_Length == 0) return;
	if (s->ts_UTF8[0])
	{
		assert(TISLISTEMPTY(&s->ts_List));
		return;
	}
	assert(!TISLISTEMPTY(&s->ts_List));
	
	for (j = 0; j < NUMFIELDS; ++j)
		elen[j] = have_data[j] = 0;
	
	for (; (next = node->tln_Succ); node = next)
	{
		tek_node *sn = (tek_node *) node;
		for (i = 0; i < sn->tsn_Length; ++i)
		{
			elen[0] += utf8encode(utf8buf, sn->tsn_Data[i].cdata[0]) - utf8buf;
			for (j = 1; j < NUMFIELDS; ++j)
			{
				tek_char c = sn->tsn_Data[i].cdata[j];
				if (c > 0)
					have_data[j] = 1;
				elen[j] += utf8encode(utf8buf, c) - utf8buf;
			}	
		}
	}
	
	tek_string_freeutf8(L, s);
	tek_string_allocutf8n(L, s, 0, elen[0]);
	for (j = 1; j < NUMFIELDS; ++j)
	{
		if (have_data[j])
			tek_string_allocutf8n(L, s, j, elen[j]);
	}
	
	node = s->ts_List.tlh_Head.tln_Succ;

	for (j = 0; j < NUMFIELDS; ++j)
		utf8[j] = s->ts_UTF8[j];
	
	for (; (next = node->tln_Succ); node = next)
	{
		tek_node *sn = (tek_node *) node;
		for (i = 0; i < sn->tsn_Length; ++i)
		{
			utf8[0] = utf8encode(utf8[0], sn->tsn_Data[i].cdata[0]);
			for (j = 1; j < NUMFIELDS; ++j)
			{
				if (have_data[j])
					utf8[j] = utf8encode(utf8[j], sn->tsn_Data[i].cdata[j]);
			}
		}
		TRemove(node);
		tek_string_free(sn, 
			sizeof(tek_node) + sn->tsn_AllocLength * sizeof(tek_entity));
	}
	assert(utf8[0] - s->ts_UTF8[0] == elen[0]);
	assert(!have_data[1] || utf8[1] - s->ts_UTF8[1] == elen[1]);
	
	tek_string_unhint(s, 0);
	
#if defined(STATS)
	s_unpacked--;
	dostats();
#endif
}

#endif /* defined(COMPACTION) */

static int tek_string_compact(lua_State *L)
{
#if defined(COMPACTION)
	tek_string *s = luaL_checkudata(L, 1, TEK_LIB_STRING_NAME "*");
	tek_string_compact_int(L, s);
#endif /* defined(COMPACTION) */
	return 0;
}

/*****************************************************************************/

static tek_node *tek_string_getnode(lua_State *L, tek_string *s, 
	tek_size pos, tek_size *ppos)
{
	struct TNode *next, *node;
	tek_size p0 = 1;
	assert(pos > 0);
#if defined(COMPACTION)
	tek_string_unpack(L, s);
#endif
	node = s->ts_List.tlh_Head.tln_Succ;

	if (pos > s->ts_Length)
	{
		*ppos = 0;
		return TNULL;
	}

	if (pos > 1) /* no point in looking for hints to position 1 */
	{
		tek_size hintpos = s->ts_Hint.pos;
		if (hintpos > 0 && hintpos <= pos)
		{
			node = &s->ts_Hint.node->tsn_Node;
			p0 = hintpos;
#if defined(SANITY_CHECKS)
			tek_string_check(s);
#endif
		}
	}

	for (; (next = node->tln_Succ); node = next)
	{
		tek_node *sn = (tek_node *) node;
		tek_size p1 = p0 + sn->tsn_Length;
		assert(sn->tsn_Length > 0);
		if (pos >= p0 && pos < p1)
		{
			tek_string_hintnode(s, p0, sn);
			*ppos = p0;
			return sn;
		}
		p0 = p1;
	}

	assert(0);
	return TNULL;
}

static tek_char tek_string_getval_int(lua_State *L, tek_string *s, 
	tek_size p0, tek_entity *p_entity)
{
	tek_size pos;	
	tek_node *sn;
	
#if defined(COMPACTION)
	if (s->ts_UTF8[0])
	{
		struct utf8reader rd[NUMFIELDS];
		tek_size pos = 1;
		tek_size rawpos[NUMFIELDS];
		int i;
		
 		if (p0 > 1 && s->ts_Hint.pos > 0 && s->ts_Hint.pos <= p0)
 		{
			memcpy(rawpos, s->ts_Hint.utf8pos, sizeof rawpos);
 			pos = s->ts_Hint.pos;
#if defined(STATS)
 			s_hints++;
#endif
		}
		else
		{
			for (i = 0; i < NUMFIELDS; ++i)
				rawpos[i] = 0;
		}
		
#if defined(STATS)
		if (p0 > 1 && ++s_reads % 5000 == 0)
			fprintf(stderr, "hints/reads = %d/%d = %.3f\n", s_hints, 
				s_reads, (float)s_hints / s_reads);
#endif
		
		for (i = 0; i < NUMFIELDS; ++i)
		{
			if (s->ts_UTF8[i])
				utf8initreader(&rd[i], s->ts_UTF8[i] + rawpos[i], 
					s->ts_UTF8Len[i] - rawpos[i]);
		}
		
		for (;;)
		{
			tek_char c[NUMFIELDS];
			for (i = 0; i < NUMFIELDS; ++i)
			{
				rawpos[i] = rd[i].rsdata.src - s->ts_UTF8[i];
				c[i] = s->ts_UTF8[i] ? utf8read(&rd[i]) : 0;
				assert(c[i] >= 0);
			}
			if (pos == p0)
			{
				if (pos > 1)
				{
					s->ts_Hint.pos = pos;
					memcpy(s->ts_Hint.utf8pos, rawpos, sizeof rawpos);
				}
				if (p_entity)
					memcpy(p_entity->cdata, c, sizeof c);
				return c[0];
			}
			pos++;
		}
		assert(0);
	}
#endif /* defined(COMPACTION) */
	
	sn = tek_string_getnode(L, s, p0, &pos);
	if (sn == NULL) return -1;
	assert(pos > 0);
	if (p_entity)
		memcpy(p_entity, &sn->tsn_Data[p0 - pos], sizeof(tek_entity));
	return sn->tsn_Data[p0 - pos].cdata[0];
}

/*-----------------------------------------------------------------------------
--	string = String:set(string2[, p0[, p1]]): Initializes the string
--	object with {{string2}}, which can be either an UTF-8
--	encoded string or another string object. An optional range starts at
--	position {{p0}} and extends to the position {{p1}}, which may be negative
--	to indicate the number of characters from the end of the string.
--	Metadata at the affected positions is set to {{0}}. Returns itself.
-----------------------------------------------------------------------------*/

static int tek_string_set(lua_State *L)
{
	tek_string *s = luaL_checkudata(L, 1, TEK_LIB_STRING_NAME "*");
	tek_string *s2 = TNULL;
	struct utf8reader rd;
	tek_size inslen;
	tek_size p0 = luaL_optinteger(L, 3, 1);
	tek_size p1 = luaL_optinteger(L, 4, -1);
	tek_char c;
	
	size_t rawlen = 0;
	const unsigned char *raws = TNULL;
	
	if (lua_type(L, 2) == LUA_TSTRING)
	{
		raws = (const unsigned char *) luaL_checklstring(L, 2, &rawlen);
		inslen = utf8getlen(raws, rawlen);
	}
	else if (lua_type(L, 2) == LUA_TNIL)
		inslen = 0;
	else
	{
		s2 = luaL_checkudata(L, 2, TEK_LIB_STRING_NAME "*");
		inslen = s2->ts_Length;
	}
	
	tek_string_freelist(L, s);
	
#if defined(COMPACTION)
	tek_string_freeutf8(L, s);
#endif

	if (inslen > 0 && getfirstlast(inslen, &p0, &p1))
	{
		tek_size partlen = p1 - p0 + 1;
#if defined(COMPACTION)
		unsigned char utf8buf[6];
		tek_size j, i;
		tek_entity e;
		
		tek_size elen[NUMFIELDS];
		int have_data[NUMFIELDS];
		unsigned char *utf8[NUMFIELDS];
		
		for (j = 0; j < NUMFIELDS; ++j)
			elen[j] = have_data[j] = 0;
		
		if (s2)
		{
			for (j = p0; j <= p1; ++j)
			{
				c = tek_string_getval_int(L, s2, j, &e);
				elen[0] += utf8encode(utf8buf, c) - utf8buf;
				for (i = 1; i < NUMFIELDS; ++i)
				{
					tek_char c = e.cdata[i];
					if (c > 0)
						have_data[i] = 1;
					elen[i] += utf8encode(utf8buf, c) - utf8buf;
				}
			}
		}
		else
		{
			tek_size pos = 0;
			utf8initreader(&rd, raws, rawlen);
			while ((c = utf8read(&rd)) >= 0)
			{
				pos++;
				if (pos < p0) continue;
				if (pos > p1) break;
				elen[0] += utf8encode(utf8buf, c) - utf8buf;
			}
#if defined(SANITY_CHECKS)
			assert(pos == inslen);
#endif
		}
		
		tek_string_freeutf8(L, s);
		tek_string_allocutf8n(L, s, 0, elen[0]);
		for (j = 1; j < NUMFIELDS; ++j)
		{
			if (have_data[j])
				tek_string_allocutf8n(L, s, j, elen[j]);
		}

		for (j = 0; j < NUMFIELDS; ++j)
			utf8[j] = s->ts_UTF8[j];
		
		if (s2)
		{
			for (j = p0; j <= p1; ++j)
			{
				c = tek_string_getval_int(L, s2, j, &e);
				utf8[0] = utf8encode(utf8[0], c);
				for (i = 1; i < NUMFIELDS; ++i)
				{
					if (have_data[i])
						utf8[i] = utf8encode(utf8[i], e.cdata[i]);
				}
			}
		}
		else
		{
			tek_size pos = 0;
			utf8initreader(&rd, raws, rawlen);
			while ((c = utf8read(&rd)) >= 0)
			{
				pos++;
				if (pos < p0) continue;
				if (pos > p1) break;
				utf8[0] = utf8encode(utf8[0], c);
			}
#if defined(SANITY_CHECKS)
			assert(pos == inslen);
#endif
		}
#else
		tek_node *sn = tek_string_allocnode(L, partlen);
		tek_entity *chars = sn->tsn_Data;
		TAddTail(&s->ts_List, &sn->tsn_Node);
		if (s2)
		{
			tek_size j, i;
			for (j = p0, i = 0; j <= p1; ++j, ++i)
				tek_string_getval_int(L, s2, j, &chars[i]);
		}
		else
		{
			tek_size pos = 0;
			tek_size i = 0, j;
			utf8initreader(&rd, raws, rawlen);
			while ((c = utf8read(&rd)) >= 0)
			{
				pos++;
				if (pos < p0) continue;
				if (pos > p1) break;
				chars[i].cdata[0] = c;
				for (j = 1; j < NUMFIELDS; ++j)
					chars[i].cdata[j] = 0;
				i++;
			}
		}
#endif
		s->ts_Length = partlen;
	}
	s->ts_NumReadAccess = 0;
	tek_string_unhint(s, 0);
	lua_pushvalue(L, 1);
	return 1;
}

/*-----------------------------------------------------------------------------
--	len = String:len(): Returns the number of characters in the string object.
-----------------------------------------------------------------------------*/

static int tek_string_len(lua_State *L)
{
	tek_string *s = luaL_checkudata(L, 1, TEK_LIB_STRING_NAME "*");
	lua_pushinteger(L, s->ts_Length);
	return 1;
}

/*-----------------------------------------------------------------------------
--	utf8str = String:get([p0[, p1]]): An alias for String:sub()
-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------
--	utf8str = String:sub([p0[, p1]]): Returns an UTF-8 encoded string
--	representing the string object or a range of it. An optional range starts
--	at position {{p0}} and extends to the position {{p1}}, which may be
--	negative to indicate the number of characters from the end of the string.
-----------------------------------------------------------------------------*/

static int tek_string_sub(lua_State *L)
{
	tek_string *s = luaL_checkudata(L, 1, TEK_LIB_STRING_NAME "*");
	tek_size p0 = luaL_optinteger(L, 2, 1);
	tek_size p1 = luaL_optinteger(L, 3, -1);
	if (getfirstlast(s->ts_Length, &p0, &p1))
	{
		tek_size pos = 0;
		struct TNode *next, *node;
		unsigned char utf8buf[6];
		luaL_Buffer b;
	
#if defined(COMPACTION)
		if (p0 == 1 && p1 == s->ts_Length && s->ts_UTF8[0])
		{
			lua_pushlstring(L, (const char *) s->ts_UTF8[0], s->ts_UTF8Len[0]);
			return 1;
		}
#endif
		node = &tek_string_getnode(L, s, p0, &pos)->tsn_Node;
		luaL_buffinit(L, &b);
		for (; (next = node->tln_Succ); node = next)
		{
			tek_node *sn = (tek_node *) node;
			tek_size i;
			size_t len;
			tek_char c;
			for (i = 0; i < sn->tsn_Length; ++i, ++pos)
			{
				if (pos < p0) continue;
				if (pos > p1) goto done;
				c = sn->tsn_Data[i].cdata[0];
				len = utf8encode(utf8buf, c) - utf8buf;
				luaL_addlstring(&b, (const char *) utf8buf, len);
			}
		}
done:	luaL_pushresult(&b);
		return 1;
	}
	lua_pushstring(L, "");
	return 1;
}

static tek_node *tek_string_breaklist(lua_State *L, tek_string *s, int pos)
{
	tek_size p0;
	tek_node *newnode1, *newnode2;
	struct TNode *node;
	tek_size i;
	tek_node *sn = tek_string_getnode(L, s, pos, &p0);
	tek_size splitpos;
		
	tek_string_unhint(s, 0);
	if (sn == TNULL)
		return (tek_node *) s->ts_List.tlh_Tail.tln_Pred;
	node = &sn->tsn_Node;
	splitpos = pos - p0;
	assert(splitpos >= 0);
	if (splitpos == 0)
		return (tek_node *) sn->tsn_Node.tln_Pred;
	assert(sn->tsn_Length - splitpos > 0);
	newnode1 = tek_string_allocnode(L, splitpos);
	newnode2 = tek_string_allocnode(L, sn->tsn_Length - splitpos);
	for (i = 0; i < splitpos; ++i)
		newnode1->tsn_Data[i] = sn->tsn_Data[i];
	for (i = splitpos; i < sn->tsn_Length; ++i)
		newnode2->tsn_Data[i - splitpos] = sn->tsn_Data[i];
	TInsert(&s->ts_List, &newnode1->tsn_Node, node);
	TInsert(&s->ts_List, &newnode2->tsn_Node, &newnode1->tsn_Node);
	TRemove(node);
	tek_string_free(sn, 
		sizeof(tek_node) + sn->tsn_AllocLength * sizeof(tek_entity));
	return newnode1;
}

/*-----------------------------------------------------------------------------
--	String:insert(string2[, pos]): Inserts {{string2}} into the string
--	object, optionally at the given starting position. The default position
--	is at the end of the string. {{string2}} may be an UTF-8 encoded string or
--	another string object. Metadata from another string object will be
--	inserted accordingly, otherwise it is set to {{0}}.
-----------------------------------------------------------------------------*/

static int tek_string_insert(lua_State *L)
{
	tek_string *s = luaL_checkudata(L, 1, TEK_LIB_STRING_NAME "*");
	tek_string *s2 = TNULL;
	tek_size p0;
	tek_size inslen;
	struct utf8reader rd;
	tek_node *nnode;
	tek_node *sn;
	tek_entity *chars;
	tek_char c;
	tek_size nodepos;
	tek_size j;
	
	if (lua_type(L, 2) == LUA_TSTRING)
	{
		size_t rawlen;
		const unsigned char *raws = 
			(const unsigned char *) luaL_checklstring(L, 2, &rawlen);
		inslen = utf8getlen(raws, rawlen);
		utf8initreader(&rd, raws, rawlen);
	}
	else
	{
		s2 = luaL_checkudata(L, 2, TEK_LIB_STRING_NAME "*");
		inslen = s2->ts_Length;
	}

	if (inslen == 0)
		return 0;
	
	p0 = luaL_optinteger(L, 3, s->ts_Length + 1);
	
	if (!getfirst(s->ts_Length, &p0, 1, s->ts_Length + 1))
	{
		/*luaL_error(L, "illegal position");*/
		fprintf(stderr, "illegal position\n");
		return 0;
	}
	
	nnode = tek_string_getnode(L, s, TCLAMP(1, p0 - 1, s->ts_Length), 
		&nodepos);
	if (nnode && (nnode->tsn_AllocLength - nnode->tsn_Length >= inslen))
	{
		/* enough free space in an existing node */
		tek_size innodepos = p0 - nodepos;
		chars = &nnode->tsn_Data[innodepos];
		memmove(chars + inslen, chars,
			(nnode->tsn_Length - innodepos) * sizeof(tek_entity));
		nnode->tsn_Length += inslen;
		s->ts_Length += inslen;
		tek_string_unhint(s, 0);
	}
	else
	{
		/* need new node, break at insertion pos */
		nnode = tek_string_breaklist(L, s, p0);
		sn = tek_string_allocnode(L, inslen);
		chars = sn->tsn_Data;
		TInsert(&s->ts_List, &sn->tsn_Node, &nnode->tsn_Node);
		s->ts_Length += inslen;
 		/* tek_string_hintnode(s, nodepos, nnode, "insert2"); */
		tek_string_unhint(s, 0);
	}
	
	if (s2)
	{
		for (j = 1; j <= inslen; ++j)
			tek_string_getval_int(L, s2, j, chars++);
	}
	else
	{
		while ((c = utf8read(&rd)) >= 0)
		{
			chars->cdata[0] = c;
			for (j = 1; j < NUMFIELDS; ++j)
				chars->cdata[j] = 0;
			chars++;
		}
	}

	s->ts_NumReadAccess = 0;
#if defined(DUMP_STRING)
	tek_string_dump(s);
#endif
	return 0;
}

/*-----------------------------------------------------------------------------
--	String:erase([p0[, p1]]): Erases a range of characters from a string
--	object. An optional range starts at position {{p0}} and extends to the
--	position {{p1}}, which may be negative to indicate the number of
--	characters from the end of the string.
-----------------------------------------------------------------------------*/

static int tek_string_erase(lua_State *L)
{
	tek_string *s = luaL_checkudata(L, 1, TEK_LIB_STRING_NAME "*");
	tek_size p0 = luaL_optinteger(L, 2, 1);
	tek_size p1 = luaL_optinteger(L, 3, -1);
	if (getfirstlast(s->ts_Length, &p0, &p1))
	{
		tek_node *nnode1, *nnode2;
		tek_size nodepos, eraselen, nodelen, copylen;
		
		nnode2 = tek_string_getnode(L, s, p1, &nodepos);
		nnode1 = p0 == p1 ? nnode2 : tek_string_getnode(L, s, p0, &nodepos);
		
		eraselen = p1 - p0 + 1;
		nodelen = nnode1->tsn_Length;
		copylen = nodelen - 1 - (p1 - nodepos);
		
		if (nnode1 == nnode2 && nodelen > eraselen && copylen < 256)
		{
			tek_entity *chars = nnode1->tsn_Data;
			p0 -= nodepos;
			p1 -= nodepos;
			if (copylen > 0)
				memmove(chars + p0, chars + p1 + 1, 
					copylen * sizeof(tek_entity));
			nnode1->tsn_Length -= eraselen;
			s->ts_Length -= eraselen;
			tek_string_unhint(s, p0);
		}
		else
		{
			struct TNode *node1, *node2, *next, *node;
#if defined(SANITY_CHECKS)
			tek_size elen = 0;
#endif
			nnode1 = tek_string_breaklist(L, s, p0);
			nnode2 = tek_string_breaklist(L, s, p1 + 1);
			node1 = &nnode1->tsn_Node;
			node2 = &nnode2->tsn_Node;
			node = node1->tln_Succ;
			for (; (next = node->tln_Succ); node = next)
			{
				tek_node *sn = (tek_node *) node;
#if defined(SANITY_CHECKS)
				elen += sn->tsn_Length;
				assert(s->ts_Hint.pos == 0 || 
					(tek_node *) node != s->ts_Hint.node);
#endif
				TRemove(node);
				tek_string_free(sn, sizeof(tek_node) + 
					sn->tsn_AllocLength * sizeof(tek_entity));
				if (node == node2) break;
			}
#if defined(SANITY_CHECKS)
			assert(elen == eraselen);
#endif
			s->ts_Length -= eraselen;
			tek_string_unhint(s, p0);
#if defined(STATS)
			if (TISLISTEMPTY(&s->ts_List))
				s_unpacked--;
#endif
		}
	}
	s->ts_NumReadAccess = 0;
#if defined(DUMP_STRING)
	tek_string_dump(s);
#endif
	return 0;
}

/*-----------------------------------------------------------------------------
--	String:overwrite(utf8str[, pos]): Overwrites a range of characters in a
--	string object from an UTF-8 encoded string, starting at the given position
--	(default: {{1}}). Metadata in the string object at the affected positions
--	will be set to {{0}}.
-----------------------------------------------------------------------------*/

static int tek_string_overwrite(lua_State *L)
{
	tek_string *s = luaL_checkudata(L, 1, TEK_LIB_STRING_NAME "*");
	struct utf8reader rd;
	size_t rawlen;
	tek_size pos = 0;
	const unsigned char *raws = 
		(const unsigned char *) luaL_checklstring(L, 2, &rawlen);
	tek_size p0 = luaL_optinteger(L, 3, 1);
	struct TNode *next, *node;
	
	if (!getfirst(s->ts_Length, &p0, 1, s->ts_Length))
		luaL_error(L, "illegal position");

	utf8initreader(&rd, raws, rawlen);
	
#if defined(COMPACTION)
	tek_string_unpack(L, s);
#endif
	node = s->ts_List.tlh_Head.tln_Succ;
	
	for (; (next = node->tln_Succ); node = next)
	{
		tek_node *sn = (tek_node *) node;
		tek_size i, j;
		tek_char c;
		
		if (pos + sn->tsn_Length < p0) 
		{
			pos += sn->tsn_Length;
			continue;
		}
		
		for (i = 0; i < sn->tsn_Length; ++i)
		{
			pos++;
			if (pos < p0) continue;
			
			c = utf8read(&rd);
			if (c < 0) break;
			sn->tsn_Data[i].cdata[0] = c;
			for (j = 1; j < NUMFIELDS; ++j)
				sn->tsn_Data[i].cdata[j] = 0;
		}
	}
	
	return 1;
}

/*-----------------------------------------------------------------------------
--	char, metadata = String:getval(pos): Returns a character's and its
--	metadata's numeric codepoints at the given position in a string object.
-----------------------------------------------------------------------------*/

static int tek_string_getval(lua_State *L)
{
	tek_string *s = luaL_checkudata(L, 1, TEK_LIB_STRING_NAME "*");
	tek_size p0 = luaL_checkinteger(L, 2);
	int meta_idx = luaL_optinteger(L, 3, METAIDX);
	tek_char c;
	tek_entity e;
	
	if (meta_idx < 1 || meta_idx >= NUMFIELDS)
		luaL_error(L, "metadata index out of range");
	
	if (!getfirst(s->ts_Length, &p0, 1, s->ts_Length))
		return 0;
#if defined(COMPACTION)	
	if (++s->ts_NumReadAccess > COMPACT_THRESHOLD * s->ts_Length)
	{
		tek_string_compact_int(L, s);
		s->ts_NumReadAccess = 0;
	}
#endif
	c = tek_string_getval_int(L, s, p0, &e);
	if (c >= 0)
	{
		lua_pushinteger(L, c);
		lua_pushinteger(L, e.cdata[meta_idx]);
		return 2;
	}
	return 0;
}

/*-----------------------------------------------------------------------------
--	utf8 = String:getchar(pos): Returns an UTF-8 encoded character at the
--	given position in a string object.
-----------------------------------------------------------------------------*/

static int tek_string_getchar(lua_State *L)
{
	tek_string *s = luaL_checkudata(L, 1, TEK_LIB_STRING_NAME "*");
	tek_size p0 = luaL_checkinteger(L, 2);
	if (getfirst(s->ts_Length, &p0, 1, s->ts_Length))
	{
		tek_char c;
#if defined(COMPACTION)	
		if (++s->ts_NumReadAccess > COMPACT_THRESHOLD * s->ts_Length)
		{
			tek_string_compact_int(L, s);
			s->ts_NumReadAccess = 0;
		}
#endif		
		c = tek_string_getval_int(L, s, p0, TNULL);
		if (c >= 0)
		{
			unsigned char utf8buf[6];
			size_t len = utf8encode(utf8buf, c) - utf8buf;
			lua_pushlstring(L, (const char *) utf8buf, len);
			return 1;
		}
	}
	lua_pushlstring(L, "", 0);
	return 1;
}

/*-----------------------------------------------------------------------------
--	p0, p1 = String:find(utf8[, pos]): Finds the specified UTF-8 encoded
--	string in the string object, and if found, returns the first and last
--	position of its next occurrence from the starting position
--	(default: {{1}}).
-----------------------------------------------------------------------------*/

static int tek_string_find(lua_State *L)
{
	tek_string *s = luaL_checkudata(L, 1, TEK_LIB_STRING_NAME "*");
	size_t rawlen;
	tek_size slen;
	tek_char *sbuf;
	const unsigned char *raws = 
		(const unsigned char *) luaL_checklstring(L, 2, &rawlen);
	tek_size p0 = luaL_optinteger(L, 3, 1);
	struct utf8reader rd;
	tek_size rdlen = 0;
	int j = 1;
	int ss = 0;
	int i;
	
	if (!getfirst(s->ts_Length, &p0, 1, s->ts_Length))
		return 0;

	slen = utf8getlen(raws, rawlen);
	if (slen == 0) return 0;
	sbuf = tek_string_alloc(slen * sizeof(tek_char));
	if (sbuf == NULL) luaL_error(L, "out of memory");


	utf8initreader(&rd, raws, rawlen);
	for (i = p0; i <= s->ts_Length; ++i)
	{
		tek_char c = tek_string_getval_int(L, s, i, TNULL);
		assert(c >= 0);
		if (rdlen < j)
		{
			sbuf[j-1] = utf8read(&rd);
			assert(sbuf[j - 1]);
			rdlen++;
		}
redo:	
		if (c == sbuf[j - 1])
		{
			if (j == 1)
				ss = i;
			if (j == slen)
			{
				lua_pushinteger(L, ss);
				lua_pushinteger(L, ss + j - 1);
				tek_string_free(sbuf, slen * sizeof(tek_char));
				return 2;
			}
			j++;
		}
		else
		{
			if (j > 1)
			{
				j = 1;
				goto redo;
			}
			j = 1;
		}
	}
	
	tek_string_free(sbuf, slen * sizeof(tek_char));

	return 0;
}

/*-----------------------------------------------------------------------------
--	String:setmetadata(value[, p0[, p1]]): Sets metadata in a string object
--	to the specified value. An optional range starts at position {{p0}} and
--	extends to the position {{p1}}, which may be negative to indicate the
--	number of characters from the end of the string.
-----------------------------------------------------------------------------*/

static int tek_string_setmetadata(lua_State *L)
{
	tek_string *s = luaL_checkudata(L, 1, TEK_LIB_STRING_NAME "*");
	tek_char val = luaL_checkinteger(L, 2);
	tek_size p0 = luaL_optinteger(L, 3, 1);
	tek_size p1 = luaL_optinteger(L, 4, -1);
	int meta_idx = luaL_optinteger(L, 5, METAIDX);
	struct TNode *next, *node;
	tek_size pos;
	
	if (meta_idx < 1 || meta_idx >= NUMFIELDS)
		luaL_error(L, "metadata index out of range");
	
	if (!getfirstlast(s->ts_Length, &p0, &p1))
		luaL_error(L, "illegal position");
		
	if (val < 0 || val > 0x7fffffff)
		luaL_error(L, "value out of range");
	
#if defined(COMPACTION)
	if (val < 128 && s->ts_UTF8[0] && 
		(!s->ts_UTF8[meta_idx] || s->ts_UTF8Len[meta_idx] == s->ts_Length))
	{
		assert(s->ts_Length > 0);
		if (s->ts_UTF8Len[meta_idx] == 0)
		{
			assert(!s->ts_UTF8[meta_idx]);
			s->ts_UTF8[meta_idx] = tek_string_alloc(s->ts_Length);
			memset(s->ts_UTF8[meta_idx], 0, s->ts_Length);
			s->ts_UTF8Len[meta_idx] = s->ts_Length;
			tek_string_unhint(s, 0);
		}
		if (s->ts_UTF8Len[meta_idx] == s->ts_Length)
		{
			tek_size i;
			for (i = p0 - 1; i < p1; ++i)
				s->ts_UTF8[meta_idx][i] = val;
			return 0;
		}
	}
#endif

	node = &tek_string_getnode(L, s, p0, &pos)->tsn_Node;
	for (; (next = node->tln_Succ); node = next)
	{
		tek_node *sn = (tek_node *) node;
		tek_size i;
		for (i = 0; i < sn->tsn_Length; ++i, ++pos)
		{
			if (pos < p0) continue;
			if (pos > p1) break;
			sn->tsn_Data[i].cdata[meta_idx] = val;
		}
	}
	return 0;
}


/*-----------------------------------------------------------------------------
--	string = String.new(): Creates a new, initially empty string object.
-----------------------------------------------------------------------------*/

static int tek_string_new(lua_State *L)
{
	tek_string *s = lua_newuserdata(L, sizeof(tek_string));
	memset(s, 0, sizeof(tek_string));
	s->ts_RefData = LUA_NOREF;
	TINITLIST(&s->ts_List);
	/* s: udata */
	lua_getfield(L, LUA_REGISTRYINDEX, TEK_LIB_STRING_NAME "*");
	/* s: udata, metatable */
	lua_setmetatable(L, -2);
	/* s: udata */
#if defined(STATS)
	s_numstr++;
	dostats();
#endif
	return 1;
}

/*-----------------------------------------------------------------------------
--	String:free(): Free resources held by a string object
-----------------------------------------------------------------------------*/

static int tek_string_collect(lua_State *L)
{
	tek_string *s = luaL_checkudata(L, 1, TEK_LIB_STRING_NAME "*");
	if (s->ts_RefData != LUA_NOREF)
	{
		lua_getmetatable(L, 1);
		luaL_unref(L, -1, s->ts_RefData);
		s->ts_RefData = LUA_NOREF;
		lua_pop(L, 1);
	}
	tek_string_freelist(L, s);
#if defined(COMPACTION)
	tek_string_freeutf8(L, s);
#endif
#if defined(STATS)
	s_numstr--;
	dostats();
	if (s_numstr == 0)
		fprintf(stderr, "allocs=%d,max=%ld bytes=%ld,max=%ld\n", 
			s_allocnum, s_maxnum, s_allocbytes, s_maxbytes);
#endif
	return 0;
}

/*-----------------------------------------------------------------------------
--	utf8string = String.utf8encode(table or number[, ...]): If the first
--	argument is a table, encodes the numerically indexed codepoints in that
--	table to an UTF-8 string; an optional second argument specifies the
--	first position and an optional third argument the last position in the
--	table.
--	If the first argument is a number, encodes this and the following
--	arguments as codepoints to an UTF-8 string.
-----------------------------------------------------------------------------*/

static int tek_string_utf8encode(lua_State *L)
{
	luaL_Buffer b;
	unsigned char utf8buf[6];
	int i;
	
	luaL_buffinit(L, &b);
	
	if (lua_type(L, 1) == LUA_TTABLE)
	{
#if LUA_VERSION_NUM < 502
		lua_Integer n = lua_objlen(L, 1);
#else
		lua_Integer n = lua_rawlen(L, 1);
#endif
		lua_Integer p0 = luaL_optinteger(L, 2, 1);
		lua_Integer p1 = luaL_optinteger(L, 3, n);
		for (i = 0; i < p1 - p0 + 1; ++i)
		{
			lua_Integer c;
			size_t len;
			lua_rawgeti(L, 1, i + p0);
			c = lua_tointeger(L, -1);
			lua_pop(L, 1);
			len = utf8encode(utf8buf, c) - utf8buf;
			luaL_addlstring(&b, (const char *) utf8buf, len);
		}
	}
	else
	{
		lua_Integer n = lua_gettop(L);
		for (i = 0; i < n; ++i)
		{
			lua_Integer c = lua_tointeger(L, i + 1);
			size_t len = utf8encode(utf8buf, c) - utf8buf;
			luaL_addlstring(&b, (const char *) utf8buf, len);
		}
	}
	luaL_pushresult(&b);
	return 1;
}

/*-----------------------------------------------------------------------------
--	length, maxval = String.foreachutf8(utf8string[, arg]):
--	Returns the number of codepoints in {{utf8string}} and the highest
--	codepoint number that occured in the string.
--	{{arg}} can be a table, in which case each codepoint will be inserted
--	into it as a number value, or a function, which will be called for each
--	codepoint as its first, and the current position as its second argument.
-----------------------------------------------------------------------------*/

static int tek_string_foreachutf8(lua_State *L)
{
	struct utf8reader rd;
	struct utf8readstringdata rs;
	tek_char c;
	int i = 0;
	int cwidth = 1;
	
	rd.readchar = utf8readstring;
	rd.accu = 0;
	rd.numa = 0;
	rd.bufc = -1;
	rd.udata = &rs;
	rs.src = (const unsigned char *) lua_tolstring(L, 1, &rs.srclen);
	
	if (lua_type(L, 2) == LUA_TTABLE)
	{
		/* insert each utf8 value into given table, 
		starting at optional offset */
		lua_Integer p0 = luaL_optinteger(L, 3, 1);
		while ((c = utf8read(&rd)) >= 0)
		{
			cwidth = TMAX(cwidth, c);
			lua_pushinteger(L, c);
			lua_rawseti(L, 2, p0 + i);
			i++;
		}
	}
	else if (lua_type(L, 2) == LUA_TFUNCTION)
	{
		/* call func(char, pos) for each unicode value */
		while ((c = utf8read(&rd)) >= 0)
		{
			cwidth = TMAX(cwidth, c);
			lua_pushvalue(L, 2);
			lua_pushinteger(L, c);
			lua_pushinteger(L, ++i);
			lua_call(L, 2, 0);		
		}
	}
	else
	{
		/* just determine the length and width */
		while ((c = utf8read(&rd)) >= 0)
		{
			cwidth = TMAX(cwidth, c);
			i++;
		}
	}
	
	lua_pushinteger(L, i);
	lua_pushinteger(L, cwidth);
	return 2;
}

/*****************************************************************************/

static int tek_string_getvisualpos(lua_State *L)
{
	tek_string *s = luaL_checkudata(L, 1, TEK_LIB_STRING_NAME "*");
	int tabsize = luaL_checkinteger(L, 2);
	int cx = luaL_checkinteger(L, 3);
	int vx = 0, v = 0;
	int pos0 = 1;
	int pos1;
	for (pos1 = 1; pos1 <= s->ts_Length; ++pos1)
	{
		tek_char c = tek_string_getval_int(L, s, pos1, TNULL);
		if (c == 9)
		{
			int inslen = tabsize - ((pos1 - pos0) % tabsize);
			v += inslen - 1;
			pos0 = pos1 + 1;
		}
		v++;
		if (pos1 == cx)
			break;
		vx = v;
	}
	lua_pushinteger(L, vx + 1);
	return 1;
}

static int tek_string_foreachchar(lua_State *L)
{
	tek_string *s = luaL_checkudata(L, 1, TEK_LIB_STRING_NAME "*");
	int tabsize = luaL_checkinteger(L, 2);
	int vx = 0;
	int pos0 = 1;
	int pos1;
	for (pos1 = 1; pos1 <= s->ts_Length; ++pos1)
	{
		tek_char c = tek_string_getval_int(L, s, pos1, TNULL);
		if (c == 9)
		{
			int inslen = tabsize - ((pos1 - pos0) % tabsize);
			vx += inslen - 1;
			pos0 = pos1 + 1;
		}
		vx++;
		lua_pushvalue(L, 3);
		lua_pushinteger(L, pos1);
		lua_pushinteger(L, c);
		lua_pushinteger(L, vx);
		lua_call(L, 3, 1);
		if (lua_toboolean(L, -1))
			return 1;
		lua_pop(L, 1);
	}
	return 0;
}

/*
**	text:gettextwidth(tabsize, p0, p1, fixedwidth_or_func[, arg, blankwidth])
**	func(arg, text, p0, p1): gets raw text width for a range of characters 
*/

static int tek_string_getrawwidth_int(lua_State *L, int p0, int p1, int fixedfwidth)
{
	if (fixedfwidth > 0)
		return (p1 - p0 + 1) * fixedfwidth;
	lua_pushvalue(L, 5);
	lua_pushvalue(L, 6);
	lua_pushvalue(L, 1);
	lua_pushinteger(L, p0);
	lua_pushinteger(L, p1);
	lua_call(L, 4, 1);
	int res = lua_tointeger(L, -1);
	lua_pop(L, 1);
	return res;
}

static int tek_string_gettextwidth(lua_State *L)
{
	tek_string *s = luaL_checkudata(L, 1, TEK_LIB_STRING_NAME "*");
	int tabsize = luaL_checkinteger(L, 2);
	int p0 = luaL_checkinteger(L, 3);
	int p1 = luaL_checkinteger(L, 4);
	int fixedfwidth = -1; /* fixed character width */
	int fwidth = -1; /* width of a blank character */
	
	if (lua_type(L, 5) != LUA_TFUNCTION)
	{
		fixedfwidth = luaL_checkinteger(L, 5);
		if (fixedfwidth <= 0)
			luaL_argerror(L, 5, "illegal width");
		fwidth = fixedfwidth;
	}
	else
		fwidth = luaL_checkinteger(L, 7);
	
	tek_size len = s->ts_Length;
	
	if (p1 < 0)
		p1 += len + 1;

	int w = 0;
	if (p1 >= p0 && len > 0)
	{
		int a;
		int oa = 0;
		int x = 0;
		int w0 = 0;
		p0 = TMAX(p0, 1);
		p1 = TMIN(p1, len);
		
		for (a = 1; a <= p1; ++a)
		{
			tek_char c = tek_string_getval_int(L, s, a, TNULL);
			if (a == p0)
			{
				x += tek_string_getrawwidth_int(L, oa, a - 1, fixedfwidth);
				w0 = x;
				oa = a;
			}
			if (c == 9)
			{
				x += tek_string_getrawwidth_int(L, oa, a - 1, fixedfwidth);
				int inslen = tabsize - ((a - oa) % tabsize);
				x += inslen * fwidth;
				oa = a + 1;
			}
			if (a == p1)
			{
				x += tek_string_getrawwidth_int(L, oa, a, fixedfwidth);
				oa = -1;
			}
		}
		
		if (oa > 0)
			x += tek_string_getrawwidth_int(L, oa, len, fixedfwidth);
		
		w = x - w0;
	}
	
	lua_pushinteger(L, w);
	return 1;
}


/*
**	text:attachdata(data): Attaches Lua data to a string
*/

static int tek_string_attachdata(lua_State *L)
{
	tek_string *s = luaL_checkudata(L, 1, TEK_LIB_STRING_NAME "*");
	lua_getmetatable(L, 1);
	luaL_unref(L, -1, s->ts_RefData);
	if (lua_toboolean(L, 2))
	{
		lua_pushvalue(L, 2);
		s->ts_RefData = luaL_ref(L, -2);
	}
	lua_pop(L, 1);
	return 0;
}


/*
**	data = text:getdata(): Retrieves a string's Lua data
*/

static int tek_string_getdata(lua_State *L)
{
	tek_string *s = luaL_checkudata(L, 1, TEK_LIB_STRING_NAME "*");
	if (s->ts_RefData == LUA_NOREF)
		return 0;
	lua_getmetatable(L, 1);
	lua_rawgeti(L, -1, s->ts_RefData);
	lua_remove(L, -2);
	return 1;
}



/*****************************************************************************/

static const luaL_Reg tek_string_funcs[] =
{
	{ "new", tek_string_new },
	{ "free", tek_string_collect },
	{ "encodeutf8", tek_string_utf8encode },
	{ "foreachutf8", tek_string_foreachutf8 },
	{ NULL, NULL }
};

static const luaL_Reg tek_string_methods[] =
{
	{ "__gc", tek_string_collect },
	{ "__tostring", tek_string_sub },
	{ "free", tek_string_collect },

	{ "set", tek_string_set },
	{ "sub", tek_string_sub },
	{ "get", tek_string_sub },
	{ "len", tek_string_len },
	{ "insert", tek_string_insert },
	{ "erase", tek_string_erase },
	{ "overwrite", tek_string_overwrite },
	{ "getval", tek_string_getval },
	{ "getchar", tek_string_getchar },
	{ "find", tek_string_find },
	{ "setmetadata", tek_string_setmetadata },

	{ "attachdata", tek_string_attachdata },
	{ "getdata", tek_string_getdata },
	
	{ "compact", tek_string_compact },
	{ "getVisualPos", tek_string_getvisualpos },
	{ "forEachChar", tek_string_foreachchar },
	{ "getTextWidth", tek_string_gettextwidth },
	
	{ NULL, NULL }
};

TMODENTRY int luaopen_tek_lib_string(lua_State *L)
{
	tek_lua_register(L, TEK_LIB_STRING_NAME, tek_string_funcs, 0);
	lua_pushstring(L, TEK_LIB_STRING_VERSION);
	lua_setfield(L, -2, "_VERSION");
	luaL_newmetatable(L, TEK_LIB_STRING_NAME "*");
	tek_lua_register(L, NULL, tek_string_methods, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
	return 1;
}
