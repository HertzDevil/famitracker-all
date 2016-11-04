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

// This file handles playing of VRC7 channels

#include "stdafx.h"
#include "FamiTracker.h"
#include "SoundGen.h"
#include "ChannelHandler.h"

void CChannelHandlerVRC7::PlayNote(stChanNote *pNoteData, int EffColumns)
{
	CInstrumentVRC7 *pInst;
	int MidiNote;

	if (!CChannelHandler::CheckNote(pNoteData, INST_VRC7))
		return;

	pInst = (CInstrumentVRC7*)m_pDocument->GetInstrument(m_iInstrument);

	if (pNoteData->Note == HALT || pNoteData->Note == RELEASE) {
		m_iVolume = 0x10;
		KillChannel();
		return;
	}

	// No note
	if (!pNoteData->Note)
		return;

//	if (pNoteData->Vol < 0x10)
//		m_iVolume = pNoteData->Vol;

	MidiNote = MIDI_NOTE(pNoteData->Octave, pNoteData->Note);

//	m_iFrequency	= TriggerNote(MidiNote);
	m_iNote			= MidiNote;
//	m_iOutVol		= 0xF;
	m_bEnabled		= true;
	m_iPatch		= pInst->GetPatch();
}

void CChannelHandlerVRC7::ProcessChannel()
{
	if (!m_bEnabled)
		return;

	// Default effects
	CChannelHandler::ProcessChannel();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// VRC7 Channels
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CVRC7Channel::RefreshChannel()
{
//	unsigned char LoFreq, HiFreq;
	int /*Volume,*/ Freq/*, VibFreq, TremVol*/;

	if (!m_bEnabled)
		return;

	m_bEnabled = false;

	const int FREQ_TABLE[] = {172, 181, 192, 204, 216, 229, 242, 257, 272, 288, 305, 323};

#define OPL_NOTE_ON 0x10

	Freq = m_iNote << 3;

	int Fnum;
	int Bnum;
	int Patch;
	int Sustain;

//	Fnum = (m_iNote % 12 + 1) << 3;
	Fnum = FREQ_TABLE[m_iNote % 12];
	Bnum = (m_iNote / 12);
	Patch = m_iPatch;
	Sustain = 7;

	RegWrite(0x20 + m_iChannel, 0);

	RegWrite(0x10 + m_iChannel, Fnum & 0xFF);
	RegWrite(0x20 + m_iChannel, (Sustain << 5) | ((Fnum >> 8) & 1) | (Bnum << 1) | OPL_NOTE_ON);
	RegWrite(0x30 + m_iChannel, (Patch << 4) | 0x0);

//	RegWrite(0x0E, 0x20);
}

void CVRC7Channel::ClearRegisters()
{
	for (int i = 0x10; i < 0x30; i += 0x10)
		RegWrite(i + m_iChannel, 0);
}

void CVRC7Channel::RegWrite(unsigned char Reg, unsigned char Value)
{
	m_pAPU->ExternalWrite(0x9010, Reg);
	m_pAPU->ExternalWrite(0x9030, Value);
}
