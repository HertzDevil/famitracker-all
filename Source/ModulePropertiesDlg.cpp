// Source\ModulePropertiesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "ModulePropertiesDlg.h"

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

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();
	CComboBox *ChipBox = (CComboBox*)GetDlgItem(IDC_EXPANSION);
	m_pSongList = (CListCtrl*)GetDlgItem(IDC_SONGLIST);

	m_pSongList->InsertColumn(0, "Songs", 0, 150);
	m_pSongList->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	CString Text;
	int ExpChip, Songs;
	
	// Song editor
	Songs = pDoc->GetTrackCount();
	for (int i = 0; i < (Songs + 1); i++) {
		Text.Format(TRACK_FORMAT, i + 1, pDoc->GetTrackTitle(i));
		m_pSongList->InsertItem(i, Text);
	}

	// Select first song when dialog is displayed
	SelectSong(0);

	// Expansion chip
	ExpChip = pDoc->GetExpansionChip();

	for (int i = 0; i < theApp.GetChipCount(); i++)
		ChipBox->AddString(theApp.GetChipName(i));

	ChipBox->SetCurSel(theApp.GetChipIndex(ExpChip));

	CComboBox *pVibratoBox = (CComboBox*)GetDlgItem(IDC_VIBRATO);
	pVibratoBox->SetCurSel((pDoc->GetVibratoStyle() == VIBRATO_NEW) ? 0 : 1);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CModulePropertiesDlg::OnBnClickedOk()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();
	CComboBox *ExpansionChipBox = (CComboBox*)GetDlgItem(IDC_EXPANSION);
	unsigned int iExpansionChip = theApp.GetChipIdent(ExpansionChipBox->GetCurSel());
	pDoc->SelectExpansionChip(iExpansionChip);
	CComboBox *pVibratoBox = (CComboBox*)GetDlgItem(IDC_VIBRATO);
	pDoc->SetVibratoStyle((pVibratoBox->GetCurSel() == 0) ? VIBRATO_NEW : VIBRATO_OLD);
	OnOK();
}

void CModulePropertiesDlg::OnBnClickedSongAdd()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();
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
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();
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
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();
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
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();
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

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();

	m_pSongList->SetItemText(m_iSelectedSong, 0, Title);
	pDoc->SetTrackTitle(m_iSelectedSong, Text.GetBuffer());
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
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();

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
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();

	CFileDialog OpenFileDlg(TRUE, "ftm", "", OFN_HIDEREADONLY, "FamiTracker files (*.ftm)|*.ftm|All files (*.*)|*.*||", theApp.GetMainWnd(), 0);

	if (OpenFileDlg.DoModal() == IDCANCEL)
		return;

	int Tracks = pDoc->GetTrackCount() + 1;

	CDocumentFile DocumentFile;

	if (!DocumentFile.Open(OpenFileDlg.GetPathName(), CFile::modeRead)) {
		theApp.DisplayError(IDS_FILE_OPEN_ERROR);
		return ;
	}

	if (!DocumentFile.CheckValidity()) {
		theApp.DisplayError(IDS_FILE_VALID_ERROR);
		DocumentFile.Close();
		return ;
	}

	const char *FILE_BLOCK_PATTERNS		= "PATTERNS";
	const char *FILE_BLOCK_PARAMS		= "PARAMS";
	const char *FILE_BLOCK_FRAMES		= "FRAMES";
	const char *FILE_BLOCK_HEADER		= "HEADER";

	int TrackCount = 0;
	int ImportChannels = 0;
	bool FileFinished = false;
	char *BlockID;

	// Read all blocks
	while (!DocumentFile.Finished() && !FileFinished) {
		DocumentFile.ReadBlock();
		BlockID = DocumentFile.GetBlockHeaderID();
		unsigned int Version = DocumentFile.GetBlockVersion();

		if (!strcmp(BlockID, FILE_BLOCK_PARAMS)) {				
			DocumentFile.GetBlockChar();
			ImportChannels	= DocumentFile.GetBlockInt();
			DocumentFile.GetBlockInt();
			DocumentFile.GetBlockInt();
			if (DocumentFile.GetBlockVersion() > 2)
				DocumentFile.GetBlockInt();
		}
		else if (!strcmp(BlockID, FILE_BLOCK_HEADER)) {
			unsigned int i, j;
			unsigned char ChannelType;
			TrackCount = DocumentFile.GetBlockChar();
			for (i = 0; i <= TrackCount; i++) {
				// Add songs
				CString TrackTitle, name;
				name = DocumentFile.ReadString();
				TrackTitle.Format(TRACK_FORMAT, Tracks + i, name);
				m_pSongList->InsertItem(Tracks, TrackTitle);
				pDoc->AddTrack();
				pDoc->SetTrackTitle(Tracks + i, name);
			}
			for (i = 0; i < ImportChannels; i++) {
				ChannelType = DocumentFile.GetBlockChar();
				for (j = 0; j <= TrackCount; j++) {
					pDoc->SelectTrackFast(j + Tracks);
					int k = DocumentFile.GetBlockChar();
					pDoc->SetEffColumns(i, k);
				}
			}
		}
		else if (!strcmp(BlockID, FILE_BLOCK_FRAMES)) {
			unsigned int i, x, y;
			for (y = 0; y <= TrackCount; y++) {
				pDoc->SelectTrackFast(y + Tracks);
				pDoc->SetFrameCount(DocumentFile.GetBlockInt());				
				pDoc->SetSongSpeed(DocumentFile.GetBlockInt());
				pDoc->SetSongTempo(DocumentFile.GetBlockInt());
				pDoc->SetPatternLength(DocumentFile.GetBlockInt());
				for (i = 0; i < pDoc->GetFrameCount(); i++) {
					for (x = 0; x < ImportChannels; x++) {
						pDoc->SetPatternAtFrame(i, x, DocumentFile.GetBlockChar());
					}
				}
			}
		}
		else if (!strcmp(BlockID, FILE_BLOCK_PATTERNS)) {
			unsigned char Item;
			unsigned int Channel, Pattern, i, Items, Track;
			while (!DocumentFile.BlockDone()) {
				Track = DocumentFile.GetBlockInt();
				Channel = DocumentFile.GetBlockInt();
				Pattern = DocumentFile.GetBlockInt();
				Items = DocumentFile.GetBlockInt();

				pDoc->SelectTrackFast(Track + Tracks);

				for (i = 0; i < Items; i++) {
					Item = DocumentFile.GetBlockInt();
					stChanNote Note;
					pDoc->GetDataAtPattern(Track + Tracks, Pattern, Channel, Item, &Note);
					memset(&Note, 0, sizeof(stChanNote));

					Note.Note		= DocumentFile.GetBlockChar();
					Note.Octave		= DocumentFile.GetBlockChar();
					Note.Instrument	= DocumentFile.GetBlockChar();
					Note.Vol		= DocumentFile.GetBlockChar();

					int columns = pDoc->GetEffColumns(Channel);

					for (unsigned int n = 0; n < columns + 1; n++) {
						Note.EffNumber[n] = DocumentFile.GetBlockChar();
						Note.EffParam[n]  = DocumentFile.GetBlockChar();
					}

					if (Note.Vol > 0x10)
						Note.Vol &= 0x0F;

					pDoc->SetDataAtPattern(Track + Tracks, Pattern, Channel, Item, &Note);
				}			
			}
		}
		else if (!strcmp(BlockID, "END")) {
			FileFinished = true;
		}
	}

	pDoc->SelectTrack(Tracks);
}
