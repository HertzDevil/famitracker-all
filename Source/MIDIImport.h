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



// CMIDIImport command target

class CMIDIImport : public CObject
{
public:
	CMIDIImport();
	virtual ~CMIDIImport();

	bool ImportFile(LPCTSTR FileName);

private:
	bool			ParseTrack();
	unsigned long	ReadVarLen();

	void			ReadEventText();
	void			ReadEventTrackName();
	void			ReadEventMarker();
	void			ReadEventChannelPrefix();
	void			ReadEventPortPrefix();
	void			ReadEventTimeSignature();
	void			ReadEventKeySignature();
	void			ReadEventTempo();
	void			ReadEventSMPTEOffset();

	void			ProcessNote(unsigned char Status, unsigned char Data1, unsigned char Data2, unsigned int DeltaTime);

	char			*TrackData;
	unsigned int	TrackPtr, TrackLen;

	int				m_iPatterns;
	int				m_iCurrentInstrument;

};


