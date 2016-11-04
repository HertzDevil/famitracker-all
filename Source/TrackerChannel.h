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


// CTrackerChannel

#include <afxmt.h>	// For CMutex

enum note_prio_t {
	NOTE_PRIO_0, 
	NOTE_PRIO_1, 
	NOTE_PRIO_2
};

class CTrackerChannel
{
public:
	CTrackerChannel(LPCTSTR pName, const int iChip, const int iID);
	~CTrackerChannel(void);
	LPCTSTR GetChannelName() const;
	const char GetChip() const;
	const int GetID() const;
	const int GetColumnCount() const;
	void SetColumnCount(int Count);

	stChanNote GetNote();
	void SetNote(stChanNote &Note, note_prio_t Priority);
	bool NewNoteData() const;
	void Reset();

	void SetVolumeMeter(int Value);
	int GetVolumeMeter() const;

	void SetPitch(int Pitch);
	int GetPitch() const;

	bool IsInstrumentCompatible(int Instrument, CFamiTrackerDoc *pDoc) const;

private:
	LPCTSTR m_pChannelName;

private:
	int m_iChip;
	int m_iChannelID;
	int m_iColumnCount;

	stChanNote m_Note;
	bool m_bNewNote;
	note_prio_t	m_iNotePriority;

	int m_iVolumeMeter;
	int m_iPitch;

private:
	CCriticalSection m_csNoteLock;
};
