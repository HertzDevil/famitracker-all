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

//
// This file takes care of the NES sound playback
//

// Todo:
//
// - Isolate channels and chips and take care of all setup and creation in some other class
//

#include "stdafx.h"
#include <cmath>
#include <afxmt.h>
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "DirectSound.h"

#include "ChannelHandler.h"
#include "Channels2A03.h"
#include "ChannelsVRC6.h"
#include "ChannelsMMC5.h"
#include "ChannelsVRC7.h"
#include "ChannelsFDS.h"
#include "ChannelsN106.h"
#include "Channels5B.h"

#include "SoundGen.h"
#include "Settings.h"
#include "TrackerChannel.h"

IMPLEMENT_DYNCREATE(CSoundGen, CWinThread)

const float BASE_FREQ_NTSC = 1789773;
const float BASE_FREQ_PAL	 = 1662607;

const double VIBRATO_DEPTH[] = {
//	1.0, 2.0, 3.0, 4.0, 5.0, 7.0, 10.0, 12.0, 14.0, 17.0, 22.0, 30.0, 44.0, 64.0, 96.0, 128.0
	1.0, 1.5, 2.5, 4.0, 5.0, 7.0, 10.0, 12.0, 14.0, 17.0, 22.0, 30.0, 44.0, 64.0, 96.0, 128.0
//	1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 5.0, 6.0, 7.0, 9.0, 12.0, 15.0, 19.0, 26.0, 38.0
};

CSoundGen::CSoundGen() : 
	m_pAPU(NULL),
	m_pDSound(NULL),
	m_pDSoundChannel(NULL),
	m_pAccumBuffer(NULL),
	m_iGraphBuffer(NULL),
	pDocument(NULL),
	m_bRendering(false),
	m_bPlaying(false),
	m_pPreviewSample(NULL)
{
	memset(m_pChannels, 0, CHANNELS * 4);
}

CSoundGen::~CSoundGen()
{
}

//// Interface functions ///////////////////////////////////////////////////////////////////////////////////

void CSoundGen::StartPlayer(int Mode)
{
	if (!pDocument || !pDocument->IsFileLoaded())
		return;

	m_iPlayMode = Mode;

	PostThreadMessage(M_PLAY, 0, 0);
}

void CSoundGen::StopPlayer()
{
//	if (!m_bPlaying)
//		return;

	if (!pDocument || !pDocument->IsFileLoaded())
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

	CreateChannels();

	return true;
}

void CSoundGen::CreateChannels()
{
	// Clear all channels
	for (int i = 0; i < CHANNELS; i++) {
		m_pChannels[i] = NULL;
		m_pTrackerChannels[i] = NULL;
	}

	// 2A03/2A07
	AssignChannel(new CTrackerChannel("Square 1", SNDCHIP_NONE, CHANID_SQUARE1),		new CSquare1Chan());
	AssignChannel(new CTrackerChannel("Square 2", SNDCHIP_NONE, CHANID_SQUARE2),		new CSquare2Chan());
	AssignChannel(new CTrackerChannel("Triangle", SNDCHIP_NONE, CHANID_TRIANGLE),		new CTriangleChan());
	AssignChannel(new CTrackerChannel("Noise", SNDCHIP_NONE, CHANID_NOISE),				new CNoiseChan());
	AssignChannel(new CTrackerChannel("DPCM", SNDCHIP_NONE, CHANID_DPCM),				new CDPCMChan());

	// Konami VRC6
	AssignChannel(new CTrackerChannel("Square 1", SNDCHIP_VRC6, CHANID_VRC6_PULSE1),	new CVRC6Square1());
	AssignChannel(new CTrackerChannel("Square 2", SNDCHIP_VRC6, CHANID_VRC6_PULSE2),	new CVRC6Square2());
	AssignChannel(new CTrackerChannel("Sawtooth", SNDCHIP_VRC6, CHANID_VRC6_SAWTOOTH),	new CVRC6Sawtooth());

	// Konami VRC7
	AssignChannel(new CTrackerChannel("Channel 1", SNDCHIP_VRC7, CHANID_VRC7_CH1),		new CVRC7Channel());
	AssignChannel(new CTrackerChannel("Channel 2", SNDCHIP_VRC7, CHANID_VRC7_CH2),		new CVRC7Channel());
	AssignChannel(new CTrackerChannel("Channel 3", SNDCHIP_VRC7, CHANID_VRC7_CH3),		new CVRC7Channel());
	AssignChannel(new CTrackerChannel("Channel 4", SNDCHIP_VRC7, CHANID_VRC7_CH4),		new CVRC7Channel());
	AssignChannel(new CTrackerChannel("Channel 5", SNDCHIP_VRC7, CHANID_VRC7_CH5),		new CVRC7Channel());
	AssignChannel(new CTrackerChannel("Channel 6", SNDCHIP_VRC7, CHANID_VRC7_CH6),		new CVRC7Channel());

	// Nintendo FDS
	AssignChannel(new CTrackerChannel("FDS", SNDCHIP_FDS, CHANID_FDS),					new CChannelHandlerFDS());

	// Nintendo MMC5
	AssignChannel(new CTrackerChannel("Square 1", SNDCHIP_MMC5, CHANID_MMC5_SQUARE1),	new CMMC5Square1Chan());
	AssignChannel(new CTrackerChannel("Square 2", SNDCHIP_MMC5, CHANID_MMC5_SQUARE2),	new CMMC5Square2Chan());

	// Namco N106
	AssignChannel(new CTrackerChannel("Channel 1", SNDCHIP_N106, CHANID_N106_CHAN1),	new CChannelHandlerN106());
	AssignChannel(new CTrackerChannel("Channel 2", SNDCHIP_N106, CHANID_N106_CHAN2),	new CChannelHandlerN106());
	AssignChannel(new CTrackerChannel("Channel 3", SNDCHIP_N106, CHANID_N106_CHAN3),	new CChannelHandlerN106());
	AssignChannel(new CTrackerChannel("Channel 4", SNDCHIP_N106, CHANID_N106_CHAN4),	new CChannelHandlerN106());
	AssignChannel(new CTrackerChannel("Channel 5", SNDCHIP_N106, CHANID_N106_CHAN5),	new CChannelHandlerN106());
	AssignChannel(new CTrackerChannel("Channel 6", SNDCHIP_N106, CHANID_N106_CHAN6),	new CChannelHandlerN106());
	AssignChannel(new CTrackerChannel("Channel 7", SNDCHIP_N106, CHANID_N106_CHAN7),	new CChannelHandlerN106());
	AssignChannel(new CTrackerChannel("Channel 8", SNDCHIP_N106, CHANID_N106_CHAN8),	new CChannelHandlerN106());

	// Sunsoft 5B
	AssignChannel(new CTrackerChannel("Channel 1", SNDCHIP_5B, CHANID_5B_CH1),			new C5BChannel1());
	AssignChannel(new CTrackerChannel("Channel 2", SNDCHIP_5B, CHANID_5B_CH2),			new C5BChannel2());
	AssignChannel(new CTrackerChannel("Channel 3", SNDCHIP_5B, CHANID_5B_CH3),			new C5BChannel3());

	// DPCM sample memory
	((CDPCMChan*)m_pChannels[CHANID_DPCM])->SetSampleMem(&m_SampleMem);

	// VRC7 channels setup
	for (int i = 0; i < 6; i++) 
		((CVRC7Channel*)m_pChannels[CHANID_VRC7_CH1 + i])->ChannelIndex(i);
}

void CSoundGen::AssignChannel(CTrackerChannel *pTrackerChannel, CChannelHandler *pRenderer)
{
	int ID = pTrackerChannel->GetID();

	m_pTrackerChannels[ID] = pTrackerChannel;
	m_pChannels[ID] = pRenderer;
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
	//int DevID = m_pDSound->MatchDeviceID((char*)theApp.m_pSettings->strDevice.GetBuffer());
	if (!m_pDSound->Init(m_hWnd, m_hNotificationEvent, /*DevID*/ theApp.m_pSettings->Sound.iDevice))
		return false;

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
	m_pAPU->Reset();
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

		// 1000 Hz test tone
		/*
		static double sine_phase = 0;
		Sample = sinf(sine_phase) * 10000.0;
		sine_phase += 1000.0 / (double(m_pDSoundChannel->GetSampleRate()) / 6.283184);
		if (sine_phase > 6.283184)
			sine_phase -= 6.283184;
			*/

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
	// Sound must be working before entering here!
	//

	// Generate default vibrato table
	GenerateVibratoTable(VIBRATO_NEW);

	ResetSound();

	LoadMachineSettings(NTSC, 60);

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

void CSoundGen::GenerateVibratoTable(int Type)
{

#ifdef NEW_VIBRATO

	for (int i = 0; i < 16; i++) {	// depth 
		for (int j = 0; j < 16; j++) {	// phase
			int value = 0;
			/*
			if (i < 5) {
				value = (int)(((double)(j) * VIBRATO_DEPTH[i]) / 6.0);
			}
			else {
				double angle = (double(j) / 15.0) * (3.1415 / 4.0);
				value = int((sin(angle) * 20.0 * VIBRATO_DEPTH[i]) / 6.0);
			}
			*/

			double angle = (double(j) / 16.0) * (3.1415 / 2.0);

			/*
			value = int(sin(angle) * 256);
			value >>= (8 - (i >> 1));
			if ((i & 1) == 0)
				value -= (value >> 1);
				*/

			if (Type == VIBRATO_NEW)
				value = int(sin(angle) * VIBRATO_DEPTH[i] /*+ 0.5f*/);
			else {
				const double LIST[] = {1, 1, 2, 3, 4, 7, 8, 0x0F, 0x10, 0x1F, 0x20, 0x3F, 0x40, 0x7F, 0x80, 0xFF};
//				if (i == 1) {
					//value = (j < 4) ? 0 : 1;
					value = (int)((double(j*LIST[i])/16.0)+1);
//				}
			}

			m_cVibTable[i * 16 + j] = value;
			CString a;
			a.Format("%i ", value);
			TRACE0(a);
		}
		TRACE0("\n");
	}
/*
	for (int j = 16; j < 65; j+= 16) {
	for (int i = 0; i < 64; i++) {

		int VibFreq;
		int depth = j;//16;

		if ((i & 0xF0) == 0x00)
			VibFreq = m_cVibTable[i + depth];
		else if ((i & 0xF0) == 0x10)
			VibFreq = m_cVibTable[15 - (i - 16) + depth];
		else if ((i & 0xF0) == 0x20)
			VibFreq = -m_cVibTable[(i - 32) + depth];
		else if ((i & 0xF0) == 0x30)
			VibFreq = -m_cVibTable[15 - (i - 48) + depth];
		
		VibFreq += m_cVibTable[depth + 15] + 1;
		VibFreq >>= 1;

		CString a;
		a.Format("%i ", VibFreq);
		TRACE0(a);
	}
	TRACE0("\n");
	}
*/
#ifdef _DEBUG
	CFile a("vibrato.txt", CFile::modeWrite | CFile::modeCreate);
	CString b;
	for (int i = 0; i < 16; i++) {	// depth 
		b = "\t.byte ";
		a.Write(b.GetBuffer(), b.GetLength());
		for (int j = 0; j < 16; j++) {	// phase
			int value = m_cVibTable[i * 16 + j];
			b.Format("$%02X, ", value);
			a.Write(b.GetBuffer(), b.GetLength());
		}
		b = "\n";
		a.Write(b.GetBuffer(), b.GetLength());
	}
	a.Close();
#endif

#else

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
/*
	for (i = 0; i < 16; i++) {
		for (int j = 0; j < 16; j++) {
			m_iVibratoTables[i][j] = int(sinf(float(j) / 3.1415f) * float(i));
		}
	}
*/
#endif

	// Notify channels
	for (int i = 0; i < CHANNELS; ++i)
		if (m_pChannels[i])
			m_pChannels[i]->SetVibratoStyle(Type);
}

int CSoundGen::ReadVibTable(int index)
{
	return m_cVibTable[index];
}

void CSoundGen::BeginPlayer()
{
	if (!m_pDSoundChannel)
		return;

	switch (m_iPlayMode) {
		// Play from top of pattern
		case MODE_PLAY:
			m_bPlayLooping = false;
			pTrackerView->PlayerCommand(CMD_MOVE_TO_TOP, 0);
			break;
		// Repeat pattern
		case MODE_PLAY_REPEAT:
			m_bPlayLooping = true;
			pTrackerView->PlayerCommand(CMD_MOVE_TO_TOP, 0);
			break;
		// Start of song
		case MODE_PLAY_START:
			m_bPlayLooping = false;
			pTrackerView->PlayerCommand(CMD_MOVE_TO_START, 0);
			break;
		// From cursor
		case MODE_PLAY_CURSOR:
			m_bPlayLooping = false;
			pTrackerView->PlayerCommand(CMD_MOVE_TO_CURSOR, 0);
			break;
	}

	m_bPlaying			= true;
	m_iPlayTime			= 0;
	m_iJumpToPattern	= -1;
	m_iSkipToRow		= -1;
	m_bUpdateRow		= true;
	m_bPlayerHalted		= false;
	
	ResetTempo();

	LoadMachineSettings(pDocument->GetMachine(), pDocument->GetEngineSpeed());

	theApp.SilentEverything();
}

void CSoundGen::HandleMessages()
{
	MSG msg;

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
					for (unsigned int i = 0; i < CHANNELS; ++i) {
						if (m_pChannels[i]) {
							m_pChannels[i]->ResetChannel();
							m_pChannels[i]->MakeSilent();
						}
						if (m_pTrackerChannels[i])
							m_pTrackerChannels[i]->Reset();
//							pTrackerView->NewNoteData[i] = false;
					}
					break;
				// Load new settings
				case M_LOAD_SETTINGS:
					ResetSound();
					break;
				case M_PLAY:
					BeginPlayer();
					break;
					/*
				case M_PLAY_LOOPING:
					BeginPlayer(true);
					break;
					*/
				case M_RESET:
					if (m_bPlaying)
						BeginPlayer(/*m_bPlayLooping*/);
					break;
				case M_STOP:
					m_bPlaying = false;
					PostThreadMessage(M_SILENT_ALL, 0, 0);
					break;
				case M_START_RENDER:
					ResetBuffer();
					m_bRequestRenderStop = false;
					m_bRendering = true;
					m_iDelayedStart = 5;	// Wait 5 frames until player starts
					m_iDelayedEnd = 5;
					break;
				case M_PREVIEW_SAMPLE:
					PlaySample((CDSample*)msg.wParam, LOWORD(msg.lParam), HIWORD(msg.lParam));
					break;
				case M_WRITE_APU:
					m_pAPU->Write((uint16)msg.wParam, (uint8)msg.lParam);
					break;
			}
		}
	}
}

int CSoundGen::Run()
{
	// This is the main entry of the sound synthesizer
	// 

	// Todo: split this into several functions

	unsigned int i;

	// Make sure sound isn't skipping due to heavy CPU usage by other applications
	SetThreadPriority(THREAD_PRIORITY_TIME_CRITICAL);

	m_bRunning = true;
	m_iDelayedStart = 0;

	for (i = 0; i < pDocument->GetAvailableChannels(); i++) {
//		pTrackerView->NewNoteData[i] = false;
		pTrackerView->Arpeggiate[i] = 0;
	}

	// The sound generator and NES APU emulation routine
	// This is always running in the background when the tracker is running
	while (m_bRunning) {
		// Check messages
		HandleMessages();

		theApp.StepFrame();

		SetEvent(m_hAliveCheck);

		// This need synchronisation, may fail sometimes when documents is unloaded after this point
		if (!theApp.IsDocLoaded())			// skip if doc is in a fail state
			continue;

		// Organize!

		RunFrame();

		if (pDocument->IsFileLoaded()) {
			for (i = 0; i < pDocument->GetAvailableChannels(); i++) {
				int Channel = pDocument->GetChannelType(i);
				
				// clean up!
				if (pTrackerView->Arpeggiate[i] > 0) {
					m_pChannels[Channel]->Arpeggiate(pTrackerView->Arpeggiate[i]);
					pTrackerView->Arpeggiate[i] = 0;
				}
				// !!!

				if (m_pTrackerChannels[Channel]->NewNoteData()) {
					stChanNote Note = m_pTrackerChannels[Channel]->GetNote();
					PlayNote(Channel, &Note, pDocument->GetEffColumns(i) + 1);
				}

				// Pitch wheel
				int Pitch = m_pTrackerChannels[Channel]->GetPitch();
				m_pChannels[Channel]->SetPitch(Pitch);

				// Update volume meters
				m_pTrackerChannels[Channel]->SetVolumeMeter(m_pAPU->GetVol(Channel));
			}

			// This fails sometimes but shouldn't
			if (m_pChannels[pTrackerView->GetSelectedChannel()])
				m_pChannels[pTrackerView->GetSelectedChannel()]->UpdateSequencePlayPos();
		}

		int FrameCycles = 0;
		const int CHANNEL_DELAY = 50;

		// Update channels and channel registers
		for (i = 0; i < CHANNELS; ++i) {
			if (m_pChannels[i]) {
				m_pChannels[i]->ProcessChannel();
				m_pAPU->SetNextTime(FrameCycles);
				m_pChannels[i]->RefreshChannel();
				m_pAPU->Process();
				FrameCycles += CHANNEL_DELAY;
			}
		}

		if (m_bUpdateRow && !m_bPlayerHalted)
			CheckControl();

		if (m_bPlaying)
			m_iTempoAccum -= m_iTempoDecrement;

		// Render the whole audio frame
		if (m_pDSoundChannel) {
			m_pAPU->SetNextTime(m_iCycles);
			m_pAPU->Process();
			m_pAPU->EndFrame();
		}

		if (m_bRendering && m_bRequestRenderStop) {
			StopPlayer();
			if (!m_iDelayedEnd)
				StopRendering();
			else
				--m_iDelayedEnd;
		}

		if (m_iDelayedStart > 0) {
			m_iDelayedStart--;
			if (!m_iDelayedStart) {
				m_iPlayMode = MODE_PLAY_START;
				PostThreadMessage(M_PLAY, 0, 0);
			}
		}

		// Check if a previewed sample should be removed
		if (m_pPreviewSample && PreviewDone()) {
			delete [] m_pPreviewSample->SampleData;
			delete m_pPreviewSample;
			m_pPreviewSample = NULL;
		}
	}

	return ExitInstance();
}

int CSoundGen::ExitInstance()
{
	// Shutdown this object

	theApp.SetSoundGenerator(NULL);

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
	if (m_pDSoundChannel) {
		m_pDSoundChannel->Stop();
		m_pDSound->CloseChannel(m_pDSoundChannel);
		m_pDSoundChannel = NULL;
	}

	if (m_pDSound) {
		m_pDSound->Close();
		delete m_pDSound;
		m_pDSound = NULL;
	}

	return CWinThread::ExitInstance();
}

// Get tempo values from the document
void CSoundGen::ResetTempo()
{
	if (!pDocument)
		return;

//	m_iTempoAccum	= 60 * pDocument->GetFrameRate();		// 60 seconds per minute
	m_iTickPeriod = pDocument->GetSongSpeed();
	m_iSpeed = pDocument->GetSongSpeed();
	m_iTempo = pDocument->GetSongTempo();

	m_iTempoAccum = 0;
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
				if (m_iPlayTime >= (unsigned int)m_iRenderEndParam)
					m_bRequestRenderStop = true;
			}
		}

		// Calculate playtime
		pTrackerView->PlayerCommand(CMD_TIME, (m_iPlayTime * 10) / TicksPerSec);

		m_iStepRows = 0;

		// Fetch next row
		if (m_iTempoAccum <= 0) {
			while (m_iTempoAccum <= 0)  {
				m_iTempoAccum += (60 * TicksPerSec);
				m_iStepRows++;
			}
			m_bUpdateRow = true;
			pTrackerView->PlayerCommand(CMD_READ_ROW, 0);
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
				while (m_iStepRows--)
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
				while (m_iStepRows--)
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

	const double BASE_FREQ = 32.7032;
	double clock;
	double Freq;
	double Pitch;

	if (!m_pAPU)
		return;

	m_iMachineType = Machine;

	if (Rate == 0)
		Rate = (Machine == NTSC ? 60 : 50);

	clock = (Machine == NTSC ? BASE_FREQ_NTSC : BASE_FREQ_PAL) / 16.0;

	for (int i = 0; i < 96; i++) {
		Freq = BASE_FREQ * pow(2.0, double(i) / 12.0);
		// 2A03 / 2A07 / MMC5 / VRC6
		Pitch = (clock / Freq) - 0.5;
		m_iNoteLookupTable[i] = (unsigned int)Pitch;
		// VRC6 Saw
		Pitch = (((clock / Freq) * 16.0) / 14.0) - 0.5;
		m_iNoteLookupTableSaw[i] = (unsigned int)Pitch;
		// FDS
		Pitch = (Freq * 65536.0) / (clock / 4.0) + 0.5;
		m_iNoteLookupTableFDS[i] = (unsigned int)Pitch;
	}

/*
	// Write period tables to files
	CFile file1("freq_pal.bin", CFile::modeWrite | CFile::modeCreate);
	for (int i = 0 ; i < 96; i++) {
		unsigned short s = m_iNoteLookupTable[i];
		file1.Write(&s, 2);
	}
	file1.Close();

	CFile file2("freq_sawtooth.bin", CFile::modeWrite | CFile::modeCreate);
	for (int i = 0 ; i < 96; i++) {
		unsigned short s = m_iNoteLookupTableSaw[i];
		file2.Write(&s, 2);
	}
	file2.Close();

	CFile file("freq_fds.bin", CFile::modeWrite | CFile::modeCreate);
	for (int i = 0 ; i < 96; i++) {
		unsigned short s = m_iNoteLookupTableFDS[i];
		file.Write(&s, 2);
	}
	file.Close();
*/
#if 0
	CFile file("vol.txt", CFile::modeWrite | CFile::modeCreate);

	for (int i = 0; i < 16; i++) {
		for (int j = /*(i / 2)*/0; j < 8; j++) {
			int a = (i*(j*2)) / 15;
			int b = (i*(j*2+1)) / 15;
			if (i > 0 && j > 0 && a == 0) a = 1;
			if (j > 0 && i > 0 && b == 0) b = 1;
			unsigned char c = (a<<4) | b;
			CString st;
			st.Format("$%02X, ", c);
			file.Write(st, st.GetLength());
		}
		file.Write("\n", 1);
	}

	file.Close();
#endif
	m_pNoteLookupTable = m_iNoteLookupTable;

	switch (Machine) {
		case NTSC:
			m_iCycles = (int)(BASE_FREQ_NTSC / (float)Rate);
			m_pAPU->ChangeMachine(MACHINE_NTSC);
			break;
		case PAL:
			m_iCycles = (int)(BASE_FREQ_PAL / (float)Rate);
			m_pAPU->ChangeMachine(MACHINE_PAL);
			break;
	}

	// Setup note tables
	m_pChannels[CHANID_SQUARE1]->SetNoteTable(m_iNoteLookupTable);
	m_pChannels[CHANID_SQUARE2]->SetNoteTable(m_iNoteLookupTable);
	m_pChannels[CHANID_TRIANGLE]->SetNoteTable(m_iNoteLookupTable);
	m_pChannels[CHANID_MMC5_SQUARE1]->SetNoteTable(m_iNoteLookupTable);
	m_pChannels[CHANID_MMC5_SQUARE2]->SetNoteTable(m_iNoteLookupTable);
	m_pChannels[CHANID_VRC6_PULSE1]->SetNoteTable(m_iNoteLookupTable);
	m_pChannels[CHANID_VRC6_PULSE2]->SetNoteTable(m_iNoteLookupTable);
	m_pChannels[CHANID_VRC6_SAWTOOTH]->SetNoteTable(m_iNoteLookupTableSaw);
	m_pChannels[CHANID_FDS]->SetNoteTable(m_iNoteLookupTableFDS);
}

stDPCMState CSoundGen::GetDPCMState()
{
	stDPCMState State;

	if (!m_bRunning) {
		State.DeltaCntr = 0;
		State.SamplePos = 0;
	}
	else {
		State.DeltaCntr = m_pAPU->GetDeltaCounter();
		State.SamplePos = m_pAPU->GetSamplePos();
	}

	return State;
}

// Todo: remove this
void CSoundGen::SetDocument(CFamiTrackerDoc *pDoc, CFamiTrackerView *pView)
{
	pDocument	 = pDoc;
	pTrackerView = pView;	

	for (int i = 0; i < CHANNELS; i++) {
		if (m_pChannels[i])
			m_pChannels[i]->InitChannel(m_pAPU, m_cVibTable, pDocument);
	}
}

void CSoundGen::PlayNote(int Channel, stChanNote *NoteData, int EffColumns)
{	
	if (!NoteData)
		return;

	// Update the individual channel
	m_pChannels[Channel]->PlayNote(NoteData, EffColumns);
}

void CSoundGen::EvaluateGlobalEffects(stChanNote *NoteData, int EffColumns)
{
	// Handle global effects (effects that affects all channels)
	for (int i = 0; i < EffColumns; i++) {
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

			// Dxx: Skip to next track and start at row xx
			case EF_SKIP:
				m_iSkipToRow = EffParam;
				FrameIsDone(1);
				if (m_bRendering)
					CheckRenderStop();
				//m_iRenderedFrames++;
				break;

			// Cxx: Halt playback
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
}

void CSoundGen::SetupChannels(int Chip, CFamiTrackerDoc *pDoc) 
{
	unsigned int i;

	// Select APU sound chips
	m_pAPU->SetExternalSound(Chip);

	// Enable MMC5 channels
	if (Chip & SNDCHIP_MMC5) {
		m_pAPU->ExternalWrite(0x5015, 0x03);
	}

	// Initialize channels
	for (i = 0; i < CHANNELS; i++) {
		if (m_pChannels[i]) {
			m_pChannels[i]->InitChannel(m_pAPU, m_cVibTable, pDocument);
			m_pChannels[i]->MakeSilent();
//			m_pChannels[i]->SetNoteTable(m_pNoteLookupTable);
		}
	}

	// This one needs special care
//	((CVRC6Sawtooth*)m_pChannels[CHANID_VRC6_SAWTOOTH])->SetNoteTable(m_iNoteLookupTableSaw);

	// Register the channels in the document
	//
	// Internal channels
	pDoc->RegisterChannel(m_pTrackerChannels[CHANID_SQUARE1], CHANID_SQUARE1, SNDCHIP_NONE);
	pDoc->RegisterChannel(m_pTrackerChannels[CHANID_SQUARE2], CHANID_SQUARE2, SNDCHIP_NONE);
	pDoc->RegisterChannel(m_pTrackerChannels[CHANID_TRIANGLE], CHANID_TRIANGLE, SNDCHIP_NONE);
	pDoc->RegisterChannel(m_pTrackerChannels[CHANID_NOISE], CHANID_NOISE, SNDCHIP_NONE);
	pDoc->RegisterChannel(m_pTrackerChannels[CHANID_DPCM], CHANID_DPCM, SNDCHIP_NONE);

	// Expansion channels
	for (int i = 0; i < CHANNELS; i++) {
		if (m_pTrackerChannels[i] && (m_pTrackerChannels[i]->GetChip() & Chip)) {
			pDoc->RegisterChannel(m_pTrackerChannels[i], i, m_pTrackerChannels[i]->GetChip());
		}
	}
}

bool CSoundGen::RenderToFile(char *File, int SongEndType, int SongEndParam)
{
	if (m_bPlaying)
		StopPlayer();

	m_iRenderEndWhen = (RENDER_END)SongEndType;
	m_iRenderEndParam = SongEndParam;
	m_iRenderedFrames = 0;

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

	// Send this as message instead

	m_bRendering = false;
	pTrackerView->PlayerCommand(CMD_MOVE_TO_START, 0);
	m_WaveFile.CloseFile();
	
	ResetBuffer();
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
		if (m_iRenderedFrames >= m_iRenderEndParam)
			m_bRequestRenderStop = true;
	}
}

// DPCM handling

void CSoundGen::PlaySample(CDSample *pSample, int Offset, int Pitch)
{
	if (m_pPreviewSample) {
		delete [] m_pPreviewSample->SampleData;
		delete m_pPreviewSample;
	}

//	m_pAPU->Reset();

	m_SampleMem.SetMem(pSample->SampleData, pSample->SampleSize);

	int Loop = 0;
	int Length = (pSample->SampleSize >> 4) - (Offset << 2);

	m_pAPU->Write(0x4010, Pitch | Loop);
	m_pAPU->Write(0x4012, Offset);						// load address, start at $C000
	m_pAPU->Write(0x4013, Length);						// length
	m_pAPU->Write(0x4015, 0x0F);
	m_pAPU->Write(0x4015, 0x1F);						// fire sample
	
	// Delete samples with no name
	if (pSample->Name[0] == 0)
		m_pPreviewSample = pSample;
}

// Preview a DPCM sample. If the name of sample is zero, the sample is removed after being played
void CSoundGen::PreviewSample(CDSample *pSample, int Offset, int Pitch)
{
	PostThreadMessage(M_PREVIEW_SAMPLE, (WPARAM)pSample, MAKELPARAM(Offset, Pitch));
}

bool CSoundGen::PreviewDone() const
{
	return m_pAPU->DPCMPlaying() == false;
}

void CSoundGen::WriteAPU(int Address, char Value)
{
	PostThreadMessage(M_WRITE_APU, (WPARAM)Address, (LPARAM)Value);
}
