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
#include "SoundGen.h"
#include "WavProgressDlg.h"


// CWavProgressDlg dialog

IMPLEMENT_DYNAMIC(CWavProgressDlg, CDialog)

CWavProgressDlg::CWavProgressDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWavProgressDlg::IDD, pParent)
{

}

CWavProgressDlg::~CWavProgressDlg()
{
}

void CWavProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CWavProgressDlg, CDialog)
	ON_BN_CLICKED(IDC_CANCEL, &CWavProgressDlg::OnBnClickedCancel)
	ON_WM_TIMER()
END_MESSAGE_MAP()


static CSoundGen *pSoundGen;
CProgressCtrl	*pProgressBar;
CFamiTrackerView	*pView;
CFamiTrackerDoc		*pDoc;
DWORD	StartTime;

// CWavProgressDlg message handlers

void CWavProgressDlg::OnBnClickedCancel()
{
	if (pSoundGen->IsRendering()) {
		pSoundGen->StopRendering();
	}
	EndDialog(0);
}

void CWavProgressDlg::SetFile(CString File)
{
	m_sFile = File;
}

void CWavProgressDlg::SetOptions(int LengthType, int LengthParam)
{
	// Set song length
	m_iSongEndType = LengthType;
	m_iSongEndParam = LengthParam;
}

BOOL CWavProgressDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CString FileStr;

	if (m_sFile.GetLength() == 0)
		return TRUE;

	pSoundGen = theApp.GetSoundGenerator();
	pProgressBar = (CProgressCtrl*)GetDlgItem(IDC_PROGRESS_BAR);
	pView = (CFamiTrackerView*)theApp.GetDocumentView();
	pDoc = (CFamiTrackerDoc*)pView->GetDocument();

	pView->Invalidate();
	pView->RedrawWindow();

	pProgressBar->SetRange(0, 100);

	// Start rendering
	FileStr.Format("Saving to: %s", m_sFile.GetString());
	SetDlgItemText(IDC_PROGRESS_FILE, FileStr);

	if (!pSoundGen->RenderToFile((char*)m_sFile.GetString(), m_iSongEndType, m_iSongEndParam))
		EndDialog(0);

	StartTime = GetTickCount();
	SetTimer(0, 200, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CWavProgressDlg::OnTimer(UINT_PTR nIDEvent)
{
	// Update progress status
	CString Text;
	int Frame, FrameCount, PercentDone;
	int RenderedTime;
	bool Done;
	DWORD Time = (GetTickCount() - StartTime) / 1000;
	
	//Frame = pView->GetSelectedFrame() + 1;
	FrameCount = pDoc->GetFrameCount();

	pSoundGen->GetRenderStat(Frame, RenderedTime, Done);

	if (m_iSongEndType == SONG_LOOP_LIMIT) {
		FrameCount *= m_iSongEndParam;
		PercentDone = (Frame * 100) / FrameCount;
		Text.Format("Frame: %i / %i (%i%% done) ", Frame, FrameCount, PercentDone);
	}
	else if (m_iSongEndType == SONG_TIME_LIMIT) {
		int TotalSec, TotalMin;
		int CurrSec, CurrMin;
		TotalSec = m_iSongEndParam % 60;
		TotalMin = m_iSongEndParam / 60;
		CurrSec = RenderedTime % 60;
		CurrMin = RenderedTime / 60;
		PercentDone = (RenderedTime * 100) / m_iSongEndParam;
		Text.Format("Time: %02i:%02i / %02i:%02i (%i%% done) ", CurrMin, CurrSec, TotalMin, TotalSec, PercentDone);
	}

	SetDlgItemText(IDC_PROGRESS_LBL, Text);

	Text.Format("Elapsed time: %02i:%02i", (Time / 60), (Time % 60));
	SetDlgItemText(IDC_TIME, Text);

	pProgressBar->SetPos(PercentDone);

	if (!pSoundGen->IsRendering()) {
		SetDlgItemText(IDC_CANCEL, "Done");
	//	Text.Format("%02i:02i, frame: %i / %i (%i%% done) ", (Time / 60), (Time % 60), FrameCount, FrameCount, 100);
//		Text.Format("Frame: %i / %i (%i%% done) ", FrameCount, FrameCount, 100);
//		Text.Format("%02i:%02i, (%i%% done)", (Time / 60), (Time % 60), 100);
//		SetDlgItemText(IDC_PROGRESS_LBL, Text);
		pProgressBar->SetPos(100);
		KillTimer(0);
	}

	CDialog::OnTimer(nIDEvent);
}
