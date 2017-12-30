
/*
**	teklib/src/display_dfb/display_dfb_font.c - DirectFB Display driver
**	Written by Franciska Schulze <fschulze at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/hal.h>

#include "display_dfb_mod.h"

/*****************************************************************************/

static TBOOL hostopenfont(DFBDISPLAY *mod, struct FontNode *fn,
	struct fnt_attr *fattr);
static void hostqueryfonts(DFBDISPLAY *mod, struct FontQueryHandle *fqh,
	struct fnt_attr *fattr);

/*****************************************************************************/
/* FontQueryHandle destructor
** free all memory associated with a fontqueryhandle including
** all fontquerynodes, a fontqueryhandle is obtained by calling
** dfb_hostqueryfonts()
*/
static THOOKENTRY TTAG
dfb_fqhdestroy(struct THook *hook, TAPTR obj, TTAG msg)
{
	if (msg == TMSG_DESTROY)
	{
		struct FontQueryHandle *fqh = obj;
		DFBDISPLAY *mod = fqh->handle.thn_Owner;
		TAPTR exec = TGetExecBase(mod);
		struct TNode *node, *next;

		node = fqh->reslist.tlh_Head.tln_Succ;
		for (; (next = node->tln_Succ); node = next)
		{
			struct FontQueryNode *fqn = (struct FontQueryNode *)node;

			/* remove from resultlist */
			TRemove(&fqn->node);

			/* destroy fontname */
			if (fqn->tags[0].tti_Value)
				TExecFree(exec, (TAPTR)fqn->tags[0].tti_Value);

			/* destroy node */
			TExecFree(exec, fqn);
		}

		/* destroy queryhandle */
		TExecFree(exec, fqh);
	}

	return 0;
}

/*****************************************************************************/
/* allocate a fontquerynode and fill in properties
*/
static struct FontQueryNode *
fnt_getfqnode(DFBDISPLAY *mod, TSTRPTR filename, TINT pxsize)
{
	TAPTR exec = TGetExecBase(mod);
	struct FontQueryNode *fqnode = TNULL;

	/* allocate fquery node */
	fqnode = TExecAlloc0(exec, mod->dfb_MemMgr, sizeof(struct FontQueryNode));
	if (fqnode)
	{
		/* fquerynode ready - fill in attributes */
		TSTRPTR myfname = TNULL;
		TINT flen = strlen(filename)-4; /* discard '.ttf' */

		if (flen > 0)
			myfname = TExecAlloc0(exec, mod->dfb_MemMgr, flen + 1);
		else
			TDBPRINTF(TDB_ERROR,("found invalid font: '%s'\n", filename));

		if (myfname)
		{
			TExecCopyMem(exec, filename, myfname, flen);
			fqnode->tags[0].tti_Tag = TVisual_FontName;
			fqnode->tags[0].tti_Value = (TTAG) myfname;
		}
		else
		{
			if (flen > 0)
				TDBPRINTF(TDB_FAIL,("out of memory :(\n"));
		}

		if (fqnode->tags[0].tti_Value)
		{
			TINT i = 1;

			fqnode->tags[i].tti_Tag = TVisual_FontPxSize;
			fqnode->tags[i++].tti_Value = (TTAG) pxsize;

			/* always true */
			fqnode->tags[i].tti_Tag = TVisual_FontScaleable;
			fqnode->tags[i++].tti_Value = (TTAG) TTRUE;

			fqnode->tags[i].tti_Tag = TTAG_DONE;
		}
		else
		{
			TExecFree(exec, fqnode);
			fqnode = TNULL;
		}

	} /* endif fqnode */
	else
		TDBPRINTF(TDB_FAIL,("out of memory :(\n"));

	return fqnode;
}

/*****************************************************************************/
/* check if a font with similar properties is already contained
** in our resultlist
*/
static TBOOL
fnt_checkfqnode(struct TList *rlist, struct FontQueryNode *fqnode)
{
	TUINT8 flags;
	TBOOL match = TFALSE;
	struct TNode *node, *next;
	TSTRPTR newfname = (TSTRPTR)fqnode->tags[0].tti_Value;
	TINT newpxsize = (TINT)fqnode->tags[1].tti_Value;

	/* not yet
	TBOOL newslant = (TBOOL)fqnode->tags[2].tti_Value;
	TBOOL newweight = (TBOOL)fqnode->tags[3].tti_Value;
	*/

	TSIZE flen = strlen(newfname);

	for (node = rlist->tlh_Head.tln_Succ; (next = node->tln_Succ); node = next)
	{
		struct FontQueryNode *fqn = (struct FontQueryNode *)node;
		flags = 0;

		if (strlen((TSTRPTR)fqn->tags[0].tti_Value) == flen)
		{
			if (strncmp((TSTRPTR)fqn->tags[0].tti_Value, newfname, flen) == 0)
				flags = FNT_MATCH_NAME;
		}

		if ((TINT)fqn->tags[1].tti_Value == newpxsize)
			flags |= FNT_MATCH_SIZE;

		/* not yet
		if ((TBOOL)fqn->tags[2].tti_Value == newslant)
			flags |= FNT_MATCH_SLANT;

		if ((TBOOL)fqn->tags[3].tti_Value == newweight)
			flags |= FNT_MATCH_WEIGHT;
		*/

		if (flags == FNT_MATCH_ALL)
		{
			/* fqnode is not unique */
			match = TTRUE;
			break;
		}
	}

	return match;
}

/*****************************************************************************/
/* dump properties of a fontquerynode
*/
static void
fnt_dumpnode(struct FontQueryNode *fqn)
{
	TDBPRINTF(10, ("-----------------------------------------------\n"));
	TDBPRINTF(10, ("dumping fontquerynode @ %p\n", fqn));
	TDBPRINTF(10, (" * FontName: %s\n", (TSTRPTR)fqn->tags[0].tti_Value));
	TDBPRINTF(10, (" * PxSize:   %d\n", (TINT)fqn->tags[1].tti_Value));
	//TDBPRINTF(10, (" * Italic:   %s\n", (TBOOL)fqn->tags[2].tti_Value ? "on" : "off"));
	//TDBPRINTF(10, (" * Bold:     %s\n", (TBOOL)fqn->tags[3].tti_Value ? "on" : "off"));
	TDBPRINTF(10, ("-----------------------------------------------\n"));
}

/*****************************************************************************/
/* dump all fontquerynodes of a (result-)list
*/
static void
fnt_dumplist(struct TList *rlist)
{
	struct TNode *node, *next;
	node = rlist->tlh_Head.tln_Succ;
	for (; (next = node->tln_Succ); node = next)
	{
		struct FontQueryNode *fqn = (struct FontQueryNode *)node;
		fnt_dumpnode(fqn);
	}
}

/*****************************************************************************/
/* parses a single fontname or a comma separated list of fontnames
** and returns a list of fontnames, spaces are NOT filtered, so
** "helvetica, fixed" will result in "helvetica" and " fixed"
*/
static void
fnt_getfnnodes(DFBDISPLAY *mod, struct TList *fnlist, TSTRPTR fname)
{
	TINT i, p = 0;
	TBOOL lastrun = TFALSE;
	TINT fnlen = strlen(fname);
	TAPTR exec = TGetExecBase(mod);

	for (i = 0; i < fnlen; i++)
	{
		if (i == fnlen-1) lastrun = TTRUE;

		if (fname[i] == ',' || lastrun)
		{
			TINT len = (i > p) ? (lastrun ? (i-p+1) : (i-p)) : fnlen+1;
			TSTRPTR ts = TExecAlloc0(exec, mod->dfb_MemMgr, len+1);

			if (ts)
			{
				struct fnt_node *fnn;

				TExecCopyMem(exec, fname+p, ts, len);

				fnn = TExecAlloc0(exec, mod->dfb_MemMgr, sizeof(struct fnt_node));
				if (fnn)
				{
					/* add fnnode to fnlist */
					fnn->fname = ts;
					TAddTail(fnlist, &fnn->node);
				}
				else
				{
					TDBPRINTF(TDB_FAIL,("out of memory :(\n"));
					break;
				}
			}
			else
			{
				TDBPRINTF(TDB_FAIL,("out of memory :(\n"));
				break;
			}

			p = i+1;
		}
	}
}

/*****************************************************************************/
/* examine filename according to the specified flags and set the
** corresponding bits in the flagfield the function returns
** at the moment only FNT_MATCH_NAME is suppported
*/
static TUINT
fnt_matchfont(DFBDISPLAY *mod, TSTRPTR filename, TSTRPTR fname,
	struct fnt_attr *fattr, TUINT flag)
{
	TUINT match = 0;
	TAPTR exec = TGetExecBase(mod);

	if (flag & FNT_MATCH_NAME)
	{
		TINT i;
		TINT len = strlen(fname);

		if (strncmp(fname, FNT_WILDCARD, len) == 0)
		{
			/* match all, but filter out invalid filenames like '.' or '..' */
			if (strlen(filename) > 4)
				match = FNT_MATCH_NAME;
		}
		else
		{
			TSTRPTR tempname = TNULL;

			/* convert fontnames to lower case */
			for (i = 0; i < len; i++)
				fname[i] = tolower(fname[i]);

			tempname = TExecAlloc0(exec, mod->dfb_MemMgr, len+1);
			if (!tempname)
			{
				TDBPRINTF(TDB_FAIL,("out of memory :(\n"));
				return -1;
			}

			for (i = 0; i < len; i++)
				tempname[i] = tolower(filename[i]);

			/* compare converted fontnames */
			if (strncmp(fname, tempname, len) == 0)
				match = FNT_MATCH_NAME;

			TExecFree(exec, tempname);
		}
	}

	/* not yet
	if (flag & FNT_MATCH_SLANT)		;
	if (flag & FNT_MATCH_WEIGHT)	;
	*/

	return match;
}

/*****************************************************************************/
/* CALL:
**	dfb_hostopenfont(visualbase, tags)
**
** USE:
**  to match and open exactly one font, according to its properties
**
** INPUT:
**	tag name				| description
**	------------------------+---------------------------
**	 TVisual_FontName		| font name
**   TVisual_FontPxSize		| font size in pixel
**
**	tag name				| default¹	| wildcard
**	------------------------+-----------+---------------
**	 TVisual_FontName		| "decker"	| "*"
**   TVisual_FontPxSize		| 14		|  /
**
** ¹ the defaults are used when the tag is missing
**
** RETURN:
** - a pointer to a font ready to be used or TNULL
**
** EXAMPLES:
** - to open the default font of your platform leave all tags empty
** - to open the default font in say 16px, set TVisual_FontPxSize to
**   16 and leave all other tags empty
**
** NOTES:
** - this function won't activate the font, use setfont() to make the
**   font the current active font
** - the function will open the first matching font
*/

LOCAL TAPTR
dfb_hostopenfont(DFBDISPLAY *mod, TTAGITEM *tags)
{
	struct fnt_attr fattr;
	struct FontNode *fn;
	TAPTR font = TNULL;
	TAPTR exec = TGetExecBase(mod);

	/* fetch user specified attributes */
	fattr.fname = (TSTRPTR) TGetTag(tags, TVisual_FontName, (TTAG) FNT_DEFNAME);
	fattr.fpxsize = (TINT) TGetTag(tags, TVisual_FontPxSize, (TTAG) FNT_DEFPXSIZE);

	/* not yet
	fattr.fitalic = (TBOOL) TGetTag(tags, TVisual_FontItalic, (TTAG) TFALSE);
	fattr.fbold = (TBOOL) TGetTag(tags, TVisual_FontBold, (TTAG) TFALSE);
	fattr.fscale = (TBOOL) TGetTag(tags, TVisual_FontScaleable, (TTAG) TFALSE);
	*/

	if (fattr.fname)
	{
		fn = TExecAlloc0(exec, mod->dfb_MemMgr, sizeof(struct FontNode));
		if (fn)
		{
			fn->handle.thn_Owner = mod;

			if (hostopenfont(mod, fn, &fattr))
			{
				/* load succeeded, save font attributes */
				fn->pxsize = fattr.fpxsize;
				fn->font->GetHeight(fn->font, &fn->height);
				fn->font->GetAscender(fn->font, &fn->ascent);
				fn->font->GetDescender(fn->font, &fn->descent);

				/* not yet
				if (fattr.fitalic)
					fn->attr = FNT_ITALIC;
				if (fattr.fbold)
					fn->attr |= FNT_BOLD;
				*/

				/* append to the list of open fonts */
				TDBPRINTF(TDB_INFO, ("O '%s' %dpx\n", fattr.fname, fattr.fpxsize));
				TAddTail(&mod->dfb_fm.openfonts, &fn->handle.thn_Node);
				font = (TAPTR)fn;
			}
			else
			{
				/* load failed, free fontnode */
				TDBPRINTF(TDB_INFO,("X unable to load '%s'\n", fattr.fname));
				TExecFree(exec, fn);
			}
		}
		else
			TDBPRINTF(TDB_FAIL,("out of memory :(\n"));
	}
	else
		TDBPRINTF(TDB_ERROR,("X invalid fontname '%s' specified\n", fattr.fname));

	return font;
}

static TBOOL
hostopenfont(DFBDISPLAY *mod, struct FontNode *fn, struct fnt_attr *fattr)
{
	TBOOL succ = TFALSE;
	TAPTR exec = TGetExecBase(mod);
	DFBFontDescription fdsc;
	TSTRPTR fontfile = TExecAlloc0(exec, mod->dfb_MemMgr,
		strlen(fattr->fname) + strlen(DEF_FONTDIR) + 5);

	if (fontfile)
	{
		fdsc.flags = DFDESC_HEIGHT;
		fdsc.height = fattr->fpxsize;

		sprintf(fontfile, "%s%s.ttf", DEF_FONTDIR, fattr->fname);
		TDBPRINTF(TDB_INFO,("? %s:%d\n", fontfile, fattr->fpxsize));

		if (mod->dfb_DFB->CreateFont(mod->dfb_DFB, fontfile,
			&fdsc, &fn->font) == DFB_OK)
			succ = TTRUE;

		TExecFree(exec, fontfile);
	}
	else
		TDBPRINTF(TDB_FAIL,("out of memory :(\n"));

	return succ;
}

/*****************************************************************************/
/* CALL:
**  dfb_hostqueryfonts(visualbase, tags)
**
** USE:
**  to match one or more fonts, according to their properties
**
** INPUT:
**	tag name				| description
**	------------------------+---------------------------
**	 TVisual_FontName		| font name
**   TVisual_FontPxSize		| font size in pixel
**	 TVisual_FontNumResults	| how many fonts to return
**
**	tag name				| default¹
**	------------------------+----------------------------
**	 TVisual_FontName		| FNTQUERY_UNDEFINED
**   TVisual_FontPxSize		| FNTQUERY_UNDEFINED
**	 TVisual_FontNumResults	| INT_MAX
**
** ¹ the defaults are used when the tag is missing
**
** RETURN:
** - a pointer to a FontQueryHandle, which is basically a list of
**   taglists, referring to the fonts matched
** - use dfb_hostgetnextfont() to traverse the list
** - use TDestroy to free all memory associated with a FontQueryHandle
**
** EXAMPLES:
** - to match all available fonts, use an empty taglist
** - to match more than one specific font, use a coma separated list
**   for TVisual_FontName, e.g. "helvetica,utopia,fixed", note that
**   spaces are not filtered
**
** NOTES:
** - this function won't open any fonts, to do so use dfb_hostopenfont()
*/

LOCAL TAPTR
dfb_hostqueryfonts(DFBDISPLAY *mod, TTAGITEM *tags)
{
	TSTRPTR fname = TNULL;
	struct fnt_attr fattr;
	struct TNode *node, *next;
	struct FontQueryHandle *fqh = TNULL;
	TAPTR exec = TGetExecBase(mod);

	/* init fontname list */
	TInitList(&fattr.fnlist);

	/* fetch and parse fname */
	fname = (TSTRPTR) TGetTag(tags, TVisual_FontName, (TTAG) FNT_WILDCARD);
	if (fname)	fnt_getfnnodes(mod, &fattr.fnlist, fname);

	/* fetch user specified attributes */
	fattr.fpxsize = (TINT) TGetTag(tags, TVisual_FontPxSize, (TTAG) FNT_DEFPXSIZE);
	fattr.fnum = (TINT) TGetTag(tags, TVisual_FontNumResults, (TTAG) INT_MAX);
	/* not yet
	fattr.fitalic = (TBOOL) TGetTag(tags, TVisual_FontItalic, (TTAG) FNTQUERY_UNDEFINED);
	fattr.fbold = (TBOOL) TGetTag(tags, TVisual_FontBold, (TTAG) FNTQUERY_UNDEFINED);
	*/

	/* init result list */
	fqh = TExecAlloc0(exec, mod->dfb_MemMgr, sizeof(struct FontQueryHandle));
	if (fqh)
	{
		fqh->handle.thn_Owner = mod;
		/* connect destructor */
		TInitHook(&fqh->handle.thn_Hook, dfb_fqhdestroy, fqh);
		TInitList(&fqh->reslist);
		/* init list iterator */
		fqh->nptr = &fqh->reslist.tlh_Head.tln_Succ;

		hostqueryfonts(mod, fqh, &fattr);
		TDB(TDB_INFO,(fnt_dumplist(&fqh->reslist)));
	}
	else
		TDBPRINTF(TDB_FAIL,("out of memory :(\n"));

	/* free memory of fnt_nodes */
	for (node = fattr.fnlist.tlh_Head.tln_Succ; (next = node->tln_Succ); node = next)
	{
		struct fnt_node *fnn = (struct fnt_node *)node;
		TExecFree(exec, fnn->fname);
		TRemove(&fnn->node);
		TExecFree(exec, fnn);
	}

	return fqh;
}

static void
hostqueryfonts(DFBDISPLAY *mod, struct FontQueryHandle *fqh, struct fnt_attr *fattr)
{
	TINT i, nfont, fcount = 0;
	struct TNode *node, *next;
	TAPTR exec = TGetExecBase(mod);
	struct dirent **dirlist;
	TUINT matchflg = 0;

	/* scan default font directory */
	nfont = scandir(DEF_FONTDIR, &dirlist, 0, alphasort);
	if (nfont < 0)
	{
		perror("scandir");
		return;
	}

	if (nfont > 0)
	{
		/* found fonts in default font directory */
		for (node = fattr->fnlist.tlh_Head.tln_Succ; (next = node->tln_Succ); node = next)
		{
			struct fnt_node *fnn = (struct fnt_node *)node;

			/* build matchflag, font pxsize attribute is ignored,
			   because it's not relevant when matching ttf fonts */

			matchflg = FNT_MATCH_NAME;

			/* not yet
			if (fattr->fitalic != FNTQUERY_UNDEFINED)
				matchflg |= FNT_MATCH_SLANT;
			if (fattr->fbold != FNTQUERY_UNDEFINED)
				matchflg |= FNT_MATCH_WEIGHT;
			*/

			for (i = 0; i < nfont; i++)
			{
				if (fnt_matchfont(mod, dirlist[i]->d_name, fnn->fname,
					fattr, matchflg) == matchflg)
				{
					struct FontQueryNode *fqnode;

					/* create fqnode and fill in attributes */
					fqnode = fnt_getfqnode(mod, dirlist[i]->d_name, fattr->fpxsize);
					if (!fqnode)	break;

					/* compare fqnode with nodes in result list */
					if (fnt_checkfqnode(&fqh->reslist, fqnode) == 0)
					{
						if (fcount < fattr->fnum)
						{
							/* fqnode is unique, add to result list */
							TAddTail(&fqh->reslist, &fqnode->node);
							fcount++;
						}
						else
						{
							/* max count of desired results reached */
							TExecFree(exec, (TSTRPTR)fqnode->tags[0].tti_Value);
							TExecFree(exec, fqnode);
							break;
						}
					}
					else
					{
						/* fqnode is not unique, destroy it */
						TDBPRINTF(TDB_ERROR,("X node is not unique\n"));
						TExecFree(exec, (TSTRPTR)fqnode->tags[0].tti_Value);
						TExecFree(exec, fqnode);
					}
				}
			}

			if (fcount == fattr->fnum)
				break;

		} /* end of fnlist iteration */

	} /* endif fonts found */
	else
		TDBPRINTF(TDB_WARN,("X no fonts found in '%s'\n", DEF_FONTDIR));

	while (nfont--)
		free(dirlist[nfont]);
	free(dirlist);
}

/*****************************************************************************/
/* CALL:
**  dfb_hostsetfont(visual, fontpointer)
**
** USE:
**  makes the font referred to by fontpointer the current active font
**  for the visual
**
** INPUT:
**  a pointer to a font returned by dfb_hostopenfont()
**
** NOTES:
** - if a font is active it can't be closed
*/

LOCAL void
dfb_hostsetfont(DFBDISPLAY *mod, DFBWINDOW *v, TAPTR font)
{
	if (font)
	{
		struct FontNode *fn = (struct FontNode *) font;
		v->winsurface->SetFont(v->winsurface, fn->font);
		v->curfont = font;
	}
	else
		TDBPRINTF(TDB_ERROR,("invalid font specified\n"));
}

/*****************************************************************************/
/* CALL:
**  dfb_hostgetnextfont(visualbase, fontqueryhandle)
**
** USE:
**  iterates a list of taglists, returning the next taglist
**  pointer or TNULL
**
** INPUT:
**  a fontqueryhandle obtained by calling dfb_hostqueryfonts()
**
** RETURN:
**  a pointer to a taglist, representing a font or TNULL
**
** NOTES:
**  - the taglist returned by this function can be directly fed to
**	  dfb_hostopenfont()
**  - if the end of the list is reached, TNULL is returned and the
**    iterator is reset to the head of the list
*/

LOCAL TTAGITEM *
dfb_hostgetnextfont(DFBDISPLAY *mod, TAPTR fqhandle)
{
	struct FontQueryHandle *fqh = fqhandle;
	struct TNode *next = *fqh->nptr;

	if (next->tln_Succ == TNULL)
	{
		fqh->nptr = &fqh->reslist.tlh_Head.tln_Succ;
		return TNULL;
	}

	fqh->nptr = (struct TNode **)next;
	return ((struct FontQueryNode *)next)->tags;
}

/*****************************************************************************/
/* CALL:
**  dfb_hostclosefont(visualbase, fontpointer)
**
** USE:
**  attempts to free all memory associated with the font referred to
**  by fontpointer
**
** INPUT:
**  a pointer to a font returned by dfb_hostopenfont()
**
** NOTES:
**  - the default font is only freed, if there are no more references
**	  to it left
**  - the attempt to free any other font which is currently in use,
**	  will be ignored
*/

LOCAL void
dfb_hostclosefont(DFBDISPLAY *mod, TAPTR font)
{
	struct FontNode *fn = (struct FontNode *) font;
	TAPTR exec = TGetExecBase(mod);

	if (font == mod->dfb_fm.deffont)
	{
		if (mod->dfb_fm.defref)
		{
			/* prevent freeing of default font if it's */
			/* still referenced */
			mod->dfb_fm.defref--;
			return;
		}
	}

	/* free dfbfont */
	if (fn->font)
	{
		fn->font->Release(fn->font);
		fn->font = TNULL;
	}

	/* remove font from openfonts list */
	TRemove(&fn->handle.thn_Node);

	/* free fontnode itself */
	TExecFree(exec, fn);
}

/*****************************************************************************/
/* CALL:
**  dfb_hosttextsize(visualbase, fontpointer, textstring, numchars)
**
** USE:
**  to obtain the width of a given string when the font referred to
**  by fontpointer is used to render the text
**
** INPUT:
**  - a pointer to a font returned by dfb_hostopenfont()
**  - the textstring to measure
**
** RETURN:
**  - the width of the textstring in pixel
*/

LOCAL TINT
dfb_hosttextsize(DFBDISPLAY *mod, TAPTR font, TSTRPTR text, size_t len)
{
	TINT width = 0;
	struct FontNode *fn = (struct FontNode *) font;
	fn->font->GetStringWidth(fn->font, text, len, &width);
	return width;
}

/*****************************************************************************/
/* CALL:
**	dfb_getfattrs(visualbase, fontpointer, taglist);
**
** USE:
**  fills the taglist with the requested properties of the
**  font referred to by fontpointer
**
** INPUT:
**  - a pointer to a font returned by dfb_hostopenfont()
**	- the following tags can be used
**
**  tag name				| description
**	------------------------+---------------------------
**  TVisual_FontPxSize		| font size in pixel
** 	TVisual_FontAscent		| the font ascent in pixel
**	TVisual_FontDescent		| the font descent in pixel
**  TVisual_FontHeight		| height in pixel
**	TVisual_FontUlPosition	| position of an underline
**	TVisual_FontUlThickness	| thickness of the underline
**
** RETURN:
**  - the number of processed properties
**
** NOTES:
**  - TVisual_FontUlPosition defaults to fontdescent / 2
**	  and TVisual_FontUlThickness defaults to 1
*/

LOCAL THOOKENTRY TTAG
dfb_hostgetfattrfunc(struct THook *hook, TAPTR obj, TTAG msg)
{
	struct attrdata *data = hook->thk_Data;
	TTAGITEM *item = obj;
	struct FontNode *fn = (struct FontNode *) data->font;

	switch (item->tti_Tag)
	{
		default:
			return TTRUE;

		case TVisual_FontPxSize:
			*((TINT *) item->tti_Value) = fn->pxsize;
			break;

		/* not yet
		case TVisual_FontItalic:
			*((TINT *) item->tti_Value) = (fn->attr & FNT_ITALIC) ?
				TTRUE : TFALSE;
			break;

		case TVisual_FontBold:
			*((TINT *) item->tti_Value) = (fn->attr & FNT_BOLD) ?
				TTRUE : TFALSE;
			break;
		*/
		case TVisual_FontAscent:
			*((TINT *) item->tti_Value) = fn->ascent;
			break;

		case TVisual_FontDescent:
			*((TINT *) item->tti_Value) = fn->descent;
			break;

		case TVisual_FontHeight:
			*((TINT *) item->tti_Value) = fn->height;
			break;

		case TVisual_FontUlPosition:
		{
			*((TINT *) item->tti_Value) = fn->descent / 2;
			break;
		}
		case TVisual_FontUlThickness:
		{
			*((TINT *) item->tti_Value) = 1;
			break;
		}

		/* ... */
	}
	data->num++;
	return TTRUE;
}

/*****************************************************************************/
