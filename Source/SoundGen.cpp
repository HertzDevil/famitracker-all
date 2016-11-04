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

//
// This file takes care of the NES sound playback
//

// Todo:
//  - Isolate and synchronize all thread communication
//

#include "stdafx.h"
#include <cmath>
#include <afxmt.h>
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "SoundGen.h"
#include "DirectSound.h"
#include "ChannelHandler.h"
#include "DSoundInterface.h"
#include "WaveFile.h"

IMPLEMENT_DYNCREATE(CSoundGen, CWinThread)

#define VRC6_INDEX(Channel) (Channel - CHANNELS_DEFAULT)

const int PITCH	= 1000;		// 100.0 %

//const int DEFAULT_CYCLES_NTSC	= (1364 / 12) * 262 / 2;
//const int DEFAULT_CYCLES_PAL	= (1264 / 12) * 315;		// <- may be inaccurate
const int BASE_FREQ_NTSC		= (1789773 * PITCH) / 1000;
const int BASE_FREQ_PAL			= (1662607 * PITCH) / 1000;

// move these to the class header

// 2A03/2A07
CSquare1Chan	Square1Channel;
CSquare2Chan	Square2Channel;
CTriangleChan	TriangleChannel;
CNoiseChan		NoiseChannel;
CDPCMChan		DPCMChannel;

// Konami VRC6
CVRC6Square1	VRC6Square1;
CVRC6Square2	VRC6Square2;
CVRC6Sawtooth	VRC6Sawtooth;

// Konami VRC7
CVRC7Channel	VRC7Channels[6];

int m_iTempoDecrement;
bool m_bUpdateRow;

CWaveFile m_WaveFile;
bool m_bRendering;
bool m_bRequestRenderStop;

CMutex ControlMutex;

bool m_bPlayerHalted;

CSoundGen::CSoundGen()
{
	m_pAPU				= NULL;
	m_pDSound			= NULL;
	m_pDSoundChannel	= NULL;
	m_pAccumBuffer		= NULL;
	m_iGraphBuffer		= NULL;

	m_bRendering = false;
	m_bPlaying = false;
}

CSoundGen::~CSoundGen()
{
}

//// Interface functions ///////////////////////////////////////////////////////////////////////////////////

void CSoundGen::StartPlayer(bool Looping)
{
	if (m_bPlaying || !pDocument->IsFileLoaded())
		return;

	if (Looping)
		PostThreadMessage(M_PLAY_LOOPING, 0, 0);
	else
		PostThreadMessage(M_PLAY, 0, 0);
}

void CSoundGen::StopPlayer()
{
	if (!m_bPlaying)
		return;

	PostThreadMessage(M_STOP, 0, 0);
}

void CSoundGen::ResetPlayer()
{
	PostThreadMessage(M_RESET, 0, 0);
}

//// Sound handling ////////////////////////////////////////////////////////////////////////////////////////

bool CSoundGen::CreateAPU()
{
	m_pAPU = new CAPU(this, &m_SampleMem);

	if (!m_pAPU)
		return false;

	for (int i = 0; i < 6; i++)
		VRC7Channels[i].ChannelIndex(i);

	//ControlMutex
	
	return true;
}

bool CSoundGen::InitializeSound(HWND hWnd, HANDLE hAliveCheck, HANDLE hNotification)
{
	// Initialize sound, this is only called once!
	// Start with NTSC by default

	ASSERT(m_pDSound == NULL);

	m_iMachineType			= SPEED_NTSC;
	m_hWnd					= hWnd;
	m_pDSound				= new CDSound;
	//m_hNotificationEvent	= CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hNotificationEvent	= hNotification;
	m_hAliveCheck			= hAliveCheck;

	// Out of memory
	if (!m_pDSound)
		return false;

	m_pDSound->EnumerateDevices();
	
	return true;
}

bool CSoundGen::ResetSound()
{
	// Setup sound, return false if failed
	//

	unsigned int SampleSize = theApp.m_pSettings->Sound.iSampleSize;
	unsigned int SampleRate = theApp.m_pSettings->Sound.iSampleRate;
	unsigned int BufferLen	= theApp.m_pSettings->Sound.iBufferLength;

	m_iSampleSize	= SampleSize;
	m_iBufferPtr	= 0;

	m_iAudioUnderruns = 0;

	// Allocate a direct sound interface
	if (m_pDSoundChannel)
		m_pDSound->CloseChannel(m_pDSoundChannel);

	// Reinitialize direct sound
	int DevID = m_pDSound->MatchDeviceID((char*)theApp.m_pSettings->strDevice.GetBuffer());
	m_pDSound->Init(m_hWnd, m_hNotificationEvent, DevID);

	int iBlocks = 2;

	// Create more blocks if a bigger buffer than 100ms is used to enhance program response
	if (BufferLen > 100)
		iBlocks = (BufferLen / 66);

	// Create channel
	m_pDSoundChannel = m_pDSound->OpenChannel(SampleRate, SampleSize, 1, BufferLen, iBlocks);

	// Channel failed
	if (m_pDSoundChannel == NULL)
		return false;

	// Create a buffer
	m_iBufSizeBytes	  = m_pDSoundChannel->GetBlockSize();
	m_iBufSizeSamples = m_iBufSizeBytes / (SampleSize / 8);

	if (m_pAccumBuffer)
		delete [] m_pAccumBuffer;

	m_pAccumBuffer = new char[m_iBufSizeBytes];

	// Out of memory
	if (!m_pAccumBuffer)
		return false;

	// Sample graph buffer
//	if (m_iGraphBuffer)
//		delete [] m_iGraphBuffer;

	m_iGraphBuffer = new int32[m_iBufSizeSamples];

	// Out of memory
	if (!m_iGraphBuffer)
		return false;

	if (!m_pAPU->SetupSound(SampleRate, 1, MACHINE_NTSC))
		return false;

	switch (m_iMachineType) {
		case NTSC:
			m_pAPU->ChangeMachine(MACHINE_NTSC);
			break;
		case PAL:
			m_pAPU->ChangeMachine(MACHINE_PAL);	
			break;
	}

	// Update blip-buffer filtering 
	m_pAPU->SetupMixer(theApp.m_pSettings->Sound.iBassFilter, 
		theApp.m_pSettings->Sound.iTrebleFilter, 
		theApp.m_pSettings->Sound.iTrebleDamping,
		theApp.m_pSettings->Sound.iMixVolume);

	return true;
}

void CSoundGen::ResetBuffer()
{
	m_iBufferPtr = 0;
	m_pDSoundChannel->Clear();
	//m_pAPU->EndFrame();

	m_pAPU->Reset();
	//m_pAPU->ResetBuffer();

}

void CSoundGen::FlushBuffer(int16 *Buffer, uint32 Size)
{
	// Called when a buffer is filled enough for playing
	//

	const int SAMPLE_MAX = 32767;
	const int SAMPLE_MIN = -32768;
	int32 Sample;

	for (uint32 i = 0; i < Size; i++) {

		Sample = Buffer[i];

		// Limit
		if (Sample > SAMPLE_MAX)
			Sample = SAMPLE_MAX;
		if (Sample < SAMPLE_MIN)
			Sample = SAMPLE_MIN;
		
		m_iGraphBuffer[m_iBufferPtr] = Sample;

		// Convert sample
		if (m_iSampleSize == 8)
			((int8*)m_pAccumBuffer)[m_iBufferPtr++] = (uint8)(Sample >> 8) ^ 0x80;
		else
			((int16*)m_pAccumBuffer)[m_iBufferPtr++] = (int16)Sample;

		// If buffer is filled, throw it to direct sound
		if (m_iBufferPtr == m_iBufSizeSamples) {

			if (m_bRendering) {
				// Output to file
				m_WaveFile.WriteWave((char*)m_pAccumBuffer, m_iBufSizeBytes);
				m_iBufferPtr = 0;
			}
			else {
				// Output to direct sound

				// Wait until it's time
				DWORD dwEvent;
				while ((dwEvent = m_pDSoundChannel->WaitForDirectSoundEvent()) != IN_SYNC) {
					// Custom event, quit
					if (dwEvent == EVENT_FLAG)
						return;

					static DWORD LastUnderrun;

					m_iAudioUnderruns++;

					if ((GetTickCount() - LastUnderrun) < 2000)
						theApp.BufferUnderrun();

					LastUnderrun = GetTickCount();
				}

				m_pDSoundChannel->WriteSoundBuffer(m_pAccumBuffer, m_iBufSizeBytes);

				theApp.DrawSamples((int*)m_iGraphBuffer, m_iBufSizeSamples);
				m_iGraphBuffer = new int32[m_iBufSizeSamples + 16];

				m_iBufferPtr = 0;
			}
		}
	}

	return;
}

unsigned int CSoundGen::GetUnderruns()
{
	return m_iAudioUnderruns;
}

//// Tracker playing routines //////////////////////////////////////////////////////////////////////////////

BOOL CSoundGen::InitInstance()
{
	//
	// Setup the synth, called when thread is started
	//
	// The object must be initialized before!
	//

	unsigned int i;

	// Create the LFO-table for vibrato and tremolo
	for (i = 0; i < VIBRATO_LENGTH; i++) {		
		double k, m, C, o;
		k = (double(i) / double((VIBRATO_LENGTH - 1))) * 6.283185;
		m = (6.283185 * 3.0) / 4.0;
		C = 128.0;
		o = 1.0f;		// 1.0
		m_cVibTable[i] = (int)(C * (sin(k + m) + o));
	}

	DPCMChannel.SetSampleMem(&m_SampleMem);

	LoadMachineSettings(NTSC, 60);

	// Make sure sound works (settings must be loaded here)
	if (!ResetSound())
		return 0;

	// Fix the last
	m_pAPU->Reset();

	// Enable all channels
	m_pAPU->Write(0x4015, 0x0F);
	m_pAPU->Write(0x4017, 0x00);

	m_SampleMem.SetMem(0, 0);

	m_iSpeed = DEFAULT_SPEED;
	m_iTempo = DEFAULT_TEMPO_NTSC;

	return TRUE;
}

void CSoundGen::BeginPlayer(bool bLooping)
{
	m_bPlaying			= true;
	m_bPlayLooping		= bLooping;
	m_iPlayTime			= 0;
	m_iJumpToPattern	= -1;
	m_iSkipToRow		= -1;
	m_bUpdateRow		= true;
	m_bPlayerHalted		= false;
	ResetTempo();
	LoadMachineSettings(pDocument->GetMachine(), pDocument->GetEngineSpeed());
	theApp.SilentEverything();
	pTrackerView->PlayerCommand(CMD_BEGIN, 0);
	if (!bLooping)
		pTrackerView->PlayerCommand(CMD_MOVE_TO_TOP, 0);
}

int CSoundGen::Run()
{
	// This is the main entry of the sound synthesizer
	// 

	unsigned int i;
	MSG	msg;

	if (!m_pDSoundChannel)
		return 0;

	// Make sure sound isn't skipping due to heavy CPU usage by other applications
	SetThreadPriority(THREAD_PRIORITY_TIME_CRITICAL);

	m_bRunning = true;

	for (i = 0; i < pDocument->GetAvailableChannels(); i++) {
		pTrackerView->NewNoteData[i] = false;
		pTrackerView->Arpeggiate[i] = 0;
	}

	// The sound generator and NES APU emulation routine
	// This is always running in the background when the tracker is running
	while (m_bRunning) {
		// Check messages
		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) && m_bRunning) {
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				switch (msg.message) {
					// Close the thread
					case WM_QUIT:
						m_bRunning = false;
						if (m_pDSoundChannel)
							m_pDSoundChannel->Stop();
						break;
					// Make everything silent
					case M_SILENT_ALL:
						m_pAPU->Reset();
						m_SampleMem.SetMem(0, 0);
						for (unsigned int i = 0; i < m_iChannelsActive; i++) {
							ChannelCollection[i]->MakeSilent();
							pTrackerView->NewNoteData[i] = false;
						}
						break;
					// Load new settings
					case M_LOAD_SETTINGS:
						ResetSound();
						break;
					case M_PLAY:
						BeginPlayer(false);
						break;
					case M_PLAY_LOOPING:
						BeginPlayer(true);

						/*
						m_bPlaying			= true;
						m_bPlayLooping		= true;
						m_iPlayTime			= 0;
						m_iJumpToPattern	= -1;
						m_iSkipToRow		= -1;
						m_bUpdateRow		= true;
						m_bPlayerHalted		= false;
						ResetTempo();
						LoadMachineSettings(pDocument->GetMachine(), pDocument->GetEngineSpeed());
						theApp.SilentEverything();
						pTrackerView->PlayerCommand(CMD_BEGIN, 0);
						*/
						break;
					case M_RESET:
						if (m_bPlaying)
							BeginPlayer(m_bPlayLooping);
						break;
					case M_STOP:
						m_bPlaying = false;
						//theApp.SilentEverything();
						PostThreadMessage(M_SILENT_ALL, 0, 0);
						break;
					case M_START_RENDER:
						ResetBuffer();
						m_bRequestRenderStop = false;
						m_bRendering = true;
						PostThreadMessage(M_PLAY, 0, 0);
						break;
				}
			}
		}

//		ControlMutex.Lock();

		theApp.StepFrame();

		SetEvent(m_hAliveCheck);

		// This need synchronisation, may fail sometimes when documents is unloaded after this point

		if (!theApp.IsDocLoaded())			// skip if doc is in a fail state
			continue;

		// Organize!

		RunFrame();

		if (pDocument->IsFileLoaded()) {
			for (i = 0; i < pDocument->GetAvailableChannels(); i++) {
				// clean up!
				if (pTrackerView->Arpeggiate[i] > 0) {
					ChannelCollection[i]->Arpeggiate(pTrackerView->Arpeggiate[i]);
					pTrackerView->Arpeggiate[i] = 0;
				}
				if (pTrackerView->NewNoteData[i]) {
					pTrackerView->NewNoteData[i] = false;
					//PlayNote(i, &pTrackerView->CurrentNotes[i], pDocument->GetEffColumns(i) + 1);
					PlayNote(i, &pTrackerView->GetNoteSynced(i), pDocument->GetEffColumns(i) + 1);
				}
			}
		}

		for (i = 0; i < m_iChannelsActive; i++) {
			ChannelCollection[i]->ProcessChannel();
			ChannelCollection[i]->RefreshChannel();
		}

		if (m_bUpdateRow && !m_bPlayerHalted)
			CheckControl();

		if (m_bPlaying)
			m_iTempoAccum -= m_iTempoDecrement;

		// Render the audio and play it
		if (m_pDSoundChannel) {
			m_pAPU->SetNextTime(m_iCycles);
			m_pAPU->Process();
			m_pAPU->EndFrame();
		}

//		ControlMutex.Unlock();

		if (m_bRequestRenderStop)
			StopRendering();
	}

	ExitInstance();

	return 0;
}

int CSoundGen::ExitInstance()
{
	// Shutdown this object

	// Free allocated memory
	if (m_iGraphBuffer) {
		delete [] m_iGraphBuffer;
		m_iGraphBuffer = NULL;
	}

	if (m_pAccumBuffer) {
		delete [] m_pAccumBuffer;
		m_pAccumBuffer = NULL;
	}

	// Kill APU
	// This is causing problems for the moment!
//	m_pAPU->Shutdown();
//	delete m_pAPU;
//	m_pAPU = NULL;

	// Kill DirectSound
	m_pDSoundChannel->Stop();
	m_pDSound->CloseChannel(m_pDSoundChannel);
	m_pDSound->Close();
	delete m_pDSound;
	m_pDSound = NULL;

	return CWinThread::ExitInstance();
}

// Get tempo values from the document
void CSoundGen::ResetTempo()
{
	if (!pDocument)
		return;

	m_iTempoAccum	= 60 * pDocument->GetFrameRate();		// 60 seconds per minute
	m_iTickPeriod	= pDocument->GetSongSpeed();
	m_iSpeed		= pDocument->GetSongSpeed();
	m_iTempo		= pDocument->GetSongTempo();

	m_iTempoAccum	  = 0;
	m_iTempoDecrement = (m_iTempo * 24) / m_iSpeed;
}

// Return current tempo setting in BPM
unsigned int CSoundGen::GetTempo()
{
	return (!m_iSpeed ? 0 : (m_iTempo * 6) / m_iSpeed);
}

void CSoundGen::RunFrame()
{
	int TicksPerSec = pDocument->GetFrameRate();

	pTrackerView->PlayerCommand(CMD_TICK, 0);

	if (m_bPlaying) {

		m_iPlayTime++;

		if (m_bRendering) {
			if (m_iRenderEndWhen == SONG_TIME_LIMIT) {
				if (m_iRenderEndParam == m_iPlayTime)
					m_bRequestRenderStop = true;
			}
		}

		// Tempo control
//		m_iTempoAccum -= m_iTempoDecrement;
		// Calculate playtime
		pTrackerView->PlayerCommand(CMD_TIME, (m_iPlayTime * 10) / TicksPerSec);

		// Fetch next row
		if (m_iTempoAccum <= 0) {
			m_iTempoAccum += (60 * TicksPerSec);
			m_bUpdateRow = true;
			pTrackerView->PlayerCommand(CMD_UPDATE_ROW, 0);
		}
		else {
			m_bUpdateRow = false;
		}
	}
}

void CSoundGen::CheckControl()
{
	// This function takes care of jumping and skipping

	if (m_bPlaying) {
		// If looping, halt when a jump or skip command are encountered
		if (m_bPlayLooping) {
			if (m_iJumpToPattern != -1 || m_iSkipToRow != -1)
				pTrackerView->PlayerCommand(CMD_MOVE_TO_TOP, 0);
			else
				pTrackerView->PlayerCommand(CMD_STEP_DOWN, 1);
		}
		else {
			// Jump
			if (m_iJumpToPattern != -1)
				pTrackerView->PlayerCommand(CMD_JUMP_TO, m_iJumpToPattern - 1);
			// Skip
			else if (m_iSkipToRow != -1)
				pTrackerView->PlayerCommand(CMD_SKIP_TO, m_iSkipToRow);
			// or just move on
			else
				pTrackerView->PlayerCommand(CMD_STEP_DOWN, 0);
		}

		m_iJumpToPattern = -1;
		m_iSkipToRow = -1;
	}
}

void CSoundGen::LoadMachineSettings(int Machine, int Rate)
{
	// Setup machine-type and speed
	//

	const double BASE_FREQ = 32.70;
	double clock;
	int Freq;

	if (!m_pAPU)
		return;

	m_iMachineType = Machine;

	if (Rate == 0)
		Rate = (Machine == NTSC ? 60 : 50);

	clock = (Machine == NTSC ? BASE_FREQ_NTSC : BASE_FREQ_PAL);

//	short ff;

//	clock = BASE_FREQ_PAL;

//	CFile f("C:\\freq_pal.bin", CFile::modeWrite | CFile::modeCreate);

	for (int i = 0; i < 96; i++) {
		Freq = (unsigned int)(double(clock / 16) / double(BASE_FREQ * pow(2.0, double(i) / 12)));
//		if (Freq > 0x7FF)
//			Freq = 0x7FF;
		m_iNoteLookupTable[i] = Freq;

//		ff = Freq;
//		f.Write(&ff, 2);

		// VRC6 Saw
		Freq = (unsigned int)(((double(clock / 16) / double(BASE_FREQ * pow(2.0, double(i) / 12))) * 16) / 14);
		m_iNoteLookupTableSaw[i] = Freq;
	}

//	f.Close();

	m_pNoteLookupTable = m_iNoteLookupTable;

	switch (Machine) {
		case NTSC:
			m_iCycles = BASE_FREQ_NTSC / Rate;
			m_pAPU->ChangeMachine(MACHINE_NTSC);
			break;
		case PAL:
			m_iCycles = BASE_FREQ_PAL / Rate;
			m_pAPU->ChangeMachine(MACHINE_PAL);
			break;
	}

	// Change period tables
	for (unsigned int i = 0; i < m_iChannelsActive; i++)
		ChannelCollection[i]->SetNoteTable(m_pNoteLookupTable);

	// This one is special
	VRC6Sawtooth.SetNoteTable(m_iNoteLookupTableSaw);
}

unsigned int CSoundGen::GetOutput(int Chan)
{
	if (!m_bRunning)
		return 0;

	return m_pAPU->GetVol(Chan);
}

void CSoundGen::SetDocument(CFamiTrackerDoc *pDoc, CFamiTrackerView *pView)
{
	pDocument	 = pDoc;
	pTrackerView = pView;	

	for (unsigned int i = 0; i < m_iChannelsActive; i++) {
		ChannelCollection[i]->InitChannel(m_pAPU, m_cVibTable, pDocument);
	}
}

void CSoundGen::PlayNote(int Channel, stChanNote *NoteData, int EffColumns)
{
	int i;

	// Evaluate global effects
	for (i = 0; i < EffColumns; i++) {
		unsigned char EffNum   = NoteData->EffNumber[i];
		unsigned char EffParam = NoteData->EffParam[i];

		switch (EffNum) {
			// Axx: Sets speed to xx
			case EF_SPEED:
				m_iTickPeriod = EffParam;
				if (!m_iTickPeriod)
					m_iTickPeriod++;
				if (m_iTickPeriod > 20)
					m_iTempo = m_iTickPeriod;
				else
					m_iSpeed = m_iTickPeriod;
				m_iTempoDecrement = (m_iTempo * 24) / m_iSpeed;  // 24 = 6 * 4
				break;

			// Bxx: Jump to pattern xx
			case EF_JUMP:
				m_iJumpToPattern = EffParam;
				FrameIsDone(1);
				if (m_bRendering)
					CheckRenderStop();
				FrameIsDone(m_iJumpToPattern);
				if (m_bRendering)
					CheckRenderStop();
				break;

			// Cxx: Skip to next track and start at row xx
			case EF_SKIP:
				m_iSkipToRow = EffParam;
				FrameIsDone(1);
				//m_iRenderedFrames++;
				break;

			// Dxx: Halt playback
			case EF_HALT:
				m_bPlayerHalted = true;
				StopPlayer();
				if (m_bRendering) {
					// Unconditional stop
					m_iRenderedFrames++;
					m_bRequestRenderStop = true;
				}
				break;
		}
	}
	
	// Update the individual channel
	ChannelCollection[Channel]->PlayNote(NoteData, EffColumns);
}

void CSoundGen::SetupChannels(int Chip) 
{
	unsigned int i;

	// Basic channels
	ChannelCollection[0] = dynamic_cast<CChannelHandler*>(&Square1Channel);
	ChannelCollection[1] = dynamic_cast<CChannelHandler*>(&Square2Channel);
	ChannelCollection[2] = dynamic_cast<CChannelHandler*>(&TriangleChannel);
	ChannelCollection[3] = dynamic_cast<CChannelHandler*>(&NoiseChannel);
	ChannelCollection[4] = dynamic_cast<CChannelHandler*>(&DPCMChannel);

	m_iChannelsActive = 5;		// default number of channels

	// Expansion
	switch (Chip) {
		case SNDCHIP_NONE:
			m_pAPU->SetExternalSound(SNDCHIP_NONE);
			break;
		case SNDCHIP_VRC6:
			ChannelCollection[5] = dynamic_cast<CChannelHandler*>(&VRC6Square1);
			ChannelCollection[6] = dynamic_cast<CChannelHandler*>(&VRC6Square2);
			ChannelCollection[7] = dynamic_cast<CChannelHandler*>(&VRC6Sawtooth);
			m_iChannelsActive += 3;
			m_pAPU->SetExternalSound(SNDCHIP_VRC6);
			break;
		case SNDCHIP_VRC7 :
			for (i = 0; i < 6; i++)
				ChannelCollection[i + m_iChannelsActive] = dynamic_cast<CChannelHandler*>(&VRC7Channels[i]);
			m_iChannelsActive += 6;
			m_pAPU->SetExternalSound(SNDCHIP_VRC7);
			break;
	}

	// Initialize channels
	for (i = 0; i < m_iChannelsActive; i++) {
		ChannelCollection[i]->InitChannel(m_pAPU, m_cVibTable, pDocument);
		ChannelCollection[i]->MakeSilent();
		ChannelCollection[i]->SetNoteTable(m_pNoteLookupTable);
	}

	// This one needs special care
	VRC6Sawtooth.SetNoteTable(m_iNoteLookupTableSaw);
}

bool CSoundGen::RenderToFile(char *File, int SongEndType, int SongEndParam)
{
	if (m_bPlaying)
		StopPlayer();

	m_iRenderEndWhen = (RENDER_END)SongEndType;
	m_iRenderEndParam = SongEndParam;
	m_iRenderedFrames = 0;
//	m_iRenderedSong = 0;

	pTrackerView->PlayerCommand(CMD_MOVE_TO_START, 0);

	if (m_iRenderEndWhen == SONG_TIME_LIMIT) {
		// This variable is stored in seconds, convert to frames
		m_iRenderEndParam *= pTrackerView->GetDocument()->GetFrameRate();
	}
	else if (m_iRenderEndWhen == SONG_LOOP_LIMIT) {
		m_iRenderEndParam *= pTrackerView->GetDocument()->GetFrameCount();
	}

	if (!m_WaveFile.OpenFile(File, theApp.m_pSettings->Sound.iSampleRate, theApp.m_pSettings->Sound.iSampleSize, 1)) {
		AfxMessageBox("Could not open output file!");
		return false;
	}
	else
		PostThreadMessage(M_START_RENDER, 0, 0);

	return true;
}

void CSoundGen::StopRendering()
{
	if (!IsRendering())
		return;

//	ControlMutex.Lock();

	// Send this as message instead

	StopPlayer();
	m_bRendering = false;
	pTrackerView->PlayerCommand(CMD_MOVE_TO_START, 0);
	m_WaveFile.CloseFile();
	
	ResetBuffer();

//	ControlMutex.Unlock();
}

void CSoundGen::GetRenderStat(int &Frame, int &Time, bool &Done)
{
	Frame = m_iRenderedFrames;
	Time = m_iPlayTime / pTrackerView->GetDocument()->GetFrameRate();
	Done = m_bRendering;
}

bool CSoundGen::IsRendering()
{
	return m_bRendering;
}

void CSoundGen::SongIsDone()
{
	if (IsRendering())
		CheckRenderStop();
}

void CSoundGen::FrameIsDone(int SkipFrames)
{
	if (m_bRequestRenderStop)
		return;

	//m_iRenderedFrames++;
	m_iRenderedFrames += SkipFrames;
}

void CSoundGen::CheckRenderStop()
{
	if (m_bRequestRenderStop)
		return;

	if (m_iRenderEndWhen == SONG_LOOP_LIMIT) {
		if (m_iRenderEndParam == m_iRenderedFrames)
			m_bRequestRenderStop = true;
	}
}