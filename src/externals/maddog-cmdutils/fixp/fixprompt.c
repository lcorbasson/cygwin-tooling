//fixp.c				(part of the MaDdoG cmdline utils)
//fixes prompt for Windows NT
//created 1/7/97		magi
//modified 1/13/98		magi -- fixed to actually work (send to console input)
//modified 5/16/2000	magi -- fixed crash on root directory


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


// This program:
// cmd.exe in WinNT sometimes reverts to using the short name in the prompt, if you cd
// to a short name or run a 16-bit app.  fixp fixes the prompt to use the real (long)
// name again.

// Strategy: by no means straightforward.  What we have to do is:
// a) get the long filename representation
// b) change the directory to that.
//
// Difficult because there is no direct way to do either.
// I have code below (GetLongName) to do a), because there is a way (FindFirstFile) to
// get the long equivalent of a filename (not a path), so we can get the long equivalent
// of the last component of the path, chop that off, repeat recursively until we have
// only the drive letter, then cat it all back together again.
//
// Then, we can't directly change the working directory of another process, but we know
// that our caller is a command prompt, which is a console window and responds to cd commands.
// So, we build up a cd command that will change to the long name of the current directory,
// then send this command to the console input buffer, one character at a time.  Since we exit,
// this input is read by the calling command prompt.

#include <stdio.h>
#include <windows.h>


void ShowHelp (char* pszName);
void CmdLineError (char* pszName);
void GetLongName (char* pszShort, char* pszLong);
void SendToConsole (char* pszCmd);

int main (int argc, char** argv)
{
	char szCurrentDir[MAX_PATH];
	char szLongDir[MAX_PATH];

	if (argc >= 2)
	{
		if (argv[1][0] == '/' || argv[1][0] == '-'
		 && argv[1][1] == '?')
			ShowHelp (argv[0]);
		else
			CmdLineError (argv[0]);

		return 1;
	}

	GetCurrentDirectory (sizeof(szCurrentDir), szCurrentDir);
	szLongDir[0] = 0;
	GetLongName (szCurrentDir, szLongDir);

#if 0
	printf ("%s\n", szLongDir);
	SetCurrentDirectory (szLongDir);		//unfortunately, has no effect on calling process
#endif

	SendToConsole ("cd ");
	SendToConsole (szLongDir);
	SendToConsole ("\015");					//chr$(13), in octal
	
	return 0;
}


void GetLongName (char* pszShort, char* pszLong)
{
	HANDLE hSearch;
	WIN32_FIND_DATA findDat;
	char szBuf[MAX_PATH];
	char* pLast;

	if (lstrlen (pszShort) > 3)
	{
		GetFullPathName (pszShort, sizeof(szBuf), szBuf, &pLast);
		if (pLast != NULL)
			pLast[-1] = 0;	//remove the last component
		GetLongName (szBuf, pszLong + lstrlen (pszLong));

		hSearch = FindFirstFile (pszShort, &findDat);
		lstrcat (pszLong, "\\");
		lstrcat (pszLong, findDat.cFileName);
		FindClose (hSearch);
	}
	else
	{
		lstrcpy (pszLong, pszShort);
	}
}


void SendToConsole (char* pszCmd)
{
	HANDLE hConsIn = GetStdHandle (STD_INPUT_HANDLE);
	DWORD numWritten;
	INPUT_RECORD data;

	data.EventType = KEY_EVENT;
	data.Event.KeyEvent.wRepeatCount = 1;
	data.Event.KeyEvent.dwControlKeyState = 0;
	data.Event.KeyEvent.wVirtualKeyCode = 0;
	data.Event.KeyEvent.wVirtualScanCode = 0;

	for (; *pszCmd; pszCmd++)
	{
		data.Event.KeyEvent.bKeyDown = TRUE;
		data.Event.KeyEvent.uChar.AsciiChar = *pszCmd;
		WriteConsoleInput (hConsIn, &data, 1, &numWritten);

		data.Event.KeyEvent.bKeyDown = FALSE;
		WriteConsoleInput (hConsIn, &data, 1, &numWritten);
	}
}


void ShowHelp (char* pszName)
{
	printf ("FixPrompt v1.5 -- (c) 2000 Matt Ginzton, MaDdoG Software\n\n"
			"Restores the NT command prompt to full long-filenames.\n"
			"Usage: %s\n", pszName);
}


void CmdLineError (char* pszName)
{
	printf ("Command line error -- use %s /? for help\n", pszName);
}
