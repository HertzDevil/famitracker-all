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
#include "MIDI.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern int Underruns;		// remove this
bool ShuttingDown;
bool Initialized;

// CFamiTrackerApp

BEGIN_MESSAGE_MAP(CFamiTrackerApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()


// CFamiTrackerApp construction

CFamiTrackerApp::CFamiTrackerApp()
{
	EnableHtmlHelp();

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CFamiTrackerApp object

CFamiTrackerApp theApp;
static CMainFrame *pMainFrame;
static CFamiTrackerView *pTrackerView;

// CFamiTrackerApp initialization

BOOL CFamiTrackerApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	Initialized = false;

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

	m_pSettings = new CSettings;
	m_pSettings->LoadSettings();

	// Create a synth
	pSoundGen = new CSoundGen;

	// Create the MIDI interface
	pMIDI = new CMIDI();

	// Allow SoundGen access to the document, for instruments and things
	pSoundGen->LoadSettings(m_pSettings->Sound.iSampleRate, m_pSettings->Sound.iSampleSize, m_pSettings->Sound.iBufferLength);

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CFamiTrackerDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CFamiTrackerView));
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

	POSITION Pos			= pDocTemplate->GetFirstDocPosition();
	CFamiTrackerDoc *pDoc	= (CFamiTrackerDoc*)pDocTemplate->GetNextDoc(Pos);

	Pos				= pDoc->GetFirstViewPosition();
	pTrackerView	= (CFamiTrackerView*)pDoc->GetNextView(Pos);
	pMainFrame		= (CMainFrame*)((pTrackerView)->GetParentFrame());

	pDocument	= pTrackerView->GetDocument();
	pView		= pTrackerView;

	if (!pSoundGen->InitializeSound(GetMainWnd()->m_hWnd)) {
		delete pSoundGen;
		// If failed, restore default settings
		m_pSettings->DefaultSettings();
		m_pSettings->SaveSettings();
		// Quit program
		AfxMessageBox("Program could not load properly, default settings has been restored. Restart the program.");
		return FALSE;
	}

	pSoundGen->SetDocument(pDoc, pTrackerView);

	m_iFrameRate	= 0;
	ShuttingDown	= false;
	Initialized		= true;

	GetCurrentDirectory(256, m_cAppPath);

	// Initialize sound generator
	if (pSoundGen->CreateThread() == 0) {
		AfxMessageBox("Couldn't start sound generator thread.");
		return FALSE;
	}
	
//	pTrackerView->SetupMidi();
	pMIDI->Init();

	return TRUE;
}

void CFamiTrackerApp::LoadSoundConfig()
{
	pSoundGen->LoadSettings(m_pSettings->Sound.iSampleRate, m_pSettings->Sound.iSampleSize, m_pSettings->Sound.iBufferLength);
}

void CFamiTrackerApp::SetMachineType(int Type, int Rate)
{
	pSoundGen->LoadMachineSettings(Type, Rate);
}

CMIDI *CFamiTrackerApp::GetMIDI()
{
	return pMIDI;
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
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


// CFamiTrackerApp message handlers

void CFamiTrackerApp::SilentEverything()
{
	pSoundGen->PostThreadMessage(WM_USER + 1, 0, 0);
}

void CFamiTrackerApp::DrawSamples(int *Samples, int Count)
{	
	pMainFrame->DrawSamples(Samples, Count);
}

void CFamiTrackerApp::ShutDownSynth()
{
	// Shut down sound generator

	bool	ThreadClosed = false;
	int		i;

	ShuttingDown = true;

	pSoundGen->PostThreadMessage(WM_QUIT, 0, 0);

	for (i = 0; i < 50; i++) {
		if (pSoundGen->IsRunning() == false) {
			ThreadClosed = true;
			break;
		}
		else
			Sleep(10);
	}

	if (!ThreadClosed)
		AfxMessageBox("Could not close sound generator thread!");
	else
		delete pSoundGen;
}

int CFamiTrackerApp::GetCPUUsage()
{
	static FILETIME KernelLastTime[2], UserLastTime[2];
	FILETIME CreationTime[2], ExitTime[2], KernelTime[2], UserTime[2];
	unsigned int TotalTime[2];

	HANDLE hThreads[2] = {m_hThread, pSoundGen->m_hThread};

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

int CFamiTrackerApp::GetFrameRate()
{
	int RetVal = m_iFrameRate;
	m_iFrameRate = 0;
	return RetVal;
}

void CFamiTrackerApp::StepFrame()
{
	m_iFrameRate++;
}

int CFamiTrackerApp::GetUnderruns()
{
	return Underruns;
}

int CFamiTrackerApp::ExitInstance()
{
	pMIDI->Shutdown();
	pMIDI->CloseDevice();
	m_pSettings->SaveSettings();

	delete [] pMIDI;

	return CWinApp::ExitInstance();
}

void CFamiTrackerApp::MidiEvent(void)
{
	if (pTrackerView->m_bInitialized)
		pTrackerView->PostMessage(WM_USER + 1);
}

unsigned int CFamiTrackerApp::GetOutput(int Chan)
{
	if (!Initialized)
		return 0;

	return pSoundGen->GetOutput(Chan);
}

BOOL CAboutDlg::OnInitDialog()
{
	CString Text;

	CDialog::OnInitDialog();

	Text.Format("FamiTracker Version %i.%i.%i Beta\n\nA Famicom/NES music tracker", VERSION_MAJ, VERSION_MIN, VERSION_REV);

	SetDlgItemText(IDC_ABOUT, Text);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CFamiTrackerApp::ReloadColorScheme(void)
{
	((CFamiTrackerView*)pView)->ForceRedraw();
	((CMainFrame*)(pView->GetParentFrame()))->SetupColors();
}
