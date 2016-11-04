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

#include <iterator> 
#include <string>
#include <sstream>
#include <cmath>

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "InstrumentEditPanel.h"
#include "InstrumentEditorFDS.h"
#include "MainFrm.h"

using namespace std;

// CInstrumentEditorFDS dialog

IMPLEMENT_DYNAMIC(CInstrumentEditorFDS, CInstrumentEditPanel)

CInstrumentEditorFDS::CInstrumentEditorFDS(CWnd* pParent) : CInstrumentEditPanel(CInstrumentEditorFDS::IDD, pParent),
	m_pWaveEditor(NULL), 
	m_pModSequenceEditor(NULL), 
	m_pInstrument(NULL)
{
}

CInstrumentEditorFDS::~CInstrumentEditorFDS()
{
	SAFE_RELEASE(m_pModSequenceEditor);
	SAFE_RELEASE(m_pWaveEditor);
}

void CInstrumentEditorFDS::DoDataExchange(CDataExchange* pDX)
{
	CInstrumentEditPanel::DoDataExchange(pDX);
}

void CInstrumentEditorFDS::SelectInstrument(int Instrument)
{
	CInstrumentFDS *pInst = (CInstrumentFDS*)GetDocument()->GetInstrument(Instrument);

	m_pInstrument = pInst;

	if (m_pWaveEditor)
		m_pWaveEditor->SetInstrument(m_pInstrument);

	if (m_pModSequenceEditor)
		m_pModSequenceEditor->SetInstrument(m_pInstrument);

	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_RATE_SPIN))->SetPos(pInst->GetModulationFreq());
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_DEPTH_SPIN))->SetPos(pInst->GetModulationDepth());
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_DELAY_SPIN))->SetPos(pInst->GetModulationDelay());

	WaveChanged();
}


BEGIN_MESSAGE_MAP(CInstrumentEditorFDS, CInstrumentEditPanel)
	ON_COMMAND(IDC_PRESET_SINE, OnPresetSine)
	ON_COMMAND(IDC_PRESET_TRIANGLE, OnPresetTriangle)
	ON_COMMAND(IDC_PRESET_SQUARE, OnPresetSquare)
	ON_COMMAND(IDC_PRESET_SAWTOOTH, OnPresetSawtooth)
	ON_COMMAND(IDC_MOD_PRESET_FLAT, OnModPresetFlat)
	ON_COMMAND(IDC_MOD_PRESET_SINE, OnModPresetSine)
	ON_WM_VSCROLL()
	ON_EN_CHANGE(IDC_MOD_RATE, OnModRateChange)
	ON_EN_CHANGE(IDC_MOD_DEPTH, OnModDepthChange)
	ON_EN_CHANGE(IDC_MOD_DELAY, OnModDelayChange)
END_MESSAGE_MAP()

// CInstrumentEditorFDS message handlers

BOOL CInstrumentEditorFDS::OnInitDialog()
{
	CInstrumentEditPanel::OnInitDialog();

	// Create wave editor
	CRect rect(SX(20), SY(30), 0, 0);
	m_pWaveEditor = new CWaveEditor(4, 2, 64, 64);
	m_pWaveEditor->CreateEx(WS_EX_CLIENTEDGE, NULL, _T(""), WS_CHILD | WS_VISIBLE, rect, this);
	m_pWaveEditor->ShowWindow(SW_SHOW);
	m_pWaveEditor->UpdateWindow();

	// Create modulation sequence editor
	rect = CRect(SX(10), SY(200), 0, 0);
	m_pModSequenceEditor = new CModSequenceEditor();
	m_pModSequenceEditor->CreateEx(WS_EX_CLIENTEDGE, NULL, _T(""), WS_CHILD | WS_VISIBLE, rect, this);
	m_pModSequenceEditor->ShowWindow(SW_SHOW);
	m_pModSequenceEditor->UpdateWindow();

	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_RATE_SPIN))->SetRange(0, 4095);
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_DEPTH_SPIN))->SetRange(0, 63);
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_DELAY_SPIN))->SetRange(0, 255);
/*
	CSliderCtrl *pModSlider;
	pModSlider = (CSliderCtrl*)GetDlgItem(IDC_MOD_FREQ);
	pModSlider->SetRange(0, 0xFFF);
*/
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditorFDS::OnPresetSine()
{
	for (int i = 0; i < 64; i++) {
		float angle = (float(i) * 3.141592f * 2.0f) / 64.0f + 0.049087375f;
		int sample = int((sinf(angle) + 1.0f) * 31.5f + 0.5f);
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->RedrawWindow();
	WaveChanged();
}

void CInstrumentEditorFDS::OnPresetTriangle()
{
	int sample;

	for (int i = 0; i < 64; i++) {
		sample = (i < 32 ? i << 1 : (63 - i) << 1);
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->RedrawWindow();
	WaveChanged();
}

void CInstrumentEditorFDS::OnPresetSquare()
{
	int sample;

	for (int i = 0; i < 64; i++) {
		sample = (i < 32 ? 0 : 63);
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->RedrawWindow();
	WaveChanged();
}

void CInstrumentEditorFDS::OnPresetSawtooth()
{
	int sample;

	for (int i = 0; i < 64; i++) {
		sample = i;
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->RedrawWindow();
	WaveChanged();
}

void CInstrumentEditorFDS::OnModPresetFlat()
{
	for (int i = 0; i < 32; i++) {
		m_pInstrument->SetModulation(i, 0);
	}

	m_pModSequenceEditor->RedrawWindow();
	WaveChanged();
}

void CInstrumentEditorFDS::OnModPresetSine()
{
	for (int i = 0; i < 8; i++) {
		m_pInstrument->SetModulation(i, 7);
		m_pInstrument->SetModulation(i + 8, 1);
		m_pInstrument->SetModulation(i + 16, 1);
		m_pInstrument->SetModulation(i + 24, 7);
	}

	m_pInstrument->SetModulation(7, 0);
	m_pInstrument->SetModulation(8, 0);
	m_pInstrument->SetModulation(9, 0);

	m_pInstrument->SetModulation(23, 0);
	m_pInstrument->SetModulation(24, 0);
	m_pInstrument->SetModulation(25, 0);

	m_pModSequenceEditor->RedrawWindow();
	WaveChanged();
}

void CInstrumentEditorFDS::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int ModFreq = GetDlgItemInt(IDC_MOD_RATE);
	int ModDepth = GetDlgItemInt(IDC_MOD_DEPTH);
	int ModDelay = GetDlgItemInt(IDC_MOD_DELAY);

	m_pInstrument->SetModulationFreq(ModFreq);
}

void CInstrumentEditorFDS::OnModRateChange()
{
	if (m_pInstrument) {
		int ModFreq = GetDlgItemInt(IDC_MOD_RATE);
		LIMIT(ModFreq, 4095, 0);
		m_pInstrument->SetModulationFreq(ModFreq);
	}
}

void CInstrumentEditorFDS::OnModDepthChange()
{
	if (m_pInstrument) {
		int ModDepth = GetDlgItemInt(IDC_MOD_DEPTH);
		LIMIT(ModDepth, 63, 0);
		m_pInstrument->SetModulationDepth(ModDepth);
	}
}

void CInstrumentEditorFDS::OnModDelayChange()
{
	if (m_pInstrument) {
		int ModDelay = GetDlgItemInt(IDC_MOD_DELAY);
		LIMIT(ModDelay, 255, 0);
		m_pInstrument->SetModulationDelay(ModDelay);
	}
}

BOOL CInstrumentEditorFDS::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_USER) {
		WaveChanged();
		return TRUE;
	}
	else if (pMsg->message == WM_USER + 1) {
		WaveChanged();
		return TRUE;
	}
	else if (pMsg->message == WM_KEYDOWN) {
		if (pMsg->wParam == 13) {
			if (GetFocus() == GetDlgItem(IDC_WAVE)) {
				ReadWaveString();
			}
			else if (GetFocus() == GetDlgItem(IDC_MODULATION)) {
				ReadModString();
			}
		}
	}

	return CInstrumentEditPanel::PreTranslateMessage(pMsg);
}

// Update wave and modulation string
void CInstrumentEditorFDS::WaveChanged()
{
	CString wave;

	for (int i = 0; i < 64; i++) {
		CString temp;
		temp.Format(_T("%i "), m_pInstrument->GetSample(i));
		wave.Append(temp);
	}

	SetDlgItemText(IDC_WAVE, wave);

	CString mod;

	for (int i = 0; i < 32; i++) {
		CString temp;
		temp.Format(_T("%i "), m_pInstrument->GetModulation(i));
		mod.Append(temp);
	}

	SetDlgItemText(IDC_MODULATION, mod);
}

void CInstrumentEditorFDS::ReadWaveString()
{
	CString in_str;
	string str;
	GetDlgItemText(IDC_WAVE, in_str);
	str.assign(in_str);
	istringstream values(str);
	istream_iterator<int> begin(values);
	istream_iterator<int> end;

	for (int i = 0; i < 64; i++) {
		int val = 0;
		if (begin != end)
			val = *begin++;
		val = (val < 0) ? 0 : ((val > 63) ? 63 : val);
		m_pInstrument->SetSample(i, val);
	}

	m_pWaveEditor->RedrawWindow();
	WaveChanged();
}

void CInstrumentEditorFDS::ReadModString()
{
	CString in_str;
	string str;
	GetDlgItemText(IDC_MODULATION, in_str);
	str.assign(in_str);
	istringstream values(str);
	istream_iterator<int> begin(values);
	istream_iterator<int> end;

	for (int i = 0; i < 32; i++) {
		int val = 0;
		if (begin != end)
			val = *begin++;
		val = (val < 0) ? 0 : ((val > 7) ? 7 : val);
		m_pInstrument->SetModulation(i, val);
	}

	m_pModSequenceEditor->RedrawWindow();
	WaveChanged();
}

void CInstrumentEditorFDS::PreviewNote(unsigned char Key)
{
	// Skip if text windows are selected
	CWnd *pFocus = GetFocus();
	if (pFocus != GetDlgItem(IDC_WAVE) && pFocus != GetDlgItem(IDC_MODULATION))
		static_cast<CFamiTrackerView*>(theApp.GetActiveView())->PreviewNote(Key);
}
