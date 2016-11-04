/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2007  Jonathan Liss
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
#include "FamiTrackerView.h"
#include "FamiTrackerDoc.h"
#include "InstrumentSettingsVRC6.h"
#include "InstrumentSettings.h"

static const char *INST_SETTINGS_VRC6[]	= {"Volume", "Arpeggio", "Pitch",  "Hi-pitch", "Pulse Width"};


// CInstrumentSettingsVRC6 dialog

IMPLEMENT_DYNAMIC(CInstrumentSettingsVRC6, CDialog)
CInstrumentSettingsVRC6::CInstrumentSettingsVRC6(CWnd* pParent /*=NULL*/)
	: CDialog(CInstrumentSettingsVRC6::IDD, pParent)
{
}

CInstrumentSettingsVRC6::~CInstrumentSettingsVRC6()
{
}

void CInstrumentSettingsVRC6::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CInstrumentSettingsVRC6, CDialog)
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_INSTSETTINGS, &CInstrumentSettingsVRC6::OnLvnItemchangedInstsettings)
END_MESSAGE_MAP()


// CInstrumentSettingsVRC6 message handlers

BOOL CInstrumentSettingsVRC6::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_bInitializing = true;
//	m_iCurrentEffect = 0;

	pDoc = (CFamiTrackerDoc*)theApp.GetDocument();
	pView = (CFamiTrackerView*)theApp.GetView();

	// Instrument settings

	m_pSettingsListCtrl = (CListCtrl*)GetDlgItem(IDC_INSTSETTINGS);
	m_pSettingsListCtrl->DeleteAllItems();
	m_pSettingsListCtrl->InsertColumn(0, "", LVCFMT_LEFT, 26);
	m_pSettingsListCtrl->InsertColumn(1, "#", LVCFMT_LEFT, 30);
	m_pSettingsListCtrl->InsertColumn(2, "Effect name", LVCFMT_LEFT, 84);
	m_pSettingsListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
	
	for (int i = MOD_COUNT - 1; i > -1; i--) {
		m_pSettingsListCtrl->InsertItem(0, "", 0);
		m_pSettingsListCtrl->SetCheck(0, 0);
		m_pSettingsListCtrl->SetItemText(0, 1, "0");
		m_pSettingsListCtrl->SetItemText(0, 2, INST_SETTINGS_VRC6[i]);
	}

	SetDlgItemText(IDC_SEQ_INDEX, "0");

	SequenceEditor.CreateEx(WS_EX_STATICEDGE, NULL, "", WS_CHILD | WS_VISIBLE, CRect(190 - 2, 30 - 2, SEQUENCE_EDIT_WIDTH, SEQUENCE_EDIT_HEIGHT), this, 0);
	SequenceEditor.ShowWindow(SW_SHOW);
	SequenceEditor.SetParent(this);
	
	m_bInitializing = false;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CInstrumentSettingsVRC6::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

HBRUSH CInstrumentSettingsVRC6::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	if (!theApp.IsThemeActive())
		return hbr;

	if (nCtlColor == CTLCOLOR_STATIC) {
		pDC->SetBkMode(TRANSPARENT);
		// this might fail on some themes?
		return CreateSolidBrush(GetPixel(pDC->m_hDC, 15, 5));
		//return (HBRUSH)GetStockObject(WHITE_BRUSH);
	}

	return hbr;
}

void CInstrumentSettingsVRC6::SetCurrentInstrument(int Index)
{
	char Text[256];

	CInstrumentVRC6 *InstConf = (CInstrumentVRC6*)pDoc->GetInstrument(Index);
	m_pSettingsListCtrl	= (CListCtrl*)GetDlgItem(IDC_INSTSETTINGS);

	int SelectedEffect = m_pSettingsListCtrl->GetSelectionMark();

	if (SelectedEffect == -1)
		SelectedEffect = 0;

	for (int i = 0; i < MOD_COUNT; i++) {
		sprintf(Text, "%i", InstConf->GetModIndex(i));
		m_pSettingsListCtrl->SetCheck(i, InstConf->GetModEnable(i));
		m_pSettingsListCtrl->SetItemText(i, 1, Text);
	} 

	int Item = InstConf->GetModIndex(SelectedEffect);
	m_iCurrentEffect = SelectedEffect;
	m_iInstrument = Index;

	SelectSequence(Item);
}

void CInstrumentSettingsVRC6::SelectSequence(int Sequence)
{
	// Selects the current sequence in the sequence editor

	ASSERT(Sequence < MAX_SEQUENCES);

	m_SelectedSeq		= pDoc->GetSequenceVRC6(Sequence, m_iCurrentEffect);
	m_iSelectedSequence = Sequence;

	SequenceEditor.SetCurrentSequence(m_iCurrentEffect, m_SelectedSeq);

	SetDlgItemText(IDC_MML, SequenceEditor.CompileList());

	if (!m_bInitializing)
		SequenceEditor.RedrawWindow();
}

void CInstrumentSettingsVRC6::OnLvnItemchangedInstsettings(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	int Setting = pNMLV->iItem;
	int SeqIndex;
	int Checked, i;
	CString Text;

	if (pNMLV->uNewState != 3 && pNMLV->uNewState != 8192 && pNMLV->uNewState != 4096)
		return;

	if (m_bInitializing)
		return;

	CInstrumentVRC6 *pInst = (CInstrumentVRC6*)pDoc->GetInstrument(m_iInstrument);

	m_pSettingsListCtrl = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS));

	m_iCurrentEffect	= Setting;
	SeqIndex			= pInst->GetModIndex(Setting);

	Text.Format("%i", SeqIndex);
	GetDlgItem(IDC_SEQ_INDEX)->SetWindowText(Text);

	for (i = 0; i < MOD_COUNT; i++) {
		Checked = m_pSettingsListCtrl->GetCheck(i);
		pInst->SetModEnable(i, (Checked == 1));
	}

	SelectSequence(SeqIndex);

	*pResult = 0;
}

void CInstrumentSettingsVRC6::CompileSequence()
{
	SetDlgItemText(IDC_MML, SequenceEditor.CompileList());
	
	CInstrumentVRC6 *pInst = (CInstrumentVRC6*)pDoc->GetInstrument(m_iInstrument);

	CString Text;
	GetDlgItemText(IDC_MML, Text);

	switch (m_iCurrentEffect) {
		case 0:
			TranslateMML(Text, m_SelectedSeq, 0, 15); break;
		case 1:
		case 2:
		case 3:
			TranslateMML(Text, m_SelectedSeq, -127, 126); break;
		case 4:
			TranslateMML(Text, m_SelectedSeq, 0, 7); break;
	}

	m_pSettingsListCtrl = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS));
	m_pSettingsListCtrl->SetCheck(m_iCurrentEffect);

	pInst->SetModEnable(m_iCurrentEffect, 1);
}

BOOL CInstrumentSettingsVRC6::DestroyWindow()
{
	SequenceEditor.DestroyWindow();
	return __super::DestroyWindow();
}
