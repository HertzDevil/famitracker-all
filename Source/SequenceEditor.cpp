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
#include "Sequence.h"
#include "SequenceEditor.h"
#include "GraphEditor.h"
#include "InstrumentEditPanel.h"
#include "SizeEditor.h"
#include "SequenceSetting.h"

// This file contains the sequence editor and sequence size control

enum {MENU_ARP_ABSOLUTE = WM_USER, MENU_ARP_RELATIVE, MENU_ARP_FIXED};

// CSequenceEditor

IMPLEMENT_DYNAMIC(CSequenceEditor, CWnd)

CSequenceEditor::CSequenceEditor(CFamiTrackerDoc *pDoc) : CWnd(), 
	m_pGraphEditor(NULL), 
	m_pSizeEditor(NULL),
	m_pFont(NULL),
	m_iMaxVol(15), 
	m_iMaxDuty(3),
	m_pDocument(pDoc)
{
}

CSequenceEditor::~CSequenceEditor()
{
	SAFE_RELEASE(m_pFont);
	SAFE_RELEASE(m_pSizeEditor);
	SAFE_RELEASE(m_pGraphEditor);
}

BEGIN_MESSAGE_MAP(CSequenceEditor, CWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

CRect m_MenuRect;

//CSequenceSetting *m_pSetting;

BOOL CSequenceEditor::CreateEditor(CWnd *pParentWnd, const RECT &rect)
{
	if (CWnd::CreateEx(WS_EX_CLIENTEDGE, NULL, _T(""), WS_CHILD | WS_VISIBLE, rect, pParentWnd, 0) == -1)
		return -1;

	m_pFont = new CFont();
	m_pFont->CreateFont(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, _T("Tahoma"));

	m_pParent = pParentWnd;

	CRect GraphRect;
	GetClientRect(GraphRect);
	GraphRect.bottom -= 25;

	m_pSizeEditor = new CSizeEditor(this);
	
	if (m_pSizeEditor->CreateEx(NULL, NULL, _T(""), WS_CHILD | WS_VISIBLE, CRect(40, GraphRect.bottom + 5, 104, GraphRect.bottom + 22), this, 0) == -1)
		return -1;

	m_MenuRect = CRect(GraphRect.right - 80, GraphRect.bottom + 5, GraphRect.right - 10, GraphRect.bottom + 22);
/*
	m_pSetting = new CSequenceSetting(this);

	if (m_pSetting->CreateEx(NULL, NULL, "", WS_CHILD | WS_VISIBLE, m_MenuRect, this, 0) == -1)
		return -1;

	m_pSetting->Setup(m_pFont);
*/
	return 0;
}

void CSequenceEditor::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rect;
	GetClientRect(rect);

	// Update size editor
	if (m_pSequence)
		m_pSizeEditor->SetValue(m_pSequence->GetItemCount());

	dc.SelectObject(m_pFont);
	dc.TextOut(10, rect.bottom - 19, _T("Size:"));

	CString LengthStr;
	LengthStr.Format(_T("%i ms  "), (1000 * m_pSizeEditor->GetValue()) / m_pDocument->GetFrameRate());

	dc.TextOut(120, rect.bottom - 19, LengthStr);

	// TODO: For arpeggio, make this more general
#if 0
	if (m_iSelectedSetting == SEQ_ARPEGGIO) {		// Temporarily disabled
		dc.FillSolidRect(m_MenuRect, 0);
		dc.Draw3dRect(m_MenuRect, 0x808080, 0xFFFFFF);
		dc.SetTextColor(0xFFFFFF);
		dc.SetBkColor(0);
//		dc.TextOut(m_MenuRect.top + 2, m_MenuRect.left + 2, "Relative");

		static const char* MODES[] = {"Absolute", "Relative", "Fixed"};
		dc.TextOut(280 + 2, rect.bottom - 19, MODES[m_pSequence->GetSetting()]);
	}
	else {
		dc.FillSolidRect(m_MenuRect, 0xFFFFFF);
	}
#endif
}

BOOL CSequenceEditor::PreTranslateMessage(MSG* pMsg)
{
	CDC *pDC;
	CRect rect;
	CString Text;

	switch (pMsg->message) {
		case WM_SIZE_CHANGE:
			// Number of sequence items has changed
			m_pSequence->SetItemCount(pMsg->wParam);
			m_pGraphEditor->RedrawWindow();
			RedrawWindow();
			PostMessage(WM_SEQUENCE_CHANGED, 1);
			return TRUE;
		case WM_CURSOR_CHANGE:
			// Graph cursor has changed
			pDC = GetDC();
			pDC->SelectObject(m_pFont);
			GetClientRect(rect);
			// Arpeggio
			if (m_iSelectedSetting == SEQ_ARPEGGIO && m_pSequence->GetSetting() == 1) {
				Text.Format(_T("{%i, %s}  "), pMsg->wParam, ((CArpeggioGraphEditor*)m_pGraphEditor)->GetNoteString(pMsg->lParam));
			}
			else
				Text.Format(_T("{%i, %i}  "), pMsg->wParam, pMsg->lParam);
			pDC->TextOut(170, rect.bottom - 19, Text);
			ReleaseDC(pDC);
			return TRUE;
		case WM_SEQUENCE_CHANGED:
			if (this == NULL)
				return FALSE;
			SequenceChangedMessage(pMsg->wParam == 1);
			return TRUE;
	}

	return CWnd::PreTranslateMessage(pMsg);
}

void CSequenceEditor::ChangedSetting()
{
	// Called when the setting selector has changed
	SelectSequence(m_pSequence, m_iSelectedSetting, m_iInstrumentType);
}

void CSequenceEditor::SetMaxValues(int MaxVol, int MaxDuty)
{
	m_iMaxVol = MaxVol;
	m_iMaxDuty = MaxDuty;
}

void CSequenceEditor::SequenceChangedMessage(bool Changed)
{
	CString Text;

	// Translate sequence to MML-like string
	Text = "";

	for (unsigned i = 0; i < m_pSequence->GetItemCount(); ++i) {
		if (m_pSequence->GetLoopPoint() == i)
			Text.Append(_T("| "));
		else if (m_pSequence->GetReleasePoint() == i)
			Text.Append(_T("/ "));
		Text.AppendFormat(_T("%i "), m_pSequence->GetItem(i));
	}

	dynamic_cast<CSequenceInstrumentEditPanel*>(m_pParent)->SetSequenceString(Text, Changed);
	//((CSequenceInstrumentEditPanel*)m_pParent)->SetSequenceString(Text, Changed);
}

void CSequenceEditor::SelectSequence(CSequence *pSequence, int Type, int InstrumentType)
{
	// Select a sequence to edit
	m_pSequence = pSequence;
	m_iSelectedSetting = Type;
	m_iInstrumentType = InstrumentType;

	DestroyGraphEditor();

	// Create the graph
	switch (Type) {
		case SEQ_VOLUME:
			m_pGraphEditor = new CBarGraphEditor(pSequence, m_iMaxVol);
			break;
		case SEQ_ARPEGGIO:
			m_pGraphEditor = new CArpeggioGraphEditor(pSequence);
			break;
		case SEQ_PITCH:
		case SEQ_HIPITCH:
			m_pGraphEditor = new CPitchGraphEditor(pSequence);
			break;
		case SEQ_DUTYCYCLE:
			m_pGraphEditor = new CBarGraphEditor(pSequence, m_iMaxDuty);
			break;
			/*
		case SEQ_SUNSOFT_NOISE:
			m_pGraphEditor = new CNoiseEditor(pSequence, 32);
			break;
			*/
	}
/*
	m_pSetting->SelectSequence(pSequence, Type, InstrumentType);
*/
	CRect GraphRect;
	GetClientRect(GraphRect);
	GraphRect.bottom -= 25;

	if (m_pGraphEditor->CreateEx(NULL, NULL, _T(""), WS_CHILD, GraphRect, this, 0) == -1)
		return;

	m_pGraphEditor->UpdateWindow();
	m_pGraphEditor->ShowWindow(SW_SHOW);

	m_pSizeEditor->SetValue(pSequence->GetItemCount());

	Invalidate();
	RedrawWindow();

	// Update sequence string
	SequenceChangedMessage(false);
}

void CSequenceEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
#if 0
	CRect rect;
	GetWindowRect(rect);

	if (m_iSelectedSetting == SEQ_ARPEGGIO) {
		if (m_MenuRect.PtInRect(point)) {
			m_menuPopup.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x + rect.left, point.y + rect.top, this);
		}
	}
#endif
}

BOOL CSequenceEditor::DestroyWindow()
{
	DestroyGraphEditor();
	return CWnd::DestroyWindow();
}

void CSequenceEditor::DestroyGraphEditor()
{
	if (m_pGraphEditor) {
		m_pGraphEditor->ShowWindow(SW_HIDE);
		m_pGraphEditor->DestroyWindow();
		delete m_pGraphEditor;
		m_pGraphEditor = NULL;
	}
}

void CSequenceEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	CWnd::OnLButtonDown(nFlags, point);
	// Set focus to parent to allow keyboard note preview
	GetParent()->SetFocus();
}
