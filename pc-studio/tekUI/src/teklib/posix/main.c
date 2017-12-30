
/*
**	$Id: main.c,v 1.1.1.1 2006/08/20 22:15:20 tmueller Exp $
**	teklib/src/teklib/posix/main.c - Standalone application entrypoint
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <stdlib.h>
#include <tek/lib/init.h>

/*****************************************************************************/
/*
**	Implementation of POSIX main()
*/

int
main(int argc, char **argv)
{
	TAPTR apptask;
	TTAGITEM tags[4];
	TUINT retval = EXIT_FAILURE;

	tags[0].tti_Tag = TExecBase_ArgC;
	tags[0].tti_Value = (TTAG) argc;
	tags[1].tti_Tag = TExecBase_ArgV;
	tags[1].tti_Value = (TTAG) argv;
	tags[2].tti_Tag = TExecBase_RetValP;
	tags[2].tti_Value = (TTAG) &retval;
	tags[3].tti_Tag = TTAG_DONE;

	apptask = TEKCreate(tags);
	if (apptask)
	{
		retval = EXIT_SUCCESS;
		TEKMain(apptask);
		TDestroy(apptask);
	}

	return retval;
}
