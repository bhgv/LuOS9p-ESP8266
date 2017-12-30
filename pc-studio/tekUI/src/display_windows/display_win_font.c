
/*
**	display_win_font.c - Windows display driver
**	Written by Timm S. Mueller <tmueller@schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <ctype.h>
#include <limits.h>

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/hal.h>

#include "display_win_mod.h"

/*****************************************************************************/

LOCAL TAPTR fb_hostopenfont(WINDISPLAY *mod, TTAGITEM *tags)
{
	TSTRPTR name = (TSTRPTR) TGetTag(tags, TVisual_FontName, TNULL);
	TUINT sz = (TUINT) TGetTag(tags, TVisual_FontPxSize, 0);
	HFONT font;
	TBOOL italic = (TBOOL) TGetTag(tags, TVisual_FontItalic, (TTAG) TFALSE);
	TBOOL bold = (TBOOL) TGetTag(tags, TVisual_FontBold, (TTAG) TFALSE);

	/* check availability: */
	TAPTR fqh = fb_hostqueryfonts(mod, tags);
	if (fqh)
	{
		TBOOL have_font = fb_hostgetnextfont(mod, fqh) != TNULL;
		TDestroy(fqh);
		if (!have_font)
		{
			TDBPRINTF(TDB_INFO,("Failed to open font '%s':%d\n", name, sz));
			return TNULL;
		}
	}
	else
		return TNULL;
	
	font = CreateFont(
		sz,
		0,
		0,
		0,
		bold ? FW_BOLD : FW_NORMAL,
		italic,
		0,				/* underline */
		0,				/* strikeout */
		ANSI_CHARSET,	/* charset */
		OUT_DEFAULT_PRECIS,	/* precision */
		CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		FF_DONTCARE | DEFAULT_PITCH,
		name);

	TDBPRINTF(TDB_INFO,("CreateFont %s:%d => %p\n", name, sz, font));

	return font;
}


LOCAL void fb_hostclosefont(WINDISPLAY *mod, TAPTR font)
{
	DeleteObject(font);
}


LOCAL void fb_hostsetfont(WINDISPLAY *mod, WINWINDOW *v, TAPTR font)
{
	SelectObject(v->fbv_HDC, font);
	v->fbv_CurrentFont = font;
}


LOCAL TTAGITEM *fb_hostgetnextfont(WINDISPLAY *mod, struct FontQueryHandle *fqh)
{
	struct TNode *next = *fqh->nptr;
	TDBPRINTF(TDB_INFO,("getnextfont\n"));
	if (next->tln_Succ == TNULL)
	{
		fqh->nptr = &fqh->reslist.tlh_Head.tln_Succ;
		return TNULL;
	}
	fqh->nptr = (struct TNode **)next;
	return ((struct FontQueryNode *)next)->tags;
}


LOCAL TINT fb_hosttextsize(WINDISPLAY *mod, TAPTR font, TSTRPTR text, TSIZE len)
{
	SIZE sz;
	SelectObject(mod->fbd_DeviceHDC, font);
	int clen = utf8getlen((const unsigned char *) text, len);
	TUINT16 wstr[2048];
	MultiByteToWideChar(CP_UTF8, 0, (LPCSTR) text, len, (LPWSTR) &wstr, 2048);
	GetTextExtentPoint32W(mod->fbd_DeviceHDC, wstr, clen, &sz);
	return sz.cx;
}


static THOOKENTRY TTAG
fb_fqhdestroy(struct THook *hook, TAPTR obj, TTAG msg)
{
	if (msg == TMSG_DESTROY)
	{
		struct FontQueryHandle *fqh = obj;
		WINDISPLAY *mod = fqh->handle.thn_Owner;
		TAPTR TExecBase = TGetExecBase(mod);
		struct TNode *node, *next;
		node = fqh->reslist.tlh_Head.tln_Succ;
		for (; (next = node->tln_Succ); node = next)
		{
			struct FontQueryNode *fqn = (struct FontQueryNode *)node;
			TRemove(&fqn->node);
			TFree(fqn);
		}
	}
	return 0;
}


static int CALLBACK fontenumcb(
	ENUMLOGFONTEX *lpelfe,
	NEWTEXTMETRICEX *lpntme,
	int fonttype,
	LPARAM param)
{
	LOGFONT *lf = &lpelfe->elfLogFont;
	TSTRPTR fullname = (TSTRPTR) lpelfe->elfFullName;
	TSTRPTR style = (TSTRPTR) lpelfe->elfStyle;
	TSTRPTR script = (TSTRPTR) lpelfe->elfScript;
	struct FontQueryHandle *fqh = (struct FontQueryHandle *) param;
	WINDISPLAY *mod = fqh->handle.thn_Owner;
	struct TExecBase *TExecBase = TGetExecBase(mod);
	struct FontQueryNode *fqn;

	if (fqh->match_depth == 0)
	{
		LOGFONT logfont;
		memcpy(&logfont, lf, sizeof(LOGFONT));
		fqh->match_depth = 1;
		TDBPRINTF(TDB_INFO,("fontenum - entering recursion\n"));
		logfont.lfCharSet = ANSI_CHARSET;
		logfont.lfPitchAndFamily = 0;
		EnumFontFamiliesEx(
			mod->fbd_DeviceHDC,
			&logfont,
			(FONTENUMPROCA) fontenumcb,
			(LPARAM) fqh, 0);
		fqh->match_depth = 0;
		return TTRUE;
	}


	if (fqh->fscale && !(fonttype & TRUETYPE_FONTTYPE))
		return TTRUE;
	if (fqh->fitalic && !lf->lfItalic)
		return TTRUE;
	if (fqh->fbold && (lf->lfWeight < FW_BOLD))
		return TTRUE;

	TDBPRINTF(TDB_INFO,("entry:'%s' style:'%s' script:'%s' type:%04x weight:%d italic:%d recurs:%d, size:%d\n",
		fullname, style, script, fonttype, lf->lfWeight, !!lf->lfItalic,
		fqh->match_depth, lf->lfHeight));

	fqn = TAlloc(TNULL,
		sizeof(struct FontQueryNode) + strlen(fullname) + 1);
	if (fqn == TNULL)
		return TFALSE;

	strcpy((char *) (fqn + 1), fullname);
	fqn->tags[0].tti_Tag = TVisual_FontName;
	fqn->tags[0].tti_Value = (TTAG) (fqn + 1);
	fqn->tags[1].tti_Tag = TVisual_FontItalic;
	fqn->tags[1].tti_Value = !!lf->lfItalic;
	fqn->tags[2].tti_Tag = TVisual_FontBold;
	fqn->tags[2].tti_Value = (lf->lfWeight >= FW_BOLD);
	fqn->tags[3].tti_Tag = TTAG_DONE;

	TAddTail(&fqh->reslist, &fqn->node);
	return TTRUE;
}


LOCAL TAPTR fb_hostqueryfonts(WINDISPLAY *mod, TTAGITEM *tags)
{
	struct TExecBase *TExecBase = TGetExecBase(mod);
	TSTRPTR names;
	LOGFONT logfont;
	char *namebuf = logfont.lfFaceName;
	struct FontQueryHandle *fqh;

	fqh = TAlloc0(TNULL, sizeof(struct FontQueryHandle));
	if (fqh == TNULL)
		return TNULL;

	fqh->handle.thn_Owner = mod;
	TInitHook(&fqh->handle.thn_Hook, fb_fqhdestroy, fqh);
	TInitList(&fqh->reslist);

	fqh->fpxsize = (TINT) TGetTag(tags, TVisual_FontPxSize, (TTAG) FNTQUERY_UNDEFINED);
	fqh->fitalic = (TBOOL) TGetTag(tags, TVisual_FontItalic, (TTAG) FNTQUERY_UNDEFINED);
	fqh->fbold = (TBOOL) TGetTag(tags, TVisual_FontBold, (TTAG) FNTQUERY_UNDEFINED);
	fqh->fscale = (TBOOL) TGetTag(tags, TVisual_FontScaleable, (TTAG) FNTQUERY_UNDEFINED);
	fqh->fnum = (TINT) TGetTag(tags, TVisual_FontNumResults, (TTAG) INT_MAX);
	fqh->success = TTRUE;

	names = (TSTRPTR) TGetTag(tags, TVisual_FontName, TNULL);
	if (names == TNULL)
		names = "";

	for (;;)
	{
		TSTRPTR p = strchr(names, ',');
		if (p)
		{
			size_t len2 = TMIN(p - names, LF_FACESIZE - 1);
			memcpy(namebuf, names, len2);
			namebuf[len2] = 0;
			names += p - names + 1;
		}
		else
		{
			size_t len = strlen(names);
			size_t len2 = TMIN(len, LF_FACESIZE - 1);
			memcpy(namebuf, names, len2);
			namebuf[len2] = 0;
			names += len;
		}

		logfont.lfCharSet = ANSI_CHARSET;
		logfont.lfPitchAndFamily = 0;

		TDBPRINTF(TDB_INFO,("query font: %s\n", namebuf));

		if (strcmp(namebuf, "") == 0)
			fqh->match_depth = 0; /* find font first */
		else
			fqh->match_depth = 1; /* find style */

		EnumFontFamiliesEx(
			mod->fbd_DeviceHDC,
			&logfont,
			(FONTENUMPROCA) fontenumcb,
			(LPARAM) fqh, 0);

		if (strlen(names) == 0)
			break;
	}

	fqh->nptr = &fqh->reslist.tlh_Head.tln_Succ;
	if (fqh->success)
		return fqh;

	TDestroy(&fqh->handle);
	return TNULL;
}


LOCAL THOOKENTRY TTAG fb_hostgetfattrfunc(struct THook *hook, TAPTR obj,
	TTAG msg)
{
	struct attrdata *data = hook->thk_Data;
	TEXTMETRIC *tm = &data->textmetric;
	TTAGITEM *item = obj;
	TTAG *valp = (TTAG *) item->tti_Value;

	switch (item->tti_Tag)
	{
		default:
			return TTRUE;
		case TVisual_FontPxSize:
		case TVisual_FontHeight:
			*valp = tm->tmHeight;
			break;
		case TVisual_FontItalic:
			*valp = tm->tmItalic;
			break;
		case TVisual_FontBold:
			*valp = tm->tmWeight >= 0x400;
			break;
		case TVisual_FontAscent:
			*valp = tm->tmAscent;
			break;
		case TVisual_FontDescent:
			*valp = tm->tmDescent;
			break;
		case TVisual_FontUlPosition:
			*valp = tm->tmDescent;
			break;
		case TVisual_FontUlThickness:
			*valp = 1;
			break;
	}

	data->num++;
	return TTRUE;
}
