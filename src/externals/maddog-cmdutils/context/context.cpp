//context.cpp			(part of the MaDdoG cmdline utils)
//displays shell properties for a file object
//created 4/98			magi -- modified from pf.cpp
//modified 6/28/98		magi -- added text mode and IContextMenu2 support
//modified 11/11/98		magi -- fix Copy/Cut by using OLEInit
//modified 11/29/99		magi -- fix for when file argument is drive (ex. context c:\)
//modified 5/27/00		magi -- merge with PropsFor, fixes wildcards and gives common codebase


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
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "resource.h"
#include "DetachFromCommandPrompt.h"
#include "PidlFromFileSpec.h"


BOOL ObeyMenuCmd (IShellFolder* psf, LPCITEMIDLIST* aPidls, int cPidls, BOOL bTextMode);
void ShowHelp (char* pszCmd);
void WaitForDialogs (int nMaxWindows);
int ChooseFromTextMenu (HMENU hMenu);
char* RemoveAmpersands (char* string);
BOOL CALLBACK MenuHandlerDlgProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


int main (int argc, char** argv)
{
	BOOL bTextMode = FALSE;
	int nUsed = 1;	//for app name

	if (argc == 1 || !lstrcmp (argv[1], "/?") || !lstrcmp (argv[1], "-?"))
	{
		ShowHelp (argv[0]);
		return 1;
	}

	if (!lstrcmp (argv[1], "/text"))
	{
		bTextMode = TRUE;
		nUsed++;
	}

	argv += nUsed;	//skip past app name
	argc -= nUsed;	//and flags

	// initialize COM, *with* OLE clipboard support
	if (!SUCCEEDED(OleInitialize (NULL)))
	{
		printf ("\rContextMenu: Cannot communicate with the shell because OLE cannot be initialized.\n");
		return -1;
	}

	//now go build and invoke context menus
	for (int i = 0; i < argc; i++)
	{
		PidlVector pidls;
		IShellFolder* psfParent = ShellFolderPidlsFromFileSpec (argv[i], pidls);
		if (!psfParent || !ObeyMenuCmd (psfParent, const_cast<LPCITEMIDLIST*>(pidls.begin()), pidls.size(), bTextMode))
		{
			printf ("\rContextMenu: Unable to create menu for %s\n", argv[i]);
		}

		if (psfParent)
			psfParent->Release();
		FreePidls (pidls);
	}

	//detach from console to free command processor for other tasks while we wait...
	FreeConsole();
	WaitForDialogs (argc);	//don't exit until all props dialogs OK'd/Cancel'd, because they die when we do

	OleUninitialize();

#ifdef _DEBUG
	MessageBeep (0);		// debug exit confirmation
#endif

	return 0;
}

void ShowHelp (char* pszCmd)
{
	printf ("ContextMenu v1.5 -- (c) 2000 Matt Ginzton, MaDdoG Software\n\n"
			"Activates the shell context menu for the specified file(s).\n\n"
			"Usage: %s [/text] file1 [file2] [file3] ...\n"
			"\t/text: display menu as text on console\n"
			"\tA context menu is created for each named file.\n",
		pszCmd);
}


BOOL ObeyMenuCmd (IShellFolder* psf, LPCITEMIDLIST* aPidls, int cPidls, BOOL bTextMode)
{
	BOOL success = FALSE;

#if 0
	STRRET strret;
	strret.uType = STRRET_CSTR;
	psf->GetDisplayNameOf (pidl, SHGDN_FORPARSING, &strret);
	printf ("ContextMenu: Activating viewer for %s\n", strret.cStr);
#endif

	//get its context menu
	IContextMenu* pContextMenu;
	if (SUCCEEDED (psf->GetUIObjectOf (NULL, cPidls, aPidls, IID_IContextMenu, NULL, (void**)&pContextMenu)))
	{
		//try to get IContextMenu2 for SendTo
		IContextMenu2* pCM2;
		if (SUCCEEDED (pContextMenu->QueryInterface (IID_IContextMenu2, (LPVOID*)&pCM2)))
		{
			pContextMenu->Release();
			pContextMenu = (IContextMenu*)pCM2;
		}

		//prepare to invoke properties command
		CMINVOKECOMMANDINFO invoke;
		memset (&invoke, 0, sizeof(invoke));
		invoke.cbSize = sizeof(invoke);
		invoke.fMask = 0;
		invoke.hwnd = NULL;
		invoke.nShow = SW_SHOWNORMAL;

		HMENU hMenu = CreatePopupMenu();
		pContextMenu->QueryContextMenu (hMenu, 0, 0, 0x7fff, CMF_EXPLORE);

		int idCmd;
		if (bTextMode)
		{
			idCmd = ChooseFromTextMenu (hMenu);
		}
		else
		{
			HWND hDlg = CreateDialogParam (GetModuleHandle(NULL), MAKEINTRESOURCE (IDD_NULL), NULL, (DLGPROC)MenuHandlerDlgProc, (LPARAM)pContextMenu);
			POINT pt;
			::GetCursorPos (&pt);
			idCmd = ::TrackPopupMenu (hMenu, TPM_LEFTALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON, 
				pt.x, pt.y, 0, hDlg, NULL);
			DestroyWindow (hDlg);
			DestroyMenu (hMenu);
		}

		invoke.lpVerb = MAKEINTRESOURCE(idCmd);

		//invoke command
		if (SUCCEEDED (pContextMenu->InvokeCommand (&invoke)))
		{
			success = TRUE;
		}
		else
		{
			printf ("ContextMenu: command could not be carried out.\n");
		}

		pContextMenu->Release();
	}

	return success;
}


int ChooseFromTextMenu (HMENU hMenu)
{
	MENUITEMINFO mii;
	char szName[MAX_PATH];
	char* printableName;

	// display menu
	int nItems = ::GetMenuItemCount (hMenu);
	for (int iItem = 0; iItem < nItems; iItem++)
	{
		memset (&mii, 0, sizeof(mii));
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_SUBMENU | MIIM_TYPE;
		mii.dwTypeData = szName;
		mii.cch = sizeof(szName);
		::GetMenuItemInfo (hMenu, iItem, TRUE, &mii);
		
		if (mii.fType & MF_BITMAP)
			printableName = "(bitmap)";
		else if (mii.fType & MF_SEPARATOR)
			printableName = "------------------------";
		else
			printableName = RemoveAmpersands (mii.dwTypeData);

		printf ("%2d: %-20s %s\n", iItem+1, printableName, mii.hSubMenu ? "-->" : "");
	}

	int choice = -1;
	while (choice == -1)
	{
		printf ("Choice? ");
		scanf ("%d", &choice);
		if (choice > 0 && choice <= nItems)
		{
			choice--;	// convert to 0-based

			memset (&mii, 0, sizeof(mii));
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_SUBMENU | MIIM_ID | MIIM_TYPE;
			mii.dwTypeData = szName;
			mii.cch = 0;
			::GetMenuItemInfo (hMenu, choice, TRUE, &mii);

			if (mii.hSubMenu)
			{
				choice = ChooseFromTextMenu (mii.hSubMenu);
				break;
			}

			if (!(mii.fType & MF_SEPARATOR))
			{
				choice = mii.wID;
				break;
			}
		}

		printf ("Invalid choice; please try again.\n");
		choice = -1;
	}

	return choice;
}


char* RemoveAmpersands (char* string)
{
	for (char* psz = string; *psz != 0; psz++)
	{
		if (*psz == '&')
			memmove (psz, psz + 1, strlen (psz));
	}

	return string;
}


BOOL CALLBACK MenuHandlerDlgProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	IContextMenu2* pcm;

	//grab object pointer from window data -- we put it there in WM_CREATE
	pcm = (IContextMenu2*)::GetWindowLong (hWnd, GWL_USERDATA);

	switch (msg)
	{
	case WM_INITDIALOG:
		//get pointer to the IContextMenu2 on whose behalf we're acting
		pcm = (IContextMenu2*)lParam;		//passed to CreateDialog()
		//save it in window info
		::SetWindowLong (hWnd, GWL_USERDATA, (LONG)pcm);
		::ShowWindow (hWnd, SW_HIDE);
		break;

	//these are the important ones -- the messages that IContextMenu2::HandlMenuMsg is supposed to handle.
	case WM_DRAWITEM:
	case WM_MEASUREITEM:
	case WM_INITMENUPOPUP:
		if (pcm != NULL)
		{
			//pass on to callback
			pcm->HandleMenuMsg (msg, wParam, lParam);
		}

		//damn messages just HAVE to specify different return values, don't they!
		return (msg == WM_INITMENUPOPUP ? 0 : TRUE);
	}

	return TRUE;
}
