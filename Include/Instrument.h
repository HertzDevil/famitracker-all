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

class CInstrument {
public:
	CInstrument();
	bool IsFree();
	void SetFree(bool Free);
	void LoadInst(int Type);
	void SetName(char *Name);
	void GetName(char *Name) const;
	virtual int GetType() = 0;

private:
	char	m_cName[256];
	int		m_iType;
	bool	m_bFree;
};

class CInstrument2A03 : public CInstrument {
public:
	CInstrument2A03();
	int	GetType() { return INST_2A03; };
public:
	int		GetModEnable(int Index);
	int		GetModIndex(int Index);
	void	SetModIndex(int Index, int Value);
	void	SetModEnable(int Index, int Value);

	char	GetSample(int Octave, int Note);
	char	GetSamplePitch(int Octave, int Note);
	void	SetSample(int Octave, int Note, char Sample);
	void	SetSamplePitch(int Octave, int Note, char Pitch);

private:
	int		m_iModEnable[MOD_COUNT];
	int		m_iModIndex[MOD_COUNT];
	char	m_cSamples[OCTAVE_RANGE][12];				// Samples
	char	m_cSamplePitch[OCTAVE_RANGE][12];			// Play pitch/loop
};

class CInstrumentVRC6 : public CInstrument {
public:
	CInstrumentVRC6();
	int	GetType() { return INST_VRC6; };
public:
	int		GetModEnable(int Index);
	int		GetModIndex(int Index);
	void	SetModEnable(int Index, int Value);
	void	SetModIndex(int Index, int Value);

private:
	int		m_iModEnable[MOD_COUNT];
	int		m_iModIndex[MOD_COUNT];
};

class CInstrumentVRC7 : public CInstrument {
public:
	CInstrumentVRC7();
	int	GetType() { return INST_VRC7; };
public:
	void SetPatch(int Patch);
	int GetPatch() const { return m_iPatch; };
private:
	int m_iPatch;
};