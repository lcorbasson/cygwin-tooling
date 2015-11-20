// DetachFromCommandPrompt.cpp		utility functions for CmdUtils
// created 5/27/2000				magi@cs.stanford.edu


/*  COPYLEFT NOTICE:
 *  This source file is one piece of CmdUtils.  CmdUtils source code
 *  has been released by its author as free software under the GPL.
 *  For updates and the complete collection, visit:
 *      http://www.maddogsw.com/
 *
 *  CmdUtils: a collection of command-line tool interfaces to the Win95 shell
 *  Copyright (C) 1996-2000 Matt Ginzton / MaDdoG Software
 *  original author: Matt Ginzton, magi@cs.stanford.edu
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  If this concept intrigues you, read http://www.gnu.org/copyleft/
 */


#include <windows.h>
#include <assert.h>
#include <vector>
#include "DetachFromCommandPrompt.h"


// implementation of DetachFromCommandPrompt:
// trick command prompt into thinking we've exited,
// so it doesn't wait for us.

bool DetachFromCommandPrompt (int& argc, char**& argv, bool& bHaveConsole)
{
	// Need to detach from console and run asynchronously -- we don't want the command line
	// to be waiting for the dialog... but it waits for the process it starts, and the props
	// dialogs belong to our process (unfortunately not Explorer), so our process has to
	// stick around.  Solution -- have two; the first one just calls us again with a flag
	// that says "this time I mean it" and then exits to fool the command prompt.  The second
	// one does all the work.

	// This function encapsulates the process by:
	// check argv[1] to see if we're first invocation or second invocation
	// If 1st: invoke 2nd and exit
	// If 2nd: hide our args and return

	if (lstrcmp (argv[1], ":synchronous:"))
	{
		char szPath[MAX_PATH + 1000];
		
		GetModuleFileName (NULL, szPath, MAX_PATH);
		lstrcat (szPath, " :synchronous:");
		for (int i = 1; i < argc; i++)
		{
			lstrcat (szPath, " \"");
			lstrcat (szPath, argv[i]);
			if (szPath[lstrlen(szPath) - 1] == '\\')
				lstrcat (szPath, "\\");		// don't want trailing backslash before quote
			lstrcat (szPath, "\"");
		}

#ifdef _DEBUG
		printf ("PropsFor: child cmdline: %s\n", szPath);
#endif
		WinExec (szPath, SW_SHOW);
		exit (0);
	}
	else
	{
		argv += 2;	// skip past app name
		argc -= 2;	// and :synch: arguments
		if (IsOs95())
		{
			// we've already tricked the NT command processor into resuming interactivity by spawning
			// another copy of ourselves, but it's not that simple in 95 -- the command processor
			// waits on every child process that uses the console.  So we have to detach and
			// use the GUI for any further output.
			FreeConsole();
			bHaveConsole = false;
		}
	}

	// output after this point WILL work if bHaveConsole is set (i.e., on WinNT)
	// but will overwrite the shell prompt, which has already been printed...
	// kind of like unix bg apps writing to stderr...
	// may not be a good idea to have error messages below this point.

	return true;
}


void ErrorMessage (bool bHaveConsole, char* pszMessage, char* pszOpt)
{
	char szBuf[1000];
	wsprintf (szBuf, pszMessage, pszOpt);

	if (bHaveConsole)
	{
		printf ("PropsFor: %s\n", szBuf);
	}
	else
	{
		MessageBox (NULL, szBuf, "PropsFor", 0);
	}
}



// implemenation of WaitForDialogs:
// wait for all our windows to close before we exit

typedef std::vector<HWND> WindowVector;
static BOOL CALLBACK FillWindowArray (HWND hWnd, LPARAM lParam);


void WaitForDialogs (int nMaxWindows)
{
	assert (nMaxWindows >= 1);
	Sleep (3000 * nMaxWindows);		//give system time to create windows

	//find all windows that belong to us
	WindowVector windows;
	EnumWindows ((WNDENUMPROC)FillWindowArray, (LPARAM)&windows);
	
	//wait on windows
	while (TRUE)
	{
		int i;
		for (i = 0; i < windows.size(); i++)
		{
			if (IsWindow (windows[i]))
				break;
		}
		
		if (i == windows.size())	//got to the end without finding a real window...
			break;	//so they're all dead, exit loop (and app)

		Sleep (2000);
	}
}


BOOL CALLBACK FillWindowArray (HWND hWnd, LPARAM lParam)
{
	WindowVector* pWindowList = (WindowVector*)lParam;

	DWORD idProcessWnd;
	GetWindowThreadProcessId (hWnd, &idProcessWnd);

	if (idProcessWnd == GetCurrentProcessId())
	{	//it's one of ours; add it to the list iff it's a Properties dialog
		//(we also "own" the command prompt window and some OLE windows)
		char szBuf[100];
		GetClassName (hWnd, szBuf, 100);
		
		if (!lstrcmp (szBuf, "#32770"))		//if it's a dialog
		{
			//GetWindowText (hWnd, szBuf, 100);
			//printf ("\rAdopting window %s\n", szBuf);

			pWindowList->push_back (hWnd);
		}
	}

	return TRUE;
}


bool IsOs95()
{
	OSVERSIONINFO osInfo;

	osInfo.dwOSVersionInfoSize = sizeof(osInfo);
	GetVersionEx (&osInfo);
	
	if (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		return false;

	return true;
}
