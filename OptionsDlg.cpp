/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005  Jonathan Liss
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
#include "OptionsDlg.h"
#include ".\optionsdlg.h"


// COptionsDlg dialog

IMPLEMENT_DYNAMIC(COptionsDlg, CDialog)
COptionsDlg::COptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COptionsDlg::IDD, pParent)
{
}

COptionsDlg::~COptionsDlg()
{
}

void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

COptionsDlg *pCallback;

BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

int CALLBACK EnumFontFamExProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam)
{
	if (lpelfe->elfLogFont.lfCharSet == ANSI_CHARSET && lpelfe->elfFullName[0] != '@')
		pCallback->AddFontName((char*)&lpelfe->elfFullName);

	return 1;
}


// COptionsDlg message handlers

void COptionsDlg::OnBnClickedOk()
{
	theApp.m_bWrapCursor		= IsDlgButtonChecked(IDC_OPT_WRAPCURSOR) != 0;
	theApp.m_bFreeCursorEdit	= IsDlgButtonChecked(IDC_OPT_FREECURSOR) != 0;
	theApp.m_bWavePreview		= IsDlgButtonChecked(IDC_OPT_WAVEPREVIEW) != 0;
	theApp.m_bKeyRepeat			= IsDlgButtonChecked(IDC_OPT_KEYREPEAT) != 0;

	m_pFontList->GetLBText(m_pFontList->GetCurSel(), theApp.m_strFont);

	OnOK();
}

BOOL COptionsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CDC *pDC = GetDC();
	LOGFONT LogFont;

	memset(&LogFont, 0, sizeof(LOGFONT));
	LogFont.lfCharSet = DEFAULT_CHARSET;

	m_pFontList = (CComboBox*)GetDlgItem(IDC_FONT);
	pCallback = this;

	CheckDlgButton(IDC_OPT_WRAPCURSOR, theApp.m_bWrapCursor);
	CheckDlgButton(IDC_OPT_FREECURSOR, theApp.m_bFreeCursorEdit);
	CheckDlgButton(IDC_OPT_WAVEPREVIEW, theApp.m_bWavePreview);
	CheckDlgButton(IDC_OPT_KEYREPEAT, theApp.m_bKeyRepeat);	

	EnumFontFamiliesEx(pDC->m_hDC, &LogFont, (FONTENUMPROC)EnumFontFamExProc, 0, 0);

	ReleaseDC(pDC);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void COptionsDlg::AddFontName(char *Name)
{
	m_pFontList->AddString(Name);

	if (theApp.m_strFont.Compare(Name) == 0)
		m_pFontList->SelectString(0, Name);
}
