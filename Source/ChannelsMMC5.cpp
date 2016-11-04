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

// MMC5 file

#include <cmath>
#include "stdafx.h"
#include "FamiTracker.h"
#include "SoundGen.h"
#include "ChannelHandler.h"
#include "ChannelsMMC5.h"

void CChannelHandlerMMC5::PlayNote(stChanNote *NoteData, int EffColumns)
{
	CInstrument2A03 *Inst;
	unsigned int Note, Octave;
	unsigned char Sweep = 0;
	unsigned int Instrument, Volume, LastInstrument;

	int	InitVolume = 0x0F;

	if (HandleDelay(NoteData, EffColumns))
		return;

//	if (!CChannelHandler::CheckNote(NoteData, INST_2A03))
//		return;

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
					m_iDefaultDuty = m_cDutyCycle = EffParam;
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

		for (int i = 0; i < MOD_COUNT; i++) {
			if (ModIndex[i] != Inst->GetModIndex(i)) {
				ModEnable[i]	= Inst->GetModEnable(i);
				ModIndex[i]		= Inst->GetModIndex(i);
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

		if ((Inst = (CInstrument2A03*)m_pDocument->GetInstrument(Instrument)) == NULL)
			return;
		if (Inst->GetType() != INST_2A03)
			return;
	}

	if (Volume < 0x10)
		m_iVolume = Volume << VOL_SHIFT;

	if (Note == 0) {
//		if (Sweeping)
//			m_cSweep = Sweep;
		return;
	}
	
	if (Note == HALT || Note == RELEASE) {
		KillChannel();
		return;
	}
/*
	if (!Sweeping && (m_cSweep != 0 || Sweep != 0)) {
		Sweep = 0;
		m_cSweep = 0;
		m_iLastFrequency = 0xFFFF;
	}
	else if (Sweeping) {
		m_cSweep = Sweep;
		*/
	//	m_iLastFrequency = 0xFFFF;
//	}

	// Trigger instrument
	for (int i = 0; i < MOD_COUNT; i++) {
		ModEnable[i]	= Inst->GetModEnable(i);
		ModIndex[i]		= Inst->GetModIndex(i);
		ModPointer[i]	= 0;
		ModDelay[i]		= 1;
	}

	RunNote(Octave, Note);

	m_cDutyCycle	= m_iDefaultDuty;
	m_iNote			= MIDI_NOTE(Octave, Note);
	m_iOutVol		= InitVolume;
	m_bEnabled		= true;

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
	for (int i = 0; i < MOD_COUNT; i++)
		RunSequence(i, m_pDocument->GetSequence(ModIndex[i], i));
}

void CChannelHandlerMMC5::RunSequence(int Index, CSequence *pSequence)
{
	int Value;

	if (ModEnable[Index]) {

		Value = pSequence->GetItem(ModPointer[Index]);

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

		ModPointer[Index]++;

		if (ModPointer[Index] == pSequence->GetItemCount()) {
			if (pSequence->GetLoopPoint() != -1)
				// Loop
				ModPointer[Index] = pSequence->GetLoopPoint();
			else
				// End of sequence
				ModEnable[Index] = 0;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Square 1 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CMMC5Square1Chan::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	unsigned char LastLoFreq/*, LastHiFreq*/;

	char DutyCycle;
	unsigned int Volume, Freq, TremVol;

	int VibFreq;

	VibFreq = GetVibrato();

//	VibFreq	= sinf(float(m_iVibratoPhase) / 10.0f) * float(m_iVibratoDepth);

//	VibFreq	= m_pcVibTable[m_iVibratoPhase] >> (0x8 - (m_iVibratoDepth >> 1));
	//VibFreq = sinf(m_iVibratoPhase / 2) * 10;

//	if ((m_iVibratoDepth & 1) == 0)
//		VibFreq -= (VibFreq >> 1);

	/*
	TremVol	= (m_pcVibTable[m_iTremoloPhase] >> 4) >> (4 - (m_iTremoloDepth >> 1));

	if ((m_iTremoloDepth & 1) == 0)
		TremVol -= (TremVol >> 1);
		*/

	TremVol = GetTremolo();

	Freq = m_iFrequency - VibFreq + (0x80 - m_iFinePitch);

	if (Freq > 0x7FF)
		Freq = 0x7FF;

	HiFreq		= (Freq & 0xFF);
	LoFreq		= (Freq >> 8);
	LastLoFreq	= (m_iLastFrequency >> 8);

	DutyCycle	= (m_cDutyCycle & 0x03);
	Volume		= (m_iOutVol * (m_iVolume >> VOL_SHIFT)) / 15 - TremVol;

	if (Volume < 0)
		Volume = 0;
	if (Volume > 15)
		Volume = 15;

	if (m_iOutVol > 0 && m_iVolume > 0 && Volume == 0)
		Volume = 1;

	m_iLastFrequency = Freq;

//	Volume = 15;
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
	int Volume, Freq, VibFreq, TremVol;

//	if (!m_bEnabled)
//		return;
/*
	VibFreq	= m_pcVibTable[m_iVibratoPhase] >> (0x8 - (m_iVibratoDepth >> 1));

	if ((m_iVibratoDepth & 1) == 0)
		VibFreq -= (VibFreq >> 1);
*/

	VibFreq = GetVibrato();

//	VibFreq	= sinf(float(m_iVibratoPhase) / 10.0f) * float(m_iVibratoDepth);

	/*
	TremVol	= (m_pcVibTable[m_iTremoloPhase] >> 4) >> (4 - (m_iTremoloDepth >> 1));

	if ((m_iTremoloDepth & 1) == 0)
		TremVol -= (TremVol >> 1);
*/

	TremVol = GetTremolo();

	Freq = m_iFrequency - VibFreq + (0x80 - m_iFinePitch);

	HiFreq		= (Freq & 0xFF);
	LoFreq		= (Freq >> 8);
	LastLoFreq	= (m_iLastFrequency >> 8);

	DutyCycle	= (m_cDutyCycle & 0x03);
	Volume		= (m_iOutVol * (m_iVolume >> VOL_SHIFT)) / 15 - TremVol;

	if (Volume < 0)
		Volume = 0;
	if (Volume > 15)
		Volume = 15;

	if (m_iOutVol > 0 && m_iVolume > 0 && Volume == 0)
		Volume = 1;

	m_iLastFrequency = Freq;

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
