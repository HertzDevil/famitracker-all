/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2009  Jonathan Liss
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
#include "MIDIImportDialog.h"
#include ".\midiimportdialog.h"


// CMIDIImportDialog dialog

IMPLEMENT_DYNAMIC(CMIDIImportDialog, CDialog)
CMIDIImportDialog::CMIDIImportDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CMIDIImportDialog::IDD, pParent)
{
}

CMIDIImportDialog::~CMIDIImportDialog()
{
}

void CMIDIImportDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CMIDIImportDialog, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CMIDIImportDialog message handlers

INT_PTR CMIDIImportDialog::DoModal(int Channels)
{
	m_iChannels = Channels;

	return CDialog::DoModal();
}

BOOL CMIDIImportDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString Channels;
	int i;

	CComboBox *Chan1, *Chan2, *Chan3, *Chan4, *Chan5;

	Chan1 = (CComboBox*)GetDlgItem(IDC_CHANNEL1);
	Chan2 = (CComboBox*)GetDlgItem(IDC_CHANNEL2);
	Chan3 = (CComboBox*)GetDlgItem(IDC_CHANNEL3);
	Chan4 = (CComboBox*)GetDlgItem(IDC_CHANNEL4);
	Chan5 = (CComboBox*)GetDlgItem(IDC_CHANNEL5);

	for (i = 1; i < (m_iChannels + 1); i++) {
		Channels.Format("Channel %i", i);
		Chan1->AddString(Channels);
		Chan2->AddString(Channels);
		Chan3->AddString(Channels);
		Chan4->AddString(Channels);
		Chan5->AddString(Channels);
	}

	Chan1->AddString("(disable)");
	Chan2->AddString("(disable)");
	Chan3->AddString("(disable)");
	Chan4->AddString("(disable)");
	Chan5->AddString("(disable)");

	Chan1->SetCurSel(0);
	Chan2->SetCurSel(1);
	Chan3->SetCurSel(2);
	Chan4->SetCurSel(3);
	Chan5->SetCurSel(4);

	SetDlgItemText(IDC_PATLEN, "128");

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CMIDIImportDialog::OnBnClickedOk()
{	
	CComboBox *Chan1, *Chan2, *Chan3, *Chan4, *Chan5;

	Chan1 = (CComboBox*)GetDlgItem(IDC_CHANNEL1);
	Chan2 = (CComboBox*)GetDlgItem(IDC_CHANNEL2);
	Chan3 = (CComboBox*)GetDlgItem(IDC_CHANNEL3);
	Chan4 = (CComboBox*)GetDlgItem(IDC_CHANNEL4);
	Chan5 = (CComboBox*)GetDlgItem(IDC_CHANNEL5);

	m_iChannelMap[0] = Chan1->GetCurSel();
	m_iChannelMap[1] = Chan2->GetCurSel();
	m_iChannelMap[2] = Chan3->GetCurSel();
	m_iChannelMap[3] = Chan4->GetCurSel();
	m_iChannelMap[4] = Chan5->GetCurSel();

	m_iPatLen = GetDlgItemInt(IDC_PATLEN, NULL, FALSE);

	if (m_iPatLen < 1)
		m_iPatLen = 1;
	if (m_iPatLen > MAX_PATTERN_LENGTH)
		m_iPatLen = MAX_PATTERN_LENGTH;

	OnOK();
}
