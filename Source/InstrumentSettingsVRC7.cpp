/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2009  Jonathan Liss
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
#include "InstrumentSettingsVRC7.h"


// CInstrumentSettingsVRC7 dialog

IMPLEMENT_DYNAMIC(CInstrumentSettingsVRC7, CDialog)

CInstrumentSettingsVRC7::CInstrumentSettingsVRC7(CWnd* pParent /*=NULL*/)
	: CDialog(CInstrumentSettingsVRC7::IDD, pParent)
{

}

CInstrumentSettingsVRC7::~CInstrumentSettingsVRC7()
{
}

void CInstrumentSettingsVRC7::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CInstrumentSettingsVRC7, CDialog)
	ON_CBN_SELCHANGE(IDC_PATCH, &CInstrumentSettingsVRC7::OnCbnSelchangePatch)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


// CInstrumentSettingsVRC7 message handlers

BOOL CInstrumentSettingsVRC7::OnInitDialog()
{
	CDialog::OnInitDialog();

	CComboBox *pPatchBox = (CComboBox*)GetDlgItem(IDC_PATCH);
	CString Text;

	for (int i = 0; i < 16; i++) {
		Text.Format("Patch #%i", i);
		pPatchBox->AddString(Text);
	}

	pPatchBox->SetCurSel(0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentSettingsVRC7::OnCbnSelchangePatch()
{
	int Patch;
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetDocument();
	CComboBox *pPatchBox = (CComboBox*)GetDlgItem(IDC_PATCH);
	Patch = pPatchBox->GetCurSel();
	CInstrumentVRC7 *InstConf = (CInstrumentVRC7*)pDoc->GetInstrument(m_iInstrument);
	InstConf->SetPatch(Patch);
}

void CInstrumentSettingsVRC7::SetCurrentInstrument(int Index)
{
	m_iInstrument = Index;
}


BOOL CInstrumentSettingsVRC7::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

