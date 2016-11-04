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
#include "ConfigGeneral.h"
//#include ".\configgeneral.h"
//#include "..\include\configgeneral.h"


// CConfigGeneral dialog

IMPLEMENT_DYNAMIC(CConfigGeneral, CPropertyPage)
CConfigGeneral::CConfigGeneral()
	: CPropertyPage(CConfigGeneral::IDD)
{
}

CConfigGeneral::~CConfigGeneral()
{
}

void CConfigGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConfigGeneral, CPropertyPage)
	ON_BN_CLICKED(IDC_OPT_WRAPCURSOR, OnBnClickedOptWrapcursor)
	ON_BN_CLICKED(IDC_OPT_FREECURSOR, OnBnClickedOptFreecursor)
	ON_BN_CLICKED(IDC_OPT_WAVEPREVIEW, OnBnClickedOptWavepreview)
	ON_BN_CLICKED(IDC_OPT_KEYREPEAT, OnBnClickedOptKeyrepeat)
	ON_BN_CLICKED(IDC_STYLE1, OnBnClickedStyle1)
	ON_BN_CLICKED(IDC_STYLE2, OnBnClickedStyle2)
	ON_BN_CLICKED(IDC_OPT_HEXROW, OnBnClickedOptHexadecimal)
	ON_BN_CLICKED(IDC_OPT_KEYSELECT, OnBnClickedOptKeyselect)
END_MESSAGE_MAP()


// CConfigGeneral message handlers

BOOL CConfigGeneral::OnSetActive()
{
	CheckDlgButton(IDC_OPT_WRAPCURSOR, m_bWrapCursor);
	CheckDlgButton(IDC_OPT_FREECURSOR, m_bFreeCursorEdit);
	CheckDlgButton(IDC_OPT_WAVEPREVIEW, m_bPreviewWAV);
	CheckDlgButton(IDC_OPT_KEYREPEAT, m_bKeyRepeat);
	CheckDlgButton(IDC_OPT_HEXROW, m_bRowInHex);
	CheckDlgButton(IDC_OPT_KEYSELECT, m_bKeySelect);
	CheckDlgButton(IDC_STYLE1, m_iEditStyle == EDIT_STYLE1);
	CheckDlgButton(IDC_STYLE2, m_iEditStyle == EDIT_STYLE2);

	return CPropertyPage::OnSetActive();
}

void CConfigGeneral::OnOK()
{
	CPropertyPage::OnOK();
}

void CConfigGeneral::OnBnClickedOptWrapcursor()
{
	m_bWrapCursor = IsDlgButtonChecked(IDC_OPT_WRAPCURSOR) != 0;
	SetModified();
}

void CConfigGeneral::OnBnClickedOptFreecursor()
{
	m_bFreeCursorEdit = IsDlgButtonChecked(IDC_OPT_FREECURSOR) != 0;
	SetModified();
}

void CConfigGeneral::OnBnClickedOptWavepreview()
{
	m_bPreviewWAV = IsDlgButtonChecked(IDC_OPT_WAVEPREVIEW) != 0;
	SetModified();
}

void CConfigGeneral::OnBnClickedOptKeyrepeat()
{
	m_bKeyRepeat = IsDlgButtonChecked(IDC_OPT_KEYREPEAT) != 0;
	SetModified();
}

BOOL CConfigGeneral::OnApply()
{
	theApp.m_pSettings->General.bWrapCursor		= m_bWrapCursor;
	theApp.m_pSettings->General.bFreeCursorEdit = m_bFreeCursorEdit;
	theApp.m_pSettings->General.bWavePreview	= m_bPreviewWAV;
	theApp.m_pSettings->General.bKeyRepeat		= m_bKeyRepeat;
	theApp.m_pSettings->General.bRowInHex		= m_bRowInHex;
	theApp.m_pSettings->General.iEditStyle		= m_iEditStyle;
	theApp.m_pSettings->General.bKeySelect		= m_bKeySelect;

	return CPropertyPage::OnApply();
}

BOOL CConfigGeneral::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_bWrapCursor		= theApp.m_pSettings->General.bWrapCursor;
	m_bFreeCursorEdit	= theApp.m_pSettings->General.bFreeCursorEdit;
	m_bPreviewWAV		= theApp.m_pSettings->General.bWavePreview;
	m_bKeyRepeat		= theApp.m_pSettings->General.bKeyRepeat;
	m_bRowInHex			= theApp.m_pSettings->General.bRowInHex;
	m_iEditStyle		= theApp.m_pSettings->General.iEditStyle;
	m_bKeySelect		= theApp.m_pSettings->General.bKeySelect;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigGeneral::OnBnClickedStyle1()
{
	m_iEditStyle = EDIT_STYLE1;
	SetModified();
}

void CConfigGeneral::OnBnClickedStyle2()
{
	m_iEditStyle = EDIT_STYLE2;
	SetModified();
}

void CConfigGeneral::OnBnClickedOptHexadecimal()
{
	m_bRowInHex = IsDlgButtonChecked(IDC_OPT_HEXROW) != 0;
	SetModified();
}

void CConfigGeneral::OnBnClickedOptKeyselect()
{
	m_bKeySelect = IsDlgButtonChecked(IDC_OPT_KEYSELECT) != 0;
	SetModified();
}
