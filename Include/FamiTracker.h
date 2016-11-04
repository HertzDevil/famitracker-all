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

// FamiTracker.h : main header file for the FamiTracker application
//

#pragma once

//#define WIP

const int VERSION_MAJ = 0;
const int VERSION_MIN = 2;
const int VERSION_REV = 6;

const int VERSION_WIP = 0;

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

//#include "../sound driver/SoundGen.h"

#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"

class CMIDI;

// CFamiTrackerApp:
// See FamiTracker.cpp for the implementation of this class
//

class CFamiTrackerApp : public CWinApp
{
public:
	CFamiTrackerApp();
	void DisplayError(int Message);
	void SilentEverything();
	void LoadSoundConfig();
	void SetMachineType(int Type, int Rate);
	void DrawSamples(int *Samples, int Count);
	void MidiEvent(void);

	unsigned int GetOutput(int Chan);
	
	void RegisterKeyState(int Channel, int Note);

	int GetCPUUsage();
	int GetFrameRate();
	int GetUnderruns();

	void StepFrame();

	void StopPlayer();
//	void PlayTracker();
//	void StopTracker();
	int GetTempo();

	char m_cAppPath[MAX_PATH];

	int m_iFrameRate;

private:
//	CSoundGen	*pSoundGen;
	CMIDI		*pMIDI;

// Overrides
public:
	virtual BOOL InitInstance();

	void	ShutDownSynth();
	void	ReloadColorScheme(void);

	void	*GetSoundGenerator();

	CDocument	*pDocument;
	CView		*pView;

	CMIDI		*GetMIDI();

	// Program settings
	CSettings	*m_pSettings;

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
};

extern CFamiTrackerApp theApp;