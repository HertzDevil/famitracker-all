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

#pragma once

//
// This thread will take care of the NES sound generation
// Notes are sent to the thread as standard window messages
//

#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "player/APU.h"
#include "player/DSound.h"

struct stVolLevels {
	int Chan1, Chan2, Chan3, Chan4, Chan5;
};

struct stChanData {
	unsigned int Enabled;
	unsigned int OutVol, InitVol;
	unsigned int Length;
	unsigned int LastFreq;
	int	Freq;

	unsigned char Sweep;

	unsigned char DutyCycle;

	unsigned int Note, LastNote;
	unsigned int Octave;

	unsigned int PortaTo;
	unsigned int PortaSpeed;

	unsigned int Volume;

	int LastInstrument;

	int ModEnable[MOD_COUNT];
	int ModIndex[MOD_COUNT];
	int ModDelay[MOD_COUNT];
	int ModPointer[MOD_COUNT];
};

class CSoundGen : public CWinThread, ICallback
{
protected:
	DECLARE_DYNCREATE(CSoundGen)
public:
	CSoundGen();
	~CSoundGen();
private:
	CFamiTrackerDoc *pDocument;
	CFamiTrackerView *pView;
	int				m_iSampleRate;
	unsigned int	m_iSampleSize;
	unsigned int	m_iBufferLen;						// Buffer length in ms
	unsigned int	m_iBufferSize;						// Buffer size in bytes
	uint32			m_iBufferPtr;

	int				m_iBassFreq;
	int				m_iTrebleFreq;
	int				m_iTrebleDamp;

	unsigned int	m_iCycles;							// Cycles between each APU update
	
	bool			m_bNewSettings;

	int				*NoteLookupTable;
	int				m_iNoteLookupTable_NTSC[12 * 8];
	int				m_iNoteLookupTable_PAL[12 * 8];
	int				m_iMachineType;						// NTSC/PAL
	bool			m_bRunning;

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
	void SetDocument(CFamiTrackerDoc *pDoc);
	void PlayChannel(int Channel, stChanNote *NoteData);
	void ModifyChannel(int Channel);
	void RefreshChannel(int Channel);
	void LoadMachineSettings(int Machine, int Rate);

	// Sound
	bool InitializeSound(HWND hWnd);
	bool LoadBuffer();
	void LoadSettings(int SampleRate, int SampleSize, int BufferLength);

	void FlushBuffer(int32 *Buffer, uint32 Size);

	bool IsRunning() { return m_bRunning; }

	unsigned int GetOutput(int Chan);

	stVolLevels Levels;
	virtual int ExitInstance();
};