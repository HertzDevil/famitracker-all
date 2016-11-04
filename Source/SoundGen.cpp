/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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
// TODO: 
//  - Break out actual player functions to a new class CPlayer
//  - Create new interface for CFamiTrackerView with thread-safe functions
//  - Same for CFamiTrackerDoc
//  - Perhaps this should be a worker thread and not GUI thread?
//

#include "stdafx.h"
#include <cmath>
#include <afxmt.h>
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "VisualizerWnd.h"
#include "MainFrm.h"
#include "DirectSound.h"
#include "APU/APU.h"
#include "ChannelHandler.h"
#include "Channels2A03.h"
#include "ChannelsVRC6.h"
#include "ChannelsMMC5.h"
#include "ChannelsVRC7.h"
#include "ChannelsFDS.h"
#include "ChannelsN163.h"
#include "ChannelsS5B.h"
#include "SoundGen.h"
#include "Settings.h"
#include "TrackerChannel.h"
#include "MIDI.h"

#ifdef EXPORT_TEST
#include "ExportTest/ExportTest.h"
#endif /* EXPORT_TEST */

// 1kHz test tone
//#define AUDIO_TEST

// Write period tables to files
//#define WRITE_PERIOD_FILES

// Write a file with the volume table
//#define WRITE_VOLUME_FILE

// Enable audio dithering
//#define DITHERING

// The depth of each vibrato level
const double CSoundGen::NEW_VIBRATO_DEPTH[] = {
	1.0, 1.5, 2.5, 4.0, 5.0, 7.0, 10.0, 12.0, 14.0, 17.0, 22.0, 30.0, 44.0, 64.0, 96.0, 128.0
};

const double CSoundGen::OLD_VIBRATO_DEPTH[] = {
	1.0, 1.0, 2.0, 3.0, 4.0, 7.0, 8.0, 15.0, 16.0, 31.0, 32.0, 63.0, 64.0, 127.0, 128.0, 255.0
};

IMPLEMENT_DYNCREATE(CSoundGen, CWinThread)

BEGIN_MESSAGE_MAP(CSoundGen, CWinThread)
	ON_THREAD_MESSAGE(WM_USER_SILENT_ALL, OnSilentAll)
	ON_THREAD_MESSAGE(WM_USER_LOAD_SETTINGS, OnLoadSettings)
	ON_THREAD_MESSAGE(WM_USER_PLAY, OnStartPlayer)
	ON_THREAD_MESSAGE(WM_USER_STOP, OnStopPlayer)
	ON_THREAD_MESSAGE(WM_USER_RESET, OnResetPlayer)
	ON_THREAD_MESSAGE(WM_USER_START_RENDER, OnStartRender)
	ON_THREAD_MESSAGE(WM_USER_STOP_RENDER, OnStopRender)
	ON_THREAD_MESSAGE(WM_USER_PREVIEW_SAMPLE, OnPreviewSample)
	ON_THREAD_MESSAGE(WM_USER_WRITE_APU, OnWriteAPU)
	ON_THREAD_MESSAGE(WM_USER_CLOSE_SOUND, OnCloseSound)
	ON_THREAD_MESSAGE(WM_USER_SET_CHIP, OnSetChip)
	ON_THREAD_MESSAGE(WM_USER_VERIFY_EXPORT, OnVerifyExport)
	ON_THREAD_MESSAGE(WM_USER_REMOVE_DOCUMENT, OnRemoveDocument)
END_MESSAGE_MAP()

#ifdef DITHERING
int dither(long size);
#endif

CSoundGen::CSoundGen() : 
	m_pAPU(NULL),
	m_pSampleMem(NULL),
	m_pDSound(NULL),
	m_pDSoundChannel(NULL),
	m_pAccumBuffer(NULL),
	m_iGraphBuffer(NULL),
	m_pDocument(NULL),
	m_pTrackerView(NULL),
	m_bRendering(false),
	m_bPlaying(false),
	m_bHaltRequest(false),
	m_pPreviewSample(NULL),
	m_pVisualizerWnd(NULL),
	m_iSpeed(0),
	m_iTempo(0),
	m_bWaveChanged(0),
	m_iMachineType(NTSC),
	m_bRunning(false),
	m_hInterruptEvent(NULL),
	m_bBufferTimeout(false),
	m_bDirty(false),
	m_iQueuedFrame(-1),
	m_iPlayFrame(0),
	m_iPlayRow(0),
	m_iPlayTrack(0),
	m_iPlayTicks(0),
	m_bBufferUnderrun(false),
	m_bAudioClipping(false),
	m_iClipCounter(0),
	m_pSequencePlayPos(NULL),
	m_iSequencePlayPos(0),
	m_iSequenceTimeout(0)
{
	TRACE0("SoundGen: Object created\n");

	// DPCM sample interface
	m_pSampleMem = new CSampleMem();

	// Create APU
	m_pAPU = new CAPU(this, m_pSampleMem);

	// Create all kinds of channels
	CreateChannels();

#ifdef EXPORT_TEST
	m_bExportTesting = false;
#endif /* EXPORT_TEST */
}

CSoundGen::~CSoundGen()
{
	// Delete APU
	SAFE_RELEASE(m_pAPU);
	SAFE_RELEASE(m_pSampleMem);

	// Remove channels
	for (int i = 0; i < CHANNELS; ++i) {
		SAFE_RELEASE(m_pChannels[i]);
		SAFE_RELEASE(m_pTrackerChannels[i]);
	}
}

//
// Object initialization, local
//

void CSoundGen::CreateChannels()
{
	// Only called once!

	// Clear all channels
	for (int i = 0; i < CHANNELS; ++i) {
		m_pChannels[i] = NULL;
		m_pTrackerChannels[i] = NULL;
	}

	// 2A03/2A07
	AssignChannel(new CTrackerChannel(_T("Pulse 1"), SNDCHIP_NONE, CHANID_SQUARE1), new CSquare1Chan());
	AssignChannel(new CTrackerChannel(_T("Pulse 2"), SNDCHIP_NONE, CHANID_SQUARE2), new CSquare2Chan());
	AssignChannel(new CTrackerChannel(_T("Triangle"), SNDCHIP_NONE, CHANID_TRIANGLE), new CTriangleChan());
	AssignChannel(new CTrackerChannel(_T("Noise"), SNDCHIP_NONE, CHANID_NOISE), new CNoiseChan());
	AssignChannel(new CTrackerChannel(_T("DPCM"), SNDCHIP_NONE, CHANID_DPCM), new CDPCMChan(m_pSampleMem));

	// Konami VRC6
	AssignChannel(new CTrackerChannel(_T("Pulse 1"), SNDCHIP_VRC6, CHANID_VRC6_PULSE1), new CVRC6Square1());
	AssignChannel(new CTrackerChannel(_T("Pulse 2"), SNDCHIP_VRC6, CHANID_VRC6_PULSE2), new CVRC6Square2());
	AssignChannel(new CTrackerChannel(_T("Sawtooth"), SNDCHIP_VRC6, CHANID_VRC6_SAWTOOTH), new CVRC6Sawtooth());

	// Konami VRC7
	AssignChannel(new CTrackerChannel(_T("FM Channel 1"), SNDCHIP_VRC7, CHANID_VRC7_CH1), new CVRC7Channel());
	AssignChannel(new CTrackerChannel(_T("FM Channel 2"), SNDCHIP_VRC7, CHANID_VRC7_CH2), new CVRC7Channel());
	AssignChannel(new CTrackerChannel(_T("FM Channel 3"), SNDCHIP_VRC7, CHANID_VRC7_CH3), new CVRC7Channel());
	AssignChannel(new CTrackerChannel(_T("FM Channel 4"), SNDCHIP_VRC7, CHANID_VRC7_CH4), new CVRC7Channel());
	AssignChannel(new CTrackerChannel(_T("FM Channel 5"), SNDCHIP_VRC7, CHANID_VRC7_CH5), new CVRC7Channel());
	AssignChannel(new CTrackerChannel(_T("FM Channel 6"), SNDCHIP_VRC7, CHANID_VRC7_CH6), new CVRC7Channel());

	// Nintendo FDS
	AssignChannel(new CTrackerChannel(_T("FDS"), SNDCHIP_FDS, CHANID_FDS), new CChannelHandlerFDS());

	// Nintendo MMC5
	AssignChannel(new CTrackerChannel(_T("Pulse 1"), SNDCHIP_MMC5, CHANID_MMC5_SQUARE1), new CMMC5Square1Chan());
	AssignChannel(new CTrackerChannel(_T("Pulse 2"), SNDCHIP_MMC5, CHANID_MMC5_SQUARE2), new CMMC5Square2Chan());

	// Namco N163
	AssignChannel(new CTrackerChannel(_T("Namco 1"), SNDCHIP_N163, CHANID_N163_CHAN1), new CChannelHandlerN163());
	AssignChannel(new CTrackerChannel(_T("Namco 2"), SNDCHIP_N163, CHANID_N163_CHAN2), new CChannelHandlerN163());
	AssignChannel(new CTrackerChannel(_T("Namco 3"), SNDCHIP_N163, CHANID_N163_CHAN3), new CChannelHandlerN163());
	AssignChannel(new CTrackerChannel(_T("Namco 4"), SNDCHIP_N163, CHANID_N163_CHAN4), new CChannelHandlerN163());
	AssignChannel(new CTrackerChannel(_T("Namco 5"), SNDCHIP_N163, CHANID_N163_CHAN5), new CChannelHandlerN163());
	AssignChannel(new CTrackerChannel(_T("Namco 6"), SNDCHIP_N163, CHANID_N163_CHAN6), new CChannelHandlerN163());
	AssignChannel(new CTrackerChannel(_T("Namco 7"), SNDCHIP_N163, CHANID_N163_CHAN7), new CChannelHandlerN163());
	AssignChannel(new CTrackerChannel(_T("Namco 8"), SNDCHIP_N163, CHANID_N163_CHAN8), new CChannelHandlerN163());

	// Sunsoft 5B
#if 0
	AssignChannel(new CTrackerChannel(_T("Square 1"), SNDCHIP_S5B, CHANID_S5B_CH1), new CS5BChannel1());
	AssignChannel(new CTrackerChannel(_T("Square 2"), SNDCHIP_S5B, CHANID_S5B_CH2), new CS5BChannel2());
	AssignChannel(new CTrackerChannel(_T("Square 3"), SNDCHIP_S5B, CHANID_S5B_CH3), new CS5BChannel3());
#endif
}

void CSoundGen::AssignChannel(CTrackerChannel *pTrackerChannel, CChannelHandler *pRenderer)
{
	int ID = pTrackerChannel->GetID();

	pRenderer->SetChannelID(ID);

	m_pTrackerChannels[ID] = pTrackerChannel;
	m_pChannels[ID] = pRenderer;
}

//
// Object initialization, global
//

void CSoundGen::AssignDocument(CFamiTrackerDoc *pDoc)
{
	// Called from main thread
	ASSERT(GetCurrentThreadId() == theApp.m_nThreadID);

	// Ignore all but the first document (as new documents are used to import files)
	if (m_pDocument != NULL)
		return;

	// Assigns a document to this object
	m_pDocument = pDoc;

	// Setup all channels
	for (int i = 0; i < CHANNELS; ++i) {
		if (m_pChannels[i])
			m_pChannels[i]->InitChannel(m_pAPU, m_iVibratoTable, this);
	}
}

void CSoundGen::AssignView(CFamiTrackerView *pView)
{
	// Called from main thread
	ASSERT(GetCurrentThreadId() == theApp.m_nThreadID);

	if (m_pTrackerView != NULL)
		return;

	// Assigns the tracker view to this object
	m_pTrackerView = pView;
}

void CSoundGen::RemoveDocument()
{
	// Removes both the document and view from this object

	// Called from main thread
	ASSERT(GetCurrentThreadId() == theApp.m_nThreadID);
	ASSERT(m_pDocument != NULL);
	ASSERT(m_hThread != NULL);

	// Player cannot play when removing the document
	StopPlayer();
	WaitForStop();

	PostThreadMessage(WM_USER_REMOVE_DOCUMENT, 0, 0);

	// Wait 5s for thread to clear the pointer
	for (int i = 0; i < 50 && m_pDocument != NULL; ++i)
		Sleep(100);

	if (m_pDocument != NULL) {
		// Thread stuck
		TRACE0("SoundGen: Could not remove document pointer!\n");
	}
}

void CSoundGen::SetVisualizerWindow(CVisualizerWnd *pWnd)
{
	// Called from main thread
	ASSERT(GetCurrentThreadId() == theApp.m_nThreadID);

	m_csVisualizerWndLock.Lock();
	m_pVisualizerWnd = pWnd;
	m_csVisualizerWndLock.Unlock();
}

void CSoundGen::RegisterChannels(int Chip, CFamiTrackerDoc *pDoc)
{
	// This method will add channels to the document object, depending on the expansion chip used.
	// Called from the document object (from the main thread)

	// Called from main thread
	ASSERT(GetCurrentThreadId() == theApp.m_nThreadID);

	// This affects the sound channel interface so it must be synchronized
	pDoc->LockDocument();

	// Clear all registered channels
	pDoc->ResetChannels();

	// Register the channels in the document
	// Expansion & internal channels
	for (int i = 0; i < CHANNELS; ++i) {
		if (m_pTrackerChannels[i] && ((m_pTrackerChannels[i]->GetChip() & Chip) || (i < 5))) {
			pDoc->RegisterChannel(m_pTrackerChannels[i], i, m_pTrackerChannels[i]->GetChip());
		}
	}

	pDoc->UnlockDocument();
}

void CSoundGen::SelectChip(int Chip)
{
	if (IsPlaying())
		StopPlayer();

	if (!WaitForStop()) {
		TRACE0("CSoundGen: Could not stop player!");
		return;
	}

	PostThreadMessage(WM_USER_SET_CHIP, Chip, 0);
}

CChannelHandler *CSoundGen::GetChannel(int Index) const
{
	return m_pChannels[Index];
}

void CSoundGen::DocumentPropertiesChanged(CFamiTrackerDoc *pDocument)
{
	ASSERT(pDocument != NULL);

	for (int i = 0; i < CHANNELS; ++i) {
		if (m_pChannels[i])
			m_pChannels[i]->DocumentPropertiesChanged(pDocument);
	}
	
	m_iSpeedSplitPoint = pDocument->GetSpeedSplitPoint();
}

//
// Interface functions
//

void CSoundGen::StartPlayer(play_mode_t Mode, int Track)
{
	if (!m_hThread)
		return;

	PostThreadMessage(WM_USER_PLAY, Mode, Track);
}

void CSoundGen::StopPlayer()
{
	if (!m_hThread)
		return;

	PostThreadMessage(WM_USER_STOP, 0, 0);
}

void CSoundGen::ResetPlayer(int Track)
{
	if (!m_hThread)
		return;

	PostThreadMessage(WM_USER_RESET, Track, 0);
}

void CSoundGen::LoadSettings()
{
	if (!m_hThread)
		return;

	PostThreadMessage(WM_USER_LOAD_SETTINGS, 0, 0);
}

void CSoundGen::SilentAll()
{
	if (!m_hThread)
		return;

	PostThreadMessage(WM_USER_SILENT_ALL, 0, 0);
}

void CSoundGen::WriteAPU(int Address, char Value)
{
	if (!m_hThread)
		return;

	// Direct APU interface
	PostThreadMessage(WM_USER_WRITE_APU, (WPARAM)Address, (LPARAM)Value);
}

void CSoundGen::PreviewSample(CDSample *pSample, int Offset, int Pitch)
{
	if (!m_hThread)
		return;

	// Preview a DPCM sample. If the name of sample is null, 
	// the sample will be removed after played
	PostThreadMessage(WM_USER_PREVIEW_SAMPLE, (WPARAM)pSample, MAKELPARAM(Offset, Pitch));
}

void CSoundGen::CancelPreviewSample()
{
	// Remove references to selected sample.
	// This must be done if a sample is about to be deleted!
	m_pSampleMem->Clear();
}

bool CSoundGen::IsRunning() const
{
	return (m_hThread != NULL) && m_bRunning;
}

//// Sound buffer handling /////////////////////////////////////////////////////////////////////////////////

bool CSoundGen::InitializeSound(HWND hWnd)
{
	// Initialize sound, this is only called once!
	// Start with NTSC by default

	// Called from main thread
	ASSERT(GetCurrentThread() == theApp.m_hThread);
	ASSERT(m_pDSound == NULL);

	// Event used to interrupt the sound buffer synchronization
	m_hInterruptEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	// Create DirectSound object
	m_pDSound = new CDSound(hWnd, m_hInterruptEvent);

	// Out of memory
	if (!m_pDSound)
		return false;

	m_pDSound->EnumerateDevices();

	// Start thread when audio is done
	ResumeThread();

	return true;
}

void CSoundGen::Interrupt() const
{
	if (m_hInterruptEvent != NULL)
		::SetEvent(m_hInterruptEvent);
}

bool CSoundGen::GetSoundTimeout() const 
{
	// Read without reset
	return m_bBufferTimeout;
}

bool CSoundGen::IsBufferUnderrun()
{
	// Read and reset flag
	bool ret(m_bBufferUnderrun);
	m_bBufferUnderrun = false;
	return ret;
}

bool CSoundGen::IsAudioClipping()
{
	// Read and reset flag
	bool ret(m_bAudioClipping);
	m_bAudioClipping = false;
	return ret;
}

bool CSoundGen::ResetAudioDevice()
{
	// Setup sound, return false if failed
	//
	// The application must be able to continue even if this fails
	//

	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);
	ASSERT(m_pDSound != NULL);

	CSettings *pSettings = theApp.GetSettings();

	unsigned int SampleSize = pSettings->Sound.iSampleSize;
	unsigned int SampleRate = pSettings->Sound.iSampleRate;
	unsigned int BufferLen	= pSettings->Sound.iBufferLength;
	unsigned int Device		= pSettings->Sound.iDevice;

	m_iSampleSize = SampleSize;
	m_iAudioUnderruns = 0;
	m_iBufferPtr = 0;

	// Close the old sound channel
	CloseAudioDevice();

	if (Device >= m_pDSound->GetDeviceCount()) {
		// Invalid device detected, reset to 0
		Device = 0;
		pSettings->Sound.iDevice = 0;
	}

	// Reinitialize direct sound
	if (!m_pDSound->SetupDevice(Device)) {
		AfxMessageBox(IDS_DSOUND_ERROR, MB_ICONERROR);
		return false;
	}

	int iBlocks = 2;	// default = 2

	// Create more blocks if a bigger buffer than 100ms is used to reduce lag
	if (BufferLen > 100)
		iBlocks += (BufferLen / 66);

	// Create channel
	m_pDSoundChannel = m_pDSound->OpenChannel(SampleRate, SampleSize, 1, BufferLen, iBlocks);

	// Channel failed
	if (m_pDSoundChannel == NULL) {
		AfxMessageBox(IDS_DSOUND_BUFFER_ERROR, MB_ICONERROR);
		return false;
	}

	// Create a buffer
	m_iBufSizeBytes	  = m_pDSoundChannel->GetBlockSize();
	m_iBufSizeSamples = m_iBufSizeBytes / (SampleSize / 8);

	// Temp. audio buffer
	SAFE_RELEASE_ARRAY(m_pAccumBuffer);
	m_pAccumBuffer = new char[m_iBufSizeBytes];

	// Sample graph buffer
	SAFE_RELEASE_ARRAY(m_iGraphBuffer);
	m_iGraphBuffer = new short[m_iBufSizeSamples];

	// Sample graph rate
	m_csVisualizerWndLock.Lock();

	if (m_pVisualizerWnd)
		m_pVisualizerWnd->SetSampleRate(SampleRate);

	m_csVisualizerWndLock.Unlock();

	if (!m_pAPU->SetupSound(SampleRate, 1, (m_iMachineType == NTSC) ? MACHINE_NTSC : MACHINE_PAL))
		return false;

	m_pAPU->SetChipLevel(CHIP_LEVEL_APU1, float(pSettings->ChipLevels.iLevelAPU1 / 10.0f));
	m_pAPU->SetChipLevel(CHIP_LEVEL_APU2, float(pSettings->ChipLevels.iLevelAPU2 / 10.0f));
	m_pAPU->SetChipLevel(CHIP_LEVEL_VRC6, float(pSettings->ChipLevels.iLevelVRC6 / 10.0f));
	m_pAPU->SetChipLevel(CHIP_LEVEL_VRC7, float(pSettings->ChipLevels.iLevelVRC7 / 10.0f));
	m_pAPU->SetChipLevel(CHIP_LEVEL_MMC5, float(pSettings->ChipLevels.iLevelMMC5 / 10.0f));
	m_pAPU->SetChipLevel(CHIP_LEVEL_FDS, float(pSettings->ChipLevels.iLevelFDS / 10.0f));
	m_pAPU->SetChipLevel(CHIP_LEVEL_N163, float(pSettings->ChipLevels.iLevelN163 / 10.0f));
	m_pAPU->SetChipLevel(CHIP_LEVEL_S5B, float(pSettings->ChipLevels.iLevelS5B / 10.0f));
/*
	m_pAPU->SetChipLevel(SNDCHIP_NONE, 0);//pSettings->ChipLevels.iLevel2A03);
	m_pAPU->SetChipLevel(SNDCHIP_VRC6, 0);//pSettings->ChipLevels.iLevelVRC6);
	m_pAPU->SetChipLevel(SNDCHIP_VRC7, 0);//pSettings->ChipLevels.iLevelVRC7);
	m_pAPU->SetChipLevel(SNDCHIP_MMC5, 0);//pSettings->ChipLevels.iLevelMMC5);
	m_pAPU->SetChipLevel(SNDCHIP_FDS, 0);//pSettings->ChipLevels.iLevelFDS);
//	m_pAPU->SetChipLevel(SNDCHIP_N163, pSettings->ChipLevels.iLevelN163);
//	m_pAPU->SetChipLevel(SNDCHIP_S5B, pSettings->ChipLevels.iLevelS5B);
*/
	// Update blip-buffer filtering 
	m_pAPU->SetupMixer(pSettings->Sound.iBassFilter, pSettings->Sound.iTrebleFilter,  pSettings->Sound.iTrebleDamping, pSettings->Sound.iMixVolume);

	m_bAudioClipping = false;
	m_bBufferUnderrun = false;
	m_bBufferTimeout = false;
	m_iClipCounter = 0;

	TRACE("SoundGen: Created sound channel with params: %i Hz, %i bits, %i ms (%i blocks)\n", SampleRate, SampleSize, BufferLen, iBlocks);

	return true;
}

void CSoundGen::CloseAudioDevice()
{
	// Kill DirectSound
	if (m_pDSoundChannel) {
		m_pDSoundChannel->Stop();
		m_pDSound->CloseChannel(m_pDSoundChannel);
		m_pDSoundChannel = NULL;
	}
}

void CSoundGen::CloseAudio()
{
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);

	CloseAudioDevice();

	if (m_pDSound) {
		m_pDSound->CloseDevice();
		delete m_pDSound;
		m_pDSound = NULL;
	}

	if (m_hInterruptEvent) {
		::CloseHandle(m_hInterruptEvent);
		m_hInterruptEvent = NULL;
	}
}

void CSoundGen::ResetBuffer()
{
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);

	m_iBufferPtr = 0;

	if (m_pDSoundChannel)
		m_pDSoundChannel->ClearBuffer();

	m_pAPU->Reset();
}

void CSoundGen::FlushBuffer(int16 *pBuffer, uint32 Size)
{
	// Callback method from emulation

	// May only be called from sound player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);

	if (!m_pDSoundChannel)
		return;

#ifdef EXPORT_TEST
	if (m_bExportTesting)
		return;
#endif /* EXPORT_TEST */

	if (m_iSampleSize == 8)
		FillBuffer<uint8, 8>(pBuffer, Size);
	else
		FillBuffer<int16, 0>(pBuffer, Size);

	if (m_iClipCounter > 50) {
		// Ignore some clipping to allow the HP-filter adjust itself
		m_iClipCounter = 0;
		m_bAudioClipping = true;
	}
	else if (m_iClipCounter > 0)
		--m_iClipCounter;
}

template <class T, int SHIFT>
void CSoundGen::FillBuffer(int16 *pBuffer, uint32 Size)
{
	// Called when the APU audio buffer is full and
	// ready for playing

	const int SAMPLE_MAX = 32768;

	T *pConversionBuffer = (T*)m_pAccumBuffer;

	for (uint32 i = 0; i < Size; ++i) {
		int16 Sample = pBuffer[i];

		// 1000 Hz test tone
#ifdef AUDIO_TEST
		static double sine_phase = 0;
		Sample = int32(sin(sine_phase) * 10000.0);

		static double freq = 1000;
		// Sweep
		//freq+=0.1;
		if (freq > 20000)
			freq = 20;

		sine_phase += freq / (double(m_pDSoundChannel->GetSampleRate()) / 6.283184);
		if (sine_phase > 6.283184)
			sine_phase -= 6.283184;
#endif /* AUDIO_TEST */

		// Clip detection
		if (Sample == (SAMPLE_MAX - 1) || Sample == -SAMPLE_MAX) {
			++m_iClipCounter;
		}

		ASSERT(m_iBufferPtr < m_iBufSizeSamples);

		// Visualizer
		m_iGraphBuffer[m_iBufferPtr] = (short)Sample;

		// Convert sample and store in temp buffer
#ifdef DITHERING
		if (SHIFT > 0)
			Sample = (Sample + dither(1 << SHIFT)) >> SHIFT;
#else
		Sample >>= SHIFT;
#endif

		if (SHIFT == 8)
			Sample ^= 0x80;

		pConversionBuffer[m_iBufferPtr++] = (T)Sample;

		// If buffer is filled, throw it to direct sound
		if (m_iBufferPtr >= m_iBufSizeSamples) {
			if (!PlayBuffer())
				return;
		}
	}
}

bool CSoundGen::PlayBuffer()
{
	if (m_bRendering) {
		// Output to file
		m_wfWaveFile.WriteWave(m_pAccumBuffer, m_iBufSizeBytes);
		m_iBufferPtr = 0;
	}
	else {
		// Output to direct sound
		DWORD dwEvent;

		// Wait for a buffer event
		while ((dwEvent = m_pDSoundChannel->WaitForSyncEvent(AUDIO_TIMEOUT)) != BUFFER_IN_SYNC) {
			switch (dwEvent) {
				case BUFFER_TIMEOUT:
					// Buffer timeout
					m_bBufferTimeout = true;
				case BUFFER_CUSTOM_EVENT:
					// Custom event, quit
					m_iBufferPtr = 0;
					return false;
				case BUFFER_OUT_OF_SYNC:
					// Buffer underrun detected
					m_iAudioUnderruns++;
					m_bBufferUnderrun = true;
					break;
			}
		}

		// Write audio to buffer
		m_pDSoundChannel->WriteBuffer(m_pAccumBuffer, m_iBufSizeBytes);

		// Draw graph
		m_csVisualizerWndLock.Lock();

		if (m_pVisualizerWnd)
			m_pVisualizerWnd->FlushSamples(m_iGraphBuffer, m_iBufSizeSamples);

		m_csVisualizerWndLock.Unlock();

		// Reset buffer position
		m_iBufferPtr = 0;
		m_bBufferTimeout = false;
	}

	return true;
}

unsigned int CSoundGen::GetUnderruns() const
{
	return m_iAudioUnderruns;
}

unsigned int CSoundGen::GetFrameRate()
{
	int FrameRate = m_iFrameCounter;
	m_iFrameCounter = 0;
	return FrameRate;
}

//// Tracker playing routines //////////////////////////////////////////////////////////////////////////////

void CSoundGen::GenerateVibratoTable(int Type)
{
	for (int i = 0; i < 16; ++i) {	// depth 
		for (int j = 0; j < 16; ++j) {	// phase
			int value = 0;
			double angle = (double(j) / 16.0) * (3.1415 / 2.0);

			if (Type == VIBRATO_NEW)
				value = int(sin(angle) * NEW_VIBRATO_DEPTH[i] /*+ 0.5f*/);
			else {
				value = (int)((double(j * OLD_VIBRATO_DEPTH[i]) / 16.0) + 1);
			}

			m_iVibratoTable[i * 16 + j] = value;
		}
	}

#ifdef _DEBUG
/*
	CFile a("vibrato.txt", CFile::modeWrite | CFile::modeCreate);
	CString b;
	for (int i = 0; i < 16; i++) {	// depth 
		b = "\t.byte ";
		a.Write(b.GetBuffer(), b.GetLength());
		for (int j = 0; j < 16; j++) {	// phase
			int value = m_iVibratoTable[i * 16 + j];
			b.Format("$%02X, ", value);
			a.Write(b.GetBuffer(), b.GetLength());
		}
		b = "\n";
		a.Write(b.GetBuffer(), b.GetLength());
	}
	a.Close();
*/
#endif
}

void CSoundGen::SetupVibratoTable(int Type)
{
	GenerateVibratoTable(Type);
}

int CSoundGen::ReadVibratoTable(int index) const
{
	return m_iVibratoTable[index];
}

int CSoundGen::ReadNamcoPeriodTable(int index) const
{
	return m_iNoteLookupTableN163[index];
}

void CSoundGen::BeginPlayer(play_mode_t Mode, int Track)
{
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);
	ASSERT(m_pDocument != NULL);
	ASSERT(m_pTrackerView != NULL);

	if (!m_pDocument || !m_pDSoundChannel || !m_pDocument->IsFileLoaded())
		return;

	switch (Mode) {
		// Play from top of pattern
		case MODE_PLAY:
			m_bPlayLooping = false;
			m_iPlayFrame = m_pTrackerView->GetSelectedFrame();
			m_iPlayRow = 0;
			break;
		// Repeat pattern
		case MODE_PLAY_REPEAT:
			m_bPlayLooping = true;
			m_iPlayFrame = m_pTrackerView->GetSelectedFrame();
			m_iPlayRow = 0;
			break;
		// Start of song
		case MODE_PLAY_START:
			m_bPlayLooping = false;
			m_iPlayFrame = 0;
			m_iPlayRow = 0;
			break;
		// From cursor
		case MODE_PLAY_CURSOR:
			m_bPlayLooping = false;
			m_iPlayFrame = m_pTrackerView->GetSelectedFrame();
			m_iPlayRow = m_pTrackerView->GetSelectedRow();
			break;
	}

	m_bPlaying			= true;
	m_bHaltRequest      = false;
	m_iPlayTicks		= 0;
	m_iFramesPlayed		= 0;
	m_iJumpToPattern	= -1;
	m_iSkipToRow		= -1;
	m_bUpdateRow		= true;
	m_iPlayMode			= Mode;
	m_bDirty			= true;
	m_iPlayTrack		= Track;

	memset(m_bFramePlayed, false, sizeof(bool) * MAX_FRAMES);

	ResetTempo();
	ResetAPU();

	MakeSilent();

	m_pTrackerView->MakeSilent();
}

void CSoundGen::HaltPlayer()
{
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);

	// Move player to non-playing state
	m_bPlaying = false;
	m_bHaltRequest = false;

	MakeSilent();

	m_pSampleMem->SetMem(0, 0);
/*
	for (int i = 0; i < CHANNELS; ++i) {
		if (m_pChannels[i] != NULL) {
			m_pChannels[i]->ResetChannel();
		}
	}
*/
#ifdef EXPORT_TEST
	if (m_bExportTesting)
		EndExportTest();
#endif

	// Signal that playback has stopped
	if (m_pTrackerView != NULL)
		m_pTrackerView->PostMessage(WM_USER_PLAYER, m_iPlayFrame, m_iPlayRow);
}

void CSoundGen::ResetAPU()
{
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);

	// Reset the APU
	m_pAPU->Reset();

	// Enable all channels
	m_pAPU->Write(0x4015, 0x0F);
	m_pAPU->Write(0x4017, 0x00);

	// MMC5
	m_pAPU->ExternalWrite(0x5015, 0x03);

	m_pSampleMem->Clear();
}

void CSoundGen::AddCycles(int Count)
{
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);

	// Add APU cycles
	m_iConsumedCycles += Count;
	m_pAPU->AddTime(Count);
}

uint8 CSoundGen::GetReg(int Chip, int Reg) const
{ 
	return m_pAPU->GetReg(Chip, Reg);
}

void CSoundGen::MakeSilent()
{
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);

	m_pAPU->Reset();
	m_pSampleMem->Clear();

	for (int i = 0; i < CHANNELS; ++i) {
		if (m_pChannels[i])
			m_pChannels[i]->ResetChannel();
		if (m_pTrackerChannels[i])
			m_pTrackerChannels[i]->Reset();
	}
}

void CSoundGen::ResetState()
{
	// Called when a new module is loaded
	m_iPlayTrack = 0;
}

// Get tempo values from the document
void CSoundGen::ResetTempo()
{
	ASSERT(m_pDocument != NULL);

	if (!m_pDocument)
		return;

	m_iSpeed = m_pDocument->GetSongSpeed(m_iPlayTrack);
	m_iTempo = m_pDocument->GetSongTempo(m_iPlayTrack);
	
	SetupSpeed();
	m_iTempoAccum = 0;
	m_iTempoFrames = 0;

	m_bUpdateRow = false;
}

void CSoundGen::SetupSpeed()
{
	m_iTempoDecrement = (m_iTempo * 24) / m_iSpeed;
	m_iTempoRemainder = (m_iTempo * 24) % m_iSpeed;
}

// Return current tempo setting in BPM
float CSoundGen::GetTempo() const
{
	return !m_iSpeed ? 0 : float(m_iTempo * 6) / float(m_iSpeed);
}

void CSoundGen::RunFrame()
{
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);
	ASSERT(m_pDocument != NULL);
	ASSERT(m_pTrackerView != NULL);

	// View callback
	m_pTrackerView->PlayerTick();

	if (IsPlaying()) {
		
		++m_iPlayTicks;

		if (m_bRendering) {
			if (m_iRenderEndWhen == SONG_TIME_LIMIT) {
				if (m_iPlayTicks >= (unsigned int)m_iRenderEndParam)
					m_bRequestRenderStop = true;
			}
			else if (m_iRenderEndWhen == SONG_LOOP_LIMIT) {
				if (m_iFramesPlayed >= m_iRenderEndParam)
					m_bRequestRenderStop = true;
			}
			if (m_bRequestRenderStop)
				m_bHaltRequest = true;
		}

#ifdef EXPORT_TEST
		if (m_bExportTesting) {
			if (m_bFramePlayed[m_iPlayFrame] == true) {
				EndExportTest();
			}
		}
#endif /* EXPORT_TEST */

		m_iStepRows = 0;

		// Fetch next row
		if (m_iTempoAccum <= 0) {
			// Enable this to skip rows on high tempos
//			while (m_iTempoAccum <= 0)  {
				m_iStepRows++;
				m_iTempoFrames = 0;
//			}
			m_bUpdateRow = true;
			ReadPatternRow();
			++m_iRenderRow;
		}
		else {
			m_bUpdateRow = false;
		}
	}
}

void CSoundGen::CheckControl()
{
	// This function takes care of jumping and skipping
	ASSERT(m_pTrackerView != NULL);

	if (IsPlaying()) {
		// If looping, halt when a jump or skip command are encountered
		if (m_bPlayLooping) {
			if (m_iJumpToPattern != -1 || m_iSkipToRow != -1)
				m_iPlayRow = 0;
			else {
				while (m_iStepRows--)
					PlayerStepRow();
			}
		}
		else {
			// Jump
			if (m_iJumpToPattern != -1)
				PlayerJumpTo(m_iJumpToPattern);
			// Skip
			else if (m_iSkipToRow != -1)
				PlayerSkipTo(m_iSkipToRow);
			// or just move on
			else {
				while (m_iStepRows--)
					PlayerStepRow();
			}
		}

		m_iJumpToPattern = -1;
		m_iSkipToRow = -1;
	}

#ifdef EXPORT_TEST
	if (m_bExportTesting)
		m_bDirty = false;
#endif

	if (m_bDirty) {
		m_bDirty = false;
		if (!m_bRendering)
			m_pTrackerView->PostMessage(WM_USER_PLAYER, m_iPlayFrame, m_iPlayRow);
	}
}

void CSoundGen::LoadMachineSettings(int Machine, int Rate, int NamcoChannels)
{
	// Setup machine-type and speed
	//
	// Machine = NTSC or PAL
	//
	// Rate = frame rate (0 means machine default)
	//

	ASSERT(m_pAPU != NULL);

	const double BASE_FREQ = 32.7032;

	int BaseFreq	= (Machine == NTSC) ? CAPU::BASE_FREQ_NTSC  : CAPU::BASE_FREQ_PAL;
	int DefaultRate = (Machine == NTSC) ? CAPU::FRAME_RATE_NTSC : CAPU::FRAME_RATE_PAL;

	m_iMachineType = Machine;

	m_pAPU->ChangeMachine(Machine == NTSC ? MACHINE_NTSC : MACHINE_PAL);

	// Choose a default rate if not predefined
	if (Rate == 0)
		Rate = DefaultRate;

	double clock_ntsc = CAPU::BASE_FREQ_NTSC / 16.0;
	double clock_pal = CAPU::BASE_FREQ_PAL / 16.0;

	for (int i = 0; i < NOTE_COUNT; ++i) {
		// Frequency (in Hz)
		double Freq = BASE_FREQ * pow(2.0, double(i) / 12.0);
		double Pitch;

		// 2A07
		Pitch = (clock_pal / Freq) - 0.5;
		m_iNoteLookupTablePAL[i] = (unsigned int)Pitch;

		// 2A03 / MMC5 / VRC6
		Pitch = (clock_ntsc / Freq) - 0.5;
		m_iNoteLookupTableNTSC[i] = (unsigned int)Pitch;

		// VRC6 Saw
		Pitch = ((clock_ntsc * 16.0) / (Freq * 14.0)) - 0.5;
		m_iNoteLookupTableSaw[i] = (unsigned int)Pitch;

		// FDS
#ifdef TRANSPOSE_FDS
		Pitch = (Freq * 65536.0) / (clock_ntsc / 1.0) + 0.5;
#else
		Pitch = (Freq * 65536.0) / (clock_ntsc / 4.0) + 0.5;
#endif
		m_iNoteLookupTableFDS[i] = (unsigned int)Pitch;

		// N163
		Pitch = (Freq * double(NamcoChannels) * 983040.0) / clock_ntsc;
		m_iNoteLookupTableN163[i] = (unsigned int)(Pitch) / 4;

		if (m_iNoteLookupTableN163[i] > 0xFFFF)	// 0x3FFFF
			m_iNoteLookupTableN163[i] = 0xFFFF;	// 0x3FFFF

		// Sunsoft 5B
		Pitch = (clock_ntsc / Freq) - 0.5;
		m_iNoteLookupTableS5B[i] = (unsigned int)Pitch;
	}
/*
	CStdioFile period_file("periods.txt", CStdioFile::modeWrite | CStdioFile::modeCreate);

	const char NOTES_A[] = {'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'};
	const char NOTES_B[] = {'-', '#', '-', '#', '-', '-', '#', '-', '#', '-', '#', '-'};
	const char NOTES_C[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

	double total_diff = 0;

	for (int i = 0; i < NOTE_COUNT; ++i) {
		CString str;
		int period = m_iNoteLookupTableNTSC[i];
		double freq;
		if (period > 0x7FF)
			period = 0x7FF;
		double real_freq = BASE_FREQ * pow(2.0, double(i) / 12.0);

		freq = clock_ntsc / (1 + period);
		str.Format("%c%c%c: $%04X (%f Hz),\t%f Hz\t(diff = %f Hz)\n", NOTES_A[i % 12], NOTES_B[i % 12], NOTES_C[i / 12], period, freq, real_freq, (freq - real_freq));
		period_file.WriteString(str);

		total_diff += abs(freq - real_freq);
	}

	CString str;
	str.Format("total diff: %f\n", total_diff);
	period_file.WriteString(str);

	period_file.Close();
*/
#ifdef WRITE_PERIOD_FILES

	// Write periods to a single file with assembly formatting
	CStdioFile period_file("..\\nsf driver\\periods.s", CStdioFile::modeWrite | CStdioFile::modeCreate);

	// One possible optimization is to store the PAL table as the difference from the NTSC table

	period_file.WriteString("; 2A03 NTSC\n");
	period_file.WriteString(".ifdef NTSC_PERIOD_TABLE\n");
	period_file.WriteString("ft_periods_ntsc:\n\t.word\t");

	for (int i = 0; i < 96; ++i) {
		CString str;
		str.Format("$%04X%s", m_iNoteLookupTableNTSC[i], ((i % 12) < 11) && (i < 95) ? ", " : ((i < 95) ? "\n\t.word\t" : "\n"));
		period_file.WriteString(str);
	}

	period_file.WriteString(".endif\n\n");
	period_file.WriteString("; 2A03 PAL\n");
	period_file.WriteString(".ifdef PAL_PERIOD_TABLE\n");
	period_file.WriteString("ft_periods_pal:\n\t.word\t");

	for (int i = 0; i < 96; ++i) {
		CString str;
		str.Format("$%04X%s", m_iNoteLookupTablePAL[i], ((i % 12) < 11) && (i < 95) ? ", " : ((i < 95) ? "\n\t.word\t" : "\n"));
		period_file.WriteString(str);
	}

	period_file.WriteString(".endif\n\n");
	period_file.WriteString("; VRC6 Sawtooth\n");
	period_file.WriteString(".ifdef VRC6_PERIOD_TABLE\n");
	period_file.WriteString("ft_periods_sawtooth:\n\t.word\t");

	for (int i = 0; i < 96; ++i) {
		CString str;
		str.Format("$%04X%s", m_iNoteLookupTableSaw[i], ((i % 12) < 11) && (i < 95) ? ", " : ((i < 95) ? "\n\t.word\t" : "\n"));
		period_file.WriteString(str);
	}

	period_file.WriteString(".endif\n\n");
	period_file.WriteString("; FDS\n");
	period_file.WriteString(".ifdef FDS_PERIOD_TABLE\n");
	period_file.WriteString("ft_periods_fds:\n\t.word\t");

	for (int i = 0; i < 96; ++i) {
		CString str;
		str.Format("$%04X%s", m_iNoteLookupTableFDS[i], ((i % 12) < 11) && (i < 95) ? ", " : ((i < 95) ? "\n\t.word\t" : "\n"));
		period_file.WriteString(str);
	}

	period_file.WriteString(".endif\n\n");
	period_file.WriteString("; N163\n");
	period_file.WriteString(".ifdef N163_PERIOD_TABLE\n");
	period_file.WriteString("ft_periods_n163:\n\t.word\t");

	for (int i = 0; i < 96; ++i) {
		CString str;
		str.Format("$%04X%s", m_iNoteLookupTableN163[i] & 0xFFFF, ((i % 12) < 11) && (i < 95) ? ", " : ((i < 95) ? "\n\t.word\t" : "\n"));
		period_file.WriteString(str);
	}

	period_file.WriteString(".endif\n\n");

	period_file.Close();

#endif

#if WRITE_VOLUME_FILE
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
#endif /* WRITE_VOLUME_FILE */

	if (Machine == NTSC)
		m_pNoteLookupTable = m_iNoteLookupTableNTSC;
	else
		m_pNoteLookupTable = m_iNoteLookupTablePAL;

	// Number of cycles between each APU update
	m_iUpdateCycles = BaseFreq / Rate;

	// Setup note tables
	m_pChannels[CHANID_SQUARE1]->SetNoteTable(m_pNoteLookupTable);
	m_pChannels[CHANID_SQUARE2]->SetNoteTable(m_pNoteLookupTable);
	m_pChannels[CHANID_TRIANGLE]->SetNoteTable(m_pNoteLookupTable);
	m_pChannels[CHANID_MMC5_SQUARE1]->SetNoteTable(m_iNoteLookupTableNTSC);
	m_pChannels[CHANID_MMC5_SQUARE2]->SetNoteTable(m_iNoteLookupTableNTSC);
	m_pChannels[CHANID_VRC6_PULSE1]->SetNoteTable(m_iNoteLookupTableNTSC);
	m_pChannels[CHANID_VRC6_PULSE2]->SetNoteTable(m_iNoteLookupTableNTSC);
	m_pChannels[CHANID_VRC6_SAWTOOTH]->SetNoteTable(m_iNoteLookupTableSaw);
	m_pChannels[CHANID_FDS]->SetNoteTable(m_iNoteLookupTableFDS);

	m_pChannels[CHANID_N163_CHAN1]->SetNoteTable(m_iNoteLookupTableN163);
	m_pChannels[CHANID_N163_CHAN2]->SetNoteTable(m_iNoteLookupTableN163);
	m_pChannels[CHANID_N163_CHAN3]->SetNoteTable(m_iNoteLookupTableN163);
	m_pChannels[CHANID_N163_CHAN4]->SetNoteTable(m_iNoteLookupTableN163);
	m_pChannels[CHANID_N163_CHAN5]->SetNoteTable(m_iNoteLookupTableN163);
	m_pChannels[CHANID_N163_CHAN6]->SetNoteTable(m_iNoteLookupTableN163);
	m_pChannels[CHANID_N163_CHAN7]->SetNoteTable(m_iNoteLookupTableN163);
	m_pChannels[CHANID_N163_CHAN8]->SetNoteTable(m_iNoteLookupTableN163);
#if 0
	m_pChannels[CHANID_S5B_CH1]->SetNoteTable(m_iNoteLookupTableS5B);
	m_pChannels[CHANID_S5B_CH2]->SetNoteTable(m_iNoteLookupTableS5B);
	m_pChannels[CHANID_S5B_CH3]->SetNoteTable(m_iNoteLookupTableS5B);
#endif
}

int CSoundGen::FindNote(unsigned int Period) const
{
	for (int i = 0; i < NOTE_COUNT; ++i) {
		if (Period >= m_pNoteLookupTable[i])
			return i;
	}
	return 0;
}

unsigned int CSoundGen::GetPeriod(int Note) const
{
	return m_pNoteLookupTable[Note];
}

stDPCMState CSoundGen::GetDPCMState() const
{
	stDPCMState State;

	if (m_pAPU == NULL) {
		State.DeltaCntr = 0;
		State.SamplePos = 0;
	}
	else {
		State.DeltaCntr = m_pAPU->GetDeltaCounter();
		State.SamplePos = m_pAPU->GetSamplePos();
	}

	return State;
}

void CSoundGen::PlayNote(int Channel, stChanNote *NoteData, int EffColumns)
{	
	if (!NoteData)
		return;

	// Update the individual channel
	m_pChannels[Channel]->PlayNote(NoteData, EffColumns);
}

void CSoundGen::SetSkipRow(int Row)
{
	m_iSkipToRow = Row;
}

void CSoundGen::SetJumpPattern(int Pattern)
{
	m_iJumpToPattern = Pattern;
}

void CSoundGen::EvaluateGlobalEffects(stChanNote *NoteData, int EffColumns)
{
	// Handle global effects (effects that affects all channels)
	for (int i = 0; i < EffColumns; ++i) {

		unsigned char EffNum   = NoteData->EffNumber[i];
		unsigned char EffParam = NoteData->EffParam[i];

		switch (EffNum) {
			// Fxx: Sets speed to xx
			case EF_SPEED:
				if (!EffParam)
					++EffParam;
				if (EffParam >= m_iSpeedSplitPoint)
					m_iTempo = EffParam;
				else
					m_iSpeed = EffParam;
				SetupSpeed();
				break;

			// Bxx: Jump to pattern xx
			case EF_JUMP:
				SetJumpPattern(EffParam);
				break;

			// Dxx: Skip to next track and start at row xx
			case EF_SKIP:
				SetSkipRow(EffParam);
				break;

			// Cxx: Halt playback
			case EF_HALT:
				m_bHaltRequest = true;
				if (m_bRendering) {
					// Unconditional stop
					++m_iFramesPlayed;
					m_bRequestRenderStop = true;
				}
				break;
		}
	}
}

// File rendering functions

bool CSoundGen::RenderToFile(LPTSTR pFile, render_end_t SongEndType, int SongEndParam, int Track)
{
	// Called from main thread
	ASSERT(GetCurrentThreadId() == theApp.m_nThreadID);
	ASSERT(m_pDocument != NULL);

	if (IsPlaying()) {
		//HaltPlayer();
		m_bHaltRequest = true;
		WaitForStop();
	}

	m_iRenderEndWhen = SongEndType;
	m_iRenderEndParam = SongEndParam;
	m_iRenderTrack = Track;
	m_iRenderRowCount = 0;
	m_iRenderRow = 0;

	if (m_iRenderEndWhen == SONG_TIME_LIMIT) {
		// This variable is stored in seconds, convert to frames
		m_iRenderEndParam *= m_pDocument->GetFrameRate();
	}
	else if (m_iRenderEndWhen == SONG_LOOP_LIMIT) {
		m_iRenderEndParam = m_pDocument->ScanActualLength(Track, m_iRenderEndParam, m_iRenderRowCount);
	}

	if (!m_wfWaveFile.OpenFile(pFile, theApp.GetSettings()->Sound.iSampleRate, theApp.GetSettings()->Sound.iSampleSize, 1)) {
		AfxMessageBox(IDS_FILE_OPEN_ERROR);
		return false;
	}
	else
		PostThreadMessage(WM_USER_START_RENDER, 0, 0);

	return true;
}

void CSoundGen::StopRendering()
{
	// Called from player thread
	ASSERT(GetCurrentThreadId() == m_nThreadID);
	ASSERT(m_bRendering);

	if (!IsRendering())
		return;

	m_bPlaying = false;
	m_bRendering = false;
	m_iPlayFrame = 0;
	m_iPlayRow = 0;
	m_wfWaveFile.CloseFile();

	MakeSilent();
	ResetBuffer();
}

void CSoundGen::GetRenderStat(int &Frame, int &Time, bool &Done, int &FramesToRender, int &Row, int &RowCount) const
{
	Frame = m_iFramesPlayed;
	Time = m_iPlayTicks / m_pDocument->GetFrameRate();
	Done = m_bRendering;
	FramesToRender = m_iRenderEndParam;
	RowCount = m_iRenderRowCount;
	Row = m_iRenderRow;
}

bool CSoundGen::IsRendering() const
{
	return m_bRendering;
}

bool CSoundGen::IsBackgroundTask() const
{
#ifdef EXPORT_TEST
	if (m_bExportTesting)
		return true;
#endif
	return m_bRendering;
}

// DPCM handling

void CSoundGen::PlaySample(const CDSample *pSample, int Offset, int Pitch)
{
	SAFE_RELEASE(m_pPreviewSample);

	// Sample may not be removed when used by the sample memory class!
	m_pSampleMem->SetMem(pSample->GetData(), pSample->GetSize());

	int Loop = 0;
	int Length = ((pSample->GetSize() - 1) >> 4) - (Offset << 2);

	m_pAPU->Write(0x4010, Pitch | Loop);
	m_pAPU->Write(0x4012, Offset);			// load address, start at $C000
	m_pAPU->Write(0x4013, Length);			// length
	m_pAPU->Write(0x4015, 0x0F);
	m_pAPU->Write(0x4015, 0x1F);			// fire sample
	
	// Auto-delete samples with no name
	if (*pSample->GetName() == 0)
		m_pPreviewSample = pSample;
}

bool CSoundGen::PreviewDone() const
{
	return (m_pAPU->DPCMPlaying() == false);
}

bool CSoundGen::WaitForStop() const
{
	// Wait for player to stop, timeout = 4s
	// The player must have received the stop command or this will fail

	ASSERT(GetCurrentThreadId() != m_nThreadID);

	//return ::WaitForSingleObject(m_hIsPlaying, 4000) == WAIT_OBJECT_0;

	for (int i = 0; i < 40 && IsPlaying(); ++i)
		Sleep(100);

	return !IsPlaying();	// return false if still playing
}

//
// Overloaded functions
//

BOOL CSoundGen::InitInstance()
{
	//
	// Setup the sound player object, called when thread is started
	//

	ASSERT(m_pDocument != NULL);
	ASSERT(m_pTrackerView != NULL);

	// First check if thread creation should be cancelled
	// This will occur when no sound object is available
	
	if (m_pDSound == NULL)
		return FALSE;

	// Set running flag
	m_bRunning = true;

	// Generate default vibrato table
	GenerateVibratoTable(VIBRATO_NEW);

	if (!ResetAudioDevice()) {
		TRACE0("SoundGen: Failed to reset audio device!\n");
		if (m_pVisualizerWnd != NULL)
			m_pVisualizerWnd->ReportAudioProblem();
	}

//	LoadMachineSettings(DEFAULT_MACHINE_TYPE, DEFAULT_MACHINE_TYPE == NTSC ? CAPU::FRAME_RATE_NTSC : CAPU::FRAME_RATE_PAL);

	ResetAPU();

	// Default tempo & speed
	m_iSpeed = DEFAULT_SPEED;
	m_iTempo = (DEFAULT_MACHINE_TYPE == NTSC) ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL;

	TRACE1("SoundGen: Created thread (0x%04x)\n", m_nThreadID);

	SetThreadPriority(THREAD_PRIORITY_TIME_CRITICAL);

	m_iDelayedStart = 0;
	m_iFrameCounter = 0;

//	SetupChannels();

	return TRUE;
}

int CSoundGen::ExitInstance()
{
	// Shutdown the thread

	TRACE1("SoundGen: Closing thread (0x%04x)\n", m_nThreadID);

	// Free allocated memory
	SAFE_RELEASE_ARRAY(m_iGraphBuffer);
	SAFE_RELEASE_ARRAY(m_pAccumBuffer);

	// Make sure sound interface is shut down
	CloseAudio();

	theApp.RemoveSoundGenerator();

	m_bRunning = false;

	return CWinThread::ExitInstance();
}

BOOL CSoundGen::OnIdle(LONG lCount)
{
	//
	// Main loop for audio playback thread
	//

	if (CWinThread::OnIdle(lCount))
		return TRUE;

	if (!m_pDocument || !m_pDSoundChannel || !m_pDocument->IsFileLoaded())
		return TRUE;

	++m_iFrameCounter;

	// Access the document object, skip if access wasn't granted to avoid gaps in audio playback
	if (m_pDocument->LockDocument(0)) {

		// Read module framerate
		m_iFrameRate = m_pDocument->GetFrameRate();

		RunFrame();

		// Play queued notes
		PlayChannelNotes();

		// Update player
		UpdatePlayer();

		// Channel updates (instruments, effects etc)
		UpdateChannels();

		// Unlock document
		m_pDocument->UnlockDocument();
	}

	// Update APU registers
	UpdateAPU();

#ifdef EXPORT_TEST
	if (m_bExportTesting && !m_bHaltRequest)
		CompareRegisters();
#endif /* EXPORT_TEST */

	if (m_bHaltRequest) {
		// Halt has been requested, abort playback here
		HaltPlayer();
	}

	// Rendering
	if (m_bRendering && m_bRequestRenderStop) {
		if (!m_iDelayedEnd)
			StopRendering();
		else
			--m_iDelayedEnd;
	}

	if (m_iDelayedStart > 0) {
		--m_iDelayedStart;
		if (!m_iDelayedStart) {
			PostThreadMessage(WM_USER_PLAY, MODE_PLAY_START, m_iRenderTrack);
		}
	}

	// Check if a previewed sample should be removed
	if (m_pPreviewSample && PreviewDone()) {
		delete m_pPreviewSample;
		m_pPreviewSample = NULL;
	}

	return TRUE;
}

void CSoundGen::PlayChannelNotes()
{
	// Feed queued notes into channels
	const int Channels = m_pDocument->GetChannelCount();

	// Read notes
	for (int i = 0; i < Channels; ++i) {
		int Channel = m_pDocument->GetChannelType(i);
		
		// Run auto-arpeggio, if enabled
		int Arpeggio = m_pTrackerView->GetAutoArpeggio(i);
		if (Arpeggio > 0) {
			m_pChannels[Channel]->Arpeggiate(Arpeggio);
		}

		// Check if new note data has been queued for playing
		if (m_pTrackerChannels[Channel]->NewNoteData()) {
			stChanNote Note = m_pTrackerChannels[Channel]->GetNote();
			PlayNote(Channel, &Note, m_pDocument->GetEffColumns(m_iPlayTrack, i) + 1);
		}

		// Pitch wheel
		int Pitch = m_pTrackerChannels[Channel]->GetPitch();
		m_pChannels[Channel]->SetPitch(Pitch);

		// Update volume meters
		m_pTrackerChannels[Channel]->SetVolumeMeter(m_pAPU->GetVol(Channel));
	}

	// Instrument sequence visualization
	int SelectedChan = m_pTrackerView->GetSelectedChannel();
	if (m_pChannels[SelectedChan])
		m_pChannels[SelectedChan]->UpdateSequencePlayPos();

}

void CSoundGen::UpdatePlayer()
{
	// Update player state

	if (m_bUpdateRow && !m_bHaltRequest)
		CheckControl();

	if (m_bPlaying) {
		if (m_iTempoAccum <= 0) {
			int TicksPerSec = m_pDocument->GetFrameRate();
			m_iTempoAccum += (60 * TicksPerSec) - m_iTempoRemainder;
		}
		m_iTempoAccum -= m_iTempoDecrement;
		++m_iTempoFrames;
	}
}

void CSoundGen::UpdateChannels()
{
	// Update channels
	for (int i = 0; i < CHANNELS; ++i) {
		if (m_pChannels[i] != NULL) {
			if (m_bHaltRequest)
				m_pChannels[i]->ResetChannel();
			else
				m_pChannels[i]->ProcessChannel();
		}
	}
}

void CSoundGen::UpdateAPU()
{
	// Write to APU registers

	const int CHANNEL_DELAY = 250;

	m_iConsumedCycles = 0;

	// Copy wave changed flag
	m_bInternalWaveChanged = m_bWaveChanged;
	m_bWaveChanged = false;

	// Update APU channel registers
	for (int i = 0; i < CHANNELS; ++i) {
		if (m_pChannels[i] != NULL) {
			m_pChannels[i]->RefreshChannel();
			m_pAPU->Process();
			// Add some delay between each channel update
			if (m_iFrameRate == CAPU::FRAME_RATE_NTSC || m_iFrameRate == CAPU::FRAME_RATE_PAL)
				AddCycles(CHANNEL_DELAY);
		}
	}

	// Finish the audio frame
	m_pAPU->AddTime(m_iUpdateCycles - m_iConsumedCycles);
	m_pAPU->Process();

#ifdef LOGGING
	if (m_bPlaying)
		m_pAPU->Log();
#endif
}

// End of overloaded functions

// Thread message handler

void CSoundGen::OnStartPlayer(WPARAM wParam, LPARAM lParam)
{
	BeginPlayer((play_mode_t)wParam, lParam);
}

void CSoundGen::OnSilentAll(WPARAM wParam, LPARAM lParam)
{
	MakeSilent();
}

void CSoundGen::OnLoadSettings(WPARAM wParam, LPARAM lParam)
{
	if (!ResetAudioDevice()) {
		TRACE0("SoundGen: Failed to reset audio device!\n");
		if (m_pVisualizerWnd != NULL)
			m_pVisualizerWnd->ReportAudioProblem();
	}
}

void CSoundGen::OnStopPlayer(WPARAM wParam, LPARAM lParam)
{
	HaltPlayer();
}

void CSoundGen::OnResetPlayer(WPARAM wParam, LPARAM lParam)
{
	// Called when the selected song has changed

	if (IsPlaying())
		BeginPlayer(m_iPlayMode, wParam);

	m_iPlayTrack = wParam;
	m_iPlayFrame = 0;
	m_iPlayRow = 0;
}

void CSoundGen::OnStartRender(WPARAM wParam, LPARAM lParam)
{
	ResetBuffer();
	m_bRequestRenderStop = false;
	m_bRendering = true;
	m_iDelayedStart = 5;	// Wait 5 frames until player starts
	m_iDelayedEnd = 5;
}

void CSoundGen::OnStopRender(WPARAM wParam, LPARAM lParam)
{
	StopRendering();
}

void CSoundGen::OnPreviewSample(WPARAM wParam, LPARAM lParam)
{
	PlaySample(reinterpret_cast<CDSample*>(wParam), LOWORD(lParam), HIWORD(lParam));
}

void CSoundGen::OnWriteAPU(WPARAM wParam, LPARAM lParam)
{
	m_pAPU->Write((uint16)wParam, (uint8)lParam);
}

void CSoundGen::OnCloseSound(WPARAM wParam, LPARAM lParam)
{
	CloseAudio();

	// Notification
	CEvent *pEvent = (CEvent*)wParam;
	if (pEvent != NULL && pEvent->IsKindOf(RUNTIME_CLASS(CEvent)))
		pEvent->SetEvent();
}

void CSoundGen::OnSetChip(WPARAM wParam, LPARAM lParam)
{
	int Chip = wParam;

	m_pAPU->SetExternalSound(Chip);

	// Enable internal channels after reset
	m_pAPU->Write(0x4015, 0x0F);
	m_pAPU->Write(0x4017, 0x00);

	// MMC5
	if (Chip & SNDCHIP_MMC5)
		m_pAPU->ExternalWrite(0x5015, 0x03);
}

void CSoundGen::OnVerifyExport(WPARAM wParam, LPARAM lParam)
{
#ifdef EXPORT_TEST
	m_pExportTest = reinterpret_cast<CExportTest*>(wParam);
	m_bExportTesting = true;
	BeginPlayer(MODE_PLAY_START, lParam);
#endif /* EXPORT_TEST */
}

void CSoundGen::OnRemoveDocument(WPARAM wParam, LPARAM lParam)
{
	// Remove document and view pointers
	m_pDocument = NULL;
	m_pTrackerView = NULL;
	TRACE0("SoundGen: Document removed\n");
}

/*
void CSoundGen::SetMeterDecayRate(int Rate)
{

}
*/

void CSoundGen::RegisterKeyState(int Channel, int Note)
{
	if (m_pTrackerView != NULL)
		m_pTrackerView->PostMessage(WM_USER_NOTE_EVENT, Channel, Note);
}

// FDS & N163

void CSoundGen::WaveChanged()
{
	// Call when FDS or N163 wave is altered from the instrument editor
	m_bWaveChanged = true;
}

bool CSoundGen::HasWaveChanged() const
{
	return m_bInternalWaveChanged;
}

// Player state functions

void CSoundGen::ReadPatternRow()
{
	const int Channels = m_pDocument->GetChannelCount();
	stChanNote NoteData;

	for (int i = 0; i < Channels; ++i) {
		if (m_pTrackerView->PlayerGetNote(m_iPlayTrack, m_iPlayFrame, i, m_iPlayRow, NoteData))
			QueueNote(i, NoteData, NOTE_PRIO_1);
	}
}

void CSoundGen::PlayerStepRow()
{
	const int PatternLen = m_pDocument->GetPatternLength(m_iPlayTrack);

	if (++m_iPlayRow >= PatternLen) {
		m_iPlayRow = 0;
		if (!m_bPlayLooping)
			PlayerStepFrame();
	}

	m_bDirty = true;
}

void CSoundGen::PlayerStepFrame()
{
	const int Frames = m_pDocument->GetFrameCount(m_iPlayTrack);

	m_bFramePlayed[m_iPlayFrame] = true;

	if (m_iQueuedFrame == -1) {
		if (++m_iPlayFrame >= Frames)
			m_iPlayFrame = 0;
	}
	else {
		m_iPlayFrame = m_iQueuedFrame;
		m_iQueuedFrame = -1;
	}

	++m_iFramesPlayed;

	m_bDirty = true;
}

void CSoundGen::PlayerJumpTo(int Frame)
{
	const int Frames = m_pDocument->GetFrameCount(m_iPlayTrack);

	m_bFramePlayed[m_iPlayFrame] = true;

	m_iPlayFrame = Frame;

	if (m_iPlayFrame >= Frames)
		m_iPlayFrame = Frames - 1;

	m_iPlayRow = 0;

	++m_iFramesPlayed;

	m_bDirty = true;
}

void CSoundGen::PlayerSkipTo(int Row)
{
	const int Frames = m_pDocument->GetFrameCount(m_iPlayTrack);
	const int Rows = m_pDocument->GetPatternLength(m_iPlayTrack);
	
	m_bFramePlayed[m_iPlayFrame] = true;

	if (++m_iPlayFrame >= Frames)
		m_iPlayFrame = 0;

	m_iPlayRow = Row;

	if (m_iPlayRow >= Rows)
		m_iPlayRow = Rows - 1;

	++m_iFramesPlayed;

	m_bDirty = true;
}

void CSoundGen::QueueNote(int Channel, stChanNote &NoteData, note_prio_t Priority) const
{
	if (m_pDocument == NULL)
		return;

	// Queue a note for play
	m_pDocument->GetChannel(Channel)->SetNote(NoteData, Priority);
	theApp.GetMIDI()->WriteNote(Channel, NoteData.Note, NoteData.Octave, NoteData.Vol);
}

int	CSoundGen::GetPlayerRow() const
{
	return m_iPlayRow;
}

int CSoundGen::GetPlayerFrame() const
{
	return m_iPlayFrame;
}

int CSoundGen::GetPlayerTrack() const
{
	return m_iPlayTrack;
}

int CSoundGen::GetPlayerTicks() const
{
	return m_iPlayTicks;
}

void CSoundGen::MoveToFrame(int Frame)
{
	// Todo: synchronize
	m_iPlayFrame = Frame;
	m_iPlayRow = 0;
}

void CSoundGen::SetQueueFrame(int Frame)
{
	m_iQueuedFrame = Frame;
}

int CSoundGen::GetQueueFrame() const
{
	return m_iQueuedFrame;
}

// Verification

#ifdef EXPORT_TEST

static stRegs InternalRegs;

void CSoundGen::WriteRegister(uint16 Reg, uint8 Value)
{
	InternalRegs.R_2A03[Reg & 0x1F] = Value;
}

void CSoundGen::WriteExternalRegister(uint16 Reg, uint8 Value)
{
	// VRC6
	if (Reg >= 0x9000 && Reg < 0x9003)
		InternalRegs.R_VRC6[Reg & 0x03] = Value;
	else if (Reg >= 0xA000 && Reg < 0xA003)
		InternalRegs.R_VRC6[(Reg & 0x03) + 3] = Value;
	else if (Reg >= 0xB000 && Reg < 0xB003)
		InternalRegs.R_VRC6[(Reg & 0x03) + 6] = Value;
}

#else /* EXPORT_TEST */

void CSoundGen::WriteRegister(uint16 Reg, uint8 Value)
{
	// Empty
}

void CSoundGen::WriteExternalRegister(uint16 Reg, uint8 Value)
{
	// Empty
}

#endif /* EXPORT_TEST */

#ifdef EXPORT_TEST

void CSoundGen::EndExportTest()
{
	m_bExportTesting = false;
	m_bHaltRequest = true;

	m_pExportTest->ReportSuccess();

	SAFE_RELEASE(m_pExportTest);
}

void CSoundGen::CompareRegisters()
{
	bool bFailed = false;
	stRegs ExternalRegs;

	m_pExportTest->RunPlay();

	// Read and compare regs

	for (int i = 0; i < 0x14; ++i) {
		ExternalRegs.R_2A03[i] = m_pExportTest->ReadReg(i, SNDCHIP_NONE);
		if (InternalRegs.R_2A03[i] != ExternalRegs.R_2A03[i]) {
			if (i == 0x11) {
				if ((InternalRegs.R_2A03[i] & 0x7F) != (ExternalRegs.R_2A03[i] & 0x7F))
					bFailed = true;
			}
			else if (i != 0x12)	// Ignore DPCM start address
				bFailed = true;
		}
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_VRC6)) {
		for (int i = 0; i < 9; ++i) {
			ExternalRegs.R_VRC6[i] = m_pExportTest->ReadReg(i, SNDCHIP_VRC6);
			if (InternalRegs.R_VRC6[i] != ExternalRegs.R_VRC6[i]) {
				bFailed = true;
			}
		}
	}

	if (bFailed) {
		// Update tracker view
		m_pTrackerView->PostMessage(WM_USER_PLAYER);

		// Display error message
		if (m_pExportTest->ReportError(&InternalRegs, &ExternalRegs, m_iTempoFrames, m_pDocument->GetExpansionChip())) {
			// Abort
			m_bHaltRequest = true;
			m_bExportTesting = false;
			SAFE_RELEASE(m_pExportTest);
		}
	}
}

#endif /* EXPORT_TEST */

void CSoundGen::SetSequencePlayPos(const CSequence *pSequence, int Pos)
{
	if (pSequence == m_pSequencePlayPos) {
		m_iSequencePlayPos = Pos;
		m_iSequenceTimeout = 5;
	}
}

int CSoundGen::GetSequencePlayPos(const CSequence *pSequence)
{
	if (m_pSequencePlayPos != pSequence)
		m_iSequencePlayPos = -1;

	if (m_iSequenceTimeout == 0)
		m_iSequencePlayPos = -1;
	else
		--m_iSequenceTimeout;

	int Ret = m_iSequencePlayPos;
	m_pSequencePlayPos = pSequence;
	return Ret;
}

int CSoundGen::GetDefaultInstrument() const
{
	return ((CMainFrame*)theApp.m_pMainWnd)->GetSelectedInstrument();
}
