#ifndef _TEK_MOD_IO_H
#define _TEK_MOD_IO_H

/*
**	$Id: io.h,v 1.1.1.1 2006/08/20 22:15:25 tmueller Exp $
**	tek/mod/io.h - Filesystem I/O interface
*/

#include <tek/exec.h>

/*****************************************************************************/
/*
**	Forward declarations
*/

/* IO module base structure: */
struct TIOBase;

/*****************************************************************************/

/* File structure: */
typedef struct TIOPacket *TFILE;

#define TEOF	(-1)

/*
**	File attribute tags - these can be passed to
**	io_examine() and io_exnext() for attribute queries.
**
**	if only TFATTR_Size is specified, and the file is larger than
**	2^32-2 bytes, the size returned will be 0xffffffff.
*/

#define TFATTR_Name		(TTAG_USER + 0x4000)	/* Object name */
#define TFATTR_Type		(TTAG_USER + 0x4001)	/* Object type, see below */
#define TFATTR_Size		(TTAG_USER + 0x4002)	/* Size, in bytes */
#define TFATTR_Date		(TTAG_USER + 0x4003)	/* Object's TDATE */
#define TFATTR_DateBox	(TTAG_USER + 0x4004)	/* Datebox structure */
#define TFATTR_SizeHigh	(TTAG_USER + 0x4005)	/* High-order 32 bit of size */

/*
**	Object types
*/

#define TFTYPE_Unknown		0x00	/* Unknown object type */
#define TFTYPE_File			0x01	/* Regular file */
#define TFTYPE_Directory	0x02	/* Directory */
#define TFTYPE_Volume		0x12	/* Indicates a volume */

/*
**	Lock modes
*/

#define TFLOCK_NONE			0		/* No locking */
#define TFLOCK_READ			1		/* Shared lock */
#define TFLOCK_WRITE		2		/* Exclusive lock */

/*
**	Open modes
*/

#define TFMODE_OLDFILE		1		/* Existing file, read/write, not locked */
#define TFMODE_NEWFILE		2		/* New file, read/write, exclusive lock */
#define TFMODE_READONLY		3		/* Existing file, readonly, not locked */
#define TFMODE_READWRITE	4		/* Existing or new, read/write, shared */

/*
**	Seek modes
*/

#define TFPOS_CURRENT		0		/* Seek from current */
#define TFPOS_BEGIN			1		/* Seek from beginning */
#define TFPOS_END			(-1)	/* Seek from end */

/*
**	I/O Error codes -
**	see exec.h for device-specific codes
*/

#define TIOERR_NOT_ENOUGH_MEMORY	16

#define TIOERR_BAD_ARGUMENTS		32
#define TIOERR_INVALID_NAME			33
#define TIOERR_LINE_TOO_LONG		34		/* nameof, addpart... */
#define TIOERR_OUT_OF_RANGE			35		/* address range (seek...) */

#define TIOERR_DISK_FULL			48
#define TIOERR_DISK_WRITE_PROTECTED	49		/* physically */
#define TIOERR_DISK_NOT_READY		50
#define TIOERR_DISK_CORRUPT			51

#define TIOERR_OBJECT_NOT_FOUND		64
#define TIOERR_OBJECT_WRONG_TYPE	65
#define TIOERR_OBJECT_EXISTS		66
#define TIOERR_OBJECT_TOO_LARGE		67
#define TIOERR_OBJECT_IN_USE		68
#define TIOERR_ACCESS_DENIED		69		/* logically */

#define TIOERR_NO_MORE_ENTRIES		80		/* exnext */
#define TIOERR_NOT_SAME_DEVICE		81
#define TIOERR_DIRECTORY_NOT_EMPTY	82
#define TIOERR_TOO_MANY_LEVELS		83

/*
**	Name conversion modes
*/

#define TPPF_TEK2HOST		0x0012	/* convert to HOST naming convention */
#define TPPF_HOST2TEK		0x0021	/* convert to TEK naming convention */

/*
**	Tags for opening and mounting
*/

#define TIOMount_Handler	(TTAG_USER + 0x4100)	/* Name of handler */
#define TIOMount_HndVersion	(TTAG_USER + 0x4101)	/* Handler version */
#define TIOMount_Device		(TTAG_USER + 0x4102)	/* Name of Exec device */
#define TIOMount_InitString	(TTAG_USER + 0x4103)	/* Startup control */

#define TIOMount_IOBase		(TTAG_USER + 0x410f)	/* Internal use */
#define TIOMount_Extended	(TTAG_USER + 0x4110)	/* User-defined tags */

/* Tags for locking */

#define TIOLock_NamePart	(TTAG_USER + 0x4200)	/* Passed to handler */

/*
**	Mount action codes
*/

#define TIOMNT_REMOVE	0
#define TIOMNT_ADD		1

#endif
