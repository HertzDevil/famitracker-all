/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2007  Jonathan Liss
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
#include "Instrument.h"

/*
 * class CInstrument
 *
 */

CInstrument::CInstrument()
{
	m_bFree = true;
}

bool CInstrument::IsFree()
{
	return m_bFree;
}

void CInstrument::SetFree(bool Free)
{
	m_bFree = Free;
}

void CInstrument::SetName(char *Name)
{
	strcpy(m_cName, Name);
}

void CInstrument::GetName(char *Name) const
{
	strcpy(Name, m_cName);
}

/*
 * class CInstrument2A03
 *
 */

CInstrument2A03::CInstrument2A03()
{
	for (int i = 0; i < OCTAVE_RANGE; i++) {
		for (int j = 0; j < 12; j++) {
			m_cSamples[i][j] = 0;
			m_cSamplePitch[i][j] = 0;
		}
	}
}

int	CInstrument2A03::GetModEnable(int Index)
{
	return m_iModEnable[Index];
}

int	CInstrument2A03::GetModIndex(int Index)
{
	return m_iModIndex[Index];
}

void CInstrument2A03::SetModEnable(int Index, int Value)
{
	m_iModEnable[Index] = Value;
}

void CInstrument2A03::SetModIndex(int Index, int Value)
{
	m_iModIndex[Index] = Value;
}

char CInstrument2A03::GetSample(int Octave, int Note)
{
	return m_cSamples[Octave][Note];
}

char CInstrument2A03::GetSamplePitch(int Octave, int Note)
{
	return m_cSamplePitch[Octave][Note];
}

void CInstrument2A03::SetSample(int Octave, int Note, char Sample)
{
	m_cSamples[Octave][Note] = Sample;
}

void CInstrument2A03::SetSamplePitch(int Octave, int Note, char Pitch)
{
	m_cSamplePitch[Octave][Note] = Pitch;
}

/*
 * class CInstrumentVRC6
 *
 */

CInstrumentVRC6::CInstrumentVRC6()
{
	for (int i = 0; i < MOD_COUNT; i++) {
		m_iModEnable[i] = 0;
		m_iModIndex[i] = 0;
	}	
}

int	CInstrumentVRC6::GetModEnable(int Index)
{
	return m_iModEnable[Index];
}

int	CInstrumentVRC6::GetModIndex(int Index)
{
	return m_iModIndex[Index];
}

void CInstrumentVRC6::SetModEnable(int Index, int Value)
{
	m_iModEnable[Index] = Value;
}

void CInstrumentVRC6::SetModIndex(int Index, int Value)
{
	m_iModIndex[Index] = Value;
}
