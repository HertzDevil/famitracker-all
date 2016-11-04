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
// Derived channels, VRC7
//

class CChannelHandlerVRC7 : public CChannelHandler {
public:
	CChannelHandlerVRC7(int ChanID);
	virtual void ProcessChannel();
	virtual void ResetChannel();
protected:
	virtual void PlayChannelNote(stChanNote *NoteData, int EffColumns);
	unsigned int TriggerNote(int Note);
	int LimitFreq(int Freq);
protected:
	unsigned int GetFnum(int Note);

protected:
	unsigned char m_iChannel;
	unsigned char m_iPatch;
	int m_iCommand;
	char m_iRegs[8];

	int m_iTriggeredNote;
	bool m_bHold;

	int m_iOctave;
};

class CVRC7Channel : public CChannelHandlerVRC7 {
public:
	CVRC7Channel() : CChannelHandlerVRC7(5) {};
	void RefreshChannel();
	void ChannelIndex(int Channel) {m_iChannel = Channel; m_iChannelID = 5 + Channel; };
protected:
	void ClearRegisters();
private:
	void RegWrite(unsigned char Reg, unsigned char Value);
};
