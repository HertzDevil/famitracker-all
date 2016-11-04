/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2006  Jonathan Liss
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
#include "NSFDialog.h"
#include "FamiTrackerDoc.h"
#include "Compile.h"
#include ".\nsfdialog.h"
#include "..\include\nsfdialog.h"

//#include "NewCompile.h"

// CNSFDialog dialog

IMPLEMENT_DYNAMIC(CNSFDialog, CDialog)
CNSFDialog::CNSFDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CNSFDialog::IDD, pParent)
{
}

CNSFDialog::~CNSFDialog()
{
}

void CNSFDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CNSFDialog, CDialog)
	ON_BN_CLICKED(IDC_WRITE_NSF, OnBnClickedWriteNSF)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDC_WRITE_BIN, OnBnClickedWriteBIN)
	ON_BN_CLICKED(IDC_WRITE_PRG, OnBnClickedWritePrg)
	ON_BN_CLICKED(IDC_BANKSWITCH, OnBnClickedBankswitch)
	ON_BN_CLICKED(IDC_BANKOPTIMIZE, OnBnClickedBankoptimize)
	ON_BN_CLICKED(IDC_NEW_DATA, OnBnClickedNewData)
END_MESSAGE_MAP()

// CNSFDialog message handlers

void CNSFDialog::OnBnClickedWriteNSF()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetDocument();
	CEdit *EditLog = (CEdit*)GetDlgItem(IDC_LOG);
	CString DefFileName = pDoc->GetTitle();
	CCompile Compiler;
	unsigned int InitOrg;
	bool BankSwitch = false, BankOptimize = false, ForcePAL = false;

	InitOrg = MUSIC_ORIGIN;

	if (DefFileName.Right(4).CompareNoCase(".ftm") == 0) {
		DefFileName = DefFileName.Left(DefFileName.GetLength() - 4);
		DefFileName.Append(".nsf");
	};

	CFileDialog FileDialog(FALSE, "nsf", DefFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "NSF song (*.nsf)|*.nsf|All files|*.*||");

	FileDialog.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_NSF);

	if (IsDlgButtonChecked(IDC_BANKOPTIMIZE) != 0)
		BankOptimize = true;

	if (IsDlgButtonChecked(IDC_BANKSWITCH) != 0)
		BankSwitch = true;

	if (IsDlgButtonChecked(IDC_PAL) != 0)
		ForcePAL = true;

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	SaveInfo();

	Compiler.BuildMusicData(InitOrg, BankSwitch, pDoc);
	Compiler.CreateNSF(FileDialog.GetPathName(), pDoc, BankOptimize, ForcePAL);

	EditLog->SetWindowText(Compiler.GetLogOutput());

	theApp.m_pSettings->SetPath(FileDialog.GetPathName(), PATH_NSF);
}

void CNSFDialog::OnBnClickedCancel()
{
	SaveInfo();
	OnCancel();
}

void CNSFDialog::SaveInfo()
{
	CFamiTrackerDoc	*pDoc = (CFamiTrackerDoc*)GetDocument();

	GetDlgItemText(IDC_NAME, m_strName, 32);
	GetDlgItemText(IDC_ARTIST, m_strArtist, 32);
	GetDlgItemText(IDC_COPYRIGHT, m_strCopyright, 32);

	pDoc->SetSongInfo(m_strName, m_strArtist, m_strCopyright);
}

BOOL CNSFDialog::OnInitDialog()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetDocument();
	CString OrgString;

	CDialog::OnInitDialog();

	SetDlgItemText(IDC_NAME, pDoc->GetSongName());
	SetDlgItemText(IDC_ARTIST, pDoc->GetSongArtist());
	SetDlgItemText(IDC_COPYRIGHT, pDoc->GetSongCopyright());

	OrgString.Format("$%04X", MUSIC_ORIGIN);

	if (pDoc->GetMachine() == PAL) {
		CheckDlgButton(IDC_PAL, 1);
	}

	((CComboBox*)GetDlgItem(IDC_ORG))->AddString(OrgString);
	((CComboBox*)GetDlgItem(IDC_ORG))->SelectString(0, OrgString);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

CDocument *CNSFDialog::GetDocument()
{
	CDocTemplate	*pDocTemp;
	POSITION		TempPos, DocPos;
	
	TempPos		= theApp.GetFirstDocTemplatePosition();
	pDocTemp	= theApp.GetNextDocTemplate(TempPos);
	DocPos		= pDocTemp->GetFirstDocPosition();

	return pDocTemp->GetNextDoc(DocPos);
}

void CNSFDialog::OnBnClickedWriteBIN()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetDocument();
	CEdit *EditLog = (CEdit*)GetDlgItem(IDC_LOG);
	CString DefFileName = "music.bin", DmcName = "samples.bin";
	CCompile Compiler;

	unsigned int InitOrg;

	InitOrg = ReadStartAddress();

	CFileDialog FileDialogBin(FALSE, "bin", DefFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Binary song data (*.bin)|*.bin|All files|*.*||");
	CFileDialog FileDialogDmc(FALSE, "bin", DmcName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Sample data (*.bin)|*.bin|All files|*.*||");

	if ((FileDialogBin.DoModal() == IDCANCEL) || (FileDialogDmc.DoModal() == IDCANCEL))
		return;

	Compiler.BuildMusicData(InitOrg, false, pDoc);
	Compiler.CreateBIN(FileDialogBin.GetPathName(), FileDialogDmc.GetPathName(), pDoc);

	EditLog->SetWindowText(Compiler.GetLogOutput());
}

int CNSFDialog::ReadStartAddress()
{
	char Buffer[256], *bp;
	unsigned int InitOrg;

	GetDlgItemText(IDC_ORG, Buffer, 256);

	bp = Buffer;
	if (*bp == '$')
		bp++;

	sscanf(bp, "%X", &InitOrg);

	return InitOrg;
}

void CNSFDialog::OnBnClickedWritePrg()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetDocument();
	CEdit *EditLog = (CEdit*)GetDlgItem(IDC_LOG);
	CString DefFileName = "prg.bin";
	CCompile Compiler;
	unsigned int InitOrg;
	bool ForcePAL = false;

	InitOrg = ReadStartAddress();

	CFileDialog FileDialog(FALSE, "bin", DefFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Binary song data (*.bin)|*.bin|All files|*.*||");

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	if (IsDlgButtonChecked(IDC_PAL) != 0)
		ForcePAL = true;

	Compiler.BuildMusicData(InitOrg, false, pDoc);
	Compiler.CreatePRG(FileDialog.GetPathName(), pDoc, ForcePAL);

	EditLog->SetWindowText(Compiler.GetLogOutput());
}

void CNSFDialog::OnBnClickedBankswitch()
{
	if (IsDlgButtonChecked(IDC_BANKSWITCH))
		GetDlgItem(IDC_BANKOPTIMIZE)->EnableWindow(FALSE);
	else
		GetDlgItem(IDC_BANKOPTIMIZE)->EnableWindow(TRUE);
}

void CNSFDialog::OnBnClickedBankoptimize()
{
	if (IsDlgButtonChecked(IDC_BANKOPTIMIZE))
		GetDlgItem(IDC_BANKSWITCH)->EnableWindow(FALSE);
	else
		GetDlgItem(IDC_BANKSWITCH)->EnableWindow(TRUE);
}

void CNSFDialog::OnBnClickedNewData()
{/*
	CNewCompile Compile;

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetDocument();

	Compile.Export("e:\\programmering\\net\\famitracker\\nsf driver\\new version\\music", pDoc, (CEdit*)GetDlgItem(IDC_LOG));
*/}
