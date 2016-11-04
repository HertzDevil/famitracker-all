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
#include "InstrumentDPCM.h"
#include "PCMImport.h"
#include ".\instrumentdpcm.h"

int m_iOctave, m_iSelectedKey;


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
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_TABLE, OnLvnItemchangedTable)
	ON_NOTIFY(NM_CLICK, IDC_TABLE, OnNMClickTable)
	ON_CBN_SELCHANGE(IDC_SAMPLES, OnCbnSelchangeSamples)
	ON_BN_CLICKED(IDC_SAVE, OnBnClickedSave)
END_MESSAGE_MAP()

CListCtrl *m_pTableListCtrl;
CListCtrl *m_pSampleListCtrl;

CFamiTrackerDoc		*pDoc;
CFamiTrackerView	*pView;

int m_iSelectedSample;

// CInstrumentDPCM message handlers

BOOL CInstrumentDPCM::OnInitDialog()
{
	CDialog::OnInitDialog();

	CComboBox	*pPitch, *pOctave;

	m_iOctave = 3;
	m_iSelectedKey = 0;

	pDoc	= (CFamiTrackerDoc*)theApp.pDocument;
	pView	= (CFamiTrackerView*)theApp.pView;

	pPitch	= (CComboBox*)GetDlgItem(IDC_PITCH);
	pOctave = (CComboBox*)GetDlgItem(IDC_OCTAVE);

	m_pTableListCtrl = (CListCtrl*)GetDlgItem(IDC_TABLE);
	m_pTableListCtrl->DeleteAllItems();
	m_pTableListCtrl->InsertColumn(0, "Key", LVCFMT_LEFT, 34);
	m_pTableListCtrl->InsertColumn(1, "Pitch", LVCFMT_LEFT, 38);
	m_pTableListCtrl->InsertColumn(2, "Sample", LVCFMT_LEFT, 74);
	m_pTableListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	m_pSampleListCtrl = (CListCtrl*)GetDlgItem(IDC_SAMPLE_LIST);
	m_pSampleListCtrl->DeleteAllItems();
	m_pSampleListCtrl->InsertColumn(0, "Name", LVCFMT_LEFT, 130);
	m_pSampleListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	pPitch->AddString("0");
	pPitch->AddString("1");
	pPitch->AddString("2");
	pPitch->AddString("3");
	pPitch->AddString("4");
	pPitch->AddString("5");
	pPitch->AddString("6");
	pPitch->AddString("7");
	pPitch->AddString("8");
	pPitch->AddString("9");
	pPitch->AddString("10");
	pPitch->AddString("11");
	pPitch->AddString("12");
	pPitch->AddString("13");
	pPitch->AddString("14");
	pPitch->AddString("15");
	pPitch->SetCurSel(15);

	pOctave->AddString("0");
	pOctave->AddString("1");
	pOctave->AddString("2");
	pOctave->AddString("3");
	pOctave->AddString("4");
	pOctave->AddString("5");
	pOctave->SetCurSel(3);

	m_pTableListCtrl->DeleteAllItems();

	m_pTableListCtrl->InsertItem(0, "C", 0);
	m_pTableListCtrl->InsertItem(1, "C#", 0);
	m_pTableListCtrl->InsertItem(2, "D", 0);
	m_pTableListCtrl->InsertItem(3, "D#", 0);
	m_pTableListCtrl->InsertItem(4, "E", 0);
	m_pTableListCtrl->InsertItem(5, "F", 0);
	m_pTableListCtrl->InsertItem(6, "F#", 0);
	m_pTableListCtrl->InsertItem(7, "G", 0);
	m_pTableListCtrl->InsertItem(8, "G#", 0);
	m_pTableListCtrl->InsertItem(9, "A", 0);
	m_pTableListCtrl->InsertItem(10, "A#", 0);
	m_pTableListCtrl->InsertItem(11, "B", 0);

	BuildKeyList();
	BuildSampleList();

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

	if (pDoc->Instruments[pView->m_iInstrument].Samples[m_iOctave][Index] > 0) {
		Item	= pDoc->Instruments[pView->m_iInstrument].Samples[m_iOctave][Index] - 1;
		Pitch	= pDoc->Instruments[pView->m_iInstrument].SamplePitch[m_iOctave][Index];
		pDoc->GetSampleName(Item, Name);
		m_pTableListCtrl->SetItemText(Index, 2, Name);
		sprintf(Name, "%i", Pitch);
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
	stDSample	*pDSample;
	int			Size;
	CString		Text;

	CComboBox *m_pSampleBox;

	m_pSampleBox = (CComboBox*)GetDlgItem(IDC_SAMPLES);

	m_pSampleListCtrl = (CListCtrl*)GetDlgItem(IDC_SAMPLE_LIST);
	m_pSampleListCtrl->DeleteAllItems();

	m_pSampleBox->ResetContent();

	Size = 0;

	m_pSampleBox->AddString("(no sample)");

	for (int i = 0; i < MAX_DSAMPLES; i++) {
		pDSample = pDoc->GetDSample(i);
		if (pDSample->SampleSize > 0) {
			Text.Format("%i - %s (%i)", i, pDSample->Name, pDSample->SampleSize);
			m_pSampleListCtrl->InsertItem(i, Text);
			Text.Format("%i - %s", i, pDSample->Name);
			m_pSampleBox->AddString(Text);
			Size += pDSample->SampleSize;
		}
	}

	Text.Format("Space left %i/16kb", (0x4000 - Size) / 1024);
	SetDlgItemText(IDC_SPACE, Text);
}

void CInstrumentDPCM::OnBnClickedLoad()
{
	CString		Path, FileName, Name;
	CFile		SampleFile;

	CFileDialog OpenFileDialog(TRUE, 0, 0, OFN_HIDEREADONLY, "Delta modulation samples (*.dmc)|*.dmc|All files|*.*||");

	if (OpenFileDialog.DoModal() == IDCANCEL)
		return;

	Path		= OpenFileDialog.GetPathName();
	FileName	= OpenFileDialog.GetFileName();

	if (!SampleFile.Open(Path, CFile::modeRead)) {
		MessageBox("Could not open file!");
		return;
	}

	stDSample *NewSample = pDoc->GetFreeDSample();
	
	int Size = 0;

	for (int i = 0; i < MAX_DSAMPLES; i++) {
		Size += pDoc->GetDSample(i)->SampleSize;
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

	m_pSampleListCtrl = (CListCtrl*)GetDlgItem(IDC_SAMPLE_LIST);

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

	stDSample *NewSample = pDoc->GetFreeDSample();
	
	int Size = 0;

	for (int i = 0; i < MAX_DSAMPLES; i++) {
		Size += pDoc->GetDSample(i)->SampleSize;
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
	CComboBox *m_OctaveBox;

	m_OctaveBox	= (CComboBox*)GetDlgItem(IDC_OCTAVE);
	m_iOctave	= m_OctaveBox->GetCurSel();	

	BuildKeyList();
}

void CInstrumentDPCM::OnCbnSelchangePitch()
{
	int Pitch;

	CComboBox *m_pPitchBox;

	m_pPitchBox	= (CComboBox*)GetDlgItem(IDC_PITCH);

	if (m_iSelectedKey == -1)
		return;

	Pitch = m_pPitchBox->GetCurSel();

	pDoc->Instruments[pView->m_iInstrument].SamplePitch[m_iOctave][m_iSelectedKey] = Pitch;

	UpdateKey(m_iSelectedKey);
}

void CInstrumentDPCM::OnLvnItemchangedTable(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

void CInstrumentDPCM::OnNMClickTable(NMHDR *pNMHDR, LRESULT *pResult)
{
	CComboBox *pSampleBox, *pPitchBox;
	int Sample, Pitch;

	m_pTableListCtrl	= (CListCtrl*)GetDlgItem(IDC_TABLE);
	m_iSelectedKey		= m_pTableListCtrl->GetSelectionMark();

	Sample	= pDoc->Instruments[pView->m_iInstrument].Samples[m_iOctave][m_iSelectedKey] - 1;
	Pitch	= pDoc->Instruments[pView->m_iInstrument].SamplePitch[m_iOctave][m_iSelectedKey];

	m_pTableListCtrl	= (CListCtrl*)GetDlgItem(IDC_TABLE);
	pSampleBox			= (CComboBox*)GetDlgItem(IDC_SAMPLES);
	pPitchBox			= (CComboBox*)GetDlgItem(IDC_PITCH);

	pSampleBox->SetCurSel(Sample + 1);
	
	if (Sample > 0)
		pPitchBox->SetCurSel(Pitch);

	*pResult = 0;
}

void CInstrumentDPCM::OnCbnSelchangeSamples()
{
	CComboBox *m_pSampleBox = (CComboBox*)GetDlgItem(IDC_SAMPLES);
	CComboBox *pPitchBox = (CComboBox*)GetDlgItem(IDC_PITCH);
	
	int Sample, PrevSample;
	char Name[256];

	PrevSample = pDoc->Instruments[pView->m_iInstrument].Samples[m_iOctave][m_iSelectedKey];

	Sample = m_pSampleBox->GetCurSel();

	if (Sample > 0) {
		m_pSampleBox->GetLBText(Sample, Name);
		sscanf(Name, "%i", &Sample);
		Sample++;

		if (PrevSample == 0) {
			int Pitch = pPitchBox->GetCurSel();
			pDoc->Instruments[pView->m_iInstrument].SamplePitch[m_iOctave][m_iSelectedKey] = Pitch;
		}
	}

	pDoc->Instruments[pView->m_iInstrument].Samples[m_iOctave][m_iSelectedKey] = Sample;

	UpdateKey(m_iSelectedKey);
}

void CInstrumentDPCM::OnBnClickedSave()
{
	CListCtrl	*List = (CListCtrl*)GetDlgItem(IDC_LIST);
	CString		Path;
	CFile		SampleFile;

	stDSample	*DSample;
	char		Text[256];
	int			Index;

	Index = m_pSampleListCtrl->GetSelectionMark();

	if (Index == -1)
		return;

	m_pSampleListCtrl->GetItemText(Index, 0, Text, 256);
	sscanf(Text, "%i", &Index);
	
	DSample = pDoc->GetDSample(Index);

	if (DSample->SampleSize == 0)
		return;

	CFileDialog SaveFileDialog(FALSE, "dmc", (LPCSTR)DSample->Name, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Delta modulation samples (*.dmc)|*.dmc|All files|*.*||");
	
	if (SaveFileDialog.DoModal() == IDCANCEL)
		return;

	Path = SaveFileDialog.GetPathName();

	if (!SampleFile.Open(Path, CFile::modeWrite | CFile::modeCreate)) {
		MessageBox("Could not open file!");
		return;
	}

	SampleFile.Write(DSample->SampleData, DSample->SampleSize);
	SampleFile.Close();
}
