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
#include "FamiTrackerDoc.h"
#include "ConfigSound.h"
#include "SoundGen.h"
#include "Settings.h"
#include "DirectSound.h"

// CConfigSound dialog

IMPLEMENT_DYNAMIC(CConfigSound, CPropertyPage)
CConfigSound::CConfigSound()
	: CPropertyPage(CConfigSound::IDD)
{
}

CConfigSound::~CConfigSound()
{
}

void CConfigSound::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConfigSound, CPropertyPage)
	ON_WM_HSCROLL()
	ON_CBN_SELCHANGE(IDC_SAMPLE_RATE, OnCbnSelchangeSampleRate)
	ON_CBN_SELCHANGE(IDC_SAMPLE_SIZE, OnCbnSelchangeSampleSize)
	ON_CBN_SELCHANGE(IDC_DEVICES, OnCbnSelchangeDevices)
END_MESSAGE_MAP()

int iDeviceIndex;

// CConfigSound message handlers

BOOL CConfigSound::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CString Text;

	CComboBox	*SampleRate, *SampleSize, *Devices;
	CSliderCtrl	*Slider, *BassSlider, *TrebleSliderFreq, *TrebleSliderDamping, *VolumeSlider;

	SampleRate	= (CComboBox*)GetDlgItem(IDC_SAMPLE_RATE);
	SampleSize	= (CComboBox*)GetDlgItem(IDC_SAMPLE_SIZE);
	Devices		= (CComboBox*)GetDlgItem(IDC_DEVICES);
	Slider		= (CSliderCtrl*)GetDlgItem(IDC_BUF_LENGTH);

	BassSlider			= (CSliderCtrl*)GetDlgItem(IDC_BASS_FREQ);
	TrebleSliderFreq	= (CSliderCtrl*)GetDlgItem(IDC_TREBLE_FREQ);
	TrebleSliderDamping = (CSliderCtrl*)GetDlgItem(IDC_TREBLE_DAMP);
	VolumeSlider		= (CSliderCtrl*)GetDlgItem(IDC_VOLUME);

	SampleRate->AddString(_T("48 000 Hz"));
	SampleRate->AddString(_T("44 100 Hz"));
	SampleRate->AddString(_T("22 050 Hz"));
	SampleRate->AddString(_T("11 025 Hz"));

	SampleSize->AddString(_T("16 bit"));
	SampleSize->AddString(_T("8 bit"));

	Slider->SetRange(1, 500);

	switch (theApp.GetSettings()->Sound.iSampleRate) {
		case 11025: SampleRate->SelectString(0, _T("11 025 Hz")); break;
		case 22050: SampleRate->SelectString(0, _T("22 050 Hz")); break;
		case 44100: SampleRate->SelectString(0, _T("44 100 Hz")); break;
		case 48000: SampleRate->SelectString(0, _T("48 000 Hz")); break;
	}

	switch (theApp.GetSettings()->Sound.iSampleSize) {
		case 16: SampleSize->SelectString(0, _T("16 bit")); break;
		case 8:	 SampleSize->SelectString(0, _T("8 bit")); break;
	}

	Slider->SetPos(theApp.GetSettings()->Sound.iBufferLength);

	Text.Format(_T("%i ms"), theApp.GetSettings()->Sound.iBufferLength);
	SetDlgItemText(IDC_BUF_LEN, Text);

	BassSlider->SetRange(16, 4000);
	TrebleSliderFreq->SetRange(20, 20000);
	TrebleSliderDamping->SetRange(0, 90);
	VolumeSlider->SetRange(0, 100);

	BassSlider->SetPos(theApp.GetSettings()->Sound.iBassFilter);
	TrebleSliderFreq->SetPos(theApp.GetSettings()->Sound.iTrebleFilter);
	TrebleSliderDamping->SetPos(theApp.GetSettings()->Sound.iTrebleDamping);
	VolumeSlider->SetPos(theApp.GetSettings()->Sound.iMixVolume);

	Text.Format(_T("%i Hz"), ((CSliderCtrl*)GetDlgItem(IDC_BASS_FREQ))->GetPos());
	SetDlgItemText(IDC_BASS_FREQ_T, Text);

	Text.Format(_T("%i Hz"), ((CSliderCtrl*)GetDlgItem(IDC_TREBLE_FREQ))->GetPos());
	SetDlgItemText(IDC_TREBLE_FREQ_T, Text);

	Text.Format(_T("-%i dB"), ((CSliderCtrl*)GetDlgItem(IDC_TREBLE_DAMP))->GetPos());
	SetDlgItemText(IDC_TREBLE_DAMP_T, Text);

	Text.Format(_T("%i %%"), ((CSliderCtrl*)GetDlgItem(IDC_VOLUME))->GetPos());
	SetDlgItemText(IDC_VOLUME_T, Text);

	CDSound *pDSound = theApp.GetSoundGenerator()->GetSoundInterface();
	int iCount = pDSound->GetDeviceCount();

	for (int i = 0; i < iCount; i++)
		Devices->AddString(pDSound->GetDeviceName(i));

//	Devices->SetCurSel(pDSound->MatchDeviceID(theApp.m_pSettings->strDevice.GetBuffer()));
	Devices->SetCurSel(theApp.GetSettings()->Sound.iDevice);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigSound::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CString Text;

	Text.Format(_T("%i ms"), ((CSliderCtrl*)GetDlgItem(IDC_BUF_LENGTH))->GetPos());
	SetDlgItemText(IDC_BUF_LEN, Text);

	Text.Format(_T("%i Hz"), ((CSliderCtrl*)GetDlgItem(IDC_BASS_FREQ))->GetPos());
	SetDlgItemText(IDC_BASS_FREQ_T, Text);

	Text.Format(_T("%i Hz"), ((CSliderCtrl*)GetDlgItem(IDC_TREBLE_FREQ))->GetPos());
	SetDlgItemText(IDC_TREBLE_FREQ_T, Text);

	Text.Format(_T("-%i dB"), ((CSliderCtrl*)GetDlgItem(IDC_TREBLE_DAMP))->GetPos());
	SetDlgItemText(IDC_TREBLE_DAMP_T, Text);

	Text.Format(_T("%i %%"), ((CSliderCtrl*)GetDlgItem(IDC_VOLUME))->GetPos());
	SetDlgItemText(IDC_VOLUME_T, Text);

	SetModified();

	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

BOOL CConfigSound::OnApply()
{
	CComboBox	*SampleRate, *SampleSize, *Devices;
	CSliderCtrl	*Slider;

	Devices = (CComboBox*)GetDlgItem(IDC_DEVICES);
	SampleRate = (CComboBox*)GetDlgItem(IDC_SAMPLE_RATE);
	SampleSize = (CComboBox*)GetDlgItem(IDC_SAMPLE_SIZE);
	Slider = (CSliderCtrl*)GetDlgItem(IDC_BUF_LENGTH);

	switch (SampleRate->GetCurSel()) {
		case 0: theApp.GetSettings()->Sound.iSampleRate = 11025; break;
		case 1: theApp.GetSettings()->Sound.iSampleRate = 22050; break;
		case 2: theApp.GetSettings()->Sound.iSampleRate = 44100; break;
		case 3: theApp.GetSettings()->Sound.iSampleRate = 48000; break;
	}

	switch (SampleSize->GetCurSel()) {
		case 0: theApp.GetSettings()->Sound.iSampleSize = 16; break;
		case 1: theApp.GetSettings()->Sound.iSampleSize = 8; break;
	}

	theApp.GetSettings()->Sound.iBufferLength = Slider->GetPos();

	theApp.GetSettings()->Sound.iBassFilter		= ((CSliderCtrl*)GetDlgItem(IDC_BASS_FREQ))->GetPos();
	theApp.GetSettings()->Sound.iTrebleFilter		= ((CSliderCtrl*)GetDlgItem(IDC_TREBLE_FREQ))->GetPos();
	theApp.GetSettings()->Sound.iTrebleDamping	= ((CSliderCtrl*)GetDlgItem(IDC_TREBLE_DAMP))->GetPos();
	theApp.GetSettings()->Sound.iMixVolume		= ((CSliderCtrl*)GetDlgItem(IDC_VOLUME))->GetPos();

	theApp.GetSettings()->Sound.iDevice			= Devices->GetCurSel();

	theApp.LoadSoundConfig();

	return CPropertyPage::OnApply();
}

void CConfigSound::OnCbnSelchangeSampleRate()
{
	SetModified();
}

void CConfigSound::OnCbnSelchangeSampleSize()
{
	SetModified();
}

void CConfigSound::OnCbnSelchangeDevices()
{
	SetModified();
}
