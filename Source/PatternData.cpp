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

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "PatternData.h"

CPatternData::CPatternData()
{
	memset(m_iFrameList, 0, sizeof(short) * MAX_FRAMES * MAX_CHANNELS);
}

CPatternData::~CPatternData()
{
	// Deallocate memory
	for (int i = 0; i < MAX_CHANNELS; i++) {
		for (int j = 0; j < MAX_PATTERN; j++) {
			if (m_pPatternData[i][j])
				delete [] m_pPatternData[i][j];
		}
	}
}

void CPatternData::Init(unsigned int PatternLength, unsigned int FrameCount, unsigned int Speed, unsigned int Tempo)
{
	m_iPatternLength = PatternLength;
	m_iFrameCount	 = FrameCount;
	m_iSongSpeed	 = Speed;
	m_iSongTempo	 = Tempo;

	for (int i = 0; i < MAX_CHANNELS; i++) {
		for (int j = 0; j < MAX_PATTERN; j++) {
			m_pPatternData[i][j] = NULL;
		}
		AllocatePattern(i, 0);
		m_iEffectColumns[i] = 0;
	}
}

void CPatternData::SetEffect(unsigned int Channel, unsigned int Pattern, unsigned int Row, unsigned int Column, char EffNumber, char EffParam)
{
	GetPatternData(Channel, Pattern, Row)->EffNumber[Column] = EffNumber;
	GetPatternData(Channel, Pattern, Row)->EffParam[Column] = EffParam;
}

void CPatternData::SetInstrument(unsigned int Channel, unsigned int Pattern, unsigned int Row, char Instrument)
{
	GetPatternData(Channel, Pattern, Row)->Instrument = Instrument;
}

void CPatternData::SetNote(unsigned int Channel, unsigned int Pattern, unsigned int Row, char Note)
{
	GetPatternData(Channel, Pattern, Row)->Note = Note;
}

void CPatternData::SetOctave(unsigned int Channel, unsigned int Pattern, unsigned int Row, char Octave)
{
	GetPatternData(Channel, Pattern, Row)->Octave = Octave;
}

void CPatternData::SetVolume(unsigned int Channel, unsigned int Pattern, unsigned int Row, char Volume)
{
	GetPatternData(Channel, Pattern, Row)->Vol = Volume;
}

bool CPatternData::IsCellFree(unsigned int Channel, unsigned int Pattern, unsigned int Row)
{
	stChanNote *Note = GetPatternData(Channel, Pattern, Row);
	bool IsFree = Note->Note != 0 || 
		Note->EffNumber[0] != 0 || Note->EffNumber[1] != 0 || 
		Note->EffNumber[2] != 0 || Note->EffNumber[3] != 0 || 
		Note->Vol < 0x10 || Note->Instrument != MAX_INSTRUMENTS;
	return IsFree;
}

bool CPatternData::IsPatternFree(unsigned int Channel, unsigned int Pattern)
{
	for (unsigned int i = 0; i < m_iPatternLength; i++) {
		if (IsCellFree(Channel, Pattern, i))
			return false;
	}
	return true;
}

stChanNote *CPatternData::GetPatternData(int Channel, int Pattern, int Row)
{
	if (!m_pPatternData[Channel][Pattern])		// Allocate pattern if first time access
		AllocatePattern(Channel, Pattern);

	return m_pPatternData[Channel][Pattern] + Row;
}

void CPatternData::AllocatePattern(int Channel, int Pattern)
{
	// Allocate memory
	m_pPatternData[Channel][Pattern] = new stChanNote[MAX_PATTERN_LENGTH];

	// Clear memory
	for (int i = 0; i < MAX_PATTERN_LENGTH; i++) {
		stChanNote *pNote = m_pPatternData[Channel][Pattern] + i;
		pNote->Note		  = 0;
		pNote->Octave	  = 0;
		pNote->Instrument = MAX_INSTRUMENTS;
		pNote->Vol		  = 0x10;
		for (int n = 0; n < MAX_EFFECT_COLUMNS; n++) {
			pNote->EffNumber[n] = 0;
			pNote->EffParam[n] = 0;
		}
	}
}

void CPatternData::ClearEverything()
{
	// Resets everything

	// Frame list
	memset(m_iFrameList, 0, sizeof(short) * MAX_FRAMES * MAX_CHANNELS);
	
	// Patterns, deallocate everything
	for (int i = 0; i < MAX_CHANNELS; i++) {
		for (int j = 0; j < MAX_PATTERN; j++) {
			if (m_pPatternData[i][j]) {
				delete [] m_pPatternData[i][j];
				m_pPatternData[i][j] = NULL;
			}
		}
	}
}
