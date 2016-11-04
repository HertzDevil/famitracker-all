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

// This takes care of the 2A03 channels

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "SoundGen.h"
#include "ChannelHandler.h"

void CChannelHandler::InitChannel(CAPU *pAPU, unsigned char *pVibTable, CFamiTrackerDoc *pDoc)
{
	m_pAPU = pAPU;
	m_pcVibTable = pVibTable;
	m_pDocument = pDoc;

	m_DelayNote = NULL;
	m_bDelayEnabled = false;

	KillChannel();
}

void CChannelHandler::ProcessChannel()
{
	int	i;

	if (m_bDelayEnabled) {
		if (!m_cDelayCounter) {
			m_bDelayEnabled = false;
			PlayNote(m_DelayNote, m_iDelayEffColumns);
			delete m_DelayNote;
		}
		else
			m_cDelayCounter--;
	}

	if (!m_bEnabled)
		return;

	// Vibrato and tremolo
	m_iVibratoPhase = (m_iVibratoPhase + m_iVibratoSpeed) & (VIBRATO_LENGTH - 1);
	m_iTremoloPhase = (m_iTremoloPhase + m_iTremoloSpeed) & (TREMOLO_LENGTH - 1);

	// Automatic portamento
	if (m_iPortaSpeed > 0 && m_iPortaTo > 0) {
		if (m_iFrequency > m_iPortaTo) {
			m_iFrequency -= m_iPortaSpeed;
			if (m_iFrequency > 0x1000)	// it was negative
				m_iFrequency = 0x00;
			if (m_iFrequency < m_iPortaTo)
				m_iFrequency = m_iPortaTo;
		}
		else if (m_iFrequency < m_iPortaTo) {
			m_iFrequency += m_iPortaSpeed;
			if (m_iFrequency > m_iPortaTo)
				m_iFrequency = m_iPortaTo;
		}
	}

	// Arpeggio
	if (m_cArpeggio != 0) {
		switch (m_cArpVar) {
			case 0:
				m_iFrequency = TriggerNote(m_iNote);
				break;
			case 1:
				m_iFrequency = TriggerNote(m_iNote + (m_cArpeggio >> 4));
				if ((m_cArpeggio & 0x0F) == 0)
					m_cArpVar = 2;
				break;
			case 2:
				m_iFrequency = TriggerNote(m_iNote + (m_cArpeggio & 0x0F));
				break;
		}

		if (++m_cArpVar > 2)
			m_cArpVar = 0;
	}
	
	// Sequences
	for (i = 0; i < MOD_COUNT; i++) {
		RunSequence(i, m_pDocument->GetSequence(ModIndex[i], i));
	}
}

void CChannelHandler::RunSequence(int Index, stSequence *pSequence)
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
					m_iOutVol = Value;
					break;
				// Arpeggiator
				case MOD_ARPEGGIO:
					m_iFrequency = TriggerNote(m_iNote + Value);
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
					m_cDutyCycle = Value;
					break;
			}

			ModDelay[Index] = Delay + 1;
			ModPointer[Index]++;
		}

		if (ModPointer[Index] == pSequence->Count)
			ModEnable[Index] = 0;
	}
}

void CChannelHandler::LimitFreq()
{
	if (m_iFrequency > 0x7FF)
		m_iFrequency = 0x7FF;

	if (m_iFrequency < 0)
		m_iFrequency = 0;
}

void CChannelHandler::SetNoteTable(unsigned int *NoteLookupTable)
{
	m_pNoteLookupTable = NoteLookupTable;
}

unsigned int CChannelHandler::TriggerNote(int Note)
{
	theApp.RegisterKeyState(m_iChannelID, Note);

	return m_pNoteLookupTable[Note];
}

void CChannelHandler::Arpeggiate(unsigned int Note)
{
	m_iFrequency = TriggerNote(Note);
}

void CChannelHandler::PlayNote(stChanNote *NoteData, int EffColumns)
{
	stInstrument *Inst;

	unsigned int NesFreq;
	unsigned int Note, Octave;
	unsigned char Sweep = 0;
	int PortUp = 0, PortDown = 0;
	int Instrument, Volume, LastInstrument;
	bool Sweeping = false;

	int	InitVolume = 0x0F;

	if (m_bDelayEnabled) {
		m_bDelayEnabled = false;
		PlayNote(m_DelayNote, m_iDelayEffColumns);
		delete m_DelayNote;
	}

	/*
	if (m_bDelayEnabled && (NoteData != m_DelayNote)) {
		m_bDelayEnabled = false;
		delete m_DelayNote;
	}
	*/

	// Check delay
	for (int n = 0; n < EffColumns; n++) {
		if (NoteData->EffNumber[n] == EF_DELAY) {
			m_bDelayEnabled = true;
			m_cDelayCounter = NoteData->EffParam[n];
			m_DelayNote = new stChanNote;
			m_iDelayEffColumns = EffColumns;
			memcpy(m_DelayNote, NoteData, sizeof(stChanNote));
			m_DelayNote->EffNumber[n] = EF_NONE;
			m_DelayNote->EffParam[n] = 0;
			return;
		}
	}	

	if (!NoteData)
		return;

	Note		= NoteData->Note;
	Octave		= NoteData->Octave;
	Volume		= NoteData->Vol;
	Instrument	= NoteData->Instrument;

	LastInstrument = m_iInstrument;

	if (Note == HALT || Note == RELEASE) {
		Instrument	= MAX_INSTRUMENTS;
		Volume		= 0x10;
		Octave		= 0;
	}

	// Evaluate effects
	for (int n = 0; n < EffColumns; n++) {
		int EffNum		= NoteData->EffNumber[n];
		int EffParam	= NoteData->EffParam[n];

		switch (EffNum) {
			case EF_VOLUME:
				InitVolume = EffParam;
				if (Note == 0)
					m_iOutVol = InitVolume;
				break;
			case EF_PORTAON:
				m_iPortaSpeed = EffParam + 1;
				break;
			case EF_PORTAOFF:
				m_iPortaSpeed = 0;
				m_iPortaTo = 0;
				break;
			case EF_SWEEPUP: {
				int Shift = EffParam & 0x07;
				int Period = ((EffParam >> 4) & 0x07) << 4;
				Sweep = 0x88 | Shift | Period;
				m_iLastFrequency = 0xFFFF;
//				if (Note == 0)
//					Sweep = Sweep;
				Sweeping = true;
				break;
				}
			case EF_SWEEPDOWN: {
				int Shift = EffParam & 0x07;
				int Period = ((EffParam >> 4) & 0x07) << 4;
				Sweep = 0x80 | Shift | Period;
				m_iLastFrequency = 0xFFFF;
//				if (Note == 0)
//					Sweep = Sweep;
				Sweeping = true;
				break;
				}
			case EF_VIBRATO:
				m_iVibratoDepth = (EffParam & 0x0F) + 2;
				m_iVibratoSpeed = EffParam >> 4;
				if (!EffParam)
					m_iVibratoPhase = 0;
				break;
			case EF_TREMOLO:
				m_iTremoloDepth = (EffParam & 0x0F) + 2;
				m_iTremoloSpeed = EffParam >> 4;
				if (!EffParam)
					m_iTremoloPhase = 0;
				break;
			case EF_ARPEGGIO:
				m_cArpeggio = EffParam;
				break;
			case EF_PITCH:
				m_iFinePitch = EffParam;
				break;
		}
	}

	// Change instrument
	if (Instrument != LastInstrument) {

		if (Instrument == MAX_INSTRUMENTS)
			Instrument = LastInstrument;
		else
			LastInstrument = Instrument;

		Inst = m_pDocument->GetInstrument(Instrument);
		
		if (Inst == NULL)
			return;

		for (int i = 0; i < MOD_COUNT; i++) {
			if (ModIndex[i] != Inst->ModIndex[i]) {
				ModEnable[i]	= Inst->ModEnable[i];
				ModIndex[i]		= Inst->ModIndex[i];
				ModPointer[i]	= 0;
				ModDelay[i]		= 1;
			}
		}

		m_iInstrument = Instrument;
	}
	else {
		if (Instrument == MAX_INSTRUMENTS)
			Instrument = m_iLastInstrument;
		else
			m_iLastInstrument = Instrument;

		Inst = m_pDocument->GetInstrument(Instrument);

		if (Inst == NULL)
			return;
	}

	if (Volume < 0x10)
		m_iVolume = Volume;

	if (Note == 0) {
		m_iLastNote = 0;
		if (Sweeping)
			m_cSweep = Sweep;
		return;
	}
	
	if (Note == HALT || Note == RELEASE) {
		KillChannel();
		return;
	}

	if (!Sweeping && (m_cSweep != 0 || Sweep != 0)) {
		Sweep = 0;
		m_cSweep = 0;
		m_iLastFrequency = 0xFFFF;
	}
	else if (Sweeping) {
		m_cSweep = Sweep;
		m_iLastFrequency = 0xFFFF;
	}

	// Trigger instrument
	for (int i = 0; i < MOD_COUNT; i++) {
		ModEnable[i]	= Inst->ModEnable[i];
		ModIndex[i]		= Inst->ModIndex[i];
		ModPointer[i]	= 0;
		ModDelay[i]		= 1;
	}

	// And note
	int NewNote = (Note - 1) + (Octave * 12);

	NesFreq = TriggerNote(NewNote);

	if (m_iPortaSpeed > 0) {
		if (m_iFrequency == 0)
			m_iFrequency = NesFreq;
		m_iPortaTo = NesFreq;
	}
	else
		m_iFrequency = NesFreq;

	m_cDutyCycle	= 0;
	m_iNote			= NewNote;
	m_iOctave		= Octave;
	m_iOutVol		= InitVolume;
	m_bEnabled		= true;
}

void CChannelHandler::MakeSilent()
{
	m_iVolume		= 0x0F;
	m_iPortaSpeed	= 0;
	m_cDutyCycle	= 0;
	m_cArpeggio		= 0;
	m_cArpVar		= 0;
	m_iVibratoSpeed = m_iVibratoPhase = 0;
	m_iTremoloSpeed = m_iTremoloPhase = 0;
	m_iFinePitch	= 0x80;
	m_iFrequency	= 0;
	m_iLastFrequency = 0xFFFF;
	m_bDelayEnabled	= false;

	KillChannel();
}

void CChannelHandler::KillChannel()
{
	m_bEnabled			= false;
	//m_iFrequency		= 0;
	m_iLastFrequency	= 0xFFFF;
	m_iOutVol			= 0x00;
	m_iPortaTo			= 0;

	theApp.RegisterKeyState(m_iChannelID, -1);

	ClearRegisters();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Square 1 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CSquare1Chan::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	unsigned char LastLoFreq, LastHiFreq;

	char DutyCycle;
	int Volume, Freq, VibFreq, TremVol;

	if (!m_bEnabled)
		return;

	VibFreq	= m_pcVibTable[m_iVibratoPhase] >> (0x8 - (m_iVibratoDepth >> 1));

	if ((m_iVibratoDepth & 1) == 0)
		VibFreq -= (VibFreq >> 1);

	TremVol	= (m_pcVibTable[m_iTremoloPhase] >> 4) >> (4 - (m_iTremoloDepth >> 1));

	if ((m_iTremoloDepth & 1) == 0)
		TremVol -= (TremVol >> 1);

	Freq = m_iFrequency - VibFreq + (0x80 - m_iFinePitch);

	HiFreq		= (Freq & 0xFF);
	LoFreq		= (Freq >> 8);
	LastHiFreq	= (m_iLastFrequency & 0xFF);
	LastLoFreq	= (m_iLastFrequency >> 8);

	DutyCycle	= (m_cDutyCycle & 0x03);
	Volume		= (m_iOutVol - (0x0F - m_iVolume)) - TremVol;

	if (Volume < 0)
		Volume = 0;
	if (Volume > 15)
		Volume = 15;

	m_iLastFrequency = Freq;

	m_pAPU->Write(0x4000, (DutyCycle << 6) | 0x30 | Volume);

	if (m_cSweep) {
		if (m_cSweep & 0x80) {
			m_pAPU->Write(0x4001, m_cSweep);
			m_pAPU->Write(0x4002, HiFreq);
			m_pAPU->Write(0x4003, LoFreq);
			m_cSweep &= 0x7F;
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
}

void CSquare1Chan::ClearRegisters()
{
	m_pAPU->Write(0x4000, 0);
	m_pAPU->Write(0x4001, 0);
	m_pAPU->Write(0x4002, 0);
	m_pAPU->Write(0x4003, 0);	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Square 2 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CSquare2Chan::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	unsigned char LastLoFreq, LastHiFreq;

	char DutyCycle;
	int Volume, Freq, VibFreq, TremVol;

	if (!m_bEnabled)
		return;

	VibFreq	= m_pcVibTable[m_iVibratoPhase] >> (0x8 - (m_iVibratoDepth >> 1));

	if ((m_iVibratoDepth & 1) == 0)
		VibFreq -= (VibFreq >> 1);

	TremVol	= (m_pcVibTable[m_iTremoloPhase] >> 4) >> (4 - (m_iTremoloDepth >> 1));

	if ((m_iTremoloDepth & 1) == 0)
		TremVol -= (TremVol >> 1);

	Freq = m_iFrequency - VibFreq + (0x80 - m_iFinePitch);

	HiFreq		= (Freq & 0xFF);
	LoFreq		= (Freq >> 8);
	LastHiFreq	= (m_iLastFrequency & 0xFF);
	LastLoFreq	= (m_iLastFrequency >> 8);

	DutyCycle	= (m_cDutyCycle & 0x03);
	Volume		= (m_iOutVol - (0x0F - m_iVolume)) - TremVol;

	if (Volume < 0)
		Volume = 0;
	if (Volume > 15)
		Volume = 15;

	m_iLastFrequency = Freq;

	m_pAPU->Write(0x4004, (DutyCycle << 6) | 0x30 | Volume);

	if (m_cSweep) {
		if (m_cSweep & 0x80) {
			m_pAPU->Write(0x4005, m_cSweep);
			m_pAPU->Write(0x4006, HiFreq);
			m_pAPU->Write(0x4007, LoFreq);
			m_cSweep &= 0x7F;
			// Immediately trigger one sweep cycle (by request)
			m_pAPU->Write(0x4017, 0x80);
			m_pAPU->Write(0x4017, 0x00);
		}
	}
	else {
		m_pAPU->Write(0x4005, 0x08);
		// This one was tricky. Manually execute one APU frame sequence to kill the sweep unit
		m_pAPU->Write(0x4017, 0x80);
		m_pAPU->Write(0x4017, 0x00);
		m_pAPU->Write(0x4006, HiFreq);
		
		if (LoFreq != LastLoFreq)
			m_pAPU->Write(0x4007, LoFreq);
	}
}

void CSquare2Chan::ClearRegisters()
{
	m_pAPU->Write(0x4004, 0);
	m_pAPU->Write(0x4005, 0);
	m_pAPU->Write(0x4006, 0);
	m_pAPU->Write(0x4007, 0);	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Triangle 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CTriangleChan::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	unsigned char LastLoFreq, LastHiFreq;

	char DutyCycle;
	int Volume, Freq, VibFreq, TremVol;

	if (!m_bEnabled)
		return;

	VibFreq	= m_pcVibTable[m_iVibratoPhase] >> (0x8 - (m_iVibratoDepth >> 1));

	if ((m_iVibratoDepth & 1) == 0)
		VibFreq -= (VibFreq >> 1);

	TremVol	= (m_pcVibTable[m_iTremoloPhase] >> 4) >> (4 - (m_iTremoloDepth >> 1));

	if ((m_iTremoloDepth & 1) == 0)
		TremVol -= (TremVol >> 1);

	Freq = m_iFrequency - VibFreq + (0x80 - m_iFinePitch);

	HiFreq		= (Freq & 0xFF);
	LoFreq		= (Freq >> 8);
	LastHiFreq	= (m_iLastFrequency & 0xFF);
	LastLoFreq	= (m_iLastFrequency >> 8);

	DutyCycle	= (DutyCycle & 0x03);
	Volume		= (m_iOutVol - (0x0F - m_iVolume)) - TremVol;

	if (Volume < 0)
		Volume = 0;
	if (Volume > 15)
		Volume = 15;

	m_iLastFrequency = Freq;

	if (m_iOutVol)
		m_pAPU->Write(0x4008, 0xC0);
	else
		m_pAPU->Write(0x4008, 0x00);

	m_pAPU->Write(0x4009, 0x00);
	m_pAPU->Write(0x400A, HiFreq);
	m_pAPU->Write(0x400B, LoFreq);
}

void CTriangleChan::ClearRegisters()
{
	m_pAPU->Write(0x4008, 0);
	m_pAPU->Write(0x4009, 0);
	m_pAPU->Write(0x400A, 0);
	m_pAPU->Write(0x400B, 0);	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Noise
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CNoiseChan::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	unsigned char LastLoFreq, LastHiFreq;

	char NoiseMode;
	int Volume, Freq, VibFreq, TremVol;

	if (!m_bEnabled)
		return;

	VibFreq	= m_pcVibTable[m_iVibratoPhase] >> (0x8 - (m_iVibratoDepth >> 1));

	if ((m_iVibratoDepth & 1) == 0)
		VibFreq -= (VibFreq >> 1);

	TremVol	= (m_pcVibTable[m_iTremoloPhase] >> 4) >> (4 - (m_iTremoloDepth >> 1));

	if ((m_iTremoloDepth & 1) == 0)
		TremVol -= (TremVol >> 1);

	Freq = m_iFrequency - VibFreq + (0x80 - m_iFinePitch);

	HiFreq		= (Freq & 0xFF);
	LoFreq		= (Freq >> 8);
	LastHiFreq	= (m_iLastFrequency & 0xFF);
	LastLoFreq	= (m_iLastFrequency >> 8);

	NoiseMode	= (m_cDutyCycle & 0x01) << 7;
	Volume		= (m_iOutVol - (0x0F - m_iVolume)) - TremVol;

	if (Volume < 0)
		Volume = 0;
	if (Volume > 15)
		Volume = 15;

	m_iLastFrequency = Freq;

	m_pAPU->Write(0x400C, 0x30 | Volume);
	m_pAPU->Write(0x400D, 0x00);
	m_pAPU->Write(0x400E, ((HiFreq & 0x0F) ^ 0x0F) | NoiseMode);
	m_pAPU->Write(0x400F, 0x00);
}

void CNoiseChan::ClearRegisters()
{
	m_pAPU->Write(0x400C, 0x30);
	m_pAPU->Write(0x400D, 0);
	m_pAPU->Write(0x400E, 0);
	m_pAPU->Write(0x400F, 0);	
}

unsigned int CNoiseChan::TriggerNote(int Note)
{
	theApp.RegisterKeyState(m_iChannelID, Note);
	return Note;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// DPCM
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CDPCMChan::RefreshChannel()
{
	unsigned char HiFreq;

	if (m_cDAC != 255) {
		m_pAPU->Write(0x4011, m_cDAC);
		m_cDAC = 255;
	}

	if (!m_bEnabled)
		return;

	HiFreq	= (m_iFrequency & 0xFF);

	m_pAPU->Write(0x4010, 0x00 | (HiFreq & 0x0F) | m_iOctave);
	m_pAPU->Write(0x4012, 0x00);			// load address, start at $C000
	m_pAPU->Write(0x4013, Length / 16);		// length
	m_pAPU->Write(0x4015, 0x0F);
	m_pAPU->Write(0x4015, 0x1F);			// fire sample

	m_bEnabled = false;		// don't write to this channel anymore
}

void CDPCMChan::ClearRegisters()
{
	m_pAPU->Write(0x4015, 0x0F);
	m_pAPU->Write(0x4010, 0);
	
	if (m_bKeyRelease && !theApp.m_pSettings->General.bNoDPCMReset)
		m_pAPU->Write(0x4011, 0);		// regain full volume for TN

	m_pAPU->Write(0x4012, 0);
	m_pAPU->Write(0x4013, 0);
}

void CDPCMChan::PlayNote(stChanNote *NoteData, int EffColumns)
{
	unsigned int Note, Octave, SampleIndex, LastInstrument;
	stInstrument *Inst;

	Note	= NoteData->Note;
	Octave	= NoteData->Octave;

	for (int i = 0; i < EffColumns; i++) {
		if (NoteData->EffNumber[i] == EF_DAC) {
			m_cDAC = NoteData->EffParam[i] & 0x7F;
		}
	}

	if (Note == 0)
		return;

	if (Note == RELEASE) {
		m_bKeyRelease = true;
		KillChannel();
		return;
	}
	else {
		m_bKeyRelease = false;
	}

	if (Note == HALT) {
		KillChannel();
		return;
	}

	LastInstrument = m_iInstrument;

	// Change instrument
	if (NoteData->Instrument != m_iLastInstrument) {

		if (NoteData->Instrument == MAX_INSTRUMENTS)
			NoteData->Instrument = LastInstrument;
		else
			LastInstrument = NoteData->Instrument;

		Inst = m_pDocument->GetInstrument(NoteData->Instrument);
		
		if (Inst == NULL)
			return;

		m_iInstrument = NoteData->Instrument;
	}
	else {
		if (NoteData->Instrument == MAX_INSTRUMENTS)
			NoteData->Instrument = m_iLastInstrument;
		else
			m_iLastInstrument = NoteData->Instrument;

		Inst = m_pDocument->GetInstrument(NoteData->Instrument);

		if (Inst == NULL)
			return;
	}

	SampleIndex = Inst->Samples[Octave][Note - 1];

	if (SampleIndex > 0 /*&& m_pDocument->GetSampleSize(SampleIndex) > 0*/) {
		int Pitch = Inst->SamplePitch[Octave][Note - 1];
		if (Pitch & 0x80)
			m_iOctave = 0x40;
		else
			m_iOctave = 0;

		stDSample *DSample = m_pDocument->GetDSample(SampleIndex - 1);
		m_pSampleMem->SetMem(DSample->SampleData, DSample->SampleSize);
		Length = DSample->SampleSize;		// this will be adjusted
		m_iFrequency = Pitch & 0x0F;
		m_bEnabled = true;
	}

	theApp.RegisterKeyState(m_iChannelID, (Note - 1) + (Octave * 12));
}

void CDPCMChan::SetSampleMem(CSampleMem *pSampleMem)
{
	m_pSampleMem = pSampleMem;
}
