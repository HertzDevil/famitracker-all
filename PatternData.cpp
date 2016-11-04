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

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "PatternData.h"

void CPatternData::Init(unsigned int PatternLength, unsigned int FrameCount, unsigned int Speed)
{
	m_iPatternLength		= PatternLength;
	m_iFrameCount			= FrameCount;
	m_iSongSpeed			= Speed;
}

void CPatternData::ClearPattern(unsigned int Channels)
{
	unsigned int x, y, z;

	// Clear frame list
	for (x = 0; x < Channels; x++) {
		for (y = 0; y < MAX_FRAMES; y++) {
			m_iFrameList[x][y] = 0;
		}
	}

	// Clear pattern
	for (x = 0; x < Channels; x++) {
		for (y = 0; y < MAX_PATTERN; y++) {
			for (z = 0; z < MAX_PATTERN_LENGTH; z++) {
				ClearNote(x, y, z);
			}
		}
		m_iEffectColumns[x] = 0;
	}
}

void CPatternData::ClearNote(unsigned int Channel, unsigned int Pattern, unsigned int Row)
{
	m_stPatternData[Channel][Pattern][Row].Note			= 0;
	m_stPatternData[Channel][Pattern][Row].Octave		= 0;
	m_stPatternData[Channel][Pattern][Row].Instrument	= MAX_INSTRUMENTS;
	m_stPatternData[Channel][Pattern][Row].Vol			= 0x10;

	for (unsigned int n = 0; n < MAX_EFFECT_COLUMNS; n++) {
		m_stPatternData[Channel][Pattern][Row].EffNumber[n] = 0;
		m_stPatternData[Channel][Pattern][Row].EffParam[n] = 0;
	}
}

void CPatternData::SetEffect(unsigned int Channel, unsigned int Pattern, unsigned int Row, unsigned int Column, char EffNumber, char EffParam)
{
	m_stPatternData[Channel][Pattern][Row].EffNumber[Column] = EffNumber;
	m_stPatternData[Channel][Pattern][Row].EffParam[Column] = EffParam;
}

void CPatternData::SetInstrument(unsigned int Channel, unsigned int Pattern, unsigned int Row, char Instrument)
{
	m_stPatternData[Channel][Pattern][Row].Instrument = Instrument;
}

void CPatternData::SetNote(unsigned int Channel, unsigned int Pattern, unsigned int Row, char Note)
{
	m_stPatternData[Channel][Pattern][Row].Note = Note;
}

void CPatternData::SetOctave(unsigned int Channel, unsigned int Pattern, unsigned int Row, char Octave)
{
	m_stPatternData[Channel][Pattern][Row].Octave = Octave;
}

void CPatternData::SetVolume(unsigned int Channel, unsigned int Pattern, unsigned int Row, char Volume)
{
	m_stPatternData[Channel][Pattern][Row].Vol = Volume;
}

bool CPatternData::IsCellFree(unsigned int Channel, unsigned int Pattern, unsigned int Row)
{
	return (m_stPatternData[Channel][Pattern][Row].Note != 0 || 
		m_stPatternData[Channel][Pattern][Row].EffNumber[0] != 0 || m_stPatternData[Channel][Pattern][Row].EffNumber[1] != 0 || 
		m_stPatternData[Channel][Pattern][Row].EffNumber[2] != 0 || m_stPatternData[Channel][Pattern][Row].EffNumber[3] != 0 || 
		m_stPatternData[Channel][Pattern][Row].Vol < 0x10 || m_stPatternData[Channel][Pattern][Row].Instrument != MAX_INSTRUMENTS);
}
