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
** Any permitted reproduction of these routin, in whole or in part,
** must bear this legend.
*/

#include "stdafx.h"

#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"

#include "ExportDialog.h"
#include "CreateWaveDlg.h"
#include "InstrumentEditDlg.h"
#include "ModulePropertiesDlg.h"

#include "InstrumentEditor2A03.h"
#include "InstrumentEditorDPCM.h"
#include "MidiImport.h"

#include "ConfigGeneral.h"
#include "ConfigAppearance.h"
#include "ConfigMIDI.h"
#include "ConfigSound.h"
#include "ConfigShortcuts.h"
#include "ConfigWindow.h"

#include "Settings.h"
#include "Accelerator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// these are not beautiful
#define GET_VIEW	((CFamiTrackerView*)GetActiveView())
#define GET_DOC		((CFamiTrackerDoc*)GetDocument())

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	ON_WM_KILLFOCUS()
	// Global help commands
	ON_COMMAND(ID_FILE_GENERALSETTINGS, OnFileGeneralsettings)
	ON_COMMAND(ID_FILE_IMPORTMIDI, OnFileImportmidi)
	ON_COMMAND(ID_FILE_CREATE_NSF, OnCreateNSF)
	ON_COMMAND(ID_FILE_CREATEWAV, OnCreateWAV)
	ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP_PERFORMANCE, OnHelpPerformance)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_TRACKER_KILLSOUND, OnTrackerKillsound)
	ON_COMMAND(ID_NEXT_FRAME, OnNextFrame)
	ON_COMMAND(ID_PREV_FRAME, OnPrevFrame)
	ON_COMMAND(IDC_KEYREPEAT, OnKeyRepeat)
	ON_COMMAND(IDC_CHANGE_ALL, OnChangeAll)
	ON_COMMAND(ID_MODULE_ADDINSTRUMENT, OnAddInstrument)
	ON_COMMAND(ID_MODULE_REMOVEINSTRUMENT, OnRemoveInstrument)
	ON_COMMAND(ID_MODULE_SAVEINSTRUMENT, OnSaveInstrument)
	ON_COMMAND(ID_MODULE_LOADINSTRUMENT, OnLoadInstrument)
	ON_COMMAND(ID_MODULE_EDITINSTRUMENT, OnEditInstrument)
	ON_COMMAND(ID_MODULE_MODULEPROPERTIES, OnModuleModuleproperties)
	ON_COMMAND(ID_TRACKER_SWITCHTOTRACKINSTRUMENT, OnTrackerSwitchToInstrument)
	ON_COMMAND(ID_TRACKER_DPCM, OnTrackerDPCM)
	// Edit
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)		// these are shared with frame editor
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)		// 
	ON_BN_CLICKED(IDC_FRAME_INC, OnBnClickedIncFrame)
	ON_BN_CLICKED(IDC_FRAME_DEC, OnBnClickedDecFrame)
//	ON_BN_CLICKED(IDC_EDIT_INST, OnBnClickedEditInst)
	ON_BN_CLICKED(IDC_FOLLOW, OnClickedFollow)
	ON_NOTIFY(NM_CLICK, IDC_INSTRUMENTS, OnClickInstruments)
	//ON_NOTIFY(NM_RCLICK, IDC_INSTRUMENTS, OnRClickInstruments)
	ON_NOTIFY(NM_DBLCLK, IDC_INSTRUMENTS, OnDblClkInstruments)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPEED_SPIN, OnDeltaposSpeedSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_TEMPO_SPIN, OnDeltaposTempoSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_ROWS_SPIN, OnDeltaposRowsSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_FRAME_SPIN, OnDeltaposFrameSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_KEYSTEP_SPIN, OnDeltaposKeyStepSpin)
	ON_EN_CHANGE(IDC_INSTNAME, OnInstNameChange)
	ON_EN_CHANGE(IDC_KEYSTEP, OnEnKeyStepChange)
	ON_EN_CHANGE(IDC_SONG_NAME, OnEnSongNameChange)
	ON_EN_CHANGE(IDC_SONG_ARTIST, OnEnSongArtistChange)
	ON_EN_CHANGE(IDC_SONG_COPYRIGHT, OnEnSongCopyrightChange)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_INSTRUMENT, OnUpdateSBInstrument)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_OCTAVE, OnUpdateSBOctave)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_RATE, OnUpdateSBFrequency)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_TEMPO, OnUpdateSBTempo)
	ON_UPDATE_COMMAND_UI(IDC_KEYSTEP, OnUpdateKeyStepEdit)
	ON_UPDATE_COMMAND_UI(IDC_KEYREPEAT, OnUpdateKeyRepeat)
	ON_UPDATE_COMMAND_UI(IDC_SPEED, OnUpdateSpeedEdit)
	ON_UPDATE_COMMAND_UI(IDC_TEMPO, OnUpdateTempoEdit)
	ON_UPDATE_COMMAND_UI(IDC_ROWS, OnUpdateRowsEdit)
	ON_UPDATE_COMMAND_UI(IDC_FRAMES, OnUpdateFramesEdit)
	ON_UPDATE_COMMAND_UI(ID_NEXT_SONG, OnUpdateNextSong)
	ON_UPDATE_COMMAND_UI(ID_PREV_SONG, OnUpdatePrevSong)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_SWITCHTOTRACKINSTRUMENT, OnUpdateTrackerSwitchToInstrument)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CONTROLPANEL, OnUpdateViewControlpanel)
	ON_UPDATE_COMMAND_UI(IDC_HIGHLIGHT1, OnUpdateHighlight)
	ON_UPDATE_COMMAND_UI(IDC_HIGHLIGHT2, OnUpdateHighlight)
	ON_EN_SETFOCUS(IDC_KEYREPEAT, OnRemoveFocus)
	ON_EN_KILLFOCUS(IDC_SONG_NAME, OnEnKillfocusSongName)
	ON_EN_KILLFOCUS(IDC_SONG_ARTIST, OnEnKillfocusSongArtist)
	ON_EN_KILLFOCUS(IDC_SONG_COPYRIGHT, OnEnKillfocusSongCopyright)
	ON_CBN_SELCHANGE(IDC_SUBTUNE, OnCbnSelchangeSong)
	ON_CBN_SELCHANGE(IDC_OCTAVE, OnCbnSelchangeOctave)
	ON_COMMAND(ID_NEXT_SONG, OnNextSong)
	ON_COMMAND(ID_PREV_SONG, OnPrevSong)
	ON_COMMAND(IDC_FOLLOW_TOGGLE, OnToggleFollow)
	ON_COMMAND(ID_VIEW_CONTROLPANEL, OnViewControlpanel)
	ON_COMMAND(ID_EDIT_CLEARPATTERNS, OnClearPatterns)
	ON_COMMAND(ID_MODULE_DUPLICATEFRAME, OnDuplicateFrame)
	ON_WM_SHOWWINDOW()
	ON_COMMAND(ID_FOCUS_PATTERN_EDITOR, OnSelectPatternEditor)
	ON_COMMAND(ID_FOCUS_FRAME_EDITOR, OnSelectFrameEditor)
	ON_COMMAND(ID_INSTRUMENT_ADD, OnAddInstrument)
	ON_COMMAND(ID_INSTRUMENT_REMOVE, OnRemoveInstrument)
	ON_COMMAND(ID_INSTRUMENT_EDIT, OnEditInstrument)
	ON_COMMAND(ID_INSTRUMENT_CLONE, OnCloneInstrument)
	ON_COMMAND(ID_HELP_EFFECTTABLE, &CMainFrame::OnHelpEffecttable)
	ON_COMMAND(ID_INSTRUMENT_CLONE33018, OnCloneInstrument)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CHIP,
	ID_INDICATOR_INSTRUMENT, 
	ID_INDICATOR_OCTAVE,
	ID_INDICATOR_RATE,
	ID_INDICATOR_TEMPO,
	ID_INDICATOR_TIME,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

const char CMainFrame::NEW_INST_NAME[] = "New instrument";

bool m_bInitialShow = true;

const int DEFAULT_DPI = 96;
int _dpiX, _dpiY;

// DPI scaling functions
int SX(int pt)
{
	return MulDiv(pt, _dpiX, DEFAULT_DPI);
}

int SY(int pt)
{
	return MulDiv(pt, _dpiY, DEFAULT_DPI);
}

void ScaleMouse(CPoint &pt)
{
	pt.x = MulDiv(pt.x, DEFAULT_DPI, _dpiX);
	pt.y = MulDiv(pt.y, DEFAULT_DPI, _dpiY);
}

// CMainFrame construction/destruction

CMainFrame::CMainFrame() : 
	m_pSampleWindow(NULL), 
	m_pSampleProc(NULL), 
	m_bInitialized(false),
	m_iDPIX(DEFAULT_DPI),
	m_iDPIY(DEFAULT_DPI)
{
	_dpiX = DEFAULT_DPI;
	_dpiY = DEFAULT_DPI;
}

CMainFrame::~CMainFrame()
{
	if (m_pImageList)
		delete m_pImageList;

	if (m_pCustomEditSpeed)
		delete m_pCustomEditSpeed;

	if (m_pCustomEditTempo)
		delete m_pCustomEditTempo;

	if (m_pCustomEditLength)
		delete m_pCustomEditLength;
	
	if (m_pCustomEditFrames)
		delete m_pCustomEditFrames;

	if (m_pCustomEditStep)
		delete m_pCustomEditStep;

}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	CDC *pDC = GetDC();

	// Get the DPI setting
	if (pDC) {
		_dpiX = pDC->GetDeviceCaps(LOGPIXELSX);
		_dpiY = pDC->GetDeviceCaps(LOGPIXELSY);
		ReleaseDC(pDC);
	}

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!CreateToolbars())
		return -1;

	if (!CreateDialogPanels())
		return -1;

	if (!m_wndStatusBar.Create(this) || !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT))) {
		TRACE0("Failed to create status bar\n");
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
	m_pImageList->Create(16, 16, ILC_COLOR32, 1, 1);
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_2A03INV));
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_VRC6INV));
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_VRC7INV));
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_FDS));
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_N106));
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_5B));

	InstrumentList->SetImageList(m_pImageList, LVSIL_NORMAL);
	InstrumentList->SetImageList(m_pImageList, LVSIL_SMALL);

	SetTimer(0, 100, 0);

	m_wndOctaveBar.CheckDlgButton(IDC_FOLLOW, TRUE);
	m_wndOctaveBar.SetDlgItemInt(IDC_HIGHLIGHT1, 4, 0);
	m_wndOctaveBar.SetDlgItemInt(IDC_HIGHLIGHT2, 16, 0);

	m_iInstrumentIcons[INST_2A03] = 0;
	m_iInstrumentIcons[INST_VRC6] = 1;
	m_iInstrumentIcons[INST_VRC7] = 2;
	m_iInstrumentIcons[INST_FDS] = 3;
	//m_iInstrumentIcons[INST_MMC5] = 0;
	m_iInstrumentIcons[INST_N106] = 4;
	m_iInstrumentIcons[INST_5B] = 5;

	m_bInitialized = true;

	return 0;
}

bool CMainFrame::CreateToolbars()
{
	REBARBANDINFO rbi1;

	// Add the toolbar
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))  {
		TRACE0("Failed to create toolbar\n");
		return false;      // fail to create
	}

	m_wndToolBar.SetBarStyle(CBRS_ALIGN_TOP | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

	if (!m_wndOctaveBar.Create(this, (UINT)IDD_OCTAVE, CBRS_TOOLTIPS | CBRS_FLYBY, IDD_OCTAVE)) {
		TRACE0("Failed to create octave bar\n");
		return false;      // fail to create
	}

	if (!m_wndToolBarReBar.Create(this)) {
		TRACE0("Failed to create rebar\n");
		return false;      // fail to create
	}

	rbi1.cbSize		= sizeof(REBARBANDINFO);
	rbi1.fMask		= RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_SIZE;
	rbi1.fStyle		= RBBS_NOGRIPPER;
	rbi1.hwndChild	= m_wndToolBar;
	rbi1.cxMinChild	= SX(525);
	rbi1.cyMinChild	= SY(22);
	rbi1.cx			= SX(480);

	if (!m_wndToolBarReBar.GetReBarCtrl().InsertBand(-1, &rbi1)) {
		TRACE0("Failed to create rebar\n");
		return false;      // fail to create
	}

	rbi1.cbSize		= sizeof(REBARBANDINFO);
	rbi1.fMask		= RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_SIZE;
	rbi1.fStyle		= RBBS_NOGRIPPER;
	rbi1.hwndChild	= m_wndOctaveBar;
	rbi1.cxMinChild	= SX(120);
	rbi1.cyMinChild	= SY(22);
	rbi1.cx			= SX(100);

	if (!m_wndToolBarReBar.GetReBarCtrl().InsertBand(-1, &rbi1)) {
		TRACE0("Failed to create rebar\n");
		return false;      // fail to create
	}

	m_wndToolBarReBar.GetReBarCtrl().MinimizeBand(0);

	HBITMAP hbm = (HBITMAP)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_TOOLBAR_256), IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION /*| LR_LOADMAP3DCOLORS*/);
	m_bmToolbar.Attach(hbm); 
	
	m_ilToolBar.Create(16, 15, ILC_COLOR8 | ILC_MASK, 4, 4);
	m_ilToolBar.Add(&m_bmToolbar, RGB(192, 192, 192));
	m_wndToolBar.GetToolBarCtrl().SetImageList(&m_ilToolBar);

	return true;
}

// Todo: fix
CDialogBar m_wndInstrumentBar;
CDialogBar m_wndFrameBar;
bool m_bDisplayFrameBar = /*true*/ false;

CInstrumentList *m_pInstrumentList;

bool CMainFrame::CreateDialogPanels()
{
	if (!m_wndControlBar.Create(this, IDD_MAINBAR, CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY, IDD_MAINBAR)) {
		TRACE0("Failed to create frame main bar\n");
		return false;
	}
	
	if (!m_wndDialogBar.Create(IDD_MAINFRAME, &m_wndControlBar)) {
		TRACE0("Failed to create dialog bar\n");
		return false;
	}

	m_wndDialogBar.ShowWindow(SW_SHOW);

	m_pFrameWindow = new CFrameBoxWnd(this);

	if (!m_pFrameWindow->CreateEx(WS_EX_STATICEDGE, NULL, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL /*| WS_DLGFRAME*/, CRect(12, 12, 162, 173), (CWnd*)&m_wndControlBar, 0)) {
		TRACE0("Failed to create pattern window\n");
		return false;
	}

	// Subclass edit boxes
	m_pCustomEditSpeed	= new CCustomEdit;
	m_pCustomEditTempo	= new CCustomEdit;
	m_pCustomEditLength = new CCustomEdit;
	m_pCustomEditFrames = new CCustomEdit;
	m_pCustomEditStep	= new CCustomEdit;

	m_pCustomEditSpeed->SubclassDlgItem(IDC_SPEED, &m_wndDialogBar);
	m_pCustomEditTempo->SubclassDlgItem(IDC_TEMPO, &m_wndDialogBar);
	m_pCustomEditLength->SubclassDlgItem(IDC_ROWS, &m_wndDialogBar);
	m_pCustomEditFrames->SubclassDlgItem(IDC_FRAMES, &m_wndDialogBar);
	m_pCustomEditStep->SubclassDlgItem(IDC_KEYSTEP, &m_wndDialogBar);

	// Subclass the instrument list
	m_pInstrumentList = new CInstrumentList(this);
	m_pInstrumentList->SubclassDlgItem(IDC_INSTRUMENTS, &m_wndDialogBar);

	//ResizeFrameWindow();

	// New instrument editor

#ifdef NEW_INSTRUMENTPANEL
/*
	if (!m_wndInstrumentBar.Create(this, IDD_INSTRUMENTPANEL, CBRS_RIGHT | CBRS_TOOLTIPS | CBRS_FLYBY, IDD_INSTRUMENTPANEL)) {
		TRACE0("Failed to create frame instrument bar\n");
	}

	m_wndInstrumentBar.ShowWindow(SW_SHOW);
*/
#endif

	// Frame bar
/*
	if (!m_wndFrameBar.Create(this, IDD_FRAMEBAR, CBRS_LEFT | CBRS_TOOLTIPS | CBRS_FLYBY, IDD_FRAMEBAR)) {
		TRACE0("Failed to create frame bar\n");
	}
	
	m_wndFrameBar.ShowWindow(SW_SHOW);
*/

	return true;
}

bool CMainFrame::CreateSampleWindow()
{
	m_pSampleWindow = new CSampleWindow();
	m_pSampleProc = new CSampleWinProc();

	if (!m_pSampleWindow->CreateEx(WS_EX_CLIENTEDGE, NULL, "", WS_CHILD | WS_VISIBLE, CRect(SX(137), SY(115), SX(137) + CSampleWindow::WIN_WIDTH, SX(115) + CSampleWindow::WIN_HEIGHT), (CWnd*)&m_wndDialogBar, 0))
		return false;

	m_pSampleProc->Wnd = m_pSampleWindow;

	if (!m_pSampleProc->CreateThread())
		return false;

	return true;
}

bool CMainFrame::CreateInstrumentToolbar()
{
	const TBBUTTON buttons[] = {
		{0, ID_MODULE_ADDINSTRUMENT,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, (BYTE)ID_MODULE_ADDINSTRUMENT},
		{1, ID_MODULE_REMOVEINSTRUMENT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, (BYTE)ID_MODULE_REMOVEINSTRUMENT},
		{2, ID_INSTRUMENT_CLONE,		TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, (BYTE)ID_INSTRUMENT_CLONE},
		{0, 0,							TBSTATE_ENABLED, TBSTYLE_SEP,	 0, (BYTE)-1},
		{3, ID_MODULE_LOADINSTRUMENT,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, (BYTE)ID_MODULE_LOADINSTRUMENT},
		{4, ID_MODULE_SAVEINSTRUMENT,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, (BYTE)ID_MODULE_SAVEINSTRUMENT},
		{0, 0,							TBSTATE_ENABLED, TBSTYLE_SEP,	 0, (BYTE)-1},
		{5, ID_MODULE_EDITINSTRUMENT,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, (BYTE)ID_MODULE_EDITINSTRUMENT}};
		
	REBARBANDINFO rbi;

	if (!m_wndInstToolBarWnd.CreateEx(0, NULL, "", WS_CHILD | WS_VISIBLE, CRect(SX(288), SY(173), SX(442), SY(199)), (CWnd*)&m_wndDialogBar, 0))
		return false;

	if (!m_wndInstToolBar.Create(WS_CHILD | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT, CRect(0, 0, 0, 0), &m_wndInstToolBarWnd, 0))
		return false;

	HBITMAP hbm = (HBITMAP)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_TOOLBAR_INST_256), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION /*| LR_LOADMAP3DCOLORS*/);
	m_bmInstToolbar.Attach(hbm); 

	m_ilInstToolBar.Create(16, 16, ILC_COLOR24 | ILC_MASK, 4, 4);
	m_ilInstToolBar.Add(&m_bmInstToolbar, RGB(255, 0, 255));
	m_wndInstToolBar.SetImageList(&m_ilInstToolBar);

//	m_wndInstToolBar.AddBitmap(6, IDB_TOOLBAR_INST_256);
	m_wndInstToolBar.AddButtons(8, (LPTBBUTTON)&buttons);

	// Route messages to this window
	m_wndInstToolBar.SetOwner(this);
	m_wndInstToolBar.SetAnchorHighlight(0);
	
	m_wndInstToolReBar.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), &m_wndInstToolBarWnd, AFX_IDW_REBAR);

	rbi.cbSize		= sizeof(REBARBANDINFO);
	rbi.fMask		= RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_TEXT;
	rbi.fStyle		= RBBS_GRIPPERALWAYS;
	rbi.cxMinChild	= SX(300);
	rbi.cyMinChild	= SY(30);
	rbi.lpText		= "";
	rbi.cch			= 7;
	rbi.cx			= SX(300);
	rbi.hwndChild	= m_wndInstToolBar;

	m_wndInstToolReBar.InsertBand(-1, &rbi);

	return true;
}

void CMainFrame::ResizeFrameWindow()
{
	// Called when the number of channels has changed
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();

	if (pDoc) {
		int Channels = pDoc->GetAvailableChannels();
		m_pFrameWindow->MoveWindow(SX(12), SY(12), SX(51 + CFrameBoxWnd::FRAME_ITEM_WIDTH * Channels), SY(161));

		switch (pDoc->GetExpansionChip()) {
			case SNDCHIP_NONE:
				m_wndStatusBar.SetPaneText(1, "No expansion chip");
				break;
			case SNDCHIP_VRC6:
				m_wndStatusBar.SetPaneText(1, "Konami VRC6");
				break;
			case SNDCHIP_MMC5:
				m_wndStatusBar.SetPaneText(1, "Nintendo MMC5");
				break;
			case SNDCHIP_FDS:
				m_wndStatusBar.SetPaneText(1, "Nintendo FDS");
				break;
			case SNDCHIP_VRC7:
				m_wndStatusBar.SetPaneText(1, "Konami VRC7");
				break;
			case SNDCHIP_N106:
				m_wndStatusBar.SetPaneText(1, "Namco N106");
				break;
			case SNDCHIP_5B:
				m_wndStatusBar.SetPaneText(1, "Sunsoft 5B");
				break;
		}
	}

//	GetDeviceCaps(

	CRect ChildRect, ParentRect, FramesRect;

	m_wndControlBar.GetClientRect(&ParentRect);
	m_pFrameWindow->GetClientRect(&FramesRect);

	int DialogStartPos = FramesRect.right + 32;

	m_wndDialogBar.MoveWindow(DialogStartPos, 2, ParentRect.Width() - DialogStartPos, ParentRect.Height() - 4);
	m_wndDialogBar.GetWindowRect(&ChildRect);
	m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS)->MoveWindow(SX(288), SY(14), ChildRect.Width() - SX(296), SY(154));
	m_wndDialogBar.GetDlgItem(IDC_INSTNAME)->MoveWindow(SX(444), SY(175), ChildRect.Width() - SX(455), SY(22));

	if (m_bDisplayFrameBar) {
//		m_wndFrameBar.MoveWindow(
	}
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

void CMainFrame::AddInstrument(int Index/*, const char *Name, int Type*/)
{
	// Adds a new instrument
	//
	// Index = instrument slot
	// Name = instrument name
	// Type = chip type

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CComBSTR ErrorMsg;
	CString Text;
	char Name[128];
	int Type;

	if (Index == -1) {
		// Limit
		ErrorMsg.LoadString(IDS_INST_LIMIT);
		AfxMessageBox(CW2CT(ErrorMsg), MB_ICONERROR, 0);
		return;
	}

	pDoc->GetInstrumentName(Index, Name);
	Type = pDoc->GetInstrument(Index)->GetType();

	Text.Format("%02X - %s", Index, Name);

	if (Text.GetLength() > 30) {
		Text.Format("%s...", Text.GetBufferSetLength(30));
	}

	InstrumentList = (CListCtrl*)m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS);
	InstrumentList->InsertItem(Index, Text, m_iInstrumentIcons[Type]);
}

void CMainFrame::RemoveInstrument(int Index) 
{
	InstrumentList = (CListCtrl*)m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS);

	//Index = InstrumentList->GetSelectionMark();

	if (InstrumentList->GetItemCount() == -1)
		return;

	InstrumentList->DeleteItem(Index);
	InstrumentList->SetSelectionMark(Index + 1);
}

void CMainFrame::UpdateInstrumentList()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();

	ClearInstrumentList();

	for (int i = 0; i < MAX_INSTRUMENTS; i++) {
		if (pDoc->IsInstrumentUsed(i)) {
			AddInstrument(i);
		}
	}
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
		m_wndStatusBar.SetPaneText(6, String);
	}
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

	if (!m_bInitialized)
		return;

	m_wndToolBarReBar.GetReBarCtrl().MinimizeBand(0);

	ResizeFrameWindow();
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

	if (m_InstEdit.IsOpened())
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
	CFamiTrackerView *pView = (CFamiTrackerView*)GetActiveView();
	int ChipType = pView->GetSelectedChipType();
	int Index = pDoc->AddInstrument(NEW_INST_NAME, ChipType);
	AddInstrument(Index/*, NEW_INST_NAME, ChipType + 1*/);
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

	int Index = InstrumentList->GetSelectionMark();

	RemoveInstrument(Index);

	if (Index == 0 || !pDoc->IsInstrumentUsed(Index - 1)) {
		pView->SetInstrument(0);
	}
	else {
		pView->SetInstrument(Index - 1);
		InstrumentList->SetSelectionMark(Index - 1);
	}
}

void CMainFrame::OnCloneInstrument() 
{
	CFamiTrackerView *pView = (CFamiTrackerView*)GetActiveView();
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	pDoc->CloneInstrument(pView->GetInstrument());
}

void CMainFrame::OnBnClickedEditInst()
{
	OpenInstrumentSettings();
}

void CMainFrame::OnEditInstrument()
{
	OpenInstrumentSettings();
}

void CMainFrame::OnLoadInstrument()
{
	// Loads an instrument from a file

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CFamiTrackerView *pView = (CFamiTrackerView*)GetActiveView();
	CFileDialog FileDialog(TRUE, "fti", 0, OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT, "FamiTracker Instrument (*.fti)|*.fti|All files (*.*)|*.*|");

	FileDialog.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_FTI);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	unsigned int Index;
	POSITION pos (FileDialog.GetStartPosition());

	while(pos) {
		CString csFileName(FileDialog.GetNextPathName(pos));

		Index = pDoc->LoadInstrument(csFileName);
		if (Index == -1)
			return;
		AddInstrument(Index);
	}

/*
	unsigned int Index = pDoc->LoadInstrument(FileDialog.GetPathName());
	char Name[256];

	if (Index == -1)
		return;

	pDoc->GetInstrumentName(Index, Name);

	AddInstrument(Index);
*/
	theApp.m_pSettings->SetPath(FileDialog.GetPathName(), PATH_FTI);
	
}

void CMainFrame::OnSaveInstrument()
{
	// Saves instrument to a file

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

	CFileDialog FileDialog(FALSE, "fti", Name, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "FamiTracker Instrument (*.fti)|*.fti|All files (*.*)|*.*|");

	FileDialog.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_FTI);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	pDoc->SaveInstrument(pView->GetInstrument(), FileDialog.GetPathName());

	theApp.m_pSettings->SetPath(FileDialog.GetPathName(), PATH_FTI);
}

void CMainFrame::OnDeltaposSpeedSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int Speed = m_wndDialogBar.GetDlgItemInt(IDC_SPEED, 0, 0);
	SetSpeed(Speed - ((NMUPDOWN*)pNMHDR)->iDelta);
}

void CMainFrame::OnDeltaposTempoSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int Tempo = m_wndDialogBar.GetDlgItemInt(IDC_TEMPO, 0, 0);
	SetTempo(Tempo - ((NMUPDOWN*)pNMHDR)->iDelta);
}

void CMainFrame::OnDeltaposRowsSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int Rows = m_wndDialogBar.GetDlgItemInt(IDC_ROWS);
	SetRowCount(Rows - ((NMUPDOWN*)pNMHDR)->iDelta);
}

void CMainFrame::OnDeltaposFrameSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int Frames = m_wndDialogBar.GetDlgItemInt(IDC_FRAMES);
	SetFrameCount(Frames - ((NMUPDOWN*)pNMHDR)->iDelta);
}

void CMainFrame::SetTempo(int Tempo)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetDocument());
	if (Tempo > 255) Tempo = 255;
	if (Tempo < 20) Tempo = 20;
	pDoc->SetSongTempo(Tempo);
	theApp.ResetTempo();
}

void CMainFrame::SetSpeed(int Speed)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetDocument());
	if (Speed > 19) Speed = 19;
	if (Speed < 1) Speed = 1;
	pDoc->SetSongSpeed(Speed);
	theApp.ResetTempo();
}

void CMainFrame::SetRowCount(int Count)
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();

	if (!m_bInitialized || !pDoc)
		return;
	
	LIMIT(Count, MAX_PATTERN_LENGTH, 1);

	if (pDoc->IsFileLoaded())
		pDoc->SetPatternLength(Count);

	GetActiveView()->RedrawWindow();
}

void CMainFrame::SetFrameCount(int Count)
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();

	if (!m_bInitialized || !pDoc)
		return;

	LIMIT(Count, MAX_FRAMES, 1);

	if (pDoc->IsFileLoaded())
		pDoc->SetFrameCount(Count);
}

void CMainFrame::OnTrackerKillsound()
{
	theApp.LoadSoundConfig();
	theApp.SilentEverything();
}

void CMainFrame::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	m_pFrameWindow->RedrawWindow();	
}

void CMainFrame::DrawFrameWindow()
{
	if (m_bInitialized)
		m_pFrameWindow->RedrawWindow();
}

void CMainFrame::OnBnClickedIncFrame()
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());
	pView->IncreaseCurrentPattern();
	pView->SetFocus();
}

void CMainFrame::OnBnClickedDecFrame()
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());
	pView->DecreaseCurrentPattern();
	pView->SetFocus();
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
	CExportDialog ExportDialog;
	ExportDialog.DoModal();
}

void CMainFrame::OnCreateWAV()
{
	CCreateWaveDlg WaveDialog;
	WaveDialog.ShowDialog();
}

BOOL CMainFrame::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle , const RECT& rect , CWnd* pParentWnd , LPCTSTR lpszMenuName , DWORD dwExStyle , CCreateContext* pContext)
{
	RECT newrect;

	CSettings *pSettings = theApp.m_pSettings;

	// Load stored position
	newrect.bottom	= pSettings->WindowPos.iBottom;
	newrect.left	= pSettings->WindowPos.iLeft;
	newrect.right	= pSettings->WindowPos.iRight;
	newrect.top		= pSettings->WindowPos.iTop;

/*
	if ((dwStyle & WS_MAXIMIZE) == 0) {
		if (theApp.m_pSettings->WindowPos.iState == STATE_MAXIMIZED)
			dwStyle |= WS_MAXIMIZE;
	}
*/

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
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());
	bool Enabled = m_wndControlBar.IsDlgButtonChecked(IDC_CHANGE_ALL) != 0;
	pView->SetChangeAllPattern(Enabled);
}

void CMainFrame::DrawSamples(int *Samples, int Count)
{
	if (!m_bInitialized)
		return;

	m_pSampleProc->PostThreadMessage(WM_USER, (WPARAM)Samples, Count);
}

void CMainFrame::OnHelpPerformance()
{
	m_PerformanceDlg.Create(MAKEINTRESOURCE(IDD_PERFORMANCE), this);
	m_PerformanceDlg.ShowWindow(SW_SHOW);
}

void CMainFrame::OnUpdateSBInstrument(CCmdUI *pCmdUI)
{
	CString String;
	String.Format("Instrument: %02X", GET_VIEW->GetInstrument());

	UpdateInstrumentIndex();

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

	if (!m_pCustomEditSpeed->IsEditable()) {
		if (m_pCustomEditSpeed->Update())
			SetSpeed(m_pCustomEditSpeed->GetValue());
		else {
			Text.Format("%i", GET_DOC->GetSongSpeed());
			pCmdUI->SetText(Text);
		}
	}	
}

void CMainFrame::OnUpdateTempoEdit(CCmdUI *pCmdUI)
{
	CString Text;

	if (!m_pCustomEditTempo->IsEditable()) {
		if (m_pCustomEditTempo->Update())
			SetTempo(m_pCustomEditTempo->GetValue());
		else {
			Text.Format("%i", GET_DOC->GetSongTempo());
			pCmdUI->SetText(Text);
		}
	}	
}

void CMainFrame::OnUpdateRowsEdit(CCmdUI *pCmdUI)
{
	CString Text;

	if (!m_pCustomEditLength->IsEditable()) {
		if (m_pCustomEditLength->Update())
			SetRowCount(m_pCustomEditLength->GetValue());
		else {
			Text.Format("%i", GET_DOC->GetPatternLength());
			pCmdUI->SetText(Text);
		}
	}	
}

void CMainFrame::OnUpdateFramesEdit(CCmdUI *pCmdUI)
{
	CString Text;

	if (!m_pCustomEditFrames->IsEditable()) {
		if (m_pCustomEditFrames->Update())
			SetFrameCount(m_pCustomEditFrames->GetValue());
		else {
			Text.Format("%i", GET_DOC->GetFrameCount());
			pCmdUI->SetText(Text);
		}
	}	
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
	CConfigShortcuts	TabShortcuts;

	ConfigWindow.m_psh.dwFlags	&= ~PSH_HASHELP;
	TabGeneral.m_psp.dwFlags	&= ~PSP_HASHELP;
	TabAppearance.m_psp.dwFlags &= ~PSP_HASHELP;
	TabMIDI.m_psp.dwFlags		&= ~PSP_HASHELP;
	TabSound.m_psp.dwFlags		&= ~PSP_HASHELP;
	TabShortcuts.m_psp.dwFlags	&= ~PSP_HASHELP;
	
	ConfigWindow.AddPage((CPropertyPage*)&TabGeneral);
	ConfigWindow.AddPage((CPropertyPage*)&TabAppearance);
	ConfigWindow.AddPage((CPropertyPage*)&TabMIDI);
	ConfigWindow.AddPage((CPropertyPage*)&TabSound);
	ConfigWindow.AddPage((CPropertyPage*)&TabShortcuts);

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
	char Text[64], *Name;
	CFamiTrackerDoc* pDoc = ((CFamiTrackerDoc*)GetActiveDocument());

	m_wndDialogBar.GetDlgItemText(IDC_SONG_NAME, Text, 32);
	Name = pDoc->GetSongName();

	if (strcmp(Name, Text) != 0) {
		strcpy(Name, Text);
		pDoc->SetModifiedFlag();
	}
}

void CMainFrame::OnEnSongArtistChange()
{
	char Text[64], *Artist;
	CFamiTrackerDoc* pDoc = ((CFamiTrackerDoc*)GetActiveDocument());

	m_wndDialogBar.GetDlgItemText(IDC_SONG_ARTIST, Text, 32);
	Artist = pDoc->GetSongArtist();

	if (strcmp(Artist, Text) != 0) {
		strcpy(Artist, Text);
		pDoc->SetModifiedFlag();
	}
}

void CMainFrame::OnEnSongCopyrightChange()
{
	char Text[64], *Copyright;
	CFamiTrackerDoc* pDoc = ((CFamiTrackerDoc*)GetActiveDocument());

	m_wndDialogBar.GetDlgItemText(IDC_SONG_COPYRIGHT, Text, 32);
	Copyright = pDoc->GetSongCopyright();

	if (strcmp(Copyright, Text) != 0) {
		strcpy(Copyright, Text);
		pDoc->SetModifiedFlag();
	}
}

void CMainFrame::ChangeNoteState(int Note)
{
	m_InstEdit.ChangeNoteState(Note);
}

void CMainFrame::OpenInstrumentSettings()
{
	CFamiTrackerDoc	*pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CFamiTrackerView *pView	= (CFamiTrackerView*)GetActiveView();

	if (pDoc->IsInstrumentUsed(pView->GetInstrument())) {
		if (m_InstEdit.IsOpened() == false) {
			m_InstEdit.Create(IDD_INSTRUMENT, this);
			m_InstEdit.SetCurrentInstrument(pView->GetInstrument());
			m_InstEdit.ShowWindow(SW_SHOW);
		}
		else
			m_InstEdit.SetCurrentInstrument(pView->GetInstrument());
		m_InstEdit.UpdateWindow();
	}
}

void CMainFrame::CloseInstrumentSettings()
{
	if (m_InstEdit.IsOpened())
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

	m_bInitialized = false;

	GetWindowRect(WinRect);

	if (IsZoomed()) {
		State = STATE_MAXIMIZED;
		// Ignore window position if maximized
		WinRect.top = theApp.m_pSettings->WindowPos.iTop;
		WinRect.bottom = theApp.m_pSettings->WindowPos.iBottom;
		WinRect.left = theApp.m_pSettings->WindowPos.iLeft;
		WinRect.right = theApp.m_pSettings->WindowPos.iRight;
	}

	if (IsIconic()) {
		WinRect.top = WinRect.left = 100;
		WinRect.bottom = 920;
		WinRect.right = 950;
	}

	theApp.m_pSettings->SetWindowPos(WinRect.left, WinRect.top, WinRect.right, WinRect.bottom, State);

	m_pFrameWindow->DestroyWindow();

	return CFrameWnd::DestroyWindow();
}

BOOL CMainFrame::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

void CMainFrame::OnTrackerSwitchToInstrument()
{
	CFamiTrackerView *pView	= (CFamiTrackerView*)GetActiveView();
	pView->SwitchToInstrument(!pView->SwitchToInstrument());
}

void CMainFrame::OnTrackerDPCM()
{
	CMenu *pMenu = GetMenu();

	if (pMenu->GetMenuState(ID_TRACKER_DPCM, MF_BYCOMMAND) == MF_CHECKED)
		pMenu->CheckMenuItem(ID_TRACKER_DPCM, MF_UNCHECKED);
	else
		pMenu->CheckMenuItem(ID_TRACKER_DPCM, MF_CHECKED);
}

void CMainFrame::OnUpdateTrackerSwitchToInstrument(CCmdUI *pCmdUI)
{
	CFamiTrackerView *pView	= (CFamiTrackerView*)GetActiveView();
	pCmdUI->SetCheck(pView->SwitchToInstrument() ? 1 : 0);
}

void CMainFrame::OnModuleModuleproperties()
{
	CModulePropertiesDlg PropertiesDlg;
	PropertiesDlg.DoModal();
	// Select first track to keep track box and document view in sync
	((CFamiTrackerDoc*)GetDocument())->SelectTrack(0);
	UpdateTrackBox();
}

void CMainFrame::UpdateTrackBox()
{
	CComboBox		*TrackBox	= (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_SUBTUNE);
	CFamiTrackerDoc	*pDoc		= (CFamiTrackerDoc*)GetActiveDocument();
	CString			Text;

	ASSERT(TrackBox != 0);
	ASSERT(pDoc != 0);

	TrackBox->ResetContent();

	for (unsigned int i = 0; i < (pDoc->GetTrackCount() + 1); i++) {
		Text.Format("#%i %s", i + 1, pDoc->GetTrackTitle(i));
		TrackBox->AddString(Text);
	}

	TrackBox->SetCurSel(pDoc->GetSelectedTrack());
}

void CMainFrame::OnCbnSelchangeSong()
{
	CComboBox *TrackBox	  = (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_SUBTUNE);
	CFamiTrackerDoc	*pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	pDoc->SelectTrack(TrackBox->GetCurSel());
}

void CMainFrame::OnCbnSelchangeOctave()
{
	CComboBox *TrackBox		= (CComboBox*)m_wndOctaveBar.GetDlgItem(IDC_OCTAVE);
	CFamiTrackerView *pView	= (CFamiTrackerView*)GetActiveView();
	unsigned int Octave		= TrackBox->GetCurSel();

	if (pView->GetOctave() != Octave)
		pView->SetOctave(Octave);
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_COMMAND) {
		m_pFrameWindow->SendMessage(WM_COMMAND, pMsg->wParam, pMsg->lParam);
	}
//	CAccelerator *pAccel = theApp.GetAccelerator();

	// Dynamic accelerator translation
//	if ((pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) /*&& (GetFocus() == (CWnd*)GetActiveView())*/ ) {
/*		// WM_SYSKEYDOWN is for ALT
		unsigned short ID = pAccel->GetAction((unsigned char)pMsg->wParam);
		if (ID != 0) {
//			if (!(ID == ID_TRACKER_EDIT && GetFocus() != (CWnd*)GetActiveView()))	// <- ugly fix, remove it in some way!
				SendMessage(WM_COMMAND, (0 << 16) | ID, NULL);
		}
	}
	else if (pMsg->message == WM_KEYUP || pMsg->message == WM_SYSKEYUP) {
		pAccel->KeyReleased((unsigned char)pMsg->wParam);
	}
*/
	return CFrameWnd::PreTranslateMessage(pMsg);
}

void CMainFrame::OnKillFocus(CWnd* pNewWnd)
{
	CFrameWnd::OnKillFocus(pNewWnd);
	CAccelerator *pAccel = theApp.GetAccelerator();
	pAccel->LostFocus();
}

void CMainFrame::DisplayOctave()
{
	CComboBox *pOctaveList  = (CComboBox*)m_wndOctaveBar.GetDlgItem(IDC_OCTAVE);
	CFamiTrackerView *pView	= (CFamiTrackerView*)GetActiveView();
	pOctaveList->SetCurSel(pView->GetOctave());
}

void CMainFrame::UpdateInstrumentIndex()
{
	CFamiTrackerView *pView	= (CFamiTrackerView*)GetActiveView();
	InstrumentList = reinterpret_cast<CListCtrl*>(m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS));
	LVFINDINFO info;
	char Txt[256];
	int Index;

	sprintf(Txt, "%02X", pView->GetInstrument());

	info.flags = LVFI_PARTIAL | LVFI_STRING;
	info.psz = Txt;

	Index = InstrumentList->FindItem(&info);
	InstrumentList->SetItemState(Index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	InstrumentList->EnsureVisible(Index, FALSE);
}

void CMainFrame::OnRemoveFocus()
{
	GetActiveView()->SetFocus();
}

void CMainFrame::OnNextSong()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CComboBox *TrackBox	  = (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_SUBTUNE);
	unsigned int CurrentSong = pDoc->GetSelectedTrack();
	if (CurrentSong < pDoc->GetTrackCount()) {
		pDoc->SelectTrack(CurrentSong + 1);
		TrackBox->SetCurSel(CurrentSong + 1);
	}
}

void CMainFrame::OnPrevSong()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CComboBox *TrackBox = (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_SUBTUNE);
	int CurrentSong = pDoc->GetSelectedTrack();
	if (CurrentSong > 0) {
		pDoc->SelectTrack(CurrentSong - 1);
		TrackBox->SetCurSel(CurrentSong - 1);
	}
}

void CMainFrame::OnUpdateNextSong(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	if (pDoc->GetSelectedTrack() < pDoc->GetTrackCount())
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdatePrevSong(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	if (pDoc->GetSelectedTrack() > 0)
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnClickedFollow()
{
	CFamiTrackerView *pView	= (CFamiTrackerView*)GetActiveView();
	pView->SetFollowMode(m_wndOctaveBar.IsDlgButtonChecked(IDC_FOLLOW) != 0);
	pView->SetFocus();
}

void CMainFrame::OnToggleFollow()
{
	CFamiTrackerView *pView	= (CFamiTrackerView*)GetActiveView();
	m_wndOctaveBar.CheckDlgButton(IDC_FOLLOW, !m_wndOctaveBar.IsDlgButtonChecked(IDC_FOLLOW));
	pView->SetFollowMode(m_wndOctaveBar.IsDlgButtonChecked(IDC_FOLLOW) != 0);
	pView->SetFocus();
}

int CMainFrame::GetHighlightRow()
{
	return m_wndOctaveBar.GetDlgItemInt(IDC_HIGHLIGHT1);
}

int CMainFrame::GetSecondHighlightRow()
{
	return m_wndOctaveBar.GetDlgItemInt(IDC_HIGHLIGHT2);
}


void CMainFrame::OnViewControlpanel()
{
	if (m_wndControlBar.IsVisible()) {
		m_wndControlBar.ShowWindow(SW_HIDE);
	}
	else {
		m_wndControlBar.ShowWindow(SW_SHOW);
		m_wndControlBar.UpdateWindow();
	}

	RecalcLayout();
}

void CMainFrame::OnUpdateViewControlpanel(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_wndControlBar.IsVisible());
}

void CMainFrame::OnUpdateHighlight(CCmdUI *pCmdUI)
{
	static int LastHighlight1, LastHighlight2;
	int Highlight1, Highlight2;
	Highlight1 = m_wndOctaveBar.GetDlgItemInt(IDC_HIGHLIGHT1);
	Highlight2 = m_wndOctaveBar.GetDlgItemInt(IDC_HIGHLIGHT2);
	if (Highlight1 != LastHighlight1 || Highlight2 != LastHighlight2) {
		GetActiveDocument()->UpdateAllViews(NULL, UPDATE_ENTIRE);
		LastHighlight1 = Highlight1;
		LastHighlight2 = Highlight2;
	}
}

void CMainFrame::OnClearPatterns()
{
	CComBSTR WarnMsg;
	WarnMsg.LoadString(IDS_CLEARPATTERN);
	if (MessageBox(CW2CT(WarnMsg), "Warning", MB_OKCANCEL | MB_ICONWARNING) == IDOK) {
		CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
		pDoc->ClearPatterns();
	}
}

void CMainFrame::UpdateControls()
{
	m_wndDialogBar.UpdateDialogControls(&m_wndDialogBar, TRUE);
}

void CMainFrame::OnDuplicateFrame()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CFamiTrackerView *pView	= (CFamiTrackerView*)GetActiveView();

	int i, x, frames = pDoc->GetFrameCount(), channels = pDoc->GetAvailableChannels();
	int Frame = pView->GetSelectedFrame();

	if (frames == MAX_FRAMES) {
		return;
	}

	pDoc->SetFrameCount(frames + 1);

	for (x = (frames + 1); x > (Frame + 1); x--) {
		for (i = 0; i < channels; i++) {
			pDoc->SetPatternAtFrame(x, i, pDoc->GetPatternAtFrame(x - 1, i));
		}
	}

	for (i = 0; i < channels; i++) {
		pDoc->SetPatternAtFrame(Frame + 1, i, pDoc->GetPatternAtFrame(Frame, i));
	}

	pDoc->UpdateAllViews(NULL, CHANGED_FRAMES);
}

void CMainFrame::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CFrameWnd::OnShowWindow(bShow, nStatus);

	if (m_bInitialShow) {
		if (theApp.m_pSettings->WindowPos.iState == STATE_MAXIMIZED)
			CFrameWnd::ShowWindow(SW_MAXIMIZE);
	}

	m_bInitialShow = false;
}

void CMainFrame::OnSelectPatternEditor()
{
	GetActiveView()->SetFocus();
}

void CMainFrame::OnSelectFrameEditor()
{
	m_pFrameWindow->EnableInput();
}

bool CMainFrame::HasFocus()
{
	//return GetFocus() == GetView();
	return true;
}

// UI updates

void CMainFrame::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	CFamiTrackerView *pView	= (CFamiTrackerView*)GetActiveView();
	pCmdUI->Enable((pView->IsSelecting() || m_pFrameWindow->InputEnabled()) ? 1 : 0);
}

void CMainFrame::OnUpdateEditPaste(CCmdUI *pCmdUI)
{
	CFamiTrackerView *pView	= (CFamiTrackerView*)GetActiveView();
	pCmdUI->Enable(pView->IsClipboardAvailable() ? 1 : 0);
}

///
/// CCustomEdit
///

// This class takes care of locking/unlocking edit boxes by double clicking

IMPLEMENT_DYNAMIC(CCustomEdit, CEdit)

BEGIN_MESSAGE_MAP(CCustomEdit, CEdit)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

bool CCustomEdit::IsEditable()
{
	return !((GetWindowLong(m_hWnd, GWL_STYLE) & ES_READONLY) == ES_READONLY);
}

bool CCustomEdit::Update()
{
	bool ret_val = m_bUpdate;
	m_bUpdate = false;
	return ret_val;
}

int CCustomEdit::GetValue()
{
	return m_iValue;
}

void CCustomEdit::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	m_bUpdate = false;
	if (IsEditable())
		SetSel(0, -1);	// select all
	else {
		SendMessage(EM_SETREADONLY, FALSE);
		SetFocus();
		SetSel(0, -1);
	}
}

void CCustomEdit::OnSetFocus(CWnd* pOldWnd)
{
	CEdit::OnSetFocus(pOldWnd);
	if (!IsEditable())
		theApp.GetDocumentView()->SetFocus();
}

void CCustomEdit::OnKillFocus(CWnd* pNewWnd)
{
	char Text[256];
	CEdit::OnKillFocus(pNewWnd);
	if (!IsEditable())
		return;
	GetWindowText(Text, 256);
	sscanf(Text, "%i", &m_iValue);
	m_bUpdate = true;
	SendMessage(EM_SETREADONLY, TRUE);
}

BOOL CCustomEdit::PreTranslateMessage(MSG* pMsg)
{
	// For some reason OnKeyDown won't work
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
		theApp.GetDocumentView()->SetFocus();
		return TRUE;
	}

	return CEdit::PreTranslateMessage(pMsg);
}



///
/// CInstrumentList
///

// This class takes care of the context menu message

IMPLEMENT_DYNAMIC(CInstrumentList, CListCtrl)

BEGIN_MESSAGE_MAP(CInstrumentList, CListCtrl)
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

CInstrumentList::CInstrumentList(CMainFrame *pMainFrame) : m_pMainFrame(pMainFrame)
{
}

void CInstrumentList::OnContextMenu(CWnd* pWnd, CPoint point)
{
	int Instrument = 0;
	char Text[256];

	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(m_pMainFrame->GetActiveView());
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(m_pMainFrame->GetActiveDocument());

	if (GetSelectionMark() != -1) {
		// Select the instrument
		GetItemText(GetSelectionMark(), 0, Text, 256);
		sscanf(Text, "%X", &Instrument);
		pDoc->GetInstrumentName(Instrument, Text);
		pView->SetInstrument(Instrument);
		// Todo: fix??
		//m_wndDialogBar.GetDlgItem(IDC_INSTNAME)->SetWindowText(Text);
	}

	// Display the popup menu
	CMenu *PopupMenu, PopupMenuBar;
	PopupMenuBar.LoadMenu(IDR_INSTRUMENT_POPUP);
	PopupMenu = PopupMenuBar.GetSubMenu(0);
	// Route the menu messages to mainframe
	PopupMenu->TrackPopupMenu(TPM_LEFTBUTTON, point.x, point.y, m_pMainFrame);

	// Return focus to pattern editor
	m_pMainFrame->GetActiveView()->SetFocus();
}

void CMainFrame::OnHelpEffecttable()
{
	HtmlHelp((DWORD)"effect_list.htm", HH_DISPLAY_TOPIC);
}
