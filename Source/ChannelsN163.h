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

//
// Derived channels, N163
//

class CChannelHandlerN163 : public CChannelHandlerInverted {
public:
	CChannelHandlerN163();
	virtual void ResetChannel();
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

private:
	void WriteReg(int Reg, int Value);
	void SetAddress(char Addr, bool AutoInc);
	void WriteData(char Data);
	void WriteData(int Addr, char Data);
	void LoadWave();
	void CheckWaveUpdate();
private:
	inline int GetIndex() const { return m_iChannelID - CHANID_N163_CHAN1; }
private:
	bool m_bLoadWave;
	int m_iChannels;
	int m_iWaveLen;
	int m_iWavePos;
	int m_iWaveIndex;
	int m_iWaveCount;
protected:
	int m_iPostEffect;
	int m_iPostEffectParam;

	bool m_bResetPhase;
};
