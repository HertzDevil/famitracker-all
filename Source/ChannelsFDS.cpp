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

// Famicom disk sound

#include "stdafx.h"
#include <cmath>
#include "FamiTrackerDoc.h"
#include "SoundGen.h"
#include "ChannelHandler.h"
#include "ChannelsFDS.h"

void CChannelHandlerFDS::PlayChannelNote(stChanNote *pNoteData, int EffColumns)
{
	int Note, Octave, Volume;
	CInstrumentFDS *pInst;
	int PostEffect = 0, PostEffectParam;

	Note	= pNoteData->Note;
	Octave	= pNoteData->Octave;
	Volume	= pNoteData->Vol;

	if (!CChannelHandler::CheckNote(pNoteData, INST_FDS))
		return;

	if (!(pInst = (CInstrumentFDS*)m_pDocument->GetInstrument(m_iInstrument)))
		return;

	if (pNoteData->Note == RELEASE)
		m_bRelease = true;
	else if (pNoteData->Note != NONE)
		m_bRelease = false;

	// Read volume
	if (Volume < 0x10) {
		m_iVolume = Volume << VOL_SHIFT;
	}

	// Evaluate effects
	for (int n = 0; n < EffColumns; n++) {
		unsigned char EffNum   = pNoteData->EffNumber[n];
		unsigned char EffParam = pNoteData->EffParam[n];

		if (EffNum == EF_PORTA_DOWN) {
			m_iPortaSpeed = EffParam;
			m_iEffect = EF_PORTA_UP;
		}
		else if (EffNum == EF_PORTA_UP) {
			m_iPortaSpeed = EffParam;
			m_iEffect = EF_PORTA_DOWN;
		}
		else if (!CheckCommonEffects(EffNum, EffParam)) {
			// Custom effects
			switch (EffNum) {
				case EF_SLIDE_UP:
				case EF_SLIDE_DOWN:
					PostEffect = EffNum;
					PostEffectParam = EffParam;
					SetupSlide(EffNum, EffParam);
					break;
			}
		}
	}

	// No new note data, nothing more to do
	if (pNoteData->Note == NONE)
		return;

	// Halt or release
	if (Note == HALT) {
		KillChannel();		// todo: use CutNote
		m_iNote = 0x80;
		return;
	}

	// Load the instrument, only when a new instrument is loaded?
	if (!m_bRelease) {
		// Todo: check this in nsf
		FillWaveRAM(pInst);
		FillModulationTable(pInst);
	}

	if (!m_bRelease) {
		// Trigger a new note
		m_iNote	= RunNote(Octave, Note);
		m_bEnabled = true;
		m_iLastInstrument = m_iInstrument;

		m_iOutVol = 0x1F;

		m_pVolumeSeq = pInst->GetVolumeSeq();
		m_pArpeggioSeq = pInst->GetArpSeq();
		m_pPitchSeq = pInst->GetPitchSeq();

		m_iSeqEnabled[SEQ_VOLUME] = m_pVolumeSeq->GetItemCount() > 0 ? 1 : 0;
		m_iSeqPointer[SEQ_VOLUME] = 0;

		m_iSeqEnabled[SEQ_ARPEGGIO] = m_pArpeggioSeq->GetItemCount() > 0 ? 1 : 0;
		m_iSeqPointer[SEQ_ARPEGGIO] = 0;

		m_iSeqEnabled[SEQ_PITCH] = m_pPitchSeq->GetItemCount() > 0 ? 1 : 0;
		m_iSeqPointer[SEQ_PITCH] = 0;

		m_iModulationFreq = pInst->GetModulationFreq();
		m_iModulationDepth = pInst->GetModulationDepth();
		m_iModulationDelay = pInst->GetModulationDelay();
	}
	else {
		ReleaseNote();
	}

	if (PostEffect)
		SetupSlide(PostEffect, PostEffectParam);
}

void CChannelHandlerFDS::ProcessChannel()
{
	// Default effects
	CChannelHandler::ProcessChannel();	

	// Sequences
	if (m_iSeqEnabled[SEQ_VOLUME])
		CChannelHandler::RunSequence(SEQ_VOLUME, m_pVolumeSeq);

	if (m_iSeqEnabled[SEQ_ARPEGGIO])
		CChannelHandler::RunSequence(SEQ_ARPEGGIO, m_pArpeggioSeq);

	if (m_iSeqEnabled[SEQ_PITCH])
		CChannelHandler::RunSequence(SEQ_PITCH, m_pPitchSeq);
}

int CChannelHandlerFDS::LimitFreq(int Freq)
{
	if (Freq > 4095)
		Freq = 4095;
	if (Freq < 0)
		Freq = 0;
	return Freq;
}

void CChannelHandlerFDS::RefreshChannel()
{
	unsigned char LoFreq, HiFreq;
	unsigned char Volume;
	unsigned char ModFreqLo, ModFreqHi;

	/*****/
	if (m_iInstrument != MAX_INSTRUMENTS) {
		CInstrumentFDS *pInst = (CInstrumentFDS*)m_pDocument->GetInstrument(m_iInstrument);
		if (pInst && pInst->GetType() == INST_FDS) {
			if (pInst->HasChanged()) {
				// Realtime update
				m_iModulationFreq = pInst->GetModulationFreq();
				m_iModulationDepth = pInst->GetModulationDepth();
				FillWaveRAM(pInst);
				FillModulationTable(pInst);
			}
		}
	}
	/*****/

	int Frequency = m_iFrequency - GetVibrato() + GetFinePitch() + GetPitch();

	Frequency = LimitFreq(Frequency);

	LoFreq = Frequency & 0xFF;
	HiFreq = (Frequency >> 8) & 0x0F;

	ModFreqLo = m_iModulationFreq & 0xFF;
	ModFreqHi = (m_iModulationFreq >> 8) & 0x0F;

	// Calculate volume
//	Volume = ((m_iVolume >> 2) * m_iOutVol) / 15 - GetTremolo();
	//Volume = (((m_iOutVol * (m_iVolume >> VOL_SHIFT)) / 15) << 1) - GetTremolo();
	Volume = m_iVolume - GetTremolo();
	if (Volume < 0)
		Volume = 0;
	Volume >>= VOL_SHIFT;
	Volume = (m_iOutVol * Volume) / 15;
	//Volume <<= 1;

	if (m_iNote == 0x80)
		Volume = 0;

	// Write frequency
	m_pAPU->ExternalWrite(0x4082, LoFreq);
	m_pAPU->ExternalWrite(0x4083, HiFreq);

	// Write volume, disable envelope
	m_pAPU->ExternalWrite(0x4080, 0x80 | Volume);

	// Update modulation unit
	if (m_iModulationDelay == 0) {
		// Modulation frequency
		m_pAPU->ExternalWrite(0x4086, ModFreqLo);
		m_pAPU->ExternalWrite(0x4087, ModFreqHi);

		// Sweep depth, disable sweep envelope
		m_pAPU->ExternalWrite(0x4084, 0x80 | m_iModulationDepth); 
	}
	else {
		// Delayed modulation
		m_pAPU->ExternalWrite(0x4087, 0x80);
		m_iModulationDelay--;
	}
}

void CChannelHandlerFDS::ClearRegisters()
{	
	// Clear gain
	m_pAPU->ExternalWrite(0x4090, 0x00);

	// Clear volume
	m_pAPU->ExternalWrite(0x4080, 0x00);

	// Silence channel
	m_pAPU->ExternalWrite(0x4083, 0x80);

	// Default speed
	m_pAPU->ExternalWrite(0x408A, 0xFF);

	// Disable modulation
	m_pAPU->ExternalWrite(0x4087, 0x80);

//	m_iVolume = MAX_VOL;
	m_iOutVol = 0x1F;

	m_iNote = 0x80;

	m_iLastInstrument = MAX_INSTRUMENTS;
}

void CChannelHandlerFDS::FillWaveRAM(CInstrumentFDS *pInst)
{
	// Fills the 64 byte waveform table
	// Enable write for waveform RAM
	m_pAPU->ExternalWrite(0x4089, 0x80);

	// Wave ram
	for (int i = 0; i < 0x40; i++)
		m_pAPU->ExternalWrite(0x4040 + i, pInst->GetSample(i));

	// Disable write for waveform RAM, master volume = full
	m_pAPU->ExternalWrite(0x4089, 0x00);
}

void CChannelHandlerFDS::FillModulationTable(CInstrumentFDS *pInst)
{
	// Fills the 32 byte modulation table
	// Disable modulation
	m_pAPU->ExternalWrite(0x4087, 0x80);
	// Reset modulation table pointer, set bias to zero
	m_pAPU->ExternalWrite(0x4085, 0x00);
	// Fill the table
	for (int i = 0; i < 32; i++)
		m_pAPU->ExternalWrite(0x4088, pInst->GetModulation(i));
}
