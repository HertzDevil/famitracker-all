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
#include "Exception.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "AboutDlg.h"
#include "TrackerChannel.h"
#include "MIDI.h"
#include "SoundGen.h"
#include "Accelerator.h"
#include "Settings.h"
#include "ChannelMap.h"
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
END_MESSAGE_MAP()

// Include this for windows xp style in visual studio 2005 or later
#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")

// CFamiTrackerApp construction

CFamiTrackerApp::CFamiTrackerApp() :
	m_bThemeActive(false),
	m_pMIDI(NULL),
	m_pAccel(NULL),
	m_pSettings(NULL),
	m_pSoundGenerator(NULL),
	m_pChannelMap(NULL),
	m_customExporters(NULL),
	m_hAliveCheck(NULL),
	m_hNotificationEvent(NULL)
{
	// Place all significant initialization in InitInstance
	EnableHtmlHelp();

#ifdef ENABLE_CRASH_HANDLER
	// This will cover the whole process
	SetUnhandledExceptionFilter(ExceptionHandler);
#endif
}


// The one and only CFamiTrackerApp object
CFamiTrackerApp	theApp;

// CFamiTrackerApp initialization

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

	// Load program settings
	m_pSettings = new CSettings();
	m_pSettings->LoadSettings();

	//who: added by Derek Andrews <derek.george.andrews@gmail.com>
	//why: Load all custom exporter plugins on startup.
	
	TCHAR pathToPlugins[MAX_PATH];
	GetModuleFileName(NULL, pathToPlugins, MAX_PATH);
	PathRemoveFileSpec(pathToPlugins);
	PathAppend(pathToPlugins, "\\Plugins");
	m_customExporters = new CCustomExporters( pathToPlugins );

	// Load custom accelerator
	m_pAccel = new CAccelerator();
	m_pAccel->LoadShortcuts(m_pSettings);

	// Create the MIDI interface
	m_pMIDI = new CMIDI();

	// Create sound generator
	m_pSoundGenerator = new CSoundGen();

	// Create channel map
	m_pChannelMap = new CChannelMap();

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
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
		long res_reg = ::RegOpenKey(HKEY_CURRENT_USER, _T("Software\\Classes"), &HKCU);
		if(res_reg == ERROR_SUCCESS)
			res_reg = RegOverridePredefKey(HKEY_CLASSES_ROOT, HKCU);
	}

	// Enable DDE Execute open
	EnableShellOpen();

	// Skip this if in wip/beta mode
#ifndef WIP
	//RegisterShellFileTypes(TRUE); // EDIT
#endif
	
	// Parse command line for standard shell commands, DDE, file open + some custom ones
	/*CCommandLineInfo*/ CFTCommandLineInfo cmdInfo;
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

	// This object is used to indicate if the sound synth thread locks up
	m_hAliveCheck = CreateEvent(NULL, TRUE, FALSE, NULL);
	// Used to awake the sound generator thread in case of lockup
	m_hNotificationEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	// Initialize the sound generator
	if (!m_pSoundGenerator->InitializeSound(m_pMainWnd->m_hWnd, m_hAliveCheck, m_hNotificationEvent)) {
		// If failed, restore and save default settings
		m_pSettings->DefaultSettings();
		m_pSettings->SaveSettings();
		// Quit program
		AfxMessageBox(IDS_START_ERROR, MB_ICONERROR);
		return FALSE;
	}

	// Start sound generator thread
	if (!m_pSoundGenerator->CreateThread()) {
		// If failed, restore and save default settings
		m_pSettings->DefaultSettings();
		m_pSettings->SaveSettings();
		// Show message and quit
		AfxMessageBox(IDS_SOUNDGEN_ERROR, MB_ICONERROR);
		return FALSE;
	}

	// Initialize midi unit
	m_pMIDI->Init();

	// Check if the application is themed
	CheckAppThemed();
	
	if (cmdInfo.m_bPlay)
		theApp.OnTrackerPlay();

	// Initialization is done
	TRACE0("App: InitInstance done\n");

	return TRUE;
}

int CFamiTrackerApp::ExitInstance()
{
	// Close program
	// The document is already closed at this point (and detached from sound player)

	TRACE("App: ExitInstance started\n");

	ShutDownSynth();

	if (m_pMIDI) {
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

	if (m_customExporters) {
		delete m_customExporters;
		m_customExporters = NULL;
	}

	if (m_pChannelMap) {
		delete m_pChannelMap;
		m_pChannelMap = NULL;
	}

	TRACE0("App: ExitInstance done\n");

	return CWinApp::ExitInstance();
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

bool CFamiTrackerApp::IsThemeActive() const
{ 
	return m_bThemeActive; 
}

void CFamiTrackerApp::ShutDownSynth()
{
	// Shut down sound generator
	if (!m_pSoundGenerator)
		return;

	// Save a handle to the thread since the object will delete itself
	HANDLE hThread = m_pSoundGenerator->m_hThread;

	if (!hThread) {
		// Object was found but thread not created
		delete m_pSoundGenerator;
		m_pSoundGenerator = NULL;
		return;
	}

	// Send quit message
	m_pSoundGenerator->PostThreadMessage(WM_QUIT, 0, 0);
	SetEvent(m_hNotificationEvent);

	// Wait for thread to exit, timout = 2s
	DWORD dwResult = ::WaitForSingleObject(hThread, 2000);

	if (dwResult != WAIT_OBJECT_0) {
		TRACE0("App: Closing the sound generator thread failed\n");
#ifdef _DEBUG
		AfxMessageBox(_T("Error: Could not close sound generator thread"));
#endif
		// Unclean exit
		return;
	}

	// Object is auto-deleted
	m_pSoundGenerator = NULL;

	// Close handles
	CloseHandle(m_hNotificationEvent);
	CloseHandle(m_hAliveCheck);

	m_hNotificationEvent = NULL;
	m_hAliveCheck = NULL;

	TRACE0("App: Sound generator has closed\n");
}

CCustomExporters* CFamiTrackerApp::GetCustomExporters(void) const
{
	return m_customExporters;
}

////////////////////////////////////////////////////////
//  Things that belongs to the synth are kept below!  //
////////////////////////////////////////////////////////

// Load sound configuration
void CFamiTrackerApp::LoadSoundConfig()
{
	GetSoundGenerator()->PostThreadMessage(M_LOAD_SETTINGS, 0, 0);
	SetEvent(m_hNotificationEvent);

	((CFrameWnd*)GetMainWnd())->SetMessageText(_T("New sound configuration loaded"));
}

// Silences everything
void CFamiTrackerApp::SilentEverything()
{
	GetSoundGenerator()->PostThreadMessage(M_SILENT_ALL, 0, 0);
	static_cast<CFamiTrackerView*>(GetActiveView())->MakeSilent();
}

void CFamiTrackerApp::CheckSynth() 
{
	// Monitor performance
	static DWORD LastTime;

	if (!m_hAliveCheck)
		return;

	if (LastTime == 0)
		LastTime = GetTickCount();

	// Wait for signals from the player thread
	if (WaitForSingleObject(m_hAliveCheck, 0) == WAIT_OBJECT_0) {	// return immediately
		if ((GetTickCount() - LastTime) > 1000) {
			((CMainFrame*) GetMainWnd())->SetMessageText(AFX_IDS_IDLEMESSAGE);
		}
		LastTime = GetTickCount();
	}
	else {
		// Timeout after 1 s
		if ((GetTickCount() - LastTime) > 1000) {
			// Display message
			((CMainFrame*) GetMainWnd())->SetMessageText(IDS_SOUND_FAIL);
		}
	}

	ResetEvent(m_hAliveCheck);
}

int CFamiTrackerApp::GetCPUUsage() const
{
	// Calculate CPU usage
	/*
	static FILETIME KernelLastTime[2], UserLastTime[2];
	FILETIME CreationTime[2], ExitTime[2], KernelTime[2], UserTime[2];
	unsigned int TotalTime[2];
	*/

	const int THREAD_COUNT = 2;

	static FILETIME KernelLastTime[THREAD_COUNT], UserLastTime[THREAD_COUNT];
	const HANDLE hThreads[THREAD_COUNT] = {m_hThread, m_pSoundGenerator->m_hThread};

	unsigned int TotalTime = 0;

	for (int i = 0; i < THREAD_COUNT; ++i) {
		FILETIME CreationTime, ExitTime, KernelTime, UserTime;
		GetThreadTimes(hThreads[i], &CreationTime, &ExitTime, &KernelTime, &UserTime);
		TotalTime += (KernelTime.dwLowDateTime - KernelLastTime[i].dwLowDateTime) / 1000;
		TotalTime += (UserTime.dwLowDateTime - UserLastTime[i].dwLowDateTime) / 1000;
		KernelLastTime[i] = KernelTime;
		UserLastTime[i]	= UserTime;
	}

	return TotalTime;
}

void CFamiTrackerApp::ReloadColorScheme(void)
{
	// Main window
	CMainFrame *pMainFrm = dynamic_cast<CMainFrame*>(GetMainWnd());

	if (pMainFrm != NULL)
		pMainFrm->SetupColors();

	// Notify all views	
	POSITION TemplatePos = GetFirstDocTemplatePosition();
	CDocTemplate *pDocTemplate = GetNextDocTemplate(TemplatePos);
	POSITION DocPos = pDocTemplate->GetFirstDocPosition();

	while (CDocument* pDoc = pDocTemplate->GetNextDoc(DocPos)) {
		POSITION ViewPos = pDoc->GetFirstViewPosition();
		while (CView *pView = pDoc->GetNextView(ViewPos)) {
			static_cast<CFamiTrackerView*>(pView)->SetupColors();
		}
	}
}

void CFamiTrackerApp::RegisterKeyState(int Channel, int Note)
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());

	if (pView)
		pView->RegisterKeyState(Channel, Note);
}

// App command to run the about dialog
void CFamiTrackerApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CFamiTrackerApp message handlers

void CFamiTrackerApp::OnTrackerPlay()
{
	// Play
	GetSoundGenerator()->StartPlayer(MODE_PLAY);
}

void CFamiTrackerApp::OnTrackerPlaypattern()
{
	// Loop pattern
	GetSoundGenerator()->StartPlayer(MODE_PLAY_REPEAT);
}

void CFamiTrackerApp::OnTrackerPlayStart()
{
	// Play from start of song
	GetSoundGenerator()->StartPlayer(MODE_PLAY_START);
}

void CFamiTrackerApp::OnTrackerPlayCursor()
{
	// Play from cursor
	GetSoundGenerator()->StartPlayer(MODE_PLAY_CURSOR);
}

void CFamiTrackerApp::OnTrackerTogglePlay()
{
	if (GetSoundGenerator()->IsPlaying())
		OnTrackerStop();
	else
		OnTrackerPlay();
}

void CFamiTrackerApp::OnTrackerStop()
{
	// Stop tracker
	GetSoundGenerator()->StopPlayer();
	m_pMIDI->ResetOutput();
}

// Player interface

bool CFamiTrackerApp::IsPlaying()
{
	return GetSoundGenerator()->IsPlaying();
}

void CFamiTrackerApp::StopPlayer()
{
	OnTrackerStop();
}

void CFamiTrackerApp::ResetPlayer()
{
	GetSoundGenerator()->ResetPlayer();
}

// Active document & view

CDocument *CFamiTrackerApp::GetActiveDocument() const
{
	CFrameWnd *pFrameWnd = dynamic_cast<CFrameWnd*>(m_pMainWnd);

	if (!pFrameWnd)
		return NULL;

	return pFrameWnd->GetActiveDocument();
}

CView *CFamiTrackerApp::GetActiveView() const
{
	CFrameWnd *pFrameWnd = dynamic_cast<CFrameWnd*>(m_pMainWnd);

	if (!pFrameWnd)
		return NULL;
	
	return pFrameWnd->GetActiveView();
}

// File load/save

void CFamiTrackerApp::OnFileOpen() 
{
	// Overloaded in order to save the file path
	CString newName, path;

	// Get path
	path = m_pSettings->GetPath(PATH_FTM) + _T("\\");
	newName = _T("");

	if (!DoPromptFileName(newName, path, AFX_IDS_OPENFILE, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, TRUE, NULL))
		return; // open cancelled

	// Save path
	m_pSettings->SetPath(newName, PATH_FTM);
	((CFrameWnd*)GetMainWnd())->SetMessageText(_T("Loading file..."));
	AfxGetApp()->OpenDocumentFile(newName);
	((CFrameWnd*)GetMainWnd())->SetMessageText(_T("Done"));
}

BOOL CFamiTrackerApp::DoPromptFileName(CString& fileName, CString& filePath, UINT nIDSTitle, DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate)
{
	// Copied from MFC
	CFileDialog dlgFile(bOpenFileDialog, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, NULL, 0);

	CString title;
	ENSURE(title.LoadString(nIDSTitle));

	dlgFile.m_ofn.Flags |= lFlags;

	CString strFilter;
	CString strDefault;

	if (pTemplate == NULL) {
		POSITION pos = GetFirstDocTemplatePosition();
		while (pos != NULL) {
			CString strFilterExt;
			CString strFilterName;
			pTemplate = GetNextDocTemplate(pos);
			pTemplate->GetDocString(strFilterExt, CDocTemplate::filterExt);
			pTemplate->GetDocString(strFilterName, CDocTemplate::filterName);

			// Add extension
			strFilter += strFilterName;
			strFilter += (TCHAR)'\0';
			strFilter += _T("*");
			strFilter += strFilterExt;
			strFilter += (TCHAR)'\0';
			dlgFile.m_ofn.nMaxCustFilter++;
		}
	}

	// Select first filter
	dlgFile.m_ofn.nFilterIndex = 1;

	// append the "*.*" all files filter
	CString allFilter;
	VERIFY(allFilter.LoadString(AFX_IDS_ALLFILTER));
	strFilter += allFilter;
	strFilter += (TCHAR)'\0';   // next string please
	strFilter += _T("*.*");
	strFilter += (TCHAR)'\0';   // last string
	dlgFile.m_ofn.nMaxCustFilter++;

	dlgFile.m_ofn.lpstrFilter = strFilter;
	dlgFile.m_ofn.lpstrTitle = title;
	dlgFile.m_ofn.lpstrInitialDir = filePath.GetBuffer(_MAX_PATH);
	dlgFile.m_ofn.lpstrFile = fileName.GetBuffer(_MAX_PATH);

	INT_PTR nResult = dlgFile.DoModal();
	fileName.ReleaseBuffer();
	filePath.ReleaseBuffer();

	return nResult == IDOK;
}

// Various global helper functions

CString LoadDefaultFilter(LPCTSTR Name, LPCTSTR Ext)
{
	// Loads a single filter string including the all files option
	CString filter;
	CString allFilter;
	VERIFY(allFilter.LoadString(AFX_IDS_ALLFILTER));

	filter = Name;
	filter += _T("|*");
	filter += Ext;
	filter += _T("|");
	filter += allFilter;
	filter += _T("|*.*||");

	return filter;
}

/**
 * CFTCommandLineInfo, a custom command line parser
 *
 */

CFTCommandLineInfo::CFTCommandLineInfo() : CCommandLineInfo(), 
	m_bLog(false), 
	m_bExport(false), 
	m_bPlay(false),
	m_strExportFile(_T(""))
{
}

void CFTCommandLineInfo::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
{
	if (bFlag) {
		// Export file (/export or /e)
		if (!_tcsicmp(pszParam, _T("export")) || !_tcsicmp(pszParam, _T("e"))) {
			m_bExport = true;
			return;
		}
		// Auto play (/play or /p)
		else if (!_tcsicmp(pszParam, _T("play")) || !_tcsicmp(pszParam, _T("p"))) {
			m_bPlay = true;
			return;
		}
		// Enable register logger (/log), available in debug mode only
		else if (!_tcsicmp(pszParam, _T("log"))) {
#ifdef _DEBUG
			m_bLog = true;
			return;
#endif
		}
	}
	else {
		// Store NSF name
		if (m_strFileName.GetLength() != 0 && m_bExport == true) {
			m_strExportFile = CString(pszParam);
			return;
		}
	}

	// Call default implementation
	CCommandLineInfo::ParseParam(pszParam, bFlag, bLast);
}
