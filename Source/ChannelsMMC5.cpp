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

// MMC5 file

#include <cmath>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "ChannelHandler.h"
#include "ChannelsMMC5.h"

void CChannelHandlerMMC5::PlayChannelNote(stChanNote *NoteData, int EffColumns)
{
	CInstrument2A03 *Inst;
	unsigned int Note, Octave;
	unsigned char Sweep = 0;
	unsigned int Instrument, Volume, LastInstrument;

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

	int PostEffect = 0, PostEffectParam = 0;

	// Evaluate effects
	for (int n = 0; n < EffColumns; n++) {
		unsigned char EffNum   = NoteData->EffNumber[n];
		unsigned char EffParam = NoteData->EffParam[n];

		#define GET_SLIDE_SPEED(x) (((x & 0xF0) >> 3) + 1)

		if (!CheckCommonEffects(EffNum, EffParam)) {
			switch (EffNum) {
				case EF_VOLUME:
					InitVolume = EffParam;
					if (Note == 0)
						m_iOutVol = InitVolume;
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
	if (Instrument != LastInstrument) {
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

		if ((Inst = (CInstrument2A03*)m_pDocument->GetInstrument(Instrument)) == NULL)
			return;
		if (Inst->GetType() != INST_2A03)
			return;
	}

	if (Volume < 0x10)
		m_iVolume = Volume << VOL_SHIFT;

	if (Note == 0) {
		return;
	}
	
	if (Note == HALT) {
		KillChannel();
		return;
	}

	if (!m_bRelease) {

		// Trigger instrument
		for (int i = 0; i < SEQ_COUNT; i++) {
			m_iSeqEnabled[i]	= Inst->GetSeqEnable(i);
			m_iSeqIndex[i]		= Inst->GetSeqIndex(i);
			m_iSeqPointer[i]	= 0;
		}

		m_iNote			= RunNote(Octave, Note);
		m_iDutyPeriod	= m_iDefaultDuty;
		m_iOutVol		= InitVolume;
		m_bEnabled		= true;
	}
	else {
		ReleaseNote();
	}

	if (m_iEffect == EF_SLIDE_DOWN || m_iEffect == EF_SLIDE_UP)
		m_iEffect = EF_NONE;

	if (PostEffect)
		SetupSlide(PostEffect, PostEffectParam);
}

void CChannelHandlerMMC5::ProcessChannel()
{
	// Default effects
	CChannelHandler::ProcessChannel();
	
	if (!m_bEnabled)
		return;

	// Sequences
	for (int i = 0; i < SEQ_COUNT; i++)
		RunSequence(i, m_pDocument->GetSequence(m_iSeqIndex[i], i));
}

void CChannelHandlerMMC5::ResetChannel()
{
	CChannelHandler::ResetChannel();
}

int CChannelHandlerMMC5::LimitFreq(int Freq)
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

void CMMC5Square1Chan::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	unsigned char LastLoFreq;

	char DutyCycle;
	int Volume, Freq, TremVol;
	int VibFreq;

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

	m_pAPU->ExternalWrite(0x5015, 0x03);

	m_pAPU->ExternalWrite(0x5000, (DutyCycle << 6) | 0x30 | Volume);
	m_pAPU->ExternalWrite(0x5002, HiFreq);

	if (LoFreq != LastLoFreq)
		m_pAPU->ExternalWrite(0x5003, LoFreq);
}

void CMMC5Square1Chan::ClearRegisters()
{
	m_pAPU->ExternalWrite(0x5000, 0);
	m_pAPU->ExternalWrite(0x5002, 0);
	m_pAPU->ExternalWrite(0x5003, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Square 2 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CMMC5Square2Chan::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	unsigned char LastLoFreq;

	char DutyCycle;
	int Volume, Freq, TremVol;
	int VibFreq;

	VibFreq = GetVibrato();
	TremVol = GetTremolo();

	Freq = m_iFrequency - VibFreq + GetFinePitch() + GetPitch();

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

	m_pAPU->ExternalWrite(0x5015, 0x03);

	m_pAPU->ExternalWrite(0x5004, (DutyCycle << 6) | 0x30 | Volume);
	m_pAPU->ExternalWrite(0x5006, HiFreq);

	if (LoFreq != LastLoFreq)
		m_pAPU->ExternalWrite(0x5007, LoFreq);
}

void CMMC5Square2Chan::ClearRegisters()
{
	m_pAPU->ExternalWrite(0x5004, 0);
	m_pAPU->ExternalWrite(0x5006, 0);
	m_pAPU->ExternalWrite(0x5007, 0);
}
