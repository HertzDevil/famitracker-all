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

const int MIDI_MSG_NOTE_OFF			= 8;
const int MIDI_MSG_NOTE_ON			= 9;
const int MIDI_MSG_AFTER_TOUCH		= 0xA;
const int MIDI_MSG_CONTROL_CHANGE	= 0xB;
const int MIDI_MSG_PROGRAM_CHANGE	= 0xC;
const int MIDI_MSG_CHANNEL_PRESSURE = 0xD;
const int MIDI_MSG_PITCH_WHEEL		= 0xE;

// CMIDI command target

class CMIDI : public CObject
{
public:
	CMIDI();
	virtual ~CMIDI();
	bool	OpenSelectedDevice();
	void	OpenConfigDialog(void);
	bool	ReadMessage(unsigned char & Message, unsigned char & Channel, unsigned char & Note, unsigned char & Octave, unsigned char & Velocity);
	bool	CloseDevice(void);
	int		GetNumDevices();
	void	GetDeviceString(int Num, char *Text);
	void	SetDevice(int DeviceNr, bool MasterSync);
	void	Event(unsigned char Status, unsigned char Data1, unsigned char Data2);
	void	Toggle();

	bool	IsOpened()		{ return m_bOpened; }
	bool	IsAvailable()	{ return m_iDevice > 0; }

	int		m_iDevice;
	bool	m_bMasterSync;

private:
	bool	m_bOpened;

public:
	bool	Init(void);
	void	Shutdown(void);
};
