/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2012  Jonathan Liss
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
#include "MainFrm.h"
#include "ModulePropertiesDlg.h"
#include "ChannelMap.h"
#include "ModuleImportDlg.h"

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
	ON_BN_CLICKED(IDC_SONG_IMPORT, &CModulePropertiesDlg::OnBnClickedSongImport)
	//ON_CBN_SELCHANGE(IDC_EXPANSION, &CModulePropertiesDlg::OnCbnSelchangeExpansion)
	ON_WM_HSCROLL()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_SONGLIST, &CModulePropertiesDlg::OnLvnItemchangedSonglist)
	ON_BN_CLICKED(IDC_EXPANSION_FDS, &CModulePropertiesDlg::OnBnClickedExpansionFDS)
	ON_BN_CLICKED(IDC_EXPANSION_MMC5, &CModulePropertiesDlg::OnBnClickedExpansionMMC5)
    ON_BN_CLICKED(IDC_EXPANSION_VRC6, &CModulePropertiesDlg::OnBnClickedExpansionVRC6)
    ON_BN_CLICKED(IDC_EXPANSION_VRC7, &CModulePropertiesDlg::OnBnClickedExpansionVRC7)
    ON_BN_CLICKED(IDC_EXPANSION_5B, &CModulePropertiesDlg::OnBnClickedExpansion5B)
    ON_BN_CLICKED(IDC_EXPANSION_N163, &CModulePropertiesDlg::OnBnClickedExpansionN163)
END_MESSAGE_MAP()


// CModulePropertiesDlg message handlers

BOOL CModulePropertiesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Get active document
	CFrameWnd *pFrameWnd = static_cast<CFrameWnd*>(GetParent());
	m_pDocument = static_cast<CFamiTrackerDoc*>(pFrameWnd->GetActiveDocument());

	m_pSongList = (CListCtrl*)GetDlgItem(IDC_SONGLIST);
	m_pSongList->InsertColumn(0, _T("Songs"), 0, 150);
	m_pSongList->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	FillSongList();

	// Expansion chips
	/*CComboBox *pChipBox = (CComboBox*)GetDlgItem(IDC_EXPANSION);
	int ExpChip = m_pDocument->GetExpansionChip();
	CChannelMap *pChannelMap = theApp.GetChannelMap();*/
	
	m_iExpansions = m_pDocument->GetExpansionChip();
	((CButton*)GetDlgItem(IDC_EXPANSION_FDS))->SetCheck((m_iExpansions & SNDCHIP_FDS) != 0);
    ((CButton*)GetDlgItem(IDC_EXPANSION_MMC5))->SetCheck((m_iExpansions & SNDCHIP_MMC5) != 0);
    ((CButton*)GetDlgItem(IDC_EXPANSION_VRC6))->SetCheck((m_iExpansions & SNDCHIP_VRC6) != 0);
    ((CButton*)GetDlgItem(IDC_EXPANSION_VRC7))->SetCheck((m_iExpansions & SNDCHIP_VRC7) != 0);
    ((CButton*)GetDlgItem(IDC_EXPANSION_N163))->SetCheck((m_iExpansions & SNDCHIP_N163) != 0);
    ((CButton*)GetDlgItem(IDC_EXPANSION_5B))->SetCheck((m_iExpansions & SNDCHIP_S5B) != 0);
	
	/*for (int i = 0; i < pChannelMap->GetChipCount(); ++i)
		pChipBox->AddString(pChannelMap->GetChipName(i));

	pChipBox->SetCurSel(pChannelMap->GetChipIndex(ExpChip));
	*/

	// Vibrato 
	CComboBox *pVibratoBox = (CComboBox*)GetDlgItem(IDC_VIBRATO);
	pVibratoBox->SetCurSel((m_pDocument->GetVibratoStyle() == VIBRATO_NEW) ? 0 : 1);

	// Namco channel count
	/*CSliderCtrl *pChanSlider = (CSliderCtrl*)GetDlgItem(IDC_CHANNELS);
	pChanSlider->SetRange(1, 8);
	if (ExpChip == SNDCHIP_N163) {
		int Channels = m_pDocument->GetNamcoChannels();
		pChanSlider->SetPos(Channels);
		pChanSlider->EnableWindow(TRUE);
		CString text;
		text.Format(_T("Channels: %i"), Channels);
		SetDlgItemText(IDC_CHANNELS_NR, text);
	}
	else {
		pChanSlider->SetPos(0);
		pChanSlider->EnableWindow(FALSE);
		SetDlgItemText(IDC_CHANNELS_NR, _T("Channels: N/A"));
	}*/

	CSliderCtrl *pSlider = (CSliderCtrl*)GetDlgItem(IDC_CHANNELS);
	CStatic *pChannelsLabel = (CStatic*)GetDlgItem(IDC_CHANNELS_NR);
	pSlider->SetRange(1, 8);
	if (m_iExpansions & SNDCHIP_N163) {
		m_iN163Channels = m_pDocument->GetNamcoChannels();
		pSlider->SetPos(m_iN163Channels);
		pSlider->EnableWindow(TRUE);
        pChannelsLabel->EnableWindow(TRUE);
		CString text;
		text.Format(_T("Channels: %i"), m_iN163Channels);
		SetDlgItemText(IDC_CHANNELS_NR, text);
    }
    else {
        m_iN163Channels = 1;
        pSlider->EnableWindow(FALSE);
		SetDlgItemText(IDC_CHANNELS_NR, _T("Channels: N/A"));
        pChannelsLabel->EnableWindow(FALSE);
    }
	
	CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_5B);
    pCheckBox->EnableWindow(TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CModulePropertiesDlg::OnBnClickedOk()
{
	CComboBox *pExpansionChipBox = (CComboBox*)GetDlgItem(IDC_EXPANSION);
	CMainFrame *pMainFrame = (CMainFrame*)GetParentFrame();
	
	// Expansion chip
	/*unsigned int iExpansionChip = theApp.GetChannelMap()->GetChipIdent(pExpansionChipBox->GetCurSel());
	unsigned int iChannels = ((CSliderCtrl*)GetDlgItem(IDC_CHANNELS))->GetPos();

	if (m_pDocument->GetExpansionChip() != iExpansionChip || m_pDocument->GetNamcoChannels() != iChannels) {
		m_pDocument->SetNamcoChannels(iChannels);
		m_pDocument->SelectExpansionChip(iExpansionChip);
	}*/

    if (m_pDocument->GetNamcoChannels() != m_iN163Channels || m_pDocument->GetExpansionChip() != m_iExpansions) {
        m_pDocument->SetNamcoChannels(m_iN163Channels);
        m_pDocument->SelectExpansionChip(m_iExpansions);
	}

	// Vibrato 
	CComboBox *pVibratoBox = (CComboBox*)GetDlgItem(IDC_VIBRATO);
	m_pDocument->SetVibratoStyle((pVibratoBox->GetCurSel() == 0) ? VIBRATO_NEW : VIBRATO_OLD);

	if (pMainFrame->GetSelectedTrack() != m_iSelectedSong)
		m_pDocument->SelectTrack(m_iSelectedSong);

	OnOK();
}

void CModulePropertiesDlg::OnBnClickedSongAdd()
{
	CString TrackTitle;

	// Try to add a track
	if (!m_pDocument->AddTrack())
		return;
	
	// New track is always the last one
	int NewTrack = m_pDocument->GetTrackCount() - 1;
	
	TrackTitle.Format(TRACK_FORMAT, NewTrack, m_pDocument->GetTrackTitle(NewTrack));
	m_pSongList->InsertItem(NewTrack, TrackTitle);

	SelectSong(NewTrack);
}

void CModulePropertiesDlg::OnBnClickedSongRemove()
{
	ASSERT(m_iSelectedSong != -1);

	CString TrackTitle;

	unsigned Count = m_pDocument->GetTrackCount();

	if (Count == 1)
		return; // Single track

	// Display warning first
	if (AfxMessageBox(IDS_SONG_DELETE, MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL)
		return;

	m_pSongList->DeleteItem(m_iSelectedSong);
	m_pDocument->RemoveTrack(m_iSelectedSong);

	Count = m_pDocument->GetTrackCount();	// Get new track count

	// Redraw track list
	for (unsigned int i = 0; i < Count; ++i) {
		TrackTitle.Format(_T("#%02i %s"), i + 1, m_pDocument->GetTrackTitle(i));
		m_pSongList->SetItemText(i, 0, TrackTitle);
	}

	if (m_iSelectedSong == Count)
		SelectSong(m_iSelectedSong - 1);
	else
		SelectSong(m_iSelectedSong);
}

void CModulePropertiesDlg::OnBnClickedSongUp()
{
	CString Text;
	int Song = m_iSelectedSong;

	if (Song == 0)
		return;

	m_pDocument->MoveTrackUp(Song);

	Text.Format(TRACK_FORMAT, Song + 1, m_pDocument->GetTrackTitle(Song));
	m_pSongList->SetItemText(Song, 0, Text);
	Text.Format(TRACK_FORMAT, Song, m_pDocument->GetTrackTitle(Song - 1));
	m_pSongList->SetItemText(Song - 1, 0, Text);

	SelectSong(Song - 1);
}

void CModulePropertiesDlg::OnBnClickedSongDown()
{
	CString Text;
	int Song = m_iSelectedSong;

	if (Song == (m_pDocument->GetTrackCount() - 1))
		return;

	m_pDocument->MoveTrackDown(Song);

	Text.Format(TRACK_FORMAT, Song + 1, m_pDocument->GetTrackTitle(Song));
	m_pSongList->SetItemText(Song, 0, Text);
	Text.Format(TRACK_FORMAT, Song + 2, m_pDocument->GetTrackTitle(Song + 1));
	m_pSongList->SetItemText(Song + 1, 0, Text);

	SelectSong(Song + 1);
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

void CModulePropertiesDlg::SelectSong(int Song)
{
	ASSERT(Song >= 0);

	m_pSongList->SetItemState(Song, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_pSongList->EnsureVisible(Song, FALSE);
}

void CModulePropertiesDlg::UpdateSongButtons()
{
	unsigned TrackCount = m_pDocument->GetTrackCount();

	GetDlgItem(IDC_SONG_REMOVE)->EnableWindow((TrackCount == 1) ? FALSE : TRUE);
	GetDlgItem(IDC_SONG_ADD)->EnableWindow((TrackCount == MAX_TRACKS) ? FALSE : TRUE);
	GetDlgItem(IDC_SONG_DOWN)->EnableWindow((m_iSelectedSong == (TrackCount - 1)) ? FALSE : TRUE);
	GetDlgItem(IDC_SONG_UP)->EnableWindow((m_iSelectedSong == 0) ? FALSE : TRUE);
	GetDlgItem(IDC_SONG_IMPORT)->EnableWindow((TrackCount == MAX_TRACKS) ? FALSE : TRUE);
}

void CModulePropertiesDlg::OnBnClickedSongImport()
{
	CModuleImportDlg importDlg(m_pDocument);

	CFileDialog OpenFileDlg(TRUE, _T("ftm"), 0, OFN_HIDEREADONLY, _T("FamiTracker files (*.ftm)|*.ftm|All files (*.*)|*.*||"), theApp.GetMainWnd(), 0);

	if (OpenFileDlg.DoModal() == IDCANCEL)
		return;

	if (importDlg.LoadFile(OpenFileDlg.GetPathName(), m_pDocument) == false)
		return;

	importDlg.DoModal();

	FillSongList();
}

void CModulePropertiesDlg::OnCbnSelchangeExpansion()
{
	/*CComboBox *pExpansionChipBox = (CComboBox*)GetDlgItem(IDC_EXPANSION);
	CMainFrame *pMainFrame = (CMainFrame*)GetParentFrame();
	CSliderCtrl *pSlider = (CSliderCtrl*)GetDlgItem(IDC_CHANNELS);

	// Expansion chip
	unsigned int iExpansionChip = theApp.GetChannelMap()->GetChipIdent(pExpansionChipBox->GetCurSel());

	if (iExpansionChip == SNDCHIP_N163) {
		pSlider->EnableWindow(TRUE);
		int Channels = m_pDocument->GetNamcoChannels();
		pSlider->SetPos(Channels);
		CString text;
		text.Format(_T("Channels: %i"), Channels);
		SetDlgItemText(IDC_CHANNELS_NR, text);
	}
	else {
		pSlider->EnableWindow(FALSE);
		SetDlgItemText(IDC_CHANNELS_NR, _T("Channels: N/A"));
	}*/
}

void CModulePropertiesDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CSliderCtrl *pSlider = (CSliderCtrl*)GetDlgItem(IDC_CHANNELS);
	m_iN163Channels = pSlider->GetPos();
	CString text;
	//text.Format(_T("Channels: %i"),  pSlider->GetPos());
	text.Format(_T("Channels: %i"), m_iN163Channels);
	SetDlgItemText(IDC_CHANNELS_NR, text);

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CModulePropertiesDlg::OnLvnItemchangedSonglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

    if ((pNMLV->uChanged & LVIF_STATE) && (pNMLV->uNewState & LVNI_SELECTED)) {
		int Song = pNMLV->iItem;

		m_iSelectedSong = Song;

		CEdit *pName = (CEdit*)GetDlgItem(IDC_SONGNAME);
		pName->SetWindowText(CString(m_pDocument->GetTrackTitle(Song)));

		UpdateSongButtons();
    }

	*pResult = 0;
}

void CModulePropertiesDlg::FillSongList()
{
	CString Text;

	m_pSongList->DeleteAllItems();

	// Song editor
	int Songs = m_pDocument->GetTrackCount();

	for (int i = 0; i < Songs; ++i) {
		Text.Format(TRACK_FORMAT, i + 1, m_pDocument->GetTrackTitle(i));	// start counting songs from 1
		m_pSongList->InsertItem(i, Text);
	}

	// Select first song when dialog is displayed
	SelectSong(0);
}

BOOL CModulePropertiesDlg::PreTranslateMessage(MSG* pMsg)
{
	if (GetFocus() == m_pSongList) {
		if(pMsg->message == WM_KEYDOWN) {
			switch (pMsg->wParam) {
				case VK_DELETE:
					// Delete song
					if (m_iSelectedSong != -1) {
						OnBnClickedSongRemove();
					}
					break;
				case VK_INSERT:
					// Insert song
					OnBnClickedSongAdd();
					break;
			}
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CModulePropertiesDlg::OnBnClickedExpansionFDS()
{
    CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_FDS);
	pCheckBox->GetCheck() == BST_CHECKED ? m_iExpansions |= SNDCHIP_FDS : m_iExpansions &= ~SNDCHIP_FDS;
}

void CModulePropertiesDlg::OnBnClickedExpansionMMC5()
{
    CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_MMC5);
	pCheckBox->GetCheck() == BST_CHECKED ? m_iExpansions |= SNDCHIP_MMC5 : m_iExpansions &= ~SNDCHIP_MMC5;
}

void CModulePropertiesDlg::OnBnClickedExpansionVRC6()
{
    CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_VRC6);
	pCheckBox->GetCheck() == BST_CHECKED ? m_iExpansions |= SNDCHIP_VRC6 : m_iExpansions &= ~SNDCHIP_VRC6;
}

void CModulePropertiesDlg::OnBnClickedExpansionVRC7()
{
    CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_VRC7);
	pCheckBox->GetCheck() == BST_CHECKED ? m_iExpansions |= SNDCHIP_VRC7 : m_iExpansions &= ~SNDCHIP_VRC7;
}

void CModulePropertiesDlg::OnBnClickedExpansion5B()
{
    CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_5B);
	pCheckBox->GetCheck() == BST_CHECKED ? m_iExpansions |= SNDCHIP_S5B : m_iExpansions &= ~SNDCHIP_S5B;
}

void CModulePropertiesDlg::OnBnClickedExpansionN163()
{
    CButton *pCheckBox = (CButton*)GetDlgItem(IDC_EXPANSION_N163);

	CSliderCtrl *pSlider = (CSliderCtrl*)GetDlgItem(IDC_CHANNELS);
	CStatic *pChannelsLabel = (CStatic*)GetDlgItem(IDC_CHANNELS_NR);

	if (pCheckBox->GetCheck() == BST_CHECKED) {
        m_iExpansions |= SNDCHIP_N163;

        pSlider->EnableWindow(TRUE);
        pChannelsLabel->EnableWindow(TRUE);
		CString text;
		text.Format(_T("Channels: %i"), m_iN163Channels);
		SetDlgItemText(IDC_CHANNELS_NR, text);
    }
    else {
        m_iExpansions &= ~SNDCHIP_N163;

        pSlider->EnableWindow(FALSE);
		SetDlgItemText(IDC_CHANNELS_NR, _T("Channels: N/A"));
        pChannelsLabel->EnableWindow(FALSE);
    }
}
