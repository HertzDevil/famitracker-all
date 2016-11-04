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
// Derived channels, MMC5
//

class CChannelHandlerMMC5 : public CChannelHandler {
public:
	void PlayNote(stChanNote *NoteData, int EffColumns);
	void ProcessChannel();
protected:
	void RunSequence(int Index, CSequence *pSequence);

	unsigned char m_cSweep;
	unsigned char m_cDutyCycle, m_iDefaultDuty;

	int ModEnable[MOD_COUNT];
	int	ModIndex[MOD_COUNT];
	int	ModDelay[MOD_COUNT];
	int	ModPointer[MOD_COUNT];
};

// Square 1
class CMMC5Square1Chan : public CChannelHandlerMMC5 {
public:
	CMMC5Square1Chan() { m_iDefaultDuty = 0; m_iChannelID = 5; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
};

// Square 2
class CMMC5Square2Chan : public CChannelHandlerMMC5 {
public:
	CMMC5Square2Chan() { m_iDefaultDuty = 0; m_iChannelID = 6; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
};
