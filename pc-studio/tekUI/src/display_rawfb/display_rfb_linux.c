
/*
**	display_rfb_linux.c - Linux framebuffer display driver
**	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <unistd.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/input.h>
#include <linux/kd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/inotify.h>

#include "display_rfb_mod.h"
#include <tek/lib/utf8.h>

#define EVNOTIFYPATH "/dev/input"
#define EVPATH "/dev/input/by-path/"

static TUINT rfb_processmouseinput(struct rfb_Display *mod,
	struct input_event *ev);
static void rfb_processkbdinput(struct rfb_Display *mod,
	struct input_event *ev);

#include "keymap.c"

/*****************************************************************************/

static TBOOL rfb_findeventinput(const char *path, const char *what,
	char *fullname, size_t len)
{
	struct stat s;
	TBOOL found = TFALSE;
	DIR *dfd = opendir(path);

	if (dfd)
	{
		struct dirent *de;

		while ((de = readdir(dfd)))
		{
			TBOOL valid = TFALSE;

			strcpy(fullname, path);
			strcat(fullname, de->d_name);
			if (de->d_type == DT_LNK && stat(fullname, &s) == 0)
				valid = S_ISCHR(s.st_mode);
			else if (de->d_type == DT_CHR)
				valid = TTRUE;
			if (!valid)
				continue;
			if (!strstr(fullname, what))
				continue;
			found = TTRUE;
			break;
		}
		closedir(dfd);
	}
	return found;
}

static void rfb_updateinput(struct rfb_Display *mod)
{
	char fullname[1024];
	int fd_kbd = mod->rfb_fd_input_kbd;
	int fd_mouse = mod->rfb_fd_input_mouse;
	int fd_max;

	if (fd_kbd != -1)
		close(fd_kbd);
	if (fd_mouse != -1)
		close(fd_mouse);

	mod->rfb_fd_input_kbd = -1;
	mod->rfb_fd_input_mouse = -1;

	if (rfb_findeventinput(EVPATH, "event-kbd", fullname, sizeof fullname))
	{
		fd_kbd = open(fullname, O_RDONLY);
		if (fd_kbd)
		{
			if (fcntl(fd_kbd, F_SETFL, O_NONBLOCK) != -1)
				mod->rfb_fd_input_kbd = fd_kbd;
			else
			{
				TDBPRINTF(TDB_ERROR, ("Cannot access keyboard\n"));
				close(fd_kbd);
				fd_kbd = -1;
			}
		}
		else
			TDBPRINTF(TDB_WARN, ("Cannot open %s\n", fullname));
	}
	else
		TDBPRINTF(TDB_WARN, ("No keyboard found\n"));

	if (rfb_findeventinput(EVPATH, "event-mouse", fullname, sizeof fullname))
	{
		fd_mouse = open(fullname, O_RDONLY);
		if (fd_mouse)
		{
			if (fcntl(fd_mouse, F_SETFL, O_NONBLOCK) != -1)
				mod->rfb_fd_input_mouse = fd_mouse;
			else
			{
				TDBPRINTF(TDB_ERROR, ("Cannot access mouse\n"));
				close(fd_mouse);
				fd_mouse = -1;
			}
		}
		else
			TDBPRINTF(TDB_WARN, ("Cannot open %s\n", fullname));
	}
	else
		TDBPRINTF(TDB_WARN, ("No mouse found\n"));

	if (fd_mouse)
	{
		ioctl(fd_mouse, EVIOCGABS(0), &mod->rfb_absinfo[0]);
		ioctl(fd_mouse, EVIOCGABS(1), &mod->rfb_absinfo[1]);
	}

	fd_max = TMAX(mod->rfb_fd_sigpipe_read, mod->rfb_fd_inotify_input);
	fd_max = TMAX(fd_max, fd_mouse);
	fd_max = TMAX(fd_max, fd_kbd);
	mod->rfb_fd_max = fd_max + 1;
}

static void rfb_initkeytable(struct rfb_Display *mod)
{
	struct RawKey **rkeys = mod->rfb_RawKeys;
	int i;

	memset(mod->rfb_RawKeys, 0, sizeof mod->rfb_RawKeys);
	for (i = 0; i < (int) (sizeof rawkeyinit / sizeof(struct RawKeyInit)); ++i)
	{
		struct RawKeyInit *ri = &rawkeyinit[i];
		rkeys[ri->index] = &ri->rawkey;
	}
}

LOCAL void rfb_linux_wait(struct rfb_Display *mod, TTIME *waitt)
{
	char buf[512] __attribute__ ((aligned(__alignof__(struct inotify_event))));
	fd_set rset;
	int fd_mouse = mod->rfb_fd_input_mouse;
	int fd_kbd = mod->rfb_fd_input_kbd;
	int fd_inotify = mod->rfb_fd_inotify_input;
	int fd_sig = mod->rfb_fd_sigpipe_read;
	struct timeval tv, *pvt = NULL;

	if (waitt)
	{
		tv.tv_sec = waitt->tdt_Int64 / 1000000;
		tv.tv_usec = waitt->tdt_Int64 % 1000000;
		pvt = &tv;
	}

	FD_ZERO(&rset);
	FD_SET(fd_sig, &rset);
	if (fd_mouse != -1)
		FD_SET(fd_mouse, &rset);
	if (fd_kbd != -1)
		FD_SET(fd_kbd, &rset);
	if (fd_inotify != -1)
		FD_SET(fd_inotify, &rset);

	if (select(mod->rfb_fd_max, &rset, NULL, NULL, pvt) > 0)
	{
		struct input_event ie[16];
		int i;

		/* consume signal: */
		if (FD_ISSET(fd_sig, &rset))
		{
			int nbytes;

			ioctl(fd_sig, FIONREAD, &nbytes);
			if (nbytes > 0)
			{
				nbytes = TMIN((int) sizeof(buf), nbytes);
				if (read(fd_sig, buf, (size_t) nbytes) != nbytes)
					TDBPRINTF(TDB_ERROR, ("could not read wakeup signals\n"));
			}
		}
		if (fd_mouse != -1 && FD_ISSET(fd_mouse, &rset))
		{
			TUINT input_pending = 0;

			for (;;)
			{
				ssize_t nread = (int) read(fd_mouse, ie, sizeof ie);

				if (nread < (int) sizeof(struct input_event))
					break;
				for (i = 0;
					i < (int) (nread / sizeof(struct input_event)); ++i)
					input_pending |= rfb_processmouseinput(mod, &ie[i]);
			}
			if (input_pending & TITYPE_MOUSEMOVE)
			{
				/* get prototype message: */
				TIMSG *msg;

				if (rfb_getimsg(mod, TNULL, &msg, TITYPE_MOUSEMOVE))
				{
					rfb_passevent_mousemove(mod, msg);
					rfb_putbackmsg(mod, msg);
				}
			}
		}
		if (fd_kbd != -1 && FD_ISSET(fd_kbd, &rset))
		{
			for (;;)
			{
				ssize_t nread = (int) read(fd_kbd, ie, sizeof ie);

				if (nread < (int) sizeof(struct input_event))
					break;
				for (i = 0;
					i < (int) (nread / sizeof(struct input_event)); ++i)
					rfb_processkbdinput(mod, &ie[i]);
			}
		}
		if (fd_inotify != -1 && FD_ISSET(fd_inotify, &rset))
		{
			if (read(fd_inotify, buf, sizeof buf) == -1)
				TDBPRINTF(TDB_ERROR, ("Error reading from event inotify\n"));
			rfb_updateinput(mod);
		}
	}
}

/*****************************************************************************/

static void rfb_processkbdinput(struct rfb_Display *mod,
	struct input_event *ev)
{
	TINT qual = mod->rfb_KeyQual;
	TINT code = 0;
	TINT evtype = ev->value == 0 ? TITYPE_KEYUP : TITYPE_KEYDOWN;
	struct RawKey *rk;

	if (ev->type != EV_KEY)
		return;
	TDBPRINTF(TDB_DEBUG, ("code=%d,%d qual=%d\n", ev->code, ev->value, qual));
	rk = mod->rfb_RawKeys[ev->code];
	if (rk)
	{
		if (rk->qualifier)
		{
			if (ev->value == 0)
				qual &= ~rk->qualifier;
			else
				qual |= rk->qualifier;
		}
		else
		{
			code = rk->keycode;
			if (qual != 0)
			{
				int i;

				for (i = 0; i < RFB_RAWKEY_QUALS; ++i)
				{
					if ((rk->qualkeys[i].qualifier & qual) == qual &&
						rk->qualkeys[i].keycode)
					{
						code = rk->qualkeys[i].keycode;
						break;
					}
				}
			}
		}
	}

	if (code || qual != mod->rfb_KeyQual)
	{
		TIMSG *msg;

		if (rfb_getimsg(mod, TNULL, &msg, evtype))
		{
			ptrdiff_t len = (ptrdiff_t) utf8encode(msg->timsg_KeyCode, code) -
				(ptrdiff_t) msg->timsg_KeyCode;
			msg->timsg_KeyCode[len] = 0;
			msg->timsg_Code = code;
			msg->timsg_Qualifier = qual & ~TKEYQ_RALT;
			rfb_passevent_keyboard(mod, msg);
			rfb_putbackmsg(mod, msg);
		}
	}
	mod->rfb_KeyQual = qual;
}

/*****************************************************************************/

static TUINT rfb_processmouseinput(struct rfb_Display *mod,
	struct input_event *ev)
{
	TUINT input_pending = 0;

	switch (ev->type)
	{
		case EV_KEY:
		{
			TUINT bc = 0;

			switch (ev->code)
			{
				case BTN_LEFT:
					bc = ev->value ? TMBCODE_LEFTDOWN : TMBCODE_LEFTUP;
					break;
				case BTN_RIGHT:
					bc = ev->value ? TMBCODE_RIGHTDOWN : TMBCODE_RIGHTUP;
					break;
				case BTN_MIDDLE:
					bc = ev->value ? TMBCODE_MIDDLEDOWN : TMBCODE_MIDDLEUP;
					break;

				case BTN_TOOL_FINGER:
				case BTN_TOUCH:
					TDBPRINTF(TDB_DEBUG, ("TOUCH %d\n", ev->value));
					mod->rfb_button_touch = ev->value;
					if (ev->value)
					{
						mod->rfb_absstart[0] = mod->rfb_abspos[0];
						mod->rfb_absstart[1] = mod->rfb_abspos[1];
						mod->rfb_startmouse[0] = mod->rfb_MouseX;
						mod->rfb_startmouse[1] = mod->rfb_MouseY;
					}
					break;
			}
			if (bc)
			{
				TIMSG *msg;

				if (rfb_getimsg(mod, TNULL, &msg, TITYPE_MOUSEBUTTON))
				{
					msg->timsg_Code = bc;
					rfb_passevent_mousebutton(mod, msg);
					rfb_putbackmsg(mod, msg);
				}
			}
			break;
		}
		case EV_ABS:
		{
			switch (ev->code)
			{
				case ABS_X:
				{
					mod->rfb_abspos[0] = ev->value;
					if (mod->rfb_button_touch)
					{
						int mx = ev->value - mod->rfb_absstart[0];

						mx = mx * mod->rfb_Width /
							(mod->rfb_absinfo[0].maximum -
							mod->rfb_absinfo[0].minimum);
						mod->rfb_MouseX = TCLAMP(0,
							mx + mod->rfb_startmouse[0], mod->rfb_Width - 1);
						input_pending |= TITYPE_MOUSEMOVE;
					}
					break;
				}
				case ABS_Y:
				{
					mod->rfb_abspos[1] = ev->value;
					if (mod->rfb_button_touch)
					{
						int my = ev->value - mod->rfb_absstart[1];

						my = my * mod->rfb_Height /
							(mod->rfb_absinfo[1].maximum -
							mod->rfb_absinfo[1].minimum);
						mod->rfb_MouseY = TCLAMP(0,
							my + mod->rfb_startmouse[1], mod->rfb_Height - 1);
						input_pending |= TITYPE_MOUSEMOVE;
					}
					break;
				}
			}
			break;
		}
		case EV_REL:
		{
			switch (ev->code)
			{
				case REL_X:
					mod->rfb_MouseX = TCLAMP(0,
						mod->rfb_MouseX + ev->value, mod->rfb_Width - 1);
					input_pending |= TITYPE_MOUSEMOVE;
					break;
				case REL_Y:
					mod->rfb_MouseY = TCLAMP(0,
						mod->rfb_MouseY + ev->value, mod->rfb_Height - 1);
					input_pending |= TITYPE_MOUSEMOVE;
					break;
				case REL_WHEEL:
				{
					TIMSG *msg;

					if (rfb_getimsg(mod, TNULL, &msg, TITYPE_MOUSEBUTTON))
					{
						msg->timsg_Code = ev->value < 0 ?
							TMBCODE_WHEELDOWN : TMBCODE_WHEELUP;
						rfb_passevent_by_mousexy(mod, msg, TFALSE);
						rfb_putbackmsg(mod, msg);
					}
				}
			}
			break;
		}
	}
	return input_pending;
}

LOCAL void rfb_linux_wake(struct rfb_Display *inst)
{
	char sig = 0;

	if (write(inst->rfb_fd_sigpipe_write, &sig, 1) != 1)
		TDBPRINTF(TDB_ERROR, ("could not send wakeup signal\n"));
}

static void rfb_linux_closefd(int *fd)
{
	if (*fd != -1)
		close(*fd);
	*fd = -1;
}

LOCAL void rfb_linux_exit(struct rfb_Display *mod)
{
	rfb_linux_closefd(&mod->rfb_fd_input_kbd);
	rfb_linux_closefd(&mod->rfb_fd_input_mouse);
	rfb_linux_closefd(&mod->rfb_fbhnd);
	if (mod->rfb_ttyfd != -1)
	{
		ioctl(mod->rfb_ttyfd, KDSETMODE, mod->rfb_ttyoldmode);
		rfb_linux_closefd(&mod->rfb_ttyfd);
	}
	rfb_linux_closefd(&mod->rfb_fd_inotify_input);
	mod->rfb_fd_watch_input = -1;
}

static const struct rfb_pixfmt
{
	TUINT rmsk, gmsk, bmsk, pixfmt;
	TUINT8 roffs, rlen, goffs, glen, boffs, blen;
}
rfb_pixfmts[] =
{
	{ 0x00ff0000, 0x0000ff00, 0x000000ff, TVPIXFMT_08R8G8B8, 16,8, 8,8, 0,8 },
	{ 0x000000ff, 0x0000ff00, 0x00ff0000, TVPIXFMT_08B8G8R8, 0,8, 8,8, 16,8 },
	{ 0xff000000, 0x00ff0000, 0x0000ff00, TVPIXFMT_R8G8B808, 24,8, 16,8, 8,8 },
	{ 0x0000ff00, 0x00ff0000, 0xff000000, TVPIXFMT_B8G8R808, 8,8, 16,8, 24,8 },
	{ 0x0000f800, 0x000007e0, 0x0000001f, TVPIXFMT_R5G6B5, 11,5, 5,6, 0,5 },
	{ 0x00007c00, 0x000003e0, 0x0000001f, TVPIXFMT_0R5G5B5, 10,5, 5,5, 0,5 },
	{ 0x0000001f, 0x000003e0, 0x0000f800, TVPIXFMT_0B5G5R5, 0,5, 5,5, 10,5 },
};

#define MASKFROMBF(bf) ((0xffffffff << (bf)->offset) \
	& (0xffffffff >> (32 - (bf)->offset - (bf)->length)))

static void getmasksfromvinfo(struct fb_var_screeninfo *vinfo, TUINT *rmsk,
	TUINT *gmsk, TUINT *bmsk)
{
	*rmsk = MASKFROMBF(&vinfo->red);
	*gmsk = MASKFROMBF(&vinfo->green);
	*bmsk = MASKFROMBF(&vinfo->blue);
}

static TUINT rfb_getvinfopixfmt(struct fb_var_screeninfo *vinfo)
{
	TUINT i, rmsk, gmsk, bmsk;

	getmasksfromvinfo(vinfo, &rmsk, &gmsk, &bmsk);
	for (i = 0; i < sizeof(rfb_pixfmts) / sizeof(struct rfb_pixfmt); ++i)
	{
		const struct rfb_pixfmt *fmt = &rfb_pixfmts[i];

		if (fmt->rmsk == rmsk && fmt->gmsk == gmsk && fmt->bmsk == bmsk)
			return fmt->pixfmt;
	}
	return TVPIXFMT_UNDEFINED;
}

static TBOOL rfb_setvinfopixfmt(struct fb_var_screeninfo *vinfo, TUINT pixfmt)
{
	TUINT i;

	for (i = 0; i < sizeof(rfb_pixfmts) / sizeof(struct rfb_pixfmt); ++i)
	{
		const struct rfb_pixfmt *fmt = &rfb_pixfmts[i];

		if (pixfmt == fmt->pixfmt)
		{
			vinfo->red.offset = fmt->roffs;
			vinfo->red.length = fmt->rlen;
			vinfo->green.offset = fmt->goffs;
			vinfo->green.length = fmt->glen;
			vinfo->blue.offset = fmt->boffs;
			vinfo->blue.length = fmt->blen;
			return TTRUE;
		}
	}
	return TFALSE;
}

LOCAL TBOOL rfb_linux_init(struct rfb_Display *mod)
{
	for (;;)
	{
		int pipefd[2];
		TUINT pixfmt;

		mod->rfb_fd_sigpipe_read = -1;
		mod->rfb_fd_sigpipe_write = -1;
		mod->rfb_ttyfd = -1;
		mod->rfb_ttyoldmode = KD_TEXT;
		mod->rfb_fd_input_kbd = -1;
		mod->rfb_fd_input_mouse = -1;
		mod->rfb_fbhnd = -1;
		mod->rfb_fd_inotify_input = -1;
		mod->rfb_fd_watch_input = -1;

		if (pipe(pipefd) != 0)
			break;

		mod->rfb_fd_sigpipe_read = pipefd[0];
		mod->rfb_fd_sigpipe_write = pipefd[1];

		mod->rfb_fd_inotify_input = inotify_init();
		if (mod->rfb_fd_inotify_input != -1)
			mod->rfb_fd_watch_input =
				inotify_add_watch(mod->rfb_fd_inotify_input,
				EVNOTIFYPATH, IN_CREATE | IN_DELETE);
		if (mod->rfb_fd_watch_input == -1)
			TDBPRINTF(TDB_WARN, ("cannot watch input events\n"));

		mod->rfb_ttyfd = open("/dev/console", O_RDWR);
		if (mod->rfb_ttyfd != -1)
		{
			/*ioctl(mod->rfb_ttyfd, KDGETMODE, &mod->rfb_ttyoldmode); */
			ioctl(mod->rfb_ttyfd, KDSETMODE, KD_GRAPHICS);
		}
		else
			TDBPRINTF(TDB_WARN, ("Cannot access console device\n"));

		/* open framebuffer device */
		mod->rfb_fbhnd = open("/dev/fb0", O_RDWR);
		if (mod->rfb_fbhnd == -1)
		{
			TDBPRINTF(TDB_ERROR, ("Cannot open framebuffer device\n"));
			break;
		}

		if (ioctl(mod->rfb_fbhnd, FBIOGET_FSCREENINFO, &mod->rfb_finfo))
			break;

		if (mod->rfb_finfo.type != FB_TYPE_PACKED_PIXELS ||
			mod->rfb_finfo.visual != FB_VISUAL_TRUECOLOR)
		{
			TDBPRINTF(TDB_ERROR, ("Unsupported framebuffer type\n"));
			break;
		}

		/* get and backup */
		if (ioctl(mod->rfb_fbhnd, FBIOGET_VSCREENINFO, &mod->rfb_vinfo))
			break;
		mod->rfb_orig_vinfo = mod->rfb_vinfo;

		/* setting mode doesn't seem to work? */
#if 0
#if defined(RFBPIXFMT)
		if (rfb_getvinfopixfmt(&mod->rfb_vinfo) != RFBPIXFMT)
		{
			/* set properties */
			pixfmt = RFBPIXFMT;
			bpp = TVPIXFMT_BYTES_PER_PIXEL(pixfmt);
			mod->rfb_vinfo.bits_per_pixel = bpp * 8;
			rfb_setvinfopixfmt(&mod->rfb_vinfo, pixfmt);
			if (ioctl(mod->rfb_fbhnd, FBIOPUT_VSCREENINFO, &mod->rfb_vinfo))
				break;
			/* reload */
			if (ioctl(mod->rfb_fbhnd, FBIOGET_VSCREENINFO, &mod->rfb_vinfo))
				break;
		}
#endif
#endif

		pixfmt = rfb_getvinfopixfmt(&mod->rfb_vinfo);
		if (pixfmt == TVPIXFMT_UNDEFINED)
		{
			TDBPRINTF(TDB_ERROR, ("Unsupported framebuffer pixel format\n"));
			break;
		}

		mod->rfb_DevWidth = mod->rfb_vinfo.xres;
		mod->rfb_DevHeight = mod->rfb_vinfo.yres;
		mod->rfb_DevBuf.tpb_BytesPerLine = mod->rfb_finfo.line_length;
		mod->rfb_DevBuf.tpb_Format = pixfmt;
		mod->rfb_DevBuf.tpb_Data = mmap(0, mod->rfb_finfo.smem_len,
			PROT_READ | PROT_WRITE, MAP_SHARED, mod->rfb_fbhnd, 0);
		if ((TINTPTR) mod->rfb_DevBuf.tpb_Data == -1)
			break;
		memset(mod->rfb_DevBuf.tpb_Data, 0, mod->rfb_finfo.smem_len);

		mod->rfb_PixBuf = mod->rfb_DevBuf;

		rfb_updateinput(mod);
		rfb_initkeytable(mod);

		mod->rfb_Flags |= RFBFL_CANSHOWPTR | RFBFL_SHOWPTR;
		mod->rfb_Flags &= ~RFBFL_BUFFER_CAN_RESIZE;

		return TTRUE;
	}

	rfb_linux_exit(mod);
	return TFALSE;
}
