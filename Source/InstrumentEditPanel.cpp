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
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "InstrumentEditPanel.h"
#include "InstrumentEditDlg.h"

using namespace std;

// CInstrumentEditPanel dialog
//
// Base class for instrument editors
//

IMPLEMENT_DYNAMIC(CInstrumentEditPanel, CDialog)
CInstrumentEditPanel::CInstrumentEditPanel(UINT nIDTemplate, CWnd* pParent) : CDialog(nIDTemplate, pParent)
{
}

CInstrumentEditPanel::~CInstrumentEditPanel()
{
}

void CInstrumentEditPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CInstrumentEditPanel, CDialog)
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_WM_LBUTTONDOWN()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

//COLORREF m_iBGColor = 0xFF0000;

BOOL CInstrumentEditPanel::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

HBRUSH CInstrumentEditPanel::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO: Find a proper way to get the background color
	//m_iBGColor = GetPixel(pDC->m_hDC, 2, 2);

	if (!theApp.IsThemeActive())
		return hbr;
	
	switch (nCtlColor) {
		case CTLCOLOR_STATIC:
//		case CTLCOLOR_DLG:
			pDC->SetBkMode(TRANSPARENT);
			// TODO: this might fail on some themes?
			//return NULL;
			return GetSysColorBrush(COLOR_3DHILIGHT);
			//return CreateSolidBrush(m_iBGColor);
	}

	return hbr;
}

BOOL CInstrumentEditPanel::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message) {
		case WM_KEYDOWN:
			switch (pMsg->wParam) {
				case 13:	// Return
					pMsg->wParam = 0;
					OnKeyReturn();
					return TRUE;
				case 27:	// Esc, close the dialog
					((CInstrumentEditDlg*)GetParent())->DestroyWindow();
					return TRUE;
				default:	// Note keys
					// Make sure the dialog is selected when previewing
					if (GetFocus() == this) {
						// Remove repeated keys
						if ((pMsg->lParam & (1 << 30)) == 0)
							PreviewNote((unsigned char)pMsg->wParam);
						return TRUE;
					}
			}
			break;
		case WM_KEYUP:
			PreviewRelease((unsigned char)pMsg->wParam);
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CInstrumentEditPanel::OnLButtonDown(UINT nFlags, CPoint point)
{
	// Set focus on mouse clicks to enable note preview from keyboard
	SetFocus();
	CDialog::OnLButtonDown(nFlags, point);
}

void CInstrumentEditPanel::OnKeyReturn()
{
	// Empty
}

void CInstrumentEditPanel::OnSetFocus(CWnd* pOldWnd)
{
	// Kill the default handler to avoid setting focus to a child control
	//CDialog::OnSetFocus(pOldWnd);
}

CFamiTrackerDoc *CInstrumentEditPanel::GetDocument() const
{
	// Return selected document
	return static_cast<CInstrumentEditDlg*>(GetParent())->GetDocument();
}

void CInstrumentEditPanel::PreviewNote(unsigned char Key)
{
	static_cast<CFamiTrackerView*>(theApp.GetActiveView())->PreviewNote(Key);
}

void CInstrumentEditPanel::PreviewRelease(unsigned char Key)
{
	static_cast<CFamiTrackerView*>(theApp.GetActiveView())->PreviewRelease(Key);
}


//
// CSequenceInstrumentEditPanel
// 
// For dialog panels with sequence editors. Can translate MML strings 
//

IMPLEMENT_DYNAMIC(CSequenceInstrumentEditPanel, CDialog)

CSequenceInstrumentEditPanel::CSequenceInstrumentEditPanel(UINT nIDTemplate, CWnd* pParent) 
	: CInstrumentEditPanel(nIDTemplate, pParent)
{
}

CSequenceInstrumentEditPanel::~CSequenceInstrumentEditPanel()
{
}

void CSequenceInstrumentEditPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSequenceInstrumentEditPanel, CDialog)
END_MESSAGE_MAP()

void CSequenceInstrumentEditPanel::PreviewNote(unsigned char Key)
{
	// Skip if MML window has focus
	if (GetDlgItem(IDC_SEQUENCE_STRING) != GetFocus())
		static_cast<CFamiTrackerView*>(theApp.GetActiveView())->PreviewNote(Key);
}

void CSequenceInstrumentEditPanel::PreviewRelease(unsigned char Key)
{
	static_cast<CFamiTrackerView*>(theApp.GetActiveView())->PreviewRelease(Key);
}

void CSequenceInstrumentEditPanel::TranslateMML(CString String, CSequence *pSequence, int Max, int Min)
{
	// Takes a string and translates it into a sequence

	int AddedItems = 0;

	// Reset loop points
	pSequence->SetLoopPoint(-1);
	pSequence->SetReleasePoint(-1);

	string str;
	str.assign(String);
	istringstream values(str);
	istream_iterator<string> begin(values);
	istream_iterator<string> end;

	while (begin != end && AddedItems < MAX_SEQUENCE_ITEMS) {
		string item = *begin++;

		if (item[0] == '|') {
			// Set loop point
			pSequence->SetLoopPoint(AddedItems);
		}
		else if (item[0] == '/') {
			// Set release point
			pSequence->SetReleasePoint(AddedItems);
		}
		else {
			// Convert to number
			int value = _ttoi(item.c_str());
			// Check for invalid chars
			if (!(value == 0 && item[0] != '0')) {
				LIMIT(value, Max, Min);
				pSequence->SetItem(AddedItems++, value);
			}
		}
	}

	pSequence->SetItemCount(AddedItems);
	
	// Update editor
//	m_pSequenceEditor->RedrawWindow();

	// Register a document change
//	theApp.GetFirstDocument()->SetModifiedFlag();

	// Enable setting
//	((CListCtrl*)GetDlgItem(IDC_INSTSETTINGS))->SetCheck(m_iSelectedSetting, 1);
}
