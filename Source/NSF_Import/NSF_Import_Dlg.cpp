/*
** NSF Importer
** Copyright (C) 2011 Brad Smith (rainwarrior@gmail.com)
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

#include "../stdafx.h"
#include "../../resource1.h"
#include <cstdlib>
#include "NSF_Import_Dlg.h"
#include "NSF_Core.h"
#include "NSF_File.h"

static NSF_Import_Settings sSettings =
{
    1, // track 1
    120, // 120 seconds
    false // not using debug wav
};

static char sTempString[256];
static CNSFFile* sNSFFile = NULL;
static LPCSTR sFileName = NULL;

LRESULT CALLBACK NSF_Import_Dlg_Proc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch(Msg)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hWndDlg, IDC_FILENAME, sFileName);
        SetDlgItemText(hWndDlg, IDC_NSFINFO1, sNSFFile->szGameTitle);
        SetDlgItemText(hWndDlg, IDC_NSFINFO2, sNSFFile->szArtist);
        SetDlgItemText(hWndDlg, IDC_NSFINFO3, sNSFFile->szCopyright);
        SetDlgItemInt(hWndDlg, IDC_TRACK, sSettings.iTrack, FALSE);
        SetDlgItemInt(hWndDlg, IDC_TRACKCOUNT, sNSFFile->nTrackCount, FALSE);
        SetDlgItemInt(hWndDlg, IDC_SECONDS, sSettings.iSeconds, FALSE);
        SendDlgItemMessage(hWndDlg, IDC_PAL, BM_SETCHECK,
            sNSFFile->nIsPal == 1 ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessage(hWndDlg, IDC_DEBUGWAV, BM_SETCHECK,
            sSettings.bDebugWAV ? BST_CHECKED : BST_UNCHECKED, 0 );
        return TRUE;

    case WM_COMMAND:
        switch(wParam)
        {
        case IDOK:
            sSettings.iTrack = GetDlgItemInt(hWndDlg, IDC_TRACK, NULL, FALSE);
            sSettings.iSeconds = GetDlgItemInt(hWndDlg, IDC_SECONDS, NULL, FALSE);
            sSettings.bDebugWAV = (BST_CHECKED ==
                SendDlgItemMessage(hWndDlg, IDC_DEBUGWAV, BM_GETCHECK, 0, 0));
            EndDialog(hWndDlg, 0);
            return TRUE;
        case IDCANCEL:
            EndDialog(hWndDlg, -1);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

bool Do_NSF_Import_Dlg(LPCSTR FileName, CNSFFile& NSFFile, NSF_Import_Settings& Settings)
{
    sFileName = FileName;
    sNSFFile = &NSFFile;

    Settings = sSettings; // store old settings
    bool result = (0 ==
        DialogBox(NULL,
            MAKEINTRESOURCE(IDD_DIALOG_NSF_IMPORT),
            GetActiveWindow(),
            (DLGPROC)NSF_Import_Dlg_Proc));

    if (result) // true = OK
    {
        if (sSettings.iSeconds > (60 * 60)) sSettings.iSeconds = 120; // MAX 1hr
        if (sSettings.iTrack < 1) sSettings.iTrack = 1;
        if (sSettings.iTrack > NSFFile.nTrackCount) sSettings.iTrack = NSFFile.nTrackCount;
        Settings = sSettings;
    }
    else // false = CANCEL
    {
        sSettings = Settings; // put back old settings
    }
    return result;
}
