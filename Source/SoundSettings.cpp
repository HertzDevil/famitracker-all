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
#include "SoundSettings.h"
#include ".\soundsettings.h"
#include "..\include\soundsettings.h"


// CSoundSettings dialog

IMPLEMENT_DYNAMIC(CSoundSettings, CDialog)
CSoundSettings::CSoundSettings(CWnd* pParent /*=NULL*/)
	: CDialog(CSoundSettings::IDD, pParent)
{
}

CSoundSettings::~CSoundSettings()
{
}

void CSoundSettings::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSoundSettings, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_APPLY, OnBnClickedApply)
END_MESSAGE_MAP()


// CSoundSettings message handlers

void CSoundSettings::OnBnClickedOk()
{
	SaveSettings();
	OnOK();
}

BOOL CSoundSettings::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString Text;

	CComboBox	*SampleRate, *SampleSize;
	CSliderCtrl	*Slider, *BassSlider, *TrebleSliderFreq, *TrebleSliderDamping;

	SampleRate = (CComboBox*)GetDlgItem(IDC_SAMPLE_RATE);
	SampleSize = (CComboBox*)GetDlgItem(IDC_SAMPLE_SIZE);
	Slider = (CSliderCtrl*)GetDlgItem(IDC_BUF_LENGTH);

	BassSlider			= (CSliderCtrl*)GetDlgItem(IDC_BASS_FREQ);
	TrebleSliderFreq	= (CSliderCtrl*)GetDlgItem(IDC_TREBLE_FREQ);
	TrebleSliderDamping = (CSliderCtrl*)GetDlgItem(IDC_TREBLE_DAMP);

	SampleRate->AddString("48 000 Hz");
	SampleRate->AddString("44 100 Hz");
	SampleRate->AddString("22 050 Hz");
	SampleRate->AddString("11 025 Hz");

	SampleSize->AddString("16 bit");
	SampleSize->AddString("8 bit");

	Slider->SetRange(10, 200);

	switch (theApp.m_pSettings->Sound.iSampleRate) {
		case 11025: SampleRate->SelectString(0, "11 025 Hz"); break;
		case 22050: SampleRate->SelectString(0, "22 050 Hz"); break;
		case 44100: SampleRate->SelectString(0, "44 100 Hz"); break;
		case 48000: SampleRate->SelectString(0, "48 000 Hz"); break;
	}

	switch (theApp.m_pSettings->Sound.iSampleSize) {
		case 16:	SampleSize->SelectString(0, "16 bit"); break;
		case 8:		SampleSize->SelectString(0, "8 bit"); break;
	}

	Slider->SetPos(theApp.m_pSettings->Sound.iBufferLength);

	Text.Format("%i ms", theApp.m_pSettings->Sound.iBufferLength);
	SetDlgItemText(IDC_BUF_LEN, Text);

	BassSlider->SetRange(16, 4000);
	TrebleSliderFreq->SetRange(20, 20000);
	TrebleSliderDamping->SetRange(0, 90);

	BassSlider->SetPos(theApp.m_pSettings->Sound.iBassFilter);
	TrebleSliderFreq->SetPos(theApp.m_pSettings->Sound.iTrebleFilter);
	TrebleSliderDamping->SetPos(theApp.m_pSettings->Sound.iTrebleDamping);

	Text.Format("%i Hz", ((CSliderCtrl*)GetDlgItem(IDC_BASS_FREQ))->GetPos());
	SetDlgItemText(IDC_BASS_FREQ_T, Text);

	Text.Format("%i Hz", ((CSliderCtrl*)GetDlgItem(IDC_TREBLE_FREQ))->GetPos());
	SetDlgItemText(IDC_TREBLE_FREQ_T, Text);

	Text.Format("-%i dB", ((CSliderCtrl*)GetDlgItem(IDC_TREBLE_DAMP))->GetPos());
	SetDlgItemText(IDC_TREBLE_DAMP_T, Text);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSoundSettings::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CString Text;

	Text.Format("%i ms", ((CSliderCtrl*)GetDlgItem(IDC_BUF_LENGTH))->GetPos());
	SetDlgItemText(IDC_BUF_LEN, Text);

	Text.Format("%i Hz", ((CSliderCtrl*)GetDlgItem(IDC_BASS_FREQ))->GetPos());
	SetDlgItemText(IDC_BASS_FREQ_T, Text);

	Text.Format("%i Hz", ((CSliderCtrl*)GetDlgItem(IDC_TREBLE_FREQ))->GetPos());
	SetDlgItemText(IDC_TREBLE_FREQ_T, Text);

	Text.Format("-%i dB", ((CSliderCtrl*)GetDlgItem(IDC_TREBLE_DAMP))->GetPos());
	SetDlgItemText(IDC_TREBLE_DAMP_T, Text);

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CSoundSettings::OnBnClickedApply()
{
	SaveSettings();
}

void CSoundSettings::SaveSettings(void)
{
	CComboBox	*SampleRate, *SampleSize;
	CSliderCtrl	*Slider;

	SampleRate = (CComboBox*)GetDlgItem(IDC_SAMPLE_RATE);
	SampleSize = (CComboBox*)GetDlgItem(IDC_SAMPLE_SIZE);
	Slider = (CSliderCtrl*)GetDlgItem(IDC_BUF_LENGTH);

	switch (SampleRate->GetCurSel()) {
		case 0: theApp.m_pSettings->Sound.iSampleRate = 11025; break;
		case 1: theApp.m_pSettings->Sound.iSampleRate = 22050; break;
		case 2: theApp.m_pSettings->Sound.iSampleRate = 44100; break;
		case 3: theApp.m_pSettings->Sound.iSampleRate = 48000; break;
	}

	switch (SampleSize->GetCurSel()) {
		case 0: theApp.m_pSettings->Sound.iSampleSize = 16; break;
		case 1: theApp.m_pSettings->Sound.iSampleSize = 8; break;
	}

	//theApp.SetSettingBufferLength(Slider->GetPos());
	theApp.m_pSettings->Sound.iBufferLength = Slider->GetPos();

	theApp.m_pSettings->Sound.iBassFilter		= ((CSliderCtrl*)GetDlgItem(IDC_BASS_FREQ))->GetPos();
	theApp.m_pSettings->Sound.iTrebleFilter		= ((CSliderCtrl*)GetDlgItem(IDC_TREBLE_FREQ))->GetPos();
	theApp.m_pSettings->Sound.iTrebleDamping	= ((CSliderCtrl*)GetDlgItem(IDC_TREBLE_DAMP))->GetPos();

	theApp.LoadSoundConfig();
}
