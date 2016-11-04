/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005  Jonathan Liss
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

const int VERSION_MAJ = 0;
const int VERSION_MIN = 2;
const int VERSION_REV = 2;

#define LIMIT(v, max, min) if (v > max) v = max; else if (v < min) v = min;

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

struct stChanNote {
	int		Note;
	int		Octave;
	int		Vol;
	int		Instrument;
	int		ExtraStuff1;				// For the third column
	int		ExtraStuff2;
};

#include "SoundGen.h"

// CFamiTrackerApp:
// See FamiTracker.cpp for the implementation of this class
//

class CFamiTrackerApp : public CWinApp
{
public:
	CFamiTrackerApp();
	void PlayNote(int Channel, stChanNote *Note);
	void SilentEverything();
	void SendBackSyncTick();
	void LoadSoundConfig();
	void SetMachineType(int Type, int Rate);
	void DrawSamples(int *Samples, int Count);
	bool TickEvent(void);
	void MidiEvent(void);

	unsigned int GetOutput(int Chan);

	int GetCPUUsage();
	int GetFrameRate();
	int GetUnderruns();

	char m_cAppPath[MAX_PATH];

	int m_iFrameRate;

private:
	CSoundGen	*pSoundGen;


// Overrides
public:
	virtual BOOL InitInstance();

	void	ShutDownSynth();

	// Program settings
	int		m_iSampleRate;
	int		m_iSampleSize;
	int		m_iBufferLength;

	int		m_iBassFilter;
	int		m_iTrebleFilter;
	int		m_iTrebleDamping;

	bool	m_bWrapCursor;
	bool	m_bFreeCursorEdit;
	bool	m_bWavePreview;
	bool	m_bKeyRepeat;

	int		m_iMidiDevice;
	bool	m_bMidiMasterSync;
	bool	m_bMidiKeyRelease;
	bool	m_bMidiChannelMap;
	bool	m_bMidiVelocity;

	CString		m_strFont;

	CDocument	*pDocument;
	CView		*pView;

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
	//CSoundGen SoundGen;
//	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void LoadSettings(void);
	void SaveSettings(void);
	void DefaultSettings(void);
	virtual int ExitInstance();
//	virtual BOOL PreTranslateMessage(MSG* pMsg);
};

extern CFamiTrackerApp theApp;