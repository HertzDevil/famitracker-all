// Source\ModulePropertiesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FamiTracker.h"
#include "Include\ModulePropertiesDlg.h"

const char *CHIP_NAMES[] = {"Internal only (2A03/2A07)",
							"Konami VRC6",
//							"Konami VRC7",
//							"Nintendo FDS sound",
							"Nintendo MMC5",
//							"Namco N106",
//							"Sunsoft 5B",
};

const int CHIP_COUNT = sizeof(*CHIP_NAMES) - 1;// 7;

const char TRACK_FORMAT[] = "#%02i %s";

// CModulePropertiesDlg dialog

IMPLEMENT_DYNAMIC(CModulePropertiesDlg, CDialog)
CModulePropertiesDlg::CModulePropertiesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CModulePropertiesDlg::IDD, pParent)
{
}

CModulePropertiesDlg::~CModulePropertiesDlg()
{
}

void CModulePropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CModulePropertiesDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_SONG_ADD, &CModulePropertiesDlg::OnBnClickedSongAdd)
	ON_BN_CLICKED(IDC_SONG_REMOVE, &CModulePropertiesDlg::OnBnClickedSongRemove)
	ON_BN_CLICKED(IDC_SONG_UP, &CModulePropertiesDlg::OnBnClickedSongUp)
	ON_BN_CLICKED(IDC_SONG_DOWN, &CModulePropertiesDlg::OnBnClickedSongDown)
	ON_EN_CHANGE(IDC_SONGNAME, &CModulePropertiesDlg::OnEnChangeSongname)
	ON_NOTIFY(NM_CLICK, IDC_SONGLIST, &CModulePropertiesDlg::OnClickSongList)
	ON_BN_CLICKED(IDC_SONG_IMPORT, &CModulePropertiesDlg::OnBnClickedSongImport)
END_MESSAGE_MAP()


// CModulePropertiesDlg message handlers

BOOL CModulePropertiesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetDocument();
	CComboBox *ChipBox = (CComboBox*)GetDlgItem(IDC_EXPANSION);
	m_pSongList = (CListCtrl*)GetDlgItem(IDC_SONGLIST);

	m_pSongList->InsertColumn(0, "Songs", 0, 160);
	m_pSongList->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	CString Text;
	int ExpChip, Songs;

	m_pDocument = (CFamiTrackerDoc*)theApp.GetDocument();
	
	// Song editor
	Songs = pDoc->GetTrackCount();
	for (int i = 0; i < (Songs + 1); i++) {
		Text.Format(TRACK_FORMAT, i + 1, pDoc->GetTrackTitle(i));
		m_pSongList->InsertItem(i, Text);
	}

	// Select first song when dialog is displayed
	SelectSong(0);

	// Expansion chip
	ExpChip = ((CFamiTrackerDoc*)theApp.GetDocument())->GetExpansionChip();

	for (int i = 0; i < CHIP_COUNT; i++)
		ChipBox->AddString(CHIP_NAMES[i]);

	switch (ExpChip) {
		case SNDCHIP_NONE: ChipBox->SetCurSel(0); break;
		case SNDCHIP_VRC6: ChipBox->SetCurSel(1); break;
//		case SNDCHIP_VRC7: ChipBox->SetCurSel(2); break;
//		case SNDCHIP_FDS: ChipBox->SetCurSel(3); break;
		case SNDCHIP_MMC5: ChipBox->SetCurSel(2); break;
//		case SNDCHIP_N106: ChipBox->SetCurSel(5); break;
//		case SNDCHIP_5B: ChipBox->SetCurSel(6); break;
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CModulePropertiesDlg::OnBnClickedOk()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetDocument();
//	CSliderCtrl *SubTuneSlider = (CSliderCtrl*)GetDlgItem(IDC_SUBTUNE);
	CComboBox	*ExpansionChipBox = (CComboBox*)GetDlgItem(IDC_EXPANSION);

	unsigned int iExpansionChip;// = ExpansionChipBox->GetCurSel();

//	unsigned int iExpansionChip = ExpansionChipBox->GetCurSel();
//	if (iExpansionChip > 0)
//		iExpansionChip = 1 << (iExpansionChip - 1);

//	unsigned int iNewCount = SubTuneSlider->GetPos();
	switch (ExpansionChipBox->GetCurSel()) {
		case 0:
			iExpansionChip = SNDCHIP_NONE;
			break;
		case 1:
			iExpansionChip = SNDCHIP_VRC6;
			break;
		case 2:
			iExpansionChip = SNDCHIP_MMC5;
			break;
	}

//	pDoc->SetTracks(iNewCount);

	pDoc->SelectExpansionChip(iExpansionChip);

	OnOK();
}

void CModulePropertiesDlg::OnBnClickedSongAdd()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetDocument();
	CString TrackTitle;
	if (!pDoc->AddTrack())
		return;
	int Tracks = pDoc->GetTrackCount();
	TrackTitle.Format(TRACK_FORMAT, Tracks + 1, pDoc->GetTrackTitle(Tracks));
	m_pSongList->InsertItem(Tracks, TrackTitle);
//	m_pSongList->SetItemState(Tracks, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	SelectSong(Tracks);

	if (pDoc->GetTrackCount() == MAX_TRACKS) {
	}

	//m_pSongList->EnsureVisible(Tracks, FALSE);
}

void CModulePropertiesDlg::OnBnClickedSongRemove()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetDocument();
	CString TrackTitle;
	CString Msg;

	if (m_iSelectedSong == -1 || pDoc->GetTrackCount() == 0)
		return;

//	LoadString(theApp.m_hInstance, IDS_SONG_DELETE, buf, 256);
	Msg.LoadStringA(theApp.m_hInstance, IDS_SONG_DELETE);

	if (MessageBox(Msg, "Warning", MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL)
		return;

	m_pSongList->DeleteItem(m_iSelectedSong);

	pDoc->RemoveTrack(m_iSelectedSong);

	for (unsigned int i = 0; i <= pDoc->GetTrackCount(); i++) {
		TrackTitle.Format("#%02i %s", i + 1, pDoc->GetTrackTitle(i));
		m_pSongList->SetItemText(i, 0, TrackTitle);
	}

	//m_iSelectedSong--;
	//m_pSongList->SetItemState(m_iSelectedSong, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	SelectSong(m_iSelectedSong - 1);
}

void CModulePropertiesDlg::OnBnClickedSongUp()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetDocument();
	CString Text;

	if (m_iSelectedSong == 0)
		return;

	Text.Format(TRACK_FORMAT, m_iSelectedSong + 1, pDoc->GetTrackTitle(m_iSelectedSong - 1));
	m_pSongList->SetItemText(m_iSelectedSong, 0, Text);
	Text.Format(TRACK_FORMAT, m_iSelectedSong, pDoc->GetTrackTitle(m_iSelectedSong));
	m_pSongList->SetItemText(m_iSelectedSong - 1, 0, Text);
	m_pSongList->SetItemState(m_iSelectedSong - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

	pDoc->MoveTrackUp(m_iSelectedSong);

	m_iSelectedSong--;
}

void CModulePropertiesDlg::OnBnClickedSongDown()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetDocument();
	CString Text;

	if (pDoc->GetTrackCount() == m_iSelectedSong)
		return;

	Text.Format(TRACK_FORMAT, m_iSelectedSong + 1, pDoc->GetTrackTitle(m_iSelectedSong + 1));
	m_pSongList->SetItemText(m_iSelectedSong, 0, Text);
	Text.Format(TRACK_FORMAT, m_iSelectedSong + 2, pDoc->GetTrackTitle(m_iSelectedSong));
	m_pSongList->SetItemText(m_iSelectedSong + 1, 0, Text);
	m_pSongList->SetItemState(m_iSelectedSong + 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

	pDoc->MoveTrackDown(m_iSelectedSong);

	m_iSelectedSong++;
}

void CModulePropertiesDlg::OnEnChangeSongname()
{
	CString Text, Title;
	CEdit *pName = (CEdit*)GetDlgItem(IDC_SONGNAME);

	if (m_iSelectedSong == -1)
		return;

	pName->GetWindowText(Text);

	Title.Format(TRACK_FORMAT, m_iSelectedSong + 1, Text);

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetDocument();

	m_pSongList->SetItemText(m_iSelectedSong, 0, Title);
	m_pDocument->SetTrackTitle(m_iSelectedSong, Text.GetBuffer());
}

void CModulePropertiesDlg::OnClickSongList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int Song = m_pSongList->GetSelectionMark();
	SelectSong(Song);
	*pResult = 0;
}

void CModulePropertiesDlg::SelectSong(int Song)
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetDocument();

	if (Song == -1)
		return;

	m_iSelectedSong = Song;
	m_pSongList->SetItemState(Song, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_pSongList->EnsureVisible(Song, FALSE);
	CEdit *pName = (CEdit*)GetDlgItem(IDC_SONGNAME);
	pName->SetWindowTextA(pDoc->GetTrackTitle(Song));

	if (pDoc->GetTrackCount() == 0) {
		GetDlgItem(IDC_SONG_REMOVE)->EnableWindow(FALSE);
	}
	else {
		GetDlgItem(IDC_SONG_REMOVE)->EnableWindow(TRUE);
	}

	if (pDoc->GetTrackCount() == MAX_TRACKS) {
		GetDlgItem(IDC_SONG_ADD)->EnableWindow(FALSE);
	}
	else {
		GetDlgItem(IDC_SONG_ADD)->EnableWindow(TRUE);
	}
}

void CModulePropertiesDlg::OnBnClickedSongImport()
{
//	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetDocument();
//	pDoc->ImportFile();
}
