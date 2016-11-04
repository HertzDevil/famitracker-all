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


#pragma once

struct stChanNote {
	int		Note;
	int		Octave;
	int		Vol;
	int		Instrument;
	int		EffNumber[MAX_EFFECT_COLUMNS];
	int		EffParam[MAX_EFFECT_COLUMNS];
};

class CPatternData {
public:
	void	Init(unsigned int PatternLength, unsigned int FrameCount, unsigned int Speed);

	void	ClearNote(unsigned int Channel, unsigned int Pattern, unsigned int Row);
	void	ClearPattern(unsigned int Channels);

	void	SetEffect(unsigned int Channel, unsigned int Pattern, unsigned int Row, unsigned int Column, char EffNumber, char EffParam);
	void	SetInstrument(unsigned int Channel, unsigned int Pattern, unsigned int Row, char Instrument);
	void	SetNote(unsigned int Channel, unsigned int Pattern, unsigned int Row, char Note);
	void	SetOctave(unsigned int Channel, unsigned int Pattern, unsigned int Row, char Octave);
	void	SetVolume(unsigned int Channel, unsigned int Pattern, unsigned int Row, char Volume);

	char GetNote(unsigned int Channel, unsigned int Pattern, unsigned int Row) 
		{ return m_stPatternData[Channel][Pattern][Row].Note; };
	char GetOctave(unsigned int Channel, unsigned int Pattern, unsigned int Row) 
		{ return m_stPatternData[Channel][Pattern][Row].Octave; };
	char GetInstrument(unsigned int Channel, unsigned int Pattern, unsigned int Row) 
		{ return m_stPatternData[Channel][Pattern][Row].Instrument; };
	char GetVolume(unsigned int Channel, unsigned int Pattern, unsigned int Row) 
		{ return m_stPatternData[Channel][Pattern][Row].Vol; };
	char GetEffect(unsigned int Channel, unsigned int Pattern, unsigned int Row, unsigned int Column) 
		{ return m_stPatternData[Channel][Pattern][Row].EffNumber[Column]; };
	char GetEffectParam(unsigned int Channel, unsigned int Pattern, unsigned int Row, unsigned int Column) 
		{ return m_stPatternData[Channel][Pattern][Row].EffParam[Column]; };

	bool	IsCellFree(unsigned int Channel, unsigned int Pattern, unsigned int Row);

	stChanNote		m_stPatternData[MAX_CHANNELS][MAX_PATTERN][MAX_PATTERN_LENGTH];	// The patterns

	unsigned int	m_iFrameList[MAX_FRAMES][MAX_CHANNELS];		// List of the patterns assigned to frames

	unsigned int	m_iPatternLength;							// Amount of rows in one pattern
	unsigned int	m_iFrameCount;								// Number of frames
	unsigned int	m_iSongSpeed;								// Song speed

	unsigned int	m_iEffectColumns[MAX_CHANNELS];				// Effect columns enabled

private:

};
