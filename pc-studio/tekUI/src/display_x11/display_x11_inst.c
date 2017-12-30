
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#ifdef __sun
#include <sys/filio.h>
#endif
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <X11/cursorfont.h>

#include "display_x11_mod.h"
#include <tek/inline/exec.h>

static TBOOL x11_getimsg(struct X11Display *mod, struct X11Window *v,
	TIMSG ** msgptr, TUINT type)
{
	TAPTR TExecBase = TGetExecBase(mod);
	TIMSG *msg = (TIMSG *) TRemHead(&mod->x11_imsgpool);

	if (msg == TNULL)
		msg = TAllocMsg0(sizeof(TIMSG));
	if (msg)
	{
		msg->timsg_Instance = v;
		msg->timsg_UserData = v->userdata;
		msg->timsg_Type = type;
		msg->timsg_Qualifier = mod->x11_KeyQual;
		msg->timsg_ScreenMouseX = mod->x11_ScreenMouseX;
		msg->timsg_ScreenMouseY = mod->x11_ScreenMouseY;
		msg->timsg_MouseX = v->mousex;
		msg->timsg_MouseY = v->mousey;
		TGetSystemTime(&msg->timsg_TimeStamp);
		*msgptr = msg;
		return TTRUE;
	}
	*msgptr = TNULL;
	return TFALSE;
}

LOCAL void x11_sendimessages(struct X11Display *mod)
{
	TAPTR TExecBase = TGetExecBase(mod);
	struct TNode *next, *node = mod->x11_vlist.tlh_Head.tln_Succ;

	for (; (next = node->tln_Succ); node = next)
	{
		struct X11Window *v = (struct X11Window *) node;
		TIMSG *imsg;

		while ((imsg = (TIMSG *) TRemHead(&v->imsgqueue)))
		{
			/* only certain input message types are sent two-way */
			struct TMsgPort *rport = imsg->timsg_Type == TITYPE_REQSELECTION ?
				mod->x11_IReplyPort : TNULL;
			TPutMsg(v->imsgport, rport, imsg);
		}
	}
}

static void x11_setmousepos(struct X11Display *mod, struct X11Window *v,
	TINT x, TINT y)
{
	v->mousex = x;
	v->mousey = y;
	mod->x11_ScreenMouseX = x + v->winleft;
	mod->x11_ScreenMouseY = y + v->wintop;
}

static TBOOL x11_processkey(struct X11Display *mod, struct X11Window *v,
	XKeyEvent *ev, TBOOL keydown)
{
	KeySym keysym;
	XComposeStatus compose;
	char buffer[10];

	TIMSG *imsg;
	TUINT evtype = 0;
	TUINT newqual;
	TUINT evmask = v->eventmask;
	TBOOL newkey = TFALSE;

	x11_setmousepos(mod, v, ev->x, ev->y);

	XLookupString(ev, buffer, 10, &keysym, &compose);

	switch (keysym)
	{
		case XK_Shift_L:
			newqual = TKEYQ_LSHIFT;
			break;
		case XK_Shift_R:
			newqual = TKEYQ_RSHIFT;
			break;
		case XK_Control_L:
			newqual = TKEYQ_LCTRL;
			break;
		case XK_Control_R:
			newqual = TKEYQ_RCTRL;
			break;
		case XK_Alt_L:
			newqual = TKEYQ_LALT;
			break;
		case XK_Alt_R:
			newqual = TKEYQ_RALT;
			break;
		default:
			newqual = 0;
	}

	if (newqual != 0)
	{
		if (keydown)
			mod->x11_KeyQual |= newqual;
		else
			mod->x11_KeyQual &= ~newqual;
	}

	if (keydown && (evmask & TITYPE_KEYDOWN))
		evtype = TITYPE_KEYDOWN;
	else if (!keydown && (evmask & TITYPE_KEYUP))
		evtype = TITYPE_KEYUP;

	if (evtype && x11_getimsg(mod, v, &imsg, evtype))
	{
		imsg->timsg_Qualifier = mod->x11_KeyQual;

		if (keysym >= XK_F1 && keysym <= XK_F12)
		{
			imsg->timsg_Code = (TUINT) (keysym - XK_F1) + TKEYC_F1;
			newkey = TTRUE;
		}
		else if (keysym < 256)
		{
			/* cooked ASCII/Latin-1 code */
			imsg->timsg_Code = keysym;
			newkey = TTRUE;
		}
		else if (keysym >= XK_KP_0 && keysym <= XK_KP_9)
		{
			imsg->timsg_Code = (TUINT) (keysym - XK_KP_0) + 48;
			imsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
			newkey = TTRUE;
		}
		else
		{
			newkey = TTRUE;
			switch (keysym)
			{
				case XK_Left:
					imsg->timsg_Code = TKEYC_CRSRLEFT;
					break;
				case XK_Right:
					imsg->timsg_Code = TKEYC_CRSRRIGHT;
					break;
				case XK_Up:
					imsg->timsg_Code = TKEYC_CRSRUP;
					break;
				case XK_Down:
					imsg->timsg_Code = TKEYC_CRSRDOWN;
					break;

				case XK_Escape:
					imsg->timsg_Code = TKEYC_ESC;
					break;
				case XK_Delete:
					imsg->timsg_Code = TKEYC_DEL;
					break;
				case XK_BackSpace:
					imsg->timsg_Code = TKEYC_BCKSPC;
					break;
				case XK_ISO_Left_Tab:
				case XK_Tab:
					imsg->timsg_Code = TKEYC_TAB;
					break;
				case XK_Return:
					imsg->timsg_Code = TKEYC_RETURN;
					break;

				case XK_Help:
					imsg->timsg_Code = TKEYC_HELP;
					break;
				case XK_Insert:
					imsg->timsg_Code = TKEYC_INSERT;
					break;
				case XK_Page_Up:
					imsg->timsg_Code = TKEYC_PAGEUP;
					break;
				case XK_Page_Down:
					imsg->timsg_Code = TKEYC_PAGEDOWN;
					break;
				case XK_Home:
					imsg->timsg_Code = TKEYC_POSONE;
					break;
				case XK_End:
					imsg->timsg_Code = TKEYC_POSEND;
					break;
				case XK_Print:
					imsg->timsg_Code = TKEYC_PRINT;
					break;
				case XK_Scroll_Lock:
					imsg->timsg_Code = TKEYC_SCROLL;
					break;
				case XK_Pause:
					imsg->timsg_Code = TKEYC_PAUSE;
					break;
				case XK_KP_Enter:
					imsg->timsg_Code = TKEYC_RETURN;
					imsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
					break;
				case XK_KP_Decimal:
					imsg->timsg_Code = '.';
					imsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
					break;
				case XK_KP_Add:
					imsg->timsg_Code = '+';
					imsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
					break;
				case XK_KP_Subtract:
					imsg->timsg_Code = '-';
					imsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
					break;
				case XK_KP_Multiply:
					imsg->timsg_Code = '*';
					imsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
					break;
				case XK_KP_Divide:
					imsg->timsg_Code = '/';
					imsg->timsg_Qualifier |= TKEYQ_NUMBLOCK;
					break;
				default:
					if (keysym > 31 && keysym <= 0x20ff)
						imsg->timsg_Code = keysym;
					else if (keysym >= 0x01000100 && keysym <= 0x0110ffff)
						imsg->timsg_Code = keysym - 0x01000000;
					else
						newkey = TFALSE;
					break;
			}
		}

		if (!newkey && newqual)
		{
			imsg->timsg_Code = TKEYC_NONE;
			newkey = TTRUE;
		}

		if (newkey)
		{
			ptrdiff_t len =
				(ptrdiff_t) utf8encode(imsg->timsg_KeyCode, imsg->timsg_Code) -
				(ptrdiff_t) imsg->timsg_KeyCode;
			imsg->timsg_KeyCode[len] = 0;
			TAddTail(&v->imsgqueue, &imsg->timsg_Node);
		}
		else
		{
			/* put back message: */
			TAddTail(&mod->x11_imsgpool, &imsg->timsg_Node);
		}
	}

	return newkey;
}

static TBOOL x11_processvisualevent(struct X11Display *mod,
	struct X11Window *v, TAPTR msgstate, XEvent *ev)
{
	TAPTR TExecBase = TGetExecBase(mod);
	TIMSG *imsg;

	switch (ev->type)
	{
		case ClientMessage:
			if ((v->eventmask & TITYPE_CLOSE) &&
				(Atom) ev->xclient.data.l[0] == v->atom_wm_delete_win)
			{
				if (x11_getimsg(mod, v, &imsg, TITYPE_CLOSE))
					TAddTail(&v->imsgqueue, &imsg->timsg_Node);
			}
			break;

		case ConfigureNotify:
			if (mod->x11_RequestInProgress && (v->flags & X11WFL_WAIT_RESIZE))
			{
				TReplyMsg(mod->x11_RequestInProgress);
				mod->x11_RequestInProgress = TNULL;
				v->flags &= ~X11WFL_WAIT_RESIZE;
				TDBPRINTF(TDB_INFO, ("Released request (ConfigureNotify)\n"));
			}

			v->winleft = ev->xconfigure.x;
			v->wintop = ev->xconfigure.y;

			if ((v->winwidth != ev->xconfigure.width ||
					v->winheight != ev->xconfigure.height))
			{
				v->flags |= X11WFL_WAIT_EXPOSE;
				v->winwidth = ev->xconfigure.width;
				v->winheight = ev->xconfigure.height;
				if (v->eventmask & TITYPE_NEWSIZE)
				{
					if (x11_getimsg(mod, v, &imsg, TITYPE_NEWSIZE))
					{
						imsg->timsg_Width = v->winwidth;
						imsg->timsg_Height = v->winheight;
						TAddTail(&v->imsgqueue, &imsg->timsg_Node);
					}
					TDBPRINTF(TDB_TRACE, ("Configure: NEWSIZE: %d %d\n",
							v->winwidth, v->winheight));
				}
			}
			break;

		case EnterNotify:
		case LeaveNotify:
			if (v->eventmask & TITYPE_MOUSEOVER)
			{
				if (x11_getimsg(mod, v, &imsg, TITYPE_MOUSEOVER))
				{
					imsg->timsg_Code = (ev->type == EnterNotify);
					TAddTail(&v->imsgqueue, &imsg->timsg_Node);
				}
			}
			break;

		case MapNotify:
			if (mod->x11_RequestInProgress)
			{
				TReplyMsg(mod->x11_RequestInProgress);
				mod->x11_RequestInProgress = TNULL;
				v->flags |= X11WFL_WAIT_EXPOSE;
				TDBPRINTF(TDB_TRACE, ("Released request (MapNotify)\n"));
			}
			break;

		case Expose:
			if (v->flags & X11WFL_WAIT_EXPOSE)
				v->flags &= ~X11WFL_WAIT_EXPOSE;
			else if ((v->eventmask & TITYPE_REFRESH) &&
				x11_getimsg(mod, v, &imsg, TITYPE_REFRESH))
			{
				imsg->timsg_X = ev->xexpose.x;
				imsg->timsg_Y = ev->xexpose.y;
				imsg->timsg_Width = ev->xexpose.width;
				imsg->timsg_Height = ev->xexpose.height;
				TAddTail(&v->imsgqueue, &imsg->timsg_Node);
				TDBPRINTF(TDB_TRACE, ("Expose: REFRESH: %d %d %d %d\n",
						imsg->timsg_X, imsg->timsg_Y,
						imsg->timsg_Width, imsg->timsg_Height));
			}
			break;

		case GraphicsExpose:
			if (mod->x11_CopyExposeHook)
			{
				TINT rect[4];

				rect[0] = ev->xgraphicsexpose.x;
				rect[1] = ev->xgraphicsexpose.y;
				rect[2] = rect[0] + ev->xgraphicsexpose.width - 1;
				rect[3] = rect[1] + ev->xgraphicsexpose.height - 1;
				TCallHookPkt(mod->x11_CopyExposeHook,
					mod->x11_RequestInProgress->tvr_Op.CopyArea.Window,
					(TTAG) rect);
			}

			if (ev->xgraphicsexpose.count > 0)
				break;

			/* no more graphics expose events, fallthru: */

		case NoExpose:
			if (mod->x11_RequestInProgress)
			{
				TReplyMsg(mod->x11_RequestInProgress);
				mod->x11_RequestInProgress = TNULL;
				mod->x11_CopyExposeHook = TNULL;
				TDBPRINTF(TDB_TRACE, ("Released request (NoExpose)\n"));
			}
			else
				TDBPRINTF(TDB_INFO, ("NoExpose: TITYPE_REFRESH not set\n"));
			break;

		case FocusIn:
		case FocusOut:
			mod->x11_KeyQual = 0;
			if (v->eventmask & TITYPE_FOCUS)
			{
				if (x11_getimsg(mod, v, &imsg, TITYPE_FOCUS))
				{
					imsg->timsg_Code = (ev->type == FocusIn);
					TAddTail(&v->imsgqueue, &imsg->timsg_Node);
				}
			}
			break;

		case MotionNotify:
		{
			struct TNode *next, *node = mod->x11_vlist.tlh_Head.tln_Succ;

			x11_setmousepos(mod, v, ev->xmotion.x, ev->xmotion.y);
			v->mousex = mod->x11_ScreenMouseX - v->winleft;
			v->mousey = mod->x11_ScreenMouseY - v->wintop;
			for (; (next = node->tln_Succ); node = next)
			{
				struct X11Window *v = (struct X11Window *) node;

				if (v->eventmask & TITYPE_MOUSEMOVE &&
					x11_getimsg(mod, v, &imsg, TITYPE_MOUSEMOVE))
					TAddTail(&v->imsgqueue, &imsg->timsg_Node);
			}
			break;
		}

		case ButtonRelease:
		case ButtonPress:
			x11_setmousepos(mod, v, ev->xbutton.x, ev->xbutton.y);
			if (v->eventmask & TITYPE_MOUSEBUTTON)
			{
				if (x11_getimsg(mod, v, &imsg, TITYPE_MOUSEBUTTON))
				{
					unsigned int button = ev->xbutton.button;

					if (ev->type == ButtonPress)
					{
						switch (button)
						{
							case Button1:
								imsg->timsg_Code = TMBCODE_LEFTDOWN;
								break;
							case Button2:
								imsg->timsg_Code = TMBCODE_MIDDLEDOWN;
								break;
							case Button3:
								imsg->timsg_Code = TMBCODE_RIGHTDOWN;
								break;
							case Button4:
								imsg->timsg_Code = TMBCODE_WHEELUP;
								break;
							case Button5:
								imsg->timsg_Code = TMBCODE_WHEELDOWN;
								break;
						}
					}
					else
					{
						switch (button)
						{
							case Button1:
								imsg->timsg_Code = TMBCODE_LEFTUP;
								break;
							case Button2:
								imsg->timsg_Code = TMBCODE_MIDDLEUP;
								break;
							case Button3:
								imsg->timsg_Code = TMBCODE_RIGHTUP;
								break;
						}
					}
					TAddTail(&v->imsgqueue, &imsg->timsg_Node);
				}
			}
			break;

		case KeyRelease:
			x11_processkey(mod, v, (XKeyEvent *) ev, TFALSE);
			break;

		case KeyPress:
			x11_processkey(mod, v, (XKeyEvent *) ev, TTRUE);
			break;

		case SelectionRequest:
		{
			XSelectionRequestEvent *req = (XSelectionRequestEvent *) ev;
			XEvent replyevent;
			XSelectionEvent *reply = &replyevent.xselection;
			memset(&replyevent, 0, sizeof replyevent);

			reply->type = SelectionNotify;
			reply->serial = ev->xany.send_event;
			reply->send_event = True;
			reply->display = req->display;
			reply->requestor = req->requestor;
			reply->selection = req->selection;
			reply->property = req->property;
			reply->target = None;
			reply->time = req->time;

			if (req->target == mod->x11_XA_TARGETS)
			{
				XChangeProperty(mod->x11_Display, req->requestor,
					req->property, XA_ATOM, 32, PropModeReplace,
					(unsigned char *) &mod->x11_XA_UTF8_STRING, 1);
			}
			else if (req->target == mod->x11_XA_UTF8_STRING)
			{
				XSelectionEvent *rcopy = TAlloc(TNULL, sizeof *reply);

				if (rcopy && x11_getimsg(mod, v, &imsg, TITYPE_REQSELECTION))
				{
					*rcopy = *reply;
					imsg->timsg_Requestor = (TTAG) rcopy;
					imsg->timsg_Code =
						req->selection == mod->x11_XA_PRIMARY ? 2 : 1;
					TAddTail(&v->imsgqueue, &imsg->timsg_Node);
					break;
				}
				TFree(rcopy);
			}
			else
				reply->property = None;

			XSendEvent(mod->x11_Display, req->requestor, 0, NoEventMask,
				&replyevent);
			XSync(mod->x11_Display, False);
			break;
		}

	}
	return TFALSE;
}

static void x11_processevent(struct X11Display *mod)
{
	TAPTR TExecBase = TGetExecBase(mod);
	struct TNode *next, *node;
	XEvent ev;
	struct X11Window *v;
	Window w;

	while ((XPending(mod->x11_Display)) > 0)
	{
		XNextEvent(mod->x11_Display, &ev);
		if (ev.type == mod->x11_ShmEvent)
		{
			if (mod->x11_RequestInProgress)
			{
				TReplyMsg(mod->x11_RequestInProgress);
				mod->x11_RequestInProgress = TNULL;
				TDBPRINTF(TDB_TRACE, ("Released request (ShmEvent)\n"));
			}
			else
				TDBPRINTF(TDB_ERROR, ("shm event while no request pending\n"));
			continue;
		}

		/* lookup window: */
		w = ev.xany.window;
		v = TNULL;
		node = mod->x11_vlist.tlh_Head.tln_Succ;
		for (; (next = node->tln_Succ); node = next)
		{
			v = (struct X11Window *) node;
			if (v->window == w)
				break;
			v = TNULL;
		}

		if (v == TNULL)
		{
			TDBPRINTF(TDB_INFO,
				("Message Type %04x from unknown window: %p\n", ev.type, w));
			continue;
		}

		/* while true, spool out messages for this particular event: */
		while (x11_processvisualevent(mod, v, TNULL, &ev)) ;
	}
}

/*
**	Task
*/

LOCAL void x11_taskfunc(struct TTask *task)
{
	TAPTR TExecBase = TGetExecBase(task);
	struct X11Display *inst = TGetTaskData(task);
	TUINT sig = 0;
	fd_set rset;
	struct TVRequest *req;
	TIMSG *imsg;
	char buf[256];
	struct timeval tv, *ptv;
	struct TMsgPort *cmdport = TGetUserPort(task);
	TUINT cmdportsignal = TGetPortSignal(cmdport);
	TUINT ireplysignal = TGetPortSignal(inst->x11_IReplyPort);
	TUINT waitsigs = cmdportsignal | ireplysignal | TTASK_SIG_ABORT;

	/* interval time: 1/50s: */
	TTIME intt = { 20000 };

	/* next absolute time to send interval message: */
	TTIME nextt;
	TTIME waitt, nowt;

	TDBPRINTF(TDB_INFO, ("Device instance running\n"));

	TGetSystemTime(&nowt);
	nextt = nowt;
	TAddTime(&nextt, &intt);

	do
	{
		if (sig & ireplysignal)
		{
			while ((imsg = TGetMsg(inst->x11_IReplyPort)))
			{
				/* returned input message */
				if (imsg->timsg_Type == TITYPE_REQSELECTION)
				{
					XSelectionEvent *reply =
						(XSelectionEvent *) imsg->timsg_Requestor;
					struct TTagItem *replytags =
						(struct TTagItem *) imsg->timsg_ReplyData;
					size_t len =
						TGetTag(replytags, TIMsgReply_UTF8SelectionLen, 0);
					TUINT8 *xdata = (TUINT8 *) TGetTag(replytags,
						TIMsgReply_UTF8Selection, TNULL);

					XChangeProperty(inst->x11_Display, reply->requestor,
						reply->property, XA_ATOM, 8, PropModeReplace,
						(unsigned char *) xdata, len);
					XSendEvent(inst->x11_Display, reply->requestor, 0,
						NoEventMask, (XEvent *) reply);
					XSync(inst->x11_Display, False);
					TFree((TAPTR) imsg->timsg_Requestor);
					TFree(xdata);
					/* reqselect roundtrip ended */
				}
				TFree(imsg);
			}
		}

		if (sig & cmdportsignal)
		{
			while (inst->x11_RequestInProgress == TNULL &&
				(req = TGetMsg(cmdport)))
			{
				x11_docmd(inst, req);
				if (inst->x11_RequestInProgress)
					break;
				TReplyMsg(req);
			}
		}

		XFlush(inst->x11_Display);

		FD_ZERO(&rset);
		FD_SET(inst->x11_fd_display, &rset);
		FD_SET(inst->x11_fd_sigpipe_read, &rset);

		TGetSystemTime(&nowt);

		if (inst->x11_NumInterval > 0 || inst->x11_RequestInProgress)
		{
			if (TCmpTime(&nowt, &nextt) >= 0)
			{
				/* expired; send interval: */
				struct TNode *next, *node = inst->x11_vlist.tlh_Head.tln_Succ;

				for (; (next = node->tln_Succ); node = next)
				{
					struct X11Window *v = (struct X11Window *) node;
					TIMSG *imsg;

					if ((v->eventmask & TITYPE_INTERVAL) &&
						x11_getimsg(inst, v, &imsg, TITYPE_INTERVAL))
						TPutMsg(v->imsgport, TNULL, imsg);
				}
				TAddTime(&nextt, &intt);
			}

			/* calculate new wait time: */
			waitt = nextt;
			TSubTime(&waitt, &nowt);
			if (waitt.tdt_Int64 <= 0 || waitt.tdt_Int64 > 20000)
			{
				nextt = nowt;
				TAddTime(&nextt, &intt);
				waitt = nextt;
				TSubTime(&waitt, &nowt);
			}
			tv.tv_sec = waitt.tdt_Int64 / 1000000;
			tv.tv_usec = waitt.tdt_Int64 % 1000000;
			ptv = &tv;
		}
		else
			ptv = NULL;

		/* wait for display, signal fd and timeout: */
		if (select(inst->x11_fd_max, &rset, NULL, NULL, ptv) > 0)
		{
			int nbytes;

			/* consume signal: */
			if (FD_ISSET(inst->x11_fd_sigpipe_read, &rset))
			{
				ioctl(inst->x11_fd_sigpipe_read, FIONREAD, &nbytes);
				if (nbytes > 0)
					if (read(inst->x11_fd_sigpipe_read, buf,
							TMIN(sizeof(buf), (size_t) nbytes)) != nbytes)
						TDBPRINTF(TDB_ERROR,
							("could not read wakeup signal\n"));
			}
		}

		/* process input messages: */
		x11_processevent(inst);

		/* send out input messages to owners: */
		x11_sendimessages(inst);

		/* get signal state: */
		sig = TSetSignal(0, waitsigs);

	}
	while (!(sig & TTASK_SIG_ABORT));

	TDBPRINTF(TDB_INFO, ("Device instance exit\n"));

	x11_exitinstance(inst);
}
