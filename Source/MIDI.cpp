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

#include "stdafx.h"
#include <mmsystem.h>
#include "FamiTracker.h"
#include "MIDI.h"


// CMIDI

CMIDI::CMIDI()
{
	m_bOpened = false;
	m_iDevice = 0;			// Disabled
}

CMIDI::~CMIDI()
{
}

const int MAX_QUEUE = 100;

char MsgTypeQueue[MAX_QUEUE], MsgChanQueue[MAX_QUEUE], NoteQueue[MAX_QUEUE], Data2Queue[MAX_QUEUE];

int QueuePtr = 0;
int QueueHead = 0, QueueTail = 0;

char LastMsgType, LastMsgChan;
char LastNote;

HMIDIIN		hMIDIIn;

void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

static CMIDI *pInterface;

void Enqueue(unsigned char MsgType, unsigned char MsgChannel, unsigned char Data1, unsigned char Data2)
{
	MsgTypeQueue[QueueHead] = MsgType;
	MsgChanQueue[QueueHead] = MsgChannel;
	NoteQueue[QueueHead]	= Data1;
	Data2Queue[QueueHead]	= Data2;

	QueueHead = (QueueHead + 1) % MAX_QUEUE;
}

void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	unsigned char Status, Data1, Data2;

	if (wMsg == MIM_DATA) {

		Status	= (char)(dwParam1 & 0xFF);
		Data1	= (char)(dwParam1 >> 8) & 0xFF;
		Data2	= (char)(dwParam1 >> 16) & 0xFF;

		pInterface->Event(Status, Data1, Data2);
	}
}

// CMIDI member functions

bool CMIDI::Init(void)
{
	// Load MIDI

	pInterface = this;

	m_iDevice = theApp.m_pSettings->Midi.iMidiDevice;

	OpenSelectedDevice();

	return true;
}

void CMIDI::Shutdown(void)
{
	theApp.m_pSettings->Midi.iMidiDevice = m_iDevice;

	CloseDevice();
}

bool CMIDI::OpenSelectedDevice()
{
	MMRESULT Result;

	if (m_iDevice == 0)
		return true;

	Result = midiInOpen(&hMIDIIn, m_iDevice - 1, (DWORD_PTR)MidiInProc, 0, CALLBACK_FUNCTION);

	if (Result != MMSYSERR_NOERROR) {
		AfxMessageBox("MIDI Error: Could not open MIDI input device!");
		return false;
	}

	midiInStart(hMIDIIn);

	QueueHead = QueueTail = 0;

	m_bOpened = true;

	return true;
}

bool CMIDI::CloseDevice(void)
{
	if (!m_bOpened)
		return false;

	midiInClose(hMIDIIn);

	m_bOpened = false;

	return false;
}

int CMIDI::GetNumDevices()
{
	return midiInGetNumDevs();
}

void CMIDI::GetDeviceString(int Num, char *Text)
{
	MIDIINCAPS	InCaps;

	midiInGetDevCaps(Num, &InCaps, sizeof(MIDIINCAPS));

	sprintf(Text, "%s", InCaps.szPname);
}

void CMIDI::OpenConfigDialog(void)
{
}

void CMIDI::SetDevice(int DeviceNr, bool MasterSync)
{
	m_iDevice = DeviceNr;
	m_bMasterSync = MasterSync;

	if (m_bOpened)
		CloseDevice();

	OpenSelectedDevice();
}

void CMIDI::Event(unsigned char Status, unsigned char Data1, unsigned char Data2)
{
	static int TimingCounter;

	unsigned char MsgType, MsgChannel;

	MsgType		= Status >> 4;
	MsgChannel	= Status & 0x0F;

	switch (MsgType) {
		case MIDI_MSG_NOTE_OFF:
		case MIDI_MSG_NOTE_ON: 
			Enqueue(MsgType, MsgChannel, Data1, Data2);
			theApp.MidiEvent();
			break;
	}

	// Timing
	if (Status == 0xF8 && m_bMasterSync) {
		if (TimingCounter++ == 6) {
			TimingCounter = 0;
			Enqueue(MsgType, MsgChannel, Data1, Data2);
			theApp.MidiEvent();
		}
	}
}

bool CMIDI::ReadMessage(unsigned char & Message, unsigned char & Channel, unsigned char & Note, unsigned char & Octave, unsigned char & Velocity)
{
	if (QueueHead == QueueTail)
		return false;

	LastMsgType = MsgTypeQueue[QueueTail];
	LastMsgChan = MsgChanQueue[QueueTail];
	LastNote = NoteQueue[QueueTail];

	Velocity = Data2Queue[QueueTail];

	QueueTail = (QueueTail + 1) % MAX_QUEUE;

	Message = LastMsgType;
	Channel = LastMsgChan;
	Note	= (LastNote % 12) + 1;
	Octave	= (LastNote / 12) - 2;

	return true;
}

void CMIDI::Toggle()
{
	if (m_bOpened)
		CloseDevice();
	else
		OpenSelectedDevice();
}