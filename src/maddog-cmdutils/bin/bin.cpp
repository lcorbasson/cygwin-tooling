//bin.cpp			(part of the MaDdoG cmdline utils)
//manipulates Recycle Bin object
//created 11/98			magi -- modified from context.cpp


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


void ShowHelp (char* pszCmd);
bool EmptyBin (char* pszRoot, bool bSilent, bool bForce);
bool CalcBinSize (char* pszRoot);
char* FillSizeBuffer (char* pszBuf, __int64 nBytes, bool bLong);


int main (int argc, char** argv)
{
	bool bSilent = false;
	bool bForce = false;
	char* pszRoot = NULL;

	if (argc == 1 || !lstrcmp (argv[1], "/?") || !lstrcmp (argv[1], "-?"))
	{
		ShowHelp (argv[0]);
		return 1;
	}

	if (!SUCCEEDED(CoInitialize (NULL)))
	{
		printf ("\rBin: Cannot communicate with the shell because OLE cannot be initialized.\n");
		return -1;
	}

	for (int i = 2; i < argc; i++)
	{
		if (!strcmp (argv[i], "/force"))
			bForce = true;
		else if (!strcmp (argv[i], "/quiet"))
			bSilent = true;
		else
			pszRoot = argv[i];
	}

	if (!lstrcmp (argv[1], "/empty"))
	{
		if (!EmptyBin (pszRoot, bSilent, bForce))
			printf ("The recycle bin was not emptied.\n");
	}
	else if (!lstrcmp (argv[1], "/size"))
	{
		if (!CalcBinSize (pszRoot))
			printf ("Unable to calculate size -- perhaps the specified bin does not exist?\n");
	}
	else
	{
		ShowHelp (argv[0]);
		return 1;
	}

	CoUninitialize();

	return 0;
}

void ShowHelp (char* pszCmd)
{
	printf ("Bin v1.5 -- (c) 2000 Matt Ginzton, MaDdoG Software\n\n"
			"Manipulates the Windows Recycle Bin.\n\n"
			"Usage:\n"
			"%s /empty [/force] [/quiet] [drive:]\n"
			"\tEmpties the recycle bin for specified drive\n"
			"\t/force: do not ask for confirmation, just empty the bin\n"
			"\t/quiet: do not display progress information\n"
			"%s /size [drive:]\n"
			"\tDisplay the space used by the recycled bin on specified drive\n"
			"\n"
			"In all cases, if a drive is not specified, the command applies to all\n"
			"recycle bins for all drives.\n",
		pszCmd, pszCmd);
}


bool EmptyBin (char* pszRoot, bool bSilent, bool bForce)
{
	DWORD flags = SHERB_NOSOUND;

	if (bSilent)
		flags |= SHERB_NOPROGRESSUI;
	if (bForce)
		flags |= SHERB_NOCONFIRMATION;

	return (SUCCEEDED (SHEmptyRecycleBin (NULL, pszRoot, flags)));
}


bool CalcBinSize (char* pszRoot)
{
	SHQUERYRBINFO rbi;
	rbi.cbSize = sizeof (rbi);
	if (FAILED (SHQueryRecycleBin (pszRoot, &rbi)))
		return false;

	char szSize[100];
	FillSizeBuffer (szSize, rbi.i64Size, true);

	if (pszRoot == NULL)
		pszRoot = "for all drives";
	printf ("Statistics for recycle bin %s:\n"
			"Items:\t\t%I64d\n"
			"Space occupied:\t%s\n",
		pszRoot, rbi.i64NumItems, szSize);

	return true;
}


char* FillSizeBuffer (char* pszBuf, __int64 nBytes, bool bLong)
{
	char szNum[12];

	*pszBuf = 0;
	if (nBytes < 1024)
		bLong = TRUE;	//if it's this small, the long format IS the appropriate short format

	if (bLong)
	{
		//xx,xxx,xxx
		_ui64toa (nBytes, szNum, 10);
		int nDigits = lstrlen (szNum);
		char* psz = szNum;
		if (int nExtra = nDigits % 3)
		{
			lstrcpyn (pszBuf + lstrlen (pszBuf), szNum, nExtra + 1);
			psz += nExtra;
			nDigits -= nExtra;
			if (nDigits > 0)
				lstrcat (pszBuf, ",");
		}
		while (nDigits > 0)
		{
			lstrcpyn (pszBuf + lstrlen (pszBuf), psz, 3 + 1);
			psz += 3;
			nDigits -= 3;
			if (nDigits > 0)
				lstrcat (pszBuf, ",");
		}

		// bytes
		lstrcat (pszBuf, " bytes");
	}

	__int64 nScale = 1;
	while (nBytes/nScale > 1024)
	{
		nScale *= 1024;
	}

	if (nScale > 1)
	{
		if (bLong)
		{
			// (
			lstrcat (pszBuf, " (");
		}
		
		// xx.x Kb
		sprintf (szNum, "%.1lf %cb", (double)nBytes/nScale,
			nScale == 1024 ? 'K' :
			nScale == 1024 * 1024 ? 'M' :
			nScale == 1024 * 1024 * 1024 ? 'G' :
				'T');	//heaven forbid!
		lstrcat (pszBuf, szNum);

		if (bLong)
		{
			//)
			lstrcat (pszBuf, ")");
		}
	}

	return pszBuf;
}
