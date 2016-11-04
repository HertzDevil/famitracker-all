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
// Notes are sent to the thread as standard window messages
//

#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "../sound driver/APU.h"
#include "../sound driver/DSound.h"

const int VIBRATO_LENGTH = 64;
const int TREMOLO_LENGTH = 64;

struct stVolLevels {
	int Chan1, Chan2, Chan3, Chan4, Chan5;
};

struct stChanData {
	unsigned int Enabled;
	unsigned int OutVol, InitVol;
	unsigned int Length;
	unsigned int LastFreq;
	unsigned int Freq;

	unsigned char Sweep;

	unsigned char DutyCycle;

	unsigned int Note, LastNote;
	unsigned int Octave;

	unsigned int PortaTo;
	unsigned int PortaSpeed;

	unsigned int Volume;

	int LastInstrument, Instrument;

	int ModEnable[MOD_COUNT];
	int ModIndex[MOD_COUNT];
	int ModDelay[MOD_COUNT];
	int ModPointer[MOD_COUNT];

	unsigned int VibratoDepth, VibratoSpeed, VibratoPhase;
	unsigned int TremoloDepth, TremoloSpeed, TremoloPhase;
	signed char TremoloAcc;

	unsigned char Effect, EffParam, EffVar;
};

class CSoundGen : public CWinThread, ICallback
{
protected:
	DECLARE_DYNCREATE(CSoundGen)
public:
	CSoundGen();
	~CSoundGen();
private:
	CFamiTrackerDoc		*pDocument;
	CFamiTrackerView	*pTrackerView;

	unsigned int	m_iSampleRate;
	unsigned int	m_iSampleSize;
	unsigned int	m_iBufferLen;						// Buffer length in ms
	unsigned int	m_iBufferSize;						// Buffer size in bytes
	uint32			m_iBufferPtr;

	int				m_iBassFreq;
	int				m_iTrebleFreq;
	int				m_iTrebleDamp;

	unsigned int	m_iCycles;							// Cycles between each APU update
	
	bool			m_bNewSettings;

	unsigned int	*m_pNoteLookupTable;
	unsigned int	m_iNoteLookupTable_NTSC[12 * 8];
	unsigned int	m_iNoteLookupTable_PAL[12 * 8];
	unsigned int	m_iMachineType;						// NTSC/PAL
	bool			m_bRunning;

	unsigned char	m_cVibTable[VIBRATO_LENGTH];
	unsigned char	m_cTremTable[TREMOLO_LENGTH];

	void			*m_pAccumBuffer;
	int32			*m_iGraphBuffer;

	CAPU			*m_pAPU;

	CDSound			*m_pDSound;
	CDSoundChannel	*m_pDSoundChannel;
	stChanData		m_stChannels[5];
	CSampleMem		m_SampleMem;

	HANDLE			m_hNotificationEvent;

	void			KillChannel(int Chan);

public:
	virtual BOOL InitInstance();
	virtual int Run();
	void SetDocument(CFamiTrackerDoc *pDoc, CFamiTrackerView *pView);
	void PlayChannel(int Channel, stChanNote *NoteData, int EffColumns);
	void ModifyChannel(int Channel);
	void RefreshChannel(int Channel);
	void LoadMachineSettings(int Machine, int Rate);

	// Sound
	bool InitializeSound(HWND hWnd);
	bool LoadBuffer();
	void LoadSettings(int SampleRate, int SampleSize, int BufferLength);

	void FlushBuffer(int16 *Buffer, uint32 Size);

	bool IsRunning() { return m_bRunning; }

	unsigned int GetOutput(int Chan);

	stVolLevels Levels;
	virtual int ExitInstance();
};