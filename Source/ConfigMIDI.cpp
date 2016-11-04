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
#include "./FamiTracker.h"
#include "ConfigMIDI.h"
#include "./MIDI.h"
#include "..\include\configmidi.h"


// CConfigMIDI dialog

IMPLEMENT_DYNAMIC(CConfigMIDI, CPropertyPage)
CConfigMIDI::CConfigMIDI()
	: CPropertyPage(CConfigMIDI::IDD)
{
}

CConfigMIDI::~CConfigMIDI()
{
}

void CConfigMIDI::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConfigMIDI, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_INDEVICES, OnCbnSelchangeDevices)
	ON_BN_CLICKED(IDC_MASTER_SYNC, OnBnClickedMasterSync)
	ON_BN_CLICKED(IDC_KEY_RELEASE, OnBnClickedKeyRelease)
	ON_BN_CLICKED(IDC_CHANMAP, OnBnClickedChanmap)
	ON_BN_CLICKED(IDC_VELOCITY, OnBnClickedVelocity)
	ON_BN_CLICKED(IDC_ARPEGGIATE, OnBnClickedArpeggiate)
	ON_CBN_SELCHANGE(IDC_OUTDEVICES, OnCbnSelchangeOutdevices)
END_MESSAGE_MAP()


// CConfigMIDI message handlers

BOOL CConfigMIDI::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CMIDI	*pMIDI = theApp.GetMIDI();
	int		NumDev, i;
	char	Text[256];

	CComboBox *InDevices = (CComboBox*)GetDlgItem(IDC_INDEVICES);
	CComboBox *OutDevices = (CComboBox*)GetDlgItem(IDC_OUTDEVICES);

	InDevices->AddString("<none>");
	OutDevices->AddString("<none>");

	// Input
	NumDev = pMIDI->GetNumDevices(true);

	for (i = 0; i < NumDev; i++) {
		pMIDI->GetDeviceString(i, Text, true);
		InDevices->AddString(Text);
	}

	// Output
	NumDev = pMIDI->GetNumDevices(false);

	for (i = 0; i < NumDev; i++) {
		pMIDI->GetDeviceString(i, Text, false);
		OutDevices->AddString(Text);
	}

	InDevices->SetCurSel(pMIDI->m_iDevice);
	OutDevices->SetCurSel(pMIDI->m_iOutDevice);

	CheckDlgButton(IDC_MASTER_SYNC, theApp.m_pSettings->Midi.bMidiMasterSync	? 1 : 0);
	CheckDlgButton(IDC_KEY_RELEASE, theApp.m_pSettings->Midi.bMidiKeyRelease	? 1 : 0);
	CheckDlgButton(IDC_CHANMAP,		theApp.m_pSettings->Midi.bMidiChannelMap	? 1 : 0);
	CheckDlgButton(IDC_VELOCITY,	theApp.m_pSettings->Midi.bMidiVelocity		? 1 : 0);
	CheckDlgButton(IDC_ARPEGGIATE,	theApp.m_pSettings->Midi.bMidiArpeggio		? 1 : 0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CConfigMIDI::OnApply()
{
	CComboBox	*InDevices	= (CComboBox*)GetDlgItem(IDC_INDEVICES);
	CComboBox	*OutDevices	= (CComboBox*)GetDlgItem(IDC_OUTDEVICES);
	CMIDI		*pMIDI		= theApp.GetMIDI();
	
	pMIDI->SetDevice(InDevices->GetCurSel(), IsDlgButtonChecked(IDC_MASTER_SYNC) != 0);
	pMIDI->SetOutDevice(OutDevices->GetCurSel());
	
	theApp.m_pSettings->Midi.bMidiMasterSync	= IsDlgButtonChecked(IDC_MASTER_SYNC)	== 1;
	theApp.m_pSettings->Midi.bMidiKeyRelease	= IsDlgButtonChecked(IDC_KEY_RELEASE)	== 1;
	theApp.m_pSettings->Midi.bMidiChannelMap	= IsDlgButtonChecked(IDC_CHANMAP)		== 1;
	theApp.m_pSettings->Midi.bMidiVelocity		= IsDlgButtonChecked(IDC_VELOCITY)		== 1;
	theApp.m_pSettings->Midi.bMidiArpeggio		= IsDlgButtonChecked(IDC_ARPEGGIATE)	== 1;

	return CPropertyPage::OnApply();
}

void CConfigMIDI::OnCbnSelchangeDevices()
{
	SetModified();
}

void CConfigMIDI::OnBnClickedMasterSync()
{
	SetModified();
}

void CConfigMIDI::OnBnClickedKeyRelease()
{
	SetModified();
}

void CConfigMIDI::OnBnClickedChanmap()
{
	SetModified();
}

void CConfigMIDI::OnBnClickedVelocity()
{
	SetModified();
}

void CConfigMIDI::OnBnClickedArpeggiate()
{
	SetModified();
}

void CConfigMIDI::OnCbnSelchangeOutdevices()
{
	SetModified();
}
