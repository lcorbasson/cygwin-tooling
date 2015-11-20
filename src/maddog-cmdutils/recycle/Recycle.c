//recycle.c				(part of the MaDdoG cmdline utils)
//sends files to the recycle bin
//created 1/7/97		magi
//modified 1/13/98		magi -- fixed to actually work (full pathnames)
//modified 6/28/98		magi -- added -f parameter


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
// mostly a straightforward call to SHFileOperation; the one quirk is undo information
// can be preserved (meaning we delete to the recycle bin instead of a normal delete)
// only if we pass the full pathname of each file.

#include <stdio.h>
#include <windows.h>
#include <stdlib.h>


void ShowHelp (char* pszName);
void CmdLineError (char* pszName);
BOOL RecycleFiles (char** filenames, int nFiles, BOOL bConfirmed);
BOOL IsOption (char* string, char option);


int main (int argc, char** argv)
{
	BOOL success;
	BOOL bConfirmed = FALSE;
	char* pszName;
	int nUsed;

	// get just filename from argv[0]
	pszName = strrchr (argv[0], '\\');
	if (pszName == NULL)
		pszName = argv[0];
	else
		pszName++;

	if (argc == 1)
	{
		CmdLineError (pszName);
		return 1;
	}

	if (IsOption (argv[1], '?'))
	{
		ShowHelp (pszName);
		return 1;
	}

	nUsed = 1;		// program name = argv[0]
	if (IsOption (argv[1], 'f'))
	{
		++nUsed;
		bConfirmed = TRUE;
	}

	success = RecycleFiles (argv+nUsed, argc-nUsed, bConfirmed);
	return (success ? 0 : 1);
}



BOOL IsOption (char* string, char option)
{
	return ((string[0] == '/' || string[0] == '-')
		  && tolower(string[1]) == tolower (option)
		  && string[2] == 0);
}

void ShowHelp (char* pszName)
{
	printf ("Recyle v1.5 -- (c) 2000 Matt Ginzton, MaDdoG Software\n\n"
			"Sends files to the Recycle bin.\n"
			"Usage: %s [-f] filename1 [filename2 ...]\n"
			"\t-f: Force recycle, no prompt for confirmation\n"
			"\tfilenames can include wildcards.\n", pszName);
}


void CmdLineError (char* pszName)
{
	printf ("Command line error -- use %s /? for help\n", pszName);
}


BOOL RecycleFiles (char** filenames, int nFiles, BOOL bConfirmed)
{
	SHFILEOPSTRUCT opRecycle;
	char* pszFilesToRecycle;
	char* pszNext;
	int i, len;
	BOOL success = TRUE;
	char szLongBuf[MAX_PATH];
	char* lastComponent;

	//fill filenames to delete
	len = 0;
	for (i = 0; i < nFiles; i++)
	{
		GetFullPathName (filenames[i], sizeof(szLongBuf), szLongBuf, &lastComponent);
		len += lstrlen (szLongBuf) + 1;
	}

	pszFilesToRecycle = malloc (len + 1);
	pszNext = pszFilesToRecycle;
	for (i = 0; i < nFiles; i++)
	{
		GetFullPathName (filenames[i], sizeof(szLongBuf), szLongBuf, &lastComponent);

		lstrcpy (pszNext, szLongBuf);
		pszNext += lstrlen (pszNext) + 1;		//advance past terminator
	}
	*pszNext = 0;		//double-terminate

	//fill fileop structure
	opRecycle.hwnd = NULL;
	opRecycle.wFunc = FO_DELETE;
	opRecycle.pFrom = pszFilesToRecycle;
	opRecycle.pTo = "\0\0";
	opRecycle.fFlags = FOF_ALLOWUNDO;
	if (bConfirmed)
		opRecycle.fFlags |= FOF_NOCONFIRMATION;
	opRecycle.lpszProgressTitle = "Recycling files...";

	if (0 != SHFileOperation (&opRecycle))
		success = FALSE;
	if (opRecycle.fAnyOperationsAborted)
		success = FALSE;

	free (pszFilesToRecycle);

	return success;
}
/*
 Dim sTempFilename As String * 100
     Dim sSendMeToTheBin As String
     lReturn = GetTempFileName("c:\", "VB_", 0, sTempFilename)
     sSendMeToTheBin = Left(sTempFilename, InStr(sTempFilename, Chr$(0)))
     With FileOperation
        .wFunc = FO_DELETE
        .pFrom = sSendMeToTheBin
        .fFlags = FOF_ALLOWUNDO
     End With
     lReturn = SHFileOperation(FileOperation)
   */