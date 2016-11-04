/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2010  Jonathan Liss
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include "stdafx.h"
#include <Dbghelp.h>
#include "Exception.h"
#include "FamiTracker.h"

// This file contains an unhandled exception handler
// It will dump a memory file

const TCHAR* FTM_DUMP = _T("crash_recover");
const TCHAR* MINIDUMP_FILE = _T("MiniDump.dmp");

//#ifdef ENABLE_CRASH_HANDLER

LONG WINAPI ExceptionHandler(__in struct _EXCEPTION_POINTERS *ep)
{
	TRACE("App: Crash handler called\n");	// though this should never happen when running the debugger

	HANDLE hFile = CreateFile(MINIDUMP_FILE, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 

	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))  {
		// Create the minidump 
		MINIDUMP_EXCEPTION_INFORMATION mdei; 
		mdei.ThreadId			= GetCurrentThreadId();
		mdei.ExceptionPointers	= ep;
		mdei.ClientPointers		= FALSE;

		MINIDUMP_TYPE mdt		= MiniDumpNormal;

		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, mdt, (ep != 0) ? &mdei : NULL, 0, 0);

		CloseHandle(hFile);
	}

	// Find a free filename
	CString DocDumpFile = FTM_DUMP;
	int counter = 1;

	while (GetFileAttributes(DocDumpFile + _T(".ftm")) != 0xFFFFFFFF)
		DocDumpFile.Format(_T("%s%i"), FTM_DUMP, counter++);

	DocDumpFile.Append(_T(".ftm"));

	// Display a message
	CString text;
	text.Format(_T("Unhandled exception %X.\n\n"), ep->ExceptionRecord->ExceptionCode);
	text.AppendFormat(_T("A memory dump file has been made (%s), please include this if you file a bug report.\n\n"), MINIDUMP_FILE);
	text.AppendFormat(_T("Current module will be saved as %s. (Please note that this operation might fail)\n\n"), DocDumpFile);
	text.Append(_T("Application will now close."));
	AfxMessageBox(text, MB_ICONSTOP);

	// Try to save the document
	CDocument *pDoc = theApp.GetActiveDocument();

	if (pDoc)
		pDoc->OnSaveDocument(DocDumpFile);

	return EXCEPTION_EXECUTE_HANDLER; 
}

//#endif

