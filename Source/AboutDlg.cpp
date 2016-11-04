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

#define WINVER 0x0500 

#include "stdafx.h"
#include "FamiTracker.h"
#include "AboutDlg.h"
#include "CustomExporters.h"

// CAboutDlg dialog used for App About

BEGIN_MESSAGE_MAP(CLinkLabel, CStatic)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

CLinkLabel::CLinkLabel(char *address)
{
	m_pAddress = address;
}

HBRUSH CLinkLabel::CtlColor(CDC* pDC, UINT /*nCtlColor*/)
{
	pDC->SetTextColor(0xFF0000);
	pDC->SetBkMode(TRANSPARENT);
	return (HBRUSH)GetStockObject(NULL_BRUSH);
}

void CLinkLabel::OnLButtonUp(UINT nFlags, CPoint point)
{
	ShellExecute(NULL, "open", m_pAddress, NULL, NULL, SW_SHOWNORMAL);
	CStatic::OnLButtonUp(nFlags, point);
}


BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD), m_pMail(NULL), m_pWeb(NULL)
{
}

CAboutDlg::~CAboutDlg()
{
	if (m_pMail)
		delete m_pMail;
	if (m_pWeb)
		delete m_pWeb;
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL CAboutDlg::OnInitDialog()
{
	CString aboutString;
	CString beta;
#ifdef WIP
	beta = "beta";
#else
	beta = "";
#endif
	aboutString.Format("FamiTracker Version %i.%i.%i %s\n\nA Famicom/NES music tracker", VERSION_MAJ, VERSION_MIN, VERSION_REV, beta);
	SetDlgItemTextA(IDC_ABOUT, aboutString);

	m_pMail = new CLinkLabel("mailto:zxy965r@tninet.se");
	m_pWeb = new CLinkLabel("http://famitracker.shoodot.net");

	m_pMail->SubclassDlgItem(IDC_MAIL, this);
	m_pWeb->SubclassDlgItem(IDC_WEBPAGE, this);

	LOGFONT LogFont;
	CFont *pFont;
	
	pFont = m_pMail->GetFont();
	pFont->GetLogFont(&LogFont);
	LogFont.lfUnderline = 1;
	CFont *newFont = new CFont();
	newFont->CreateFontIndirectA(&LogFont);
	m_pMail->SetFont(newFont);
	m_pWeb->SetFont(newFont);

	return TRUE;
}
