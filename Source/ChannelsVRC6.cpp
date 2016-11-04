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

// This file handles playing of VRC6 channels

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "ChannelHandler.h"
#include "ChannelsVRC6.h"

void CChannelHandlerVRC6::PlayChannelNote(stChanNote *pNoteData, int EffColumns)
{
	CInstrumentVRC6 *pInst;
	int PostEffect = 0, PostEffectParam;

	int LastInstrument = m_iInstrument;

	if (!CChannelHandler::CheckNote(pNoteData, INST_VRC6))
		return;

	if ((pInst = (CInstrumentVRC6*)m_pDocument->GetInstrument(m_iInstrument)) == NULL)
		return;

	unsigned int Note, Octave;
	unsigned int Volume;

	Note	= pNoteData->Note;
	Octave	= pNoteData->Octave;
	Volume	= pNoteData->Vol;

	if (Note != 0)
		m_bRelease = false;

	if (Note == RELEASE) {
		m_bRelease = true;
	}
	else if (Note == HALT) {
		m_iInstrument	= LastInstrument;
		Volume			= 0x10;
		Octave			= 0;
	}

	// Evaluate effects
	for (int n = 0; n < EffColumns; n++) {
		int EffCmd	 = pNoteData->EffNumber[n];
		int EffParam = pNoteData->EffParam[n];

		if (!CheckCommonEffects(EffCmd, EffParam)) {
			switch (EffCmd) {
				case EF_DUTY_CYCLE:
					m_iDefaultDuty = m_iDutyPeriod = EffParam;
					break;
				case EF_SLIDE_UP:
				case EF_SLIDE_DOWN:
					PostEffect = EffCmd;
					PostEffectParam = EffParam;
					SetupSlide(EffCmd, EffParam);
					break;
			}
		}
	}

	if (LastInstrument != m_iInstrument || Note > 0 && Note != HALT && Note != RELEASE) {
		// Setup instrument
		for (int i = 0; i < SEQ_COUNT; i++) {
			m_iSeqEnabled[i] = pInst->GetSeqEnable(i);
			m_iSeqIndex[i]	 = pInst->GetSeqIndex(i);
			m_iSeqPointer[i] = 0;
		}
	}

	// Get volume
	if (Volume < 0x10)
		m_iVolume = Volume << VOL_SHIFT;

	if (Note == HALT) {
		KillChannel();
		return;
	}

	// No note
	if (!Note)
		return;

	if (!m_bRelease) {

		// Get the note
		m_iNote				= RunNote(Octave, Note);
		m_iOutVol			= 0xF;
		m_iDutyPeriod		= m_iDefaultDuty;
		m_bEnabled			= true;
		m_iLastInstrument	= m_iInstrument;
	}
	else {
		ReleaseNote();
	}

	if (m_iEffect == EF_SLIDE_DOWN || m_iEffect == EF_SLIDE_UP)
		m_iEffect = EF_NONE;

	if (PostEffect)
		SetupSlide(PostEffect, PostEffectParam);
}

void CChannelHandlerVRC6::ProcessChannel()
{
	// Default effects
	CChannelHandler::ProcessChannel();

	if (!m_bEnabled)
		return;

	// Sequences
	for (int i = 0; i < SEQ_COUNT; i++)
		CChannelHandler::RunSequence(i, m_pDocument->GetSequenceVRC6(m_iSeqIndex[i], i));
}

void CChannelHandlerVRC6::ResetChannel()
{
	CChannelHandler::ResetChannel();
}

int CChannelHandlerVRC6::LimitFreq(int Freq)
{
	if (Freq > 0xFFF)
		Freq = 0xFFF;
	if (Freq < 0)
		Freq = 0;
	return Freq;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// VRC6 Square 1
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CVRC6Square1::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	char DutyCycle = m_iDutyPeriod << 4;
	int Volume, Freq, VibFreq, TremVol;

	if (!m_bEnabled)
		return;

	VibFreq = GetVibrato();
	TremVol = GetTremolo();

	Freq = m_iFrequency - VibFreq + GetFinePitch() + GetPitch();

	HiFreq = (Freq & 0xFF);
	LoFreq = (Freq >> 8);

	Volume	= (m_iOutVol * (m_iVolume >> VOL_SHIFT)) / 15 - TremVol;

	if (Volume < 0)
		Volume = 0;
	if (Volume > 15)
		Volume = 15;

	if (m_iOutVol > 0 && m_iVolume > 0 && Volume == 0)
		Volume = 1;

	m_pAPU->ExternalWrite(0x9000, DutyCycle | Volume);
	m_pAPU->ExternalWrite(0x9001, HiFreq);
	m_pAPU->ExternalWrite(0x9002, 0x80 | LoFreq);
}

void CVRC6Square1::ClearRegisters()
{
	m_pAPU->ExternalWrite(0x9000, 0);
	m_pAPU->ExternalWrite(0x9001, 0);
	m_pAPU->ExternalWrite(0x9002, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// VRC6 Square 2
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CVRC6Square2::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	char DutyCycle = m_iDutyPeriod << 4;
	int Volume, Freq, VibFreq, TremVol;

	if (!m_bEnabled)
		return;

	VibFreq = GetVibrato();
	TremVol = GetTremolo();

	Freq = m_iFrequency - VibFreq + GetFinePitch() + GetPitch();

	HiFreq = (Freq & 0xFF);
	LoFreq = (Freq >> 8);

	Volume	= (m_iOutVol * (m_iVolume >> VOL_SHIFT)) / 15 - TremVol;

	if (Volume < 0)
		Volume = 0;
	if (Volume > 15)
		Volume = 15;

	if (m_iOutVol > 0 && m_iVolume > 0 && Volume == 0)
		Volume = 1;

	m_pAPU->ExternalWrite(0xA000, DutyCycle | Volume);
	m_pAPU->ExternalWrite(0xA001, HiFreq);
	m_pAPU->ExternalWrite(0xA002, 0x80 | LoFreq);
}

void CVRC6Square2::ClearRegisters()
{
	m_pAPU->ExternalWrite(0xA000, 0);
	m_pAPU->ExternalWrite(0xA001, 0);
	m_pAPU->ExternalWrite(0xA002, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// VRC6 Sawtooth
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CVRC6Sawtooth::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	int Volume, Freq, VibFreq, TremVol;

	if (!m_bEnabled)
		return;

	VibFreq = GetVibrato();
	TremVol = GetTremolo();

	Freq = m_iFrequency - VibFreq + GetFinePitch() + GetPitch();

	HiFreq = (Freq & 0xFF);
	LoFreq = (Freq >> 8);

	Volume = (m_iOutVol * (m_iVolume >> VOL_SHIFT)) / 15 - TremVol;

	Volume = (Volume << 1) | ((m_iDutyPeriod & 1) << 5);

	if (Volume < 0)
		Volume = 0;
	if (Volume > 63)
		Volume = 63;

	if (m_iOutVol > 0 && m_iVolume > 0 && Volume == 0)
		Volume = 1;

	m_pAPU->ExternalWrite(0xB000, Volume);
	m_pAPU->ExternalWrite(0xB001, HiFreq);
	m_pAPU->ExternalWrite(0xB002, 0x80 | LoFreq);
}

void CVRC6Sawtooth::ClearRegisters()
{
	m_pAPU->ExternalWrite(0xB000, 0);
	m_pAPU->ExternalWrite(0xB001, 0);
	m_pAPU->ExternalWrite(0xB002, 0);
}
