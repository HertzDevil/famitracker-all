/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2007  Jonathan Liss
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
#include "MIDI.h"
#include "AboutBox.h"
#include "SoundGen.h"
#include "Accelerator.h"

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
	ON_COMMAND(ID_TRACKER_STOP, OnTrackerStop)
	ON_COMMAND(ID_TRACKER_TOGGLE_PLAY, OnTrackerTogglePlay)
	ON_COMMAND(ID_TRACKER_PLAYPATTERN, OnTrackerPlaypattern)
	ON_COMMAND(ID_EDIT_ENABLEMIDI, OnEditEnableMIDI)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_PLAY, OnUpdateTrackerPlay)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_STOP, OnUpdateTrackerStop)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_PLAYPATTERN, OnUpdateTrackerPlay)
	ON_UPDATE_COMMAND_UI(ID_EDIT_ENABLEMIDI, OnUpdateEditEnablemidi)
END_MESSAGE_MAP()

// Include this for windows xp style in visual studio 2005
#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")

// CFamiTrackerApp construction

CFamiTrackerApp::CFamiTrackerApp()
{
	// Place all significant initialization in InitInstance
	EnableHtmlHelp();
	m_bInitialized = false;
	m_bShuttingDown	= false;
	m_bDocLoaded = false;
	m_bThemeActive = false;
}


// The one and only CFamiTrackerApp object
CFamiTrackerApp			theApp;

static CMainFrame		*pMainFrame;
static CFamiTrackerView	*pTrackerView;

// CFamiTrackerApp initialization

void CFamiTrackerApp::OnFileOpen() 
{
	// Overloaded this for separate saving of the ftm-path
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
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views

	// Load settings
	m_pSettings = new CSettings;
	m_pSettings->LoadSettings();

	// Load custom accelerator
	m_pAccel = new CAccelerator;
	m_pAccel->LoadShortcuts(m_pSettings);

	// Create the MIDI interface
	pMIDI = new CMIDI();

	// Sound generator
	m_pSoundGenerator = new CSoundGen();

	CSingleDocTemplate* pDocTemplate;

	pDocTemplate = new CSingleDocTemplate(IDR_MAINFRAME, RUNTIME_CLASS(CFamiTrackerDoc), RUNTIME_CLASS(CMainFrame), RUNTIME_CLASS(CFamiTrackerView));
	
	if (!pDocTemplate)
		return FALSE;

	AddDocTemplate(pDocTemplate);

	// Enable DDE Execute open
	EnableShellOpen();

	// Kolla upp det här 
	RegisterShellFileTypes(TRUE);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Setup document/view
	POSITION Pos			= pDocTemplate->GetFirstDocPosition();
	CFamiTrackerDoc *pDoc	= (CFamiTrackerDoc*)pDocTemplate->GetNextDoc(Pos);

	Pos				= pDoc->GetFirstViewPosition();
	pTrackerView	= (CFamiTrackerView*)pDoc->GetNextView(Pos);
	pMainFrame		= (CMainFrame*)((pTrackerView)->GetParentFrame());

	pTrackerView->SetSoundGen(m_pSoundGenerator);

	pDocument	= pTrackerView->GetDocument();
	pView		= pTrackerView;

	// Initialize the sound generator
	if (!m_pSoundGenerator->InitializeSound(GetMainWnd()->m_hWnd)) {	
		// If failed, restore and save default settings
		m_pSettings->DefaultSettings();
		m_pSettings->SaveSettings();
		// Quit program
		DisplayError(IDS_START_ERROR);
		return FALSE;
	}

	m_pSoundGenerator->SetDocument(pDoc, pTrackerView);

	m_iFrameRate = 0;

	GetCurrentDirectory(256, m_cAppPath);

	// Start sound generator
	if (!m_pSoundGenerator->CreateThread()) {
		// If failed, restore and save default settings
		m_pSettings->DefaultSettings();
		m_pSettings->SaveSettings();
		// Show message and quit
		DisplayError(IDS_SOUNDGEN_ERROR);
		return FALSE;
	}
	
	pMIDI->Init();

	HMODULE hinstDll;

	// Check if the application is themed
	hinstDll = ::LoadLibrary(_T("UxTheme.dll"));
	
	if (hinstDll) {
		typedef BOOL (*ISAPPTHEMEDPROC)();
		ISAPPTHEMEDPROC pIsAppThemed;
		pIsAppThemed = (ISAPPTHEMEDPROC) ::GetProcAddress(hinstDll, "IsAppThemed");

		if(pIsAppThemed)
			m_bThemeActive = (pIsAppThemed() == TRUE);

		::FreeLibrary(hinstDll);
	}

	m_bInitialized = true;

	return TRUE;
}

// Close program
int CFamiTrackerApp::ExitInstance()
{
	if (!m_bInitialized)
		return CWinApp::ExitInstance();

	ShutDownSynth();

	pMIDI->CloseDevice();
	pMIDI->Shutdown();
	m_pAccel->SaveShortcuts(m_pSettings);
	m_pSettings->SaveSettings();

	delete pMIDI;
	delete m_pAccel;
	delete m_pSettings;

	return CWinApp::ExitInstance();
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

	::DuplicateHandle(0, m_pSoundGenerator->m_hThread, 0, &hThread, 0, FALSE, 0);

	m_pSoundGenerator->PostThreadMessage(WM_QUIT, 0, 0);

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


// Display an error message from the string lib
void CFamiTrackerApp::DisplayError(int Message)
{
	CComBSTR ErrorMsg;
	ErrorMsg.LoadString(Message);
	AfxMessageBox(CW2CT(ErrorMsg), MB_ICONERROR, 0);
}


// Return sound gen
void *CFamiTrackerApp::GetSoundGenerator()
{
	return m_pSoundGenerator;
}

// Load sound configuration
void CFamiTrackerApp::LoadSoundConfig()
{
	m_pSoundGenerator->PostThreadMessage(M_LOAD_SETTINGS, 0, 0);
}

// Get the midi interface
CMIDI *CFamiTrackerApp::GetMIDI()
{
	return pMIDI;
}

// Command messages

void CFamiTrackerApp::OnEditEnableMIDI()
{
	pMIDI->Toggle();
}

void CFamiTrackerApp::OnUpdateEditEnablemidi(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(pMIDI->IsAvailable());
	pCmdUI->SetCheck(pMIDI->IsOpened());
}

////////////////////////////////////////////////////////
//  Things that belongs to the synth are kept below!  //
////////////////////////////////////////////////////////

// Silences everything
void CFamiTrackerApp::SilentEverything()
{
	if (m_bInitialized)
		m_pSoundGenerator->PostThreadMessage(M_SILENT_ALL, 0, 0);
}

// Machine type, NTSC/PAL
void CFamiTrackerApp::SetMachineType(int Type, int Rate)
{
	m_pSoundGenerator->LoadMachineSettings(Type, Rate);
}

// Volume level
unsigned int CFamiTrackerApp::GetOutput(int Chan)
{
	if (!m_bInitialized || m_bShuttingDown)
		return 0;

	return m_pSoundGenerator->GetOutput(Chan);
}

void CFamiTrackerApp::RegisterKeyState(int Channel, int Note)
{
	static_cast<CFamiTrackerView*>(pView)->RegisterKeyState(Channel, Note);
}

// Read and reset frame rate
int CFamiTrackerApp::GetFrameRate()
{
	int RetVal = m_iFrameRate;
	m_iFrameRate = 0;
	return RetVal;
}

// Increases frame
void CFamiTrackerApp::StepFrame()
{
	m_iFrameRate++;
}

int CFamiTrackerApp::GetUnderruns()
{
	return m_pSoundGenerator->GetUnderruns();
}

int CFamiTrackerApp::GetTempo()
{
	return m_pSoundGenerator->GetTempo();
}

void CFamiTrackerApp::ResetTempo()
{
	if (m_bInitialized)
		m_pSoundGenerator->ResetTempo();
}

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void CFamiTrackerApp::OnAppAbout()
{
	CAboutBox aboutDlg;
	aboutDlg.DoModal();
}

// CFamiTrackerApp message handlers

void CFamiTrackerApp::DrawSamples(int *Samples, int Count)
{	
	pMainFrame->DrawSamples(Samples, Count);
}

int CFamiTrackerApp::GetCPUUsage()
{
	static FILETIME KernelLastTime[2], UserLastTime[2];
	FILETIME CreationTime[2], ExitTime[2], KernelTime[2], UserTime[2];
	unsigned int TotalTime[2];

	HANDLE hThreads[2] = {m_hThread, m_pSoundGenerator->m_hThread};

	GetThreadTimes(hThreads[0], CreationTime + 0, ExitTime + 0, KernelTime + 0, UserTime + 0);
	GetThreadTimes(hThreads[1], CreationTime + 1, ExitTime + 1, KernelTime + 1, UserTime + 1);

	TotalTime[0]		= (KernelTime[0].dwLowDateTime - KernelLastTime[0].dwLowDateTime) / 1000;
	TotalTime[1]		= (KernelTime[1].dwLowDateTime - KernelLastTime[1].dwLowDateTime) / 1000;
	TotalTime[0]		+= (UserTime[0].dwLowDateTime - UserLastTime[0].dwLowDateTime) / 1000;
	TotalTime[1]		+= (UserTime[1].dwLowDateTime - UserLastTime[1].dwLowDateTime) / 1000;
	KernelLastTime[0]	= KernelTime[0];
	KernelLastTime[1]	= KernelTime[1];
	UserLastTime[0]		= UserTime[0];
	UserLastTime[1]		= UserTime[1];

	return TotalTime[0] + TotalTime[1];
}

void CFamiTrackerApp::MidiEvent(void)
{
	pTrackerView->PostMessage(WM_USER + 1);
}

BOOL CAboutDlg::OnInitDialog()
{
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CFamiTrackerApp::ReloadColorScheme(void)
{
	((CFamiTrackerView*)pView)->ForceRedraw();
	((CMainFrame*)(pView->GetParentFrame()))->SetupColors();
}

void CFamiTrackerApp::OnTrackerPlay()
{
	m_pSoundGenerator->StartPlayer(false);
}

void CFamiTrackerApp::OnTrackerStop()
{
	m_pSoundGenerator->StopPlayer();
	pMIDI->ResetOutput();
}

void CFamiTrackerApp::OnUpdateTrackerPlay(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_pSoundGenerator->IsPlaying() ? 1 : 0);
}

void CFamiTrackerApp::OnUpdateTrackerStop(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pSoundGenerator->IsPlaying() ? 1 : 0);
}

void CFamiTrackerApp::OnTrackerTogglePlay()
{
	if (m_pSoundGenerator->IsPlaying())
		OnTrackerStop();
	else
		OnTrackerPlay();
}

void CFamiTrackerApp::OnTrackerPlaypattern()
{
	if (m_bInitialized)
		m_pSoundGenerator->StartPlayer(true);
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
	return CWinApp::PreTranslateMessage(pMsg);
}

CDocument *CFamiTrackerApp::GetFirstDocument()
{
	CDocTemplate	*pDocTemp;
	POSITION		TempPos, DocPos;
	
	TempPos		= GetFirstDocTemplatePosition();
	pDocTemp	= GetNextDocTemplate(TempPos);
	DocPos		= pDocTemp->GetFirstDocPosition();

	return pDocTemp->GetNextDoc(DocPos);
}
