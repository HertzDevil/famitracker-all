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
#include "SequenceEditor.h"
#include "InstrumentEditorVRC6.h"

LPCTSTR CInstrumentEditorVRC6::INST_SETTINGS_VRC6[] = {_T("Volume"), _T("Arpeggio"), _T("Pitch"), _T("Hi-pitch"), _T("Pulse Width")};

// CInstrumentEditorVRC6 dialog

IMPLEMENT_DYNAMIC(CInstrumentEditorVRC6, CSequenceInstrumentEditPanel)
CInstrumentEditorVRC6::CInstrumentEditorVRC6(CWnd* pParent) : CSequenceInstrumentEditPanel(CInstrumentEditorVRC6::IDD, pParent),
	m_pParentWin(pParent),
	m_pInstrument(NULL),
	m_pSequence(NULL),
	m_pSequenceEditor(NULL),
	m_iSelectedSetting(0)
{
}

CInstrumentEditorVRC6::~CInstrumentEditorVRC6()
{
	SAFE_RELEASE(m_pSequenceEditor);
}

void CInstrumentEditorVRC6::DoDataExchange(CDataExchange* pDX)
{
	CSequenceInstrumentEditPanel::DoDataExchange(pDX);
}

void CInstrumentEditorVRC6::SelectInstrument(int Instrument)
{
	CInstrumentVRC6 *pInst = (CInstrumentVRC6*)GetDocument()->GetInstrument(Instrument);
	CListCtrl *pList = (CListCtrl*) GetDlgItem(IDC_INSTSETTINGS);

	m_pInstrument = pInst;

	// Update instrument setting list
	for (int i = 0; i < SEQ_COUNT; i++) {
		CString IndexStr;
		IndexStr.Format(_T("%i"), pInst->GetSeqIndex(i));
		pList->SetCheck(i, pInst->GetSeqEnable(i));
		pList->SetItemText(i, 1, IndexStr);
	} 

	// Setting text box
	SetDlgItemInt(IDC_SEQ_INDEX, pInst->GetSeqIndex(m_iSelectedSetting));

	// Select new sequence
	SelectSequence(pInst->GetSeqIndex(m_iSelectedSetting), m_iSelectedSetting);
}

void CInstrumentEditorVRC6::SelectSequence(int Sequence, int Type)
{
	// Selects the current sequence in the sequence editor
	m_pSequence = GetDocument()->GetSequenceVRC6(Sequence, Type);
	m_pSequenceEditor->SelectSequence(m_pSequence, Type, INST_VRC6);
}

void CInstrumentEditorVRC6::TranslateMML(CString String, int Max, int Min)
{
	CSequenceInstrumentEditPanel::TranslateMML(String, m_pSequence, Max, Min);

	// Update editor
	m_pSequenceEditor->RedrawWindow();

	// Register a document change
	GetDocument()->SetModifiedFlag();

	// Enable setting
	((CListCtrl*)GetDlgItem(IDC_INSTSETTINGS))->SetCheck(m_iSelectedSetting, 1);
}

void CInstrumentEditorVRC6::SetSequenceString(CString Sequence, bool Changed)
{
	// Update sequence string
	SetDlgItemText(IDC_SEQUENCE_STRING, Sequence);
	// If the sequence was changed, assume the user wants to enable it
	if (Changed) {
		((CListCtrl*)GetDlgItem(IDC_INSTSETTINGS))->SetCheck(m_iSelectedSetting, 1);
	}
}

BEGIN_MESSAGE_MAP(CInstrumentEditorVRC6, CInstrumentEditPanel)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_INSTSETTINGS, OnLvnItemchangedInstsettings)	
	ON_EN_CHANGE(IDC_SEQ_INDEX, OnEnChangeSeqIndex)
	ON_BN_CLICKED(IDC_FREE_SEQ, OnBnClickedFreeSeq)
	ON_BN_CLICKED(IDC_PARSE, OnBnClickedParse)
END_MESSAGE_MAP()

// CInstrumentSettings message handlers

BOOL CInstrumentEditorVRC6::OnInitDialog()
{
	CInstrumentEditPanel::OnInitDialog();

	// Instrument settings
	CListCtrl *pList = (CListCtrl*) GetDlgItem(IDC_INSTSETTINGS);
	pList->DeleteAllItems();
	pList->InsertColumn(0, _T(""), LVCFMT_LEFT, 26);
	pList->InsertColumn(1, _T("#"), LVCFMT_LEFT, 30);
	pList->InsertColumn(2, _T("Effect name"), LVCFMT_LEFT, 84);
	pList->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
	
	for (int i = SEQ_COUNT - 1; i > -1; i--) {
		pList->InsertItem(0, _T(""), 0);
		pList->SetCheck(0, 0);
		pList->SetItemText(0, 1, _T("0"));
		pList->SetItemText(0, 2, INST_SETTINGS_VRC6[i]);
	}

	pList->SetItemState(m_iSelectedSetting, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

	SetDlgItemInt(IDC_SEQ_INDEX, m_iSelectedSetting);

	CSpinButtonCtrl *pSequenceSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SEQUENCE_SPIN);
	pSequenceSpin->SetRange(0, MAX_SEQUENCES - 1);

	CRect rect(190 - 2, 30 - 2, CSequenceEditor::SEQUENCE_EDIT_WIDTH, CSequenceEditor::SEQUENCE_EDIT_HEIGHT);
	
	m_pSequenceEditor = new CSequenceEditor(GetDocument());	
	m_pSequenceEditor->CreateEditor(this, rect);
	m_pSequenceEditor->ShowWindow(SW_SHOW);
	m_pSequenceEditor->SetMaxValues(MAX_VOLUME, MAX_DUTY);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditorVRC6::OnLvnItemchangedInstsettings(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if (pNMLV->uNewState & LVIS_SELECTED && pNMLV->uNewState & LVIS_FOCUSED) {
		// If a new item is selected
		m_iSelectedSetting = pNMLV->iItem;
		if (m_pInstrument) {
			int Sequence = m_pInstrument->GetSeqIndex(m_iSelectedSetting);
			SetDlgItemInt(IDC_SEQ_INDEX, Sequence);
			SelectSequence(Sequence, m_iSelectedSetting);
		}
	}
	else if ((pNMLV->uNewState == 8192 || pNMLV->uNewState == 4096) && (m_pInstrument != NULL)) {
		// Changed state of checkbox
		int Item = pNMLV->iItem;
		CListCtrl *pList = (CListCtrl*)GetDlgItem(IDC_INSTSETTINGS);
		int Checked = pList->GetCheck(Item);
		m_pInstrument->SetSeqEnable(Item, (Checked == 1));
	}

	*pResult = 0;
}

void CInstrumentEditorVRC6::OnEnChangeSeqIndex()
{
	// Selected sequence changed
	CListCtrl *pList = (CListCtrl*) GetDlgItem(IDC_INSTSETTINGS);
	int Index = GetDlgItemInt(IDC_SEQ_INDEX);

	if (Index < 0)
		Index = 0;
	if (Index > (MAX_SEQUENCES - 1))
		Index = (MAX_SEQUENCES - 1);
	
	// Update list
	CString Text;
	Text.Format(_T("%i"), Index);
	pList->SetItemText(m_iSelectedSetting, 1, Text);

	if (m_pInstrument) {
		m_pInstrument->SetSeqIndex(m_iSelectedSetting, Index);
		SelectSequence(Index, m_iSelectedSetting);
	}
}

void CInstrumentEditorVRC6::OnBnClickedFreeSeq()
{
	CString Text;
	int FreeIndex = GetDocument()->GetFreeSequenceVRC6(m_iSelectedSetting);
	Text.Format(_T("%i"), FreeIndex);
	SetDlgItemText(IDC_SEQ_INDEX, Text);	// Things will update automatically by changing this
}

void CInstrumentEditorVRC6::OnBnClickedParse()
{
	// Translate the sequence text string to a sequence
	CString Text;
	GetDlgItemText(IDC_SEQUENCE_STRING, Text);

	switch (m_iSelectedSetting) {
		case SEQ_VOLUME:
			TranslateMML(Text, MAX_VOLUME, 0);
			break;
		case SEQ_ARPEGGIO:
			TranslateMML(Text, 96, -96);
			break;
		case SEQ_PITCH:
			TranslateMML(Text, 126, -127);
			break;
		case SEQ_HIPITCH:
			TranslateMML(Text, 126, -127);
			break;
		case SEQ_DUTYCYCLE:
			TranslateMML(Text, MAX_DUTY, 0);
			break;
	}
}

BOOL CInstrumentEditorVRC6::DestroyWindow()
{
	m_pSequenceEditor->DestroyWindow();
	return CDialog::DestroyWindow();
}

void CInstrumentEditorVRC6::OnKeyReturn()
{
	OnBnClickedParse();
}
