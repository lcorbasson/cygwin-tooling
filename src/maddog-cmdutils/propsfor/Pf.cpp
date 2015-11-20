//pf.cpp				(part of the MaDdoG cmdline utils)
//displays shell properties for a file object
//created 12/96			magi
//modified much 1/97	magi -- got properties to display
//modified 1/23/98		magi -- fixed to fully work (need to keep process around until dialog OK'd)
//modified 1/23/98		magi -- fixed to detach properly from console
//modified 4/13/98		magi -- only NT worked; now fixed to detach properly from console in 95
//modified 11/11/98		magi -- didn't take /? option as documented
//modified 11/28/99		magi -- fix when argument is a drive (ie propsfor c:\)
//modified 5/27/00		magi -- fix wildcards on NT (much of this was written on 3/18/00)
//modified 5/27/00		magi -- source merge w/context.cpp: move common code (WRT context) to helper files
//modified 5/28/00		magi -- add /separate option


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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "DetachFromCommandPrompt.h"
#include "PidlFromFileSpec.h"


BOOL ObeyPropsCmd (IShellFolder* psf, LPCITEMIDLIST* aPidls, int cPidls);
void ShowHelp (char* pszCmd);


int main (int argc, char** argv)
{
	bool bHaveConsole = true;

	if (argc == 1 || !lstrcmp (argv[1], "/?") || !lstrcmp (argv[1], "-?"))
	{
		ShowHelp (argv[0]);
		return 1;
	}

	// don't make command prompt wait for us to exit
	// (if curious, see implementation of this function)
	DetachFromCommandPrompt (argc, argv, bHaveConsole);

	// output after this point WILL work if bHaveConsole is set (i.e., on WinNT)
	// but will overwrite the shell prompt, which has already been printed...
	// kind of like unix bg apps writing to stderr...
	// may not be a good idea to have error messages below this point.

	// initialize COM; don't need full OLE support
	if (!SUCCEEDED(CoInitialize (NULL)))
	{
		ErrorMessage (bHaveConsole, "Cannot communicate with the shell because OLE cannot be initialized.", "");
		return -1;
	}

	// process command line
	bool bSingle = false;
	if (argc && !lstrcmp (argv[0], "/separate"))
	{
		++argv; --argc;		// strip this argument
		bSingle = true;
	}

	// now go and invoke Properties for each argument
	for (int i = 0; i < argc; i++)
	{
		PidlVector pidls;
		IShellFolder* psfParent = ShellFolderPidlsFromFileSpec (argv[i], pidls);

		if (psfParent && pidls.size() > 0)
		{
			int nGroup = bSingle ? 1 : pidls.size();

			for (int i = 0; i < pidls.size(); i += nGroup)
			{
				if (!ObeyPropsCmd (psfParent, const_cast<LPCITEMIDLIST*>(&pidls[i]), nGroup))
				{
					ErrorMessage (bHaveConsole, "Unable to get properties for %s", argv[i]);
				}
			}
		}
		else
		{
			ErrorMessage (bHaveConsole, "Cannot find file %s", argv[i]);
		}

		if (psfParent)
			psfParent->Release();
		FreePidls (pidls);
	}

	// wait till user's done with us...
	WaitForDialogs (argc);	//don't exit until all props dialogs OK'd/Cancel'd, because they die when we do

	CoUninitialize();

#ifdef _DEBUG
	if (bHaveConsole)
		printf ("\nPropsFor: exit confimation\n\n");
	else
		MessageBeep (0);
#endif

	return 0;
}


void ShowHelp (char* pszCmd)
{
	printf ("PropsFor v1.5 -- (c) 2000 Matt Ginzton, MaDdoG Software\n\n"
			"Displays the shell properties dialog for a file object.\n\n"
			"Usage: %s [/separate] file1 [file2] [file3] ...\n"
			"\tThe shell properties are displayed for each named file.\n"
			"\tWildcards are allowed; one single Properties dialog will be displayed"
			"\tfor each filespec unless you specify /separate, in which case each"
			"\tfile in the wildcard expansion will get its own Properties dialog."
		, pszCmd);
}


BOOL ObeyPropsCmd (IShellFolder* psf, LPCITEMIDLIST* aPidls, int cPidls)
{
	BOOL success = FALSE;

#if 0
	STRRET strret;
	strret.uType = STRRET_CSTR;
	psf->GetDisplayNameOf (pidl, SHGDN_FORPARSING, &strret);
	printf ("PropsFor: Displaying properties for %s\n", strret.cStr);
#endif

	//get its context menu
	IContextMenu* pContextMenu;
	if (SUCCEEDED (psf->GetUIObjectOf (NULL, cPidls, aPidls, IID_IContextMenu, NULL, (void**)&pContextMenu)))
	{
		//prepare to invoke properties command
		CMINVOKECOMMANDINFO invoke;
		memset (&invoke, 0, sizeof(invoke));
		invoke.cbSize = sizeof(invoke);
		invoke.fMask = 0;
		invoke.hwnd = NULL;//GetDesktopWindow();
		invoke.nShow = SW_SHOWNORMAL;
		invoke.lpVerb = "Properties";	//MAKEINTRESOURCE(20-1) like EnumDesk?

		//invoke command
		if (SUCCEEDED (pContextMenu->InvokeCommand (&invoke)))
		{
			success = TRUE;
		}
		else
		{
			printf ("PropsFor: object does not have a properties command\n");
		}

		pContextMenu->Release();
	}

	return success;
}
