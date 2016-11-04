/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2009  Jonathan Liss
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

// FamiTracker.h : main header file for the FamiTracker application
//

#pragma once

//#define WIP

const int VERSION_MAJ = 0;
const int VERSION_MIN = 3;
const int VERSION_REV = 0;

const int VERSION_WIP = 1;

#define LIMIT(v, max, min) if (v > max) v = max; else if (v < min) v = min;

// Color macros

#define RED(x)		((x >> 16) & 0xFF)
#define GREEN(x)	((x >> 8) & 0xFF)
#define BLUE(x)		(x & 0xFF)

#define COMBINE(r, g, b) (((r) << 16) | ((g) << 8) | b)

//#define DIM(c, l) ( (((c & 0xFF) * l) / 100) + (((((c >> 8) & 0xFF) * l) / 100) << 8) + (((((c >> 16) & 0xFF) * l) / 100) << 16))
#define DIM(c, l) (COMBINE((RED(c) * l) / 100, (GREEN(c) * l) / 100, (BLUE(c) * l) / 100))

#define DIM_TO(c1, c2, level) (COMBINE((RED(c1) * level) / 100 + (RED(c2) * (100 - level)) / 100, \
									   (GREEN(c1) * level) / 100 + (GREEN(c2) * (100 - level)) / 100, \
									   (BLUE(c1) * level) / 100 + (BLUE(c2) * (100 - level)) / 100)) \

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "Settings.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "Accelerator.h"

class CMIDI;
class CSoundGen;

// CFamiTrackerApp:
// See FamiTracker.cpp for the implementation of this class
//

class CFamiTrackerApp : public CWinApp
{
public:
	// Constructor
	CFamiTrackerApp();

	// Different app-oriented functions
	void			DisplayError(int Message);
	void			LoadSoundConfig();
	void			ReloadColorScheme(void);
	void			SetMachineType(int Type, int Rate);
	void			DrawSamples(int *Samples, int Count);
	void			MidiEvent(void);
	int				GetCPUUsage();
	int				GetFrameRate();
	int				GetUnderruns();
	void			SetDocumentLoaded(bool Loaded);
	bool			IsDocLoaded() { return m_bDocLoaded; };
	bool			IsThemeActive() { return m_bThemeActive; };

	// Sound functions
	void			SilentEverything();
	void			ShutDownSynth();
	unsigned int	GetOutput(int Chan);

	// Tracker player functions
	void			RegisterKeyState(int Channel, int Note);
	void			StepFrame();
	void			StopPlayer();
	bool			IsPlaying();
	int				GetTempo();
	void			ResetTempo();
	void			ResetPlayer();

	void			CheckSynth();
	void			BufferUnderrun();

	// Different get-functions
	CAccelerator	*GetAccelerator() { return m_pAccel; };
	void			*GetSoundGenerator();
	CMIDI			*GetMIDI();
	CDocument		*GetDocument() { return pDocument; };
	CView			*GetView() { return pView; };
	CDocument		*GetFirstDocument();

	void SetSoundChip(int Chip);

	// Private variables and objects
private:
	CDocument		*pDocument;
	CView			*pView;
	CMIDI			*pMIDI;
	CAccelerator	*m_pAccel;
	CSoundGen		*m_pSoundGenerator;
	char			m_cAppPath[MAX_PATH];
	int				m_iFrameRate, m_iFrameCounter;
	bool			m_bShuttingDown;
	bool			m_bInitialized;
	bool			m_bDocLoaded;
	bool			m_bThemeActive;

	HANDLE			m_hAliveCheck;
	HANDLE			m_hNotificationEvent;

// Overrides
public:
	// Program settings. I'll leave this global to make things easier
	CSettings	*m_pSettings;

	virtual BOOL InitInstance();

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
	afx_msg void OnTrackerPlay();
	afx_msg void OnTrackerStop();
	afx_msg void OnUpdateTrackerPlay(CCmdUI *pCmdUI);
	afx_msg void OnUpdateTrackerStop(CCmdUI *pCmdUI);
	afx_msg void OnTrackerTogglePlay();
	afx_msg void OnTrackerPlaypattern();
	afx_msg void OnUpdateTrackerPlaypattern(CCmdUI *pCmdUI);
	afx_msg void OnFileOpen();
	afx_msg void OnEditEnableMIDI();
	afx_msg void OnUpdateEditEnablemidi(CCmdUI *pCmdUI);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};

extern CFamiTrackerApp theApp;