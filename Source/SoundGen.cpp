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

//
// Sound generator
//
// This takes care of the NES sound playback
//
// ToDo:
//
//  - Change this to take better care of playback. Effectively,
//    the view will only be called for refreshing the view.
//

#include "stdafx.h"
#include <cmath>
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "MainFrm.h"
#include "SoundGen.h"
#include "DirectSound.h"
#include "ChannelHandler.h"

IMPLEMENT_DYNCREATE(CSoundGen, CWinThread)

const int PITCH	= 1000;		// 100.0 %

const int DEFAULT_CYCLES_NTSC	= (1364 / 12) * 262 / 2;
const int DEFAULT_CYCLES_PAL	= (1264 / 12) * 315;
const int BASE_FREQ_NTSC		= (1789773 * PITCH) / 1000;
const int BASE_FREQ_PAL			= (1662607 * PITCH) / 1000;

//
// These can be found on internet, but the formula is: 
//  A = 440 Hz, one octave lower is 220 Hz and there are twelve notes on each octave. 
//  Add or remove 440 ^ (1 / 12) for next / previous note.
//
const unsigned int NOTE_FREQS[] = {262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494};
const unsigned int BASE_OCTAVE = 3;

CSquare1Chan	Square1Channel;
CSquare2Chan	Square2Channel;
CTriangleChan	TriangleChannel;
CNoiseChan		NoiseChannel;
CDPCMChan		DPCMChannel;

CChannelHandler	*ChannelCollection2A03[5];

CSoundGen::CSoundGen()
{
	m_pAPU				= NULL;
	m_pDSound			= NULL;
	m_pDSoundChannel	= NULL;
	m_pAccumBuffer		= NULL;
	m_iGraphBuffer		= NULL;
}

CSoundGen::~CSoundGen()
{
}

//// Sound handling ////////////////////////////////////////////////////////////////////////////////////////

bool CSoundGen::InitializeSound(HWND hWnd)
{
	// Initialize sound, this is only called once!
	// Start with NTSC by default

	ASSERT(m_pDSound == NULL && m_pAPU == NULL);

	m_iMachineType			= SPEED_NTSC;
	m_hWnd					= hWnd;
	m_pDSound				= new CDSound;
	m_pAPU					= new CAPU;
	m_hNotificationEvent	= CreateEvent(NULL, FALSE, FALSE, NULL);

	// Out of memory
	if (!m_pDSound || !m_pAPU)
		return false;

	m_pDSound->EnumerateDevices();
	
	if (!m_pAPU->Init(this, &m_SampleMem))
		return false;

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

	// Allocate a direct sound interface
	if (m_pDSoundChannel)
		m_pDSound->CloseChannel(m_pDSoundChannel);

	// Reinitialize direct sound
	int DevID = m_pDSound->MatchDeviceID((char*)theApp.m_pSettings->strDevice.GetBuffer());
	m_pDSound->Init(m_hWnd, m_hNotificationEvent, DevID);

	// Create channel
	m_pDSoundChannel = m_pDSound->OpenChannel(SampleRate, SampleSize, 1, BufferLen, 2);

	// Channel failed
	if (m_pDSoundChannel == NULL)
		return false;

	m_pDSoundChannel->Reset();

	// Create a buffer
	m_iBufSizeBytes		= m_pDSoundChannel->GetBlockSize();
	m_iBufSizeSamples	= m_iBufSizeBytes / (SampleSize / 8);

	if (m_pAccumBuffer)
		delete [] m_pAccumBuffer;

	m_pAccumBuffer = new char[m_iBufSizeBytes];

	// Out of memory
	if (!m_pAccumBuffer)
		return false;

	// Sample graph buffer
	if (m_iGraphBuffer)
		delete [] m_iGraphBuffer;

	m_iGraphBuffer = new int32[m_iBufSizeSamples];

	// Out of memory
	if (!m_iGraphBuffer)
		return false;

	// Allocate an APU buffer
	if (!m_pAPU->AllocateBuffer(SampleRate, 1, SPEED_NTSC))
		return false;

	switch (m_iMachineType) {
		case NTSC:	m_pAPU->ChangeSpeed(SPEED_NTSC);	break;
		case PAL:	m_pAPU->ChangeSpeed(SPEED_PAL);		break;
	}

	// Update blip-buffer filtering 
	m_pAPU->SetupMixer(theApp.m_pSettings->Sound.iBassFilter, 
		theApp.m_pSettings->Sound.iTrebleFilter, 
		theApp.m_pSettings->Sound.iTrebleDamping);

	return true;
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

			// Wait until it's time
			while (m_pDSoundChannel->WaitForDirectSoundEvent() == 1) {
				m_pDSoundChannel->ResetNotification();
			}

			m_pDSoundChannel->WriteSoundBuffer(m_pAccumBuffer, m_iBufSizeBytes);

			theApp.DrawSamples((int*)m_iGraphBuffer, m_iBufSizeSamples);
			m_iGraphBuffer = new int32[m_iBufSizeSamples + 16];

			m_iBufferPtr = 0;
		}
	}

	return;
}

unsigned int CSoundGen::GetUnderruns()
{
	return m_pDSoundChannel->Underruns; 
}

//// Tracker playing routines //////////////////////////////////////////////////////////////////////////////

BOOL CSoundGen::InitInstance()
{
	//
	// Setup the synth, called when thread is started
	//
	// The object must be initialized before!
	//

	unsigned int i, j;
	unsigned int NesFreqPAL, NesFreqNTSC;

	// Setup proper note frequency tables for the NES
	for (i = 0; i < 8; i++) {		// octaves
		for (j = 0; j < 12; j++) {	// notes
			NesFreqNTSC = (BASE_FREQ_NTSC / NOTE_FREQS[j]) / 16;
			NesFreqPAL	= (BASE_FREQ_PAL / NOTE_FREQS[j]) / 16;

			if (i > BASE_OCTAVE) {
				NesFreqNTSC /= (1 << (i - BASE_OCTAVE));
				NesFreqPAL  /= (1 << (i - BASE_OCTAVE));
			}
			else if (i < BASE_OCTAVE) {
				NesFreqNTSC *= (1 << (BASE_OCTAVE - i));
				NesFreqPAL  *= (1 << (BASE_OCTAVE - i));
			}

			// Limit the freq, cannot go below $7FF on a NES
			if (NesFreqNTSC > 0x7FF)
				NesFreqNTSC = 0x7FF;
			if (NesFreqPAL > 0x7FF)
				NesFreqPAL = 0x7FF;

			m_iNoteLookupTable_NTSC[(i * 12) + j] = NesFreqNTSC;
			m_iNoteLookupTable_PAL[(i * 12) + j] = NesFreqPAL;
		}
	}

	// Create the LFO-table for vibrato and tremolo
	for (i = 0; i < VIBRATO_LENGTH; i++) {		
		double k, m, C, o;
		k = (double(i) / double((VIBRATO_LENGTH - 1))) * 6.283185;
		m = (6.283185 * 3.0) / 4.0;
		C = 128.0;
		o = 1.0f;
		
		m_cVibTable[i] = unsigned char(C * (sin(k + m) + o));
	}

	// Load channels
	ChannelCollection2A03[0] = dynamic_cast<CChannelHandler*>(&Square1Channel);
	ChannelCollection2A03[1] = dynamic_cast<CChannelHandler*>(&Square2Channel);
	ChannelCollection2A03[2] = dynamic_cast<CChannelHandler*>(&TriangleChannel);
	ChannelCollection2A03[3] = dynamic_cast<CChannelHandler*>(&NoiseChannel);
	ChannelCollection2A03[4] = dynamic_cast<CChannelHandler*>(&DPCMChannel);

	DPCMChannel.SetSampleMem(&m_SampleMem);

	LoadMachineSettings(NTSC, 60);

	// Make sure sound works (settings must be loaded here)
	if (!ResetSound())
		return 0;

	// Fix the last
	m_pAPU->Reset();

	// Enable all channels
	m_pAPU->Write(0x4015, 0x0F);

	m_SampleMem.SetMem(0, 0);

	for (i = 0; i < 5; i++) {
		ChannelCollection2A03[i]->InitChannel(m_pAPU, m_cVibTable, pDocument);
		ChannelCollection2A03[i]->MakeSilent();
	}

	m_iSpeed = DEFAULT_SPEED;
	m_iTempo = DEFAULT_TEMPO_NTSC;

	return TRUE;
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

	// The sound generator and NES APU emulation routine
	// This will always run in the background when the tracker is running
	while (m_bRunning) {

		// As of version 0.2.3 the old sync code is exterminated.
		// Notes are fetched on the fly from FamiTrackerView,
		// and everything seems to work well so far

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
						//SquareChannel.MakeSilent();
						for (int i = 0; i < 5; i++) {
							ChannelCollection2A03[i]->MakeSilent();
							//KillChannelHard(i);
						}
						break;
					// Load new settings
					case M_LOAD_SETTINGS:
						ResetSound();
						break;
				}
			}
		}

		theApp.StepFrame();

		RunFrame();

		// Organize!

		pTrackerView->PlayerCommand(CMD_TICK, 0);

		if (pDocument->IsFileLoaded()) {
			for (i = 0; i < pDocument->GetAvailableChannels(); i++) {
				if (pTrackerView->Arpeggiate[i] > 0) {
					ChannelCollection2A03[i]->Arpeggiate(pTrackerView->Arpeggiate[i]);
					pTrackerView->Arpeggiate[i] = 0;
				}
				//else {
					if (pTrackerView->NewNoteData[i]) {
						pTrackerView->NewNoteData[i] = false;
						PlayNote(i, &pTrackerView->CurrentNotes[i], pDocument->GetEffColumns(i) + 1);
					}
				//}
			}
		}

		if (m_bRunning) {

			for (i = 0; i < 5; i++) {
				ChannelCollection2A03[i]->ProcessChannel();
				ChannelCollection2A03[i]->RefreshChannel();
			}
			
			if (m_pDSoundChannel)
				m_pAPU->Run(m_iCycles);
		}
	}

	// Close down thread
	m_pDSoundChannel->Stop();
	m_pDSound->CloseChannel(m_pDSoundChannel);

	delete m_pDSound;
	delete m_pAPU;

	this->m_bAutoDelete = FALSE;

	return 0;
}

void CSoundGen::StartPlayer(bool Looping)
{
	if (m_bPlaying || !pDocument->IsFileLoaded())
		return;

	m_bPlaying			= true;
	m_bPlayLooping		= Looping;
	m_iPlayTime			= 0;
	m_bNewRow			= true;
	m_iJumpToPattern	= -1;
	m_iSkipToRow		= -1;

	ResetTempo();

	LoadMachineSettings(pDocument->GetMachine(), pDocument->GetEngineSpeed());

	theApp.SilentEverything();

	if (!Looping)
		pTrackerView->PlayerCommand(CMD_MOVE_TO_TOP, 0);

	pTrackerView->SetMessageText("Playing");
}

void CSoundGen::StopPlayer()
{
	if (!m_bPlaying)
		return;

	m_bPlaying = false;

	theApp.SilentEverything();
}

// Get tempo values from the document
void CSoundGen::ResetTempo()
{
	if (!pDocument)
		return;

	m_iTempoAccum	= 60 * pDocument->GetFrameRate();		// 60 as in 60 seconds per minute
	m_iTickPeriod	= pDocument->GetSongSpeed();
	m_iSpeed		= DEFAULT_SPEED;
	m_iTempo		= m_iTempoAccum / 24;

	if (m_iTickPeriod > 20)
		m_iTempo = m_iTickPeriod;
	else
		m_iSpeed = m_iTickPeriod;
}

// Return current tempo setting in BPM
int	CSoundGen::GetTempo()
{
	if (m_iSpeed == 0)
		return 0;

	return (m_iTempo * 6) / m_iSpeed;
}

void CSoundGen::RunFrame()
{
	int TicksPerSec = pDocument->GetFrameRate();

	if (m_bPlaying) {
		
		m_iPlayTime++;

		// A new row is being played, fetch notes
		if (m_bNewRow) {			
			m_bNewRow = false;
			pTrackerView->PlayerCommand(CMD_UPDATE_ROW, 0);
		}

		// Calculate tens of seconds
		pTrackerView->PlayerCommand(CMD_TIME, (m_iPlayTime * 10) / TicksPerSec);

		// Tempo control
		m_iTempoAccum -= (m_iTempo * 24) / m_iSpeed; // 24 = 4 * 6
		
		if (m_iTempoAccum <= 0) {
			m_iTempoAccum += (60 * TicksPerSec);
			m_bNewRow = true;

			// If looping, halt when a jump or skip command are encountered
			if (m_bPlayLooping) {
				/*
				if (m_iJumpToPattern != -1 || m_iSkipToRow != -1) {
					//StopPlayer();
					m_iJumpToPattern = -1;
					m_iSkipToRow = -1;
				}
				else
				*/
				if (m_iJumpToPattern != -1 || m_iSkipToRow != -1)
					pTrackerView->PlayerCommand(CMD_MOVE_TO_TOP, 0);
				else
					pTrackerView->PlayerCommand(CMD_STEP_DOWN, 1);
			}
			else {
				// Jump
				if (m_iJumpToPattern != -1)
					pTrackerView->PlayerCommand(CMD_JUMP_TO, m_iJumpToPattern);
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
}

void CSoundGen::LoadMachineSettings(int Machine, int Rate)
{
	// Setup machine-type and speed
	//

	if (!m_pAPU)
		return;

	m_iMachineType = Machine;

	if (Rate == 0)
		Rate = (Machine == NTSC ? 60 : 50);

	switch (Machine) {
		case NTSC:
			m_pNoteLookupTable	= m_iNoteLookupTable_NTSC;
			m_iCycles			= BASE_FREQ_NTSC / Rate;
			m_pAPU->ChangeSpeed(SPEED_NTSC);
			break;
		case PAL:
			m_pNoteLookupTable	= m_iNoteLookupTable_PAL;
			m_iCycles			= BASE_FREQ_PAL / Rate;
			m_pAPU->ChangeSpeed(SPEED_PAL);
			break;
	}

	for (int i = 0; i < 5; i++) {
		ChannelCollection2A03[i]->SetNoteTable(m_pNoteLookupTable);
	}
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
}

void CSoundGen::PlayNote(int Channel, stChanNote *NoteData, int EffColumns)
{
	int i;

	// Evaluate global effects
	for (i = 0; i < EffColumns; i++) {
		int EffNum	 = NoteData->EffNumber[i];
		int EffParam = NoteData->EffParam[i];

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

				break;

			// Bxx: Jump to pattern xx
			case EF_JUMP:
				m_iJumpToPattern = EffParam - 1;
				break;

			// Cxx: Skip to next track and start at row xx
			case EF_SKIP:
				m_iSkipToRow = EffParam;
				break;

			// Dxx: Halt playback
			case EF_HALT:
				StopPlayer();
				break;
		}
	}
	
	// Update the individual channel
	ChannelCollection2A03[Channel]->PlayNote(NoteData, EffColumns);
}

int CSoundGen::ExitInstance()
{
	// Close this
	m_bRunning = false;
	return CWinThread::ExitInstance();
}
