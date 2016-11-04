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
#include "ConfigShortcuts.h"
#include "Accelerator.h"
#include "Settings.h"

// CConfigShortcuts dialog

CAccelerator Accelerator;

IMPLEMENT_DYNAMIC(CConfigShortcuts, CPropertyPage)
CConfigShortcuts::CConfigShortcuts()
	: CPropertyPage(CConfigShortcuts::IDD)
{
}

CConfigShortcuts::~CConfigShortcuts()
{
}

void CConfigShortcuts::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConfigShortcuts, CPropertyPage)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_SHORTCUTS, OnLvnItemchangedShortcuts)
	ON_NOTIFY(NM_CLICK, IDC_SHORTCUTS, OnNMClickShortcuts)
	ON_CBN_SELCHANGE(IDC_MODIFIERS, OnCbnSelchangeModifiers)
	ON_CBN_SELCHANGE(IDC_KEYS, OnCbnSelchangeKeys)
	ON_BN_CLICKED(IDC_DEFAULT, OnBnClickedDefault)
END_MESSAGE_MAP()


// CConfigShortcuts message handlers

BOOL CConfigShortcuts::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CAccelerator *pAccel = theApp.GetAccelerator();
	CListCtrl *pListView = (CListCtrl*)GetDlgItem(IDC_SHORTCUTS);
	CComboBox *pModifiers = (CComboBox*)GetDlgItem(IDC_MODIFIERS);
	CComboBox *pKeys = (CComboBox*)GetDlgItem(IDC_KEYS);

	pListView->DeleteAllItems();
	pListView->InsertColumn(0, _T("Action"), LVCFMT_LEFT, 170);
	pListView->InsertColumn(1, _T("Modifier"), LVCFMT_LEFT, 105);
	pListView->InsertColumn(2, _T("Key"), LVCFMT_LEFT, 75);

	int Count = Accelerator.GetItemCount();

	for (int i = 0; i < Count; i++) {
		pListView->InsertItem(i, Accelerator.GetItemName(i), 0);
		pListView->SetItemText(i, 1, Accelerator.GetModName(i));
		pListView->SetItemText(i, 2, Accelerator.GetKeyName(i));
	}

	pModifiers->AddString(CAccelerator::MOD_NAMES[MOD_NONE]);
	pModifiers->AddString(CAccelerator::MOD_NAMES[MOD_SHIFT]);
	pModifiers->AddString(CAccelerator::MOD_NAMES[MOD_CONTROL]);
	pModifiers->AddString(CAccelerator::MOD_NAMES[MOD_ALT]);
	pModifiers->AddString(CAccelerator::MOD_NAMES[MOD_ALT | MOD_CONTROL]);
	pModifiers->AddString(CAccelerator::MOD_NAMES[MOD_ALT | MOD_SHIFT]);
	pModifiers->AddString(CAccelerator::MOD_NAMES[MOD_CONTROL | MOD_SHIFT]);
	pModifiers->AddString(CAccelerator::MOD_NAMES[MOD_ALT | MOD_CONTROL | MOD_SHIFT]);

	pKeys->AddString(_T("None"));

	for (int i = 0; i < 0xFF; i++) {
		LPCTSTR Name = Accelerator.EnumKeyNames(i);
		if (_tcslen(Name) > 0)
			pKeys->AddString(Name);
	}
	
	pListView->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	pListView->SetSelectionMark(0);
	pModifiers->SetCurSel(0);
	pKeys->SetCurSel(0);

	m_iSelectedItem = 0;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigShortcuts::OnLvnItemchangedShortcuts(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

void CConfigShortcuts::OnNMClickShortcuts(NMHDR *pNMHDR, LRESULT *pResult)
{
	CAccelerator *pAccel = theApp.GetAccelerator();
	CListCtrl *pListView = (CListCtrl*)GetDlgItem(IDC_SHORTCUTS);
	CComboBox *pModifiers = (CComboBox*)GetDlgItem(IDC_MODIFIERS);
	CComboBox *pKeys = (CComboBox*)GetDlgItem(IDC_KEYS);

	m_iSelectedItem = pListView->GetSelectionMark();

	CString Name = pListView->GetItemText(m_iSelectedItem, 0);

	int Item = pAccel->GetItem(Name);

	if (Item == -1)
		return;

	pModifiers->SetCurSel(pModifiers->FindStringExact(/*-1*/0, pAccel->GetModName(Item)));
	pKeys->SetCurSel(pKeys->FindStringExact(0, pAccel->GetKeyName(Item)));

	*pResult = 0;
}

void CConfigShortcuts::OnCbnSelchangeModifiers()
{
	CAccelerator *pAccel = theApp.GetAccelerator();
	CListCtrl *pListView = (CListCtrl*)GetDlgItem(IDC_SHORTCUTS);
	CComboBox *pModifiers = (CComboBox*)GetDlgItem(IDC_MODIFIERS);
	CString ModText;

	int CurrentMod = pModifiers->GetCurSel();

	pModifiers->GetLBText(CurrentMod, ModText);
	pListView->SetItemText(m_iSelectedItem, 1, ModText);

//	pAccel->SelectMod(m_iSelectedItem, CurrentMod);
	SetModified();
}

void CConfigShortcuts::OnCbnSelchangeKeys()
{
	CAccelerator *pAccel = theApp.GetAccelerator();
	CListCtrl *pListView = (CListCtrl*)GetDlgItem(IDC_SHORTCUTS);
	CComboBox *pKeys = (CComboBox*)GetDlgItem(IDC_KEYS);

	int CurrentKey = pKeys->GetCurSel();
	CString KeyText;

	pKeys->GetLBText(CurrentKey, KeyText);
	pListView->SetItemText(m_iSelectedItem, 2, KeyText);

//	pAccel->SelectKey(m_iSelectedItem, KeyText);
	SetModified();
}

void CConfigShortcuts::OnBnClickedDefault()
{
	CAccelerator *pAccel = theApp.GetAccelerator();
	CListCtrl *pListView = (CListCtrl*)GetDlgItem(IDC_SHORTCUTS);
	
	if (AfxMessageBox(_T("Do you want to restore default keys? There's no undo."), MB_ICONWARNING | MB_YESNO, 0) == IDNO)
		return;

	pAccel->LoadDefaults();

	pListView->DeleteAllItems();

	int Count = Accelerator.GetItemCount();

	for (int i = 0; i < Count; i++) {
		pListView->InsertItem(i, Accelerator.GetItemName(i), 0);
		pListView->SetItemText(i, 1, Accelerator.GetModName(i));
		pListView->SetItemText(i, 2, Accelerator.GetKeyName(i));
	}
}

BOOL CConfigShortcuts::OnApply()
{
	CAccelerator *pAccel = theApp.GetAccelerator();
	CListCtrl *pListView = (CListCtrl*)GetDlgItem(IDC_SHORTCUTS);
	
	int Count = Accelerator.GetItemCount();

	for (int i = 0; i < Count; i++) {
		CString ModTxt = pListView->GetItemText(i, 1);

		for (int j = 0; j < 7; ++j) {
			if (ModTxt == CAccelerator::MOD_NAMES[j]) {
				pAccel->SelectMod(i, j);
				break;
			}
		}

		pAccel->SelectKey(i, pListView->GetItemText(i, 2));
	}

	return CPropertyPage::OnApply();
}
