// Source\ModulePropertiesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "ModulePropertiesDlg.h"
#include "ChannelMap.h"

LPCTSTR TRACK_FORMAT = _T("#%02i %s");

// CModulePropertiesDlg dialog

//
// Contains song list editor and expansion chip selector
//

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

	// Get active document
	CFrameWnd *pFrameWnd = static_cast<CFrameWnd*>(GetParent());
	m_pDocument = static_cast<CFamiTrackerDoc*>(pFrameWnd->GetActiveDocument());

	CComboBox *ChipBox = (CComboBox*)GetDlgItem(IDC_EXPANSION);
	m_pSongList = (CListCtrl*)GetDlgItem(IDC_SONGLIST);

	m_pSongList->InsertColumn(0, _T("Songs"), 0, 150);
	m_pSongList->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	CString Text;
	
	// Song editor
	int Songs = m_pDocument->GetTrackCount();
	for (int i = 0; i < Songs; i++) {
		Text.Format(TRACK_FORMAT, i + 1, m_pDocument->GetTrackTitle(i));	// start counting songs from 1
		m_pSongList->InsertItem(i, Text);
	}

	// Select first song when dialog is displayed
	SelectSong(0);

	// Expansion chip
	int ExpChip = m_pDocument->GetExpansionChip();

	for (int i = 0; i < theApp.GetChannelMap()->GetChipCount(); i++)
		ChipBox->AddString(theApp.GetChannelMap()->GetChipName(i));

	ChipBox->SetCurSel(theApp.GetChannelMap()->GetChipIndex(ExpChip));

	CComboBox *pVibratoBox = (CComboBox*)GetDlgItem(IDC_VIBRATO);
	pVibratoBox->SetCurSel((m_pDocument->GetVibratoStyle() == VIBRATO_NEW) ? 0 : 1);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CModulePropertiesDlg::OnBnClickedOk()
{
	CComboBox *ExpansionChipBox = (CComboBox*)GetDlgItem(IDC_EXPANSION);
	unsigned int iExpansionChip = theApp.GetChannelMap()->GetChipIdent(ExpansionChipBox->GetCurSel());
	m_pDocument->SelectExpansionChip(iExpansionChip);
	CComboBox *pVibratoBox = (CComboBox*)GetDlgItem(IDC_VIBRATO);
	m_pDocument->SetVibratoStyle((pVibratoBox->GetCurSel() == 0) ? VIBRATO_NEW : VIBRATO_OLD);

	if (m_pDocument->GetSelectedTrack() != m_iSelectedSong)
		m_pDocument->SelectTrack(m_iSelectedSong);

	OnOK();
}

void CModulePropertiesDlg::OnBnClickedSongAdd()
{
	CString TrackTitle;
	// Try to add a track
	if (!m_pDocument->AddTrack())
		return;
	int NewTrack = m_pDocument->GetTrackCount() - 1;
	TrackTitle.Format(TRACK_FORMAT, NewTrack, m_pDocument->GetTrackTitle(NewTrack));
	m_pSongList->InsertItem(NewTrack, TrackTitle);
	SelectSong(NewTrack);
}

void CModulePropertiesDlg::OnBnClickedSongRemove()
{
	CString TrackTitle;

	if (m_iSelectedSong == -1 || m_pDocument->GetTrackCount() == 1)
		return;

	// Display warning first
	if (AfxMessageBox(IDS_SONG_DELETE, MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL)
		return;

	m_pSongList->DeleteItem(m_iSelectedSong);
	m_pDocument->RemoveTrack(m_iSelectedSong);

	// Redraw track list
	for (unsigned int i = 0; i < m_pDocument->GetTrackCount(); ++i) {
		TrackTitle.Format(_T("#%02i %s"), i + 1, m_pDocument->GetTrackTitle(i));
		m_pSongList->SetItemText(i, 0, TrackTitle);
	}

	SelectSong(m_iSelectedSong - 1);
}

void CModulePropertiesDlg::OnBnClickedSongUp()
{
	CString Text;

	if (m_iSelectedSong == 0)
		return;

	Text.Format(TRACK_FORMAT, m_iSelectedSong + 1, m_pDocument->GetTrackTitle(m_iSelectedSong - 1));
	m_pSongList->SetItemText(m_iSelectedSong, 0, Text);
	Text.Format(TRACK_FORMAT, m_iSelectedSong, m_pDocument->GetTrackTitle(m_iSelectedSong));
	m_pSongList->SetItemText(m_iSelectedSong - 1, 0, Text);
	m_pSongList->SetItemState(m_iSelectedSong - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

	m_pDocument->MoveTrackUp(m_iSelectedSong);

	m_iSelectedSong--;
}

void CModulePropertiesDlg::OnBnClickedSongDown()
{
	CString Text;

	if ((m_pDocument->GetTrackCount() - 1) == m_iSelectedSong)
		return;

	Text.Format(TRACK_FORMAT, m_iSelectedSong + 1, m_pDocument->GetTrackTitle(m_iSelectedSong + 1));
	m_pSongList->SetItemText(m_iSelectedSong, 0, Text);
	Text.Format(TRACK_FORMAT, m_iSelectedSong + 2, m_pDocument->GetTrackTitle(m_iSelectedSong));
	m_pSongList->SetItemText(m_iSelectedSong + 1, 0, Text);
	m_pSongList->SetItemState(m_iSelectedSong + 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

	m_pDocument->MoveTrackDown(m_iSelectedSong);

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
	if (Song == -1)
		return;

	m_iSelectedSong = Song;
	m_pSongList->SetItemState(Song, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_pSongList->EnsureVisible(Song, FALSE);
	CEdit *pName = (CEdit*)GetDlgItem(IDC_SONGNAME);
	pName->SetWindowText(m_pDocument->GetTrackTitle(Song));

	if (m_pDocument->GetTrackCount() == 1) {
		GetDlgItem(IDC_SONG_REMOVE)->EnableWindow(FALSE);
	}
	else {
		GetDlgItem(IDC_SONG_REMOVE)->EnableWindow(TRUE);
	}

	if (m_pDocument->GetTrackCount() == MAX_TRACKS) {
		GetDlgItem(IDC_SONG_ADD)->EnableWindow(FALSE);
	}
	else {
		GetDlgItem(IDC_SONG_ADD)->EnableWindow(TRUE);
	}
}

void CModulePropertiesDlg::OnBnClickedSongImport()
{
	CString TrackTitle;

	CFileDialog OpenFileDlg(TRUE, _T("ftm"), 0, OFN_HIDEREADONLY, _T("FamiTracker files (*.ftm)|*.ftm|All files (*.*)|*.*||"), theApp.GetMainWnd(), 0);

	if (OpenFileDlg.DoModal() == IDCANCEL)
		return;

	bool bIncludeInstrument = AfxMessageBox(_T("Do you want to include instruments?"), MB_YESNO | MB_ICONQUESTION) == IDYES;

	if (m_pDocument->ImportFile(OpenFileDlg.GetPathName(), bIncludeInstrument)) {
		int Tracks = m_pDocument->GetTrackCount();
		TrackTitle.Format(TRACK_FORMAT, Tracks, m_pDocument->GetTrackTitle(Tracks));
		m_pSongList->InsertItem(Tracks, TrackTitle);
		SelectSong(Tracks);
		m_pDocument->SelectTrack(Tracks);
	}
	else {
		AfxMessageBox(_T("Import failed"), MB_ICONERROR);
	}
}
