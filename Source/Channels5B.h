/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2010  Jonathan Liss
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
// Derived channels, 5B
//

class CChannelHandler5B : public CChannelHandler {
public:
	CChannelHandler5B(int ChanID) : CChannelHandler(ChanID) {};
	virtual void ProcessChannel();
	virtual void RefreshChannel();

protected:
	virtual void PlayChannelNote(stChanNote *NoteData, int EffColumns);
	virtual void ClearRegisters();

//	void RunSequence(int Index, CSequence *pSequence);
/*
	unsigned char m_cSweep;
	unsigned char m_cDutyCycle, m_iDefaultDuty;

	int ModEnable[SEQ_COUNT];
	int	ModIndex[SEQ_COUNT];
	int	ModDelay[SEQ_COUNT];
	int	ModPointer[SEQ_COUNT];
	*/
};

// Channel 1
class C5BChannel1 : public CChannelHandler5B {
public:
	C5BChannel1() : CChannelHandler5B(5) { m_iDefaultDuty = 0; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
};

// Channel 2
class C5BChannel2 : public CChannelHandler5B {
public:
	C5BChannel2() : CChannelHandler5B(6) { m_iDefaultDuty = 0; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
};

// Channel 3
class C5BChannel3 : public CChannelHandler5B {
public:
	C5BChannel3() : CChannelHandler5B(7) { m_iDefaultDuty = 0; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
};
