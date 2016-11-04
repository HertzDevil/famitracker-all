/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005  Jonathan Liss
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
#include "MIDIConfig.h"
#include ".\midiconfig.h"
#include "MIDI.h"

CMIDI *Parent;

// CMIDIConfig dialog

IMPLEMENT_DYNAMIC(CMIDIConfig, CDialog)
CMIDIConfig::CMIDIConfig(CWnd* pParent /*=NULL*/)
	: CDialog(CMIDIConfig::IDD, pParent)
{
}

CMIDIConfig::~CMIDIConfig()
{
}

void CMIDIConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CMIDIConfig, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CMIDIConfig message handlers

BOOL CMIDIConfig::OnInitDialog()
{
	CDialog::OnInitDialog();

	int NumDev, i;
	char Text[256];

	CComboBox *Devices = (CComboBox*)GetDlgItem(IDC_DEVICES);

	NumDev = Parent->GetNumDevices();

	Devices->AddString("<none>");

	for (i = 0; i < NumDev; i++) {
		Parent->GetDeviceString(i, Text);
		Devices->AddString(Text);
	}

	Devices->SetCurSel(Parent->m_iDevice);

	CheckDlgButton(IDC_MASTER_SYNC, theApp.m_bMidiMasterSync	? 1 : 0);
	CheckDlgButton(IDC_KEY_RELEASE, theApp.m_bMidiKeyRelease	? 1 : 0);
	CheckDlgButton(IDC_CHANMAP,		theApp.m_bMidiChannelMap	? 1 : 0);
	CheckDlgButton(IDC_VELOCITY,	theApp.m_bMidiVelocity		? 1 : 0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CMIDIConfig::OpenDialog(CMIDI *pParent)
{
	Parent = pParent;

	DoModal();
}
void CMIDIConfig::OnBnClickedOk()
{
	CComboBox *Devices = (CComboBox*)GetDlgItem(IDC_DEVICES);
	Parent->SetDevice(Devices->GetCurSel(), IsDlgButtonChecked(IDC_MASTER_SYNC) != 0);
	theApp.m_bMidiMasterSync	= IsDlgButtonChecked(IDC_MASTER_SYNC)	== 1;
	theApp.m_bMidiKeyRelease	= IsDlgButtonChecked(IDC_KEY_RELEASE)	== 1;
	theApp.m_bMidiChannelMap	= IsDlgButtonChecked(IDC_CHANMAP)		== 1;
	theApp.m_bMidiVelocity		= IsDlgButtonChecked(IDC_VELOCITY)		== 1;
	OnOK();
}
