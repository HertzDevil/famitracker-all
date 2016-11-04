/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2007  Jonathan Liss
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
#include "FamiTrackerView.h"
#include "InstrumentDPCM.h"
#include "PCMImport.h"
#include "..\include\instrumentdpcm.h"

const char *KEY_NAMES[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

// CInstrumentDPCM dialog

IMPLEMENT_DYNAMIC(CInstrumentDPCM, CDialog)
CInstrumentDPCM::CInstrumentDPCM(CWnd* pParent /*=NULL*/)
	: CDialog(CInstrumentDPCM::IDD/*, pParent*/)
{
}

CInstrumentDPCM::~CInstrumentDPCM()
{
}

void CInstrumentDPCM::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CInstrumentDPCM, CDialog)
	ON_BN_CLICKED(IDC_LOAD, OnBnClickedLoad)
	ON_BN_CLICKED(IDC_UNLOAD, OnBnClickedUnload)
	ON_NOTIFY(NM_CLICK, IDC_SAMPLE_LIST, OnNMClickSampleList)
	ON_BN_CLICKED(IDC_IMPORT, OnBnClickedImport)
	ON_CBN_SELCHANGE(IDC_OCTAVE, OnCbnSelchangeOctave)
	ON_CBN_SELCHANGE(IDC_PITCH, OnCbnSelchangePitch)
	ON_NOTIFY(NM_CLICK, IDC_TABLE, OnNMClickTable)
	ON_CBN_SELCHANGE(IDC_SAMPLES, OnCbnSelchangeSamples)
	ON_BN_CLICKED(IDC_SAVE, OnBnClickedSave)
	ON_BN_CLICKED(IDC_LOOP, OnBnClickedLoop)
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_ADD, OnBnClickedAdd)
	ON_BN_CLICKED(IDC_REMOVE, OnBnClickedRemove)
END_MESSAGE_MAP()

// CInstrumentDPCM message handlers

BOOL CInstrumentDPCM::OnInitDialog()
{
	CDialog::OnInitDialog();

	CComboBox *pPitch, *pOctave;
	CString Text;
	int i;

	m_iOctave = 3;
	m_iSelectedKey = 0;

	pDoc	= reinterpret_cast<CFamiTrackerDoc*>(theApp.GetDocument());
	pView	= reinterpret_cast<CFamiTrackerView*>(theApp.GetView());

	pPitch	= reinterpret_cast<CComboBox*>(GetDlgItem(IDC_PITCH));
	pOctave = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_OCTAVE));

	m_pTableListCtrl = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));
	m_pTableListCtrl->DeleteAllItems();
	m_pTableListCtrl->InsertColumn(0, "Key", LVCFMT_LEFT, 34);
	m_pTableListCtrl->InsertColumn(1, "Pitch", LVCFMT_LEFT, 38);
	m_pTableListCtrl->InsertColumn(2, "Sample", LVCFMT_LEFT, 91);
	m_pTableListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	m_pSampleListCtrl = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_SAMPLE_LIST));
	m_pSampleListCtrl->DeleteAllItems();
	m_pSampleListCtrl->InsertColumn(0, "#", LVCFMT_LEFT, 22);
	m_pSampleListCtrl->InsertColumn(1, "Name", LVCFMT_LEFT, 88);
	m_pSampleListCtrl->InsertColumn(2, "Size", LVCFMT_LEFT, 39);
	m_pSampleListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	for (i = 0; i < 16; i++) {
		Text.Format("%i", i);
		pPitch->AddString(Text);
	}

	pPitch->SetCurSel(15);

	for (i = 0; i < OCTAVE_RANGE; i++) {
		Text.Format("%i", i);
		pOctave->AddString(Text);
	}

	pOctave->SetCurSel(3);

	CheckDlgButton(IDC_LOOP, 0);

	m_pTableListCtrl->DeleteAllItems();

	for (i = 0; i < 12; i++) {
		m_pTableListCtrl->InsertItem(i, KEY_NAMES[i], 0);
	}

	BuildSampleList();

	SetCurrentInstrument(pView->GetInstrument());

	m_iSelectedSample = 0;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentDPCM::BuildKeyList()
{
	for (int i = 0; i < 12; i++) {
		UpdateKey(i);
	}
}

void CInstrumentDPCM::UpdateKey(int Index)
{
	int Pitch, Item;
	char Name[256];

	CInstrument2A03 *pInst = (CInstrument2A03*)pDoc->GetInstrument(pView->GetInstrument());

	m_pTableListCtrl = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));

	if (pInst->GetSample(m_iOctave, Index) > 0) {
		Item = pInst->GetSample(m_iOctave, Index) - 1;
		Pitch = pInst->GetSamplePitch(m_iOctave, Index);

		if (pDoc->GetSampleSize(Item) == 0) {
			strcpy(Name, "(n/a)");
		}
		else {
			pDoc->GetSampleName(Item, Name);
		}
		m_pTableListCtrl->SetItemText(Index, 2, Name);
		sprintf(Name, "%i", Pitch & 0x0F);
		m_pTableListCtrl->SetItemText(Index, 1, Name);
	}
	else {
		sprintf(Name, "(no sample)");
		m_pTableListCtrl->SetItemText(Index, 2, Name);
		sprintf(Name, "-");
		m_pTableListCtrl->SetItemText(Index, 1, Name);
	}
}

void CInstrumentDPCM::BuildSampleList()
{
	CComboBox		*m_pSampleBox;
	CDSample		*pDSample;
	unsigned int	Size, i, Index = 0;
	CString			Text;

	m_pSampleBox		= reinterpret_cast<CComboBox*>(GetDlgItem(IDC_SAMPLES));
	m_pSampleListCtrl	= reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_SAMPLE_LIST));

	m_pSampleListCtrl->DeleteAllItems();
	m_pSampleBox->ResetContent();

	Size = 0;

	m_pSampleBox->AddString("(no sample)");

	for (i = 0; i < MAX_DSAMPLES; i++) {
		pDSample = pDoc->GetDSample(i);
		if (pDSample->SampleSize > 0) {
			Text.Format("%i", i);							m_pSampleListCtrl->InsertItem(Index, Text);
			Text.Format("%s", pDSample->Name);				m_pSampleListCtrl->SetItemText(Index, 1, Text);
			Text.Format("%i", pDSample->SampleSize);		m_pSampleListCtrl->SetItemText(Index, 2, Text);
			Text.Format("%02i - %s", i, pDSample->Name);	m_pSampleBox->AddString(Text);
			Size += pDSample->SampleSize;
			Index++;
		}
	}

	Text.Format("Space used %i kB, left %i kB (16 kB available)", Size / 0x400, (0x4000 - Size) / 0x400);
	SetDlgItemText(IDC_SPACE, Text);
}

// When saved in NSF, the samples has to be aligned at even 6-bits addresses
//#define ADJUST_FOR_STORAGE(x) (((x >> 6) + (x & 0x3F ? 1 : 0)) << 6)
#define ADJUST_FOR_STORAGE(x) (x)

void CInstrumentDPCM::LoadSample(char *FilePath, char *FileName)
{
	CFile SampleFile;

	if (!SampleFile.Open(FilePath, CFile::modeRead)) {
		MessageBox("Could not open file!");
		return;
	}

	CDSample *NewSample = pDoc->GetFreeDSample();
	
	int Size = 0;

	for (int i = 0; i < MAX_DSAMPLES; i++) {
		Size += ADJUST_FOR_STORAGE(pDoc->GetDSample(i)->SampleSize);
	}
	
	if (NewSample != NULL) {
		if ((Size + (int)SampleFile.GetLength()) > 0x4000) {
			MessageBox("Couldn't load sample, out of sample memory (max 16 kB is avaliable)\n");
			NewSample = NULL;
		}
		else {
			sprintf(NewSample->Name, "%s", FileName);
			NewSample->SampleSize = (int)SampleFile.GetLength();
			NewSample->SampleData = new char[NewSample->SampleSize];
			SampleFile.Read(NewSample->SampleData, NewSample->SampleSize);
		}
	}
	
	SampleFile.Close();
	BuildSampleList();
}

void CInstrumentDPCM::OnBnClickedLoad()
{
	char *Path, *FileName;
	char FileNameBuffer[MAX_PATH];

	CFileDialog OpenFileDialog(TRUE, 0, 0, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER, "Delta modulation samples (*.dmc)|*.dmc|All files|*.*||");

	OpenFileDialog.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_DMC);

	if (OpenFileDialog.DoModal() == IDCANCEL)
		return;

	theApp.m_pSettings->SetPath(OpenFileDialog.GetPathName(), PATH_DMC);

	if (OpenFileDialog.GetFileName().GetLength() == 0) {

		Path		= OpenFileDialog.m_pOFN->lpstrFile;
		FileName	= OpenFileDialog.m_pOFN->lpstrFile + OpenFileDialog.m_pOFN->nFileOffset;

		while (*FileName != 0) {
			strcpy(FileNameBuffer, Path);
			strcat(FileNameBuffer, "\\");
			strcat(FileNameBuffer, FileName);
			LoadSample(FileNameBuffer, FileName);
			FileName += strlen(FileName) + 1;
		}
	}
	else {
		LoadSample(OpenFileDialog.GetPathName().GetBuffer(), OpenFileDialog.GetFileName().GetBuffer());
	}
}

void CInstrumentDPCM::OnBnClickedUnload()
{
	if (m_iSelectedSample == MAX_DSAMPLES)
		return;

	pDoc->RemoveDSample(m_iSelectedSample);

	BuildSampleList();
}

void CInstrumentDPCM::OnNMClickSampleList(NMHDR *pNMHDR, LRESULT *pResult)
{
	char ItemName[256];
	int Index;

	m_pSampleListCtrl = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_SAMPLE_LIST));

	if (m_pSampleListCtrl->GetItemCount() == 0)
		return;

	Index = m_pSampleListCtrl->GetSelectionMark();
	m_pSampleListCtrl->GetItemText(Index, 0, ItemName, 256);
	sscanf(ItemName, "%i", &m_iSelectedSample);

	*pResult = 0;
}

void CInstrumentDPCM::OnBnClickedImport()
{
	CPCMImport		ImportDialog;
	stImportedPCM	*Imported;
	CString			Name;

	Imported = ImportDialog.ShowDialog();

	if (Imported->Size == 0)
		return;

	CDSample *NewSample = pDoc->GetFreeDSample();
	
	int Size = 0;

	for (int i = 0; i < MAX_DSAMPLES; i++) {
		Size += ADJUST_FOR_STORAGE(pDoc->GetDSample(i)->SampleSize);
	}
	
	if (NewSample != NULL) {
		if ((Size + Imported->Size) > 0x4000) {
			MessageBox("Couldn't load sample, out of sample memory (max 16 kB is avaliable)\n");
			NewSample = NULL;
		}
		else {
			strcpy(NewSample->Name, Imported->Name);
			NewSample->SampleSize = Imported->Size;
			NewSample->SampleData = new char[NewSample->SampleSize];
			memcpy(NewSample->SampleData, Imported->Data, NewSample->SampleSize);
		}
	}

	delete [] Imported->Data;

	BuildSampleList();
}

void CInstrumentDPCM::OnCbnSelchangeOctave()
{
	CComboBox *m_OctaveBox = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_OCTAVE));
	m_iOctave = m_OctaveBox->GetCurSel();
	BuildKeyList();
}

void CInstrumentDPCM::OnCbnSelchangePitch()
{
	CComboBox *m_pPitchBox	= reinterpret_cast<CComboBox*>(GetDlgItem(IDC_PITCH));
	CInstrument2A03 *pInst = (CInstrument2A03*)pDoc->GetInstrument(pView->GetInstrument());
	int Pitch;

	if (m_iSelectedKey == -1)
		return;

	Pitch = m_pPitchBox->GetCurSel();

	if (IsDlgButtonChecked(IDC_LOOP))
		Pitch |= 0x80;

	pInst->SetSamplePitch(m_iOctave, m_iSelectedKey, Pitch);

	UpdateKey(m_iSelectedKey);
}

void CInstrumentDPCM::OnNMClickTable(NMHDR *pNMHDR, LRESULT *pResult)
{
	CInstrument2A03 *pInst = (CInstrument2A03*)pDoc->GetInstrument(pView->GetInstrument());
	CComboBox *pSampleBox, *pPitchBox;
	CString Text;
	int Sample, Pitch;

	m_pTableListCtrl	= reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));
	m_iSelectedKey		= m_pTableListCtrl->GetSelectionMark();

	Sample				= pInst->GetSample(m_iOctave, m_iSelectedKey) - 1;
	Pitch				= pInst->GetSamplePitch(m_iOctave, m_iSelectedKey);

	m_pTableListCtrl	= static_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));
	pSampleBox			= static_cast<CComboBox*>(GetDlgItem(IDC_SAMPLES));
	pPitchBox			= static_cast<CComboBox*>(GetDlgItem(IDC_PITCH));

	Text.Format("%02i - %s", Sample, m_pTableListCtrl->GetItemText(m_pTableListCtrl->GetSelectionMark(), 2));

	if (Sample != -1)
		pSampleBox->SelectString(0, Text);
	else
		pSampleBox->SetCurSel(0);
	
	if (Sample > 0)
		pPitchBox->SetCurSel(Pitch & 0x0F);

	if (Pitch & 0x80)
		CheckDlgButton(IDC_LOOP, 1);
	else
		CheckDlgButton(IDC_LOOP, 0);

	*pResult = 0;
}

void CInstrumentDPCM::OnCbnSelchangeSamples()
{
	CComboBox *m_pSampleBox = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_SAMPLES));
	CComboBox *pPitchBox = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_PITCH));
	CInstrument2A03 *pInst = (CInstrument2A03*)pDoc->GetInstrument(pView->GetInstrument());
	
	int Sample, PrevSample;
	char Name[256];

	PrevSample = pInst->GetSample(m_iOctave, m_iSelectedKey);

	Sample = m_pSampleBox->GetCurSel();

	if (Sample > 0) {
		m_pSampleBox->GetLBText(Sample, Name);
		
		Name[2] = 0;
		if (Name[0] == '0') {
			Name[0] = Name[1];
			Name[1] = 0;
		}

		sscanf(Name, "%i", &Sample);
		Sample++;

		if (PrevSample == 0) {
			int Pitch = pPitchBox->GetCurSel();
			pInst->SetSamplePitch(m_iOctave, m_iSelectedKey, Pitch);
		}
	}

//	m_iSelectedSample = Sample;

	pInst->SetSample(m_iOctave, m_iSelectedKey, Sample);

	UpdateKey(m_iSelectedKey);
}

void CInstrumentDPCM::OnBnClickedSave()
{
	//CListCtrl	*List = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_SAMPLE_LIST));
	CString		Path;
	CFile		SampleFile;

	CDSample	*DSample;
	char		Text[256];
	int			Index;

	m_pSampleListCtrl = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_SAMPLE_LIST));

	Index = m_pSampleListCtrl->GetSelectionMark();

	if (Index == -1)
		return;

	m_pSampleListCtrl->GetItemText(Index, 0, Text, 256);
	sscanf(Text, "%i", &Index);
	
	DSample = pDoc->GetDSample(Index);

	if (DSample->SampleSize == 0)
		return;

	CFileDialog SaveFileDialog(FALSE, "dmc", (LPCSTR)DSample->Name, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Delta modulation samples (*.dmc)|*.dmc|All files|*.*||");
	
	SaveFileDialog.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_DMC);

	if (SaveFileDialog.DoModal() == IDCANCEL)
		return;

	theApp.m_pSettings->SetPath(SaveFileDialog.GetPathName(), PATH_DMC);

	Path = SaveFileDialog.GetPathName();

	if (!SampleFile.Open(Path, CFile::modeWrite | CFile::modeCreate)) {
		MessageBox("Could not open file!");
		return;
	}

	SampleFile.Write(DSample->SampleData, DSample->SampleSize);
	SampleFile.Close();
}

void CInstrumentDPCM::OnBnClickedLoop()
{
	CInstrument2A03 *pInst = (CInstrument2A03*)pDoc->GetInstrument(pView->GetInstrument());
	int Pitch;

	Pitch = pInst->GetSamplePitch(m_iOctave, m_iSelectedKey) & 0x0F;

	if (IsDlgButtonChecked(IDC_LOOP))
		Pitch |= 0x80;

	pInst->SetSamplePitch(m_iOctave, m_iSelectedKey, Pitch);
}

void CInstrumentDPCM::SetCurrentInstrument(int Index)
{
	BuildKeyList();
}

BOOL CInstrumentDPCM::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message) {
		case WM_KEYDOWN:
			switch (pMsg->wParam) {
				case 27:	// Esc
					pMsg->wParam = 0;
					return TRUE;
				default:
					static_cast<CFamiTrackerView*>(theApp.GetView())->PreviewNote((unsigned char)pMsg->wParam);
			}
			break;
		case WM_KEYUP:
			static_cast<CFamiTrackerView*>(theApp.GetView())->PreviewRelease((unsigned char)pMsg->wParam);
			return TRUE;
	}

	return CDialog::PreTranslateMessage(pMsg);
}


BOOL CInstrumentDPCM::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

HBRUSH CInstrumentDPCM::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	if (!theApp.IsThemeActive())
		return hbr;

	if (nCtlColor == CTLCOLOR_STATIC) {
		pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)GetStockObject(WHITE_BRUSH);
	}

	return hbr;
}

void CInstrumentDPCM::OnBnClickedAdd()
{
	// Add sample to key list	
	CComboBox *pPitchBox = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_PITCH));
	CInstrument2A03 *pInst = (CInstrument2A03*)pDoc->GetInstrument(pView->GetInstrument());

	int Pitch = pPitchBox->GetCurSel();

	if (pDoc->GetSampleSize(m_iSelectedSample) > 0) {
		pInst->SetSample(m_iOctave, m_iSelectedKey, m_iSelectedSample + 1);
		pInst->SetSamplePitch(m_iOctave, m_iSelectedKey, Pitch);
		UpdateKey(m_iSelectedKey);
	}

	m_pSampleListCtrl = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_SAMPLE_LIST));
	m_pTableListCtrl = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));

	if (m_iSelectedKey < 12 && m_iSelectedSample < MAX_DSAMPLES) {
		m_pSampleListCtrl->SetItemState(m_iSelectedSample, 0, LVIS_FOCUSED | LVIS_SELECTED);
		m_pTableListCtrl->SetItemState(m_iSelectedKey, 0, LVIS_FOCUSED | LVIS_SELECTED);
		m_iSelectedSample++;
		m_iSelectedKey++;
		m_pSampleListCtrl->SetSelectionMark(m_iSelectedSample);
		m_pTableListCtrl->SetSelectionMark(m_iSelectedKey);
		m_pSampleListCtrl->SetItemState(m_iSelectedSample, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		m_pTableListCtrl->SetItemState(m_iSelectedKey, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
}

void CInstrumentDPCM::OnBnClickedRemove()
{
	// Remove sample from key list
	CInstrument2A03 *pInst = (CInstrument2A03*)pDoc->GetInstrument(pView->GetInstrument());
	pInst->SetSample(m_iOctave, m_iSelectedKey, 0);
	UpdateKey(m_iSelectedKey);

	if (m_iSelectedKey > 0) {
		m_pTableListCtrl->SetItemState(m_iSelectedKey, 0, LVIS_FOCUSED | LVIS_SELECTED);
		m_iSelectedKey--;
		m_pTableListCtrl->SetSelectionMark(m_iSelectedKey);
		m_pTableListCtrl->SetItemState(m_iSelectedKey, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
}
