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
#include "SequenceSetting.h"

enum {MENU_ARP_ABSOLUTE, MENU_ARP_RELATIVE, MENU_ARP_FIXED};

IMPLEMENT_DYNAMIC(CSequenceSetting, CWnd)

CSequenceSetting::CSequenceSetting(CWnd *pParent) 
	: CWnd(), m_pParent(pParent), m_pSequence(NULL)
{
}

CSequenceSetting::~CSequenceSetting()
{
}

BEGIN_MESSAGE_MAP(CSequenceSetting, CWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

int mode = 0;

void CSequenceSetting::Setup(CFont *pFont)
{
	m_menuPopup.CreatePopupMenu();

	m_menuPopup.AppendMenu(MF_STRING, MENU_ARP_ABSOLUTE, _T("Absolute"));
	m_menuPopup.AppendMenu(MF_STRING, MENU_ARP_RELATIVE, _T("Relative"));
	m_menuPopup.AppendMenu(MF_STRING, MENU_ARP_FIXED, _T("Fixed"));

	m_pFont = pFont;
}

void CSequenceSetting::OnPaint()
{
	CPaintDC dc(this);

	CRect rect;
	GetClientRect(&rect);

	dc.FillSolidRect(rect, 0);
	dc.Draw3dRect(rect, 0x808080, 0xFFFFFF);

	dc.SelectObject(m_pFont);

	dc.SetTextColor(0xFFFFFF);
	dc.SetBkColor(0);
//		dc.TextOut(m_MenuRect.top + 2, m_MenuRect.left + 2, "Relative");

	LPCTSTR MODES[] = {_T("Absolute"), _T("Relative"), _T("Fixed")};
	dc.TextOut(2, 2, MODES[mode]);
}

void CSequenceSetting::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rect;
	GetWindowRect(rect);

	m_menuPopup.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x + rect.left, point.y + rect.top, this);

	CWnd::OnLButtonDown(nFlags, point);
}


BOOL CSequenceSetting::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message) {
		case WM_COMMAND:
			switch (pMsg->wParam) {
				case MENU_ARP_ABSOLUTE:
					mode = 0;
					break;
				case MENU_ARP_RELATIVE:
					mode = 1;
					break;
				case MENU_ARP_FIXED:
					mode = 2;
					break;
		}
		((CSequenceEditor*)m_pParent)->ChangedSetting();
	}

	return CWnd::PreTranslateMessage(pMsg);
}

void CSequenceSetting::SelectSequence(CSequence *pSequence, int Type, int InstrumentType)
{
	m_pSequence = pSequence;
	m_iType		= Type;
	m_iInstType = InstrumentType;

	if (m_iType == SEQ_ARPEGGIO) {
		ShowWindow(SW_SHOW);
	}
	else {
		ShowWindow(SW_HIDE);
	}

	RedrawWindow();
}
