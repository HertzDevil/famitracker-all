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
const int VERSION_REV = 5;

const int VERSION_WIP = 2;

#define LIMIT(v, max, min) if (v > max) v = max; else if (v < min) v = min;

#define DIM(c, l) ( (((c & 0xFF) * l) / 100) + (((((c >> 8) & 0xFF) * l) / 100) << 8) + (((((c >> 16) & 0xFF) * l) / 100) << 16))

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
};

extern CFamiTrackerApp theApp;