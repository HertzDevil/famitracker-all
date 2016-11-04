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

int Underruns;

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
	unsigned int BaseOctave = 3;
	unsigned int i;
	unsigned int Octave, NesFreqPAL, NesFreqNTSC;

	memset(m_stChannels, 0, sizeof(stChanData) * 5);

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

	for (i = 0; i < VIBRATO_LENGTH; i++) {
//		m_cVibTable[i] = char(sin((float(i) / float(VIBRATO_LENGTH)) * (2 * 3.1415)) * 64);
		
		double k, m, C, o;
		k = (double(i) / double((VIBRATO_LENGTH - 1))) * 6.283185;
		m = (6.283185 * 3) / 4;
		C = 128;
		o = 1.0f;
		
		//m_cVibTable[i] = char((sin( ) * (2 * 3.1415) /*+ (6.2832 * (3 / 4))*/ ) + 0.0f) * 64 );
		m_cVibTable[i] = unsigned char(C * (sin(k + m) + o));

		// 128 * sin(phase * 2pi + 3pi/4) + 1
		//m_cVibTable[i] = unsigned char(128.0f * sinf( (double(i) / double(VIBRATO_LENGTH - 1)) * 6.283185 + ((6.283185 * 3.0) / 4.0)) + 1.0f);
	}

	for (i = 0; i < TREMOLO_LENGTH; i++) {
//		m_cTremTable[i] = char(sin((float(i) / float(TREMOLO_LENGTH)) * (2 * 3.1415)) * 8);
		//m_cTremTable[i] = unsigned char(sin((float(i) / float(TREMOLO_LENGTH)) * (2 * 3.1415)) * 8);
	}

	return TRUE;
}

bool CSoundGen::InitializeSound(HWND hWnd)
{
	// Start NTSC by default
	m_iMachineType			= SPEED_NTSC;

	m_pDSound				= new CDSound;
	m_pAPU					= new CAPU;
	m_hNotificationEvent	= CreateEvent(NULL, FALSE, FALSE, NULL);

	m_pDSound->Init(hWnd, m_hNotificationEvent);

	m_pAPU->Init(this, &m_SampleMem);

	if (!LoadBuffer())
		return false;

	return true;
}

void CSoundGen::FlushBuffer(int16 *Buffer, uint32 Size)
{
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

			Underruns		= m_pDSoundChannel->Underruns;
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
	Underruns		= 0;

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

	int	i;
	MSG	msg;

	if (m_pDSoundChannel == NULL) {
		m_bAutoDelete = FALSE;
		return 0;
	}

	m_pAPU->Reset();

	// Enable all channels
	m_pAPU->Write(0x4015, 0x0F);

	KillChannel(0);
	KillChannel(1);
	KillChannel(2);
	KillChannel(3);
	KillChannel(4);
	
	m_SampleMem.SetMem(0, 0);

	SetThreadPriority(THREAD_PRIORITY_TIME_CRITICAL);

	m_bRunning = true;

	while (m_bRunning) {

		// As of this version (v0.2.3) I exterminated the sync code
		// Notes are fetched on the fly from the FamiTrackerView, and
		// everything seems to work well so far

		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) && m_bRunning) {

			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				switch (msg.message) {
					case WM_QUIT:
						m_bRunning = false;
						if (m_pDSoundChannel)
							m_pDSoundChannel->Stop();
						break;
					case WM_USER + 1:		// Make everything silent
						m_pAPU->Reset();
						m_SampleMem.SetMem(0, 0);
						memset(m_stChannels, 0, sizeof(stChanData) * 5);
						KillChannel(0);
						KillChannel(1);
						KillChannel(2);
						KillChannel(3);
						KillChannel(4);
						m_stChannels[0].Volume = 0x0F;
						m_stChannels[1].Volume = 0x0F;
						m_stChannels[2].Volume = 0x0F;
						m_stChannels[3].Volume = 0x0F;
						m_stChannels[0].PortaSpeed = 0;
						m_stChannels[1].PortaSpeed = 0;
						m_stChannels[2].PortaSpeed = 0;
						m_stChannels[3].PortaSpeed = 0;
						break;
				}
			}
		}

		pTrackerView->PlaybackTick(pDocument);
		theApp.StepFrame();

		for (i = 0; i < 5; i++) {
			if (pTrackerView->NewNoteData[i]) {
				pTrackerView->NewNoteData[i] = false;
				PlayChannel(i, &pTrackerView->CurrentNotes[i], pTrackerView->EffectColumns[i]);
			}
		}

		if (m_bRunning) {
			for (i = 0; i < 5; i++) {
				ModifyChannel(i);
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
	stChanData		*Chan;

	unsigned int NesFreq;
	unsigned int Note, Octave;
	int PortUp = 0, PortDown = 0;
	unsigned char Sweep = 0;
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
				Chan->LastFreq = 0xFFFF;
				if (Note == 0)
					Chan->Sweep = Sweep;
				Sweeping = true;
				break;
				}
			case EF_SWEEPDOWN: {
				int Shift = EffParam & 0x07;
				int Period = ((EffParam >> 4) & 0x07) << 4;
				Sweep = 0x80 | Shift | Period;
				Chan->LastFreq = 0xFFFF;
				if (Note == 0)
					Chan->Sweep = Sweep;
				Sweeping = true;
				break;
				}
			case EF_VIBRATO:
				Chan->VibratoDepth = (EffParam & 0x0F) + 2;
				Chan->VibratoSpeed = EffParam >> 4;
				if (!EffParam)
					Chan->VibratoPhase = 0;
				break;
			case EF_TREMOLO:
				Chan->TremoloDepth = (EffParam & 0x0F) + 2;
				Chan->TremoloSpeed = EffParam >> 4;
				if (!EffParam)
					Chan->TremoloPhase = 0;
				break;
			case EF_ARPEGGIO:
				Chan->Effect	= EF_ARPEGGIO;
				Chan->EffParam	= EffParam;
				break;
		}
	}

	// Change instrument
	if (Instrument != LastInstrument) {

		if (/*NoteData->Instrument*/ Instrument == MAX_INSTRUMENTS)
			Instrument = Chan->LastInstrument;
		else
			Chan->LastInstrument = /*NoteData->Instrument*/Instrument;

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
		if (/*NoteData->Instrument*/ Instrument == MAX_INSTRUMENTS)
			Instrument = Chan->LastInstrument;
		else
			Chan->LastInstrument = /*NoteData->Instrument*/Instrument;

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

	// sweep is wrong! try enl2

//	if (NoteData->EffNumber[0] != EF_SWEEPUP && NoteData->EffNumber[0] != EF_SWEEPDOWN && (Chan->Sweep != 0 || Sweep  != 0)) {
	if (!Sweeping && (Chan->Sweep != 0 || Sweep  != 0)) {
		Sweep = 0;
		Chan->Sweep = 0;
		Chan->LastFreq = 0xFFFF;
	}
	else if (Sweeping) {
		Chan->Sweep = Sweep;
		Chan->LastFreq = 0xFFFF;
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
		if (Chan->Freq == 0)
			Chan->Freq = NesFreq;

		Chan->PortaTo = NesFreq;
	}
	else
		Chan->Freq = NesFreq;

	Chan->DutyCycle	= 0;
	Chan->Note		= NewNote;
	Chan->Octave	= Octave;
	Chan->OutVol	= InitVolume;
	Chan->Enabled	= 1;

	pTrackerView->RegisterKeyState(Channel, (Note - 1) + (Octave * 12));

	if (Channel == 4) {

		if (Octave > 5)
			Octave -= 5;

		int SampleIndex = Inst->Samples[Octave][Note - 1];

		if (SampleIndex > 0) {

			if (Octave > 5)
				Octave -= 6;

			stDSample *DSample = pDocument->GetDSample(SampleIndex - 1);

			m_SampleMem.SetMem(DSample->SampleData, DSample->SampleSize);
			Chan->Length = DSample->SampleSize;		// this will be adjusted
			Chan->Freq = Inst->SamplePitch[Octave][Note - 1];
		}
		else
			Chan->Enabled = 0;
	}
}

void CSoundGen::ModifyChannel(int Channel)
{
	// Apply selected sequnces on channel

	stSequence	*Mod;
	int			Delay, Value, Pointer;
	int			i;

	stChanData	*Chan = m_stChannels + Channel;

	if (!Chan->Enabled || Channel == 4)
		return;

	Chan->VibratoPhase = (Chan->VibratoPhase + Chan->VibratoSpeed) & (VIBRATO_LENGTH - 1);
	Chan->TremoloPhase = (Chan->TremoloPhase + Chan->TremoloSpeed) & (TREMOLO_LENGTH - 1);

	if (Chan->PortaSpeed > 0 && Chan->PortaTo > 0) {
		if (Chan->Freq > Chan->PortaTo) {
			Chan->Freq -= Chan->PortaSpeed;
			if (Chan->Freq > 0x1000)	// it was negative
				Chan->Freq = 0x00;
			if (Chan->Freq < Chan->PortaTo)
				Chan->Freq = Chan->PortaTo;
		}
		else if (Chan->Freq < Chan->PortaTo) {
			Chan->Freq += Chan->PortaSpeed;
			if (Chan->Freq > Chan->PortaTo)
				Chan->Freq = Chan->PortaTo;
		}
	}

	if (Chan->Effect == EF_ARPEGGIO) {
		switch (Chan->EffVar) {
			case 0:
				Chan->Freq = m_pNoteLookupTable[Chan->Note];
				pTrackerView->RegisterKeyState(Channel, Chan->Note);
				break;
			case 1:
				Chan->Freq = m_pNoteLookupTable[Chan->Note + (Chan->EffParam >> 4)];
				pTrackerView->RegisterKeyState(Channel, (Chan->Note + (Chan->EffParam >> 4)));
				if ((Chan->EffParam & 0x0F) == 0)
					Chan->EffVar = 2;
				break;
			case 2:
				Chan->Freq = m_pNoteLookupTable[Chan->Note + (Chan->EffParam & 0x0F)];
				pTrackerView->RegisterKeyState(Channel, (Chan->Note + (Chan->EffParam & 0x0F)));
				break;
		}

		Chan->EffVar++;

		if (Chan->EffVar > 2)
			Chan->EffVar = 0;
	}

	for (i = 0; i < MOD_COUNT; i++) {		
		if (Chan->ModEnable[i]) {
			Chan->ModDelay[i]--;
			Mod = pDocument->GetModifier(Chan->ModIndex[i]);

			if (Chan->ModDelay[i] == 0) {
				Pointer = Chan->ModPointer[i];
				Value	= Mod->Value[Pointer];
				Delay	= Mod->Length[Pointer];
				
				// Jump, jump and fetch new values
				if (Delay < 0) {
					Chan->ModPointer[i] += Delay;
					if (Chan->ModPointer[i] < 0)
						Chan->ModPointer[i] = 0;

					// New values
					Pointer = Chan->ModPointer[i];
					Value	= Mod->Value[Pointer];
					Delay	= Mod->Length[Pointer];
				}

				// Only apply values if it wasn't a jump
				switch (i) {
					case MOD_VOLUME:	// Volume modifier
						Chan->OutVol = Value;
						break;
					case MOD_ARPEGGIO:	// Arpeggiator
						if (Channel == 3)
							Chan->Freq = Chan->Note + Value;
						else
							Chan->Freq = m_pNoteLookupTable[Chan->Note + Value];
						pTrackerView->RegisterKeyState(Channel, (Chan->Note + Value));
						break;
					case MOD_HIPITCH:	// Hi-pitch
						Chan->Freq += Value << 4;
						if (Chan->Freq > 0x7FF)
							Chan->Freq = 0x7FF;
						if (Chan->Freq < 0)
							Chan->Freq = 0;
						break;
					case MOD_PITCH:		// Pitch
						Chan->Freq += Value;
						if (Chan->Freq > 0x7FF)
							Chan->Freq = 0x7FF;
						if (Chan->Freq < 0)
							Chan->Freq = 0;
						break;
					case MOD_DUTYCYCLE:	// Duty cycling
						Chan->DutyCycle = Value;
						break;
				}

				Chan->ModDelay[i] = Delay + 1;
				Chan->ModPointer[i]++;
			}

			if (Chan->ModPointer[i] == Mod->Count)
				Chan->ModEnable[i] = 0;
		}
	}
}

void CSoundGen::RefreshChannel(int Channel)
{
	unsigned char LoFreq, HiFreq;
	unsigned char LastLoFreq, LastHiFreq;

	char DutyCycle;

	int Volume, Freq, VibFreq, TremVol;

	stChanData	*Chan = m_stChannels + Channel;

	if (!Chan->Enabled)
		return;

	VibFreq	= m_cVibTable[Chan->VibratoPhase] >> (0x8 - (Chan->VibratoDepth >> 1));
	
	if ((Chan->VibratoDepth & 1) == 0)
		VibFreq -= (VibFreq >> 1);

	TremVol	= (m_cVibTable[Chan->TremoloPhase] >> 4) >> (4 - (Chan->TremoloDepth >> 1));

	if ((Chan->TremoloDepth & 1) == 0)
		TremVol -= (TremVol >> 1);

	Freq = Chan->Freq - VibFreq;

	HiFreq		= (Freq & 0xFF);
	LoFreq		= (Freq >> 8);
	LastHiFreq	= (Chan->LastFreq & 0xFF);
	LastLoFreq	= (Chan->LastFreq >> 8);

	DutyCycle	= (Chan->DutyCycle & 0x03);
	Volume		= (Chan->OutVol - (0x0F - Chan->Volume)) - TremVol;

	if (Volume < 0)
		Volume = 0;
	if (Volume > 15)
		Volume = 15;

	Chan->LastFreq = Freq;

	switch (Channel) {
		case 0:	// Square 1
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
				if (LoFreq != LastLoFreq)
					m_pAPU->Write(0x4003, LoFreq);
			}
			break;
		case 1:	// Square 2
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
				if (LoFreq != LastLoFreq) 
					m_pAPU->Write(0x4007, LoFreq);
			}
			break;
		case 2: // Triangle
			if (Chan->OutVol)
				m_pAPU->Write(0x4008, 0xC0);
			else
				m_pAPU->Write(0x4008, 0x00);
			m_pAPU->Write(0x4009, 0x00);
			m_pAPU->Write(0x400A, HiFreq);
			m_pAPU->Write(0x400B, LoFreq);
			break;
		case 3: // Noise
			m_pAPU->Write(0x400C, 0x30 | Volume);
			m_pAPU->Write(0x400D, 0x00);
			m_pAPU->Write(0x400E, ((HiFreq & 0x0F) ^ 0x0F) | ((DutyCycle & 0x01) << 7));
			m_pAPU->Write(0x400F, 0x00);
			break;
		case 4: // DPCM, this is special
			m_pAPU->Write(0x4010, (HiFreq & 0x0F) | (Chan->Octave > 5 ? 0x40 : 0x00));	// freq & loop 
			//Player->Write(0x4011, 0x00);	// counter load (altering this causes clicks)
			m_pAPU->Write(0x4012, 0x00);	// load address, start at $C000
			m_pAPU->Write(0x4013, Chan->Length / 16);	// length
			m_pAPU->Write(0x4015, 0x0F);
			m_pAPU->Write(0x4015, 0x1F);	// fire sample
			Chan->Enabled = 0;	// don't write to this channel anymore
			break;
	}
}

void CSoundGen::KillChannel(int Chan)
{
	m_stChannels[Chan].Freq		= 0;
	m_stChannels[Chan].PortaTo	= 0;
	m_stChannels[Chan].LastFreq	= 0xFFFF;
	m_stChannels[Chan].OutVol	= 0x00;
	m_stChannels[Chan].Enabled	= 0;
//	m_stChannels[Chan].Volume	= 0x0F;
//	m_stChannels[Chan].Effect	= 0;
//	m_stChannels[Chan].EffVar	= 0;

	switch (Chan) {
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
