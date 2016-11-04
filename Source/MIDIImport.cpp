/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2012  Jonathan Liss
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
#include "MIDIImport.h"
#include "FamiTrackerDoc.h"
#include "MainFrm.h"
#include "MIDI.h"
#include "MIDIImportDialog.h"

/*
 *
 * Refrence used to do midi-importing:
 * http://www.borg.com/~jglatt/tech/midifile.htm
 *
 */

const unsigned int MICROSECONDS_PER_MINUTE = 60000000;

struct MTHD_CHUNK
{
   /* Here's the 8 byte header that all chunks must have */
   char           ID[4];  /* This will be 'M','T','h','d' */
   unsigned long  Length; /* This will be 6 */

   /* Here are the 6 bytes */
   unsigned short Format;
   unsigned short NumTracks;
   unsigned short Division;
};

struct CHUNK_HEADER
{
   char           ID[4];
   unsigned long  Length; 
};

struct MTRK_CHUNK
{
   /* Here's the 8 byte header that all chunks must have */
   char           ID[4];   /* This will be 'M','T','r','k' */
   unsigned long  Length;  /* This will be the actual size of Data[] */

   /* Here are the data bytes */
//   unsigned char  Data[];  /* Its actual size is Data[Length] */
};

CFamiTrackerDoc	*pDocument;
CMainFrame		*pMainFrame;

int m_iChannelMap[MAX_CHANNELS];
int m_iTempo;
int m_iPatternLength;

void ByteSwap(char *Buffer, int Size)
{
	char Temp;

	for (int i = 0; i < Size; i += 2) {
		Temp = Buffer[i];
		Buffer[i] = Buffer[i + 1];
		Buffer[i + 1] = Temp;
	}
}

unsigned int IntSwap(unsigned int Value)
{
	return ((Value & 0xFF) << 24) | ((Value & 0xFF00) << 8) | ((Value & 0xFF0000) >> 8) | ((Value & 0xFF000000) >> 24);
}

unsigned int TimeTracker = 0;
unsigned int TrackNumber = 0;
unsigned int StartOffset = 0xFFFFFFFF;

// CMIDIImport

CMIDIImport::CMIDIImport()
{
}

CMIDIImport::~CMIDIImport()
{
}


// CMIDIImport member functions

bool CMIDIImport::ImportFile(LPCTSTR FileName)
{
	CHUNK_HEADER	Chunk;
	CFile			File;
	int				i;
	
	CMIDIImportDialog ImportDialog;

	pDocument = (CFamiTrackerDoc*)theApp.GetActiveDocument();
	pMainFrame = (CMainFrame*)theApp.GetMainWnd();

	MTHD_CHUNK MTHD;

	if (!pDocument->OnNewDocument())
		return false;

	m_iPatterns = 1;
	m_iTempo	= 120;

	ImportDialog.DoModal(16);

	m_iChannelMap[0] = ImportDialog.m_iChannelMap[0];
	m_iChannelMap[1] = ImportDialog.m_iChannelMap[1];
	m_iChannelMap[2] = ImportDialog.m_iChannelMap[2];
	m_iChannelMap[3] = ImportDialog.m_iChannelMap[3];
	m_iChannelMap[4] = ImportDialog.m_iChannelMap[4];

	m_iPatternLength = ImportDialog.m_iPatLen;

	pDocument->SetPatternLength(m_iPatternLength);
	pDocument->SetFrameCount(m_iPatterns);

	/*
	pDocument->FrameList[m_iPatterns][0] = m_iPatterns;
	pDocument->FrameList[m_iPatterns][1] = m_iPatterns;
	pDocument->FrameList[m_iPatterns][2] = m_iPatterns;
	pDocument->FrameList[m_iPatterns][3] = m_iPatterns;
	pDocument->FrameList[m_iPatterns][4] = m_iPatterns;*/

	pDocument->SetPatternAtFrame(m_iPatterns, 0, m_iPatterns);
	pDocument->SetPatternAtFrame(m_iPatterns, 1, m_iPatterns);
	pDocument->SetPatternAtFrame(m_iPatterns, 2, m_iPatterns);
	pDocument->SetPatternAtFrame(m_iPatterns, 3, m_iPatterns);
	pDocument->SetPatternAtFrame(m_iPatterns, 4, m_iPatterns);

	File.Open(FileName, CFile::modeRead);

	File.Read(&MTHD, 14);
	ByteSwap((char*)&MTHD + 4, 10);

	for (i = 0; i < MTHD.NumTracks; i++) {
		File.Read(&Chunk, sizeof(CHUNK_HEADER));
		if (Chunk.ID[0] == 'M' && Chunk.ID[1] == 'T' && Chunk.ID[2] == 'r' && Chunk.ID[3] == 'k') {
			TrackLen	= IntSwap(Chunk.Length);
			TrackData	= new char[TrackLen];
			TrackPtr	= 0;
			File.Read(TrackData, TrackLen);
			ParseTrack();
			delete [] TrackData;
		}
	}
	
	if (m_iTempo > 0xFF)
		m_iTempo = 0xFF;

	pDocument->SetSongSpeed(m_iTempo);

	File.Close();
	
	pDocument->SetModifiedFlag();
	pDocument->UpdateAllViews(NULL, UPDATE_ENTIRE);

	pMainFrame->RedrawWindow();

	return true;
}

bool CMIDIImport::ParseTrack()
{
	unsigned int	DeltaTime;
	unsigned char	Event;

	char LastStatus = 0;

	CString Text;

	TimeTracker = 0;
	StartOffset = 0xFFFFFFFF;

	while (TrackPtr < TrackLen) {

		// Delta time is before everything else
		DeltaTime = ReadVarLen();

		TimeTracker += DeltaTime;

		// First byte 
		Event = TrackData[TrackPtr++];

		// 0xFF, MIDI event
		if (Event == 0xFF) {
			// Second byte
			Event = TrackData[TrackPtr++];

			switch (Event) {
				case 0x01: ReadEventText();				break;
				case 0x03: ReadEventTrackName();		break;
				case 0x06: ReadEventMarker();			break;
				case 0x20: ReadEventChannelPrefix();	break;
				case 0x21: ReadEventPortPrefix();		break;
				case 0x58: ReadEventTimeSignature();	break;
				case 0x59: ReadEventKeySignature();		break;
				case 0x51: ReadEventTempo();			break;
				case 0x54: ReadEventSMPTEOffset();		break;
				case 0x02: ReadEventText();				break;
				case 0x2F:
					// End of track
					TrackPtr++;
					break;
				default:
					Text.Format(_T("Unnamned %X"), Event);
					return true;
			}
		}
		// MIDI data
		else {
			unsigned char Status, Data1, Data2;

			//Status	= TrackData[TrackPtr++];
			Status	= Event;
			Data1	= TrackData[TrackPtr++];
			Data2	= TrackData[TrackPtr++];
			
			if ((Status >> 4) < 8) {
				Data2 = Data1;
				Data1 = Status;
				TrackPtr--;
				Status = LastStatus;
			}
			else
				LastStatus = Status;

			switch (Status >> 4) {
				case MIDI_MSG_NOTE_OFF:
					break;
				case MIDI_MSG_NOTE_ON:
					ProcessNote(Status, Data1, Data2, DeltaTime);
					break;
				case MIDI_MSG_AFTER_TOUCH:
					//
					break;
				case MIDI_MSG_PROGRAM_CHANGE:
					TrackPtr--;
					m_iCurrentInstrument = Data1;
					if (m_iCurrentInstrument > (MAX_INSTRUMENTS - 1))
						m_iCurrentInstrument = (MAX_INSTRUMENTS - 1);
					break;
				case MIDI_MSG_CONTROL_CHANGE:
					//
					break;
				case MIDI_MSG_CHANNEL_PRESSURE:
					//
					break;
				case MIDI_MSG_PITCH_WHEEL:
					//
					break;
//				default:
//					Text.Format("Unimplemented MIDI event %X", Status >> 4);
//					AfxMessageBox(Text);
			}
		}
	}

	TrackNumber++;

	return true;
}

unsigned long CMIDIImport::ReadVarLen()
{
    register unsigned long value;
    register unsigned char c;

    if ( (value = TrackData[TrackPtr++]) & 0x80 )
    {
       value &= 0x7F;
       do
       {
         value = (value << 7) + ((c = TrackData[TrackPtr++]) & 0x7F);
       } while (c & 0x80);
    }

    return(value);
}

// Read Sequence/Track Name event
//
void CMIDIImport::ReadEventTrackName()
{
	unsigned int Len, i;
	int Slot;
	char Text[256];

	Len = ReadVarLen();

	for (i = 0; i < Len; i++) {
		Text[i] = TrackData[TrackPtr++];
	}

	Text[i] = 0;

	Slot = pDocument->AddInstrument(Text, INST_2A03);

	if (Slot == -1)
		return;

	pMainFrame->AddInstrument(Slot/*, Text, INST_2A03*/);
}

void CMIDIImport::ReadEventMarker()
{
	unsigned int Len = ReadVarLen();
	TrackPtr += Len;
}

void CMIDIImport::ReadEventChannelPrefix()
{
	unsigned int Len		= TrackData[TrackPtr++];
	unsigned int Channel	= TrackData[TrackPtr++];
}

void CMIDIImport::ReadEventPortPrefix()
{
	unsigned int Len	= TrackData[TrackPtr++];
	unsigned int Port	= TrackData[TrackPtr++];
}

void CMIDIImport::ReadEventTimeSignature()
{
	unsigned int Len = TrackData[TrackPtr++];
	TrackPtr += Len;
}

void CMIDIImport::ReadEventKeySignature()
{
	unsigned int Len = TrackData[TrackPtr++];
	TrackPtr += Len;
}

void CMIDIImport::ReadEventTempo()
{
	unsigned int Len = TrackData[TrackPtr++];
	unsigned int MPQN = (TrackData[TrackPtr]<< 16) + (TrackData[TrackPtr + 1] << 8) + (TrackData[TrackPtr + 2] );

	m_iTempo = MICROSECONDS_PER_MINUTE / MPQN;

	TrackPtr += Len;
}

void CMIDIImport::ReadEventText()
{
	unsigned int Len = TrackData[TrackPtr++], i;
	char Text[256];

	for (i = 0; i < Len; i++)
		Text[i] = TrackData[TrackPtr++];

	Text[i] = 0;
}

void CMIDIImport::ReadEventSMPTEOffset()
{
	unsigned int Len = TrackData[TrackPtr++];
	TrackPtr += Len;
}

void CMIDIImport::ProcessNote(unsigned char Status, unsigned char Data1, unsigned char Data2, unsigned int DeltaTime)
{
	stChanNote Note;

	int Row, Pattern, Channel, i;

	if (StartOffset == 0xFFFFFFFF)
		StartOffset = TimeTracker;

	Row			= ((TimeTracker - StartOffset) / 24) % m_iPatternLength;
	Pattern		= ((TimeTracker - StartOffset) / 24) / m_iPatternLength;
	Channel		= Status & 0x0F;

	if (Pattern > (MAX_PATTERN - 1))
		return;

	if (Pattern > m_iPatterns) {
		m_iPatterns++;
		pDocument->SetFrameCount(m_iPatterns);
		for (i = 0; i < MAX_CHANNELS; i++) {
			if (m_iChannelMap[i] < 16 ) {
				//pDocument->FrameList[m_iPatterns][i] = m_iPatterns;
				pDocument->SetPatternAtFrame(m_iPatterns, i, m_iPatterns);
			}
		}
	}

	Note.EffNumber[0]	= 0;
	Note.EffParam[0]	= 0;
	Note.Instrument		= 0;
	Note.Note			= (Data1 % 12) + 1;
	Note.Octave			= (Data1 / 12) - 2;
	Note.Vol			= 0x10;
	
	for (i = 0; i < MAX_CHANNELS; i++) {
		if (m_iChannelMap[i] == Channel) {
			Note.Instrument = i;
			pDocument->SetNoteData(Pattern, i, Row, &Note);
		}
	}
}
