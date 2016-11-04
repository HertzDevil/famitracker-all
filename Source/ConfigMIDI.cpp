// ConfigMIDI.cpp : implementation file
//

#include "stdafx.h"
#include "./FamiTracker.h"
#include "ConfigMIDI.h"
#include "./MIDI.h"


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
END_MESSAGE_MAP()


// CConfigMIDI message handlers

BOOL CConfigMIDI::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CMIDI	*pMIDI = theApp.GetMIDI();
	int		NumDev, i;
	char	Text[256];

	CComboBox *Devices = (CComboBox*)GetDlgItem(IDC_DEVICES);

	NumDev = pMIDI->GetNumDevices();

	Devices->AddString("<none>");

	for (i = 0; i < NumDev; i++) {
		pMIDI->GetDeviceString(i, Text);
		Devices->AddString(Text);
	}

	Devices->SetCurSel(pMIDI->m_iDevice);

	CheckDlgButton(IDC_MASTER_SYNC, theApp.m_pSettings->Midi.bMidiMasterSync	? 1 : 0);
	CheckDlgButton(IDC_KEY_RELEASE, theApp.m_pSettings->Midi.bMidiKeyRelease	? 1 : 0);
	CheckDlgButton(IDC_CHANMAP,		theApp.m_pSettings->Midi.bMidiChannelMap	? 1 : 0);
	CheckDlgButton(IDC_VELOCITY,	theApp.m_pSettings->Midi.bMidiVelocity		? 1 : 0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CConfigMIDI::OnApply()
{
	CComboBox	*Devices	= (CComboBox*)GetDlgItem(IDC_DEVICES);
	CMIDI		*pMIDI		= theApp.GetMIDI();
	
	pMIDI->SetDevice(Devices->GetCurSel(), IsDlgButtonChecked(IDC_MASTER_SYNC) != 0);
	
	theApp.m_pSettings->Midi.bMidiMasterSync	= IsDlgButtonChecked(IDC_MASTER_SYNC)	== 1;
	theApp.m_pSettings->Midi.bMidiKeyRelease	= IsDlgButtonChecked(IDC_KEY_RELEASE)	== 1;
	theApp.m_pSettings->Midi.bMidiChannelMap	= IsDlgButtonChecked(IDC_CHANMAP)		== 1;
	theApp.m_pSettings->Midi.bMidiVelocity		= IsDlgButtonChecked(IDC_VELOCITY)		== 1;

	return CPropertyPage::OnApply();
}
