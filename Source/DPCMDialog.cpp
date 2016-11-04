// DPCMDialog.cpp : implementation file
//

#include "stdafx.h"
#include <mmsystem.h>
#include "FamiTracker.h"
#include "DPCMDialog.h"
#include ".\dpcmdialog.h"
#include "FamiTrackerDoc.h"
#include "PCMImport.h"

// CDPCMDialog dialog

IMPLEMENT_DYNAMIC(CDPCMDialog, CDialog)
CDPCMDialog::CDPCMDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CDPCMDialog::IDD, pParent)
{
}

CFamiTrackerDoc	*pDocument;

CDPCMDialog::~CDPCMDialog()
{
}

void CDPCMDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDPCMDialog, CDialog)
	ON_BN_CLICKED(IDC_CLOSE, OnBnClickedClose)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedClose)
	ON_BN_CLICKED(IDC_DPCM_ADD, OnBnClickedAdd)
	ON_BN_CLICKED(ID_REMOVE, OnBnClickedRemove)
	ON_BN_CLICKED(IDC_IMPORT, OnBnClickedImport)
	ON_BN_CLICKED(IDC_SAVE, OnBnClickedSave)
END_MESSAGE_MAP()

void CDPCMDialog::SetDocument(CFamiTrackerDoc *pDoc)
{
	pDocument = pDoc;
}

void CDPCMDialog::FillList()
{
	CListCtrl	*List;
	stDSample	*DSample;
	char		Text[256];
	int			Added = 0;
	int			Size = 0;

	List = (CListCtrl*)GetDlgItem(IDC_LIST);

	List->DeleteAllItems();

	for (int i = 0; i < MAX_DSAMPLES; i++) {
		DSample = pDocument->GetDSample(i);
		if (DSample->SampleSize != 0) {
			sprintf(Text, "%i", i);
			List->InsertItem(Added, Text);
			List->SetItemText(Added, 1, DSample->Name);
			Size += DSample->SampleSize;
			Added++;
		}
	}

	sprintf(Text, "%ikB / 16kB", Size / 1024);
	SetDlgItemText(IDC_SAMPLES_SIZE, Text);
}

// CDPCMDialog message handlers

void CDPCMDialog::OnBnClickedClose()
{
	// TODO: Add your control notification handler code here

	EndDialog(0);
}

BOOL CDPCMDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here

	CListCtrl *List;

	List = (CListCtrl*)GetDlgItem(IDC_LIST);

	List->InsertColumn(0, "#", LVCFMT_LEFT, 25);
	List->InsertColumn(1, "File", LVCFMT_LEFT, 103);

	List->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	FillList();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDPCMDialog::OnBnClickedAdd()
{
	// TODO: Add your control notification handler code here

	CString		Path, FileName;
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

	stDSample *NewSample = pDocument->GetFreeDSample();
	
	int Size = 0;

	for (int i = 0; i < MAX_DSAMPLES; i++) {
		Size += pDocument->GetDSample(i)->SampleSize;
	}
	
	if (NewSample != NULL) {
		if ((Size + (int)SampleFile.GetLength()) > 0x4000) {
			MessageBox("Couldn't load sample, out of sample memory (max 16 kB is avaliable)\n");
		}
		else {
			strcpy(NewSample->Name, FileName);
			NewSample->SampleSize = (int)SampleFile.GetLength();
			NewSample->SampleData = new char[NewSample->SampleSize];
			SampleFile.Read(NewSample->SampleData, NewSample->SampleSize);
		}
	}
	else {
		MessageBox("Couldn't load sample, add more instruments\n");
	}
	
	SampleFile.Close();

	FillList();
}

void CDPCMDialog::OnBnClickedRemove()
{
	// TODO: Add your control notification handler code here

	CListCtrl *List;
	char Text[256];

	List = (CListCtrl*)GetDlgItem(IDC_LIST);

	int Index = List->GetSelectionMark();

	if (Index == -1)
		return;

	List->GetItemText(Index, 0, Text, 256);
	sscanf(Text, "%i", &Index);
	
	pDocument->RemoveDSample(Index);

	FillList();
}

void CDPCMDialog::OnCancel()
{
	EndDialog(0);
	CDialog::OnCancel();
}

void CDPCMDialog::OnBnClickedImport()
{
	CPCMImport		ImportDialog;
	stImportedPCM	*Imported;

	Imported = ImportDialog.ShowDialog();

	if (Imported->Size == 0)
		return;

	stDSample *NewSample = pDocument->GetFreeDSample();
	
	int Size = 0;

	for (int i = 0; i < MAX_DSAMPLES; i++) {
		Size += pDocument->GetDSample(i)->SampleSize;
	}
	
	if (NewSample != NULL) {
		if ((Size + Imported->Size) > 0x4000) {
			MessageBox("Couldn't load sample, out of sample memory (max 16 kB is avaliable)\n");
		}
		else {
			strcpy(NewSample->Name, Imported->Name);
			NewSample->SampleSize = Imported->Size;
			NewSample->SampleData = new char[NewSample->SampleSize];
			memcpy(NewSample->SampleData, Imported->Data, NewSample->SampleSize);
		}
	}
	else {
		MessageBox("Couldn't load sample, add more instruments\n");
	}

	delete [] Imported->Data;

	FillList();
}

void CDPCMDialog::OnBnClickedSave()
{
	CListCtrl	*List = (CListCtrl*)GetDlgItem(IDC_LIST);
	CString		Path;
	CFile		SampleFile;

	stDSample	*DSample;
	char		Text[256];
	int			Index;

	Index = List->GetSelectionMark();

	if (Index == -1)
		return;

	List->GetItemText(Index, 0, Text, 256);
	sscanf(Text, "%i", &Index);
	
	DSample = pDocument->GetDSample(Index);

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
