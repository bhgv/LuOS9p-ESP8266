#ifndef _TEK_MOD_TIME_H
#define _TEK_MOD_TIME_H

/*
**	teklib/tek/mod/time.h - Exec time definitions
**
**	Written by Frank Pagels <copper at coplabs.org>
**	and Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>
#include <tek/mod/ioext.h>

/*****************************************************************************/
/*
**	Timer I/O request
*/

struct TTimeRequest
{
	/* I/O request header: */
	struct TIORequest ttr_Req;
	/* I/O request data: */
	union
	{
		/* Relative time: */
		TTIME ttr_Time;
		/* Absolute universal datetime: */
		TDATE ttr_Date;
		/* Absolute local datetime: */
		TDATE ttr_LocalDate;
		/* Timezone bias for the given date: */
		TTIME ttr_TZBias;
		/* Daylight saving bias for the given date: */
		TTIME ttr_DSBias;
	} ttr_Data;
};

/*****************************************************************************/
/*
**	Timer I/O commands
*/

/* Get system releative time: */
#define TTREQ_GETSYSTEMTIME		(TIOCMD_EXTENDED + 0)
/* Get universal datetime: */
#define	TTREQ_GETUNIVERSALDATE	(TIOCMD_EXTENDED + 1)
/* Get local datetime: */
#define	TTREQ_GETLOCALDATE		(TIOCMD_EXTENDED + 2)
/* Wait relative time: */
#define	TTREQ_WAITTIME			(TIOCMD_EXTENDED + 3)
/* Wait for universal date: */
#define	TTREQ_WAITUNIVERSALDATE	(TIOCMD_EXTENDED + 4)
/* Wait for local date: */
#define	TTREQ_WAITLOCALDATE		(TIOCMD_EXTENDED + 5)
/* Timezone bias for given local date [TODO]: */
#define TTREQ_GETTZFROMDATE 	(TIOCMD_EXTENDED + 6)
/* Daylight saving bias for given local date [TODO]: */
#define TTREQ_GETDSFROMDATE 	(TIOCMD_EXTENDED + 7)

#endif
