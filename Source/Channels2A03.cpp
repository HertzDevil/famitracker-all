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

// This file handles playing of 2A03 channels

#include <cmath>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "ChannelHandler.h"
#include "Channels2A03.h"
#include "Settings.h"

void CChannelHandler2A03::PlayChannelNote(stChanNote *NoteData, int EffColumns)
{
	CInstrument2A03 *Inst = NULL;
	unsigned int Note, Octave;
	unsigned char Sweep = 0;
	unsigned int Instrument, Volume, LastInstrument;
	bool Sweeping = false;

	int	InitVolume = 0x0F;

	Note		= NoteData->Note;
	Octave		= NoteData->Octave;
	Volume		= NoteData->Vol;
	Instrument	= NoteData->Instrument;

	LastInstrument = m_iInstrument;

	if (Note == HALT) {
		Instrument	= MAX_INSTRUMENTS;
		Volume		= 0x10;
	}

	if (Note == RELEASE)
		m_bRelease = true;
	else if (Note != NONE)
		m_bRelease = false;

	if (Note != 0) {
		m_iNoteCut = 0;
	}

	int PostEffect = 0, PostEffectParam = 0;

	// Evaluate effects
	for (int n = 0; n < EffColumns; n++) {
		unsigned char EffNum   = NoteData->EffNumber[n];
		unsigned char EffParam = NoteData->EffParam[n];

		#define GET_SLIDE_SPEED(x) (((x & 0xF0) >> 3) + 1)

		if (!CheckCommonEffects(EffNum, EffParam)) {
			// Custom effects
			switch (EffNum) {
				case EF_VOLUME:
					// Kill this maybe?
					InitVolume = EffParam;
					if (Note == 0)
						m_iOutVol = InitVolume;
					break;
				case EF_SWEEPUP:
					Sweep = 0x88 | EffParam;
					m_iLastFrequency = 0xFFFF;
					Sweeping = true;
					break;
				case EF_SWEEPDOWN:
					Sweep = 0x80 | EffParam;
					m_iLastFrequency = 0xFFFF;
					Sweeping = true;
					break;
				case EF_DUTY_CYCLE:
					m_iDefaultDuty = m_iDutyPeriod = EffParam;
					break;
				case EF_SLIDE_UP:
				case EF_SLIDE_DOWN:
					PostEffect = EffNum;
					PostEffectParam = EffParam;
					SetupSlide(EffNum, EffParam);
					break;
			}
		}
	}

	// Change instrument
 	if (Instrument != LastInstrument /*|| (m_iLastInstrument == MAX_INSTRUMENTS && Instrument != MAX_INSTRUMENTS)*/) {
		if (Instrument == MAX_INSTRUMENTS)
			Instrument = LastInstrument;
		else
			LastInstrument = Instrument;

		if ((Inst = (CInstrument2A03*)m_pDocument->GetInstrument(Instrument)) == NULL)
			return;

		if (Inst->GetType() != INST_2A03)
			return;

		for (int i = 0; i < SEQ_COUNT; i++) {
			if (m_iSeqIndex[i] != Inst->GetSeqIndex(i)) {
				m_iSeqEnabled[i] = Inst->GetSeqEnable(i);
				m_iSeqIndex[i]	 = Inst->GetSeqIndex(i);
				m_iSeqPointer[i] = 0;
			}
		}

		m_iInstrument = Instrument;
	}
	else {
		if (Instrument == MAX_INSTRUMENTS)
			Instrument = m_iLastInstrument;
		else
			m_iLastInstrument = Instrument;

		if (Instrument < MAX_INSTRUMENTS) {
			if ((Inst = (CInstrument2A03*)m_pDocument->GetInstrument(Instrument)) == NULL)
				return;
			if (Inst->GetType() != INST_2A03)
				return;
		}
	}

	if (Volume < 0x10)
		m_iVolume = Volume << VOL_SHIFT;

	if (Note == NONE) {
		if (Sweeping)
			m_cSweep = Sweep;
		// No note specified, stop here
		return;
	}
	
	if (Note == HALT) {
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

	if (!m_bRelease) {

		if (Inst == NULL)
			return;

		// Trigger instrument
		for (int i = 0; i < SEQ_COUNT; i++) {
			m_iSeqEnabled[i] = Inst->GetSeqEnable(i);
			m_iSeqIndex[i]	 = Inst->GetSeqIndex(i);
			m_iSeqPointer[i] = 0;
		}

		m_iNote			= RunNote(Octave, Note);
		m_iDutyPeriod	= m_iDefaultDuty;
		m_iOutVol		= InitVolume;
		m_bEnabled		= true;
	}
	else {
		ReleaseNote();

		for (int i = 0; i < SEQ_COUNT; ++i) {
			if (m_iSeqEnabled[i] == 1) {
				CSequence *pSeq = m_pDocument->GetSequence(m_iSeqIndex[i], i);
				int ReleasePoint = pSeq->GetReleasePoint();
				if (ReleasePoint != -1) {
					m_iSeqPointer[i] = ReleasePoint;
				}
			}
		}
	}

	if (m_iEffect == EF_SLIDE_DOWN || m_iEffect == EF_SLIDE_UP)
		m_iEffect = EF_NONE;

	if (PostEffect)
		SetupSlide(PostEffect, PostEffectParam);
}

void CChannelHandler2A03::ProcessChannel()
{
	// Default effects
	CChannelHandler::ProcessChannel();
	
	// Skip when DPCM
	if (m_iChannelID == 4)
		return;

	if (!m_bEnabled)
		return;

	// Sequences
	for (int i = 0; i < SEQ_COUNT; i++)
		CChannelHandler::RunSequence(i, m_pDocument->GetSequence(m_iSeqIndex[i], i));
}

void CChannelHandler2A03::ResetChannel()
{
	CChannelHandler::ResetChannel();
}

int CChannelHandler2A03::LimitFreq(int Freq)
{
	if (Freq > 0x7FF)
		Freq = 0x7FF;
	if (Freq < 0)
		Freq = 0;
	return Freq;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Square 1 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CSquare1Chan::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	unsigned char LastLoFreq;

	char DutyCycle;
	int Volume, Freq, TremVol;
	int VibFreq;

	VibFreq = GetVibrato();
	TremVol = GetTremolo();

	Freq = m_iFrequency - VibFreq + GetFinePitch() + GetPitch();

//	if (Freq > 0x7FF)
//		Freq = 0x7FF;

	Freq = LimitFreq(Freq);

	HiFreq		= (Freq & 0xFF);
	LoFreq		= (Freq >> 8);
	LastLoFreq	= (m_iLastFrequency >> 8);
	DutyCycle	= (m_iDutyPeriod & 0x03);
	Volume		= CalculateVolume(15);

	m_iLastFrequency = Freq;

	m_pAPU->Write(0x4000, (DutyCycle << 6) | 0x30 | Volume);

	if (m_cSweep) {
		if (m_cSweep & 0x80) {
			m_pAPU->Write(0x4001, m_cSweep);
			m_cSweep &= 0x7F;
			m_pAPU->Write(0x4017, 0x80);
			m_pAPU->Write(0x4017, 0x00);
			m_pAPU->Write(0x4002, HiFreq);
			m_pAPU->Write(0x4003, LoFreq);
			// Immediately trigger one sweep cycle (by request)
			m_iLastFrequency = 0xFFFF;
		}
	}
	else {
		m_pAPU->Write(0x4001, 0x08);
		m_pAPU->Write(0x4017, 0x80);	// Manually execute one APU frame sequence to kill the sweep unit
		m_pAPU->Write(0x4017, 0x00);
		m_pAPU->Write(0x4002, HiFreq);
		
		if (LoFreq != LastLoFreq)
			m_pAPU->Write(0x4003, LoFreq);
	}
}

void CSquare1Chan::ClearRegisters()
{
	m_pAPU->Write(0x4000, 0x30);
	m_pAPU->Write(0x4001, 0x08);
	m_pAPU->Write(0x4002, 0x00);
	m_pAPU->Write(0x4003, 0x00);	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Square 2 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CSquare2Chan::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	unsigned char LastLoFreq;

	char DutyCycle;
	int Volume, Freq, VibFreq, TremVol;

	if (!m_bEnabled)
		return;

	VibFreq = GetVibrato();
	TremVol = GetTremolo();

	Freq = m_iFrequency - VibFreq + GetFinePitch() + GetPitch();

	Freq = LimitFreq(Freq);

	HiFreq		= (Freq & 0xFF);
	LoFreq		= (Freq >> 8);
	LastLoFreq	= (m_iLastFrequency >> 8);

	DutyCycle	= (m_iDutyPeriod & 0x03);
	Volume		= (m_iOutVol * (m_iVolume >> VOL_SHIFT)) / 15 - TremVol;

	if (Volume < 0)
		Volume = 0;
	if (Volume > 15)
		Volume = 15;

	if (m_iOutVol > 0 && m_iVolume > 0 && Volume == 0)
		Volume = 1;

	m_iLastFrequency = Freq;

	m_pAPU->Write(0x4004, (DutyCycle << 6) | 0x30 | Volume);

	if (m_cSweep) {
		if (m_cSweep & 0x80) {
			m_pAPU->Write(0x4005, m_cSweep);
			m_pAPU->Write(0x4006, HiFreq);
			m_pAPU->Write(0x4007, LoFreq);
			m_cSweep &= 0x7F;
			// Immediately trigger one sweep cycle (by request)
//			m_pAPU->Write(0x4017, 0x80);
//			m_pAPU->Write(0x4017, 0x00);
			m_iLastFrequency = 0xFFFF;
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
	m_pAPU->Write(0x4004, 0x30);
	m_pAPU->Write(0x4005, 0x08);
	m_pAPU->Write(0x4006, 0x00);
	m_pAPU->Write(0x4007, 0x00);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Triangle 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CTriangleChan::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	int Freq, VibFreq;

	if (!m_bEnabled)
		return;

	VibFreq = GetVibrato();

	Freq	= m_iFrequency - VibFreq + GetFinePitch() + GetPitch();

	Freq = LimitFreq(Freq);

	HiFreq	= (Freq & 0xFF);
	LoFreq	= (Freq >> 8);
	
	if (m_iOutVol) {
		m_pAPU->Write(0x4008, 0x81);
		m_pAPU->Write(0x400A, HiFreq);
		m_pAPU->Write(0x400B, LoFreq);
	}
	else
		m_pAPU->Write(0x4008, 0);
}

void CTriangleChan::ClearRegisters()
{
	m_pAPU->Write(0x4008, 0);
//	m_pAPU->Write(0x4009, 0);		// these had to be disabled as the triangle generator is now better (more accurate)
//	m_pAPU->Write(0x400A, 0);
//	m_pAPU->Write(0x400B, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Noise
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CNoiseChan::RefreshChannel()
{
	char NoiseMode;
	int Volume, Freq, VibFreq, TremVol;

	if (!m_bEnabled)
		return;

	VibFreq = GetVibrato();
	TremVol = GetTremolo();

	Freq = m_iFrequency - VibFreq + GetFinePitch() + GetPitch();

	NoiseMode = (m_iDutyPeriod & 0x01) << 7;
	Volume = (m_iOutVol * (m_iVolume >> VOL_SHIFT)) / 15 - TremVol;

	if (Volume < 0)
		Volume = 0;
	if (Volume > 15)
		Volume = 15;

	if (m_iOutVol > 0 && m_iVolume > 0 && Volume == 0)
		Volume = 1;

	m_iLastFrequency = Freq;

	m_pAPU->Write(0x400C, 0x30 | Volume);
	m_pAPU->Write(0x400D, 0x00);
	m_pAPU->Write(0x400E, ((Freq & 0x0F) ^ 0x0F) | NoiseMode);
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

	if (m_iRetrigger != 0) {
		m_iRetriggerCntr--;
		if (m_iRetriggerCntr == 0) {
			m_iRetriggerCntr = m_iRetrigger;
			m_bEnabled = true;
		}
	}

	if (!m_bEnabled)
		return;

	HiFreq	= (m_iFrequency & 0xFF);

	m_pAPU->Write(0x4010, 0x00 | (HiFreq & 0x0F) | m_iLoop);
	m_pAPU->Write(0x4012, m_iOffset);							// load address, start at $C000
	m_pAPU->Write(0x4013, m_iSampleLength);						// length
	m_pAPU->Write(0x4015, 0x0F);
	m_pAPU->Write(0x4015, 0x1F);								// fire sample

	// Loop offset
	if (m_iLoopOffset > 0) {
		m_pAPU->Write(0x4012, m_iLoopOffset);
		m_pAPU->Write(0x4013, m_iLoopLength);
	}

	m_bEnabled = false;		// don't write to this channel anymore
}

void CDPCMChan::ClearRegisters()
{
	m_pAPU->Write(0x4015, 0x0F);
	m_pAPU->Write(0x4010, 0);
	
	if (/*!m_bKeyRelease ||*/ !theApp.GetSettings()->General.bNoDPCMReset || theApp.IsPlaying()) {
		m_pAPU->Write(0x4011, 0);		// regain full volume for TN
	}

	m_pAPU->Write(0x4012, 0);
	m_pAPU->Write(0x4013, 0);

	m_iOffset = 0;
	m_cDAC = 255;
}

void CDPCMChan::PlayChannelNote(stChanNote *NoteData, int EffColumns)
{
	unsigned int Note, Octave, SampleIndex, LastInstrument;
	int EffPitch = -1;
	CInstrument2A03 *Inst;

	Note	= NoteData->Note;
	Octave	= NoteData->Octave;

	m_iRetrigger = 0;

	for (int i = 0; i < EffColumns; i++) {
		switch (NoteData->EffNumber[i]) {
		case EF_DAC:
			m_cDAC = NoteData->EffParam[i] & 0x7F;
			break;
		case EF_SAMPLE_OFFSET:
			m_iOffset = NoteData->EffParam[i];
			break;
		case EF_DPCM_PITCH:
			EffPitch = NoteData->EffParam[i];
			break;
		case EF_RETRIGGER:
//			if (NoteData->EffParam[i] > 0) {
				m_iRetrigger = NoteData->EffParam[i] + 1;
				if (m_iRetriggerCntr == 0)
					m_iRetriggerCntr = m_iRetrigger;
//			}
//			m_iEnableRetrigger = 1;
			break;
		}
	}

	if (Note == 0)
		return;
/*
	if (Note == RELEASE)
		m_bKeyRelease = true;
	else
		m_bKeyRelease = false;
*/
	if (Note == HALT || Note == RELEASE) {
		KillChannel();
		return;
	}

	LastInstrument = m_iInstrument;

	if (NoteData->Instrument != 0x40)
		m_iInstrument = NoteData->Instrument;

	if ((Inst = (CInstrument2A03*)m_pDocument->GetInstrument(m_iInstrument)) == NULL)
		return;

	if (Inst->GetType() != INST_2A03)
		return;

	// Change instrument
	if (NoteData->Instrument != m_iLastInstrument) {
		if (NoteData->Instrument == MAX_INSTRUMENTS)
			NoteData->Instrument = LastInstrument;
		else
			LastInstrument = NoteData->Instrument;

		m_iInstrument = NoteData->Instrument;
	}
	else {
		if (NoteData->Instrument == MAX_INSTRUMENTS)
			NoteData->Instrument = m_iLastInstrument;
		else
			m_iLastInstrument = NoteData->Instrument;
	}

	SampleIndex = Inst->GetSample(Octave, Note - 1);

	if (SampleIndex > 0) {
		int Pitch = Inst->GetSamplePitch(Octave, Note - 1);
		if (Pitch & 0x80)
			m_iLoop = 0x40;
		else
			m_iLoop = 0;

		if (EffPitch != -1)
			Pitch = EffPitch;

		m_iLoopOffset = Inst->GetSampleLoopOffset(Octave, Note - 1);

		CDSample *DSample = m_pDocument->GetDSample(SampleIndex - 1);

		int SampleSize = DSample->SampleSize;

		if (SampleSize > 0) {
			m_pSampleMem->SetMem(DSample->SampleData, SampleSize);
			Length = SampleSize;		// this will be adjusted
			m_iFrequency = Pitch & 0x0F;
			m_iSampleLength = (SampleSize >> 4) - (m_iOffset << 2);
			m_iLoopLength = SampleSize - m_iLoopOffset;
			m_bEnabled = true;

			m_iRetriggerCntr = m_iRetrigger;
		}
	}

	theApp.RegisterKeyState(m_iChannelID, (Note - 1) + (Octave * 12));
}
