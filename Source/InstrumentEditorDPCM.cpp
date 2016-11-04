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
#include "FamiTrackerView.h"
#include "InstrumentEditPanel.h"
#include "InstrumentEditorDPCM.h"
#include "SampleEditorDlg.h"
#include "PCMImport.h"
#include "Settings.h"
#include "SoundGen.h"

const char *CInstrumentEditorDPCM::KEY_NAMES[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

// Derive a new class from CFileDialog with implemented preview of DMC files

class CDMCFileSoundDialog : public CFileDialog
{
public:
	CDMCFileSoundDialog(BOOL bOpenFileDialog, LPCTSTR lpszDefExt = NULL, LPCTSTR lpszFileName = NULL, DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, LPCTSTR lpszFilter = NULL, CWnd* pParentWnd = NULL, DWORD dwSize = 0);
	~CDMCFileSoundDialog();

protected:
	virtual void OnFileNameChange();
	CString m_strLastFile;
};

//	CFileSoundDialog

CDMCFileSoundDialog::CDMCFileSoundDialog(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName, DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd, DWORD dwSize) 
	: CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, dwSize)
{
}

CDMCFileSoundDialog::~CDMCFileSoundDialog()
{
	// Stop any possible playing sound
	//PlaySound(NULL, NULL, SND_NODEFAULT | SND_SYNC);
}

void CDMCFileSoundDialog::OnFileNameChange()
{
	// Preview DMC file

	if (!GetFileExt().CompareNoCase("dmc") && theApp.m_pSettings->General.bWavePreview) {
		DWORD dwAttrib = GetFileAttributes(GetPathName());
		if (!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY) && GetPathName() != m_strLastFile) {
			CFile file(GetPathName(), CFile::modeRead);
			ULONGLONG size = file.GetLength();
			if (size > 4081)
				size = 4081;
			CDSample *pSample = new CDSample();
			pSample->SampleData = new char[(int)size];
			pSample->SampleSize = (int)size;
			memset(pSample->Name, 0, 256);
			file.Read(pSample->SampleData, (int)size);
			theApp.GetSoundGenerator()->PreviewSample(pSample, 0, 15);
			file.Close();
			m_strLastFile = GetPathName();
		}
	}
	
	CFileDialog::OnFileNameChange();
}

// CInstrumentDPCM dialog

IMPLEMENT_DYNAMIC(CInstrumentEditorDPCM, CInstrumentEditPanel)
CInstrumentEditorDPCM::CInstrumentEditorDPCM(CWnd* pParent) : CInstrumentEditPanel(CInstrumentEditorDPCM::IDD, pParent)
{
}

CInstrumentEditorDPCM::~CInstrumentEditorDPCM()
{
}

void CInstrumentEditorDPCM::DoDataExchange(CDataExchange* pDX)
{
	CInstrumentEditPanel::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CInstrumentEditorDPCM, CInstrumentEditPanel)
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
	ON_BN_CLICKED(IDC_ADD, OnBnClickedAdd)
	ON_BN_CLICKED(IDC_REMOVE, OnBnClickedRemove)
	ON_EN_CHANGE(IDC_LOOP_POINT, &CInstrumentEditorDPCM::OnEnChangeLoopPoint)
	ON_BN_CLICKED(IDC_EDIT, &CInstrumentEditorDPCM::OnBnClickedEdit)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_SAMPLE_LIST, &CInstrumentEditorDPCM::OnLvnItemchangedSampleList)
	ON_NOTIFY(NM_DBLCLK, IDC_SAMPLE_LIST, &CInstrumentEditorDPCM::OnNMDblclkSampleList)
	ON_BN_CLICKED(IDC_PREVIEW, &CInstrumentEditorDPCM::OnBnClickedPreview)
	ON_NOTIFY(NM_RCLICK, IDC_SAMPLE_LIST, &CInstrumentEditorDPCM::OnNMRClickSampleList)
	ON_NOTIFY(NM_RCLICK, IDC_TABLE, &CInstrumentEditorDPCM::OnNMRClickTable)
	ON_NOTIFY(NM_DBLCLK, IDC_TABLE, &CInstrumentEditorDPCM::OnNMDblclkTable)
END_MESSAGE_MAP()

// CInstrumentDPCM message handlers

BOOL CInstrumentEditorDPCM::OnInitDialog()
{
	CInstrumentEditPanel::OnInitDialog();

	CComboBox *pPitch, *pOctave;
	CString Text;
	int i;

	m_iOctave = 3;
	m_iSelectedKey = 0;

	pPitch	= reinterpret_cast<CComboBox*>(GetDlgItem(IDC_PITCH));
	pOctave = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_OCTAVE));

	m_pTableListCtrl = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));
	m_pTableListCtrl->DeleteAllItems();
	m_pTableListCtrl->InsertColumn(0, "Key", LVCFMT_LEFT, 30);
	m_pTableListCtrl->InsertColumn(1, "Pitch", LVCFMT_LEFT, 35);
	m_pTableListCtrl->InsertColumn(2, "Sample", LVCFMT_LEFT, 90);
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

	for (i = 0; i < 12; i++)
		m_pTableListCtrl->InsertItem(i, KEY_NAMES[i], 0);

	BuildSampleList();
	m_iSelectedSample = 0;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditorDPCM::BuildKeyList()
{
	for (int i = 0; i < 12; i++) {
		UpdateKey(i);
	}
}

void CInstrumentEditorDPCM::UpdateKey(int Index)
{
	int Pitch, Item;
	char Name[256];

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();

	m_pTableListCtrl = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));

	if (m_pInstrument->GetSample(m_iOctave, Index) > 0) {
		Item = m_pInstrument->GetSample(m_iOctave, Index) - 1;
		Pitch = m_pInstrument->GetSamplePitch(m_iOctave, Index);

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

void CInstrumentEditorDPCM::BuildSampleList()
{
	CComboBox		*m_pSampleBox;
	CDSample		*pDSample;
	unsigned int	Size, i, Index = 0;
	CString			Text;

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();

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
// Todo: I think I was wrong
#define ADJUST_FOR_STORAGE(x) (x)

void CInstrumentEditorDPCM::LoadSample(char *FilePath, char *FileName)
{
	CFile SampleFile;

	if (!SampleFile.Open(FilePath, CFile::modeRead)) {
		MessageBox("Could not open file!");
		return;
	}

	CDSample *NewSample = new CDSample;

	if (NewSample != NULL) {
		sprintf(NewSample->Name, "%s", FileName);
		NewSample->SampleSize = (int)SampleFile.GetLength();
		NewSample->SampleData = new char[NewSample->SampleSize];
		SampleFile.Read(NewSample->SampleData, NewSample->SampleSize);
		InsertSample(NewSample);
	}
	
	SampleFile.Close();
	BuildSampleList();
}

void CInstrumentEditorDPCM::InsertSample(CDSample *pNewSample)
{	
	int Size = 0;

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();

	CDSample *pFreeSample = pDoc->GetFreeDSample();

	for (int i = 0; i < MAX_DSAMPLES; i++)
		Size += ADJUST_FOR_STORAGE(pDoc->GetDSample(i)->SampleSize);
	
	if ((Size + pNewSample->SampleSize) > 0x4000) {
		theApp.DisplayError(IDS_OUT_OF_SAMPLEMEM);
	}
	else {
		strcpy(pFreeSample->Name, pNewSample->Name);
		pFreeSample->SampleSize = pNewSample->SampleSize;
		pFreeSample->SampleData = new char[pNewSample->SampleSize];
		memcpy(pFreeSample->SampleData, pNewSample->SampleData, pNewSample->SampleSize);
	}

	delete [] pNewSample->SampleData;
	// pNewSample must be allocated dynamically
	delete pNewSample;
}

void CInstrumentEditorDPCM::OnBnClickedLoad()
{
//	CFileDialog OpenFileDialog(TRUE, 0, 0, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER, "Delta modulated samples (*.dmc)|*.dmc|All files|*.*||");
	CDMCFileSoundDialog OpenFileDialog(TRUE, 0, 0, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER, "Delta modulated samples (*.dmc)|*.dmc|All files|*.*||");

	OpenFileDialog.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_DMC);

	if (OpenFileDialog.DoModal() == IDCANCEL)
		return;

	theApp.m_pSettings->SetPath(OpenFileDialog.GetPathName(), PATH_DMC);

	if (OpenFileDialog.GetFileName().GetLength() == 0) {
		// Multiple files
		POSITION Pos = OpenFileDialog.GetStartPosition();
		while (Pos) {
			CString Path = OpenFileDialog.GetNextPathName(Pos);
			CString FileName = Path.Right(Path.GetLength() - Path.ReverseFind('\\') - 1);
			LoadSample(Path.GetBuffer(), FileName.GetBuffer());
		}
	}
	else {
		// Single file
		LoadSample(OpenFileDialog.GetPathName().GetBuffer(), OpenFileDialog.GetFileName().GetBuffer());
	}
}

void CInstrumentEditorDPCM::OnBnClickedUnload()
{
	CListCtrl *pListBox = (CListCtrl*)GetDlgItem(IDC_SAMPLE_LIST);
	int nItem = -1, SelCount, Index;
	char ItemName[256];

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();

	if (m_iSelectedSample == MAX_DSAMPLES)
		return;
	
	if (!(SelCount = pListBox->GetSelectedCount()))
		return;

	for (int i = 0; i < SelCount; i++) {
		nItem = pListBox->GetNextItem(nItem, LVNI_SELECTED);
		ASSERT(nItem != -1);
		pListBox->GetItemText(nItem, 0, ItemName, 256);
		//sscanf(ItemName, "%i", &Index);
		Index = atoi(ItemName);
		pDoc->RemoveDSample(Index);
	}

//	pDoc->RemoveDSample(m_iSelectedSample);

	BuildSampleList();
}

void CInstrumentEditorDPCM::OnNMClickSampleList(NMHDR *pNMHDR, LRESULT *pResult)
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

void CInstrumentEditorDPCM::OnBnClickedImport()
{
	CPCMImport		ImportDialog;
	CDSample		*pImported;

	if (!(pImported = ImportDialog.ShowDialog()))
		return;

	InsertSample(pImported);
	BuildSampleList();
}

void CInstrumentEditorDPCM::OnCbnSelchangeOctave()
{
	CComboBox *m_OctaveBox = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_OCTAVE));
	m_iOctave = m_OctaveBox->GetCurSel();
	BuildKeyList();
}

void CInstrumentEditorDPCM::OnCbnSelchangePitch()
{
	CComboBox *m_pPitchBox	= reinterpret_cast<CComboBox*>(GetDlgItem(IDC_PITCH));
	int Pitch;

	if (m_iSelectedKey == -1)
		return;

	Pitch = m_pPitchBox->GetCurSel();

	if (IsDlgButtonChecked(IDC_LOOP))
		Pitch |= 0x80;

	m_pInstrument->SetSamplePitch(m_iOctave, m_iSelectedKey, Pitch);

	UpdateKey(m_iSelectedKey);
}

void CInstrumentEditorDPCM::OnNMClickTable(NMHDR *pNMHDR, LRESULT *pResult)
{
	CComboBox *pSampleBox, *pPitchBox;
	CString Text;
	int Sample, Pitch;

	m_pTableListCtrl	= reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));
	m_iSelectedKey		= m_pTableListCtrl->GetSelectionMark();

	Sample				= m_pInstrument->GetSample(m_iOctave, m_iSelectedKey) - 1;
	Pitch				= m_pInstrument->GetSamplePitch(m_iOctave, m_iSelectedKey);

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

void CInstrumentEditorDPCM::OnCbnSelchangeSamples()
{
	CComboBox *m_pSampleBox = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_SAMPLES));
	CComboBox *pPitchBox = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_PITCH));
	
	int Sample, PrevSample;
	char Name[256];

	PrevSample = m_pInstrument->GetSample(m_iOctave, m_iSelectedKey);

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
			m_pInstrument->SetSamplePitch(m_iOctave, m_iSelectedKey, Pitch);
		}
	}

//	m_iSelectedSample = Sample;

	m_pInstrument->SetSample(m_iOctave, m_iSelectedKey, Sample);

	UpdateKey(m_iSelectedKey);
}

CDSample *CInstrumentEditorDPCM::GetSelectedSample()
{
	CDSample *pSample;
	char	 Text[256];
	int		 Index;

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();

	m_pSampleListCtrl = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_SAMPLE_LIST));

	Index = m_pSampleListCtrl->GetSelectionMark();

	if (Index == -1)
		return NULL;

	m_pSampleListCtrl->GetItemText(Index, 0, Text, 256);
	sscanf(Text, "%i", &Index);
	
	pSample = pDoc->GetDSample(Index);

	return pSample;
}

void CInstrumentEditorDPCM::OnBnClickedSave()
{
	CString		Path;
	CFile		SampleFile;

	CDSample	*DSample;
	char		Text[256];
	int			Index;

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();

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

void CInstrumentEditorDPCM::OnBnClickedLoop()
{
	int Pitch;

	Pitch = m_pInstrument->GetSamplePitch(m_iOctave, m_iSelectedKey) & 0x0F;

	if (IsDlgButtonChecked(IDC_LOOP))
		Pitch |= 0x80;

	m_pInstrument->SetSamplePitch(m_iOctave, m_iSelectedKey, Pitch);
}

void CInstrumentEditorDPCM::SelectInstrument(int Instrument)
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();
	CInstrument2A03 *pInst = (CInstrument2A03*)pDoc->GetInstrument(Instrument);
	m_pInstrument = pInst;
	BuildKeyList();
}

BOOL CInstrumentEditorDPCM::PreTranslateMessage(MSG* pMsg)
{
	if (IsWindowVisible()) {
		switch (pMsg->message) {
			case WM_KEYDOWN:
				if (pMsg->wParam == 27)	// Esc
					break;
				// Select DPCM channel
				static_cast<CFamiTrackerView*>(theApp.GetDocumentView())->SelectChannel(4);
				static_cast<CFamiTrackerView*>(theApp.GetDocumentView())->PreviewNote((unsigned char)pMsg->wParam);
				return TRUE;
			case WM_KEYUP:
				static_cast<CFamiTrackerView*>(theApp.GetDocumentView())->PreviewRelease((unsigned char)pMsg->wParam);
				return TRUE;
		}
	}

	return CInstrumentEditPanel::PreTranslateMessage(pMsg);
}

void CInstrumentEditorDPCM::OnBnClickedAdd()
{
	// Add sample to key list	
	CComboBox *pPitchBox = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_PITCH));
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();

	int Pitch = pPitchBox->GetCurSel();

	if (pDoc->GetSampleSize(m_iSelectedSample) > 0) {
		m_pInstrument->SetSample(m_iOctave, m_iSelectedKey, m_iSelectedSample + 1);
		m_pInstrument->SetSamplePitch(m_iOctave, m_iSelectedKey, Pitch);
		UpdateKey(m_iSelectedKey);
	}

	m_pSampleListCtrl = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_SAMPLE_LIST));
	m_pTableListCtrl = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));

	if (m_iSelectedKey < 12 && m_iSelectedSample < MAX_DSAMPLES) {
		m_pSampleListCtrl->SetItemState(m_iSelectedSample, 0, LVIS_FOCUSED | LVIS_SELECTED);
		m_pTableListCtrl->SetItemState(m_iSelectedKey, 0, LVIS_FOCUSED | LVIS_SELECTED);
		if (m_iSelectedSample < m_pSampleListCtrl->GetItemCount() - 1)
			m_iSelectedSample++;
		m_iSelectedKey++;
		m_pSampleListCtrl->SetSelectionMark(m_iSelectedSample);
		m_pTableListCtrl->SetSelectionMark(m_iSelectedKey);
		m_pSampleListCtrl->SetItemState(m_iSelectedSample, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		m_pTableListCtrl->SetItemState(m_iSelectedKey, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
}

void CInstrumentEditorDPCM::OnBnClickedRemove()
{
	// Remove sample from key list
	m_pInstrument->SetSample(m_iOctave, m_iSelectedKey, 0);
	UpdateKey(m_iSelectedKey);

	if (m_iSelectedKey > 0) {
		m_pTableListCtrl->SetItemState(m_iSelectedKey, 0, LVIS_FOCUSED | LVIS_SELECTED);
		m_iSelectedKey--;
		m_pTableListCtrl->SetSelectionMark(m_iSelectedKey);
		m_pTableListCtrl->SetItemState(m_iSelectedKey, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
}


void CInstrumentEditorDPCM::OnEnChangeLoopPoint()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CInstrumentEditPanel::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here

	int Pitch = GetDlgItemInt(IDC_LOOP_POINT, 0, 0);

	m_pInstrument->SetSampleLoopOffset(m_iOctave, m_iSelectedKey, Pitch);
}

void CInstrumentEditorDPCM::OnBnClickedEdit()
{
	CDSample *pSample = GetSelectedSample();

	if (pSample == NULL)
		return;

	CSampleEditorDlg Editor(this, pSample);

	Editor.DoModal();
	
	// Update sample list
	BuildSampleList();
}

void CInstrumentEditorDPCM::OnLvnItemchangedSampleList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	// Todo: save current item
	*pResult = 0;
}

// Behaviour when double clicking the sample list
void CInstrumentEditorDPCM::OnNMDblclkSampleList(NMHDR *pNMHDR, LRESULT *pResult)
{
//	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<NMITEMACTIVATE>(pNMHDR);
//	OnBnClickedEdit();
	OnBnClickedPreview();
	*pResult = 0;
}

void CInstrumentEditorDPCM::OnBnClickedPreview()
{
	CDSample *pSample = GetSelectedSample();

	if (pSample == NULL)
		return;

	theApp.GetSoundGenerator()->PreviewSample(pSample, 0, 15);
}

void CInstrumentEditorDPCM::OnNMRClickSampleList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	CMenu *PopupMenu, PopupMenuBar;
	CPoint point;

	GetCursorPos(&point);
	PopupMenuBar.LoadMenu(IDR_SAMPLES_POPUP);
	PopupMenu = PopupMenuBar.GetSubMenu(0);
	PopupMenu->SetDefaultItem(IDC_PREVIEW);
	PopupMenu->TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);

	*pResult = 0;
}

void CInstrumentEditorDPCM::OnNMRClickTable(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	// Create a popup menu for key list with samples

	CDSample		*pDSample;
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();

	CMenu PopupMenu;
	CPoint point;

	m_pTableListCtrl = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));
	m_iSelectedKey	 = m_pTableListCtrl->GetSelectionMark();

	GetCursorPos(&point);
	PopupMenu.CreatePopupMenu();
	PopupMenu.AppendMenu(MF_STRING, 1, "(no sample)");

	// Fill menu
	for (int i = 0; i < MAX_DSAMPLES; i++) {
		pDSample = pDoc->GetDSample(i);
		if (pDSample->SampleSize > 0) {
			PopupMenu.AppendMenu(MF_STRING, i + 2, pDSample->Name);
		}
	}

	UINT Result = PopupMenu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RETURNCMD, point.x, point.y, this);

	if (Result == 1) {
		// Remove sample
		m_pInstrument->SetSample(m_iOctave, m_iSelectedKey, 0);
		UpdateKey(m_iSelectedKey);
	}
	else if (Result > 1) {
		// Add sample
		CComboBox *pPitchBox = reinterpret_cast<CComboBox*>(GetDlgItem(IDC_PITCH));
		int Pitch = pPitchBox->GetCurSel();
		m_pInstrument->SetSample(m_iOctave, m_iSelectedKey, Result - 1);
		m_pInstrument->SetSamplePitch(m_iOctave, m_iSelectedKey, Pitch);
		UpdateKey(m_iSelectedKey);
	}

	*pResult = 0;
}

void CInstrumentEditorDPCM::OnNMDblclkTable(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	// Preview sample from key table

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();

	int Sample = m_pInstrument->GetSample(m_iOctave, m_iSelectedKey);

	if (Sample == 0)
		return;

	CDSample *pSample = pDoc->GetDSample(Sample - 1);
	int Pitch = m_pInstrument->GetSamplePitch(m_iOctave, m_iSelectedKey) & 0x0F;

	m_pTableListCtrl = reinterpret_cast<CListCtrl*>(GetDlgItem(IDC_TABLE));

	if (pSample == NULL || pSample->SampleSize == 0 || m_pTableListCtrl->GetItemText(m_iSelectedKey, 2) == "(no sample)")
		return;

	theApp.GetSoundGenerator()->PreviewSample(pSample, 0, Pitch);

	*pResult = 0;
}
