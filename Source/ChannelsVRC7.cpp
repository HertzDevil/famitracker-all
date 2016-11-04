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

// This file handles playing of VRC7 channels

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "SoundGen.h"
#include "ChannelHandler.h"
#include "ChannelsVRC7.h"

enum {
	CMD_NONE, 
	CMD_NOTE_ON,
	CMD_NOTE_TRIGGER,
	CMD_NOTE_OFF, 
	CMD_NOTE_HALT,
	CMD_NOTE_RELEASE};

CChannelHandlerVRC7::CChannelHandlerVRC7(int ChanID) : 
	CChannelHandler(ChanID), 
	m_iCommand(0),
	m_iTriggeredNote(0)
{
	m_iVolume = MAX_VOL;
}

void CChannelHandlerVRC7::PlayChannelNote(stChanNote *pNoteData, int EffColumns)
{
	CInstrumentVRC7 *pInstrument;
	//int MidiNote;
	int PostEffect = 0, PostEffectParam;

	if (pNoteData->Instrument != MAX_INSTRUMENTS)
		m_iInstrument = pNoteData->Instrument;

	// Get the instrument
	pInstrument = (CInstrumentVRC7*)m_pDocument->GetInstrument(m_iInstrument);

	// Evaluate effects
	for (int n = 0; n < EffColumns; n++) {
		int EffCmd	 = pNoteData->EffNumber[n];
		int EffParam = pNoteData->EffParam[n];

		if (!CheckCommonEffects(EffCmd, EffParam)) {
			switch (EffCmd) {
				case EF_SLIDE_UP:
				case EF_SLIDE_DOWN:
					PostEffect = EffCmd;
					PostEffectParam = EffParam;
//					SetupSlide(EffCmd, EffParam);
					break;
			}
		}
	}

	// Volume
	if (pNoteData->Vol < 0x10) {
		m_iVolume = pNoteData->Vol << VOL_SHIFT;
	}

	if (pNoteData->Note == HALT) {
		// Halt
		m_iCommand = CMD_NOTE_HALT;
		theApp.RegisterKeyState(m_iChannelID, -1);
		m_iNote = HALT;
	}
	else if (pNoteData->Note == RELEASE) {
		// Release
		m_iCommand = CMD_NOTE_RELEASE;
		theApp.RegisterKeyState(m_iChannelID, -1);
		m_iNote = RELEASE;
	}
	else if (pNoteData->Note != NONE) {

		// Check instrument
		if (!pInstrument || pInstrument->GetType() != INST_VRC7)
			return;

		int OldNote = m_iNote;
		int OldOctave = m_iOctave;

		// Trigger note
		m_iNote		= CChannelHandler::RunNote(pNoteData->Octave, pNoteData->Note);
		m_iPatch	= pInstrument->GetPatch();
		m_bEnabled	= true;
		m_bHold		= true;

		if (m_iEffect != EF_PORTAMENTO || OldNote == 0)
			m_iCommand = CMD_NOTE_TRIGGER;

		if (m_iPortaSpeed > 0 && m_iEffect == EF_PORTAMENTO && OldNote > 0) {

			// Set current frequency to the one with highest octave
			if (m_iOctave > OldOctave) {
				m_iFrequency >>= (m_iOctave - OldOctave);
			}
			else if (OldOctave > m_iOctave) {
				// Do nothing
				m_iPortaTo >>= (OldOctave - m_iOctave);
				m_iOctave = OldOctave;
			}
		}
		
		// Load custom parameters
		if (m_iPatch == 0) {
			for (int i = 0; i < 8; ++i)
				m_iRegs[i] = pInstrument->GetCustomReg(i);
		}
	}
	
	if (pNoteData->Note != NONE && (m_iEffect == EF_SLIDE_DOWN || m_iEffect == EF_SLIDE_UP))
		m_iEffect = EF_NONE;

	if (PostEffect) {
//		SetupSlide(PostEffect, PostEffectParam);

		#define GET_SLIDE_SPEED(x) (((x & 0xF0) >> 3) + 1)

		m_iPortaSpeed = GET_SLIDE_SPEED(PostEffectParam);
		m_iEffect = PostEffect;

		if (PostEffect == EF_SLIDE_UP)
			m_iNote = m_iNote + (PostEffectParam & 0xF);
		else
			m_iNote = m_iNote - (PostEffectParam & 0xF);

		int OldOctave = m_iOctave;
		m_iPortaTo = TriggerNote(m_iNote);

		if (m_iOctave > OldOctave) {
			m_iFrequency >>= (m_iOctave - OldOctave);
		}
		else if (m_iOctave < OldOctave) {
			m_iPortaTo >>= (OldOctave - m_iOctave);
			m_iOctave = OldOctave;
		}
	}
}

void CChannelHandlerVRC7::ProcessChannel()
{
	// Default effects
	CChannelHandler::ProcessChannel();
}

void CChannelHandlerVRC7::ResetChannel()
{
	CChannelHandler::ResetChannel();
}

unsigned int CChannelHandlerVRC7::TriggerNote(int Note)
{
	m_iTriggeredNote = Note;
	theApp.RegisterKeyState(m_iChannelID, Note);
	if (m_iNote != HALT && m_iCommand != CMD_NOTE_TRIGGER)
		m_iCommand = CMD_NOTE_ON;
	m_bEnabled = true;
	m_iOctave = Note / 12;
	return GetFnum(Note);
}

unsigned int CChannelHandlerVRC7::GetFnum(int Note)
{
	const int FREQ_TABLE[] = {172, 181, 192, 204, 216, 229, 242, 257, 272, 288, 305, 323};
	return FREQ_TABLE[Note % 12] << 2;
}

int CChannelHandlerVRC7::LimitFreq(int Freq)
{
	if (Freq > 2047)
		Freq = 2047;
	
	if (Freq < 0)
		Freq = 0;

	return Freq;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// VRC7 Channels
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CVRC7Channel::RefreshChannel()
{

#define OPL_NOTE_ON 0x10
#define OPL_SUSTAIN_ON 0x20

	int Note;
	int Fnum;		// F-number
	int Bnum;		// Block
	int Patch;
	int Volume;
	
	Note = m_iTriggeredNote;
	Patch = m_iPatch;
	Volume = 15 - ((m_iVolume >> VOL_SHIFT) - GetTremolo());
	Bnum = m_iOctave;
	Fnum = (m_iFrequency >> 2) - GetVibrato() + (m_iFinePitch - 0x80);

	// Write custom instrument
	if (Patch == 0 && m_iCommand == CMD_NOTE_TRIGGER) {
		for (int i = 0; i < 8; ++i)
			RegWrite(i, m_iRegs[i]);
	}

	int Cmd = 0;

	switch (m_iCommand) {
		case CMD_NOTE_TRIGGER:
			RegWrite(0x20 + m_iChannel, 0);
			m_iCommand = CMD_NOTE_ON;
			Cmd = OPL_NOTE_ON;
			break;
		case CMD_NOTE_ON:
			Cmd = m_bHold ? OPL_NOTE_ON : OPL_SUSTAIN_ON;
			break;
		case CMD_NOTE_HALT:
			Cmd = 0;
			break;
		case CMD_NOTE_RELEASE:
			Cmd = OPL_SUSTAIN_ON;
			break;
	}
	
	// Write frequency
	RegWrite(0x10 + m_iChannel, Fnum & 0xFF);
	RegWrite(0x20 + m_iChannel, ((Fnum >> 8) & 1) | (Bnum << 1) | Cmd);
	
	if (m_iCommand != CMD_NOTE_HALT) {
		// Select volume & patch
		RegWrite(0x30 + m_iChannel, (Patch << 4) | Volume);
	}
}

void CVRC7Channel::ClearRegisters()
{
	for (int i = 0x10; i < 0x30; i += 0x10)
		RegWrite(i + m_iChannel, 0);

	m_iNote = 0;
	m_iEffect = EF_NONE;

	m_iCommand = CMD_NOTE_HALT;
}

void CVRC7Channel::RegWrite(unsigned char Reg, unsigned char Value)
{
	m_pAPU->ExternalWrite(0x9010, Reg);
	m_pAPU->ExternalWrite(0x9030, Value);
}
