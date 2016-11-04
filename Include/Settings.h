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

// CSettings command target

const int EDIT_STYLE1	= 0;
const int EDIT_STYLE2	= 1;

enum WIN_STATES {
	STATE_NORMAL,
	STATE_MAXIMIZED
};

enum PATHS {
	PATH_FTM,
	PATH_FTI,
	PATH_NSF,
	PATH_DMC,
	PATH_WAV,
	
	PATH_COUNT
};

class CSettings : public CObject
{
public:
	CSettings();
	virtual ~CSettings();

	void LoadSettings();
	void SaveSettings();
	void DefaultSettings();
	void SetWindowPos(int Left, int Top, int Right, int Bottom, int State);

	CString GetPath(unsigned int PathType);
	void	SetPath(CString PathName, unsigned int PathType);

	struct {
		bool	bWrapCursor;
		bool	bFreeCursorEdit;
		bool	bWavePreview;
		bool	bKeyRepeat;
		bool	bRowInHex;
		bool	bFramePreview;
		int		iEditStyle;
		bool	bNoDPCMReset;
		CString	strFont;
	} General;

	CString	strDevice;

	struct {
		int		iSampleRate;
		int		iSampleSize;
		int		iBufferLength;
		int		iBassFilter;
		int		iTrebleFilter;
		int		iTrebleDamping;
	} Sound;

	struct {
		int		iMidiDevice;
		int		iMidiOutDevice;
		bool	bMidiMasterSync;
		bool	bMidiKeyRelease;
		bool	bMidiChannelMap;
		bool	bMidiVelocity;
		bool	bMidiArpeggio;
	} Midi;

	struct {
		int		iColBackground;
		int		iColBackgroundHilite;
		int		iColPatternText;
		int		iColPatternTextHilite;
		int		iColSelection;
		int		iColCursor;
	} Appearance;

	struct {
		int		iLeft;
		int		iTop;
		int		iRight;
		int		iBottom;
		int		iState;
	} WindowPos;

private:
	CString Paths[PATH_COUNT];
};


