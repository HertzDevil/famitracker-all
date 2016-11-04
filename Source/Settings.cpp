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

/*

	When new settings are added, don't forget to update load, store and default setting routines

*/

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerView.h"
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
	General.bWrapCursor					= GetAppProfileInt("General", "Wrap cursor", 1) == 1;
	General.bWrapFrames					= GetAppProfileInt("General", "Wrap across frames", 1) == 1;
	General.bFreeCursorEdit				= GetAppProfileInt("General", "Free cursor edit", 0) == 1;
	General.bWavePreview				= GetAppProfileInt("General", "Wave preview", 1) == 1;
	General.bKeyRepeat					= GetAppProfileInt("General", "Key repeat", 1) == 1;
	General.bRowInHex					= GetAppProfileInt("General", "Hex row display", 1) == 1;
	General.iEditStyle					= GetAppProfileInt("General", "Edit style", EDIT_STYLE1);
	General.strFont						= GetAppProfileString("General", "Pattern font", "Fixedsys");
	General.bFramePreview				= GetAppProfileInt("General", "Frame preview", 1) == 1;
	General.bNoDPCMReset				= GetAppProfileInt("General", "No DPCM reset", 0) == 1;
	General.bNoStepMove					= GetAppProfileInt("General", "No Step moving", 0) == 1;
	General.iPageStepSize				= GetAppProfileInt("General", "Page step size", 4);
	General.bPatternColor				= GetAppProfileInt("General", "Pattern colors", 1) == 1;
	General.bPullUpDelete				= GetAppProfileInt("General", "Delete pull up", 0) == 1;
	General.bBackups					= GetAppProfileInt("General", "Backups", 0) == 1;

	// Keys
	Keys.iKeyNoteCut					= GetAppProfileInt("Keys", "Note cut", 0xE2);
	Keys.iKeyNoteRelease				= GetAppProfileInt("Keys", "Note release", 0xDC);
	Keys.iKeyClear						= GetAppProfileInt("Keys", "Clear field", 0xBD);
	Keys.iKeyRepeat						= GetAppProfileInt("Keys", "Repeat", 0x00);

	// Sound
	strDevice							= GetAppProfileString("Sound", "Device", "");
	Sound.iSampleRate					= GetAppProfileInt("Sound", "Sample rate", 44100);
	Sound.iSampleSize					= GetAppProfileInt("Sound", "Sample size", 16);
	Sound.iBufferLength					= GetAppProfileInt("Sound", "Buffer length", 40);
	Sound.iBassFilter					= GetAppProfileInt("Sound", "Bass filter freq", 16);
	Sound.iTrebleFilter					= GetAppProfileInt("Sound", "Treble filter freq", 12000);
	Sound.iTrebleDamping				= GetAppProfileInt("Sound", "Treble filter damping", 24);
	Sound.iMixVolume					= GetAppProfileInt("Sound", "Volume", 100);

	// Midi
	Midi.iMidiDevice					= GetAppProfileInt("MIDI", "Device", 0);
	Midi.iMidiOutDevice					= GetAppProfileInt("MIDI", "Out Device", 0);
	Midi.bMidiMasterSync				= GetAppProfileInt("MIDI", "Master sync", 0) == 1;
	Midi.bMidiKeyRelease				= GetAppProfileInt("MIDI", "Key release", 0) == 1;
	Midi.bMidiChannelMap				= GetAppProfileInt("MIDI", "Channel map", 0) == 1;
	Midi.bMidiVelocity					= GetAppProfileInt("MIDI", "Velocity control", 0) == 1;
	Midi.bMidiArpeggio					= GetAppProfileInt("MIDI", "Auto Arpeggio", 0) == 1;

	// Appearance
	Appearance.iColBackground			= GetAppProfileInt("Appearance", "Background", COLOR_SCHEME.BACKGROUND);
	Appearance.iColBackgroundHilite		= GetAppProfileInt("Appearance", "Background highlighted", COLOR_SCHEME.BACKGROUND_HILITE);
	Appearance.iColBackgroundHilite2	= GetAppProfileInt("Appearance", "Background highlighted 2", COLOR_SCHEME.BACKGROUND_HILITE2);
	Appearance.iColPatternText			= GetAppProfileInt("Appearance", "Pattern text", COLOR_SCHEME.TEXT_NORMAL);
	Appearance.iColPatternTextHilite	= GetAppProfileInt("Appearance", "Pattern text highlighted", COLOR_SCHEME.TEXT_HILITE);
	Appearance.iColPatternTextHilite2	= GetAppProfileInt("Appearance", "Pattern text highlighted 2", COLOR_SCHEME.TEXT_HILITE2);
	Appearance.iColPatternInstrument	= GetAppProfileInt("Appearance", "Pattern instrument", COLOR_SCHEME.TEXT_INSTRUMENT);
	Appearance.iColPatternVolume		= GetAppProfileInt("Appearance", "Pattern volume", COLOR_SCHEME.TEXT_VOLUME);
	Appearance.iColPatternEffect		= GetAppProfileInt("Appearance", "Pattern effect", COLOR_SCHEME.TEXT_EFFECT);
	Appearance.iColSelection			= GetAppProfileInt("Appearance", "Selection", COLOR_SCHEME.SELECTION);
	Appearance.iColCursor				= GetAppProfileInt("Appearance", "Cursor", COLOR_SCHEME.CURSOR);

	// Windows position
	WindowPos.iLeft						= GetAppProfileInt("Window position", "Left", 100);
	WindowPos.iTop						= GetAppProfileInt("Window position", "Top", 100);
	WindowPos.iRight					= GetAppProfileInt("Window position", "Right", 950);
	WindowPos.iBottom					= GetAppProfileInt("Window position", "Bottom", 920);
	WindowPos.iState					= GetAppProfileInt("Window position", "State", STATE_NORMAL);

	// Other
	SampleWinState						= GetAppProfileInt("Other", "Sample window state", 0);

	// Paths
	Paths[PATH_FTM]						= GetAppProfileString("Paths", "FTM path", "");
	Paths[PATH_FTI]						= GetAppProfileString("Paths", "FTI path", "");
	Paths[PATH_NSF]						= GetAppProfileString("Paths", "NSF path", "");
	Paths[PATH_DMC]						= GetAppProfileString("Paths", "DMC path", "");
	Paths[PATH_WAV]						= GetAppProfileString("Paths", "WAV path", "");
}

void CSettings::SaveSettings()
{
	// General
	WriteAppProfileInt("General", "Wrap cursor",	  General.bWrapCursor);
	WriteAppProfileInt("General", "Wrap across frames",	  General.bWrapFrames);
	WriteAppProfileInt("General", "Free cursor edit", General.bFreeCursorEdit);
	WriteAppProfileInt("General", "Wave preview",	  General.bWavePreview);
	WriteAppProfileInt("General", "Key repeat",		  General.bKeyRepeat);
	WriteAppProfileInt("General", "Hex row display",  General.bRowInHex);
	WriteAppProfileInt("General", "Edit style",		  General.iEditStyle);
	WriteAppProfileString("General", "Pattern font",  General.strFont);
	WriteAppProfileInt("General", "Frame preview",	  General.bFramePreview);
	WriteAppProfileInt("General", "No DPCM reset",	  General.bNoDPCMReset);
	WriteAppProfileInt("General", "No Step moving",	  General.bNoStepMove);
	WriteAppProfileInt("General", "Page step size",	  General.iPageStepSize);
	WriteAppProfileInt("General", "Pattern colors",	  General.bPatternColor);
	WriteAppProfileInt("General", "Delete pull up",	  General.bPullUpDelete);
	WriteAppProfileInt("General", "Backups",		  General.bBackups);

	// Keys
	WriteAppProfileInt("Keys", "Note cut", Keys.iKeyNoteCut);
	WriteAppProfileInt("Keys", "Note release", Keys.iKeyNoteRelease);
	WriteAppProfileInt("Keys", "Clear field", Keys.iKeyClear);
	WriteAppProfileInt("Keys", "Repeat", Keys.iKeyRepeat);

	// Sound
	WriteAppProfileInt("Sound", "Sample rate",			 Sound.iSampleRate);
	WriteAppProfileInt("Sound", "Sample size",			 Sound.iSampleSize);
	WriteAppProfileInt("Sound", "Buffer length",		 Sound.iBufferLength);
	WriteAppProfileInt("Sound", "Bass filter freq",		 Sound.iBassFilter);
	WriteAppProfileInt("Sound", "Treble filter freq",	 Sound.iTrebleFilter);
	WriteAppProfileInt("Sound", "Treble filter damping", Sound.iTrebleDamping);
	WriteAppProfileInt("Sound", "Volume",				 Sound.iMixVolume);
	WriteAppProfileString("Sound", "Device",			 strDevice);

	// Midi
	WriteAppProfileInt("MIDI", "Device",		   Midi.iMidiDevice);
	WriteAppProfileInt("MIDI", "Out Device",	   Midi.iMidiOutDevice);
	WriteAppProfileInt("MIDI", "Master sync",	   Midi.bMidiMasterSync);
	WriteAppProfileInt("MIDI", "Key release",	   Midi.bMidiKeyRelease);
	WriteAppProfileInt("MIDI", "Channel map",	   Midi.bMidiChannelMap);
	WriteAppProfileInt("MIDI", "Velocity control", Midi.bMidiVelocity);
	WriteAppProfileInt("MIDI", "Auto Arpeggio",	   Midi.bMidiArpeggio);

	// Appearance
	WriteAppProfileInt("Appearance", "Background",					Appearance.iColBackground);
	WriteAppProfileInt("Appearance", "Background highlighted",		Appearance.iColBackgroundHilite);
	WriteAppProfileInt("Appearance", "Background highlighted 2",	Appearance.iColBackgroundHilite2);
	WriteAppProfileInt("Appearance", "Pattern text",				Appearance.iColPatternText);
	WriteAppProfileInt("Appearance", "Pattern text highlighted",	Appearance.iColPatternTextHilite);
	WriteAppProfileInt("Appearance", "Pattern text highlighted 2",	Appearance.iColPatternTextHilite2);
	WriteAppProfileInt("Appearance", "Pattern instrument",			Appearance.iColPatternInstrument);
	WriteAppProfileInt("Appearance", "Pattern volume",				Appearance.iColPatternVolume);
	WriteAppProfileInt("Appearance", "Pattern effect",				Appearance.iColPatternEffect);
	WriteAppProfileInt("Appearance", "Selection",					Appearance.iColSelection);
	WriteAppProfileInt("Appearance", "Cursor",						Appearance.iColCursor);

	// Window position
	WriteAppProfileInt("Window position", "Left",	WindowPos.iLeft);
	WriteAppProfileInt("Window position", "Top",	WindowPos.iTop);
	WriteAppProfileInt("Window position", "Right",	WindowPos.iRight);
	WriteAppProfileInt("Window position", "Bottom",	WindowPos.iBottom);
	WriteAppProfileInt("Window position", "State",	WindowPos.iState);

	// Other
	WriteAppProfileInt("Other", "Sample window state", SampleWinState);

	// Paths
	WriteAppProfileString("Paths", "FTM path", Paths[PATH_FTM]);
	WriteAppProfileString("Paths", "FTI path", Paths[PATH_FTI]);
	WriteAppProfileString("Paths", "NSF path", Paths[PATH_NSF]);
	WriteAppProfileString("Paths", "DMC path", Paths[PATH_DMC]);
	WriteAppProfileString("Paths", "WAV path", Paths[PATH_WAV]);
}

void CSettings::DefaultSettings()
{
	// General
	General.bWrapCursor			= 1;
	General.bWrapFrames			= 0;
	General.bFreeCursorEdit		= 0;
	General.bWavePreview		= 1;
	General.bKeyRepeat			= 0;
	General.bRowInHex			= 0;
	General.strFont				= "Fixedsys";
	General.iEditStyle			= EDIT_STYLE1;
	General.bFramePreview		= true;
	General.bNoDPCMReset		= false;
	General.bNoStepMove			= false;
	General.iPageStepSize		= 4;
	General.bPatternColor		= true;
	General.bPullUpDelete		= false;
	General.bBackups			= true;

	// Keys
	Keys.iKeyNoteCut		= 0xE2;	// '<'
	Keys.iKeyNoteRelease	= 0xDC;	// ''
	Keys.iKeyClear			= 0xBD;	// '-'
	Keys.iKeyRepeat			= 0x00;	// ?

	// Sound
	strDevice				= "";
	Sound.iSampleRate		= 44100;
	Sound.iSampleSize		= 16;
	Sound.iBufferLength		= 50;
	Sound.iBassFilter		= 16;
	Sound.iTrebleFilter		= 12000;
	Sound.iTrebleDamping	= 3;
	Sound.iMixVolume		= 100;

	// Midi
	Midi.iMidiDevice		= 0;
	Midi.iMidiOutDevice		= 0;
	Midi.bMidiMasterSync	= 0;
	Midi.bMidiKeyRelease	= 0;
	Midi.bMidiChannelMap	= 0;
	Midi.bMidiArpeggio		= 0;

	// Appearance
	Appearance.iColBackground		 = COLOR_SCHEME.BACKGROUND;
	Appearance.iColBackgroundHilite	 = COLOR_SCHEME.BACKGROUND_HILITE;
	Appearance.iColBackgroundHilite	 = COLOR_SCHEME.BACKGROUND_HILITE2;
	Appearance.iColPatternText		 = COLOR_SCHEME.TEXT_NORMAL;
	Appearance.iColPatternTextHilite = COLOR_SCHEME.TEXT_HILITE;
	Appearance.iColPatternTextHilite = COLOR_SCHEME.TEXT_HILITE2;
	Appearance.iColPatternInstrument = COLOR_SCHEME.TEXT_INSTRUMENT;
	Appearance.iColPatternVolume	 = COLOR_SCHEME.TEXT_VOLUME;
	Appearance.iColPatternEffect	 = COLOR_SCHEME.TEXT_EFFECT;
	Appearance.iColSelection		 = COLOR_SCHEME.SELECTION;
	Appearance.iColCursor			 = COLOR_SCHEME.CURSOR;

	// Window position
	WindowPos.iLeft		= 100;
	WindowPos.iTop		= 100;
	WindowPos.iRight	= 950;
	WindowPos.iBottom	= 920;
	WindowPos.iState	= STATE_NORMAL;

	// Other
	SampleWinState = 0;

	for (int i = 0; i < PATH_COUNT; i++)
		Paths[i] = "";
}

void CSettings::SetWindowPos(int Left, int Top, int Right, int Bottom, int State)
{
	WindowPos.iLeft	  = Left;
	WindowPos.iTop	  = Top;
	WindowPos.iRight  = Right;
	WindowPos.iBottom = Bottom;
	WindowPos.iState  = State;
}

CString CSettings::GetPath(unsigned int PathType)
{
	ASSERT(PathType < PATH_COUNT);
	return Paths[PathType];
}

void CSettings::SetPath(CString PathName, unsigned int PathType)
{
	ASSERT(PathType < PATH_COUNT);

	// Remove file name if there is a
	if (PathName.Right(1) == "\\" || PathName.Find('\\') == -1)
		Paths[PathType] = PathName;
	else
		Paths[PathType] = PathName.Left(PathName.ReverseFind('\\'));
}

void CSettings::StoreSetting(CString Section, CString Name, int Value)
{
	WriteAppProfileInt(Section, Name, Value);
}

int CSettings::LoadSetting(CString Section, CString Name)
{
	return GetAppProfileInt(Section, Name, 0);
}
