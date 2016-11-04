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
	m_pInstrument = (CInstrumentFDS*)GetDocument()->GetInstrument(Instrument);

	if (m_pWaveEditor)
		m_pWaveEditor->SetInstrument(m_pInstrument);

	if (m_pModSequenceEditor)
		m_pModSequenceEditor->SetInstrument(m_pInstrument);

	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_RATE_SPIN))->SetPos(m_pInstrument->GetModulationSpeed());
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_DEPTH_SPIN))->SetPos(m_pInstrument->GetModulationDepth());
	static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_MOD_DELAY_SPIN))->SetPos(m_pInstrument->GetModulationDelay());

//	CheckDlgButton(IDC_ENABLE_FM, m_pInstrument->GetModulationEnable() ? 1 : 0);

	EnableModControls(m_pInstrument->GetModulationEnable());
}


BEGIN_MESSAGE_MAP(CInstrumentEditorFDS, CInstrumentEditPanel)
	ON_COMMAND(IDC_PRESET_SINE, OnPresetSine)
	ON_COMMAND(IDC_PRESET_TRIANGLE, OnPresetTriangle)
	ON_COMMAND(IDC_PRESET_SAWTOOTH, OnPresetSawtooth)
	ON_COMMAND(IDC_PRESET_PULSE_50, OnPresetPulse50)
	ON_COMMAND(IDC_PRESET_PULSE_25, OnPresetPulse25)
	ON_COMMAND(IDC_MOD_PRESET_FLAT, OnModPresetFlat)
	ON_COMMAND(IDC_MOD_PRESET_SINE, OnModPresetSine)
	ON_WM_VSCROLL()
	ON_EN_CHANGE(IDC_MOD_RATE, OnModRateChange)
	ON_EN_CHANGE(IDC_MOD_DEPTH, OnModDepthChange)
	ON_EN_CHANGE(IDC_MOD_DELAY, OnModDelayChange)
	ON_BN_CLICKED(IDC_COPY_WAVE, &CInstrumentEditorFDS::OnBnClickedCopyWave)
	ON_BN_CLICKED(IDC_PASTE_WAVE, &CInstrumentEditorFDS::OnBnClickedPasteWave)
	ON_BN_CLICKED(IDC_COPY_TABLE, &CInstrumentEditorFDS::OnBnClickedCopyTable)
	ON_BN_CLICKED(IDC_PASTE_TABLE, &CInstrumentEditorFDS::OnBnClickedPasteTable)
//	ON_BN_CLICKED(IDC_ENABLE_FM, &CInstrumentEditorFDS::OnBnClickedEnableFm)
END_MESSAGE_MAP()

// CInstrumentEditorFDS message handlers

BOOL CInstrumentEditorFDS::OnInitDialog()
{
	CInstrumentEditPanel::OnInitDialog();

	// Create wave editor
	CRect rect(SX(20), SY(30), 0, 0);
	m_pWaveEditor = new CWaveEditorFDS(5, 2, 64, 64);
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
	for (int i = 0; i < 64; ++i) {
		float angle = (float(i) * 3.141592f * 2.0f) / 64.0f + 0.049087375f;
		int sample = int((sinf(angle) + 1.0f) * 31.5f + 0.5f);
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->RedrawWindow();
}

void CInstrumentEditorFDS::OnPresetTriangle()
{
	for (int i = 0; i < 64; ++i) {
		int sample = (i < 32 ? i << 1 : (63 - i) << 1);
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->RedrawWindow();
}

void CInstrumentEditorFDS::OnPresetPulse50()
{
	for (int i = 0; i < 64; ++i) {
		int sample = (i < 32 ? 0 : 63);
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->RedrawWindow();
}

void CInstrumentEditorFDS::OnPresetPulse25()
{
	for (int i = 0; i < 64; ++i) {
		int sample = (i < 16 ? 0 : 63);
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->RedrawWindow();
}

void CInstrumentEditorFDS::OnPresetSawtooth()
{
	for (int i = 0; i < 64; ++i) {
		int sample = i;
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->RedrawWindow();
}

void CInstrumentEditorFDS::OnModPresetFlat()
{
	for (int i = 0; i < 32; ++i) {
		m_pInstrument->SetModulation(i, 0);
	}

	m_pModSequenceEditor->RedrawWindow();
}

void CInstrumentEditorFDS::OnModPresetSine()
{
	for (int i = 0; i < 8; ++i) {
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
}

int CInstrumentEditorFDS::GetModRate() const 
{
	CString str;
	GetDlgItemText(IDC_MOD_RATE, str);
	str.Remove(-96);
	return atoi(str.GetBuffer());
}

void CInstrumentEditorFDS::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int ModSpeed = GetModRate();
	int ModDepth = GetDlgItemInt(IDC_MOD_DEPTH);
	int ModDelay = GetDlgItemInt(IDC_MOD_DELAY);

	m_pInstrument->SetModulationSpeed(ModSpeed);
}

void CInstrumentEditorFDS::OnModRateChange()
{
	if (m_pInstrument) {
		int ModSpeed = GetModRate();
		LIMIT(ModSpeed, 4095, 0);
		m_pInstrument->SetModulationSpeed(ModSpeed);
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
	// TODO: remove this

	if (pMsg->message == WM_USER) {
//		WaveChanged();
		return TRUE;
	}
	else if (pMsg->message == WM_USER + 1) {
//		WaveChanged();
		return TRUE;
	}
	else if (pMsg->message == WM_KEYDOWN) {
		if (pMsg->wParam == 13) {
			if (GetFocus() == GetDlgItem(IDC_WAVE)) {
//				ReadWaveString();
			}
			else if (GetFocus() == GetDlgItem(IDC_MODULATION)) {
//				ReadModString();
			}
		}
	}

	return CInstrumentEditPanel::PreTranslateMessage(pMsg);
}

void CInstrumentEditorFDS::PreviewNote(unsigned char Key)
{
	// Skip if text windows are selected
	CWnd *pFocus = GetFocus();
	if (pFocus != GetDlgItem(IDC_WAVE) && pFocus != GetDlgItem(IDC_MODULATION))
		static_cast<CFamiTrackerView*>(theApp.GetActiveView())->PreviewNote(Key);
}

void CInstrumentEditorFDS::OnBnClickedCopyWave()
{
	CString Str;

	// Assemble a MML string
	for (int i = 0; i < 64; ++i)
		Str.AppendFormat(_T("%i "), m_pInstrument->GetSample(i));

	if (!OpenClipboard())
		return;

	EmptyClipboard();

	int size = Str.GetLength() + 1;
	HANDLE hMem = GlobalAlloc(GMEM_MOVEABLE, size);
	LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hMem);  
	strcpy_s(lptstrCopy, size, Str.GetBuffer());
	GlobalUnlock(hMem);
	SetClipboardData(CF_TEXT, hMem);

	CloseClipboard();	
}

void CInstrumentEditorFDS::OnBnClickedPasteWave()
{
	// Copy from clipboard
	if (!OpenClipboard())
		return;

	HANDLE hMem = GetClipboardData(CF_TEXT);
	LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hMem);
	string str(lptstrCopy);
	GlobalUnlock(hMem);
	CloseClipboard();

	// Convert to register values
	istringstream values(str);
	istream_iterator<int> begin(values);
	istream_iterator<int> end;

	for (int i = 0; (i < 64) && (begin != end); ++i) {
		int value = *begin++;
		if (value >= 0 && value <= 63)
			m_pInstrument->SetSample(i, value);
	}

	m_pWaveEditor->RedrawWindow();
}

void CInstrumentEditorFDS::OnBnClickedCopyTable()
{
	CString Str;

	// Assemble a MML string
	for (int i = 0; i < 32; ++i)
		Str.AppendFormat(_T("%i "), m_pInstrument->GetModulation(i));

	if (!OpenClipboard())
		return;

	EmptyClipboard();

	int size = Str.GetLength() + 1;
	HANDLE hMem = GlobalAlloc(GMEM_MOVEABLE, size);
	LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hMem);  
	strcpy_s(lptstrCopy, size, Str.GetBuffer());
	GlobalUnlock(hMem);
	SetClipboardData(CF_TEXT, hMem);

	CloseClipboard();
}

void CInstrumentEditorFDS::OnBnClickedPasteTable()
{
	// Copy from clipboard
	if (!OpenClipboard())
		return;

	HANDLE hMem = GetClipboardData(CF_TEXT);
	LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hMem);
	string str(lptstrCopy);
	GlobalUnlock(hMem);
	CloseClipboard();

	// Convert to register values
	istringstream values(str);
	istream_iterator<int> begin(values);
	istream_iterator<int> end;

	for (int i = 0; (i < 32) && (begin != end); ++i) {
		int value = *begin++;
		if (value >= 0 && value <= 7)
			m_pInstrument->SetModulation(i, value);
	}

	m_pModSequenceEditor->RedrawWindow();
}

void CInstrumentEditorFDS::OnBnClickedEnableFm()
{
	/*
	UINT button = IsDlgButtonChecked(IDC_ENABLE_FM);

	EnableModControls(button == 1);
	*/
}

void CInstrumentEditorFDS::EnableModControls(bool enable)
{
	if (!enable) {
		GetDlgItem(IDC_MOD_RATE)->EnableWindow(FALSE);
		GetDlgItem(IDC_MOD_DEPTH)->EnableWindow(FALSE);
		GetDlgItem(IDC_MOD_DELAY)->EnableWindow(FALSE);
		m_pInstrument->SetModulationEnable(false);
	}
	else {
		GetDlgItem(IDC_MOD_RATE)->EnableWindow(TRUE);
		GetDlgItem(IDC_MOD_DEPTH)->EnableWindow(TRUE);
		GetDlgItem(IDC_MOD_DELAY)->EnableWindow(TRUE);
		m_pInstrument->SetModulationEnable(true);
	}
}