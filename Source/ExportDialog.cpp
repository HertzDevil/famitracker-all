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
#include "FamiTracker.h"
#include "ExportDialog.h"
#include "FamitrackerDoc.h"
#include "Compiler.h"
#include "Settings.h"
#include "CustomExporters.h"

static int iExportOption = 0;	// Remember last option

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
	ON_BN_CLICKED(IDC_EXPORT, &CExportDialog::OnBnClickedExport)
END_MESSAGE_MAP()


// CExportDialog message handlers

void CExportDialog::OnBnClickedClose()
{
	EndDialog(0);
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

	CComboBox *pTypeBox = ((CComboBox*)GetDlgItem(IDC_TYPE));

	pTypeBox->SetCurSel(iExportOption);

	//Add selections for each custom plugin name
	CStringArray names;
	theApp.GetCustomExporters()->GetNames( names );

	for( int i = 0; i < names.GetCount(); ++i )
		pTypeBox->AddString( names[ i ] );

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CExportDialog::OnBnClickedExport()
{
	CComboBox *pTypeCombo = (CComboBox*)GetDlgItem(IDC_TYPE);

	unsigned char ListText[256];

	iExportOption = pTypeCombo->GetCurSel();

	pTypeCombo->GetLBText(iExportOption, (LPTSTR)&ListText);

	if (!_mbsnbcmp(ListText, (unsigned char*)"NSF", 3))
		CreateNSF();
	else if (!_mbsnbcmp(ListText, (unsigned char*)"NES", 3))
		CreateNES();
	else if (!_mbsnbcmp(ListText, (unsigned char*)"BIN", 3))
		CreateBIN();
	else if (!_mbsnbcmp(ListText, (unsigned char*)"PRG", 3))
		CreatePRG();
	else if (!_mbsnbcmp(ListText, (unsigned char*)"ASM", 3))
		CreateASM();
	else
	{
		//selection is the name of a custom exporter
		CreateCustom( CString( ListText ) );
	}
}

void CExportDialog::CreateNSF()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();
	CString			DefFileName = pDoc->GetTitle();
	CCompiler		Compiler;

	// Get a proper file name
	if (DefFileName.Right(4).CompareNoCase(".ftm") == 0) {
		DefFileName = DefFileName.Left(DefFileName.GetLength() - 4);
		DefFileName.Append(".nsf");
	};

	char Name[64];
	char Artist[64];
	char Copyright[64];

	GetDlgItemTextA(IDC_NAME, Name, 64);
	GetDlgItemTextA(IDC_ARTIST, Artist, 64);
	GetDlgItemTextA(IDC_COPYRIGHT, Copyright, 64);

	pDoc->SetSongInfo(Name, Artist, Copyright);

	CFileDialog FileDialog(FALSE, "nsf", DefFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "NSF song (*.nsf)|*.nsf|All files|*.*||");

	FileDialog.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_NSF);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	// Display wait cursor
	SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));

	Compiler.ExportNSF(FileDialog.GetPathName(), pDoc, (CEdit*)GetDlgItem(IDC_OUTPUT), (IsDlgButtonChecked(IDC_PAL) != 0));

	theApp.m_pSettings->SetPath(FileDialog.GetPathName(), PATH_NSF);
}

void CExportDialog::CreateNES()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();
	CString			DefFileName = pDoc->GetTitle();
	CCompiler		Compiler;

	// Get a proper file name
	if (DefFileName.Right(4).CompareNoCase(".ftm") == 0) {
		DefFileName = DefFileName.Left(DefFileName.GetLength() - 4);
		DefFileName.Append(".nes");
	};

	CFileDialog FileDialog(FALSE, "nes", DefFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "NES ROM image (*.nes)|*.nes|All files|*.*||");

	FileDialog.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_NSF);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	// Display wait cursor
	SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));

	Compiler.ExportNES(FileDialog.GetPathName(), pDoc, (CEdit*)GetDlgItem(IDC_OUTPUT), (IsDlgButtonChecked(IDC_PAL) != 0));

	theApp.m_pSettings->SetPath(FileDialog.GetPathName(), PATH_NSF);
}

void CExportDialog::CreateBIN()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();
//	CString			DefFileName = pDoc->GetTitle();
	CCompiler		Compiler;
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

	if (pDoc->GetSampleCount() > 0) {
		if (FileDialogSamples.DoModal() == IDCANCEL)
			return;
	}

	// Display wait cursor
	SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));

	Compiler.ExportBIN(FileDialogMusic.GetPathName(), FileDialogSamples.GetPathName(), pDoc, (CEdit*)GetDlgItem(IDC_OUTPUT));

	theApp.m_pSettings->SetPath(FileDialogMusic.GetPathName(), PATH_NSF);
}

void CExportDialog::CreatePRG()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();
	CCompiler		Compiler;

	CFileDialog FileDialog(FALSE, "prg", "music.prg", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "NES program bank (*.prg)|*.prg|All files|*.*||");

	FileDialog.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_NSF);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	// Display wait cursor
	SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));

	Compiler.ExportPRG(FileDialog.GetPathName(), pDoc, (CEdit*)GetDlgItem(IDC_OUTPUT), (IsDlgButtonChecked(IDC_PAL) != 0));

	theApp.m_pSettings->SetPath(FileDialog.GetPathName(), PATH_NSF);
}

void CExportDialog::CreateASM()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();
//	CString			DefFileName = pDoc->GetTitle();
	CCompiler		Compiler;
/*
	// Get a proper file name
	if (DefFileName.Right(4).CompareNoCase(".ftm") == 0) {
		DefFileName = DefFileName.Left(DefFileName.GetLength() - 4);
		DefFileName.Append(".bin");
	};
*/
	CFileDialog FileDialogMusic(FALSE, "asm", "music.asm", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Song data in text format (*.asm)|*.asm|All files|*.*||");
//	CFileDialog FileDialogSamples(FALSE, "bin", "samples.bin", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "DPCM sample bank (*.bin)|*.bin|All files|*.*||");

	FileDialogMusic.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_NSF);
//	FileDialogSamples.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_NSF);

	if (FileDialogMusic.DoModal() == IDCANCEL)
		return;
/*
	if (pDoc->GetSampleCount() > 0) {
		if (FileDialogSamples.DoModal() == IDCANCEL)
			return;
	}
*/
	// Display wait cursor
	SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));

	Compiler.ExportASM(FileDialogMusic.GetPathName(), pDoc, (CEdit*)GetDlgItem(IDC_OUTPUT));

	theApp.m_pSettings->SetPath(FileDialogMusic.GetPathName(), PATH_NSF);
}

void CExportDialog::CreateCustom( CString name )
{
	CFileDialog FileDialogCustom(FALSE, "asm", "music.asm", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Custom Song Data (*.asm)|*.asm|All files|*.*||");

	if(FileDialogCustom.DoModal() == IDCANCEL)
		return;

	theApp.GetCustomExporters()->SetCurrentExporter( name );
	CString fileName( FileDialogCustom.GetPathName() );		
	if(theApp.GetCustomExporters()->GetCurrentExporter().Export( (CFamiTrackerDoc const*)theApp.GetFirstDocument(), fileName ))
	{
		AfxMessageBox("Successfully exported!");
	}
}
