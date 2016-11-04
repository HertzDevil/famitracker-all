/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2007  Jonathan Liss
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
#include "FamiTracker.h"
#include "ExportDialog.h"
#include "FamitrackerDoc.h"
#include "Compiler.h"


// CExportDialog dialog

IMPLEMENT_DYNAMIC(CExportDialog, CDialog)
CExportDialog::CExportDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CExportDialog::IDD, pParent)
{
}

CExportDialog::~CExportDialog()
{
}

void CExportDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CExportDialog, CDialog)
	ON_BN_CLICKED(IDC_CLOSE, OnBnClickedClose)
	ON_BN_CLICKED(IDC_SAVENSF, OnBnClickedSavensf)
	ON_BN_CLICKED(IDC_SAVEBIN, OnBnClickedSavebin)
	ON_BN_CLICKED(IDC_SAVEPRG, OnBnClickedSaveprg)
	ON_BN_CLICKED(IDC_DEBUG, OnBnClickedDebug)
END_MESSAGE_MAP()


// CExportDialog message handlers

void CExportDialog::OnBnClickedClose()
{
	EndDialog(0);
}

void CExportDialog::OnBnClickedSavensf()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetDocument();
	CString			DefFileName = pDoc->GetTitle();
	CCompiler		Compile;

	// Get a proper file name
	if (DefFileName.Right(4).CompareNoCase(".ftm") == 0) {
		DefFileName = DefFileName.Left(DefFileName.GetLength() - 4);
		DefFileName.Append(".nsf");
	};

	CFileDialog FileDialog(FALSE, "nsf", DefFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "NSF song (*.nsf)|*.nsf|All files|*.*||");

	FileDialog.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_NSF);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	Compile.ExportNSF(FileDialog.GetPathName(), pDoc, (CEdit*)GetDlgItem(IDC_OUTPUT), (IsDlgButtonChecked(IDC_PAL) != 0));

	theApp.m_pSettings->SetPath(FileDialog.GetPathName(), PATH_NSF);
}

void CExportDialog::OnBnClickedSavebin()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetDocument();
//	CString			DefFileName = pDoc->GetTitle();
	CCompiler		Compile;
/*
	// Get a proper file name
	if (DefFileName.Right(4).CompareNoCase(".ftm") == 0) {
		DefFileName = DefFileName.Left(DefFileName.GetLength() - 4);
		DefFileName.Append(".bin");
	};
*/
	CFileDialog FileDialogMusic(FALSE, "bin", "music.bin", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Raw song data (*.bin)|*.bin|All files|*.*||");
	CFileDialog FileDialogSamples(FALSE, "bin", "samples.bin", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "DPCM sample bank (*.bin)|*.bin|All files|*.*||");

	FileDialogMusic.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_NSF);
	FileDialogSamples.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_NSF);

	if (FileDialogMusic.DoModal() == IDCANCEL)
		return;

	if (FileDialogSamples.DoModal() == IDCANCEL)
		return;

	Compile.ExportBIN(FileDialogMusic.GetPathName(), FileDialogSamples.GetPathName(), pDoc, (CEdit*)GetDlgItem(IDC_OUTPUT));

	theApp.m_pSettings->SetPath(FileDialogMusic.GetPathName(), PATH_NSF);
}

void CExportDialog::OnBnClickedSaveprg()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetDocument();
	CCompiler		Compile;

	CFileDialog FileDialog(FALSE, "prg", "music.prg", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "NES program bank (*.prg)|*.prg|All files|*.*||");

	FileDialog.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_NSF);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	Compile.ExportPRG(FileDialog.GetPathName(), pDoc, (CEdit*)GetDlgItem(IDC_OUTPUT), (IsDlgButtonChecked(IDC_PAL) != 0));

	theApp.m_pSettings->SetPath(FileDialog.GetPathName(), PATH_NSF);
}

void CExportDialog::OnBnClickedDebug()
{
	CCompiler Compile;
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();
//	Compile.Export("C:\\Documents and Settings\\Jonathan\\My Documents\\Visual Studio Projects\\FamiTracker\\NSF driver\\New version\\music", pDoc, (CEdit*)GetDlgItem(IDC_OUTPUT));
	Compile.Export("..\\NSF driver\\New version\\music", pDoc, (CEdit*)GetDlgItem(IDC_OUTPUT));
}

BOOL CExportDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();

	if (pDoc->GetMachine() == PAL)
		CheckDlgButton(IDC_PAL, 1);
	else
		CheckDlgButton(IDC_PAL, 0);

	SetDlgItemText(IDC_NAME, pDoc->GetSongName());
	SetDlgItemText(IDC_ARTIST, pDoc->GetSongArtist());
	SetDlgItemText(IDC_COPYRIGHT, pDoc->GetSongCopyright());

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
