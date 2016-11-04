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
#include "FamiTracker.h"
#include "Settings.h"

UINT GetAppProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nDefault)
{
	return theApp.GetProfileInt(lpszSection, lpszEntry, nDefault);
}

CString GetAppProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszDefault = NULL)
{
	return theApp.GetProfileString(lpszSection, lpszEntry, lpszDefault);
}

BOOL WriteAppProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int nValue)
{
	return theApp.WriteProfileInt(lpszSection, lpszEntry, nValue);
}

BOOL WriteAppProfileString(LPCTSTR lpszSection, LPCTSTR lpszEntry, LPCTSTR lpszValue)
{
	return theApp.WriteProfileString(lpszSection, lpszEntry, lpszValue);
}

// CSettings

CSettings::CSettings()
{
}

CSettings::~CSettings()
{
}


// CSettings member functions

void CSettings::LoadSettings()
{
	// General 
	General.bWrapCursor		= GetAppProfileInt("General", "Wrap cursor", 1) == 1;
	General.bFreeCursorEdit	= GetAppProfileInt("General", "Free cursor edit", 0) == 1;
	General.bWavePreview	= GetAppProfileInt("General", "Wave preview", 1) == 1;
	General.bKeyRepeat		= GetAppProfileInt("General", "Key repeat", 1) == 1;
	General.bRowInHex		= GetAppProfileInt("General", "Hex row display", 0) == 1;
	General.iEditStyle		= GetAppProfileInt("General", "Edit style", EDIT_STYLE1);
	General.strFont			= GetAppProfileString("General", "Pattern font", "Fixedsys");
	General.bKeySelect		= GetAppProfileInt("General", "Keyboard select", 0) == 1;

	// Sound
	Sound.iSampleRate		= GetAppProfileInt("Sound", "Sample rate", 44100);
	Sound.iSampleSize		= GetAppProfileInt("Sound", "Sample size", 16);
	Sound.iBufferLength		= GetAppProfileInt("Sound", "Buffer length", 40);
	Sound.iBassFilter		= GetAppProfileInt("Sound", "Bass filter freq", 16);
	Sound.iTrebleFilter		= GetAppProfileInt("Sound", "Treble filter freq", 12000);
	Sound.iTrebleDamping	= GetAppProfileInt("Sound", "Treble filter damping", 24);

	// Midi
	Midi.iMidiDevice		= GetAppProfileInt("MIDI", "Device", 0);
	Midi.bMidiMasterSync	= GetAppProfileInt("MIDI", "Master sync", 0) == 1;
	Midi.bMidiKeyRelease	= GetAppProfileInt("MIDI", "Key release", 0) == 1;
	Midi.bMidiChannelMap	= GetAppProfileInt("MIDI", "Channel map", 0) == 1;
	Midi.bMidiVelocity		= GetAppProfileInt("MIDI", "Velocity control", 0) == 1;

	// Appearance
	Appearance.iColBackground			= GetAppProfileInt("Appearance", "Background", COLOR_SCHEME.BACKGROUND);
	Appearance.iColBackgroundHilite		= GetAppProfileInt("Appearance", "Background highlighted", COLOR_SCHEME.BACKGROUND_HILITE);
	Appearance.iColPatternText			= GetAppProfileInt("Appearance", "Pattern text", COLOR_SCHEME.TEXT_NORMAL);
	Appearance.iColPatternTextHilite	= GetAppProfileInt("Appearance", "Pattern text highlighted", COLOR_SCHEME.TEXT_HILITE);
	Appearance.iColSelection			= GetAppProfileInt("Appearance", "Selection", COLOR_SCHEME.SELECTION);
	Appearance.iColCursor				= GetAppProfileInt("Appearance", "Cursor", COLOR_SCHEME.CURSOR);

}

void CSettings::SaveSettings()
{
	// General
	WriteAppProfileInt("General", "Wrap cursor",			General.bWrapCursor);
	WriteAppProfileInt("General", "Free cursor edit",		General.bFreeCursorEdit);
	WriteAppProfileInt("General", "Wave preview",			General.bWavePreview);
	WriteAppProfileInt("General", "Key repeat",				General.bKeyRepeat);
	WriteAppProfileInt("General", "Hex row display",		General.bRowInHex);
	WriteAppProfileInt("General", "Edit style",				General.iEditStyle);
	WriteAppProfileString("General", "Pattern font",		General.strFont);
	WriteAppProfileInt("General", "Keyboard select",		General.bKeySelect);

	// Sound
	WriteAppProfileInt("Sound", "Sample rate",				Sound.iSampleRate);
	WriteAppProfileInt("Sound", "Sample size",				Sound.iSampleSize);
	WriteAppProfileInt("Sound", "Buffer length",			Sound.iBufferLength);
	WriteAppProfileInt("Sound", "Bass filter freq",			Sound.iBassFilter);
	WriteAppProfileInt("Sound", "Treble filter freq",		Sound.iTrebleFilter);
	WriteAppProfileInt("Sound", "Treble filter damping",	Sound.iTrebleDamping);

	// Midi
	WriteAppProfileInt("MIDI", "Device",					Midi.iMidiDevice);
	WriteAppProfileInt("MIDI", "Master sync",				Midi.bMidiMasterSync);
	WriteAppProfileInt("MIDI", "Key release",				Midi.bMidiKeyRelease);
	WriteAppProfileInt("MIDI", "Channel map",				Midi.bMidiChannelMap);
	WriteAppProfileInt("MIDI", "Velocity control",			Midi.bMidiVelocity);

	// Appearance
	WriteAppProfileInt("Appearance", "Background",					Appearance.iColBackground);
	WriteAppProfileInt("Appearance", "Background highlighted",		Appearance.iColBackgroundHilite);
	WriteAppProfileInt("Appearance", "Pattern text",				Appearance.iColPatternText);
	WriteAppProfileInt("Appearance", "Pattern text highlighted",	Appearance.iColPatternTextHilite);
	WriteAppProfileInt("Appearance", "Selection",					Appearance.iColSelection);
	WriteAppProfileInt("Appearance", "Cursor",						Appearance.iColCursor);
}

void CSettings::DefaultSettings()
{
	// General
	General.bWrapCursor		= 1;
	General.bFreeCursorEdit	= 0;
	General.bWavePreview	= 1;
	General.bKeyRepeat		= 0;
	General.bRowInHex		= 0;
	General.strFont			= "Fixedsys";
	General.iEditStyle		= EDIT_STYLE1;
	General.bKeySelect		= false;

	// Sound
	Sound.iSampleRate		= 44100;
	Sound.iSampleSize		= 16;
	Sound.iBufferLength		= 40;
	Sound.iBassFilter		= 16;
	Sound.iTrebleFilter		= 12000;
	Sound.iTrebleDamping	= 24;

	// Midi
	Midi.iMidiDevice		= 0;
	Midi.bMidiMasterSync	= 0;
	Midi.bMidiKeyRelease	= 0;
	Midi.bMidiChannelMap	= 0;

	// Appearance
	Appearance.iColBackground			= COLOR_SCHEME.BACKGROUND;
	Appearance.iColBackgroundHilite		= COLOR_SCHEME.BACKGROUND_HILITE;
	Appearance.iColPatternText			= COLOR_SCHEME.TEXT_NORMAL;
	Appearance.iColPatternTextHilite	= COLOR_SCHEME.TEXT_HILITE;
	Appearance.iColSelection			= COLOR_SCHEME.SELECTION;
	Appearance.iColCursor				= COLOR_SCHEME.CURSOR;
}
