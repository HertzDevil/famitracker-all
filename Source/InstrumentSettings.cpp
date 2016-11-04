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
#include "InstrumentSettings.h"
#include "SequenceEditor.h"
#include "..\include\instrumentsettings.h"

const int SEQUENCE_EDIT_WIDTH = 540;
const int SEQUENCE_EDIT_HEIGHT = 230;

// CInstrumentSettings dialog

IMPLEMENT_DYNAMIC(CInstrumentSettings, CDialog)
CInstrumentSettings::CInstrumentSettings(CWnd* pParent /*=NULL*/)
	: CDialog(CInstrumentSettings::IDD, pParent)
{
	ParentWin = pParent;

	m_bShiftPressed			= false;
	UpdatingSequenceItem	= false;
	m_bInitializing			= true;
	m_iSelectedSequence		= 0;
	m_iInstrument			= 0;
	SelectedSeqItem			= 0;
	m_iCurrentEffect		= 0;
}

CInstrumentSettings::~CInstrumentSettings()
{
}

void CInstrumentSettings::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CInstrumentSettings, CDialog)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_INSTSETTINGS, OnLvnItemchangedInstsettings)
	ON_NOTIFY(UDN_DELTAPOS, IDC_MOD_SELECT_SPIN, OnDeltaposModSelectSpin)
	ON_EN_CHANGE(IDC_SEQ_INDEX, OnEnChangeSeqIndex)
	ON_BN_CLICKED(IDC_PARSE_MML, OnBnClickedParseMml)
	ON_BN_CLICKED(IDC_FREE_SEQ, OnBnClickedFreeSeq)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


// CInstrumentSettings message handlers

BOOL CInstrumentSettings::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_bInitializing = true;
	m_iCurrentEffect = 0;

	pDoc = (CFamiTrackerDoc*)theApp.pDocument;
	pView = (CFamiTrackerView*)theApp.pView;

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
		m_pSettingsListCtrl->SetItemText(0, 2, INST_SETTINGS[i]);
	}

	SetDlgItemText(IDC_SEQ_INDEX, "0");

	SequenceEditor.CreateEx(WS_EX_STATICEDGE, NULL, "", WS_CHILD | WS_VISIBLE, CRect(190 - 2, 30 - 2, SEQUENCE_EDIT_WIDTH, SEQUENCE_EDIT_HEIGHT), this, 0);
	SequenceEditor.ShowWindow(SW_SHOW);
	SequenceEditor.SetParent(this);

	m_bInitializing = false;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentSettings::SetCurrentInstrument(int Index)
{
	char Text[256];
	stInstrument *InstConf	= pDoc->GetInstrument(Index);
	m_pSettingsListCtrl		= (CListCtrl*)GetDlgItem(IDC_INSTSETTINGS);

	m_iInstrument	= Index;
	m_bInitializing = true;

	int SelectedEffect = m_pSettingsListCtrl->GetSelectionMark();

	if (SelectedEffect == -1)
		SelectedEffect = 0;

	for (int i = 0; i < MOD_COUNT; i++) {
		sprintf(Text, "%i", InstConf->ModIndex[i]);
		m_pSettingsListCtrl->SetCheck(i, InstConf->ModEnable[i]);
		m_pSettingsListCtrl->SetItemText(i, 1, Text);
	}

	int Item = pDoc->GetInstEffIndex(m_iInstrument, SelectedEffect);

	m_iCurrentEffect = SelectedEffect;

	SelectSequence(Item);

	m_bInitializing = false;
}

void CInstrumentSettings::CompileSequence()
{
	SetDlgItemText(IDC_MML, SequenceEditor.CompileList());
	TranslateMML();
}

void CInstrumentSettings::OnLvnItemchangedInstsettings(NMHDR *pNMHDR, LRESULT *pResult)
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

	m_iCurrentEffect	= Setting;
	SeqIndex			= pDoc->GetInstEffIndex(m_iInstrument, Setting);

	Text.Format("%i", SeqIndex);
	GetDlgItem(IDC_SEQ_INDEX)->SetWindowText(Text);

	for (i = 0; i < MOD_COUNT; i++) {
		Checked = m_pSettingsListCtrl->GetCheck(i);
		pDoc->SetInstEffect(m_iInstrument, i, pDoc->GetInstEffIndex(m_iInstrument, i), (Checked == 1));
	}

	SelectSequence(SeqIndex);

	*pResult = 0;
}

void CInstrumentSettings::OnDeltaposModSelectSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);	
	CString Text;
	int Pos;

	Pos = GetDlgItemInt(IDC_SEQ_INDEX) - ((NMUPDOWN*)pNMHDR)->iDelta;

	LIMIT(Pos, MAX_SEQUENCES, 0);

	Text.Format("%i", Pos);
	SetDlgItemText(IDC_SEQ_INDEX, Text);

	*pResult = 0;
}

void CInstrumentSettings::OnEnChangeSeqIndex()
{
	stInstrument *InstConf = pDoc->GetInstrument(m_iInstrument);
	CString Text;
	int Index;
	int Setting;

	if (m_bInitializing)
		return;

	m_pSettingsListCtrl = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS));
	Index				= GetDlgItemInt(IDC_SEQ_INDEX);
	Setting				= m_iCurrentEffect;

	Text.Format("%i", Index);
	m_pSettingsListCtrl->SetItemText(Setting, 1, Text);

	InstConf->ModIndex[Setting] = Index;

	SelectSequence(Index);
}

void CInstrumentSettings::TranslateMML()
{
	CString MML, Accum;
	
	char c, lc;
	bool First = true, Negative = false;
	int Len, i;
	int Value, Length, LastValue;
	int Items, LoopIndex, Relative = 0;
	int RelativeCtr = 0;

	CString DbgStr;

	GetDlgItemText(IDC_MML, MML);

	MML.AppendChar(' ');

	Len = MML.GetLength();

	Items	= 0;
	Value	= 0;
	LastValue = 0xFF;
	Length	= -1;
	c = lc	= 0;

	LoopIndex = -1;

	// 1 2 3 4 5

	for (i = 0; i < Len; i++) {
		c = MML.GetAt(i);

		if (c >= '0' && c <= '9') {
			Accum.AppendChar(c);
		}
		else if (c == '-') {
			Negative = true;
		}
		else if (c == '|') {
			LoopIndex = Items;
			LastValue = 0xFF;
		}

		if (c == ' ' || i == (Len - 1) && c != '|') {
			if ((Accum.GetLength() > 0 && sscanf(Accum, "%i", &Value) != 0) || i == (Len - 1)) {

				switch (m_iCurrentEffect) {
					case MOD_VOLUME:
						LIMIT(Value, 15, 0);
						break;
					case MOD_ARPEGGIO:
					case MOD_PITCH:
					case MOD_HIPITCH:
						LIMIT(Value, 126, -127);
						break;
					case MOD_DUTYCYCLE:
						LIMIT(Value, 3, 0);
						break;
				}

				if (Negative)
					Value = -Value;

				if (Value == LastValue) {
					if (i != (Len - 1))
						m_SelectedSeq->Length[Items - 1]++;
				}
				else {
					m_SelectedSeq->Length[Items] = 0;
					m_SelectedSeq->Value[Items] = Value;

					Items++;
				}

				Negative = false;
				Accum = "";

				LastValue = Value;
			}
		}
	}

	if (LoopIndex != -1) {
		m_SelectedSeq->Length[Items] = LoopIndex - Items;
		m_SelectedSeq->Value[Items] = 0;
		Items++;
	}

	m_SelectedSeq->Count = Items;

	pDoc->SetModifiedFlag();
}

void CInstrumentSettings::TranslateRelativeMML()
{
	CString MML, Accum;
	
	char c, lc;
	bool First = true, Negative = false;
	int Len, i;
	int Value, Length, LastValue;
	int Items, LoopIndex, Relative = 0;
	int RelativeCtr = 0;

	CString DbgStr;

	GetDlgItemText(IDC_MML, MML);

	MML.AppendChar(' ');

	Len = MML.GetLength();

	Items	= 0;
	Value	= 0;
	LastValue = 0xFF;
	Length	= -1;
	c = lc	= 0;

	LoopIndex = -1;

	// 1 2 3 4 5

	for (i = 0; i < Len; i++) {
		c = MML.GetAt(i);

		if (c >= '0' && c <= '9') {
			Accum.AppendChar(c);
		}
		else if (c == '-') {
			Negative = true;
		}
		else if (c == '|') {
			LoopIndex = Items;
			LastValue = 0xFF;
		}

		if (c == ' ' || i == (Len - 1) && c != '|') {
			if ((Accum.GetLength() > 0 && sscanf(Accum, "%i", &Value) != 0) || i == (Len - 1)) {

				if (Negative)
					RelativeCtr -= Value;
				else
					RelativeCtr += Value;

				switch (m_iCurrentEffect) {
					case MOD_VOLUME:
						LIMIT(RelativeCtr, 15, 0);
						break;
					case MOD_ARPEGGIO:
					case MOD_PITCH:
					case MOD_HIPITCH:
						LIMIT(RelativeCtr, 126, -127);
						break;
					case MOD_DUTYCYCLE:
						LIMIT(RelativeCtr, 3, 0);
						break;
				}


				if (RelativeCtr == LastValue) {
					if (i != (Len - 1))
						m_SelectedSeq->Length[Items - 1]++;
				}
				else {
					m_SelectedSeq->Length[Items] = 0;
					m_SelectedSeq->Value[Items] = RelativeCtr;

					Items++;
				}

				Negative = false;
				Accum = "";

				LastValue = RelativeCtr;
			}
		}
	}

	if (LoopIndex != -1) {
		m_SelectedSeq->Length[Items] = LoopIndex - Items;
		m_SelectedSeq->Value[Items] = 0;
		Items++;
	}

	m_SelectedSeq->Count = Items;

	pDoc->SetModifiedFlag();
}

void CInstrumentSettings::OnBnClickedParseMml()
{
	if (m_bShiftPressed)
		TranslateRelativeMML();
	else
		TranslateMML();

	SelectSequence(m_iSelectedSequence);
}

BOOL CInstrumentSettings::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message) {
		case WM_KEYDOWN:
			switch (pMsg->wParam) {
				case 13:	// Return
					pMsg->wParam = 0;
					OnBnClickedParseMml();
					return TRUE;
				case 27:	// Esc
					pMsg->wParam = 0;
					return TRUE;
				case 16:	// Shift
					m_bShiftPressed = true;
					return TRUE;
				default:
				//	CString a;a.Format("%i", pMsg->wParam);AfxMessageBox(a);
					if (GetDlgItem(IDC_MML) != GetFocus())
						pView->PlayNote(int(pMsg->wParam));
			}
			break;
		case WM_KEYUP:
			if (pMsg->wParam == 16)
				m_bShiftPressed = false;
			else
				pView->KeyReleased(int(pMsg->wParam));
			return TRUE;
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CInstrumentSettings::OnBnClickedFreeSeq()
{
	CString Text;
	int i;

	for (i = 0; i < MAX_SEQUENCES; i++) {
		if (pDoc->GetSequenceCount(i, m_iCurrentEffect) == 0) {
			Text.Format("%i", i);
			// Things will update automatically by changing this
			SetDlgItemText(IDC_SEQ_INDEX, Text);
			return;
		}
	}
}

void CInstrumentSettings::SelectSequence(int Sequence)
{
	// Selects the current sequence in the sequence editor

	m_SelectedSeq		= pDoc->GetSequence(Sequence, m_iCurrentEffect);
	m_iSelectedSequence = Sequence;

	SequenceEditor.SetCurrentSequence(m_iCurrentEffect, m_SelectedSeq);

	SetDlgItemText(IDC_MML, SequenceEditor.CompileList());

	if (!m_bInitializing)
		SequenceEditor.RedrawWindow();
}

void CInstrumentSettings::OnLButtonDown(UINT nFlags, CPoint point)
{
	this->SetFocus();
	CDialog::OnLButtonDown(nFlags, point);
}
