/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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

class CChannelHandlerFDS : public CChannelHandlerInverted {
public:
	CChannelHandlerFDS();
	virtual void ProcessChannel();
	virtual void RefreshChannel();
protected:
	virtual void HandleNoteData(stChanNote *pNoteData, int EffColumns);
	virtual void HandleCustomEffects(int EffNum, int EffParam);
	virtual bool HandleInstrument(int Instrument, bool Trigger, bool NewInstrument);
	virtual void HandleEmptyNote();
	virtual void HandleCut();
	virtual void HandleRelease();
	virtual void HandleNote(int Note, int Octave);
	virtual void ClearRegisters();

protected:
	// FDS functions
	void FillWaveRAM(CInstrumentFDS *pInst);
	void FillModulationTable(CInstrumentFDS *pInst);
private:
	void CheckWaveUpdate();
protected:
	// FDS control variables
	int m_iModulationSpeed;
	int m_iModulationDepth;
	int m_iModulationDelay;
	// FDS sequences
	//CSequence *m_pVolumeSeq;
	//CSequence *m_pArpeggioSeq;
	//CSequence *m_pPitchSeq;
	// Modulation table
	char m_iModTable[32];
	char m_iWaveTable[64];
	// Modulation
	bool m_bResetMod;
protected:
	int m_iPostEffect;
	int m_iPostEffectParam;

	int m_iEffModDepth;
	int m_iEffModSpeedHi;
	int m_iEffModSpeedLo;
};
