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
#include "ChannelsDlg.h"
#include "SampleWindow.h"
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
#include "SoundGen.h"
#include "MIDI.h"
#include "TrackerChannel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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

// Timers
enum {TMR_WELCOME, TMR_SOUND_CHECK, TMR_AUTOSAVE};

// File filters (move to string table)
LPCTSTR FTI_FILTER_NAME = _T("FamiTracker Instrument (*.fti)");
LPCTSTR FTI_FILTER_EXT = _T(".fti");

// DPI variables
static const int DEFAULT_DPI = 96;
static int _dpiX, _dpiY;

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

// TODO: fix
CDialogBar m_wndInstrumentBar;
CDialogBar m_wndFrameBar;
bool m_bDisplayFrameBar = /*true*/ false;

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

// CMainFrame construction/destruction

CMainFrame::CMainFrame() : 
	m_pSampleWindow(NULL), 
	m_pFrameWindow(NULL),
	m_pImageList(NULL),
	m_pLockedEditSpeed(NULL),
	m_pLockedEditTempo(NULL),
	m_pLockedEditLength(NULL),
	m_pLockedEditFrames(NULL),
	m_pLockedEditStep(NULL),
	m_pBannerEditName(NULL),
	m_pBannerEditArtist(NULL),
	m_pBannerEditCopyright(NULL),
	m_pInstrumentList(NULL),
	m_iInstrument(0),
	m_iTrack(0)
{
	_dpiX = DEFAULT_DPI;
	_dpiY = DEFAULT_DPI;
}

CMainFrame::~CMainFrame()
{
	SAFE_RELEASE(m_pImageList);
	SAFE_RELEASE(m_pLockedEditSpeed);
	SAFE_RELEASE(m_pLockedEditTempo);
	SAFE_RELEASE(m_pLockedEditLength);
	SAFE_RELEASE(m_pLockedEditFrames);
	SAFE_RELEASE(m_pLockedEditStep);
	SAFE_RELEASE(m_pBannerEditName);
	SAFE_RELEASE(m_pBannerEditArtist);
	SAFE_RELEASE(m_pBannerEditCopyright);
	SAFE_RELEASE(m_pFrameWindow);
	SAFE_RELEASE(m_pInstrumentList);
	SAFE_RELEASE(m_pSampleWindow);
}

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	ON_WM_KILLFOCUS()
	ON_WM_SHOWWINDOW()
	ON_WM_DESTROY()
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
	ON_COMMAND(ID_MODULE_CHANNELS, OnModuleChannels)
	ON_COMMAND(ID_TRACKER_SWITCHTOTRACKINSTRUMENT, OnTrackerSwitchToInstrument)
	ON_COMMAND(ID_TRACKER_DPCM, OnTrackerDPCM)
	ON_COMMAND(ID_NEXT_SONG, OnNextSong)
	ON_COMMAND(ID_PREV_SONG, OnPrevSong)
	ON_COMMAND(IDC_FOLLOW_TOGGLE, OnToggleFollow)
	ON_COMMAND(ID_VIEW_CONTROLPANEL, OnViewControlpanel)
	ON_COMMAND(ID_EDIT_CLEARPATTERNS, OnClearPatterns)
	ON_COMMAND(ID_MODULE_DUPLICATEFRAME, OnDuplicateFrame)
	ON_COMMAND(ID_FOCUS_PATTERN_EDITOR, OnSelectPatternEditor)
	ON_COMMAND(ID_FOCUS_FRAME_EDITOR, OnSelectFrameEditor)
	ON_COMMAND(ID_INSTRUMENT_ADD, OnAddInstrument)
	ON_COMMAND(ID_INSTRUMENT_REMOVE, OnRemoveInstrument)
	ON_COMMAND(ID_INSTRUMENT_EDIT, OnEditInstrument)
	ON_COMMAND(ID_INSTRUMENT_CLONE, OnCloneInstrument)
	ON_COMMAND(ID_HELP_EFFECTTABLE, &CMainFrame::OnHelpEffecttable)
	ON_COMMAND(ID_INSTRUMENT_CLONE, OnCloneInstrument)
	ON_COMMAND(ID_EDIT_ENABLEMIDI, OnEditEnableMIDI)
	ON_COMMAND(ID_CMD_NEXT_INSTRUMENT, OnNextInstrument)
	ON_COMMAND(ID_CMD_PREV_INSTRUMENT, OnPrevInstrument)
	ON_BN_CLICKED(IDC_FRAME_INC, OnBnClickedIncFrame)
	ON_BN_CLICKED(IDC_FRAME_DEC, OnBnClickedDecFrame)
	ON_BN_CLICKED(IDC_FOLLOW, OnClickedFollow)
	ON_NOTIFY(NM_CLICK, IDC_INSTRUMENTS, OnClickInstruments)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_INSTRUMENTS, OnChangedInstruments)
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
	ON_EN_SETFOCUS(IDC_KEYREPEAT, OnRemoveFocus)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)		// these are shared with frame editor
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)		// 
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
	ON_UPDATE_COMMAND_UI(ID_EDIT_ENABLEMIDI, OnUpdateEditEnablemidi)
	ON_CBN_SELCHANGE(IDC_SUBTUNE, OnCbnSelchangeSong)
	ON_CBN_SELCHANGE(IDC_OCTAVE, OnCbnSelchangeOctave)
	ON_COMMAND(ID_INSTRUMENT_ADD_2A03, OnAddInstrument2A03)
	ON_COMMAND(ID_INSTRUMENT_ADD_VRC6, OnAddInstrumentVRC6)
	ON_COMMAND(ID_INSTRUMENT_ADD_VRC7, OnAddInstrumentVRC7)
	ON_COMMAND(ID_INSTRUMENT_ADD_FDS, OnAddInstrumentFDS)
	ON_COMMAND(ID_INSTRUMENT_ADD_MMC5, OnAddInstrumentMMC5)
	ON_COMMAND(ID_INSTRUMENT_ADD_N106, OnAddInstrumentN106)
	ON_COMMAND(ID_INSTRUMENT_ADD_S5B, OnAddInstrumentS5B)
	ON_WM_COPYDATA()
END_MESSAGE_MAP()

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

	// Initial message, 100ms
	SetTimer(TMR_WELCOME, 100, NULL);

	// Periodic synth check, 50ms
	SetTimer(TMR_SOUND_CHECK, 50, NULL);

	// Auto save
//	SetTimer(TMR_AUTOSAVE, 1000, NULL);

	m_wndOctaveBar.CheckDlgButton(IDC_FOLLOW, TRUE);
	m_wndOctaveBar.SetDlgItemInt(IDC_HIGHLIGHT1, 4, 0);
	m_wndOctaveBar.SetDlgItemInt(IDC_HIGHLIGHT2, 16, 0);

	// Icons for the instrument list
	m_iInstrumentIcons[INST_2A03] = 0;
	m_iInstrumentIcons[INST_VRC6] = 1;
	m_iInstrumentIcons[INST_VRC7] = 2;
	m_iInstrumentIcons[INST_FDS] = 3;
	//m_iInstrumentIcons[INST_MMC5] = 0;
	m_iInstrumentIcons[INST_N106] = 4;
	m_iInstrumentIcons[INST_S5B] = 5;

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
	rbi1.cxMinChild	= SX(554);
	rbi1.cyMinChild	= SY(22);
	rbi1.cx			= SX(496);

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

	if (!m_pFrameWindow->CreateEx(WS_EX_STATICEDGE, NULL, "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL, CRect(12, 12, 162, 173), (CWnd*)&m_wndControlBar, 0)) {
		TRACE0("Failed to create pattern window\n");
		return false;
	}

	// Subclass edit boxes
	m_pLockedEditSpeed	= new CLockedEdit();
	m_pLockedEditTempo	= new CLockedEdit();
	m_pLockedEditLength = new CLockedEdit();
	m_pLockedEditFrames = new CLockedEdit();
	m_pLockedEditStep	= new CLockedEdit();

	m_pLockedEditSpeed->SubclassDlgItem(IDC_SPEED, &m_wndDialogBar);
	m_pLockedEditTempo->SubclassDlgItem(IDC_TEMPO, &m_wndDialogBar);
	m_pLockedEditLength->SubclassDlgItem(IDC_ROWS, &m_wndDialogBar);
	m_pLockedEditFrames->SubclassDlgItem(IDC_FRAMES, &m_wndDialogBar);
	m_pLockedEditStep->SubclassDlgItem(IDC_KEYSTEP, &m_wndDialogBar);

	// Subclass and setup the instrument list
	m_pInstrumentList = new CInstrumentList(this);
	m_pInstrumentList->SubclassDlgItem(IDC_INSTRUMENTS, &m_wndDialogBar);

	SetupColors();

	m_pImageList = new CImageList();
	m_pImageList->Create(16, 16, ILC_COLOR32, 1, 1);
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_2A03INV));
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_VRC6INV));
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_VRC7INV));
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_FDS));
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_N106));
	m_pImageList->Add(theApp.LoadIcon(IDI_INST_5B));

	m_pInstrumentList->SetImageList(m_pImageList, LVSIL_NORMAL);
	m_pInstrumentList->SetImageList(m_pImageList, LVSIL_SMALL);

	m_pInstrumentList->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	// Title, author & copyright
	m_pBannerEditName = new CBannerEdit(CString(_T("(title)")));
	m_pBannerEditArtist = new CBannerEdit(CString(_T("(author)")));
	m_pBannerEditCopyright = new CBannerEdit(CString(_T("(copyright)")));

	m_pBannerEditName->SubclassDlgItem(IDC_SONG_NAME, &m_wndDialogBar);
	m_pBannerEditArtist->SubclassDlgItem(IDC_SONG_ARTIST, &m_wndDialogBar);
	m_pBannerEditCopyright->SubclassDlgItem(IDC_SONG_COPYRIGHT, &m_wndDialogBar);

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
	// Create the sample graph window
	m_pSampleWindow = new CSampleWindow();

	if (!m_pSampleWindow->CreateEx(WS_EX_CLIENTEDGE, NULL, _T("Samples"), WS_CHILD | WS_VISIBLE, CRect(SX(137), SY(115), SX(137) + CSampleWindow::WIN_WIDTH, SX(115) + CSampleWindow::WIN_HEIGHT), (CWnd*)&m_wndDialogBar, 0))
		return false;

	// Assign this to the sound generator
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();
	
	if (pSoundGen)
		pSoundGen->SetSampleWindow(m_pSampleWindow);

	return true;
}

bool CMainFrame::CreateInstrumentToolbar()
{
	// Setup the instrument toolbar
	REBARBANDINFO rbi;

	if (!m_wndInstToolBarWnd.CreateEx(0, NULL, _T(""), WS_CHILD | WS_VISIBLE, CRect(SX(288), SY(173), SX(456), SY(199)), (CWnd*)&m_wndDialogBar, 0))
		return false;

	if (!m_wndInstToolReBar.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), &m_wndInstToolBarWnd, AFX_IDW_REBAR))
		return false;

	if (!m_wndInstToolBar.CreateEx(&m_wndInstToolReBar, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) || !m_wndInstToolBar.LoadToolBar(IDR_INSTRUMENT_TOOLBAR))
		return false;

	m_wndInstToolBar.GetToolBarCtrl().SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);

	// Set 24-bit icons
	HBITMAP hbm = (HBITMAP)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_TOOLBAR_INST_256), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	m_bmInstToolbar.Attach(hbm);
	m_ilInstToolBar.Create(16, 16, ILC_COLOR24 | ILC_MASK, 4, 4);
	m_ilInstToolBar.Add(&m_bmInstToolbar, RGB(255, 0, 255));
	m_wndInstToolBar.GetToolBarCtrl().SetImageList(&m_ilInstToolBar);

	rbi.cbSize		= sizeof(REBARBANDINFO);
	rbi.fMask		= RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_TEXT;
	rbi.fStyle		= RBBS_NOGRIPPER;
	rbi.cxMinChild	= SX(300);
	rbi.cyMinChild	= SY(30);
	rbi.lpText		= "";
	rbi.cch			= 7;
	rbi.cx			= SX(300);
	rbi.hwndChild	= m_wndInstToolBar;

	m_wndInstToolReBar.InsertBand(-1, &rbi);

	// Route messages to this window
	m_wndInstToolBar.SetOwner(this);

	// Turn add new instrument button into a drop-down list
	m_wndInstToolBar.SetButtonStyle(0, TBBS_DROPDOWN);

	return true;
}

void CMainFrame::ResizeFrameWindow()
{
	// Called when the number of channels has changed
	if (!m_pFrameWindow)
		return;

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();

	if (pDoc) {
		int Channels = pDoc->GetAvailableChannels();
		int Chip = pDoc->GetExpansionChip();
		m_pFrameWindow->MoveWindow(SX(12), SY(12), SX(51 + CFrameBoxWnd::FRAME_ITEM_WIDTH * Channels), SY(161));

		switch (Chip) {
			case SNDCHIP_NONE:
				m_wndStatusBar.SetPaneText(1, _T("No expansion chip"));
				break;
			case SNDCHIP_VRC6:
				m_wndStatusBar.SetPaneText(1, _T("Konami VRC6"));
				break;
			case SNDCHIP_MMC5:
				m_wndStatusBar.SetPaneText(1, _T("Nintendo MMC5"));
				break;
			case SNDCHIP_FDS:
				m_wndStatusBar.SetPaneText(1, _T("Nintendo FDS"));
				break;
			case SNDCHIP_VRC7:
				m_wndStatusBar.SetPaneText(1, _T("Konami VRC7"));
				break;
			case SNDCHIP_N106:
				m_wndStatusBar.SetPaneText(1, _T("Namco N106"));
				break;
			case SNDCHIP_S5B:
				m_wndStatusBar.SetPaneText(1, _T("Sunsoft 5B"));
				break;
		}
	}

	CRect ChildRect, ParentRect, FramesRect;

	m_wndControlBar.GetClientRect(&ParentRect);
	m_pFrameWindow->GetClientRect(&FramesRect);

	int DialogStartPos = FramesRect.right + 32;

	m_wndDialogBar.MoveWindow(DialogStartPos, 2, ParentRect.Width() - DialogStartPos, ParentRect.Height() - 4);
	m_wndDialogBar.GetWindowRect(&ChildRect);
	m_wndDialogBar.GetDlgItem(IDC_INSTRUMENTS)->MoveWindow(SX(288), SY(10), ChildRect.Width() - SX(296), SY(158));
	m_wndDialogBar.GetDlgItem(IDC_INSTNAME)->MoveWindow(SX(458), SY(175), ChildRect.Width() - SX(466), SY(22));

	m_pFrameWindow->RedrawWindow();

	if (m_bDisplayFrameBar) {
//		m_wndFrameBar.MoveWindow(
	}
}

void CMainFrame::SetupColors(void)
{
	// Instrument list colors
	m_pInstrumentList->SetBkColor(theApp.GetSettings()->Appearance.iColBackground);
	m_pInstrumentList->SetTextBkColor(theApp.GetSettings()->Appearance.iColBackground);
	m_pInstrumentList->SetTextColor(theApp.GetSettings()->Appearance.iColPatternText);
	m_pInstrumentList->RedrawWindow();
}

void CMainFrame::SetTempo(int Tempo)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	LIMIT(Tempo, MAX_TEMPO, MIN_TEMPO);
	pDoc->SetSongTempo(Tempo);
	theApp.GetSoundGenerator()->ResetTempo();
}

void CMainFrame::SetSpeed(int Speed)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	LIMIT(Speed, MAX_SPEED, MIN_SPEED);
	pDoc->SetSongSpeed(Speed);
	theApp.GetSoundGenerator()->ResetTempo();
}

void CMainFrame::SetRowCount(int Count)
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();

	if (!pDoc)
		return;
	
	LIMIT(Count, MAX_PATTERN_LENGTH, 1);

	if (pDoc->IsFileLoaded())
		pDoc->SetPatternLength(Count);

	GetActiveView()->RedrawWindow();
}

void CMainFrame::SetFrameCount(int Count)
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();

	if (!pDoc)
		return;

	LIMIT(Count, MAX_FRAMES, 1);

	if (pDoc->IsFileLoaded())
		pDoc->SetFrameCount(Count);
}

void CMainFrame::UpdateControls()
{
	m_wndDialogBar.UpdateDialogControls(&m_wndDialogBar, TRUE);
}

int CMainFrame::GetHighlightRow() const
{
	return m_wndOctaveBar.GetDlgItemInt(IDC_HIGHLIGHT1);
}

int CMainFrame::GetSecondHighlightRow() const
{
	return m_wndOctaveBar.GetDlgItemInt(IDC_HIGHLIGHT2);
}

void CMainFrame::SetHighlightRow(int Rows)
{
	m_wndOctaveBar.SetDlgItemInt(IDC_HIGHLIGHT1, Rows);
}

void CMainFrame::SetSecondHighlightRow(int Rows)
{
	m_wndOctaveBar.SetDlgItemInt(IDC_HIGHLIGHT2, Rows);
}

void CMainFrame::DisplayOctave()
{
	CComboBox *pOctaveList = (CComboBox*)m_wndOctaveBar.GetDlgItem(IDC_OCTAVE);
	CFamiTrackerView *pView	= (CFamiTrackerView*)GetActiveView();
	pOctaveList->SetCurSel(pView->GetOctave());
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
	m_pInstrumentList->DeleteAllItems();
	m_wndDialogBar.GetDlgItem(IDC_INSTNAME)->SetWindowText(_T(""));
}

void CMainFrame::NewInstrument(int ChipType)
{
	// Add new instrument to module
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CFamiTrackerView *pView = (CFamiTrackerView*)GetActiveView();

	int Index = pDoc->AddInstrument(CFamiTrackerDoc::NEW_INST_NAME, ChipType);

	if (Index == -1) {
		// Out of slots
		AfxMessageBox(IDS_INST_LIMIT, MB_ICONERROR);
		return;
	}

	// Add to list
	AddInstrument(Index);

	// also select it
	SelectInstrument(Index);
}

void CMainFrame::AddInstrument(int Index)
{
	// Adds an instrument to the instrument list
	// instrument must exist in document.
	//
	// Index = instrument slot

	ASSERT(Index != -1);

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();

	char Name[256];
	pDoc->GetInstrumentName(Index, Name);
	int Type = pDoc->GetInstrument(Index)->GetType();

	// Name is of type index - name
	CString Text;
	Text.Format(_T("%02X - %s"), Index, A2T(Name));
	m_pInstrumentList->InsertItem(Index, Text, m_iInstrumentIcons[Type]);
}

void CMainFrame::RemoveInstrument(int Index) 
{
	// Remove instrument from instrument list
	ASSERT(Index != -1);

	if (m_wndInstEdit.IsOpened()) {
		m_wndInstEdit.DestroyWindow();
	}

	m_pInstrumentList->DeleteItem(FindInstrument(Index));
}

int CMainFrame::FindInstrument(int Index) const
{
	// Find an instrument from the instrument list
	CString Txt;
	Txt.Format(_T("%02X"), Index);

	LVFINDINFO info;
	info.flags = LVFI_PARTIAL | LVFI_STRING;
	info.psz = Txt;

	return m_pInstrumentList->FindItem(&info);
}

int CMainFrame::GetInstrumentIndex(int ListIndex) const
{
	// Convert instrument list index to instrument slot index
	int Instrument;
	TCHAR Text[256];
	m_pInstrumentList->GetItemText(ListIndex, 0, Text, 256);
	_stscanf(Text, _T("%X"), &Instrument);
	return Instrument;
}

void CMainFrame::UpdateInstrumentList()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();

	ClearInstrumentList();

	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (pDoc->IsInstrumentUsed(i)) {
			AddInstrument(i);
		}
	}
}

void CMainFrame::SelectInstrument(int Index)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());
	
	int ListCount = m_pInstrumentList->GetItemCount();

	// No instruments added
	if (ListCount == 0) {
		m_iInstrument = 0;
		m_wndDialogBar.GetDlgItem(IDC_INSTNAME)->SetWindowText(_T(""));
		return;
	}

	int LastInst = m_iInstrument;
	m_iInstrument = Index;

	if (pDoc->IsInstrumentUsed(Index)) {
		// Select instrument in list
		int ListIndex = FindInstrument(Index);
		int Selected = m_pInstrumentList->GetSelectionMark();

		if (ListIndex != Selected || LastInst != Index) {
			m_pInstrumentList->SetSelectionMark(ListIndex);
			m_pInstrumentList->SetItemState(ListIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
			m_pInstrumentList->EnsureVisible(ListIndex, FALSE);

			// Set instrument name (this will trigger the name change routine)
			char Text[256];
			pDoc->GetInstrumentName(Index, Text);
			m_wndDialogBar.GetDlgItem(IDC_INSTNAME)->SetWindowText(Text);

			// Update instrument editor
			if (m_wndInstEdit.IsOpened())
				m_wndInstEdit.SetCurrentInstrument(Index);
		}
	}
	else {
		// Remove selection
		m_pInstrumentList->SetSelectionMark(-1);
		m_pInstrumentList->SetItemState(LastInst, 0, LVIS_SELECTED | LVIS_FOCUSED);
		m_wndDialogBar.GetDlgItem(IDC_INSTNAME)->SetWindowText(_T(""));
		if (m_wndInstEdit.IsOpened())
			m_wndInstEdit.DestroyWindow();
	}
}

int CMainFrame::GetSelectedInstrument() const
{
	// Returns selected instrument
	return m_iInstrument;
}

void CMainFrame::OnNextInstrument()
{
	// Select next instrument in the list
	int SelIndex = m_pInstrumentList->GetSelectionMark();
	int Count = m_pInstrumentList->GetItemCount();
	if (SelIndex < (Count - 1)) {
		int Slot = GetInstrumentIndex(SelIndex + 1);
		SelectInstrument(Slot);
	}
}

void CMainFrame::OnPrevInstrument()
{
	// Select previous instrument in the list
	int SelIndex = m_pInstrumentList->GetSelectionMark();
	if (SelIndex > 0) {
		int Slot = GetInstrumentIndex(SelIndex - 1);
		SelectInstrument(Slot);
	}
}

void CMainFrame::SetIndicatorTime(int Min, int Sec, int MSec)
{
	static int LMin, LSec, LMSec;

	if (Min != LMin || Sec != LSec || MSec != LMSec) {
		LMin = Min;
		LSec = Sec;
		LMSec = MSec;
		CString String;
		String.Format(_T("%02i:%02i:%01i0"), Min, Sec, MSec);
		m_wndStatusBar.SetPaneText(6, String);
	}
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

	if (m_wndToolBarReBar.m_hWnd == NULL)
		return;

	m_wndToolBarReBar.GetReBarCtrl().MinimizeBand(0);

	ResizeFrameWindow();
}

void CMainFrame::OnClickInstruments(NMHDR *pNMHDR, LRESULT *result)
{
	GetActiveView()->SetFocus();
}

void CMainFrame::OnChangedInstruments(NMHDR* pNMHDR, LRESULT* pResult)
{
	// Change selected instrument
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(GetActiveDocument());

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if (!(pNMListView->uNewState & LVIS_SELECTED))
		return;

	int SelIndex = pNMListView->iItem; 

	if (SelIndex == -1)
		return;

	// Find selected instrument
	int Instrument = GetInstrumentIndex(SelIndex);
	SelectInstrument(Instrument);
}

void CMainFrame::OnDblClkInstruments(NMHDR *pNotifyStruct, LRESULT *result)
{
	OpenInstrumentSettings();
}

void CMainFrame::OnInstNameChange()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();

	int SelIndex = m_pInstrumentList->GetSelectionMark();

	if (SelIndex == -1 || m_pInstrumentList->GetItemCount() == 0)
		return;

	if (!pDoc->IsInstrumentUsed(m_iInstrument))
		return;

	TCHAR Text[256];
	m_wndDialogBar.GetDlgItem(IDC_INSTNAME)->GetWindowText(Text, 256);

	// Doesn't need to be longer than 60 chars
	Text[60] = 0;

	// Update instrument list & document
	CString Name;
	Name.Format(_T("%02X - %s"), m_iInstrument, Text);
	m_pInstrumentList->SetItemText(SelIndex, 0, Name);
	pDoc->SetInstrumentName(m_iInstrument, T2A(Text));
}

void CMainFrame::OnAddInstrument2A03()
{
	NewInstrument(SNDCHIP_NONE);
}

void CMainFrame::OnAddInstrumentVRC6()
{
	NewInstrument(SNDCHIP_VRC6);
}

void CMainFrame::OnAddInstrumentVRC7()
{
	NewInstrument(SNDCHIP_VRC7);
}

void CMainFrame::OnAddInstrumentFDS()
{
	NewInstrument(SNDCHIP_FDS);
}

void CMainFrame::OnAddInstrumentMMC5()
{
	NewInstrument(SNDCHIP_MMC5);
}

void CMainFrame::OnAddInstrumentN106()
{
	NewInstrument(SNDCHIP_N106);
}

void CMainFrame::OnAddInstrumentS5B()
{
	NewInstrument(SNDCHIP_S5B);
}

void CMainFrame::OnAddInstrument()
{
	// Add new instrument to module

	// Chip type depends on selected channel
	CFamiTrackerView *pView = ((CFamiTrackerView*)GetActiveView());
	int ChipType = pView->GetSelectedChipType();
	NewInstrument(ChipType);
}

void CMainFrame::OnRemoveInstrument()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();

	// No instruments in list
	if (m_pInstrumentList->GetItemCount() == 0)
		return;

	int Instrument = m_iInstrument;
	int SelIndex = m_pInstrumentList->GetSelectionMark();

	ASSERT(pDoc->IsInstrumentUsed(Instrument));

	// Remove from document
	pDoc->RemoveInstrument(Instrument);

	// Remove from list
	RemoveInstrument(Instrument);

	int Count = m_pInstrumentList->GetItemCount();

	// Select a new instrument
	if (Count == 0) {
		SelectInstrument(0);
	}
	else if (Count == SelIndex) {
		SelectInstrument(GetInstrumentIndex(SelIndex - 1));
	}
	else {
		SelectInstrument(GetInstrumentIndex(SelIndex));
	}
}

void CMainFrame::OnCloneInstrument() 
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	int Slot = pDoc->CloneInstrument(m_iInstrument);
	if (Slot == -1) {
		AfxMessageBox(IDS_INST_LIMIT, MB_ICONERROR);
		return;
	}
	AddInstrument(Slot);
	SelectInstrument(Slot);
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

	CString filter;
	CString allFilter;
	VERIFY(allFilter.LoadString(AFX_IDS_ALLFILTER));

	filter = FTI_FILTER_NAME;
	filter += _T("|*");
	filter += FTI_FILTER_EXT;
	filter += _T("|");
	filter += allFilter;
	filter += _T("|*.*||");

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CFamiTrackerView *pView = (CFamiTrackerView*)GetActiveView();
	CFileDialog FileDialog(TRUE, _T("fti"), 0, OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT, filter);

	FileDialog.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_FTI);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	POSITION pos (FileDialog.GetStartPosition());

	// Load multiple files
	while(pos) {
		CString csFileName(FileDialog.GetNextPathName(pos));
		int Index = pDoc->LoadInstrument(csFileName);
		if (Index == -1)
			return;
		AddInstrument(Index);
	}

	theApp.GetSettings()->SetPath(FileDialog.GetPathName(), PATH_FTI);
}

void CMainFrame::OnSaveInstrument()
{
	// Saves instrument to a file

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CFamiTrackerView *pView = (CFamiTrackerView*)GetActiveView();

	char Name[256];
	CString String;

	int Index = GetSelectedInstrument();

	if (Index == -1)
		return;

	if (!pDoc->IsInstrumentUsed(Index))
		return;

	pDoc->GetInstrumentName(Index, Name);

	// Remove bad characters
	char *ptr = Name;

	while (*ptr != 0) {
		if (*ptr == '/')
			*ptr = ' ';
		ptr++;
	}

	CString filter;
	CString allFilter;
	VERIFY(allFilter.LoadString(AFX_IDS_ALLFILTER));

	filter = FTI_FILTER_NAME;
	filter += _T("|*");
	filter += FTI_FILTER_EXT;
	filter += _T("|");
	filter += allFilter;
	filter += _T("|*.*||");

	CFileDialog FileDialog(FALSE, _T("fti"), Name, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter);

	FileDialog.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_FTI);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	pDoc->SaveInstrument(pView->GetInstrument(), FileDialog.GetPathName());

	theApp.GetSettings()->SetPath(FileDialog.GetPathName(), PATH_FTI);
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

void CMainFrame::OnTrackerKillsound()
{
	theApp.LoadSoundConfig();
	theApp.SilentEverything();
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
	theApp.GetSettings()->General.bKeyRepeat = (m_wndDialogBar.IsDlgButtonChecked(IDC_KEYREPEAT) == 1);
}

void CMainFrame::OnDeltaposKeyStepSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int Pos = m_wndDialogBar.GetDlgItemInt(IDC_KEYSTEP) - ((NMUPDOWN*)pNMHDR)->iDelta;
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
	CExportDialog ExportDialog(this);
	ExportDialog.DoModal();
}

void CMainFrame::OnCreateWAV()
{
	CCreateWaveDlg WaveDialog;
	WaveDialog.ShowDialog();
}

BOOL CMainFrame::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle , const RECT& rect , CWnd* pParentWnd , LPCTSTR lpszMenuName , DWORD dwExStyle , CCreateContext* pContext)
{
	CSettings *pSettings = theApp.GetSettings();
	RECT newrect;

	// Load stored position
	newrect.bottom	= pSettings->WindowPos.iBottom;
	newrect.left	= pSettings->WindowPos.iLeft;
	newrect.right	= pSettings->WindowPos.iRight;
	newrect.top		= pSettings->WindowPos.iTop;

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

void CMainFrame::OnHelpPerformance()
{
	m_wndPerformanceDlg.Create(MAKEINTRESOURCE(IDD_PERFORMANCE), this);
	m_wndPerformanceDlg.ShowWindow(SW_SHOW);
}

void CMainFrame::OnUpdateSBInstrument(CCmdUI *pCmdUI)
{
	CString String;
	String.Format(_T("Instrument: %02X"), ((CFamiTrackerView*)GetActiveView())->GetInstrument());
	pCmdUI->Enable(); 
	pCmdUI->SetText(String);
}

void CMainFrame::OnUpdateSBOctave(CCmdUI *pCmdUI)
{
	CString String;
	String.Format(_T("Octave: %i"), ((CFamiTrackerView*)GetActiveView())->GetOctave());
	pCmdUI->Enable(); 
	pCmdUI->SetText(String);
}

void CMainFrame::OnUpdateSBFrequency(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc *pDoc = ((CFamiTrackerDoc*)GetActiveDocument());
	int Machine = pDoc->GetMachine();
	int EngineSpeed = pDoc->GetEngineSpeed();
	CString String;

	if (EngineSpeed == 0)
		EngineSpeed = (Machine == NTSC ? CAPU::FRAME_RATE_NTSC : CAPU::FRAME_RATE_PAL);

	String.Format(_T("%i Hz"), EngineSpeed);

	pCmdUI->Enable(); 
	pCmdUI->SetText(String);
}

void CMainFrame::OnUpdateSBTempo(CCmdUI *pCmdUI)
{
	CString String;
	String.Format(_T("%i BPM"), theApp.GetSoundGenerator()->GetTempo());
	pCmdUI->Enable(); 
	pCmdUI->SetText(String);
}

void CMainFrame::OnTimer(UINT nIDEvent)
{
	CString text;
	switch (nIDEvent) {
		// Welcome message
		case TMR_WELCOME:
			text.Format(IDS_WELCOME_VER, VERSION_MAJ, VERSION_MIN, VERSION_REV);
			SetMessageText(text);
			KillTimer(0);
			break;
		// Check sound player
		case TMR_SOUND_CHECK:
			theApp.CheckSynth();
			break;
		// Auto save
		case TMR_AUTOSAVE: {
			/*
				CFamiTrackerDoc *pDoc = dynamic_cast<CFamiTrackerDoc*>(GetActiveDocument());
				if (pDoc != NULL)
					pDoc->AutoSave();
					*/
			}
			break;
	}

	CFrameWnd::OnTimer(nIDEvent);
}

void CMainFrame::OnUpdateKeyStepEdit(CCmdUI *pCmdUI)
{
	CString Text;
	Text.Format(_T("%i"), ((CFamiTrackerView*)GetActiveView())->GetStepping());
	pCmdUI->SetText(Text);
}

void CMainFrame::OnUpdateSpeedEdit(CCmdUI *pCmdUI)
{
	if (!m_pLockedEditSpeed->IsEditable()) {
		if (m_pLockedEditSpeed->Update())
			SetSpeed(m_pLockedEditSpeed->GetValue());
		else {
			CString Text;
			Text.Format(_T("%i"), ((CFamiTrackerDoc*)GetActiveDocument())->GetSongSpeed());
			pCmdUI->SetText(Text);
		}
	}	
}

void CMainFrame::OnUpdateTempoEdit(CCmdUI *pCmdUI)
{
	if (!m_pLockedEditTempo->IsEditable()) {
		if (m_pLockedEditTempo->Update())
			SetTempo(m_pLockedEditTempo->GetValue());
		else {
			CString Text;
			Text.Format(_T("%i"), ((CFamiTrackerDoc*)GetActiveDocument())->GetSongTempo());
			pCmdUI->SetText(Text);
		}
	}
}

void CMainFrame::OnUpdateRowsEdit(CCmdUI *pCmdUI)
{
	CString Text;

	if (!m_pLockedEditLength->IsEditable()) {
		if (m_pLockedEditLength->Update())
			SetRowCount(m_pLockedEditLength->GetValue());
		else {
			Text.Format(_T("%i"), ((CFamiTrackerDoc*)GetActiveDocument())->GetPatternLength());
			pCmdUI->SetText(Text);
		}
	}
}

void CMainFrame::OnUpdateFramesEdit(CCmdUI *pCmdUI)
{
	if (!m_pLockedEditFrames->IsEditable()) {
		if (m_pLockedEditFrames->Update())
			SetFrameCount(m_pLockedEditFrames->GetValue());
		else {
			CString Text;
			Text.Format(_T("%i"), ((CFamiTrackerDoc*)GetActiveDocument())->GetFrameCount());
			pCmdUI->SetText(Text);
		}
	}	
}

void CMainFrame::OnFileGeneralsettings()
{
	CString Title;
	GetMessageString(IDS_CONFIG_WINDOW, Title);
	CConfigWindow ConfigWindow(Title, this, 0);

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
	
	ConfigWindow.AddPage(&TabGeneral);
	ConfigWindow.AddPage(&TabAppearance);
	ConfigWindow.AddPage(&TabMIDI);
	ConfigWindow.AddPage(&TabSound);
	ConfigWindow.AddPage(&TabShortcuts);

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
	m_wndInstEdit.ChangeNoteState(Note);
}

void CMainFrame::OpenInstrumentSettings()
{
	CFamiTrackerDoc	*pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CFamiTrackerView *pView	= (CFamiTrackerView*)GetActiveView();

	if (pDoc->IsInstrumentUsed(pView->GetInstrument())) {
		if (m_wndInstEdit.IsOpened() == false) {
			m_wndInstEdit.Create(IDD_INSTRUMENT, this);
			m_wndInstEdit.SetCurrentInstrument(pView->GetInstrument());
			m_wndInstEdit.ShowWindow(SW_SHOW);
		}
		else
			m_wndInstEdit.SetCurrentInstrument(pView->GetInstrument());
		m_wndInstEdit.UpdateWindow();
	}
}

void CMainFrame::CloseInstrumentSettings()
{
	if (m_wndInstEdit.IsOpened())
		m_wndInstEdit.DestroyWindow();
}

void CMainFrame::OnUpdateKeyRepeat(CCmdUI *pCmdUI)
{
	if (theApp.GetSettings()->General.bKeyRepeat)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}

void CMainFrame::OnFileImportmidi()
{
	CMIDIImport	Importer;
	CFileDialog FileDialog(TRUE, 0, 0, OFN_HIDEREADONLY, _T("MIDI files (*.mid)|*.mid|All files|*.*||"));

	if (GetActiveDocument()->SaveModified() == 0)
		return;

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	Importer.ImportFile(FileDialog.GetPathName());
}

BOOL CMainFrame::DestroyWindow()
{
	// Store window position

	CRect WinRect;
	int State = STATE_NORMAL;

	GetWindowRect(WinRect);

	if (IsZoomed()) {
		State = STATE_MAXIMIZED;
		// Ignore window position if maximized
		WinRect.top = theApp.GetSettings()->WindowPos.iTop;
		WinRect.bottom = theApp.GetSettings()->WindowPos.iBottom;
		WinRect.left = theApp.GetSettings()->WindowPos.iLeft;
		WinRect.right = theApp.GetSettings()->WindowPos.iRight;
	}

	if (IsIconic()) {
		WinRect.top = WinRect.left = 100;
		WinRect.bottom = 920;
		WinRect.right = 950;
	}

	// Save window position
	theApp.GetSettings()->SetWindowPos(WinRect.left, WinRect.top, WinRect.right, WinRect.bottom, State);

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
	// Display module properties dialog
	CModulePropertiesDlg propertiesDlg;
	propertiesDlg.DoModal();
}

void CMainFrame::OnModuleChannels()
{
	CChannelsDlg channelsDlg;
	channelsDlg.DoModal();
}

void CMainFrame::UpdateTrackBox()
{
	// Fill the track box with all songs
	CComboBox		*pTrackBox	= (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_SUBTUNE);
	CFamiTrackerDoc	*pDoc		= (CFamiTrackerDoc*)GetActiveDocument();
	CString			Text;

	ASSERT(pTrackBox != NULL);
	ASSERT(pDoc != NULL);

	pTrackBox->ResetContent();

	int Count = pDoc->GetTrackCount();

	for (int i = 0; i < Count; ++i) {
		Text.Format(_T("#%i %s"), i + 1, pDoc->GetTrackTitle(i));
		pTrackBox->AddString(Text);
	}

	if (m_iTrack >= Count)
		m_iTrack = Count - 1;

	pTrackBox->SetCurSel(m_iTrack);
}

void CMainFrame::OnCbnSelchangeSong()
{
	CComboBox *pTrackBox = (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_SUBTUNE);
	CFamiTrackerDoc	*pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	m_iTrack = pTrackBox->GetCurSel();

	pDoc->SelectTrack(m_iTrack);
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
	// TODO: check this
	if (pMsg->message == WM_COMMAND) {
		m_pFrameWindow->SendMessage(WM_COMMAND, pMsg->wParam, pMsg->lParam);
	}

	return CFrameWnd::PreTranslateMessage(pMsg);
}

void CMainFrame::OnKillFocus(CWnd* pNewWnd)
{
	CFrameWnd::OnKillFocus(pNewWnd);
	CAccelerator *pAccel = theApp.GetAccelerator();
	pAccel->LostFocus();
}

void CMainFrame::OnRemoveFocus()
{
	GetActiveView()->SetFocus();
}

void CMainFrame::OnNextSong()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CComboBox *pTrackBox = (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_SUBTUNE);
	
	if (m_iTrack < (signed)pDoc->GetTrackCount() - 1)
		pDoc->SelectTrack(m_iTrack + 1);
}

void CMainFrame::OnPrevSong()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CComboBox *pTrackBox = (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_SUBTUNE);

	if (m_iTrack > 0)
		pDoc->SelectTrack(m_iTrack - 1);
}

void CMainFrame::OnUpdateNextSong(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	if (GetSelectedTrack() < (signed)(pDoc->GetTrackCount() - 1))
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdatePrevSong(CCmdUI *pCmdUI)
{
	if (GetSelectedTrack() > 0)
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
		GetActiveDocument()->UpdateAllViews(NULL, UPDATE_HIGHLIGHT);
		LastHighlight1 = Highlight1;
		LastHighlight2 = Highlight2;
	}
}

void CMainFrame::OnClearPatterns()
{
	if (AfxMessageBox(IDS_CLEARPATTERN, MB_OKCANCEL | MB_ICONWARNING) == IDOK) {
		CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
		pDoc->ClearPatterns();
	}
}

void CMainFrame::OnDuplicateFrame()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CFamiTrackerView *pView	= (CFamiTrackerView*)GetActiveView();

	int Frames = pDoc->GetFrameCount();
	int Frame = pView->GetSelectedFrame();
	int Channels = pDoc->GetAvailableChannels();

	if (Frames == MAX_FRAMES)
		return;

	pDoc->SetFrameCount(Frames + 1);

	for (int x = Frames; x > (Frame + 1); --x) {
		for (int i = 0; i < Channels; ++i) {
			pDoc->SetPatternAtFrame(x, i, pDoc->GetPatternAtFrame(x - 1, i));
		}
	}

	for (int i = 0; i < Channels; ++i) {
		pDoc->SetPatternAtFrame(Frame + 1, i, pDoc->GetPatternAtFrame(Frame, i));
	}

	pDoc->SetModifiedFlag();
	pDoc->UpdateAllViews(NULL, CHANGED_FRAMES);
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
	// TODO: check this
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

void CMainFrame::OnHelpEffecttable()
{
	// Display effect table in help
	HtmlHelp((DWORD)_T("effect_list.htm"), HH_DISPLAY_TOPIC);
}

CFrameBoxWnd *CMainFrame::GetFrameWindow() const
{
	return m_pFrameWindow;
}

void CMainFrame::OnEditEnableMIDI()
{
	theApp.GetMIDI()->ToggleInput();
}

void CMainFrame::OnUpdateEditEnablemidi(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(theApp.GetMIDI()->IsAvailable());
	pCmdUI->SetCheck(theApp.GetMIDI()->IsOpened());
}

void CMainFrame::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CFrameWnd::OnShowWindow(bShow, nStatus);

	if (bShow == TRUE) {
		// Set the window state as saved in settings
		if (theApp.GetSettings()->WindowPos.iState == STATE_MAXIMIZED)
			CFrameWnd::ShowWindow(SW_MAXIMIZE);
	}
}

void CMainFrame::OnDestroy()
{
	TRACE("FrameWnd: Destroying frame window\n");

	CSoundGen *pSoundGen = theApp.GetSoundGenerator();
	
	// Clean up sound stuff
	if (pSoundGen && pSoundGen->m_hThread != NULL) {
		// Remove sample window from sound generator
		pSoundGen->SetSampleWindow(NULL);
		// Kill the sound interface since the main window is being destroyed
		CEvent *pSoundEvent = new CEvent(FALSE, FALSE);
		pSoundGen->PostThreadMessage(M_CLOSE_SOUND, (WPARAM)pSoundEvent, NULL);
		// Wait for sound to close
		::WaitForSingleObject(pSoundEvent->m_hObject, 5000);
		delete pSoundEvent;
	}

	CFrameWnd::OnDestroy();
}

void CMainFrame::ChangedTrack()
{
	// Called from the view
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	m_iTrack = pDoc->GetSelectedTrack();
	CComboBox *pTrackBox = (CComboBox*)m_wndDialogBar.GetDlgItem(IDC_SUBTUNE);
	pTrackBox->SetCurSel(m_iTrack);
}

int CMainFrame::GetSelectedTrack() const
{
	return m_iTrack;
}

BOOL CMainFrame::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// Handle new instrument menu
	switch (((LPNMHDR)lParam)->code) {
		case TBN_DROPDOWN:
			OnNewInstrument((LPNMHDR)lParam, pResult);
			return FALSE;
	}

	return CFrameWnd::OnNotify(wParam, lParam, pResult);
}

void CMainFrame::OnNewInstrument( NMHDR * pNotifyStruct, LRESULT * result )
{
	CRect rect;
	::GetWindowRect(pNotifyStruct->hwndFrom, &rect);

	CMenu menu;

	menu.CreatePopupMenu();

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	int Chip = pDoc->GetExpansionChip();

	CFamiTrackerView *pView = (CFamiTrackerView*)GetActiveView();

	int SelectedChip = pDoc->GetChannel(pView->GetSelectedChannel())->GetChip();	// where the cursor is located

	menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_2A03, _T("New 2A03 instrument"));

	if (Chip & SNDCHIP_VRC6)
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_VRC6, _T("New VRC6 instrument"));
	if (Chip & SNDCHIP_VRC7)
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_VRC7, _T("New VRC7 instrument"));
	if (Chip & SNDCHIP_FDS)
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_FDS, _T("New FDS instrument"));
	if (Chip & SNDCHIP_MMC5)
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_MMC5, _T("New MMC5 instrument"));
	if (Chip & SNDCHIP_N106)
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_N106, _T("New Namco instrument"));
	if (Chip & SNDCHIP_S5B)
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_S5B, _T("New Sunsoft instrument"));

	switch (SelectedChip) {
		case SNDCHIP_NONE:
			menu.SetDefaultItem(ID_INSTRUMENT_ADD_2A03);
			break;
		case SNDCHIP_VRC6:
			menu.SetDefaultItem(ID_INSTRUMENT_ADD_VRC6);
			break;
		case SNDCHIP_VRC7:
			menu.SetDefaultItem(ID_INSTRUMENT_ADD_VRC7);
			break;
		case SNDCHIP_FDS:
			menu.SetDefaultItem(ID_INSTRUMENT_ADD_FDS);
			break;
		case SNDCHIP_MMC5:
			menu.SetDefaultItem(ID_INSTRUMENT_ADD_MMC5);
			break;
		case SNDCHIP_N106:
			menu.SetDefaultItem(ID_INSTRUMENT_ADD_N106);
			break;
		case SNDCHIP_S5B:
			menu.SetDefaultItem(ID_INSTRUMENT_ADD_S5B);
			break;
	}

	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, rect.left, rect.bottom, this);
}

BOOL CMainFrame::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	switch (pCopyDataStruct->dwData) {
		case IPC_LOAD:
			// Load file
			if (_tcslen((LPCTSTR)pCopyDataStruct->lpData) > 0)
				theApp.OpenDocumentFile((LPCTSTR)pCopyDataStruct->lpData);
			return TRUE;
		case IPC_LOAD_PLAY:
			// Load file
			if (_tcslen((LPCTSTR)pCopyDataStruct->lpData) > 0)
				theApp.OpenDocumentFile((LPCTSTR)pCopyDataStruct->lpData);
			// and play
			if (((CFamiTrackerDoc*)theApp.GetActiveDocument())->IsFileLoaded())
				theApp.GetSoundGenerator()->StartPlayer(MODE_PLAY_START);
			return TRUE;
	}

	return CFrameWnd::OnCopyData(pWnd, pCopyDataStruct);
}
