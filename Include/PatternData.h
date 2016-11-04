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


#pragma once

struct stChanNote {
	unsigned char Note;
	unsigned char Octave;
	unsigned char Vol;
	unsigned char Instrument;
	unsigned char EffNumber[MAX_EFFECT_COLUMNS];
	unsigned char EffParam[MAX_EFFECT_COLUMNS];
};

class CPatternData {
public:
	CPatternData();
	~CPatternData();

	void	Init(unsigned int PatternLength, unsigned int FrameCount, unsigned int Speed, unsigned int Tempo);

	void	SetEffect(unsigned int Channel, unsigned int Pattern, unsigned int Row, unsigned int Column, char EffNumber, char EffParam);
	void	SetInstrument(unsigned int Channel, unsigned int Pattern, unsigned int Row, char Instrument);
	void	SetNote(unsigned int Channel, unsigned int Pattern, unsigned int Row, char Note);
	void	SetOctave(unsigned int Channel, unsigned int Pattern, unsigned int Row, char Octave);
	void	SetVolume(unsigned int Channel, unsigned int Pattern, unsigned int Row, char Volume);

	char GetNote(unsigned int Channel, unsigned int Pattern, unsigned int Row) 
		{ return GetPatternData(Channel, Pattern, Row)->Note; };

	char GetOctave(unsigned int Channel, unsigned int Pattern, unsigned int Row) 
		{ return GetPatternData(Channel, Pattern, Row)->Octave; };

	char GetInstrument(unsigned int Channel, unsigned int Pattern, unsigned int Row) 
		{ return GetPatternData(Channel, Pattern, Row)->Instrument; };

	char GetVolume(unsigned int Channel, unsigned int Pattern, unsigned int Row) 
		{ return GetPatternData(Channel, Pattern, Row)->Vol; };

	char GetEffect(unsigned int Channel, unsigned int Pattern, unsigned int Row, unsigned int Column) 
		{ return GetPatternData(Channel, Pattern, Row)->EffNumber[Column]; };

	char GetEffectParam(unsigned int Channel, unsigned int Pattern, unsigned int Row, unsigned int Column) 
		{ return GetPatternData(Channel, Pattern, Row)->EffParam[Column]; };

	bool IsCellFree(unsigned int Channel, unsigned int Pattern, unsigned int Row);
	bool IsPatternFree(unsigned int Channel, unsigned int Pattern);

	void ClearEverything();

	stChanNote *GetPatternData(int Channel, int Pattern, int Row);

	// Move these to the private area
	unsigned short	m_iFrameList[MAX_FRAMES][MAX_CHANNELS];		// List of the patterns assigned to frames

	unsigned int	m_iPatternLength;							// Amount of rows in one pattern
	unsigned int	m_iFrameCount;								// Number of frames
	unsigned int	m_iSongSpeed;								// Song speed
	unsigned int	m_iSongTempo;								// Song tempo

	unsigned int	m_iEffectColumns[MAX_CHANNELS];				// Effect columns enabled

private:
	void			AllocatePattern(int Channel, int Patterns);

	// All accesses to m_pPatternData must go through GetPatternData()
	stChanNote		*m_pPatternData[MAX_CHANNELS][MAX_PATTERN];
};
