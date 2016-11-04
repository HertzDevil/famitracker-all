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
#include "FamiTrackerView.h"
#include "InstrumentEditPanel.h"
#include "InstrumentEditorVRC7.h"


// CInstrumentSettingsVRC7 dialog

IMPLEMENT_DYNAMIC(CInstrumentEditorVRC7, CInstrumentEditPanel)

CInstrumentEditorVRC7::CInstrumentEditorVRC7(CWnd* pParent /*=NULL*/)
	: CInstrumentEditPanel(CInstrumentEditorVRC7::IDD, pParent)
{
}

CInstrumentEditorVRC7::~CInstrumentEditorVRC7()
{
}

void CInstrumentEditorVRC7::DoDataExchange(CDataExchange* pDX)
{
	CInstrumentEditPanel::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CInstrumentEditorVRC7, CInstrumentEditPanel)
	ON_CBN_SELCHANGE(IDC_PATCH, OnCbnSelchangePatch)
	ON_BN_CLICKED(IDC_M_AM, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_M_VIB, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_M_EG, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_M_KSR2, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_M_DM, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_C_AM, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_C_VIB, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_C_EG, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_C_KSR, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_BN_CLICKED(IDC_C_DM, &CInstrumentEditorVRC7::OnBnClickedCheckbox)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
//	ON_BN_CLICKED(IDC_HOLD, &CInstrumentEditorVRC7::OnBnClickedHold)
END_MESSAGE_MAP()


// CInstrumentSettingsVRC7 message handlers

BOOL CInstrumentEditorVRC7::OnInitDialog()
{
	CDialog::OnInitDialog();

	CComboBox *pPatchBox = (CComboBox*)GetDlgItem(IDC_PATCH);
	CString Text;

	for (int i = 0; i < 16; i++) {
		Text.Format(_T("Patch #%i %s"), i, i == 0 ? _T("(custom patch)") : _T(""));
		pPatchBox->AddString(Text);
	}

	pPatchBox->SetCurSel(0);

	SetupSlider(IDC_M_MUL, 15);
	SetupSlider(IDC_C_MUL, 15);
	SetupSlider(IDC_M_KSL, 3);
	SetupSlider(IDC_C_KSL, 3);
	SetupSlider(IDC_TL, 63);
	SetupSlider(IDC_FB, 7);
	SetupSlider(IDC_M_AR, 15);
	SetupSlider(IDC_M_DR, 15);
	SetupSlider(IDC_M_SR, 15);
	SetupSlider(IDC_M_RR, 15);
	SetupSlider(IDC_C_AR, 15);
	SetupSlider(IDC_C_DR, 15);
	SetupSlider(IDC_C_SR, 15);
	SetupSlider(IDC_C_RR, 15);

	EnableControls(true);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditorVRC7::OnCbnSelchangePatch()
{
	int Patch;
	CComboBox *pPatchBox = (CComboBox*)GetDlgItem(IDC_PATCH);
	Patch = pPatchBox->GetCurSel();
	//CInstrumentVRC7 *InstConf = (CInstrumentVRC7*)GetDocument()->GetInstrument(m_iInstrument);
	m_pInstrument->SetPatch(Patch);
	//InstConf->SetPatch(Patch);
	EnableControls(Patch == 0);
//	CheckDlgButton(IDC_HOLD, InstConf->GetHold());
}

void CInstrumentEditorVRC7::EnableControls(bool bEnable)
{
	const int SLIDER_IDS[] = {
		IDC_M_AM, IDC_M_AR, IDC_M_DM, IDC_M_DR, IDC_M_EG, IDC_M_KSL, IDC_M_KSR2, IDC_M_MUL, IDC_M_RR, IDC_M_SL, IDC_M_SR, IDC_M_VIB,
		IDC_C_AM, IDC_C_AR, IDC_C_DM, IDC_C_DR, IDC_C_EG, IDC_C_KSL, IDC_C_KSR, IDC_C_MUL, IDC_C_RR, IDC_C_SL, IDC_C_SR, IDC_C_VIB,
		IDC_TL, IDC_FB
	};

	const int SLIDERS = sizeof(SLIDER_IDS) / sizeof(SLIDER_IDS[0]);

	for (int i = 0; i < SLIDERS; ++i)
		GetDlgItem(SLIDER_IDS[i])->EnableWindow(bEnable ? TRUE : FALSE);
}

void CInstrumentEditorVRC7::SelectInstrument(int Instrument)
{
	CComboBox *pPatchBox = (CComboBox*)GetDlgItem(IDC_PATCH);
	CInstrumentVRC7 *InstConf = (CInstrumentVRC7*)GetDocument()->GetInstrument(Instrument);
	//m_iInstrument = Instrument;
	m_pInstrument = InstConf;
	int Patch = InstConf->GetPatch();
	pPatchBox->SetCurSel(Patch);
	LoadCustomPatch();
	EnableControls(Patch == 0);
}

BOOL CInstrumentEditorVRC7::OnEraseBkgnd(CDC* pDC)
{
	return CDialog::OnEraseBkgnd(pDC);
}

HBRUSH CInstrumentEditorVRC7::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CInstrumentEditorVRC7::SetupSlider(int Slider, int Max)
{
	CSliderCtrl *pSlider = (CSliderCtrl*)GetDlgItem(Slider);
	pSlider->SetRangeMax(Max);
}

int CInstrumentEditorVRC7::GetSliderVal(int Slider)
{
	CSliderCtrl *pSlider = (CSliderCtrl*)GetDlgItem(Slider);
	return pSlider->GetPos();
}

void CInstrumentEditorVRC7::SetSliderVal(int Slider, int Value)
{
	CSliderCtrl *pSlider = (CSliderCtrl*)GetDlgItem(Slider);
	pSlider->SetPos(Value);
}

void CInstrumentEditorVRC7::LoadCustomPatch()
{
	unsigned int Reg;

	// Register 0
	Reg = m_pInstrument->GetCustomReg(0);
	CheckDlgButton(IDC_M_AM, Reg & 0x80 ? 1 : 0);
	CheckDlgButton(IDC_M_VIB, Reg & 0x40 ? 1 : 0);
	CheckDlgButton(IDC_M_EG, Reg & 0x20 ? 1 : 0);
	CheckDlgButton(IDC_M_KSR2, Reg & 0x10 ? 1 : 0);
	SetSliderVal(IDC_M_MUL, Reg & 0x0F);

	// Register 1
	Reg = m_pInstrument->GetCustomReg(1);
	CheckDlgButton(IDC_C_AM, Reg & 0x80 ? 1 : 0);
	CheckDlgButton(IDC_C_VIB, Reg & 0x40 ? 1 : 0);
	CheckDlgButton(IDC_C_EG, Reg & 0x20 ? 1 : 0);
	CheckDlgButton(IDC_C_KSR, Reg & 0x10 ? 1 : 0);
	SetSliderVal(IDC_C_MUL, Reg & 0x0F);

	// Register 2
	Reg = m_pInstrument->GetCustomReg(2);
	SetSliderVal(IDC_M_KSL, Reg >> 6);
	SetSliderVal(IDC_TL, Reg & 0x3F);

	// Register 3
	Reg = m_pInstrument->GetCustomReg(3);
	SetSliderVal(IDC_C_KSL, Reg >> 6);
	SetSliderVal(IDC_FB, 7 - (Reg & 7));
	CheckDlgButton(IDC_C_DC, Reg & 0x10 ? 1 : 0);
	CheckDlgButton(IDC_M_DM, Reg & 0x08 ? 1 : 0);

	// Register 4
	Reg = m_pInstrument->GetCustomReg(4);
	SetSliderVal(IDC_M_AR, Reg >> 4);
	SetSliderVal(IDC_M_DR, Reg & 0x0F);

	// Register 5
	Reg = m_pInstrument->GetCustomReg(5);
	SetSliderVal(IDC_C_AR, Reg >> 4);
	SetSliderVal(IDC_C_DR, Reg & 0x0F);

	// Register 6
	Reg = m_pInstrument->GetCustomReg(6);
	SetSliderVal(IDC_M_SR, Reg >> 4);
	SetSliderVal(IDC_M_RR, Reg & 0x0F);

	// Register 7
	Reg = m_pInstrument->GetCustomReg(7);
	SetSliderVal(IDC_C_SR, Reg >> 4);
	SetSliderVal(IDC_C_RR, Reg & 0x0F);
}

void CInstrumentEditorVRC7::SaveCustomPatch()
{
	int Reg;

	// Register 0
	Reg  = (IsDlgButtonChecked(IDC_M_AM) ? 0x80 : 0);
	Reg |= (IsDlgButtonChecked(IDC_M_VIB) ? 0x40 : 0);
	Reg |= (IsDlgButtonChecked(IDC_M_EG) ? 0x20 : 0);
	Reg |= (IsDlgButtonChecked(IDC_M_KSR2) ? 0x10 : 0);
	Reg |= GetSliderVal(IDC_M_MUL);
	m_pInstrument->SetCustomReg(0, Reg);

	// Register 1
	Reg  = (IsDlgButtonChecked(IDC_C_AM) ? 0x80 : 0);
	Reg |= (IsDlgButtonChecked(IDC_C_VIB) ? 0x40 : 0);
	Reg |= (IsDlgButtonChecked(IDC_C_EG) ? 0x20 : 0);
	Reg |= (IsDlgButtonChecked(IDC_C_KSR) ? 0x10 : 0);
	Reg |= GetSliderVal(IDC_C_MUL);
	m_pInstrument->SetCustomReg(1, Reg);

	// Register 2
	Reg  = GetSliderVal(IDC_M_KSL) << 6;
	Reg |= GetSliderVal(IDC_TL);
	m_pInstrument->SetCustomReg(2, Reg);

	// Register 3
	Reg  = GetSliderVal(IDC_C_KSL) << 6;
	Reg |= IsDlgButtonChecked(IDC_C_DC) ? 0x10 : 0;
	Reg |= IsDlgButtonChecked(IDC_M_DM) ? 0x08 : 0;
	Reg |= 7 - GetSliderVal(IDC_FB);
	m_pInstrument->SetCustomReg(3, Reg);

	// Register 4
	Reg = GetSliderVal(IDC_M_AR) << 4;
	Reg |= GetSliderVal(IDC_M_DR);
	m_pInstrument->SetCustomReg(4, Reg);

	// Register 5
	Reg = GetSliderVal(IDC_C_AR) << 4;
	Reg |= GetSliderVal(IDC_C_DR);
	m_pInstrument->SetCustomReg(5, Reg);

	// Register 6
	Reg = GetSliderVal(IDC_M_SR) << 4;
	Reg |= GetSliderVal(IDC_M_RR);
	m_pInstrument->SetCustomReg(6, Reg);

	// Register 7
	Reg = GetSliderVal(IDC_C_SR) << 4;
	Reg |= GetSliderVal(IDC_C_RR);
	m_pInstrument->SetCustomReg(7, Reg);	
}

void CInstrumentEditorVRC7::OnBnClickedCheckbox()
{
	SaveCustomPatch();
	SetFocus();
}

void CInstrumentEditorVRC7::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SaveCustomPatch();
	SetFocus();
	CInstrumentEditPanel::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CInstrumentEditorVRC7::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SaveCustomPatch();
	SetFocus();
	CInstrumentEditPanel::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CInstrumentEditorVRC7::OnBnClickedHold()
{
//	m_pInstrument->SetHold(IsDlgButtonChecked(IDC_HOLD) == 1);
}
