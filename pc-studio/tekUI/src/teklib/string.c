#ifndef _TEK_STRING_C
#define _TEK_STRING_C

/*
**	teklib/src/teklib/string.c - string library
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/string.h>

/*****************************************************************************/
/*
**	len = TStrLen(string)
**	Return length of a string
*/

TLIBAPI TSIZE TStrLen(TSTRPTR s)
{
	TSTRPTR p;
	if (s == TNULL)
		return 0;
	p = s;
	while (*p) p++;
	return p - s;
}

/*****************************************************************************/
/*
**	p = TStrCpy(dest, source)
**	Copy string
*/

TLIBAPI TSTRPTR TStrCpy(TSTRPTR d, TSTRPTR s)
{
	if (s && d)
	{
		TSTRPTR p = d;
		while ((*d++ = *s++));
		return p;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	p = TStrNCpy(dest, source, maxlen)
**	Copy string, length-limited
*/

TLIBAPI TSTRPTR TStrNCpy(TSTRPTR d, TSTRPTR s, TSIZE maxl)
{
	if (s && d)
	{
		TSTRPTR p = d;
		TSIZE i;
		TINT c = 1;
		for (i = 0; i < maxl; ++i)
		{
			if (c)
				c = *s++;
			*d++ = c;
		}
		return p;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	p = TStrCat(dest, add)
**	Concatenate strings
*/

TLIBAPI TSTRPTR TStrCat(TSTRPTR d, TSTRPTR s)
{
	if (s && d)
	{
		TSTRPTR p = d;
		while (*d) d++;
		while ((*d++ = *s++));
		return p;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	p = TStrNCat(dest, add, maxl)
**	Concatenate strings, length-limited
*/

TLIBAPI TSTRPTR TStrNCat(TSTRPTR d, TSTRPTR s, TSIZE maxl)
{
	if (s && d)
	{
		TSTRPTR p = d;
		while (*d) d++;
		while (maxl-- && (*d++ = *s++));
		*d = 0;
		return p;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	result = TStrCmp(string1, string2)
**	Compare strings, with slightly extended semantics: Either or both
**	strings may be TNULL pointers. a TNULL string is 'less than' a
**	non-TNULL string.
*/

TLIBAPI TINT TStrCmp(TSTRPTR s1, TSTRPTR s2)
{
	if (s1)
	{
		if (s2)
		{
			TINT t1 = *s1, t2 = *s2;
			TINT c1, c2;

			do
			{
				if ((c1 = t1)) t1 = *s1++;
				if ((c2 = t2)) t2 = *s2++;

				if (!c1 || !c2) break;

			} while (c1 == c2);

			return ((TINT) c1 - (TINT) c2);
		}

		return 1;
	}

	if (s2) return -1;

	return 0;
}

/*****************************************************************************/
/*
**	result = TStrNCmp(string1, string2, count)
**	Compare strings, length-limited, with slightly extended semantics:
**	either or both strings may be TNULL pointers. a TNULL string is
**	'less than' a non-TNULL string.
*/

TLIBAPI TINT TStrNCmp(TSTRPTR s1, TSTRPTR s2, TSIZE count)
{
	if (s1)
	{
		if (s2)
		{
			TINT t1 = *s1, t2 = *s2;
			TINT c1, c2;
			do
			{
				if ((c1 = t1)) t1 = *s1++;
				if ((c2 = t2)) t2 = *s2++;
				if (!c1 || !c2) break;
			} while (count-- && c1 == c2);
			return ((TINT) c1 - (TINT) c2);
		}
		return 1;
	}
	if (s2)
		return -1;
	return 0;
}

/*****************************************************************************/
/*
**	result = TStrCaseCmp(string1, string2)
**	Compare strings without regard to case, with slightly extended
**	semantics: Either or both strings may be TNULL pointers. a TNULL
**	string is 'less than' a non-TNULL string.
*/

TLIBAPI TINT TStrCaseCmp(TSTRPTR s1, TSTRPTR s2)
{
	if (s1)
	{
		if (s2)
		{
			TINT t1 = *s1, t2 = *s2;
			TINT c1, c2;

			if (t1 >= 'a' && t1 <= 'z') t1 -= 'a' - 'A';
			if (t2 >= 'a' && t2 <= 'z') t2 -= 'a' - 'A';

			do
			{
				if ((c1 = t1))
				{
					t1 = *s1++;
					if (t1 >= 'a' && t1 <= 'z') t1 -= 'a' - 'A';
				}

				if ((c2 = t2))
				{
					t2 = *s2++;
					if (t2 >= 'a' && t2 <= 'z') t2 -= 'a' - 'A';
				}

				if (!c1 || !c2) break;

			} while (c1 == c2);

			return c1 - c2;
		}

		return 1;
	}

	if (s2) return -1;

	return 0;
}

/*****************************************************************************/
/*
**	result = TStrNCaseCmp(string1, string2, count)
**	Compare characters of strings without regard to case, length-limited,
**	with slightly extended semantics: Either or both strings may be TNULL
**	pointers. a TNULL string is 'less than' a non-TNULL string.
*/

TLIBAPI TINT TStrNCaseCmp(TSTRPTR s1, TSTRPTR s2, TSIZE count)
{
	if (s1)
	{
		if (s2)
		{
			TINT t1 = *s1, t2 = *s2;
			TINT c1, c2;
			if (t1 >= 'a' && t1 <= 'z') t1 -= 'a' - 'A';
			if (t2 >= 'a' && t2 <= 'z') t2 -= 'a' - 'A';
			do
			{
				if ((c1 = t1))
				{
					t1 = *s1++;
					if (t1 >= 'a' && t1 <= 'z') t1 -= 'a' - 'A';
				}
				if ((c2 = t2))
				{
					t2 = *s2++;
					if (t2 >= 'a' && t2 <= 'z') t2 -= 'a' - 'A';
				}
				if (!c1 || !c2) break;
			} while (count-- && c1 == c2);
			return c1 - c2;
		}
		return 1;
	}
	if (s2) return -1;
	return 0;
}

/*****************************************************************************/
/*
**	strptr = TStrStr(s1, s2)
**	Find substring s2 in string s1
*/

TLIBAPI TSTRPTR TStrStr(TSTRPTR s1, TSTRPTR s2)
{
	TINT c, d, x;

	if (s1 == TNULL)
		return TNULL;
	if (s2 == TNULL)
		return (TSTRPTR) s1;

	x = 0;
	d = *s2;
	while ((c = *s1++))
	{
		if (c != d)
		{
			d = *s2;
			x = 0;
		}

		if (c == d)
		{
			d = s2[++x];
			if (d == 0)
				return (TSTRPTR) s1 - x;
		}
	}

	return TNULL;
}

/*****************************************************************************/
/*
**	strptr = TStrChr(string, char)
**	Find a character in a string
*/

TLIBAPI TSTRPTR TStrChr(TSTRPTR s, TINT c)
{
	TINT d;
	while ((d = *s))
	{
		if (d == c)
			return (TSTRPTR) s;
		s++;
	}
	return TNULL;
}

/*****************************************************************************/
/*
**	strptr = TStrRChr(string, char)
**	Find a character in a string, reverse
*/

TLIBAPI TSTRPTR TStrRChr(TSTRPTR s, TINT c)
{
	TSTRPTR s2;
	TSIZE l = TStrLen(s);
	TBOOL suc = TFALSE;

	s2 = s + l - 1;
	while (!suc && l)
	{
		if (*s2 == c)
			suc = TTRUE;
		else
		{
			s2--;
			l--;
		}
	}

	if (suc)
		return (TSTRPTR) s2;
	else
		return TNULL;
}

/*****************************************************************************/
/*
**	numchars = TStrToI(s, &valp)
**	Get integer from ASCII (signed)
*/

TLIBAPI TINT TStrToI(TSTRPTR s, TINT *valp)
{
	TSTRPTR p = s;
	TINT n = 0;
	TINT c;
	TBOOL neg = 0;

	if (!valp) return -1;
	if (!s) goto error;

	for (;;)
	{
		c = *s++;
		switch (c)
		{
			case '-':
				neg ^= 1;
			case 9:
			case 10:
			case 13:
			case 32:
			case '+':
				continue;
		}

		c -= '0';
		if (c >= 0 && c <= 9) break;

error:	*valp = 0;
		return -1;
	}

	for (;;)
	{
		if (n < 214748364 || (n == 214748364 && c < (neg ? 9 : 8)))
		{
			n = n * 10 + c;
			c = *s++;
			c -= '0';
			if (c >= 0 && c <= 9)
				continue;
			break; /* no more digits */
		}
		/* overflow */
		*valp = 0;
		return -2;
	}

	if (neg) n = -n;
	*valp = (TINT) n;
	return s - p;
}

#endif /* _TEK_STRING_C */
