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
** Any permitted reproduction of these routin, in whole or in part,
** must bear this legend.
*/

#include "stdafx.h"
#include "FamiTracker.h"
#include "SoundGen.h"

#include "MainFrm.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "NsfDialog.h"
#include "InstrumentEditDlg.h"
#include "ModulePropertiesDlg.h"

#include "InstrumentSettings.h"
#include "InstrumentDPCM.h"
#include "MidiImport.h"

#include "ConfigGeneral.h"
#include "ConfigAppearance.h"
#include "ConfigMIDI.h"
#include "ConfigSound.h"
#include "ConfigWindow.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define GET_VIEW	((CFamiTrackerView*)GetActiveView())
#define GET_DOC		((CFamiTrackerDoc*)GetDocument())

const char NEW_INST_NAME[] = "New instrument";

extern CSoundGen SoundGenerator;

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	// Global help commands
	ON_COMMAND(ID_FILE_GENERALSETTINGS, OnFileGeneralsettings)
	ON_COMMAND(ID_FILE_IMPORTMIDI, OnFileImportmidi)
	ON_COMMAND(ID_FILE_CREATE_NSF, OnCreateNSF)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	ON_COMMAND(ID_HELP_PERFORMANCE, OnHelpPerformance)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_TRACKER_KILLSOUND, OnTrackerKillsound)
	ON_COMMAND(ID_NEXT_FRAME, OnNextFrame)
	ON_COMMAND(ID_PREV_FRAME, OnPrevFrame)
	ON_COMMAND(ID_EDIT_FRAMEINC, OnBnClickedIncFrame)
	ON_COMMAND(ID_EDIT_FRAMEDEC, OnBnClickedDecFrame)
	ON_COMMAND(IDC_CHANGE_ALL, OnChangeAll)
	ON_COMMAND(IDC_KEYREPEAT, OnKeyRepeat)
	ON_BN_CLICKED(IDC_FRAME_INC, OnBnClickedIncFrame)
	ON_BN_CLICKED(IDC_FRAME_DEC, OnBnClickedDecFrame)
	ON_COMMAND(ID_MODULE_ADDINSTRUMENT, OnAddInstrument)
	ON_COMMAND(ID_MODULE_REMOVEINSTRUMENT, OnRemoveInstrument)
	ON_COMMAND(ID_MODULE_SAVEINSTRUMENT, OnSaveInstrument)
	ON_COMMAND(ID_MODULE_LOADINSTRUMENT, OnLoadInstrument)
	ON_BN_CLICKED(IDC_EDIT_INST, OnBnClickedEditInst)
	ON_NOTIFY(NM_CLICK, IDC_INSTRUMENTS, OnClickInstruments)
	ON_NOTIFY(NM_RCLICK, IDC_INSTRUMENTS, OnClickInstruments)
	ON_NOTIFY(NM_DBLCLK, IDC_INSTRUMENTS, OnDblClkInstruments)
	ON_NOTIFY(UDN_DELTAPOS, IDC_TEMPO_SPIN, OnDeltaposTempoSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_ROWS_SPIN, OnDeltaposRowsSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_FRAME_SPIN, OnDeltaposFrameSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_KEYSTEP_SPIN, OnDeltaposKeyStepSpin)
	ON_EN_CHANGE(IDC_INSTNAME, OnInstNameChange)
	ON_EN_CHANGE(IDC_FRAMES, OnEnFramesChange)
	ON_EN_CHANGE(IDC_SPEED, OnEnTempoChange)
	ON_EN_CHANGE(IDC_ROWS, OnEnRowsChange)
	ON_EN_CHANGE(IDC_KEYSTEP, OnEnKeyStepChange)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_INSTRUMENT, OnUpdateSBInstrument)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_OCTAVE, OnUpdateSBOctave)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_RATE, OnUpdateSBFrequency)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_TEMPO, OnUpdateSBTempo)
	ON_UPDATE_COMMAND_UI(IDC_KEYSTEP, OnUpdateKeyStepEdit)
	ON_UPDATE_COMMAND_UI(IDC_KEYREPEAT, OnUpdateKeyRepeat)
	ON_UPDATE_COMMAND_UI(IDC_SPEED, OnUpdateSpeedEdit)
	ON_UPDATE_COMMAND_UI(IDC_ROWS, OnUpdateRowsEdit)
	ON_UPDATE_COMMAND_UI(IDC_FRAMES, OnUpdateFramesEdit)
	ON_EN_CHANGE(IDC_SONG_NAME, OnEnSongNameChange)
	ON_EN_CHANGE(IDC_SONG_ARTIST, OnEnSongArtistChange)
	ON_EN_CHANGE(IDC_SONG_COPYRIGHT, OnEnSongCopyrightChange)
	ON_EN_KILLFOCUS(IDC_SONG_NAME, OnEnKillfocusSongName)
	ON_EN_KILLFOCUS(IDC_SONG_ARTIST, OnEnKillfocusSongArtist)
	ON_EN_KILLFOCUS(IDC_SONG_COPYRIGHT, OnEnKillfocusSongCopyright)
	ON_COMMAND(ID_MODULE_MODULEPROPERTIES, OnModuleModuleproperties)
	ON_CBN_SELCHANGE(IDC_SUBTUNE, OnCbnSelchangeSong)
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

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	REBARBANDINFO rbi;

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT, WS_CHILD | WS_VISIBLE | CBRS_TOP | /*CBRS_GRIPPER |*/ CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME)) 
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	m_wndToolBar.SetBarStyle(CBRS_ALIGN_TOP | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

	rbi.cbSize		= sizeof(REBARBANDINFO);
	rbi.fMask		= RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_TEXT;
	rbi.fStyle		= RBBS_NOGRIPPER;
	rbi.cxMinChild	= 300;
	rbi.cyMinChild	= 23;
	rbi.lpText		= "";
	rbi.cch			= 7;
	rbi.cx			= 10;
	rbi.hwndChild	= m_wndToolBar;//(HWND)m_wndToolBar;
//	rbi.clrFore		= 00; rbi.clrBack = 0xFF;

	if (!m_wndToolBarReBar.Create(this) ||
		!m_wndToolBarReBar.GetReBarCtrl().InsertBand(-1, &rbi))
	{
		TRACE0("Failed to create rebar\n");
		return -1;      // fail to create
	}

	if (!m_wndDialogBar.Create(this, IDD_MAINFRAME, CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY, IDD_MAINFRAME)) {
		TRACE0("Failed to create dialog bar\n");
		return -1;      // fail to create
	}
/*
	rbi.cbSize		= sizeof(REBARBANDINFO);
	rbi.fMask		= RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_TEXT | RBBIM_COLORS;
	rbi.fStyle		= RBBS_NOGRIPPER | RBBS_BREAK;
	rbi.cxMinChild	= 300;
	rbi.cyMinChild	= 300;
	rbi.lpText		= "";
	rbi.cch			= 7;
	rbi.cx			= 10;
	rbi.hwndChild	= m_wndDialogBar;//(HWND)m_wndToolBar;
	rbi.clrFore		= rbi.clrBack = 0xFFFF;
	
	m_wndToolBarReBar.GetReBarCtrl().InsertBand(1, &rbi);
*/
	if (!m_wndStatusBar.Create(this) || !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT))) {
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	if (!m_wndPatternWindow.CreateEx(WS_EX_STATICEDGE, NULL, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL /*| WS_DLGFRAME*/, CRect(12, 12, 162, 173), (CWnd*)&m_wndDialogBar, 0)) {
		TRACE0("Failed to create pattern window\n");
		return -1;      // fail to create
	}

	
	if (!CreateInstrumentToolbar()) {
		TRACE0("Failed to create instrument toolbar\n");
		return -1;      // fail to create
	}

	/*
	// TODO: Delete these three lines if you don't want the toolbar to be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
	*/

	if (!CreateSampleWindow()) {
		TRACE0("Failed to create sample window\n");
		return -1;      // fail to create
	}

	InstrumentList = reinterpret_cast<CListCtrl*>(m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS));
	InstrumentList->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	SetupColors();

	m_pImageList = new CImageList();
	m_pImageList->Create(16, 16, ILC_COLOR, 1, 1);
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_2A03INV));

	InstrumentList->SetImageList(m_pImageList, LVSIL_NORMAL);
	InstrumentList->SetImageList(m_pImageList, LVSIL_SMALL);

//	InstrumentList->Set

	SetTimer(0, 100, 0);

//	MoveWindow(theApp.m_pSettings->WindowPos.iLeft, theApp.m_pSettings->WindowPos.iTop,
//		theApp.m_pSettings->WindowPos.iRight, theApp.m_pSettings->WindowPos.iBottom);

//	GetMainWnd()->SetWindowPos(NULL, 
//	m_pSettings->SetWindowPos(WinRect.left, WinRect.top, WinRect.right, WinRect.bottom);

	//m_wndDialogBar.ShowWindow(SW_HIDE);

	m_bInitialized = true;

	return 0;
}

bool CMainFrame::CreateSampleWindow()
{
	if (!m_SampleWindow.CreateEx(WS_EX_CLIENTEDGE, NULL, "", WS_CHILD | WS_VISIBLE, CRect(297, 115, 297 + CSampleWindow::WIN_WIDTH, 115 + CSampleWindow::WIN_HEIGHT), (CWnd*)&m_wndDialogBar, 0))
		return false;

	m_SampleProc.Wnd = &m_SampleWindow;

	if (!m_SampleProc.CreateThread())
		return false;

	return true;
}

bool CMainFrame::CreateInstrumentToolbar()
{
	const TBBUTTON buttons[] = {
		{0, ID_MODULE_ADDINSTRUMENT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, ID_MODULE_ADDINSTRUMENT},
		{1, ID_MODULE_REMOVEINSTRUMENT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, ID_MODULE_REMOVEINSTRUMENT},
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, -1},
		{2, ID_MODULE_LOADINSTRUMENT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, ID_MODULE_LOADINSTRUMENT},
		{3, ID_MODULE_SAVEINSTRUMENT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, ID_MODULE_SAVEINSTRUMENT},
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, -1},
		{4, IDC_EDIT_INST, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, IDC_EDIT_INST}};

	REBARBANDINFO rbi;

	if (!m_wndInstToolBarWnd.CreateEx(0, NULL, "", WS_CHILD | WS_VISIBLE, CRect(450, 173, 583, 199), (CWnd*)&m_wndDialogBar, 0))
		return false;

	if (!m_wndInstToolBar.Create(WS_CHILD | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT, CRect(0, 0, 0, 0), &m_wndInstToolBarWnd, 0))
		return false;

	m_wndInstToolBar.AddString(ID_MODULE_ADDINSTRUMENT);
	m_wndInstToolBar.AddString(ID_MODULE_REMOVEINSTRUMENT);
	m_wndInstToolBar.AddString(ID_MODULE_LOADINSTRUMENT);
	m_wndInstToolBar.AddString(ID_MODULE_SAVEINSTRUMENT);
	m_wndInstToolBar.AddString(IDC_EDIT_INST);
	m_wndInstToolBar.AddBitmap(5, IDB_INSTRUMENT_TOOLS);
	m_wndInstToolBar.AddButtons(7, (LPTBBUTTON)&buttons);

	// Route messages to this window
	m_wndInstToolBar.SetOwner(this);

	m_wndInstToolBar.SetAnchorHighlight(0);

	m_wndInstToolReBar.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), &m_wndInstToolBarWnd, AFX_IDW_REBAR);

	rbi.cbSize		= sizeof(REBARBANDINFO);
	rbi.fMask		= RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_TEXT /*| RBBIM_COLORS*/;
	rbi.fStyle		= RBBS_GRIPPERALWAYS;
	rbi.cxMinChild	= 300;
	rbi.cyMinChild	= 30;
	rbi.lpText		= "";
	rbi.cch			= 7;
	rbi.cx			= 300;
	rbi.hwndChild	= m_wndInstToolBar;//(HWND)m_wndToolBar;
//	rbi.clrFore		= rbi.clrBack = 0xFFFF;

	m_wndInstToolReBar.InsertBand(-1, &rbi);

	return true;
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
	CComBSTR ErrorMsg;
	CString Text;

	if (Index == -1) {
		// Limit
		ErrorMsg.LoadString(IDS_INST_LIMIT);
		AfxMessageBox(CW2CT(ErrorMsg), MB_ICONERROR, 0);
		return;
	}

	Text.Format("%02X - %s", Index, Name);

	if (Text.GetLength() > 30) {
		Text.Format("%s...", Text.GetBufferSetLength(30));
	}
	
	InstrumentList = (CListCtrl*)m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS);
	InstrumentList->InsertItem(Index, Text);
}

void CMainFrame::RemoveInstrument(int Index) 
{
	InstrumentList = (CListCtrl*)m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS);

	Index = InstrumentList->GetSelectionMark();

	if (InstrumentList->GetItemCount() == -1)
		return;

	InstrumentList->DeleteItem(Index);
	InstrumentList->SetSelectionMark(-1);
}

void CMainFrame::SetIndicatorTime(int Min, int Sec, int MSec)
{
	static int LMin, LSec, LMSec;
	CString String;

	if (Min != LMin || Sec != LSec || MSec != LMSec) {
		LMin = Min;
		LSec = Sec;
		LMSec = MSec;
		String.Format("%02i:%02i:%01i0", Min, Sec, MSec);
		m_wndStatusBar.SetPaneText(5, String);
		m_SampleWindow.SetTime(Min, Sec, MSec);
	}
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

	if (m_bInitialized == false)
		return;
	
	m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS)->MoveWindow(450, 14, cx - 460, 154);
	m_wndDialogBar.GetDlgItem(IDC_INSTNAME)->MoveWindow(590, 175, cx - 600, 22);
//	m_wndDialogBar.GetDlgItem(IDC_EDIT_INST)->MoveWindow(cx - 120, 175, 110, 22);
//	m_wndDialogBar.GetDlgItem(IDC_VOLUME)->MoveWindow(cx - 120, 6, 110, 18);
}

void CMainFrame::OnClickInstruments(NMHDR *pNotifyStruct, LRESULT *result)
{
	int Instrument = 0;
	char Text[256];

	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	
	InstrumentList = static_cast<CListCtrl*>(m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS));

	if (InstrumentList->GetSelectionMark() == -1)
		return;

	InstrumentList->GetItemText(InstrumentList->GetSelectionMark(), 0, Text, 256);
	sscanf(Text, "%X", &Instrument);

	pDoc->GetInstrumentName(Instrument, Text);
	pView->SetInstrument(Instrument);

	m_wndDialogBar.GetDlgItem(IDC_INSTNAME)->SetWindowText(Text);

	if (m_InstEdit.m_bOpened)
		m_InstEdit.SetCurrentInstrument(Instrument);

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

	Instrument = pView->GetInstrument();

	((CEdit*)m_wndDialogBar.GetDlgItem(IDC_INSTNAME))->GetWindowText(Text, 256);

	// Doesn't need to be longer than 60 chars
	Text[60] = 0;

	sprintf(Name, "%02X - %s", Instrument, Text);
	InstrumentList->SetItemText(InstrumentList->GetSelectionMark(), 0, Name);
	pDoc->GetInstrumentName(Instrument, Name);
	pDoc->SetInstrumentName(Instrument, Text);
}

void CMainFrame::OnAddInstrument()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	AddInstrument(pDoc->AddInstrument(NEW_INST_NAME), NEW_INST_NAME);
}

void CMainFrame::OnRemoveInstrument()
{
	CFamiTrackerView *pView = (CFamiTrackerView*)GetActiveView();
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();

	int Instrument;

	InstrumentList = (CListCtrl*)m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS);
	Instrument = pView->GetInstrument();

	if (InstrumentList->GetSelectionMark() == -1)
		return;

	pDoc->RemoveInstrument(Instrument);
	RemoveInstrument(Instrument);
	pView->SetInstrument(0);
}

void CMainFrame::OnBnClickedEditInst()
{
	OpenInstrumentSettings();
}

void CMainFrame::OnLoadInstrument()
{
	// Loads an instrument from a file

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CFamiTrackerView *pView = (CFamiTrackerView*)GetActiveView();

	CFileDialog FileDialog(TRUE, 0, 0, OFN_HIDEREADONLY, "FamiTracker Instrument (*.fti)|*.fti|All files (*.*)|*.*|");

	FileDialog.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_FTI);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	unsigned int Index = pDoc->LoadInstrument(FileDialog.GetFileName());
	char Name[256];

	if (Index == -1)
		return;

	pDoc->GetInstrumentName(Index, Name);

	AddInstrument(Index, Name);

	theApp.m_pSettings->SetPath(FileDialog.GetPathName(), PATH_FTI);
}

void CMainFrame::OnSaveInstrument()
{
	// Saves an instrument to a file

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CFamiTrackerView *pView = (CFamiTrackerView*)GetActiveView();

	char Name[256];
	CString String;

	int Index;
	InstrumentList = (CListCtrl*)m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS);

	Index = InstrumentList->GetSelectionMark();

	if (InstrumentList->GetItemCount() == -1 || Index == -1)
		return;

	pDoc->GetInstrumentName(Index, Name);
	strcat(Name, ".fti");

	// Remove bad characters
	char *ptr = Name;

	while (*ptr != 0) {
		if (*ptr == '/')
			*ptr = ' ';
		ptr++;
	}

	CFileDialog FileDialog(FALSE, 0, Name, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "FamiTracker Instrument (*.fti)|*.fti|All files (*.*)|*.*|");

	FileDialog.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_FTI);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	pDoc->SaveInstrument(pView->GetInstrument(), FileDialog.GetFileName());

	theApp.m_pSettings->SetPath(FileDialog.GetPathName(), PATH_FTI);
}

void CMainFrame::OnEnFramesChange()
{
	int Index;
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();

	if (!m_bInitialized || !pDoc)
		return;

	Index = m_wndDialogBar.GetDlgItemInt(IDC_FRAMES, 0, 0);

	LIMIT(Index, MAX_FRAMES, 1);

	if (pDoc->IsFileLoaded())
		pDoc->SetFrameCount(Index);
}

void CMainFrame::OnDeltaposTempoSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	SetTempo(m_wndDialogBar.GetDlgItemInt(IDC_SPEED) - ((NMUPDOWN*)pNMHDR)->iDelta);
}

void CMainFrame::OnEnTempoChange()
{
	CFamiTrackerView *pView = (CFamiTrackerView*)GetActiveView();
	ASSERT_VALID(pView);
	SetTempo(m_wndDialogBar.GetDlgItemInt(IDC_SPEED, 0, 0));
}

void CMainFrame::SetTempo(unsigned int Value)
{
	LIMIT(Value, MAX_TEMPO, MIN_TEMPO);
	static_cast<CFamiTrackerDoc*>(GetDocument())->SetSongSpeed(Value);
	SoundGenerator.ResetTempo();
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

void CMainFrame::OnDeltaposFrameSpin(NMHDR *pNMHDR, LRESULT *pResult)
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

	if (pDoc->IsFileLoaded())
		pDoc->SetPatternLength(Index);

	GetActiveView()->RedrawWindow();
}

void CMainFrame::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: Add your message handler code here
	// Do not call CFrameWnd::OnPaint() for painting messages

	m_wndPatternWindow.RedrawWindow();	
}

void CMainFrame::RefreshPattern()
{
	if (m_bInitialized)
		m_wndPatternWindow.RedrawWindow();
}

void CMainFrame::OnBnClickedIncFrame()
{
	static_cast<CFamiTrackerView*>(GetActiveView())->IncreaseCurrentPattern();
}

void CMainFrame::OnBnClickedDecFrame()
{
	static_cast<CFamiTrackerView*>(GetActiveView())->DecreaseCurrentPattern();
}

void CMainFrame::OnKeyRepeat()
{
	theApp.m_pSettings->General.bKeyRepeat = (m_wndDialogBar.IsDlgButtonChecked(IDC_KEYREPEAT) == 1);
}

void CMainFrame::OnDeltaposKeyStepSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int Pos= m_wndDialogBar.GetDlgItemInt(IDC_KEYSTEP) - ((NMUPDOWN*)pNMHDR)->iDelta;
	LIMIT(Pos, MAX_PATTERN_LENGTH, 0);
	m_wndDialogBar.SetDlgItemInt(IDC_KEYSTEP, Pos);
}

void CMainFrame::OnEnKeyStepChange()
{
	int Step = m_wndDialogBar.GetDlgItemInt(IDC_KEYSTEP);
	LIMIT(Step, MAX_PATTERN_LENGTH, 0);
	static_cast<CFamiTrackerView*>(GetActiveView())->SetStepping(Step);
}

void CMainFrame::OnCreateNSF()
{
	CNSFDialog NSFDialog;
	NSFDialog.DoModal();
}

BOOL CMainFrame::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle , const RECT& rect , CWnd* pParentWnd , LPCTSTR lpszMenuName , DWORD dwExStyle , CCreateContext* pContext)
{
	RECT newrect;

	// Load stored position
	newrect.bottom	= theApp.m_pSettings->WindowPos.iBottom;
	newrect.left	= theApp.m_pSettings->WindowPos.iLeft;
	newrect.right	= theApp.m_pSettings->WindowPos.iRight;
	newrect.top		= theApp.m_pSettings->WindowPos.iTop;
	
	if ((dwStyle & WS_MAXIMIZE) == 0) {
		if (theApp.m_pSettings->WindowPos.iState == STATE_MAXIMIZED)
			dwStyle |= WS_MAXIMIZE;
	}

	// Resize the window after startup
	/*
	newrect.top		= 100;
	newrect.left	= 100;
	newrect.right	= newrect.left + 850;
	newrect.bottom	= newrect.top + 820;
	*/
	return CFrameWnd::Create(lpszClassName, lpszWindowName, dwStyle, newrect, pParentWnd, lpszMenuName, dwExStyle, pContext);
}

void CMainFrame::OnNextFrame()
{
	static_cast<CFamiTrackerView*>(GetActiveView())->SelectNextFrame();
}

void CMainFrame::OnPrevFrame()
{
	static_cast<CFamiTrackerView*>(GetActiveView())->SelectPrevFrame();
}

void CMainFrame::OnChangeAll()
{	
	static_cast<CFamiTrackerView*>(GetActiveView())->SetChangeAllPattern(m_wndDialogBar.IsDlgButtonChecked(IDC_CHANGE_ALL) != 0);
}

void CMainFrame::DrawSamples(int *Samples, int Count)
{
	if (!m_bInitialized)
		return;

	m_SampleProc.PostThreadMessage(WM_USER, (WPARAM)Samples, Count);
}

void CMainFrame::OnHelpPerformance()
{
	m_PerformanceDlg.Create(MAKEINTRESOURCE(IDD_PERFORMANCE), this);
	m_PerformanceDlg.ShowWindow(SW_SHOW);
}

void CMainFrame::OnUpdateSBInstrument(CCmdUI *pCmdUI)
{
	CString String;
	String.Format("Instrument: %02i", GET_VIEW->GetInstrument());

	pCmdUI->Enable(); 
	pCmdUI->SetText(String);
}

void CMainFrame::OnUpdateSBOctave(CCmdUI *pCmdUI)
{
	CString String;
	String.Format("Octave: %i", GET_VIEW->GetOctave());

	pCmdUI->Enable(); 
	pCmdUI->SetText(String);
}

void CMainFrame::OnUpdateSBFrequency(CCmdUI *pCmdUI)
{
	int Machine = GET_DOC->GetMachine();
	int EngineSpeed = GET_DOC->GetEngineSpeed();
	CString String;

	if (EngineSpeed == 0)
		EngineSpeed = (Machine == NTSC ? 60 : 50);

	String.Format("%i Hz", EngineSpeed);

	pCmdUI->Enable(); 
	pCmdUI->SetText(String);
}

void CMainFrame::OnUpdateSBTempo(CCmdUI *pCmdUI)
{
	CString String;
	String.Format("%i BPM", theApp.GetTempo());

	pCmdUI->Enable(); 
	pCmdUI->SetText(String);
}

void CMainFrame::OnTimer(UINT nIDEvent)
{
	// Welcome message
	CComBSTR String;
	String.LoadString(IDS_WELCOME);
	SetMessageText(CW2CT(String));
	KillTimer(0);
	CFrameWnd::OnTimer(nIDEvent);
}

void CMainFrame::OnUpdateKeyStepEdit(CCmdUI *pCmdUI)
{
	CString Text;
	Text.Format("%i", GET_VIEW->GetStepping());
	pCmdUI->SetText(Text);
}

void CMainFrame::OnUpdateSpeedEdit(CCmdUI *pCmdUI)
{
	CString Text;
	Text.Format("%i", GET_DOC->GetSongSpeed());
	pCmdUI->SetText(Text);
}

void CMainFrame::OnUpdateRowsEdit(CCmdUI *pCmdUI)
{
	CString Text;
	Text.Format("%i", GET_DOC->GetPatternLength());
	pCmdUI->SetText(Text);
}

void CMainFrame::OnUpdateFramesEdit(CCmdUI *pCmdUI)
{
	CString Text;
	Text.Format("%i", GET_DOC->GetFrameCount());
	pCmdUI->SetText(Text);
}

void CMainFrame::OnFileGeneralsettings()
{
	CComBSTR Title;
	Title.LoadString(IDS_CONFIG_WINDOW);
	
	CConfigWindow ConfigWindow(CW2CT(Title), this, 0);

	CConfigGeneral		TabGeneral;
	CConfigAppearance	TabAppearance;
	CConfigMIDI			TabMIDI;
	CConfigSound		TabSound;

	TabGeneral.m_psp.dwFlags	&= ~PSP_HASHELP;
	TabAppearance.m_psp.dwFlags &= ~PSP_HASHELP;

	ConfigWindow.AddPage((CPropertyPage*)&TabGeneral);
	ConfigWindow.AddPage((CPropertyPage*)&TabAppearance);
	ConfigWindow.AddPage((CPropertyPage*)&TabMIDI);
	ConfigWindow.AddPage((CPropertyPage*)&TabSound);

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
	m_wndDialogBar.GetDlgItemText(IDC_SONG_NAME, ((CFamiTrackerDoc*)GetActiveDocument())->GetSongName(), 32);
}

void CMainFrame::OnEnSongArtistChange()
{
	m_wndDialogBar.GetDlgItemText(IDC_SONG_ARTIST, ((CFamiTrackerDoc*)GetActiveDocument())->GetSongArtist(), 32);
}

void CMainFrame::OnEnSongCopyrightChange()
{
	m_wndDialogBar.GetDlgItemText(IDC_SONG_COPYRIGHT, ((CFamiTrackerDoc*)GetActiveDocument())->GetSongCopyright(), 32);
}

void CMainFrame::ChangeNoteState(int Note)
{
	m_InstEdit.ChangeNoteState(Note);
}

void CMainFrame::OpenInstrumentSettings()
{
	CFamiTrackerDoc		*pDoc	= (CFamiTrackerDoc*)GetActiveDocument();
	CFamiTrackerView	*pView	= (CFamiTrackerView*)GetActiveView();

	if (pDoc->IsInstrumentUsed(pView->GetInstrument())) {
		if (m_InstEdit.m_bOpened == false) {
			m_InstEdit.Create(IDD_INSTRUMENT, this);
			m_InstEdit.SetCurrentInstrument(pView->GetInstrument());
			m_InstEdit.UpdateWindow();
			m_InstEdit.ShowWindow(SW_SHOW);
		}
		else
			m_InstEdit.SetCurrentInstrument(pView->GetInstrument());
	}
}

void CMainFrame::CloseInstrumentSettings()
{
	m_InstEdit.DestroyWindow();
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
	CFileDialog FileDialog(TRUE, 0, 0, OFN_HIDEREADONLY, "MIDI files (*.mid)|*.mid|All files|*.*||");

	if (GetActiveDocument()->SaveModified() == 0)
		return;

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

BOOL CMainFrame::DestroyWindow()
{
	// Store window position

	CRect WinRect;
	int State = STATE_NORMAL;

	GetWindowRect(WinRect);

	if (IsZoomed())
		State = STATE_MAXIMIZED;

	if (IsIconic()) {
		WinRect.top = WinRect.left = 100;
		WinRect.bottom = 920;
		WinRect.right = 950;
	}

	theApp.m_pSettings->SetWindowPos(WinRect.left, WinRect.top, WinRect.right, WinRect.bottom, State);

	return CFrameWnd::DestroyWindow();
}

BOOL CMainFrame::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

void CMainFrame::OnModuleModuleproperties()
{
	CModulePropertiesDlg PropertiesDlg;

	PropertiesDlg.DoModal();
}

void CMainFrame::UpdateTrackBox()
{
	CComboBox		*TrackBox	= (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_SUBTUNE);
	CFamiTrackerDoc	*pDoc		= (CFamiTrackerDoc*)GetActiveDocument();
	CString			Text;

	ASSERT(TrackBox != 0);

	TrackBox->ResetContent();

	for (unsigned int i = 0; i < (pDoc->GetTrackCount() + 1); i++) {
		Text.Format("Song #%i", i + 1);
		TrackBox->AddString(Text);
	}

	TrackBox->SetCurSel(pDoc->GetSelectedTrack());
}

void CMainFrame::OnCbnSelchangeSong()
{
	CComboBox		*TrackBox	= (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_SUBTUNE);
	CFamiTrackerDoc	*pDoc		= (CFamiTrackerDoc*)GetActiveDocument();
	
	int Song = TrackBox->GetCurSel();

	pDoc->SelectTrack(Song);
}
