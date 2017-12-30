/*
* lascii85.c
* ascii85 encoding and decoding for Lua 5.2
* Luiz Henrique de Figueiredo <lhf@tecgraf.puc-rio.br>
* 27 Sep 2012 19:36:45
* This code is hereby placed in the public domain.
*/

#include <string.h>

#include "lua.h"
#include "lauxlib.h"

#define MYNAME		"ascii85"
#define MYVERSION	MYNAME " library for " LUA_VERSION " / Sep 2012"

#define uint unsigned int

static void encode(luaL_Buffer *b, uint c1, uint c2, uint c3, uint c4, int n)
{
 unsigned long tuple=c4+256UL*(c3+256UL*(c2+256UL*c1));
 if (tuple==0 && n==4)
  luaL_addlstring(b,"z",1);
 else
 {
  int i;
  char s[5];
  for (i=0; i<5; i++) {
   s[4-i] = '!' + (tuple % 85);
   tuple /= 85;
  }
  luaL_addlstring(b,s,n+1);
 }
}

static int Lencode(lua_State *L)		/** encode(s) */
{
 size_t l;
 const unsigned char *s=(const unsigned char*)luaL_checklstring(L,1,&l);
 luaL_Buffer b;
 int n;
 luaL_buffinit(L,&b);
 luaL_addlstring(&b,"<~",2);
 for (n=l/4; n--; s+=4) encode(&b,s[0],s[1],s[2],s[3],4);
 switch (l%4)
 {
  case 1: encode(&b,s[0],0,0,0,1);		break;
  case 2: encode(&b,s[0],s[1],0,0,2);		break;
  case 3: encode(&b,s[0],s[1],s[2],0,3);	break;
 }
 luaL_addlstring(&b,"~>",2);
 luaL_pushresult(&b);
 return 1;
}

static void decode(luaL_Buffer *b, int c1, int c2, int c3, int c4, int c5, int n)
{
 unsigned long tuple=c5+85L*(c4+85L*(c3+85L*(c2+85L*c1)));
 char s[4];
 switch (--n)
 {
  case 4: s[3]=tuple;
  case 3: s[2]=tuple >> 8;
  case 2: s[1]=tuple >> 16;
  case 1: s[0]=tuple >> 24;
 }
 luaL_addlstring(b,s,n);
}

static int Ldecode(lua_State *L)		/** decode(s) */
{
 size_t l;
 const char *s=luaL_checklstring(L,1,&l);
 luaL_Buffer b;
 int n=0;
 char t[5];
 if (*s++!='<') return 0; 
 if (*s++!='~') return 0; 
 luaL_buffinit(L,&b);
 for (;;)
 {
  int c=*s++;
  switch (c)
  {
   default:
    if (c<'!' || c>'u') return 0;
    t[n++]=c-'!';
    if (n==5)
    {
     decode(&b,t[0],t[1],t[2],t[3],t[4],5);
     n=0;
    }
    break;
   case 'z':
    luaL_addlstring(&b,"\0\0\0\0",4);
    break;
   case '~':
    if (*s!='>') return 0;
    switch (n)
    {
     case 1: decode(&b,t[0],85,0,0,0,1);		break;
     case 2: decode(&b,t[0],t[1],85,0,0,2);		break;
     case 3: decode(&b,t[0],t[1],t[2],85,0,3);		break;
     case 4: decode(&b,t[0],t[1],t[2],t[3],85,4);	break;
    }
    luaL_pushresult(&b);
    return 1;
   case '\n': case '\r': case '\t': case ' ':
   case '\0': case '\f': case '\b': case 0177:
    break;
  }
 }
 return 0;
}

static const luaL_Reg R[] =
{
	{ "encode",	Lencode	},
	{ "decode",	Ldecode	},
	{ NULL,		NULL	}
};

LUALIB_API int luaopen_ascii85(lua_State *L)
{
 luaL_newlib(L,R);
 lua_pushliteral(L,"version");			/** version */
 lua_pushliteral(L,MYVERSION);
 lua_settable(L,-3);
 return 1;
}
