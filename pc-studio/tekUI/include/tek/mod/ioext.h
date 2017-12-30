#ifndef _TEK_MOD_IOEXT_H
#define _TEK_MOD_IOEXT_H

/*
**	$Id: ioext.h,v 1.2 2006/09/10 14:36:03 tmueller Exp $
**	teklib/tek/mod/ioext.h - Device driver and filesystem handler interface
*/

#include <tek/exec.h>
#include <tek/mod/io.h>

struct TIORequest
{
	/* Node header: */
	struct TNode io_Node;
	/* Name of the object: */
	TSTRPTR io_Name;
	/* For future extensions: */
	TAPTR io_Reserved;
	/* Device/handler module/instance: */
	struct TModule *io_Device;
	/* Message port for responses: */
	struct TMsgPort *io_ReplyPort;
	/* Flags: */
	TUINT io_Flags;
	/* Error code: */
	TINT16 io_Error;
	/* Command code: */
	TUINT16 io_Command;
};

/*
**	Standard I/O request;
**	Device-specific extension
*/

struct TStdIORequest
{
	struct TIORequest io_Req;	/* I/O request header */
	TAPTR io_Data;				/* Pointer to data */
	TUINT io_Length;			/* Number of bytes requested */
	TUINT io_Actual;			/* Number of bytes transferred */
	TUINT io_Offs;				/* Low 32 bit offset on block device */
	TUINT io_OffsHi;			/* High 32 bit offset on block device */
};

/*
**	I/O handler packet;
**	Handler-specific extension
*/

struct TIOPacket
{
	struct TIORequest io_Req;	/* I/O request header */
	TAPTR io_FLock;				/* File/Lock handle */
	TAPTR io_Examine;			/* Examination handle */
	TAPTR io_Buffer;			/* I/O buffer handle */
	TUINT io_BufSize;			/* Handler-recommended I/O buffer size */
	TUINT io_BufFlags;			/* Handler-recommended buffer flags */
	union						/* Command arguments */
	{
		struct { TSTRPTR Name; TUINT Mode; TBOOL Result; TTAGITEM *Tags; } Open;
		struct { TBOOL Result; } Close;
		struct { TAPTR Buf; TINT Len; TINT RdLen; } Read;
		struct { TAPTR Buf; TINT Len; TINT WrLen; } Write;
		struct { TINT Offs; TINT *OffsHi; TINT Mode; TUINT Result; } Seek;
		struct { TSTRPTR Name; TUINT Mode; TBOOL Result; TTAGITEM *Tags; } Lock;
		struct { TBOOL Result; } OpenFromLock;
		struct { TTAGITEM *Tags; TINT Result; } Examine;
		struct { TSTRPTR SrcName; TSTRPTR DstName; TBOOL Result; } Rename;
		struct { TSTRPTR Name; TBOOL Result; } Delete;
		struct { TSTRPTR Name, Dest; TINT DLen, Mode; TTAGITEM *Tags; TINT Result; } MakeName;
		struct { TINT TimeOut; TINT Result; } WaitChar;
		struct { TBOOL Result; } Interactive;
		struct { TSTRPTR Name; TDATE *Date; TBOOL Result; } SetDate;
		struct { TSTRPTR Command; TSTRPTR Args; TUINT Flags; TINT Result; } Execute;

	} io_Op;
};

/*
**	flags
*/

/* perform I/O synchronously if possible: */
#define TIOF_QUICK			0x0001

/*
**	General I/O command codes
*/

#define TIOCMD_NIL			0x0000
#define TIOCMD_READ			0x0001
#define TIOCMD_WRITE		0x0002
#define TIOCMD_RESET		0x0003
#define TIOCMD_UPDATE		0x0004
#define TIOCMD_CLEAR		0x0005
#define TIOCMD_STOP			0x0006
#define TIOCMD_START		0x0007
#define TIOCMD_FLUSH		0x0008

/*
**	Extended I/O command codes
*/

#define TIOCMD_OPEN			0x0100
#define TIOCMD_CLOSE		0x0101
#define TIOCMD_LOCK			0x0102
#define TIOCMD_UNLOCK		0x0103
#define TIOCMD_OPENFROMLOCK	0x0104
#define TIOCMD_SEEK			0x0105
#define TIOCMD_EXAMINE		0x0106
#define TIOCMD_EXNEXT		0x0107
#define TIOCMD_RENAME		0x0108
#define TIOCMD_MAKEDIR		0x0109
#define TIOCMD_DELETE		0x010a
#define TIOCMD_MAKENAME		0x010b
#define TIOCMD_WAITCHAR		0x010c
#define TIOCMD_INTERACTIVE	0x010d
#define TIOCMD_SETDATE		0x010e
#define TIOCMD_EXECUTE		0x010f

#define TIOCMD_EXTENDED		0x1000

/*
**	Filesystem buffer modes and flags
*/

#define TIOBUF_UNDEFINED	0x0000
#define TIOBUF_NONE			0x0001		/* No buffering */
#define TIOBUF_LINE			0x0002		/* Line buffering */
#define TIOBUF_FULL			0x0003		/* Full buffering */
#define TIOBUF_BUFMODE		0x000f		/* Buffermode mask */

#define TIOBUF_READ			0x0010		/* Read access buffering */
#define TIOBUF_WRITE		0x0020		/* Write access buffering */
#define TIOBUF_ACCESSMODE	0x00f0		/* Accessmode mask */

#endif
