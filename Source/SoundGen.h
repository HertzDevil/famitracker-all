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

#pragma once

//
// This thread will take care of the NES sound generation
//

#include "apu/apu.h"
#include "DirectSound.h"
#include "WaveFile.h"

#define NEW_VIBRATO

#ifdef NEW_VIBRATO

const int VIBRATO_LENGTH = /*64*/ 256;
const int TREMOLO_LENGTH = 256;//64;

#else

const int VIBRATO_LENGTH = 64;
const int TREMOLO_LENGTH = 64;

#endif

const unsigned int TOTAL_CHANNELS = MAX_CHANNELS;

// Custom messages
enum { M_SILENT_ALL = WM_USER + 1,
	   M_LOAD_SETTINGS,
	   M_PLAY,
	   M_PLAY_LOOPING,
	   M_STOP,
	   M_RESET,
	   M_START_RENDER,
	   M_PREVIEW_SAMPLE,
	   M_WRITE_APU};

enum {MODE_PLAY,
	  MODE_PLAY_START,
	  MODE_PLAY_REPEAT,
	  MODE_PLAY_CURSOR,
	  MODE_PLAY_FRAME};

typedef enum { SONG_TIME_LIMIT, SONG_LOOP_LIMIT } RENDER_END;

// Used to get the DPCM state
struct stDPCMState {
	int SamplePos;
	int DeltaCntr;
};

class CChannelHandler;
class CFamiTrackerView;

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

	int					m_iAudioUnderruns;

	CDSound				*m_pDSound;
	CDSoundChannel		*m_pDSoundChannel;

// Tracker playing variables
private:
	CFamiTrackerDoc		*pDocument;
	CFamiTrackerView	*pTrackerView;

	CAPU				*m_pAPU;
	CSampleMem			m_SampleMem;

//	stVolLevels			Levels;

	unsigned int		m_iCycles;							// Cycles between each APU update
	
	unsigned int		m_iTempo, m_iSpeed;		// Tempo and speed
	int					m_iTempoAccum;			// Used for speed calculation
	unsigned int		m_iTickPeriod;
	unsigned int		m_iPlayTime;
	bool				m_bPlaying, m_bPlayLooping;

	int					m_iJumpToPattern;
	int					m_iSkipToRow;
	int					m_iStepRows;

	unsigned int		*m_pNoteLookupTable;
	unsigned int		m_iNoteLookupTable[96];				// 12 notes / 8 octaves
	unsigned int		m_iNoteLookupTableSaw[96];			// For VRC6 sawtooth
	unsigned int		m_iNoteLookupTableFDS[96];			// For FDS

	unsigned int		m_iMachineType;						// NTSC/PAL
	bool				m_bRunning;

	int					m_iPlayMode;

	int					m_cVibTable[VIBRATO_LENGTH];
	unsigned char		m_cTremTable[TREMOLO_LENGTH];

	// General variables
	HANDLE				m_hNotificationEvent;
	HANDLE				m_hAliveCheck;
	HWND				m_hWnd;

	// Rendering
	RENDER_END			m_iRenderEndWhen;
	int					m_iRenderEndParam;
	int					m_iRenderedFrames;
	int					m_iRenderedSong;
	int					m_iDelayedStart;
	int					m_iDelayedEnd;

	int					m_iTempoDecrement;
	bool				m_bUpdateRow;

	bool				m_bRendering;
	bool				m_bRequestRenderStop;
	bool				m_bPlayerHalted;

	CWaveFile			m_WaveFile;

// Functions
private:
	void				PlayNote(int Channel, stChanNote *NoteData, int EffColumns);
	void				ResetAPU();
	void				RunFrame();
	void				CheckControl();
	void				ResetBuffer();
	void				BeginPlayer(/*bool bLooping*/);
// Initialization
	void				CreateChannels();
	void				AssignChannel(CTrackerChannel *pTrackerChannel, CChannelHandler *pRenderer);

	// Misc
	void				PlaySample(CDSample *pSample, int Offset, int Pitch);

	void				HandleMessages();

public:
	// Interface from outside the thread
	void				StartPlayer(/*bool Looping*/ int Mode);	
	void				StopPlayer();
	void				ResetPlayer();
	void				ResetTempo();
	unsigned int		GetTempo();
	bool				IsPlaying() { return m_bPlaying; };


public:
	virtual BOOL		InitInstance();
	virtual int			ExitInstance();
	virtual int			Run();
	void				SetupChannels(int Chip, CFamiTrackerDoc *pDoc);
	bool				CreateAPU();
	void				GenerateVibratoTable(int Type);
	int					ReadVibTable(int index);

	void				SetDocument(CFamiTrackerDoc *pDoc, CFamiTrackerView *pView);
	void				LoadMachineSettings(int Machine, int Rate);

	void				EvaluateGlobalEffects(stChanNote *NoteData, int EffColumns);

	// Sound
	bool				InitializeSound(HWND hWnd, HANDLE hAliveCheck, HANDLE hNotification);
	bool				ResetSound();
	void				FlushBuffer(int16 *Buffer, uint32 Size);
	
	unsigned int		GetUnderruns();
	CDSound				*GetSoundInterface() { return m_pDSound; };

	// Tracker playing
	bool				IsRunning()	{ return m_bRunning; }
	stDPCMState			GetDPCMState();

	// Rendering
	bool				RenderToFile(char *File, int SongEndType, int SongEndParam);
	void				StopRendering();
	void				GetRenderStat(int &Frame, int &Time, bool &Done);
	bool				IsRendering();
	void				CheckRenderStop();

	void				SongIsDone();
	void				FrameIsDone(int SkipFrames);

	// Misc
	void				PreviewSample(CDSample *pSample, int Offset, int Pitch);
	bool				PreviewDone() const;

	void				WriteAPU(int Address, char Value);

private:
	// Channels
	CChannelHandler		*m_pChannels[CHANNELS];
	CTrackerChannel		*m_pTrackerChannels[CHANNELS];

	CDSample			*m_pPreviewSample;

};

extern CSoundGen SoundGenerator;
