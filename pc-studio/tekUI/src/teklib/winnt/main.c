
/*
**	$Id: main.c,v 1.1 2006/08/25 21:23:42 tmueller Exp $
**	boot/win32/main.c - Win32 console application entrypoint
*/

#include <tek/lib/init.h>
#include <stdlib.h>
#include <windows.h>

/*****************************************************************************/
/*
**	Get console window handle
*/

#define BUFSIZE 1024

static TAPTR
GetConsoleHwnd(void)
{
	HWND win;
	char pszNewWindowTitle[BUFSIZE];
    char pszOldWindowTitle[BUFSIZE];
	GetConsoleTitle(pszOldWindowTitle, BUFSIZE);
	wsprintf(pszNewWindowTitle, "%d/%d",
		GetTickCount(), GetCurrentProcessId());
	SetConsoleTitle(pszNewWindowTitle);
	Sleep(40);
	win = FindWindow(NULL, pszNewWindowTitle);
    SetConsoleTitle(pszOldWindowTitle);
	return (TAPTR) win;
}

/*****************************************************************************/
/*
**	Implement host main()
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
		TAPTR exec = TGetExecBase(apptask);
		TAPTR atom = TExecLockAtom(exec, "win32.hwnd",
			TATOMF_CREATE | TATOMF_NAME);
		if (atom)
		{
			TExecSetAtomData(exec, atom, (TTAG) GetConsoleHwnd());
			TExecUnlockAtom(exec, atom, TATOMF_KEEP);
		}

		retval = EXIT_SUCCESS;
		TEKMain(apptask);

		TExecLockAtom(exec, "win32.hwnd", TATOMF_NAME | TATOMF_DESTROY);

		TDestroy(apptask);
	}

	return retval;
}
