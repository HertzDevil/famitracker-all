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
#include ".\instrumentsettings.h"

// CInstrumentSettings dialog

bool ChangedLength = false;

IMPLEMENT_DYNAMIC(CInstrumentSettings, CDialog)
CInstrumentSettings::CInstrumentSettings(CWnd* pParent /*=NULL*/)
	: CDialog(CInstrumentSettings::IDD, pParent)
{
	ParentWin = pParent;

	UpdatingSequenceItem	= false;
	Initializing			= true;
	SelectedSetting			= 0;
	SelectedSeqItem			= 0;
	SelectedSequence		= 0;
}

CInstrumentSettings::~CInstrumentSettings()
{
}

void CInstrumentSettings::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CInstrumentSettings, CDialog)
	ON_BN_CLICKED(IDC_INSERT, OnBnClickedInsert)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_INSTSETTINGS, OnLvnItemchangedInstsettings)
	ON_NOTIFY(UDN_DELTAPOS, IDC_MOD_SELECT_SPIN, OnDeltaposModSelectSpin)
	ON_EN_CHANGE(IDC_SEQ_INDEX, OnEnChangeSeqIndex)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_SEQUENCE_LIST, OnLvnItemchangedSequenceList)
	ON_BN_CLICKED(IDC_REMOVE, OnBnClickedRemove)
	ON_EN_CHANGE(IDC_LENGTH, OnEnChangeLength)
	ON_EN_CHANGE(IDC_VALUE, OnEnChangeValue)
	ON_NOTIFY(UDN_DELTAPOS, IDC_MOD_LENGTH_SPIN, OnDeltaposModLengthSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_MOD_VALUE_SPIN, OnDeltaposModValueSpin)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_PARSE_MML, OnBnClickedParseMml)
	ON_BN_CLICKED(IDC_FREE_SEQ, OnBnClickedFreeSeq)
END_MESSAGE_MAP()


// CInstrumentSettings message handlers

BOOL CInstrumentSettings::OnInitDialog()
{
	CDialog::OnInitDialog();

	char Text[256];

	Initializing = true;

	pDoc = (CFamiTrackerDoc*)theApp.pDocument;
	pView = (CFamiTrackerView*)theApp.pView;

	// Instrument settings

	m_pSettingsListCtrl = (CListCtrl*)GetDlgItem(IDC_INSTSETTINGS);
	m_pSettingsListCtrl->DeleteAllItems();
	m_pSettingsListCtrl->InsertColumn(0, "", LVCFMT_LEFT, 26);
	m_pSettingsListCtrl->InsertColumn(1, "#", LVCFMT_LEFT, 30);
	m_pSettingsListCtrl->InsertColumn(2, "Effect name", LVCFMT_LEFT, 80);
	m_pSettingsListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);

	// Sequence list

	m_pSequenceListCtrl = (CListCtrl*)GetDlgItem(IDC_SEQUENCE_LIST);
	m_pSequenceListCtrl->InsertColumn(0, "Length", LVCFMT_LEFT, 48);
	m_pSequenceListCtrl->InsertColumn(1, "Value", LVCFMT_LEFT, 44);
	m_pSequenceListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	stInstrument *InstConf = &pDoc->Instruments[pView->m_iInstrument];
	
	for (int i = MOD_COUNT - 1; i > -1; i--) {
		m_pSettingsListCtrl->InsertItem(0, "", 0);
		sprintf(Text, "%i", InstConf->ModIndex[i]);
		m_pSettingsListCtrl->SetCheck(0, InstConf->ModEnable[i]);
		m_pSettingsListCtrl->SetItemText(0, 1, Text);
		m_pSettingsListCtrl->SetItemText(0, 2, INST_SETTINGS[i]);
	}

	Initializing = false;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentSettings::OnLvnItemchangedInstsettings(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	
	int Setting = pNMLV->iItem;
	int SeqIndex;
	CString Text;

	SelectedSetting = Setting;
	SeqIndex		= pDoc->Instruments[pView->m_iInstrument].ModIndex[Setting];

	Text.Format("%i", SeqIndex);
	GetDlgItem(IDC_SEQ_INDEX)->SetWindowText(Text);

	if (!Initializing) {
		int Checked, i;

		for (i = 0; i < MOD_COUNT; i++) {
			Checked = m_pSettingsListCtrl->GetCheck(i);
			pDoc->Instruments[pView->m_iInstrument].ModEnable[i] = Checked;
		}
	}

	FillSequenceList(SeqIndex);

	*pResult = 0;
}

void CInstrumentSettings::FillSequenceList(int ListIndex)
{
	int SeqItems, i, Length, Value;
	CString Text, MML_A, MML_B, MML;
	bool Looped = false;

	m_pSequenceListCtrl = (CListCtrl*)GetDlgItem(IDC_SEQUENCE_LIST);
	m_pSequenceListCtrl->DeleteAllItems();

	SeqItems = pDoc->Sequences[ListIndex].Count;

	for (i = 0; i < SeqItems; i++) {
		Length = pDoc->Sequences[ListIndex].Length[i];
		Value = pDoc->Sequences[ListIndex].Value[i];
		Text.Format("%i", Length);
		m_pSequenceListCtrl->InsertItem(i, Text, 0);
		Text.Format("%i", Value);
		m_pSequenceListCtrl->SetItemText(i, 1, Text);
	}

	SelectedSequence = ListIndex;

	FillMML();
}

void CInstrumentSettings::FillMML()
{
	int SeqItems, i, x, Length, Value;
	CString Text, MML_A, MML_B, MML;
	bool Looped = false;
	int LoopingPoint;

	int Lengths[MAX_MML_ITEMS];

	int Scale = 1;

	SeqItems = pDoc->Sequences[SelectedSequence].Count;

	MML_Count = 0;
	LoopingPoint = MAX_MML_ITEMS;

	for (i = 0; i < SeqItems; i++) {
		Length	= pDoc->Sequences[SelectedSequence].Length[i];
		if (Length < 0) {
			LoopingPoint = i + Length;
		}
	}

	if (LoopingPoint < 0)
		LoopingPoint = 0;

	for (i = 0; i < SeqItems; i++) {
		Length	= pDoc->Sequences[SelectedSequence].Length[i];
		Value	= pDoc->Sequences[SelectedSequence].Value[i];

		if (Value > 64 || Value < -64) {
			Scale = 2;
		}

		if (i == LoopingPoint)
			MML_Values[MML_Count++] = 0x7FFF;		// set loop point
		
		for (x = 0; x < Length + 1; x++) {
			Lengths[MML_Count] = i;
			MML_Values[MML_Count++] = Value;
		}
	}

	for (i = 0; i < (int)MML_Count; i++) {
		if (MML_Values[i] == 0x7FFF)
			MML.AppendFormat("| ");
		else
			MML.AppendFormat("%i ", MML_Values[i]);
	}

	SetDlgItemText(IDC_MML, MML);


	CWnd *Wnd;

	Wnd = GetDlgItem(IDC_GRAPH);

	const int GRAPH_HEIGHT	= 175;
	const int GRAPH_WIDTH	= /*161*/ 156;

	CDC *pDC = Wnd->GetDC();

	CBrush GrayBrush, WhiteBrush, *OldBrush;
	CPen *OldPen, GrayPen;
	
	GrayBrush.CreateSolidBrush(0xD0D0D0);
	WhiteBrush.CreateSolidBrush(0xF0F0F0);

	OldBrush = pDC->SelectObject(&GrayBrush);

	//pDC->Rectangle(0, 0, GRAPH_WIDTH, GRAPH_HEIGHT);
	
	GrayPen.CreatePen(PS_DOT, 1, 0x808080);

	int Pos = 0;
	int Line;

	for (i = 0; i < (int)MML_Count; i++) {
		if (MML_Values[i] != 0x7FFF) {

			if (Lengths[i] & 0x01)
				pDC->FillRect(CRect((Pos * GRAPH_WIDTH) / MML_Count + 1, 1, ((Pos + 1) * GRAPH_WIDTH) / MML_Count + 1, GRAPH_HEIGHT), &GrayBrush);
			else
				pDC->FillRect(CRect((Pos * GRAPH_WIDTH) / MML_Count + 1, 1, ((Pos + 1) * GRAPH_WIDTH) / MML_Count + 1, GRAPH_HEIGHT), &WhiteBrush);

			Line = (GRAPH_HEIGHT / 2) - (MML_Values[i] / Scale);

			pDC->MoveTo((Pos * GRAPH_WIDTH) / MML_Count + 1, Line);
			pDC->LineTo(((Pos + 1) * GRAPH_WIDTH) / MML_Count + 1, Line);
			Pos++;
		}
		else {
			pDC->MoveTo((Pos * GRAPH_WIDTH + 1) / MML_Count, 0);
			pDC->LineTo((Pos * GRAPH_WIDTH + 1) / MML_Count, GRAPH_HEIGHT);
		}
	}

	OldPen = pDC->SelectObject(&GrayPen);

	pDC->MoveTo(0, GRAPH_HEIGHT / 2);
	pDC->LineTo(GRAPH_WIDTH, GRAPH_HEIGHT / 2);

	pDC->SelectObject(OldBrush);
	pDC->SelectObject(OldPen);

	Wnd->ReleaseDC(pDC);

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
	stInstrument *InstConf = &pDoc->Instruments[pView->m_iInstrument];
	CString Text;
	int Index;
	int Setting;

	Setting = SelectedSetting;
	Index	= GetDlgItemInt(IDC_SEQ_INDEX);

	Text.Format("%i", Index);
	m_pSettingsListCtrl->SetItemText(Setting, 1, Text);

	InstConf->ModIndex[Setting] = Index;

	FillSequenceList(Index);
}

void CInstrumentSettings::OnLvnItemchangedSequenceList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	char Text[256];
	unsigned int Length, Value;
	int Item = pNMLV->iItem;

	if (pNMLV->uOldState != 0)
		return;

	if (ChangedLength)
		return;

	m_pSequenceListCtrl = (CListCtrl*)GetDlgItem(IDC_SEQUENCE_LIST);

	Length = Value = 0;

	m_pSequenceListCtrl->GetItemText(Item, 0, Text, 256);
	sscanf(Text, "%i", &Length);

	m_pSequenceListCtrl->GetItemText(Item, 1, Text, 256);
	sscanf(Text, "%i", &Value);

	SelectedSeqItem = Item;

	SetDlgItemInt(IDC_LENGTH, Length);
	SetDlgItemInt(IDC_VALUE, Value);
	
	*pResult = 0;
}

void CInstrumentSettings::OnBnClickedInsert()
{
	int Index;

	m_pSequenceListCtrl = (CListCtrl*)GetDlgItem(IDC_SEQUENCE_LIST);

	if (m_pSequenceListCtrl->GetItemCount() == 0 )
		Index = 0;
	else {
		if (m_pSequenceListCtrl->GetSelectionMark() == -1)
			Index = m_pSequenceListCtrl->GetItemCount();
		else
			Index = m_pSequenceListCtrl->GetSelectionMark();
	}

	pDoc->InsertModifierItem(Index, SelectedSequence);

	FillSequenceList(SelectedSequence);

	m_pSequenceListCtrl->SetSelectionMark(Index);
}

void CInstrumentSettings::OnBnClickedRemove()
{
	int Index;

	m_pSequenceListCtrl = (CListCtrl*)GetDlgItem(IDC_SEQUENCE_LIST);

	if (m_pSequenceListCtrl->GetItemCount() == 0 )
		Index = 0;
	else {
		if (m_pSequenceListCtrl->GetSelectionMark() == -1)
			Index = m_pSequenceListCtrl->GetItemCount();
		else
			Index = m_pSequenceListCtrl->GetSelectionMark();
	}

	pDoc->RemoveModifierItem(Index, SelectedSequence);

	FillSequenceList(SelectedSequence);
}

void CInstrumentSettings::OnEnChangeLength()
{
	CString Text;
	char Text2[256];
	BOOL Translated;
	int Length;

	ChangedLength = true;

	Length = GetDlgItemInt(IDC_LENGTH, &Translated);

	if (Translated == FALSE) {
		ChangedLength = false;
		return;
	}

	m_pSequenceListCtrl = (CListCtrl*)GetDlgItem(IDC_SEQUENCE_LIST);
	pDoc->Sequences[SelectedSequence].Length[SelectedSeqItem] = Length;

	Text.Format("%i", Length);

	m_pSequenceListCtrl->GetItemText(SelectedSeqItem, 0, Text2, 256);

	if (Text.Compare(Text2) == 0) {
		ChangedLength = false;
		return;
	}

	m_pSequenceListCtrl->SetItemText(SelectedSeqItem, 0, Text);

	FillMML();

	ChangedLength = false;
}

void CInstrumentSettings::OnEnChangeValue()
{
	CString Text;
	char Text2[256];
	BOOL Translated;
	int Value;

	Value = GetDlgItemInt(IDC_VALUE, &Translated);

	if (Translated == FALSE) {
		return;
	}

	m_pSequenceListCtrl = (CListCtrl*)GetDlgItem(IDC_SEQUENCE_LIST);
	pDoc->Sequences[SelectedSequence].Value[SelectedSeqItem] = Value;

	Text.Format("%i", Value);

	m_pSequenceListCtrl->GetItemText(SelectedSeqItem, 1, Text2, 256);

	if (Text.Compare(Text2) == 0) {
		return;
	}

	m_pSequenceListCtrl->SetItemText(SelectedSeqItem, 1, Text);

	FillMML();
}

void CInstrumentSettings::OnDeltaposModLengthSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	CString Text;
	int Pos;

	Pos = GetDlgItemInt(IDC_LENGTH) - ((NMUPDOWN*)pNMHDR)->iDelta;

	if (Pos > 126)
		Pos = 126;
	else if (Pos < -128)
		Pos = -128;

	Text.Format("%i", Pos);
	SetDlgItemText(IDC_LENGTH, Text);

	*pResult = 0;
}

void CInstrumentSettings::OnDeltaposModValueSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	CString Text;
	int Pos;

	Pos = GetDlgItemInt(IDC_VALUE) - ((NMUPDOWN*)pNMHDR)->iDelta;

	if (Pos > 126)
		Pos = 126;
	else if (Pos < -128)
		Pos = -128;

	Text.Format("%i", Pos);
	SetDlgItemText(IDC_VALUE, Text);

	*pResult = 0;
}

void CInstrumentSettings::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	FillMML();
}

void CInstrumentSettings::OnBnClickedParseMml()
{
	CString MML, Accum;

	char c, lc;
	bool First = true, Negative = false;
	int Len, i;
	int Value, Length, LastValue;
	int Items, LoopIndex;

	GetDlgItemText(IDC_MML, MML);

	MML.AppendChar(' ');
	MML.AppendChar(' ');

	Len = MML.GetLength();

	Items	= 0;
	Value	= LastValue = 0;
	Length	= -1;
	c = lc	= 0;

	LoopIndex = -1;

	for (i = 0; i < Len; i++) {
		lc = c;
		c = MML.GetAt(i);

		if (c >= '0' && c <= '9') {
			// Numbers
			Accum.AppendChar(c);
		}
		if (c == '|') {
			// Looping point
			if (First)
				LoopIndex = 0;
			else
				LoopIndex = Items + 1;
		}
		if ((c == ' ' && lc != ' ' && (lc >= '0' && lc <= '9')) || i == (Len - 1)) {
			// Translate into an item
			sscanf(Accum, "%i", &Value);

			if (Negative)
				Value = -Value;

			if (First)
				LastValue = Value;
			
			if (i == (Len - 1)) {
				LastValue = Value;
				Value = !LastValue;
			}

			if (Value == LastValue) {
				Length++;
			}
			else {
				pDoc->Sequences[SelectedSequence].Length[Items] = Length;
				pDoc->Sequences[SelectedSequence].Value[Items] = LastValue;
				Items++;
				Length = 0;
			}

//			if (LastValue == '|')
//				Items++;

			Accum.Format("");
			LastValue = Value;
			First = false;
			Negative = false;
		}
		else if (c == '-') {
			Negative = true;
		}
	}

	if (LoopIndex != -1) {
		pDoc->Sequences[SelectedSequence].Length[Items] = LoopIndex - Items;
		pDoc->Sequences[SelectedSequence].Value[Items] = 0;
		Items++;
	}

	pDoc->Sequences[SelectedSequence].Count = Items;

	FillSequenceList(SelectedSequence);
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
				case 33:	// PgUp
					if (SelectedSeqItem > 0) {
						SelectedSeqItem--;
						m_pSequenceListCtrl = (CListCtrl*)GetDlgItem(IDC_SEQUENCE_LIST);
						m_pSequenceListCtrl->SetSelectionMark(SelectedSeqItem);
						m_pSequenceListCtrl->RedrawWindow();
						SetDlgItemInt(IDC_LENGTH, pDoc->Sequences[SelectedSequence].Length[SelectedSeqItem]);
						SetDlgItemInt(IDC_VALUE, pDoc->Sequences[SelectedSequence].Value[SelectedSeqItem]);
					}
					break;
				case 34:	// PgDn
					if (SelectedSeqItem < (pDoc->Sequences[SelectedSequence].Count - 1)) {
						SelectedSeqItem++;
						m_pSequenceListCtrl = (CListCtrl*)GetDlgItem(IDC_SEQUENCE_LIST);
						m_pSequenceListCtrl->SetSelectionMark(SelectedSeqItem);
						//m_pSequenceListCtrl->RedrawWindow();
						//m_pSequenceListCtrl->SetFocus();
						//m_pSequenceListCtrl->SendMessage(WM_KEYDOWN, 
						m_pSequenceListCtrl->RedrawItems(0, SelectedSeqItem);
						SetDlgItemInt(IDC_LENGTH, pDoc->Sequences[SelectedSequence].Length[SelectedSeqItem]);
						SetDlgItemInt(IDC_VALUE, pDoc->Sequences[SelectedSequence].Value[SelectedSeqItem]);
					}
					break;
			}
			break;
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CInstrumentSettings::OnBnClickedFreeSeq()
{
	CString Text;
	int i;

	for (i = 0; i < MAX_SEQUENCES; i++) {
		if (pDoc->Sequences[i].Count == 0) {
			Text.Format("%i", i);
			m_pSettingsListCtrl->SetItemText(SelectedSetting, 1, Text);
			SetDlgItemText(IDC_SEQ_INDEX, Text);
			pDoc->Instruments[pView->m_iInstrument].ModIndex[SelectedSetting] = i;
			FillSequenceList(i);
			return;
		}
	}
}
