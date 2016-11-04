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
#include "MainFrm.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "AboutDlg.h"
#include "TrackerChannel.h"
#include "MIDI.h"
#include "SoundGen.h"
#include "Accelerator.h"
#include "Settings.h"
#include "CustomExporters.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CFamiTrackerApp

BEGIN_MESSAGE_MAP(CFamiTrackerApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_TRACKER_PLAY, OnTrackerPlay)
	ON_COMMAND(ID_TRACKER_PLAY_START, OnTrackerPlayStart)
	ON_COMMAND(ID_TRACKER_PLAY_CURSOR, OnTrackerPlayCursor)
	ON_COMMAND(ID_TRACKER_PLAY, OnTrackerPlay)
	ON_COMMAND(ID_TRACKER_STOP, OnTrackerStop)
	ON_COMMAND(ID_TRACKER_TOGGLE_PLAY, OnTrackerTogglePlay)
	ON_COMMAND(ID_TRACKER_PLAYPATTERN, OnTrackerPlaypattern)
	ON_COMMAND(ID_EDIT_ENABLEMIDI, OnEditEnableMIDI)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_PLAY, OnUpdateTrackerPlay)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_STOP, OnUpdateTrackerStop)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_PLAYPATTERN, OnUpdateTrackerPlay)
	ON_UPDATE_COMMAND_UI(ID_EDIT_ENABLEMIDI, OnUpdateEditEnablemidi)
END_MESSAGE_MAP()

// Include this for windows xp style in visual studio 2005 or later
#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")

// CFamiTrackerApp construction

CFamiTrackerApp::CFamiTrackerApp() :
	m_bInitialized(false),
	m_bShuttingDown(false),
	m_bDocLoaded(false),
	m_bThemeActive(false),
	m_iFrameRate(0),
	m_iAddedChips(0),
	m_pMIDI(NULL),
	m_pAccel(NULL),
	m_pSettings(NULL),
	m_pSoundGenerator(NULL),
	m_customExporters(NULL)
{
	// Place all significant initialization in InitInstance
	EnableHtmlHelp();
}


// The one and only CFamiTrackerApp object
CFamiTrackerApp	theApp;

// CFamiTrackerApp initialization

void CFamiTrackerApp::OnFileOpen() 
{
	// Overloaded this to save the ftm-path
	CFileDialog FileDialog(TRUE, "ftm", "", OFN_HIDEREADONLY, "FamiTracker files (*.ftm)|*.ftm|All files (*.*)|*.*||", this->GetMainWnd(), 0);
	POSITION DocPos = GetFirstDocTemplatePosition();
	FileDialog.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_FTM);
	if (FileDialog.DoModal() == IDCANCEL)
		return;
	m_pSettings->SetPath(FileDialog.GetPathName(), PATH_FTM);
	OpenDocumentFile(FileDialog.GetPathName());
}

BOOL CFamiTrackerApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();
	
	CWinApp::InitInstance();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T(""));
	LoadStdProfileSettings(8);  // Load standard INI file options (including MRU)
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views

	// Load settings
	m_pSettings = new CSettings();
	m_pSettings->LoadSettings();

	m_customExporters = new CCustomExporters( m_pSettings->GetPath(PATH_PLUGIN) );

	//who: added by Derek Andrews <derek.george.andrews@gmail.com>
	//why: Load all custom exporter plugins on startup.
//	LoadPlugins();

	// Load custom accelerator
	m_pAccel = new CAccelerator;
	m_pAccel->LoadShortcuts(m_pSettings);

	// Create the MIDI interface
	m_pMIDI = new CMIDI();

	// Sound generator
	CSoundGen *pSoundGen = new CSoundGen();
	pSoundGen->CreateAPU();

	SetSoundGenerator(pSoundGen);

	CSingleDocTemplate* pDocTemplate;

	pDocTemplate = new CSingleDocTemplate(IDR_MAINFRAME, RUNTIME_CLASS(CFamiTrackerDoc), RUNTIME_CLASS(CMainFrame), RUNTIME_CLASS(CFamiTrackerView));
	
	if (!pDocTemplate)
		return FALSE;

	AddDocTemplate(pDocTemplate);

	// Determine windows version
    OSVERSIONINFO osvi;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);

	// Work-around to enable file type registration in windows vista/7
	if (osvi.dwMajorVersion >= 6) { 
		HKEY HKCU;

		long res_reg = ::RegOpenKey(HKEY_CURRENT_USER, "Software\\Classes", &HKCU);

		if(res_reg == ERROR_SUCCESS)
			res_reg = RegOverridePredefKey(HKEY_CLASSES_ROOT, HKCU);
	}

	// Enable DDE Execute open
	EnableShellOpen();

	// Skip this if in wip mode
#ifndef WIP
	RegisterShellFileTypes(TRUE);
#endif

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// Move root key back to default
	if (osvi.dwMajorVersion >= 6) { 
		::RegOverridePredefKey(HKEY_CLASSES_ROOT, NULL);
	}

	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Add available chips
#ifdef _DEBUG
	// Under development
	AddChip("Internal only (2A03/2A07)", SNDCHIP_NONE, new CInstrument2A03());
	AddChip("Konami VRC6", SNDCHIP_VRC6, new CInstrumentVRC6());
	AddChip("Konami VRC7", SNDCHIP_VRC7, new CInstrumentVRC7());
	AddChip("Nintendo FDS sound", SNDCHIP_FDS, new CInstrumentFDS());
	AddChip("Nintendo MMC5", SNDCHIP_MMC5, new CInstrument2A03());
	AddChip("Namco N106", SNDCHIP_N106, new CInstrumentN106());
	AddChip("Sunsoft 5B", SNDCHIP_5B, new CInstrument5B());
#else
	// Ready for use
	AddChip("Internal only (2A03/2A07)", SNDCHIP_NONE, new CInstrument2A03());
	AddChip("Konami VRC6", SNDCHIP_VRC6, new CInstrumentVRC6());
	AddChip("Nintendo MMC5", SNDCHIP_MMC5, new CInstrument2A03());
	AddChip("Konami VRC7", SNDCHIP_VRC7, new CInstrumentVRC7());
	AddChip("Nintendo FDS sound", SNDCHIP_FDS, new CInstrumentFDS());
#endif

	// This object is used to indicate if the sound synth thread locks up
	m_hAliveCheck = CreateEvent(NULL, TRUE, FALSE, NULL);
	// Used to awake the sound generator thread in case of lockup
	m_hNotificationEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// Initialize the sound generator
	if (!pSoundGen->InitializeSound(GetMainWnd()->m_hWnd, m_hAliveCheck, m_hNotificationEvent)) {
		// If failed, restore and save default settings
		m_pSettings->DefaultSettings();
		m_pSettings->SaveSettings();
		// Quit program
		DisplayError(IDS_START_ERROR);
		return FALSE;
	}

	// Todo: remove this, probably a source to some errors
	pSoundGen->SetDocument((CFamiTrackerDoc*) GetFirstDocument(), (CFamiTrackerView*) GetDocumentView());

	GetCurrentDirectory(256, m_cAppPath);

	// Start sound generator
	if (!pSoundGen->CreateThread()) {
		// If failed, restore and save default settings
		m_pSettings->DefaultSettings();
		m_pSettings->SaveSettings();
		// Show message and quit
		DisplayError(IDS_SOUNDGEN_ERROR);
		return FALSE;
	}

	// Initialize midi unit
	m_pMIDI->Init();

	// Check if the application is themed
	CheckAppThemed();
	
	// Setup windows on start
	((CMainFrame*) GetMainWnd())->ResizeFrameWindow();

	// Initialization is done
	m_bInitialized = true;

	return TRUE;
}

// Close program
int CFamiTrackerApp::ExitInstance()
{
	if (!m_bInitialized)
		return CWinApp::ExitInstance();

	ShutDownSynth();

	if (m_pMIDI) {
		m_pMIDI->CloseDevice();
		m_pMIDI->Shutdown();
		delete m_pMIDI;
		m_pMIDI = NULL;
	}

	if (m_pAccel) {
		m_pAccel->SaveShortcuts(m_pSettings);
		delete m_pAccel;
		m_pAccel = NULL;
	}

	if (m_pSettings) {
		m_pSettings->SaveSettings();
		delete m_pSettings;
		m_pSettings = NULL;
	}

	return CWinApp::ExitInstance();
}

void CFamiTrackerApp::CheckAppThemed()
{
	HMODULE hinstDll = ::LoadLibrary(_T("UxTheme.dll"));
	
	if (hinstDll) {
		typedef BOOL (*ISAPPTHEMEDPROC)();
		ISAPPTHEMEDPROC pIsAppThemed;
		pIsAppThemed = (ISAPPTHEMEDPROC) ::GetProcAddress(hinstDll, "IsAppThemed");

		if(pIsAppThemed)
			m_bThemeActive = (pIsAppThemed() == TRUE);

		::FreeLibrary(hinstDll);
	}
}

void CFamiTrackerApp::SetDocumentLoaded(bool Loaded)
{
	m_bDocLoaded = Loaded;
}

void CFamiTrackerApp::ShutDownSynth()
{
	// Shut down sound generator

	HANDLE hThread;
	DWORD ExitCode;

	m_bShuttingDown = true;

//	::DuplicateHandle(0, m_pSoundGenerator->m_hThread, 0, &hThread, 0, FALSE, 0);
	hThread = m_pSoundGenerator->m_hThread;

	m_pSoundGenerator->PostThreadMessage(WM_QUIT, 0, 0);
	SetEvent(m_hNotificationEvent);

	// Probe for thread termination
	for (int i = 0; i < 50; i++) {
		if (GetExitCodeThread(hThread, &ExitCode) != STILL_ACTIVE)
			return;
		else
			Sleep(10);
	}

	// Failed to close thread
	DisplayError(IDS_SOUNDGEN_CLOSE_ERR);
}

void CFamiTrackerApp::BufferUnderrun()
{
	// Todo: move to string table
	((CMainFrame*) GetMainWnd())->SetMessageText("Audio buffer underrun, sound settings may be too high for this system or CPU utilization is too high.");
}

void CFamiTrackerApp::CheckSynth()
{
	static DWORD LastTime;

	// Monitor performance

	if (!m_hAliveCheck)
		return;

	if (LastTime == 0)
		LastTime = GetTickCount();

	if (WaitForSingleObject(m_hAliveCheck, 0) == WAIT_OBJECT_0) {
		if ((GetTickCount() - LastTime) > 1000) {
			((CMainFrame*) GetMainWnd())->SetMessageText(AFX_IDS_IDLEMESSAGE);
		}

		LastTime = GetTickCount();
	}
	else {
		// Timeout after 1 s
		if ((GetTickCount() - LastTime) > 1000) {
			((CMainFrame*) GetMainWnd())->SetMessageText("It appears the current sound settings aren't working, change settings and try again!");
		}
	}

	ResetEvent(m_hAliveCheck);
}


// Display an error message from the string lib
void CFamiTrackerApp::DisplayError(int Message)
{
	CComBSTR ErrorMsg;
	ErrorMsg.LoadString(Message);
	AfxMessageBox(CW2CT(ErrorMsg), MB_ICONERROR, 0);
}

int CFamiTrackerApp::DisplayMessage(int Message, int Type)
{
	CComBSTR MsgString;
	MsgString.LoadString(Message);
	return AfxMessageBox(CW2CT(MsgString), Type);
}

// Return sound gen
CSoundGen *CFamiTrackerApp::GetSoundGenerator()
{
	return m_pSoundGenerator;
}

CCustomExporters* CFamiTrackerApp::GetCustomExporters(void)
{
	return m_customExporters;
}

void CFamiTrackerApp::SetSoundGenerator(CSoundGen *pGen)
{
	m_pSoundGenerator = pGen;
}

// Load sound configuration
void CFamiTrackerApp::LoadSoundConfig()
{
	m_pSoundGenerator->PostThreadMessage(M_LOAD_SETTINGS, 0, 0);
	SetEvent(m_hNotificationEvent);
}

// Get the midi interface
CMIDI *CFamiTrackerApp::GetMIDI()
{
	return m_pMIDI;
}

// Command messages

void CFamiTrackerApp::OnEditEnableMIDI()
{
	m_pMIDI->Toggle();
}

void CFamiTrackerApp::OnUpdateEditEnablemidi(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pMIDI->IsAvailable());
	pCmdUI->SetCheck(m_pMIDI->IsOpened());
}

////////////////////////////////////////////////////////
//  Things that belongs to the synth are kept below!  //
////////////////////////////////////////////////////////

// Silences everything
void CFamiTrackerApp::SilentEverything()
{
	if (m_bInitialized)
		m_pSoundGenerator->PostThreadMessage(M_SILENT_ALL, 0, 0);

	((CFamiTrackerView*)GetDocumentView())->MakeSilent();
}

// Machine type, NTSC/PAL
void CFamiTrackerApp::SetMachineType(int Type, int Rate)
{
	m_pSoundGenerator->LoadMachineSettings(Type, Rate);
}

void CFamiTrackerApp::RegisterKeyState(int Channel, int Note)
{
	CDocument *pDoc = GetFirstDocument();

	if (pDoc == NULL)
		return;

	POSITION pos = pDoc->GetFirstViewPosition();

	if (pos != NULL) {
		CFamiTrackerView *pView = (CFamiTrackerView*)pDoc->GetNextView(pos);
		pView->RegisterKeyState(Channel, Note);
	}
}

// Read and reset frame rate
int CFamiTrackerApp::GetFrameRate()
{
	m_iFrameRate = m_iFrameCounter;
	m_iFrameCounter = 0;
	return m_iFrameRate;
}

// Increases frame
void CFamiTrackerApp::StepFrame()
{
	m_iFrameCounter++;
}

int CFamiTrackerApp::GetUnderruns()
{
	return m_pSoundGenerator->GetUnderruns();
}

int CFamiTrackerApp::GetTempo()
{
	if (!m_pSoundGenerator)
		return 0;

	return m_pSoundGenerator->GetTempo();
}

void CFamiTrackerApp::ResetTempo()
{
	if (m_bInitialized)
		m_pSoundGenerator->ResetTempo();
}

// App command to run the dialog
void CFamiTrackerApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CFamiTrackerApp message handlers

void CFamiTrackerApp::DrawSamples(int *Samples, int Count)
{	
	if (GetMainWnd() != NULL)
		((CMainFrame*) GetMainWnd())->DrawSamples(Samples, Count);
}

int CFamiTrackerApp::GetCPUUsage()
{
	static FILETIME KernelLastTime[2], UserLastTime[2];
	FILETIME CreationTime[2], ExitTime[2], KernelTime[2], UserTime[2];
	unsigned int TotalTime[2];

	HANDLE hThreads[2] = {m_hThread, m_pSoundGenerator->m_hThread};

	GetThreadTimes(hThreads[0], CreationTime + 0, ExitTime + 0, KernelTime + 0, UserTime + 0);
	GetThreadTimes(hThreads[1], CreationTime + 1, ExitTime + 1, KernelTime + 1, UserTime + 1);

	TotalTime[0] = (KernelTime[0].dwLowDateTime - KernelLastTime[0].dwLowDateTime) / 1000;
	TotalTime[1] = (KernelTime[1].dwLowDateTime - KernelLastTime[1].dwLowDateTime) / 1000;
	TotalTime[0] += (UserTime[0].dwLowDateTime - UserLastTime[0].dwLowDateTime) / 1000;
	TotalTime[1] += (UserTime[1].dwLowDateTime - UserLastTime[1].dwLowDateTime) / 1000;
	KernelLastTime[0] = KernelTime[0];
	KernelLastTime[1] = KernelTime[1];
	UserLastTime[0]	= UserTime[0];
	UserLastTime[1]	= UserTime[1];

	return TotalTime[0] + TotalTime[1];
}

void CFamiTrackerApp::MidiEvent(void)
{
	GetDocumentView()->PostMessage(MSG_MIDI_EVENT);
}

void CFamiTrackerApp::ReloadColorScheme(void)
{
	// Todo: Move CreateFont() to cview::update and reload_colors

	CDocument *pDoc = GetFirstDocument();

	if (pDoc) {
		POSITION pos = pDoc->GetFirstViewPosition();
		if (pos) {
			CFamiTrackerView *pView = (CFamiTrackerView*)pDoc->GetNextView(pos);
			pView->CreateFont();
			pDoc->UpdateAllViews(NULL, RELOAD_COLORS);
			((CMainFrame*) GetMainWnd())->SetupColors();
		}
	}
}

void CFamiTrackerApp::ResetPlayer()
{
	if (m_pSoundGenerator)
		m_pSoundGenerator->ResetPlayer();
}

// Play from top of pattern
void CFamiTrackerApp::OnTrackerPlay()
{
	if (m_pSoundGenerator)
		m_pSoundGenerator->StartPlayer(MODE_PLAY);
}

// Loop pattern
void CFamiTrackerApp::OnTrackerPlaypattern()
{
	if (m_bInitialized) {
		m_pSoundGenerator->StartPlayer(MODE_PLAY_REPEAT);
	}
}

// Play from start of song
void CFamiTrackerApp::OnTrackerPlayStart()
{
	if (m_pSoundGenerator)
		m_pSoundGenerator->StartPlayer(MODE_PLAY_START);
}

// Play from cursor
void CFamiTrackerApp::OnTrackerPlayCursor()
{
	if (m_pSoundGenerator)
		m_pSoundGenerator->StartPlayer(MODE_PLAY_CURSOR);
}

void CFamiTrackerApp::OnTrackerStop()
{
	if (m_pSoundGenerator)
		m_pSoundGenerator->StopPlayer();
	m_pMIDI->ResetOutput();
}

void CFamiTrackerApp::OnUpdateTrackerPlay(CCmdUI *pCmdUI)
{
	//pCmdUI->Enable(!m_pSoundGenerator->IsPlaying() ? 1 : 0);
}

void CFamiTrackerApp::OnUpdateTrackerStop(CCmdUI *pCmdUI)
{
//	pCmdUI->Enable(m_pSoundGenerator->IsPlaying() ? 1 : 0);
}

void CFamiTrackerApp::OnTrackerTogglePlay()
{
	if (m_pSoundGenerator) {
		if (m_pSoundGenerator->IsPlaying())
			OnTrackerStop();
		else
			OnTrackerPlay();
	}
}

bool CFamiTrackerApp::IsPlaying()
{
	return m_pSoundGenerator->IsPlaying();
}

void CFamiTrackerApp::StopPlayer()
{
	OnTrackerStop();
}

BOOL CFamiTrackerApp::PreTranslateMessage(MSG* pMsg)
{
	CAccelerator *pAccel = theApp.GetAccelerator();

	// Dynamic accelerator translation
	if ((pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) ) {	// WM_SYSKEYDOWN is for ALT
		BOOL bRes = CWinApp::PreTranslateMessage(pMsg);
		if (bRes == 0) {
			// Find an ID
			unsigned short ID = pAccel->GetAction((unsigned char)pMsg->wParam);
			// If an ID was found, send a WM_COMMAND to it
			if (ID != 0 && ((CMainFrame*)GetMainWnd())->HasFocus()) {
				GetMainWnd()->SendMessage(WM_COMMAND, (0 << 16) | ID, NULL);
				// Skip internal processing for ALT-keys to avoid beeps
				if (pMsg->message == WM_SYSKEYDOWN)
					return TRUE;
			}
		}
		return bRes;
	}
	else if (pMsg->message == WM_KEYUP || pMsg->message == WM_SYSKEYUP) {
		BOOL bRes = CWinApp::PreTranslateMessage(pMsg);
//		if (bRes == 0) {
			pAccel->KeyReleased((unsigned char)pMsg->wParam);
//		}
		return bRes;
	}

	return CWinApp::PreTranslateMessage(pMsg);
}

CDocument *CFamiTrackerApp::GetFirstDocument()
{
	CDocTemplate *pDocTemp;
	POSITION TemplatePos, DocPos;
	
	TemplatePos	= GetFirstDocTemplatePosition();
	pDocTemp = GetNextDocTemplate(TemplatePos);
	DocPos = pDocTemp->GetFirstDocPosition();

	return pDocTemp->GetNextDoc(DocPos);
}

CView *CFamiTrackerApp::GetDocumentView()
{
	CDocument *pDoc = GetFirstDocument();

	if (!pDoc)
		return NULL;

	POSITION pos = pDoc->GetFirstViewPosition();

	return pDoc->GetNextView(pos);
}

void CFamiTrackerApp::SetSoundChip(int Chip) 
{ 	
	// This should be done outside of the sound generator
	m_pSoundGenerator->SetupChannels(Chip, (CFamiTrackerDoc*) GetFirstDocument()); 
	
	if (GetMainWnd())
		((CMainFrame*) GetMainWnd())->ResizeFrameWindow();
}

// Expansion chip handling

void CFamiTrackerApp::AddChip(char *pName, int Ident, CInstrument *pInst)
{
	ASSERT(m_iAddedChips < CHIP_COUNT);

	m_pChipNames[m_iAddedChips] = pName;
	m_iChipIdents[m_iAddedChips] = Ident;
	m_pChipInst[m_iAddedChips] = pInst;
	m_iAddedChips++;
}

int CFamiTrackerApp::GetChipCount()
{
	return m_iAddedChips;
}

char *CFamiTrackerApp::GetChipName(int Index)
{
	return m_pChipNames[Index];
}

int CFamiTrackerApp::GetChipIdent(int Index)
{
	return m_iChipIdents[Index];
}

int	CFamiTrackerApp::GetChipIndex(int Ident)
{
	for (int i = 0; i < m_iAddedChips; i++) {
		if (Ident == m_iChipIdents[i])
			return i;
	}
	return 0;
}

CInstrument* CFamiTrackerApp::GetChipInstrument(int Chip)
{
	int Index = GetChipIndex(Chip);

	if (m_pChipInst[Index] == NULL)
		return NULL;

	return m_pChipInst[Index]->CreateNew();
}

