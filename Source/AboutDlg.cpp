/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2012  Jonathan Liss
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
#include "AboutDlg.h"
#include "CustomExporters.h"

// CAboutDlg dialog used for App About

LPCTSTR LINK_WEB  = _T("http://www.famitracker.com");
LPCTSTR LINK_MAIL = _T("mailto:jsr@famitracker.com");

BEGIN_MESSAGE_MAP(CLinkLabel, CStatic)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSELEAVE()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

CLinkLabel::CLinkLabel(CString address)
{
	m_strAddress = address;
	m_bHover = false;
}

HBRUSH CLinkLabel::CtlColor(CDC* pDC, UINT /*nCtlColor*/)
{
	pDC->SetTextColor(m_bHover ? 0x0000FF : 0xFF0000);
	pDC->SetBkMode(TRANSPARENT);
	return (HBRUSH)GetStockObject(NULL_BRUSH);
}

void CLinkLabel::OnLButtonUp(UINT nFlags, CPoint point)
{
	ShellExecute(NULL, _T("open"), m_strAddress, NULL, NULL, SW_SHOWNORMAL);
	CStatic::OnLButtonUp(nFlags, point);
}

void CLinkLabel::OnMouseLeave()
{
	m_bHover = false;
	CRect rect, parentRect;
	GetWindowRect(&rect);
	GetParent()->GetWindowRect(parentRect);
	rect.OffsetRect(-parentRect.left - GetSystemMetrics(SM_CXDLGFRAME), -parentRect.top - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYDLGFRAME));
	GetParent()->RedrawWindow(rect);
	CStatic::OnMouseLeave();
}

void CLinkLabel::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bHover) {
		m_bHover = true;
		CRect rect, parentRect;
		GetWindowRect(&rect);
		GetParent()->GetWindowRect(parentRect);
		rect.OffsetRect(-parentRect.left - GetSystemMetrics(SM_CXDLGFRAME), -parentRect.top - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYDLGFRAME));
		GetParent()->RedrawWindow(rect);

		TRACKMOUSEEVENT t;
		t.cbSize = sizeof(TRACKMOUSEEVENT);
		t.dwFlags = TME_LEAVE;
		t.hwndTrack = m_hWnd;
		TrackMouseEvent(&t);
	}

	CStatic::OnMouseMove(nFlags, point);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD), m_pMail(NULL), m_pWeb(NULL)
{
}

CAboutDlg::~CAboutDlg()
{
	SAFE_RELEASE(m_pMail);
	SAFE_RELEASE(m_pWeb);
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL CAboutDlg::OnInitDialog()
{
	CString aboutString;

#ifdef WIP
	aboutString.Format(_T("FamiTracker Version %i.%i.%i beta %i\n\nA Famicom/NES music tracker"), VERSION_MAJ, VERSION_MIN, VERSION_REV, VERSION_WIP);
#else
	aboutString.Format(_T("FamiTracker Version %i.%i.%i\n\nA Famicom/NES music tracker"), VERSION_MAJ, VERSION_MIN, VERSION_REV);
#endif

	SetDlgItemText(IDC_ABOUT, aboutString);

	m_pMail = new CLinkLabel(LINK_MAIL);
	m_pWeb = new CLinkLabel(LINK_WEB);

	m_pMail->SubclassDlgItem(IDC_MAIL, this);
	m_pWeb->SubclassDlgItem(IDC_WEBPAGE, this);

	LOGFONT LogFont;
	CFont *pFont;
	
	EnableToolTips(TRUE);

	m_wndToolTip.Create(this, TTS_ALWAYSTIP);
	m_wndToolTip.Activate(TRUE);

	m_wndToolTip.AddTool(m_pMail, _T("Send mail to jsr@famitracker.com"));
	m_wndToolTip.AddTool(m_pWeb, _T("Go to http://www.famitracker.com"));

	pFont = m_pMail->GetFont();
	pFont->GetLogFont(&LogFont);
	LogFont.lfUnderline = 1;
	CFont *newFont = new CFont();
	newFont->CreateFontIndirect(&LogFont);
	m_pMail->SetFont(newFont);
	m_pWeb->SetFont(newFont);

	return TRUE;
}


BOOL CAboutDlg::PreTranslateMessage(MSG* pMsg)
{
	m_wndToolTip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}
