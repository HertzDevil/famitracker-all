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
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDC_WRITEBIN, OnBnClickedWritebin)
END_MESSAGE_MAP()

// CNSFDialog message handlers

void CNSFDialog::OnBnClickedOk()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetDocument();
	CEdit *EditLog = (CEdit*)GetDlgItem(IDC_LOG);
	CString DefFileName = pDoc->GetTitle();
	CCompile Compiler;
	unsigned int InitOrg;
	bool BankSwitch = false, ForcePAL = false;

	//InitOrg = ReadStartAddress();
	InitOrg = MUSIC_ORIGIN;

	if (DefFileName.Right(4).CompareNoCase(".ftm") == 0) {
		DefFileName = DefFileName.Left(DefFileName.GetLength() - 4);
		DefFileName.Append(".nsf");
	};

	CFileDialog FileDialog(FALSE, "nsf", DefFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "NSF song (*.nsf)|*.nsf|All files|*.*||");

	if (IsDlgButtonChecked(IDC_BANKSWITCH) != 0)
		BankSwitch = true;

	if (IsDlgButtonChecked(IDC_PAL) != 0)
		ForcePAL = true;

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	SaveInfo();

	Compiler.BuildMusicData(InitOrg, pDoc);
	Compiler.CreateNSF(FileDialog.GetPathName(), pDoc, BankSwitch, ForcePAL);

	EditLog->SetWindowText(Compiler.GetLogOutput());
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

	SetDlgItemText(IDC_NAME, pDoc->m_strName);
	SetDlgItemText(IDC_ARTIST, pDoc->m_strArtist);
	SetDlgItemText(IDC_COPYRIGHT, pDoc->m_strCopyright);

	OrgString.Format("$%04X", MUSIC_ORIGIN);

	if (pDoc->m_iMachine == PAL) {
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

void CNSFDialog::OnBnClickedWritebin()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetDocument();
	CEdit *EditLog = (CEdit*)GetDlgItem(IDC_LOG);
	CString DefFileName = pDoc->GetTitle();
	CCompile Compiler;
	unsigned int InitOrg;

	InitOrg = ReadStartAddress();

	DefFileName = "musicdata.bin";

	CFileDialog FileDialog(FALSE, "bin", DefFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Binary song data (*.bin)|*.bin|All files|*.*||");

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	Compiler.BuildMusicData(InitOrg, pDoc);
	Compiler.CreateBIN(FileDialog.GetPathName(), pDoc);

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
