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

#pragma once

//
// This thread will take care of the NES sound generation
//

#include "apu/APU.h"
#include "DirectSound.h"

const int VIBRATO_LENGTH = 64;
const int TREMOLO_LENGTH = 64;

// Custom messages
#define M_SILENT_ALL	WM_USER + 1
#define M_LOAD_SETTINGS WM_USER + 2

struct stVolLevels {
	int Chan1, Chan2, Chan3, Chan4, Chan5;
};

class CChannelCommon
{
public:

private:

};

class CChannelVRC6 
{
	public:
		unsigned char m_cVolume;
		unsigned char m_cNote;
		unsigned char m_cOctave;
	private:

};

class CSoundGen : public CWinThread, ICallback
{
protected:
	DECLARE_DYNCREATE(CSoundGen)
public:
	CSoundGen();
	~CSoundGen();

// Sound variables
private:
	unsigned int		m_iSampleSize;				// Size of samples, in bits
	unsigned int		m_iBufSizeSamples;			// Buffer size in samples
	unsigned int		m_iBufSizeBytes;			// Buffer size in bytes
	unsigned int		m_iBufferPtr;				// This will point in samples
	void				*m_pAccumBuffer;
	int32				*m_iGraphBuffer;

	int					m_iBassFreq;
	int					m_iTrebleFreq;
	int					m_iTrebleDamp;

	CDSound				*m_pDSound;
	CDSoundChannel		*m_pDSoundChannel;

// Tracker playing variables
private:
	CFamiTrackerDoc		*pDocument;
	CFamiTrackerView	*pTrackerView;

	CAPU				*m_pAPU;
	CSampleMem			m_SampleMem;

	stVolLevels			Levels;

	unsigned int		m_iCycles;							// Cycles between each APU update
	
	unsigned int		m_iTempo, m_iSpeed;		// Tempo and speed
	int					m_iTempoAccum;			// Used for speed calculation
	unsigned int		m_iTickPeriod;
	unsigned int		m_iPlayTime;
	bool				m_bNewRow;
	bool				m_bPlaying, m_bPlayLooping;

	int					m_iJumpToPattern;
	int					m_iSkipToRow;

	unsigned int		*m_pNoteLookupTable;
	unsigned int		m_iNoteLookupTable_NTSC[12 * 8];
	unsigned int		m_iNoteLookupTable_PAL[12 * 8];
	unsigned int		m_iMachineType;						// NTSC/PAL
	bool				m_bRunning;

	unsigned char		m_cVibTable[VIBRATO_LENGTH];
	unsigned char		m_cTremTable[TREMOLO_LENGTH];

	// General variables
	HANDLE				m_hNotificationEvent;
	HWND				m_hWnd;

// Functions
private:
//	void				KillChannel(int Channel);
//	void				KillChannelHard(int Channel);
//	void				PlayChannel(int Channel, stChanNote *NoteData, int EffColumns);
//	void				PlayChannelVRC6(int Channel, stChanNote *NoteData, int EffColumns);

	void				PlayNote(int Channel, stChanNote *NoteData, int EffColumns);

//	void				RefreshChannel(int Channel);
	void				ResetAPU();

	void				RunFrame();

public:
	virtual BOOL		InitInstance();
	virtual int			ExitInstance();
	virtual int			Run();

	void				SetDocument(CFamiTrackerDoc *pDoc, CFamiTrackerView *pView);
	void				LoadMachineSettings(int Machine, int Rate);

	// Player
	void				StartPlayer(bool Looping);
	void				StopPlayer();
	void				ResetTempo();
	int					GetTempo();
//	void				UpdateTempo(unsigned int Value);

	bool				IsPlaying() { return m_bPlaying; };

	// Sound
	bool				InitializeSound(HWND hWnd);
	bool				ResetSound();
	void				FlushBuffer(int16 *Buffer, uint32 Size);
	
	unsigned int		GetUnderruns();
	CDSound				*GetSoundInterface()	{ return m_pDSound; };

	// Tracker playing
	bool				IsRunning()				{ return m_bRunning; }
	unsigned int		GetOutput(int Chan);
};

extern CSoundGen SoundGenerator;
