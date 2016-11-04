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

//
// Derived channels, 2A03
//

class CChannelHandler2A03 : public CChannelHandler {
public:
	void PlayNote(stChanNote *NoteData, int EffColumns);
	void ProcessChannel();
protected:
	void RunSequence(int Index, CSequence *pSequence);

	unsigned char m_cSweep;
	unsigned char m_cDutyCycle;

	int ModEnable[MOD_COUNT];
	int	ModIndex[MOD_COUNT];
	int	ModDelay[MOD_COUNT];
	int	ModPointer[MOD_COUNT];
};

// Square 1
class CSquare1Chan : public CChannelHandler2A03 {
public:
	CSquare1Chan() { m_iDefaultDuty = 0; m_iChannelID = 0; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
};

// Square 2
class CSquare2Chan : public CChannelHandler2A03 {
public:
	CSquare2Chan() { m_iDefaultDuty = 0; m_iChannelID = 1; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
};

// Triangle
class CTriangleChan : public CChannelHandler2A03 {
public:
	CTriangleChan() { m_iChannelID = 2; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
};

// Noise
class CNoiseChan : public CChannelHandler2A03 {
public:
	CNoiseChan() { m_iDefaultDuty = 0; m_iChannelID = 3; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
	unsigned int TriggerNote(int Note);
};

// DPCM
class CDPCMChan : public CChannelHandler2A03 {
public:
	CDPCMChan() { m_iChannelID = 4; m_bEnabled = false; };
	void RefreshChannel();
	void PlayNote(stChanNote *NoteData, int EffColumns);
	void SetSampleMem(CSampleMem *pSampleMem);
protected:
	void ClearRegisters();
private:
	CSampleMem *m_pSampleMem;
	unsigned char m_cDAC;
	unsigned char m_iLoop;
	unsigned char m_iOffset;
	bool m_bKeyRelease;
};