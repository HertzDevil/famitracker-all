/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2006  Jonathan Liss
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

#include "MainFrm.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "SampleWindow.h"
#include "soundsettings.h"
#include "nsfdialog.h"
#include "PerformanceDlg.h"
#include "InstrumentEditDlg.h"

#include "InstrumentSettings.h"
#include "InstrumentDPCM.h"
#include "MidiImport.h"

#include "ConfigGeneral.h"
#include "ConfigAppearance.h"
#include "ConfigMIDI.h"
#include "ConfigWindow.h"
#include ".\mainfrm.h"
#include "..\include\mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define GET_VIEW	((CFamiTrackerView*)GetActiveView())
#define GET_DOC		((CFamiTrackerDoc*)GetDocument())

const char NEW_INST_NAME[] = "New instrument";

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_FILE_CREATE_NSF, OnCreateNSF)
	ON_COMMAND(ID_FILE_SOUNDSETTINGS, OnSoundSettings)
	ON_COMMAND(ID_TRACKER_KILLSOUND, OnTrackerKillsound)
	ON_COMMAND(ID_TRACKER_TOGGLE_PLAY, OnTrackerTogglePlay)
	ON_COMMAND(IDC_KEYREPEAT, OnKeyRepeat)
	ON_COMMAND(ID_NEXT_FRAME, OnNextFrame)
	ON_COMMAND(ID_PREV_FRAME, OnPrevFrame)
	ON_COMMAND(ID_EDIT_FRAMEINC, OnBnClickedIncFrame)
	ON_COMMAND(ID_EDIT_FRAMEDEC, OnBnClickedDecFrame)
	ON_COMMAND(IDC_CHANGE_ALL, OnChangeAll)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_FRAME_INC, OnBnClickedIncFrame)
	ON_BN_CLICKED(IDC_FRAME_DEC, OnBnClickedDecFrame)
	ON_BN_CLICKED(IDC_ADD_INST, OnBnClickedAddInst)
	ON_BN_CLICKED(IDC_REMOVE_INST, OnBnClickedRemoveInst)
	ON_BN_CLICKED(IDC_EDIT_INST, OnBnClickedEditInst)
	ON_NOTIFY(NM_CLICK, IDC_INSTRUMENTS, OnClickInstruments)
	ON_NOTIFY(NM_DBLCLK, IDC_INSTRUMENTS, OnDblClkInstruments)
	ON_NOTIFY(UDN_DELTAPOS, IDC_TEMPO_SPIN, OnDeltaposTempoSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_ROWS_SPIN, OnDeltaposRowsSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_PATTERNS_SPIN, OnDeltaposPatternsSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_KEYSTEP_SPIN, OnDeltaposKeyStepSpin)
	ON_EN_CHANGE(IDC_INSTNAME, OnInstNameChange)
	ON_EN_CHANGE(IDC_PATTERNS, OnEnPatternsChange)
	ON_EN_CHANGE(IDC_SPEED, OnEnTempoChange)
	ON_EN_CHANGE(IDC_ROWS, OnEnRowsChange)
	ON_EN_CHANGE(IDC_KEYSTEP, OnEnKeyStepChange)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_HELP_PERFORMANCE, OnHelpPerformance)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_INSTRUMENT, OnUpdateSBInstrument)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_OCTAVE, OnUpdateSBOctave)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_RATE, OnUpdateSBFrequency)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_TEMPO, OnUpdateSBTempo)
	ON_UPDATE_COMMAND_UI(IDC_KEYSTEP, OnUpdateKeyStepEdit)
	ON_UPDATE_COMMAND_UI(IDC_KEYREPEAT, OnUpdateKeyRepeat)
	ON_WM_TIMER()
	ON_UPDATE_COMMAND_UI(IDC_SPEED, OnUpdateSpeedEdit)
	ON_UPDATE_COMMAND_UI(IDC_ROWS, OnUpdateRowsEdit)
	ON_UPDATE_COMMAND_UI(IDC_FRAMES, OnUpdateFramesEdit)
	ON_COMMAND(ID_FILE_GENERALSETTINGS, OnFileGeneralsettings)
	ON_EN_CHANGE(IDC_SONG_NAME, OnEnSongNameChange)
	ON_EN_CHANGE(IDC_SONG_ARTIST, OnEnSongArtistChange)
	ON_EN_CHANGE(IDC_SONG_COPYRIGHT, OnEnSongCopyrightChange)
	ON_COMMAND(ID_FILE_IMPORTMIDI, OnFileImportmidi)
	ON_EN_KILLFOCUS(IDC_SONG_NAME, OnEnKillfocusSongName)
	ON_EN_KILLFOCUS(IDC_SONG_ARTIST, OnEnKillfocusSongArtist)
	ON_EN_KILLFOCUS(IDC_SONG_COPYRIGHT, OnEnKillfocusSongCopyright)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_INSTRUMENT, 
	ID_INDICATOR_OCTAVE,
	ID_INDICATOR_RATE,
	ID_INDICATOR_TEMPO,
	ID_INDICATOR_TIME,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_bInitialized = false;
}

CMainFrame::~CMainFrame()
{
}

CPerformanceDlg PerformanceDlg;

CSampleWindow	SampleWindow;
CSampleWinProc	SampleProc;

CInstrumentEditDlg	m_InstrumentEdit;

CImageList *m_pImageList;

////////////////

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| /*CBRS_GRIPPER |*/ CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC ) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	
	if (!m_wndDialogBar.Create(this, IDD_MAINFRAME, CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY, IDD_MAINFRAME))
	{
		TRACE0("Failed to create dialog bar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	if (!m_wndPatternWindow.CreateEx(WS_EX_CLIENTEDGE, NULL, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | WS_DLGFRAME, CRect(10, 10, 164, 173), (CWnd*)&m_wndDialogBar, 0)) {
		TRACE0("Failed to create pattern window\n");
		return -1;      // fail to create
	}

	/*
	// TODO: Delete these three lines if you don't want the toolbar to be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
	*/
	
	SampleWindow.CreateEx(WS_EX_CLIENTEDGE, NULL, "", WS_CHILD | WS_VISIBLE, CRect(297, 115, 297 + CSampleWindow::WIN_WIDTH, 115 + CSampleWindow::WIN_HEIGHT), (CWnd*)&m_wndDialogBar, 0);

	SampleProc.Wnd = &SampleWindow;
	SampleProc.CreateThread();

	InstrumentList = (CListCtrl*)m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS);

	InstrumentList->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	SetupColors();

	m_wndToolBar.SetButtonStyle(13, TBBS_CHECKBOX);
	m_wndToolBar.SetDlgItemText(IDC_KEYSTEP, "1");

	m_pImageList = new CImageList();
	m_pImageList->Create(16, 16, ILC_COLOR, 1, 1);
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_2A03INV));

	InstrumentList->SetImageList(m_pImageList, LVSIL_NORMAL);
	InstrumentList->SetImageList(m_pImageList, LVSIL_SMALL);

	CSliderCtrl *VolSlider;

	VolSlider = (CSliderCtrl*)m_wndDialogBar.GetDlgItem(IDC_VOLUME);

	VolSlider->SetRange(1, 100);

	SetTimer(0, 100, 0);

//	m_wndDialogBar.ShowWindow(SW_HIDE);

	m_bInitialized = true;

	return 0;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG


// CMainFrame message handlers

void CMainFrame::SetStatusText(LPCTSTR Text,...)
{
	char	Buf[512];
    va_list argp;
    
	va_start(argp, Text);
    
	if (!Text)
		return;

    vsprintf(Buf, Text, argp);

	m_wndStatusBar.SetWindowText(Buf);
}

void CMainFrame::ClearInstrumentList()
{
	InstrumentList = (CListCtrl*)m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS);
	InstrumentList->DeleteAllItems();

	m_wndDialogBar.GetDlgItem(IDC_INSTNAME)->SetWindowText("");
}

void CMainFrame::AddInstrument(int Index, const char *Name)
{
	CString Text;
//	Text.Format("%i - %s", Index, Name);
	Text.Format("%02X - %s", Index, Name);

	if (Text.GetLength() > 30) {
		Text.Format("%s...", Text.GetBufferSetLength(30));
	}
	
	InstrumentList = (CListCtrl*)m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS);
	InstrumentList->InsertItem(Index, Text);
}

void CMainFrame::RemoveInstrument(int Index) 
{
	CString Text, Text2;

	InstrumentList = (CListCtrl*)m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS);

	Index = InstrumentList->GetSelectionMark();

	if (InstrumentList->GetItemCount() == -1)
		return;

	InstrumentList->DeleteItem(Index);
}

void CMainFrame::SetIndicatorTime(int Min, int Sec, int MSec)
{
	CString String;

	String.Format("%02i:%02i:%01i0", Min, Sec, MSec);

	m_wndStatusBar.SetPaneText(5, String);
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

	if (m_bInitialized == false)
		return;
	
	m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS)->MoveWindow(450, 26, cx - 460, 144);
	m_wndDialogBar.GetDlgItem(IDC_INSTNAME)->MoveWindow(485, 175, cx - 645, 22);
	m_wndDialogBar.GetDlgItem(IDC_EDIT_INST)->MoveWindow(cx - 150, 175, 140, 22);
	m_wndDialogBar.GetDlgItem(IDC_VOLUME)->MoveWindow(cx - 120, 6, 110, 18);
}

void CMainFrame::OnClickInstruments(NMHDR *pNotifyStruct, LRESULT *result)
{
	int Instrument = 0;
	char Text[256];

	CFamiTrackerView *pView = (CFamiTrackerView*)GetActiveView();
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	
	InstrumentList = (CListCtrl*)m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS);

	if (InstrumentList->GetSelectionMark() == -1)
		return;

	InstrumentList->GetItemText(InstrumentList->GetSelectionMark(), 0, Text, 256);
	sscanf(Text, "%X", &Instrument);

	pDoc->GetInstName(Instrument, Text);

	pView->SetInstrument(Instrument);

	m_wndDialogBar.GetDlgItem(IDC_INSTNAME)->SetWindowText(Text);

	GetActiveView()->SetFocus();
}

void CMainFrame::OnDblClkInstruments(NMHDR *pNotifyStruct, LRESULT *result)
{
	OpenInstrumentSettings();
}

void CMainFrame::OnInstNameChange()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CFamiTrackerView *pView	= (CFamiTrackerView*)GetActiveView();
	char Text[256], Name[256];
	int Instrument;

	InstrumentList = (CListCtrl*)m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS);

	if (InstrumentList->GetSelectionMark() == -1)
		return;

	Instrument = pView->m_iInstrument;

	((CEdit*)m_wndDialogBar.GetDlgItem(IDC_INSTNAME))->GetWindowText(Text, 256);

	// Doesn't need to be longer than 60 chars
	Text[60] = 0;

	sprintf(Name, "%02X - %s", Instrument, Text);
	InstrumentList->SetItemText(InstrumentList->GetSelectionMark(), 0, Name);
	strcpy(Name, pDoc->Instruments[Instrument].Name);

	pDoc->InstrumentName(Instrument, Text);
}

void CMainFrame::OnBnClickedAddInst()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();

	AddInstrument(pDoc->AddInstrument(NEW_INST_NAME), NEW_INST_NAME);
}

void CMainFrame::OnBnClickedRemoveInst()
{
	CFamiTrackerView *pView = (CFamiTrackerView*)GetActiveView();
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();

	InstrumentList = (CListCtrl*)m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS);

	if (InstrumentList->GetSelectionMark() == -1)
		return;

	pDoc->RemoveInstrument(pView->m_iInstrument);
	RemoveInstrument(pView->m_iInstrument);
}

void CMainFrame::OnBnClickedEditInst()
{
	OpenInstrumentSettings();
}

void CMainFrame::OnTrackerTogglePlay()
{
	((CFamiTrackerView*)GetView())->TogglePlayback();
}

void CMainFrame::OnEnPatternsChange()
{
	int Index;
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();

	if (!m_bInitialized || !pDoc)
		return;

	Index = m_wndDialogBar.GetDlgItemInt(IDC_PATTERNS, 0, 0);

	LIMIT(Index, MAX_FRAMES, 1);

	pDoc->SetPatternCount(Index);
	RefreshPattern();
}

void CMainFrame::OnDeltaposTempoSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int Pos = m_wndDialogBar.GetDlgItemInt(IDC_SPEED) - ((NMUPDOWN*)pNMHDR)->iDelta;
	LIMIT(Pos, MAX_TEMPO, MIN_TEMPO);
	GET_VIEW->SetSongSpeed(Pos);
}

void CMainFrame::OnEnTempoChange()
{
	CFamiTrackerView *pView = (CFamiTrackerView*)GetActiveView();
	ASSERT_VALID(pView);

	int Tempo = m_wndDialogBar.GetDlgItemInt(IDC_SPEED, 0, 0);
	LIMIT(Tempo, MAX_TEMPO, MIN_TEMPO);
	pView->SetSongSpeed(Tempo);
}

void CMainFrame::OnTrackerKillsound()
{
	theApp.SilentEverything();
}

void CMainFrame::OnDeltaposRowsSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int Pos = m_wndDialogBar.GetDlgItemInt(IDC_ROWS) - ((NMUPDOWN*)pNMHDR)->iDelta;
	LIMIT(Pos, MAX_PATTERN_LENGTH, 1);
	m_wndDialogBar.SetDlgItemInt(IDC_ROWS, Pos);
}

void CMainFrame::OnDeltaposPatternsSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int Pos = m_wndDialogBar.GetDlgItemInt(IDC_PATTERNS) - ((NMUPDOWN*)pNMHDR)->iDelta;
	LIMIT(Pos, MAX_FRAMES, 1);
	m_wndDialogBar.SetDlgItemInt(IDC_PATTERNS, Pos);
}

void CMainFrame::OnEnRowsChange()
{
	int Index;

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();

	if (!m_bInitialized || !pDoc)
		return;

	Index = m_wndDialogBar.GetDlgItemInt(IDC_ROWS);
	
	LIMIT(Index, MAX_PATTERN_LENGTH, 1);

	pDoc->SetRowCount(Index);
	GetActiveView()->RedrawWindow();
}

void CMainFrame::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: Add your message handler code here
	// Do not call CFrameWnd::OnPaint() for painting messages

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CFamiTrackerView *pView = (CFamiTrackerView*)GetActiveView();

	// hmm, fix this

	m_wndPatternWindow.SetDocument(pDoc, pView);
	m_wndPatternWindow.RedrawWindow();	
}

void CMainFrame::RefreshPattern()
{
	m_wndPatternWindow.Invalidate(FALSE);
	m_wndPatternWindow.PostMessage(WM_PAINT);
}

void CMainFrame::OnBnClickedIncFrame()
{
	((CFamiTrackerView*)GetActiveView())->IncreaseCurrentFrame();
	RedrawWindow();
}

void CMainFrame::OnBnClickedDecFrame()
{
	((CFamiTrackerView*)GetActiveView())->DecreaseCurrentFrame();
	RedrawWindow();
}

void CMainFrame::OnKeyRepeat()
{
	if (m_wndDialogBar.IsDlgButtonChecked(IDC_KEYREPEAT))
		theApp.m_pSettings->General.bKeyRepeat = true;
	else
		theApp.m_pSettings->General.bKeyRepeat = false;
}

void CMainFrame::OnDeltaposKeyStepSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int Pos;

	Pos = m_wndDialogBar.GetDlgItemInt(IDC_KEYSTEP) - ((NMUPDOWN*)pNMHDR)->iDelta;

	LIMIT(Pos, MAX_PATTERN_LENGTH, 0);

	m_wndDialogBar.SetDlgItemInt(IDC_KEYSTEP, Pos);
}

void CMainFrame::OnEnKeyStepChange()
{
	int Step;

	Step = m_wndDialogBar.GetDlgItemInt(IDC_KEYSTEP);

	LIMIT(Step, MAX_PATTERN_LENGTH, 0);

	((CFamiTrackerView*)GetActiveView())->m_iKeyStepping = Step;
}

void CMainFrame::OnCreateNSF()
{
	CNSFDialog NSFDialog;

	NSFDialog.DoModal();
}

void CMainFrame::OnSoundSettings()
{
	CSoundSettings SoundSettings;

	SoundSettings.DoModal();
}

BOOL CMainFrame::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle , const RECT& rect , CWnd* pParentWnd , LPCTSTR lpszMenuName , DWORD dwExStyle , CCreateContext* pContext)
{
	RECT newrect;

	// Resize the window after startup
	newrect.top		= 100;
	newrect.left	= 100;
	newrect.right	= newrect.left + 850;
	newrect.bottom	= newrect.top + 820;

	return CFrameWnd::Create(lpszClassName, lpszWindowName, dwStyle, newrect, pParentWnd, lpszMenuName, dwExStyle, pContext);
}

void CMainFrame::OnNextFrame()
{
	((CFamiTrackerView*)GetActiveView())->SelectNextPattern();
}

void CMainFrame::OnPrevFrame()
{
	((CFamiTrackerView*)GetActiveView())->SelectPrevPattern();
}

void CMainFrame::OnChangeAll()
{
	if (m_wndDialogBar.IsDlgButtonChecked(IDC_CHANGE_ALL))
		((CFamiTrackerView*)GetActiveView())->m_bChangeAllPattern = true;
	else
		((CFamiTrackerView*)GetActiveView())->m_bChangeAllPattern = false;
}

void CMainFrame::DrawSamples(int *Samples, int Count)
{
	if (!m_bInitialized)
		return;

	SampleProc.PostThreadMessage(WM_USER, (WPARAM)Samples, Count);
}

void CMainFrame::OnHelpPerformance()
{
	PerformanceDlg.Create(MAKEINTRESOURCE(IDD_PERFORMANCE), this);
	PerformanceDlg.ShowWindow(SW_SHOW);
}

void CMainFrame::OnUpdateSBInstrument(CCmdUI *pCmdUI)
{
	CString String;
	String.Format("Instrument: %02i", GET_VIEW->m_iInstrument);

	pCmdUI->Enable(); 
	pCmdUI->SetText(String);
}

void CMainFrame::OnUpdateSBOctave(CCmdUI *pCmdUI)
{
	CString String;
	String.Format("Octave: %i", GET_VIEW->m_iOctave);

	pCmdUI->Enable(); 
	pCmdUI->SetText(String);
}

void CMainFrame::OnUpdateSBFrequency(CCmdUI *pCmdUI)
{
	CString String;
	int Machine = GET_DOC->m_iMachine;
	int EngineSpeed = GET_DOC->m_iEngineSpeed;

	if (EngineSpeed == 0)
		EngineSpeed = (Machine == NTSC ? 60 : 50);

	String.Format("%i Hz", EngineSpeed);

	pCmdUI->Enable(); 
	pCmdUI->SetText(String);
}

void CMainFrame::OnUpdateSBTempo(CCmdUI *pCmdUI)
{
	CString String;
	String.Format("%i BPM", GET_VIEW->m_iCurrentTempo);

	pCmdUI->Enable(); 
	pCmdUI->SetText(String);
}

void CMainFrame::OnUpdateSBPosition(CCmdUI *pCmdUI)
{
	CString String;
	String.Format("Row %i", GET_VIEW->m_iCursorRow);

	pCmdUI->Enable(); 
	pCmdUI->SetText(String);
}

void CMainFrame::OnTimer(UINT nIDEvent)
{
	SetMessageText("Welcome to FamiTracker, press F1 for help");
	KillTimer(0);
	CFrameWnd::OnTimer(nIDEvent);
}

void CMainFrame::OnUpdateKeyStepEdit(CCmdUI *pCmdUI)
{
	CString Text;
	Text.Format("%i", GET_VIEW->m_iKeyStepping);
	pCmdUI->SetText(Text);
}

void CMainFrame::OnUpdateSpeedEdit(CCmdUI *pCmdUI)
{
	CString Text;
	Text.Format("%i", GET_DOC->m_iSongSpeed);
	pCmdUI->SetText(Text);
}

void CMainFrame::OnUpdateRowsEdit(CCmdUI *pCmdUI)
{
	CString Text;
	Text.Format("%i", GET_DOC->m_iPatternLength);
	pCmdUI->SetText(Text);
}

void CMainFrame::OnUpdateFramesEdit(CCmdUI *pCmdUI)
{
	CString Text;
	Text.Format("%i", GET_DOC->m_iFrameCount);
	pCmdUI->SetText(Text);
}

void CMainFrame::OnFileGeneralsettings()
{
	CConfigWindow ConfigWindow("FamiTracker configuration", this, 0);

	CConfigGeneral		TabGeneral;
	CConfigAppearance	TabAppearance;
	CConfigMIDI			TabMIDI;

	TabGeneral.m_psp.dwFlags	&= ~PSP_HASHELP;
	TabAppearance.m_psp.dwFlags &= ~PSP_HASHELP;

	ConfigWindow.AddPage((CPropertyPage*)&TabGeneral);
	ConfigWindow.AddPage((CPropertyPage*)&TabAppearance);
	ConfigWindow.AddPage((CPropertyPage*)&TabMIDI);

	ConfigWindow.DoModal();
}

void CMainFrame::SetSongInfo(char *Name, char *Artist, char *Copyright)
{
	m_wndDialogBar.SetDlgItemText(IDC_SONG_NAME, Name);
	m_wndDialogBar.SetDlgItemText(IDC_SONG_ARTIST, Artist);
	m_wndDialogBar.SetDlgItemText(IDC_SONG_COPYRIGHT, Copyright);
}

void CMainFrame::OnEnSongNameChange()
{
	m_wndDialogBar.GetDlgItemText(IDC_SONG_NAME, ((CFamiTrackerDoc*)GetActiveDocument())->m_strName, 32);
}

void CMainFrame::OnEnSongArtistChange()
{
	m_wndDialogBar.GetDlgItemText(IDC_SONG_ARTIST, ((CFamiTrackerDoc*)GetActiveDocument())->m_strArtist, 32);
}

void CMainFrame::OnEnSongCopyrightChange()
{
	m_wndDialogBar.GetDlgItemText(IDC_SONG_COPYRIGHT, ((CFamiTrackerDoc*)GetActiveDocument())->m_strCopyright, 32);
}

void CMainFrame::ChangeNoteState(int Note)
{
	m_InstEdit.ChangeNoteState(Note);
}

void CMainFrame::OpenInstrumentSettings()
{
	CFamiTrackerDoc		*pDoc	= (CFamiTrackerDoc*)GetActiveDocument();
	CFamiTrackerView	*pView	= (CFamiTrackerView*)GetActiveView();

	if (pDoc->Instruments[pView->m_iInstrument].Free == false)
		m_InstEdit.DoModal();
}

void CMainFrame::OnUpdateKeyRepeat(CCmdUI *pCmdUI)
{
	if (theApp.m_pSettings->General.bKeyRepeat)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}

void CMainFrame::OnFileImportmidi()
{
	CMIDIImport	Importer;

	if (GetActiveDocument()->SaveModified() == 0)
		return;

	CFileDialog FileDialog(TRUE, 0, 0, OFN_HIDEREADONLY, "MIDI files (*.mid)|*.mid|All files|*.*||");

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	Importer.ImportFile(FileDialog.GetPathName());
}

void CMainFrame::SetupColors(void)
{
	InstrumentList = (CListCtrl*)m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS);

	InstrumentList->SetBkColor(theApp.m_pSettings->Appearance.iColBackground);
	InstrumentList->SetTextBkColor(theApp.m_pSettings->Appearance.iColBackground);
	InstrumentList->SetTextColor(theApp.m_pSettings->Appearance.iColPatternText);
	InstrumentList->RedrawWindow();
}

void CMainFrame::OnEnKillfocusSongName()
{
	char Limit[32];
	m_wndDialogBar.GetDlgItemText(IDC_SONG_NAME, Limit, 32);
	m_wndDialogBar.SetDlgItemText(IDC_SONG_NAME, Limit);
}

void CMainFrame::OnEnKillfocusSongArtist()
{
	char Limit[32];
	m_wndDialogBar.GetDlgItemText(IDC_SONG_ARTIST, Limit, 32);
	m_wndDialogBar.SetDlgItemText(IDC_SONG_ARTIST, Limit);
}

void CMainFrame::OnEnKillfocusSongCopyright()
{
	char Limit[32];
	m_wndDialogBar.GetDlgItemText(IDC_SONG_COPYRIGHT, Limit, 32);
	m_wndDialogBar.SetDlgItemText(IDC_SONG_COPYRIGHT, Limit);
}
