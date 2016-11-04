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

#include "stdafx.h"
#include <cmath>
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "SoundGen.h"
#include "DSound.h"

IMPLEMENT_DYNCREATE(CSoundGen, CWinThread)

const int PITCH	= 1000;		// 100.0 %

const int DEFAULT_CYCLES_NTSC	= (1364 / 12) * 262 / 2;
const int DEFAULT_CYCLES_PAL	= (1264 / 12) * 315;
const int BASE_FREQ_NTSC		= (1789773 * PITCH) / 1000;
const int BASE_FREQ_PAL			= (1662607 * PITCH) / 1000;

const int NotesNTSC[] = {
	(BASE_FREQ_NTSC / 262) / 16, (BASE_FREQ_NTSC / 277) / 16,
	(BASE_FREQ_NTSC / 294) / 16, (BASE_FREQ_NTSC / 311) / 16,
	(BASE_FREQ_NTSC / 330) / 16, (BASE_FREQ_NTSC / 349) / 16,
	(BASE_FREQ_NTSC / 370) / 16, (BASE_FREQ_NTSC / 392) / 16,
	(BASE_FREQ_NTSC / 415) / 16, (BASE_FREQ_NTSC / 440) / 16,
	(BASE_FREQ_NTSC / 469) / 16, (BASE_FREQ_NTSC / 495) / 16
};

const int NotesPAL[] = {
	(BASE_FREQ_PAL / 261) / 16, (BASE_FREQ_PAL / 277) / 16,
	(BASE_FREQ_PAL / 293) / 16, (BASE_FREQ_PAL / 311) / 16,
	(BASE_FREQ_PAL / 329) / 16, (BASE_FREQ_PAL / 349) / 16,
	(BASE_FREQ_PAL / 369) / 16, (BASE_FREQ_PAL / 392) / 16,
	(BASE_FREQ_PAL / 415) / 16, (BASE_FREQ_PAL / 440) / 16,
	(BASE_FREQ_PAL / 466) / 16, (BASE_FREQ_PAL / 493) / 16
};

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

BOOL CSoundGen::InitInstance()
{
	// Setup the synth

	unsigned int BaseOctave = 3;
	unsigned int i;
	unsigned int Octave, NesFreqPAL, NesFreqNTSC;

	memset(m_stChannels, 0, sizeof(CMusicChannel) * 5);

	for (Octave = 0; Octave < 8; Octave++) {

		for (i = 0; i < 12; i++) {

			NesFreqNTSC = NotesNTSC[i];
			NesFreqPAL	= NotesPAL[i];

			if (Octave > BaseOctave) {
				NesFreqNTSC /= (1 << (Octave - BaseOctave));
				NesFreqPAL /= (1 << (Octave - BaseOctave));
			}
			else if (Octave < BaseOctave) {
				NesFreqNTSC *= (1 << (BaseOctave - Octave));
				NesFreqPAL *= (1 << (BaseOctave - Octave));
			}

			if (NesFreqNTSC > 0x7FF)
				NesFreqNTSC = 0x7FF;
			
			if (NesFreqPAL > 0x7FF)
				NesFreqPAL = 0x7FF;

			m_iNoteLookupTable_NTSC[Octave * 12 + i] = NesFreqNTSC;
			m_iNoteLookupTable_PAL[Octave * 12 + i] = NesFreqPAL;
		}
	}

	LoadMachineSettings(NTSC, 60);

	// Create the LFO-table for vibrato and tremolo
	for (i = 0; i < VIBRATO_LENGTH; i++) {		
		double k, m, C, o;
		k = (double(i) / double((VIBRATO_LENGTH - 1))) * 6.283185;
		m = (6.283185 * 3) / 4;
		C = 128;
		o = 1.0f;
		
		m_cVibTable[i] = unsigned char(C * (sin(k + m) + o));
	}

	return TRUE;
}

bool CSoundGen::InitializeSound(HWND hWnd)
{
	// Start with NTSC by default

	m_iMachineType	= SPEED_NTSC;

	m_hWnd		= hWnd;
	m_pDSound	= new CDSound;
	m_pAPU		= new CAPU;

	m_hNotificationEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	m_pDSound->EnumerateDevices();

	m_pAPU->Init(this, &m_SampleMem);

	if (!LoadBuffer())
		return false;

	return true;
}

CDSound *CSoundGen::GetSoundInterface()
{
	return m_pDSound;
}

void CSoundGen::FlushBuffer(int16 *Buffer, uint32 Size)
{
	// Called when a buffer is filled enough for playing
	//

	const int SAMPLE_MAX = 32767;
	const int SAMPLE_MIN = -32768;

	static int cntr;

	uint32	SamplesInBytes;
	int32	Sample;

	for (uint32 i = 0; i < Size; i++) {

		Sample = Buffer[i];

		if (Sample > SAMPLE_MAX)
			Sample = SAMPLE_MAX;
		if (Sample < SAMPLE_MIN)
			Sample = SAMPLE_MIN;
		
		m_iGraphBuffer[m_iBufferPtr] = Sample;

		if (m_iSampleSize == 8)
			((int8*)m_pAccumBuffer)[m_iBufferPtr++] = (uint8)(Sample >> 8) ^ 0x80;
		else
			((int16*)m_pAccumBuffer)[m_iBufferPtr++] = (int16)Sample;

		if (m_iBufferPtr == m_iBufferSize) {

			// Wait until it's time
			if (m_pDSoundChannel->WaitForDirectSoundEvent() == 1) {
				m_pDSoundChannel->ResetNotification();
				m_pDSoundChannel->WaitForDirectSoundEvent();
			}

			m_iUnderruns	= m_pDSoundChannel->Underruns;
			SamplesInBytes	= m_iBufferSize * (m_iSampleSize / 8);

			m_pDSoundChannel->WriteSoundBuffer(m_pAccumBuffer, SamplesInBytes);

			theApp.DrawSamples((int*)m_iGraphBuffer, m_iBufferSize);
			m_iGraphBuffer = new int32[m_iBufferSize + 16];
			m_iBufferPtr = 0;
		}
	}

	return;
}

void CSoundGen::LoadMachineSettings(int Machine, int Rate)
{
	// Setup machine-type and speed (defaif speed is zero default 
	//

	if (!m_pAPU)
		return;

	m_iMachineType = Machine;

	if (Rate == 0)
		Rate = (Machine == NTSC ? 60 : 50);

	switch (Machine) {
		case NTSC:
			m_pNoteLookupTable = m_iNoteLookupTable_NTSC;
			m_iCycles = BASE_FREQ_NTSC / Rate;
			m_pAPU->ChangeSpeed(SPEED_NTSC);
			break;

		case PAL:
			m_pNoteLookupTable = m_iNoteLookupTable_PAL;
			m_iCycles = BASE_FREQ_PAL / Rate;
			m_pAPU->ChangeSpeed(SPEED_PAL);
			break;
	}

	for (int i = 0; i < 5; i++) {
		m_stChannels[i].SetNoteTable(m_pNoteLookupTable);
	}
}

void CSoundGen::LoadSettings(int SampleRate, int SampleSize, int BufferLength)
{
	m_iSampleRate	= theApp.m_pSettings->Sound.iSampleRate;
	m_iSampleSize	= theApp.m_pSettings->Sound.iSampleSize;
	m_iBufferLen	= theApp.m_pSettings->Sound.iBufferLength;
	m_iBassFreq		= theApp.m_pSettings->Sound.iBassFilter;
	m_iTrebleFreq	= theApp.m_pSettings->Sound.iTrebleFilter;
	m_iTrebleDamp	= theApp.m_pSettings->Sound.iTrebleDamping;

	m_bNewSettings	= true;
}

bool CSoundGen::LoadBuffer()
{
	// Setup sound, return false if failed
	//

	// Allocate a direct sound interface
	if (m_pDSoundChannel)
		m_pDSound->CloseChannel(m_pDSoundChannel);

	// Reinitialize sound channel

	int DevID = m_pDSound->MatchDeviceID((char*)theApp.m_pSettings->strDevice.GetBuffer());
	m_pDSound->Init(m_hWnd, m_hNotificationEvent, DevID);

	// Create channel
	m_pDSoundChannel = m_pDSound->OpenChannel(m_iSampleRate, m_iSampleSize, 1, m_iBufferLen, 2);

	if (m_pDSoundChannel == NULL)
		return false;

	m_pDSoundChannel->Reset();

	// Create a buffer
	m_iBufferSize = m_pDSoundChannel->GetBlockSize() / (m_iSampleSize / 8);

	if (m_pAccumBuffer)
		delete [] m_pAccumBuffer;

	if (m_iSampleSize == 16)
		m_pAccumBuffer = new int16[m_iBufferSize];
	else
		m_pAccumBuffer = new int8[m_iBufferSize];

	// Sample graph buffer
	if (m_iGraphBuffer)
		delete [] m_iGraphBuffer;

	m_iGraphBuffer = new int32[m_iBufferSize];

	// Allocate an APU buffer
	if (!m_pAPU->AllocateBuffer(m_iSampleRate, 1, SPEED_NTSC))
		return false;

	switch (m_iMachineType) {
		case NTSC: m_pAPU->ChangeSpeed(SPEED_NTSC); break;
		case PAL: m_pAPU->ChangeSpeed(SPEED_PAL); break;
	}

	// Update blip-buffer filtering 
	m_pAPU->SetupMixer(m_iBassFreq, m_iTrebleFreq, m_iTrebleDamp);

	m_bNewSettings	= false;
	m_iBufferPtr	= 0;
	m_iUnderruns	= 0;

	return true;
}

unsigned int CSoundGen::GetOutput(int Chan)
{
	if (!m_bRunning)
		return 0;

	return m_pAPU->GetVol(Chan);
}

int CSoundGen::Run()
{
	// This is the main entry of the sound synthesizer
	// 

	MSG	msg;
	int	i;

	// Make sure sound isn't skipping due to heavy CPU usage by other applications
	SetThreadPriority(THREAD_PRIORITY_TIME_CRITICAL);

	if (m_pDSoundChannel == NULL) {
		m_bAutoDelete = FALSE;
		return 0;
	}

	m_pAPU->Reset();

	// Enable all channels
	m_pAPU->Write(0x4015, 0x0F);

	m_SampleMem.SetMem(0, 0);

	for (i = 0; i < 5; i++) {
		m_stChannels[i].InitChannel(i);
		KillChannelHard(i);
	}

	m_bRunning = true;

	// The sound generator and NES APU emulation routine
	// This will always run in the background when the tracker is running
	while (m_bRunning) {

		// As of this version (v0.2.3) I exterminated the sync code
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
					case WM_USER + 1:
						m_pAPU->Reset();
						m_SampleMem.SetMem(0, 0);
						for (int i = 0; i < 5; i++) {
							KillChannelHard(i);
						}
						break;
				}
			}
		}

		pTrackerView->PlaybackTick(pDocument);
		theApp.StepFrame();

		for (i = 0; i < 5; i++) {
			if (pTrackerView->NewNoteData[i]) {
				pTrackerView->NewNoteData[i] = false;
				PlayChannel(i, &pTrackerView->CurrentNotes[i], pDocument->GetEffColumns(i) + 1);
			}
		}

		if (m_bRunning) {
			for (i = 0; i < 5; i++) {
				m_stChannels[i].Run(pDocument);
				RefreshChannel(i);
			}

			if (m_pDSoundChannel)
				m_pAPU->Run(m_iCycles);

			if (m_bNewSettings)
				LoadBuffer();
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

void CSoundGen::SetDocument(CFamiTrackerDoc *pDoc, CFamiTrackerView *pView)
{
	pDocument	 = pDoc;
	pTrackerView = pView;	
}

void CSoundGen::PlayChannel(int Channel, stChanNote *NoteData, int EffColumns)
{
	stInstrument	*Inst;
	CMusicChannel	*Chan;

	unsigned int NesFreq;
	unsigned int Note, Octave;
	unsigned char Sweep = 0;
	int PortUp = 0, PortDown = 0;
	int Instrument, Volume, LastInstrument;
	bool Sweeping = false;

	int	InitVolume = 0x0F;

	Chan = m_stChannels + Channel;

	if (!NoteData)
		return;

	Note		= NoteData->Note;
	Octave		= NoteData->Octave;
	Volume		= NoteData->Vol;
	Instrument	= NoteData->Instrument;

	LastInstrument = Chan->Instrument;

	if (Note == HALT) {
		Instrument = MAX_INSTRUMENTS;
		Volume = 0x10;
		Octave = 0;
	}

	// Evaluate effects
	for (int n = 0; n < EffColumns; n++) {
		int EffNum		= NoteData->EffNumber[n];
		int EffParam	= NoteData->EffParam[n];

		switch (EffNum) {
			case EF_VOLUME:
				InitVolume = EffParam;
				if (Note == 0)
					Chan->OutVol = InitVolume;
				break;
			case EF_PORTAON:
				Chan->PortaSpeed = EffParam + 1;
				break;
			case EF_PORTAOFF:
				Chan->PortaSpeed = 0;
				Chan->PortaTo = 0;
				break;
			case EF_SWEEPUP: {
				int Shift = EffParam & 0x07;
				int Period = ((EffParam >> 4) & 0x07) << 4;
				Sweep = 0x88 | Shift | Period;
				Chan->m_iLastFrequency = 0xFFFF;
				if (Note == 0)
					Chan->Sweep = Sweep;
				Sweeping = true;
				break;
				}
			case EF_SWEEPDOWN: {
				int Shift = EffParam & 0x07;
				int Period = ((EffParam >> 4) & 0x07) << 4;
				Sweep = 0x80 | Shift | Period;
				Chan->m_iLastFrequency = 0xFFFF;
				if (Note == 0)
					Chan->Sweep = Sweep;
				Sweeping = true;
				break;
				}
			case EF_VIBRATO:
				Chan->m_iVibratoDepth = (EffParam & 0x0F) + 2;
				Chan->m_iVibratoSpeed = EffParam >> 4;
				if (!EffParam)
					Chan->m_iVibratoPhase = 0;
				break;
			case EF_TREMOLO:
				Chan->m_iTremoloDepth = (EffParam & 0x0F) + 2;
				Chan->m_iTremoloSpeed = EffParam >> 4;
				if (!EffParam)
					Chan->m_iTremoloPhase = 0;
				break;
			case EF_ARPEGGIO:
				Chan->m_cArpeggio = EffParam;
				break;
			case EF_PITCH:
				Chan->m_iFinePitch = EffParam;
				break;
		}
	}

	// Change instrument
	if (Instrument != LastInstrument) {

		if (Instrument == MAX_INSTRUMENTS)
			Instrument = Chan->LastInstrument;
		else
			Chan->LastInstrument = Instrument;

		Inst = pDocument->GetInstrument(Instrument);
		
		if (Inst == NULL)
			return;

		for (int i = 0; i < MOD_COUNT; i++) {
			if (Chan->ModIndex[i] != Inst->ModIndex[i]) {
				Chan->ModEnable[i]	= Inst->ModEnable[i];
				Chan->ModIndex[i]	= Inst->ModIndex[i];
				Chan->ModPointer[i] = 0;
				Chan->ModDelay[i]	= 1;
			}
		}

		Chan->Instrument = Instrument;
	}
	else {
		if (Instrument == MAX_INSTRUMENTS)
			Instrument = Chan->LastInstrument;
		else
			Chan->LastInstrument = Instrument;

		Inst = pDocument->GetInstrument(Instrument);

		if (Inst == NULL)
			return;
	}

	if (Volume < 16)
		Chan->Volume = Volume;

	if (Note == 0) {
		Chan->LastNote = 0;
		return;
	}
	
	if (Note == HALT) {
		KillChannel(Channel);
		pTrackerView->RegisterKeyState(Channel, -1);
		return;
	}

	if (!Sweeping && (Chan->Sweep != 0 || Sweep  != 0)) {
		Sweep = 0;
		Chan->Sweep = 0;
		Chan->m_iLastFrequency = 0xFFFF;
	}
	else if (Sweeping) {
		Chan->Sweep = Sweep;
		Chan->m_iLastFrequency = 0xFFFF;
	}

	// Trigger instrument
	for (int i = 0; i < MOD_COUNT; i++) {
		Chan->ModEnable[i]	= Inst->ModEnable[i];
		Chan->ModIndex[i]	= Inst->ModIndex[i];
		Chan->ModPointer[i] = 0;
		Chan->ModDelay[i]	= 1;
	}

	// And note
	int NewNote = (Note - 1) + (Octave * 12);

	if (Channel == 3)
		NesFreq = NewNote;
	else
		NesFreq = m_pNoteLookupTable[NewNote];

	if (Chan->PortaSpeed > 0) {
		if (Chan->m_iFrequency == 0)
			Chan->m_iFrequency = NesFreq;
		Chan->PortaTo = NesFreq;
	}
	else
		Chan->m_iFrequency = NesFreq;

	Chan->DutyCycle	= 0;
	Chan->Note		= NewNote;
	Chan->Octave	= Octave;
	Chan->OutVol	= InitVolume;
	Chan->m_bEnabled	= true;

	pTrackerView->RegisterKeyState(Channel, (Note - 1) + (Octave * 12));

	// DPCM
	if (Channel == 4) {
		int SampleIndex = Inst->Samples[Octave][Note - 1];
		if (SampleIndex > 0) {
			int Pitch = Inst->SamplePitch[Octave][Note - 1];
			if (Pitch & 0x80)
				Chan->Octave = 0x40;
			else
				Chan->Octave = 0;

			stDSample *DSample = pDocument->GetDSample(SampleIndex - 1);
			m_SampleMem.SetMem(DSample->SampleData, DSample->SampleSize);
			Chan->Length	= DSample->SampleSize;		// this will be adjusted
			Chan->m_iFrequency		= Pitch & 0x0F;
		}
		else
			Chan->m_bEnabled = false;
	}
}

void CSoundGen::RefreshChannel(int Channel)
{
	// Write to the channels
	//

	unsigned char LoFreq, HiFreq;
	unsigned char LastLoFreq, LastHiFreq;

	char DutyCycle;

	int Volume, Freq, VibFreq, TremVol;

	CMusicChannel	*Chan = m_stChannels + Channel;

	if (!Chan->m_bEnabled)
		return;

	VibFreq	= m_cVibTable[Chan->m_iVibratoPhase] >> (0x8 - (Chan->m_iVibratoDepth >> 1));

	if ((Chan->m_iVibratoDepth & 1) == 0)
		VibFreq -= (VibFreq >> 1);

	TremVol	= (m_cVibTable[Chan->m_iTremoloPhase] >> 4) >> (4 - (Chan->m_iTremoloDepth >> 1));

	if ((Chan->m_iTremoloDepth & 1) == 0)
		TremVol -= (TremVol >> 1);

	Freq = Chan->m_iFrequency - VibFreq + (0x80 - Chan->m_iFinePitch);

	HiFreq		= (Freq & 0xFF);
	LoFreq		= (Freq >> 8);
	LastHiFreq	= (Chan->m_iLastFrequency & 0xFF);
	LastLoFreq	= (Chan->m_iLastFrequency >> 8);

	DutyCycle	= (Chan->DutyCycle & 0x03);
	Volume		= (Chan->OutVol - (0x0F - Chan->Volume)) - TremVol;

	if (Volume < 0)
		Volume = 0;
	if (Volume > 15)
		Volume = 15;

	Chan->m_iLastFrequency = Freq;

	switch (Channel) {
		//
		// Square 1
		//
		case 0:	
			m_pAPU->Write(0x4000, (DutyCycle << 6) | 0x30 | Volume);
			if (Chan->Sweep) {
				if (Chan->Sweep & 0x80) {
					m_pAPU->Write(0x4001, Chan->Sweep);
					m_pAPU->Write(0x4002, HiFreq);
					m_pAPU->Write(0x4003, LoFreq);
					Chan->Sweep &= 0x7F;
					// Immediately trigger one sweep cycle (by request)
					m_pAPU->Write(0x4017, 0x80);
					m_pAPU->Write(0x4017, 0x00);
				}
			}
			else {
				m_pAPU->Write(0x4001, 0x08);
				// This one was tricky. Manually execute one APU frame sequence to kill the sweep unit
				m_pAPU->Write(0x4017, 0x80);
				m_pAPU->Write(0x4017, 0x00);
				m_pAPU->Write(0x4002, HiFreq);

				if (LoFreq != LastLoFreq) {

					// Damn, it is impossible to use this hack below since
					// most NSF players don't support it!

//					if (theApp.m_pSettings->General.bDisableSquareHack) {
						m_pAPU->Write(0x4003, LoFreq);
/*
					}
					else {
						// Hack to set the whole frequency without accessing the high-part register
						// credits goes to blargg!
						if (LastLoFreq == 0xFF) {
							m_pAPU->Write(0x4003, LoFreq);
						}
						else if (LastLoFreq > LoFreq) {
							while (LastLoFreq != LoFreq) {
								m_pAPU->Write(0x4017, 0x40);
								m_pAPU->Write(0x4002, 0x00);
								m_pAPU->Write(0x4001, 0x8F);
								// Trigger the sweep unit twice to do it properly
								// (since there's not sure shift will be 7)
								m_pAPU->Write(0x4017, 0xC0);
								m_pAPU->Write(0x4017, 0xC0);
								m_pAPU->Write(0x4017, 0x00);
								LastLoFreq--;
							}
							// Reset the sweep unit afterward
							m_pAPU->Write(0x4001, 0x08);
							m_pAPU->Write(0x4017, 0xC0);
							m_pAPU->Write(0x4017, 0x00);
							m_pAPU->Write(0x4002, HiFreq);
						}
						else if (LastLoFreq < LoFreq) {
							while (LastLoFreq != LoFreq) {
								m_pAPU->Write(0x4017, 0x40);
								m_pAPU->Write(0x4002, 0xFF);
								m_pAPU->Write(0x4001, 0x86);
								m_pAPU->Write(0x4017, 0xC0);
								m_pAPU->Write(0x4017, 0xC0);
								m_pAPU->Write(0x4017, 0x00);
								LastLoFreq++;
							}
							m_pAPU->Write(0x4001, 0x08);
							m_pAPU->Write(0x4017, 0xC0);
							m_pAPU->Write(0x4017, 0x00);
							m_pAPU->Write(0x4002, HiFreq);
						}
					}
*/
				}
			}
			break;
		//
		// Square 2
		//
		case 1:
			m_pAPU->Write(0x4004, (DutyCycle << 6) | 0x30 | Volume);

			if (Chan->Sweep) {
				if (Chan->Sweep & 0x80) {
					m_pAPU->Write(0x4005, Chan->Sweep);
					m_pAPU->Write(0x4006, HiFreq);
					m_pAPU->Write(0x4007, LoFreq);
					Chan->Sweep &= 0x7F;
					// Immediately trigger one sweep cycle
					m_pAPU->Write(0x4017, 0x80);
					m_pAPU->Write(0x4017, 0x00);
				}
			}
			else {
				m_pAPU->Write(0x4005, 0x08);
				// Same as above
				m_pAPU->Write(0x4017, 0x80);
				m_pAPU->Write(0x4017, 0x00);
				m_pAPU->Write(0x4006, HiFreq);

				if (LoFreq != LastLoFreq)  {

//					if (theApp.m_pSettings->General.bDisableSquareHack) {
						m_pAPU->Write(0x4007, LoFreq);
						/*
					}
					else {
						// (Same as above)
						if (LastLoFreq == 0xFF) {
							m_pAPU->Write(0x4007, LoFreq);
						}
						else if (LastLoFreq > LoFreq) {
							while (LastLoFreq != LoFreq) {
								m_pAPU->Write(0x4017, 0x40);
								m_pAPU->Write(0x4006, 0x00);
								m_pAPU->Write(0x4005, 0x8F);
								// Trigger the sweep unit twice to do it properly
								// (since there's not sure shift will be 7)
								m_pAPU->Write(0x4017, 0xC0);
								m_pAPU->Write(0x4017, 0xC0);
								m_pAPU->Write(0x4017, 0x00);
								LastLoFreq--;
							}
							m_pAPU->Write(0x4005, 0x08);
							m_pAPU->Write(0x4017, 0xC0);
							m_pAPU->Write(0x4017, 0x00);
							m_pAPU->Write(0x4006, HiFreq);
						}
						else if (LastLoFreq < LoFreq) {
							while (LastLoFreq != LoFreq) {
								m_pAPU->Write(0x4017, 0x40);
								m_pAPU->Write(0x4006, 0xFF);
								m_pAPU->Write(0x4005, 0x87);
								m_pAPU->Write(0x4017, 0xC0);
								m_pAPU->Write(0x4017, 0xC0);
								m_pAPU->Write(0x4017, 0x00);
								LastLoFreq++;
							}
							m_pAPU->Write(0x4005, 0x08);
							m_pAPU->Write(0x4017, 0xC0);
							m_pAPU->Write(0x4017, 0x00);
							m_pAPU->Write(0x4006, HiFreq);
						}
					}
					*/
				}
			}
			break;
		//
		// Triangle
		//
		case 2:
			if (Chan->OutVol)
				m_pAPU->Write(0x4008, 0xC0);
			else
				m_pAPU->Write(0x4008, 0x00);
			m_pAPU->Write(0x4009, 0x00);
			m_pAPU->Write(0x400A, HiFreq);
			m_pAPU->Write(0x400B, LoFreq);
			break;
		//
		// Noise
		//
		case 3:
			m_pAPU->Write(0x400C, 0x30 | Volume);
			m_pAPU->Write(0x400D, 0x00);
			m_pAPU->Write(0x400E, ((HiFreq & 0x0F) ^ 0x0F) | ((DutyCycle & 0x01) << 7));
			m_pAPU->Write(0x400F, 0x00);
			break;
		//
		// DPCM, this is special
		//
		case 4:
			m_pAPU->Write(0x4010, 0x00 | (HiFreq & 0x0F) | Chan->Octave /*(Chan->Octave > 5 ? 0x40 : 0x00)*/);	// freq & loop 
			//Player->Write(0x4011, 0x00);	// counter load (altering this causes clicks)
			m_pAPU->Write(0x4012, 0x00);	// load address, start at $C000
			m_pAPU->Write(0x4013, Chan->Length / 16);	// length
			m_pAPU->Write(0x4015, 0x0F);
			m_pAPU->Write(0x4015, 0x1F);	// fire sample
			Chan->m_bEnabled = false;	// don't write to this channel anymore
			break;
	}
}

void CSoundGen::KillChannelHard(int Channel)
{
	KillChannel(Channel);
	m_stChannels[Channel].Volume = 0x0F;
	m_stChannels[Channel].PortaSpeed = 0;
	m_stChannels[Channel].DutyCycle = 0;
	m_stChannels[Channel].m_cArpeggio = 0;
	m_stChannels[Channel].m_cArpVar = 0;
	m_stChannels[Channel].m_iVibratoSpeed = m_stChannels[Channel].m_iVibratoPhase = 0;
	m_stChannels[Channel].m_iTremoloSpeed = m_stChannels[Channel].m_iTremoloPhase = 0;
}

void CSoundGen::KillChannel(int Channel)
{
	m_stChannels[Channel].KillChannel();

	switch (Channel) {
		case 0:
			m_pAPU->Write(0x4000, 0);
			m_pAPU->Write(0x4001, 0);
			m_pAPU->Write(0x4002, 0);
			m_pAPU->Write(0x4003, 0);
			break;
		case 1:
			m_pAPU->Write(0x4004, 0);
			m_pAPU->Write(0x4005, 0);
			m_pAPU->Write(0x4006, 0);
			m_pAPU->Write(0x4007, 0);
			break;
		case 2:
			m_pAPU->Write(0x4008, 0);
			m_pAPU->Write(0x4009, 0);
			m_pAPU->Write(0x400A, 0);
			m_pAPU->Write(0x400B, 0);
			break;
		case 3:
			m_pAPU->Write(0x400C, 0x30);	// Remove noise click
			m_pAPU->Write(0x400D, 0);
			m_pAPU->Write(0x400E, 0);
			m_pAPU->Write(0x400F, 0);
			break;
		case 4:
			m_pAPU->Write(0x4015, 0x0F);
			m_pAPU->Write(0x4010, 0);
			m_pAPU->Write(0x4011, 0);		// regain full volume for TN
			m_pAPU->Write(0x4012, 0);
			m_pAPU->Write(0x4013, 0);
			break;
	}
}

int CSoundGen::ExitInstance()
{
	m_bRunning = false;
	return CWinThread::ExitInstance();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CMusicChannel::InitChannel(int ID)
{
	m_iChannelID = ID;
}

void CMusicChannel::KillChannel()
{
	m_bEnabled = false;

	m_iFrequency = 0;
	m_iLastFrequency = 0xFFFF;
	m_iFinePitch = 0x80;
	
	PortaTo = 0;
	//PortaSpeed = 0;
	OutVol = 0x00;
}

void CMusicChannel::Run(CFamiTrackerDoc *pDocument)
{
	int	i;

	if (!m_bEnabled || m_iChannelID == 4)
		return;

	// Vibrato and tremolo
	m_iVibratoPhase = (m_iVibratoPhase + m_iVibratoSpeed) & (VIBRATO_LENGTH - 1);
	m_iTremoloPhase = (m_iTremoloPhase + m_iTremoloSpeed) & (TREMOLO_LENGTH - 1);

	// Automatic portamento
	if (PortaSpeed > 0 && PortaTo > 0) {
		if (m_iFrequency > PortaTo) {
			m_iFrequency -= PortaSpeed;
			if (m_iFrequency > 0x1000)	// it was negative
				m_iFrequency = 0x00;
			if (m_iFrequency < PortaTo)
				m_iFrequency = PortaTo;
		}
		else if (m_iFrequency < PortaTo) {
			m_iFrequency += PortaSpeed;
			if (m_iFrequency > PortaTo)
				m_iFrequency = PortaTo;
		}
	}

	// Arpeggio
	if (m_cArpeggio != 0) {
		switch (m_cArpVar) {
			case 0:
				m_iFrequency = TriggerNote(Note);
				break;
			case 1:
				m_iFrequency = TriggerNote(Note + (m_cArpeggio >> 4));
				if ((m_cArpeggio & 0x0F) == 0)
					m_cArpVar = 2;
				break;
			case 2:
				m_iFrequency = TriggerNote(Note + (m_cArpeggio & 0x0F));
				break;
		}

		if (++m_cArpVar > 2)
			m_cArpVar = 0;
	}
	
	// Sequences
	for (i = 0; i < MOD_COUNT; i++) {
		RunSequence(i, pDocument->GetSequence(ModIndex[i], i));
	}
}

void CMusicChannel::RunSequence(int Index, stSequence *pSequence)
{
	int Value, Pointer, Delay;

	if (ModEnable[Index]) {
		ModDelay[Index]--;

		if (ModDelay[Index] == 0) {
			Pointer = ModPointer[Index];
			Value	= pSequence->Value[Pointer];
			Delay	= pSequence->Length[Pointer];
			
			// Jump: jump and fetch new values
			if (Delay < 0) {
				ModPointer[Index] += Delay;
				if (ModPointer[Index] < 0)
					ModPointer[Index] = 0;

				// New values
				Pointer = ModPointer[Index];
				Value	= pSequence->Value[Pointer];
				Delay	= pSequence->Length[Pointer];
			}

			// Only apply values if it wasn't a jump
			switch (Index) {
				// Volume modifier
				case MOD_VOLUME:
					OutVol = Value;
					break;
				// Arpeggiator
				case MOD_ARPEGGIO:
					m_iFrequency = TriggerNote(Note + Value);
					break;
				// Hi-pitch
				case MOD_HIPITCH:
					m_iFrequency += Value << 4;
					LimitFreq();
					break;
				// Pitch
				case MOD_PITCH:
					m_iFrequency += Value;
					LimitFreq();
					break;
				// Duty cycling
				case MOD_DUTYCYCLE:
					DutyCycle = Value;
					break;
			}

			ModDelay[Index] = Delay + 1;
			ModPointer[Index]++;
		}

		if (ModPointer[Index] == pSequence->Count)
			ModEnable[Index] = 0;
	}
}

void CMusicChannel::LimitFreq()
{
	if (m_iFrequency > 0x7FF)
		m_iFrequency = 0x7FF;

	if (m_iFrequency < 0)
		m_iFrequency = 0;
}

void CMusicChannel::SetNoteTable(unsigned int *NoteLookupTable)
{
	m_pNoteLookupTable = NoteLookupTable;
}

unsigned int CMusicChannel::TriggerNote(int Note)
{
	theApp.RegisterKeyState(m_iChannelID, Note);
	
	if (m_iChannelID == 3)
		return Note;

	return m_pNoteLookupTable[Note];
}
