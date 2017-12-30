#ifndef _TEK_LIB_UTF8_C
#define _TEK_LIB_UTF8_C

/*
**	utf8.c - UTF-8 library
**	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
**	Placed in the Public Domain
**
**	References:
**	http://www.cl.cam.ac.uk/~mgk25/unicode.html
**	http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt
*/

#include <tek/lib/utf8.h>

TLIBAPI int utf8read(struct utf8reader *rd)
{
	int c;
	for (;;)
	{
		if (rd->bufc >= 0)
		{
			c = rd->bufc;
			rd->bufc = -1;
		}
		else
			c = rd->readchar(rd);
		if (c < 0)
			return c;

		if (c == 254 || c == 255)
			break;

		if (c < 128)
		{
			if (rd->numa > 0)
			{
				rd->bufc = c;
				break;
			}
			return c;
		}
		else if (c < 192)
		{
			if (rd->numa == 0)
				break;
			rd->accu <<= 6;
			rd->accu += c - 128;
			rd->numa--;
			if (rd->numa == 0)
			{
				if (rd->accu == 0 || rd->accu < rd->min ||
					(rd->accu >= 55296 && rd->accu <= 57343))
					break;
				c = rd->accu;
				rd->accu = 0;
				return c;
			}
		}
		else
		{
			if (rd->numa > 0)
			{
				rd->bufc = c;
				break;
			}

			if (c < 224)
			{
				rd->min = 128;
				rd->accu = c - 192;
				rd->numa = 1;
			}
			else if (c < 240)
			{
				rd->min = 2048;
				rd->accu = c - 224;
				rd->numa = 2;
			}
			else if (c < 248)
			{
				rd->min = 65536;
				rd->accu = c - 240;
				rd->numa = 3;
			}
			else if (c < 252)
			{
				rd->min = 2097152;
				rd->accu = c - 248;
				rd->numa = 4;
			}
			else
			{
				rd->min = 67108864;
				rd->accu = c - 252;
				rd->numa = 5;
			}
		}
	}
	/* bad char */
	rd->accu = 0;
	rd->numa = 0;
	return 65533;
}

TLIBAPI int utf8readstring(struct utf8reader *rd)
{
	struct utf8readstringdata *ud = rd->udata;
	if (ud->srclen == 0)
		return -1;
	ud->srclen--;
	return *ud->src++;
}


TLIBAPI void utf8initreader(struct utf8reader *rd, 
	const unsigned char *utf8text, size_t bytelen)
{
	rd->rsdata.src = utf8text;
	rd->rsdata.srclen = bytelen;
	rd->readchar = utf8readstring;
	rd->accu = 0;
	rd->numa = 0;
	rd->bufc = -1;
	rd->udata = &rd->rsdata;
}

TLIBAPI unsigned char *utf8encode(unsigned char *buf, int c)
{
	if (c < 128)
	{
		*buf++ = c;
	}
	else if (c < 2048)
	{
		*buf++ = 0xc0 + (c >> 6);
		*buf++ = 0x80 + (c & 0x3f);
	}
	else if (c < 65536)
	{
		*buf++ = 0xe0 + (c >> 12);
		*buf++ = 0x80 + ((c & 0xfff) >> 6);
		*buf++ = 0x80 + (c & 0x3f);
	}
	else if (c < 2097152)
	{
		*buf++ = 0xf0 + (c >> 18);
		*buf++ = 0x80 + ((c & 0x3ffff) >> 12);
		*buf++ = 0x80 + ((c & 0xfff) >> 6);
		*buf++ = 0x80 + (c & 0x3f);
	}
	else if (c < 67108864)
	{
		*buf++ = 0xf8 + (c >> 24);
		*buf++ = 0x80 + ((c & 0xffffff) >> 18);
		*buf++ = 0x80 + ((c & 0x3ffff) >> 12);
		*buf++ = 0x80 + ((c & 0xfff) >> 6);
		*buf++ = 0x80 + (c & 0x3f);
	}
	else
	{
		*buf++ = 0xfc + (c >> 30);
		*buf++ = 0x80 + ((c & 0x3fffffff) >> 24);
		*buf++ = 0x80 + ((c & 0xffffff) >> 18);
		*buf++ = 0x80 + ((c & 0x3ffff) >> 12);
		*buf++ = 0x80 + ((c & 0xfff) >> 6);
		*buf++ = 0x80 + (c & 0x3f);
	}
	return buf;
}

TLIBAPI size_t utf8tolatin(const unsigned char *utf8, size_t utf8len,
	unsigned char *latin, size_t latinbuflen, unsigned char repl)
{
	struct utf8reader rd;
	size_t len = 0;
	int c;
	utf8initreader(&rd, utf8, utf8len);
	while (len < latinbuflen - 1 && (c = utf8read(&rd)) >= 0)
		latin[len++] = c < 256 ? c : repl;
	latin[len] = 0;
	return len;
}


TLIBAPI size_t utf8getlen(const unsigned char *raws, size_t rawlen)
{
	struct utf8reader rd;
	int c;
	size_t reallen = 0;
	utf8initreader(&rd, raws, rawlen);
	while ((c = utf8read(&rd)) >= 0)
		reallen++;
	return reallen;
}

#endif


#if 0
/*****************************************************************************/
/*
**	Test usage:
**	# wget http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt
**	# ./utf8test < UTF-8-test.txt > out.txt
*/

struct readfiledata
{
	FILE *file;
};


int readfile(struct utf8reader *rd)
{
	struct readfiledata *ud = rd->udata;
	return fgetc(ud->file);
}


int main(int argc, char **argv)
{
	struct readfiledata rs;
	struct utf8reader rd;
	int c, ln = 1;
	char outbuf[6], *bufend;

	rs.file = stdin;

	rd.readchar = readfile;
	rd.accu = 0;
	rd.numa = 0;
	rd.bufc = -1;
	rd.udata = &rs;

	while ((c = readutf8(&rd)) >= 0)
	{
		/* unicode charcode (31bit) in c */
		if (c == 65533)
			fprintf(stderr, "Bad UTF-8 encoding / char in line %d\n", ln);
		bufend = encodeutf8(outbuf, c);
		fwrite(outbuf, bufend - outbuf, 1, stdout);
		if (c == 10)
			ln++;
	}

	return 1;
}
#endif
