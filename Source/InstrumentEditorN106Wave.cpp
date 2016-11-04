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
#include "InstrumentEditorN106Wave.h"
#include "MainFrm.h"

using namespace std;

// CInstrumentEditorN106Wave dialog

IMPLEMENT_DYNAMIC(CInstrumentEditorN106Wave, CInstrumentEditPanel)

CInstrumentEditorN106Wave::CInstrumentEditorN106Wave(CWnd* pParent) : CInstrumentEditPanel(CInstrumentEditorN106Wave::IDD, pParent),
	m_pWaveEditor(NULL), 
	m_pInstrument(NULL)
{
}

CInstrumentEditorN106Wave::~CInstrumentEditorN106Wave()
{
	SAFE_RELEASE(m_pWaveEditor);
}

void CInstrumentEditorN106Wave::DoDataExchange(CDataExchange* pDX)
{
	CInstrumentEditPanel::DoDataExchange(pDX);
}

void CInstrumentEditorN106Wave::SelectInstrument(int Instrument)
{
	m_pInstrument = (CInstrumentN106*)GetDocument()->GetInstrument(Instrument);

	if (m_pWaveEditor)
		m_pWaveEditor->SetInstrument(m_pInstrument);

	CComboBox *pSizeBox = (CComboBox*)GetDlgItem(IDC_WAVE_SIZE);
	CComboBox *pPosBox = (CComboBox*)GetDlgItem(IDC_WAVE_POS);

	CString SizeStr;
	SizeStr.Format(_T("%i"), m_pInstrument->GetWaveSize());
	pSizeBox->SelectString(0, SizeStr);

	FillSizeBox(m_pInstrument->GetWaveSize());

	CString PosStr;
	PosStr.Format(_T("%i"), m_pInstrument->GetWavePos());
	pPosBox->SetWindowText(PosStr);
}

BEGIN_MESSAGE_MAP(CInstrumentEditorN106Wave, CInstrumentEditPanel)
	ON_COMMAND(IDC_PRESET_SINE, OnPresetSine)
	ON_COMMAND(IDC_PRESET_TRIANGLE, OnPresetTriangle)
	ON_COMMAND(IDC_PRESET_SAWTOOTH, OnPresetSawtooth)
	ON_COMMAND(IDC_PRESET_PULSE_50, OnPresetPulse50)
	ON_COMMAND(IDC_PRESET_PULSE_25, OnPresetPulse25)
	ON_MESSAGE(WM_USER, OnWaveChanged)
	ON_BN_CLICKED(IDC_COPY, OnBnClickedCopy)
	ON_BN_CLICKED(IDC_PASTE, OnBnClickedPaste)
	ON_CBN_SELCHANGE(IDC_WAVE_SIZE, OnWaveSizeChange)
	ON_CBN_EDITCHANGE(IDC_WAVE_POS, OnWavePosChange)
	ON_CBN_SELCHANGE(IDC_WAVE_POS, OnWavePosSelChange)
END_MESSAGE_MAP()

// CInstrumentEditorN106Wave message handlers

BOOL CInstrumentEditorN106Wave::OnInitDialog()
{
	CInstrumentEditPanel::OnInitDialog();

	// Create wave editor
	CRect rect(SX(20), SY(30), 0, 0);
	m_pWaveEditor = new CWaveEditorN106(10, 8, 32, 16);
	m_pWaveEditor->CreateEx(WS_EX_CLIENTEDGE, NULL, _T(""), WS_CHILD | WS_VISIBLE, rect, this);
	m_pWaveEditor->ShowWindow(SW_SHOW);
	m_pWaveEditor->UpdateWindow();

	CComboBox *pSize = (CComboBox*)GetDlgItem(IDC_WAVE_SIZE);

	for (int i = 0; i < 8; ++i)  {
		CString a;
		a.Format(_T("%i"), (8 - i) * 4);
		pSize->AddString(a);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditorN106Wave::OnPresetSine()
{
	int size = m_pInstrument->GetWaveSize();

	for (int i = 0; i < size; ++i) {
		float angle = (float(i) * 3.141592f * 2.0f) / float(size) + 0.049087375f;
		int sample = int((sinf(angle) + 1.0f) * 7.5f + 0.5f);
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->WaveChanged();
}

void CInstrumentEditorN106Wave::OnPresetTriangle()
{
	int size = m_pInstrument->GetWaveSize();

	for (int i = 0; i < size; ++i) {
		int sample = (i < (size / 2) ? i : ((size - 1) - i));
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->WaveChanged();
}

void CInstrumentEditorN106Wave::OnPresetPulse50()
{
	int size = m_pInstrument->GetWaveSize();

	for (int i = 0; i < size; ++i) {
		int sample = (i < (size / 2) ? 0 : 15);
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->WaveChanged();
}

void CInstrumentEditorN106Wave::OnPresetPulse25()
{
	int size = m_pInstrument->GetWaveSize();

	for (int i = 0; i < size; ++i) {
		int sample = (i < (size / 4) ? 0 : 15);
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->WaveChanged();
}

void CInstrumentEditorN106Wave::OnPresetSawtooth()
{
	int size = m_pInstrument->GetWaveSize();

	for (int i = 0; i < size; ++i) {
		int sample = (i * 16) / size;
		m_pInstrument->SetSample(i, sample);
	}

	m_pWaveEditor->WaveChanged();
}

void CInstrumentEditorN106Wave::PreviewNote(unsigned char Key)
{
	// Skip if text windows are selected
	CWnd *pFocus = GetFocus();
	if (pFocus != GetDlgItem(IDC_WAVE) && pFocus != GetDlgItem(IDC_MODULATION))
		static_cast<CFamiTrackerView*>(theApp.GetActiveView())->PreviewNote(Key);
}

void CInstrumentEditorN106Wave::OnBnClickedCopy()
{
	CString Str;

	int len = m_pInstrument->GetWaveSize();

	//Str.Format(_T("%i, "), m_pInstrument->GetSamplePos());

	// Assemble a MML string
	for (int i = 0; i < len; ++i)
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

void CInstrumentEditorN106Wave::OnBnClickedPaste()
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
		if (value >= 0 && value <= 15)
			m_pInstrument->SetSample(i, value);
	}

	m_pWaveEditor->WaveChanged();
}

LRESULT CInstrumentEditorN106Wave::OnWaveChanged(WPARAM wParam, LPARAM lParam)
{
	CString str;
	int Size = m_pInstrument->GetWaveSize();
	for (int i = 0; i < Size; ++i) {
		str.AppendFormat(_T("%i "), m_pInstrument->GetSample(i));
	}
	SetDlgItemText(IDC_MML, str);
	return 0;
}

void CInstrumentEditorN106Wave::OnWaveSizeChange()
{
	int size;
	BOOL trans;
	size = GetDlgItemInt(IDC_WAVE_SIZE, &trans, FALSE);
	size = (size / 4) * 4;
	if (size > 32)
		size = 32;
	if (size < 4)
		size = 4;
	m_pInstrument->SetWaveSize(size);

	FillSizeBox(size);
}

void CInstrumentEditorN106Wave::OnWavePosChange()
{
	BOOL trans;
	int pos = GetDlgItemInt(IDC_WAVE_POS, &trans, FALSE);
	if (pos > 255)
		pos = 255;
	if (pos < 0)
		pos = 0;
	m_pInstrument->SetWavePos(pos);
}

void CInstrumentEditorN106Wave::OnWavePosSelChange()
{
	CString str;
	CComboBox *pPosBox = (CComboBox*)GetDlgItem(IDC_WAVE_POS);
	pPosBox->GetLBText(pPosBox->GetCurSel(), str);

	int pos = _ttoi(str);

	if (pos > 255)
		pos = 255;
	if (pos < 0)
		pos = 0;
	m_pInstrument->SetWavePos(pos);
}

void CInstrumentEditorN106Wave::FillSizeBox(int size)
{
	CComboBox *pPosBox = (CComboBox*)GetDlgItem(IDC_WAVE_POS);
	pPosBox->ResetContent();

	CString str;
	for (int i = 0; i < 128; i += size) {
		str.Format(_T("%i"), i);
		pPosBox->AddString(str);
	}
}
