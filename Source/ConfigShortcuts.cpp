// ConfigShortcuts.cpp : implementation file
//

#include "stdafx.h"
#include "FamiTracker.h"
#include "ConfigShortcuts.h"
#include "Accelerator.h"
#include ".\configshortcuts.h"

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
	pListView->InsertColumn(0, "Action", LVCFMT_LEFT, 200);
	pListView->InsertColumn(1, "Modifier", LVCFMT_LEFT, 75);
	pListView->InsertColumn(2, "Key", LVCFMT_LEFT, 75);

	int Count = Accelerator.GetItemCount();

	for (int i = 0; i < Count; i++) {
		pListView->InsertItem(i, Accelerator.GetItemName(i), 0);
		pListView->SetItemText(i, 1, Accelerator.GetModName(i));
		pListView->SetItemText(i, 2, Accelerator.GetKeyName(i));
	}

	pModifiers->AddString("None");
	pModifiers->AddString("Shift");
	pModifiers->AddString("Ctrl");
	pModifiers->AddString("Alt");

	pKeys->AddString("None");

	for (int i = 0; i < 0xFF; i++) {
		char *Name = Accelerator.EnumKeyNames(i);
		if (strlen(Name) > 0)
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

	pModifiers->SetCurSel(pModifiers->FindStringExact(-1, pAccel->GetModName(Item)));
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
	
	if (AfxMessageBox("Do you want to restore default keys? There's no undo.", MB_ICONWARNING | MB_YESNO, 0) == IDNO)
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

		if (ModTxt == "None")
			pAccel->SelectMod(i, 0);
		else if (ModTxt == "Shift")
			pAccel->SelectMod(i, MOD_SHIFT);
		else if (ModTxt == "Ctrl")
			pAccel->SelectMod(i, MOD_CONTROL);
		else if (ModTxt == "Alt")
			pAccel->SelectMod(i, MOD_ALT);

		pAccel->SelectKey(i, pListView->GetItemText(i, 2));
	}

	return CPropertyPage::OnApply();
}
