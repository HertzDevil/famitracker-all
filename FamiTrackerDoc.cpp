/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005  Jonathan Liss
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

#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "DocumentFile.h"
#include "compile.h"
#include "driver.h"
#include ".\famitrackerdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const char	*FILE_HEADER	= "FamiTracker Module";
const char	*FILE_ENDER		= "END";
const int	FILE_VER		= 0x0200;				// Current file version (1.01)
const int	COMPATIBLE_VER	= 0x0100;				// Compatible file version (1.0)

const char *FILE_BLOCK_PARAMS			= "PARAMS";
const char *FILE_BLOCK_INFO				= "INFO";
const char *FILE_BLOCK_INSTRUMENTS		= "INSTRUMENTS";
const char *FILE_BLOCK_SEQUENCES		= "SEQUENCES";
const char *FILE_BLOCK_FRAMES			= "FRAMES";
const char *FILE_BLOCK_PATTERNS			= "PATTERNS";
const char *FILE_BLOCK_DSAMPLES			= "DPCM SAMPLES";

struct stInstrumentImport {
	char	Name[256];
	bool	Free;
	int		ModEnable[MOD_COUNT];
	int		ModIndex[MOD_COUNT];
	int		AssignedSample;				// For DPCM
};

enum eFileBlock {
	FB_INSTRUMENTS,
	FB_SEQUENCES,
	FB_PATTERN_ROWS,
	FB_PATTERNS,
	FB_SPEED,
	FB_CHANNELS,
	FB_DSAMPLES,
	FB_EOF,
	FB_MACHINE,
	FB_ENGINESPEED,
	FB_SONGNAME,
	FB_SONGARTIST,
	FB_SONGCOPYRIGHT
};


// CFamiTrackerDoc

IMPLEMENT_DYNCREATE(CFamiTrackerDoc, CDocument)

BEGIN_MESSAGE_MAP(CFamiTrackerDoc, CDocument)
END_MESSAGE_MAP()


// CFamiTrackerDoc construction/destruction

CFamiTrackerDoc::CFamiTrackerDoc()
{
	// TODO: add one-time construction code here
	int i;

	m_bFileLoaded = false;

	for (i = 0; i < MAX_DSAMPLES; i++)
		DSamples[i].SampleSize = 0;
}

CFamiTrackerDoc::~CFamiTrackerDoc()
{
}

BOOL CFamiTrackerDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// Remove allocated memory
	CleanDocument();

	// Document is avaliable
	m_bFileLoaded = true;

	return TRUE;
}

void CFamiTrackerDoc::CleanDocument()
{
	int i, c, x;

	// Remove allocated memory

	for (i = 0; i < MAX_DSAMPLES; i++) {
		if (DSamples[i].SampleSize != 0) {
			DSamples[i].SampleSize = 0;
			delete [] DSamples[i].SampleData;
		}
	}

	// Reset variables

	m_iPatternLength		= MAX_PATTERN_LENGTH;
	m_iFrameCount			= 1;
	m_iChannelsAvaliable	= 5;
	
	m_iMachine				= NTSC;
	m_iEngineSpeed			= 0;

	memset(m_strName, 0, 32);
	memset(m_strArtist, 0, 32);
	memset(m_strCopyright, 0, 32);

	m_iSongSpeed = 6;

	for (c = 0; c < m_iChannelsAvaliable; c++) {
		for (i = 0; i < MAX_FRAMES; i++) {
			FrameList[i][c] = 0;
		}
	}

	for (x = 0; x < m_iChannelsAvaliable; x++) {
		for (c = 0; c < MAX_PATTERN; c++) {
			for (i = 0; i < m_iPatternLength; i++) {
				ChannelData[x][c][i].Note			= 0;
				ChannelData[x][c][i].Octave			= 0;
				ChannelData[x][c][i].Instrument		= 0;
				ChannelData[x][c][i].Vol			= 0;
				ChannelData[x][c][i].ExtraStuff1	= 0;
				ChannelData[x][c][i].ExtraStuff2	= 0;
			}
		}
	}

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		memset(Instruments + i, 0, sizeof(stInstrument));
		Instruments[i].Free = true;
	}

	for (i = 0; i < MAX_SEQUENCES; i++) {
		Sequences[i].Count = 0;
	}

	for (i = 0; i < MAX_DSAMPLES; i++) {
		DSamples[i].SampleSize = 0;
	}

	CMainFrame	*MainFrame;
	POSITION	Pos = GetFirstViewPosition();

	MainFrame = (CMainFrame*)GetNextView(Pos)->GetParentFrame();

	MainFrame->ClearInstrumentList();

	m_bFileLoaded = false;

//	m_bFileLoaded = true;
}

// CFamiTrackerDoc serialization

void CFamiTrackerDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CFamiTrackerDoc diagnostics

#ifdef _DEBUG
void CFamiTrackerDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CFamiTrackerDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CFamiTrackerDoc commands

#define GET_FRAME_ITEM(Frame, Channel)			FrameList[Frame][Channel]
#define GET_PATTERN_ITEM(Frame, Channel, Row)	ChannelData[Channel][GET_FRAME_ITEM(Frame, Channel)][Row]

void CFamiTrackerDoc::IncreaseInstrument(int Frame, int Channel, int Row)
{
	int Instrument = ChannelData[Channel][GET_FRAME_ITEM(Frame, Channel)][Row].Instrument + 1;

	if (Instrument > (MAX_INSTRUMENTS - 1))
		Instrument = (MAX_INSTRUMENTS - 1);

	ChannelData[Channel][GET_FRAME_ITEM(Frame, Channel)][Row].Instrument = Instrument;
}

void CFamiTrackerDoc::DecreaseInstrument(int Frame, int Channel, int Row)
{
	int Instrument = ChannelData[Channel][GET_FRAME_ITEM(Frame, Channel)][Row].Instrument - 1;

	if (Instrument < 0)
		Instrument = 0;

	ChannelData[Channel][GET_FRAME_ITEM(Frame, Channel)][Row].Instrument = Instrument;
}

void CFamiTrackerDoc::IncreaseVolume(int Frame, int Channel, int Row)
{
	int Vol = ChannelData[Channel][FrameList[Frame][Channel]][Row].Vol + 1;

	if (Vol > 0x10)
		Vol = 0x10;

	ChannelData[Channel][FrameList[Frame][Channel]][Row].Vol = Vol;
}

void CFamiTrackerDoc::DecreaseVolume(int Frame, int Channel, int Row)
{
	int Vol = ChannelData[Channel][FrameList[Frame][Channel]][Row].Vol - 1;

	if (Vol < 1)
		Vol = 1;

	ChannelData[Channel][FrameList[Frame][Channel]][Row].Vol = Vol;
}

void CFamiTrackerDoc::IncreaseEffect(int Frame, int Channel, int Row)
{
	int Effect = ChannelData[Channel][FrameList[Frame][Channel]][Row].ExtraStuff2 + 1;
	
	/*
	if (Effect > 99)
		Effect = 99;
		*/

	if (Effect > 255)
		Effect = 255;

	ChannelData[Channel][FrameList[Frame][Channel]][Row].ExtraStuff2 = Effect;
}

void CFamiTrackerDoc::DecreaseEffect(int Frame, int Channel, int Row)
{
	int Effect = ChannelData[Channel][FrameList[Frame][Channel]][Row].ExtraStuff2 - 1;
	
	if (Effect < 0)
		Effect = 0;

	ChannelData[Channel][FrameList[Frame][Channel]][Row].ExtraStuff2 = Effect;
}

bool CFamiTrackerDoc::DeleteNote(int RowPos, int Frame, int Channel, int Column)
{
	stChanNote Note;

	Note = ChannelData[Channel][FrameList[Frame][Channel]][RowPos];

	if (Column == 0) {
		Note.Instrument = 0;
		Note.Octave = 0;
		Note.Note = 0;
		Note.Vol = 0;
	}
	else if (Column == 1) {
		Note.Instrument = MAX_INSTRUMENTS;
	}
	else if (Column == 2) {
		Note.Vol = 0;
	}
	else if (Column == 3 || Column == 4) {
		Note.ExtraStuff1 = 0;
		Note.ExtraStuff2 = 0;
	}
	
	ChannelData[Channel][FrameList[Frame][Channel]][RowPos] = Note;

	SetModifiedFlag();

	return true;
}

bool CFamiTrackerDoc::InsertNote(int RowPos, int PatternPos, int Channel)
{
	stChanNote Note;
	int i;

	Note.ExtraStuff1	= 0;
	Note.ExtraStuff2	= 0;
	Note.Instrument		= 0;
	Note.Octave			= 0;
	Note.Note			= 0;
	Note.Vol			= 0;

	for (i = m_iPatternLength - 2; i >= RowPos; i--) {
		ChannelData[Channel][FrameList[PatternPos][Channel]][i + 1] = ChannelData[Channel][FrameList[PatternPos][Channel]][i];
	}

	ChannelData[Channel][FrameList[PatternPos][Channel]][RowPos] = Note;
	SetModifiedFlag();

	return true;
}

bool CFamiTrackerDoc::RemoveNote(int RowPos, int PatternPos, int Channel)
{
	stChanNote Note;
	int i;

	Note.ExtraStuff1	= 0;
	Note.ExtraStuff2	= 0;
	Note.Instrument		= 0;
	Note.Octave			= 0;
	Note.Note			= 0;
	Note.Vol			= 0;

	for (i = RowPos - 1; i < (m_iPatternLength - 1); i++) {
		ChannelData[Channel][FrameList[PatternPos][Channel]][i] = ChannelData[Channel][FrameList[PatternPos][Channel]][i + 1];
	}

	ChannelData[Channel][FrameList[PatternPos][Channel]][m_iPatternLength - 1] = Note;

	SetModifiedFlag();

	return true;
}

void CFamiTrackerDoc::PlayNote(int Channel, int RowPos, int Frame)
{
	theApp.PlayNote(Channel, &ChannelData[Channel][FrameList[Frame][Channel]][RowPos]);
}

void CFamiTrackerDoc::GetNoteData(stChanNote *Data, int Channel, int RowPos, int Frame)
{
	Data->ExtraStuff1	= ChannelData[Channel][FrameList[Frame][Channel]][RowPos].ExtraStuff1;
	Data->ExtraStuff2	= ChannelData[Channel][FrameList[Frame][Channel]][RowPos].ExtraStuff2;
	Data->Instrument	= ChannelData[Channel][FrameList[Frame][Channel]][RowPos].Instrument;
	Data->Note			= ChannelData[Channel][FrameList[Frame][Channel]][RowPos].Note;
	Data->Octave		= ChannelData[Channel][FrameList[Frame][Channel]][RowPos].Octave;
	Data->Vol			= ChannelData[Channel][FrameList[Frame][Channel]][RowPos].Vol;
}

int CFamiTrackerDoc::AddInstrument(const char *Name)
{
	CMainFrame	*MainFrame;
	POSITION	Pos = GetFirstViewPosition();

	int i, c;

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (Instruments[i].Free)
			break;
	}

	if (i == MAX_INSTRUMENTS) {
		theApp.GetMainWnd()->MessageBox("You cannot add more instruments, maximum number of allowed instuments are reached.");
		return -1;
	}

	strcpy(Instruments[i].Name, Name);
	Instruments[i].Free = false;
	
	for (c = 0; c < MOD_COUNT; c++) {
		Instruments[i].ModEnable[c] = 0;
		Instruments[i].ModIndex[c] = 0;
	}
	
	MainFrame = (CMainFrame*)GetNextView(Pos)->GetParentFrame();

	return i;
}

void CFamiTrackerDoc::RemoveInstrument(int Index)
{
	POSITION	Pos = GetFirstViewPosition();
	CMainFrame	*MainFrame = (CMainFrame*)GetNextView(Pos)->GetParentFrame();
	
	if (Instruments[Index].Free == true)
		return;

	memset(Instruments + Index, 0, sizeof(stInstrument));

	Instruments[Index].Free = true;
}

void CFamiTrackerDoc::GetInstName(int Instrument, char *Name)
{
	strcpy(Name, Instruments[Instrument].Name);
}

void CFamiTrackerDoc::InstrumentName(int Index, char *Name)
{
	if (Instruments[Index].Free == true)
		return;

	strcpy(Instruments[Index].Name, Name);
}

void CFamiTrackerDoc::InsertModifierItem(int Index, int SelectedSequence)
{
	POSITION	Pos = GetFirstViewPosition();
	CMainFrame	*MainFrame = (CMainFrame*)GetNextView(Pos)->GetParentFrame();

	if (Index < Sequences[SelectedSequence].Count) {

		for (int i = Sequences[SelectedSequence].Count; i > (Index+1); i--) {
			Sequences[SelectedSequence].Length[i] = Sequences[SelectedSequence].Length[i - 1];
			Sequences[SelectedSequence].Value[i] = Sequences[SelectedSequence].Value[i - 1];
		}
		Index++;
	}

	Sequences[SelectedSequence].Length[Index] = 0;
	Sequences[SelectedSequence].Value[Index] = 0;

	Sequences[SelectedSequence].Count++;
}

void CFamiTrackerDoc::RemoveModifierItem(int Index, int SelectedSequence)
{
	POSITION	Pos = GetFirstViewPosition();
	CMainFrame	*MainFrame = (CMainFrame*)GetNextView(Pos)->GetParentFrame();

	if (Index < Sequences[SelectedSequence].Count) {

		for (int i = Index; i < Sequences[SelectedSequence].Count; i++) {
			Sequences[SelectedSequence].Length[i] = Sequences[SelectedSequence].Length[i + 1];
			Sequences[SelectedSequence].Value[i] = Sequences[SelectedSequence].Value[i + 1];
		}
	}

	Sequences[SelectedSequence].Count--;
}

BOOL CFamiTrackerDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	return SaveDocument(lpszPathName);
}

BOOL CFamiTrackerDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

	CFile	OpenFile;
	char	Text[256];
	int		i, c, ReadCount, FileBlock;
	int		Version;

	CMainFrame	*MainFrame;
	POSITION	Pos = GetFirstViewPosition();

	m_bFileLoaded = false;

	MainFrame = (CMainFrame*)GetNextView(Pos)->GetParentFrame();

	if (!OpenFile.Open(lpszPathName, CFile::modeRead)) {
		theApp.GetMainWnd()->MessageBox("Could not open file");
		CleanDocument();
		return FALSE;
	}

	// Read header to validate file

	OpenFile.Read(Text, int(strlen(FILE_HEADER)));

	if (memcmp(Text, FILE_HEADER, strlen(FILE_HEADER)) != 0) {
		theApp.GetMainWnd()->MessageBox("File is not valid");
		return FALSE;
	}

	OpenFile.Read(&Version, sizeof(int));

	if (Version == FILE_VER) {
		OpenFile.Close();
		return OpenDocument(lpszPathName);
	}

	if (Version == COMPATIBLE_VER) {
		// Skipped this dialog in the release
		//MessageBox(0, "This song was created with an older version of FamiTracker, it may not sound the same as before.\n\n(Save the file to remove this dialog)", "Note", 0);
	}
	else if (Version != (COMPATIBLE_VER + 1)) {
		MessageBox(0, "File version is not supported!", "Error", 0);
		return FALSE;
	}

	CleanDocument();

	FileBlock = 0;

	stInstrumentImport ImportedInstruments;

	while (FileBlock != FB_EOF) {
		OpenFile.Read(&FileBlock, sizeof(int));

		switch (FileBlock) {
			case FB_CHANNELS:
				OpenFile.Read(&m_iChannelsAvaliable, sizeof(int));
				break;

			case FB_SPEED:
				OpenFile.Read(&m_iSongSpeed, sizeof(int));
				break;

			case FB_MACHINE:
				OpenFile.Read(&m_iMachine, sizeof(int));				
				break;

			case FB_ENGINESPEED:
				OpenFile.Read(&m_iEngineSpeed, sizeof(int));
				break;

			case FB_INSTRUMENTS:
				OpenFile.Read(&ReadCount, sizeof(int));
				for (i = 0; i < ReadCount; i++) {
					OpenFile.Read(&ImportedInstruments, sizeof(stInstrumentImport));
					Instruments[i].Free = ImportedInstruments.Free;
					memcpy(&Instruments[i].ModEnable, &ImportedInstruments.ModEnable, 4 * MOD_COUNT);
					memcpy(&Instruments[i].ModIndex, &ImportedInstruments.ModIndex, 4 * MOD_COUNT);
					strcpy(Instruments[i].Name, ImportedInstruments.Name);
					memset(Instruments[i].SamplePitch, 0, 5 * 11);
					memset(Instruments[i].Samples, 0, 5 * 11);
					if (ImportedInstruments.AssignedSample > 0) {
						int Pitch = 0;
						for (int y = 0; y < 6; y++) {
							for (int x = 0; x < 12; x++) {
								Instruments[i].Samples[y][x] = ImportedInstruments.AssignedSample;
								Instruments[i].SamplePitch[y][x] = Pitch;
								Pitch = (Pitch + 1) % 16;
							}
						}
					}
				}
				break;

			case FB_SEQUENCES:
				OpenFile.Read(&ReadCount, sizeof(int));
				for (i = 0; i < ReadCount; i++)
					OpenFile.Read(&Sequences[i], sizeof(stSequence));
				break;

			case FB_PATTERN_ROWS:
				OpenFile.Read(&m_iFrameCount, sizeof(int));
				for (c = 0; c < m_iFrameCount; c++) {
					for (i = 0; i < m_iChannelsAvaliable; i++)
						OpenFile.Read(&FrameList[c][i], sizeof(int));
				}
				break;

			case FB_PATTERNS:
				OpenFile.Read(&ReadCount, sizeof(int));
				OpenFile.Read(&m_iPatternLength, sizeof(int));
				for (int x = 0; x < m_iChannelsAvaliable; x++) {
					for (c = 0; c < ReadCount; c++) {
						for (i = 0; i < m_iPatternLength; i++) {
							OpenFile.Read(&ChannelData[x][c][i], sizeof(stChanNote));
							ChannelData[x][c][i].Vol = 0;
						}
					}
				}
				break;

			case FB_DSAMPLES:
				OpenFile.Read(&ReadCount, sizeof(int));
				for (i = 0; i < ReadCount; i++) {
					OpenFile.Read(&DSamples[i], sizeof(stDSample));
					if (DSamples[i].SampleSize != 0) {
						DSamples[i].SampleData = new char[DSamples[i].SampleSize];
						OpenFile.Read(DSamples[i].SampleData, DSamples[i].SampleSize);
					}
				}
				break;

			case FB_SONGNAME:
				OpenFile.Read(m_strName, sizeof(char) * 32);
				break;

			case FB_SONGARTIST:
				OpenFile.Read(m_strArtist, sizeof(char) * 32);
				break;
		
			case FB_SONGCOPYRIGHT:
				OpenFile.Read(m_strCopyright, sizeof(char) * 32);
				break;

		}
	}

	// File is loaded

	OpenFile.Close();

	// Create a backup of this file, since it's an old version
	char BackupFile[MAX_PATH];
	strcpy(BackupFile, lpszPathName);
	strcat(BackupFile, ".bak");
	CopyFile(lpszPathName, BackupFile, TRUE);

	m_bFileLoaded = true;

	return TRUE;
}

/*** File format description ***

0000: "FamiTracker Module"					id string
000x: Version								int, version number
000x: Start of blocks

 {FILE_BLOCK_PARAMS, 1}
  Song speed								int
  Channels									int
  Machine type								int
  Engine speed								int

 {FILE_BLOCK_INFO, 1}
  Song name									string, 32 bytes
  Artist name								string, 32 bytes
  Copyright									string, 32 bytes

000x: End of blocks
000x: "END"						End of file

********************************/

void CFamiTrackerDoc::WriteBlock_Instruments(CDocumentFile *pDocFile)
{
	int i, x, j, k;
	int Count = 0;

	// Instruments
	pDocFile->CreateBlock(FILE_BLOCK_INSTRUMENTS, 1);

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (!Instruments[i].Free)
			Count++;
	}

	pDocFile->WriteBlockInt(Count);

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		// Only write instrument if it's used
		if (Instruments[i].Free == false) {
			pDocFile->WriteBlockInt(i);
			pDocFile->WriteBlockChar(1);
//			pDocFile->WriteBlock(&Instruments[i].AssignedSample, sizeof(int));
			
			pDocFile->WriteBlockInt(MOD_COUNT);
			for (x = 0; x < MOD_COUNT; x++) {
				pDocFile->WriteBlockChar(Instruments[i].ModEnable[x]);
				pDocFile->WriteBlockChar(Instruments[i].ModIndex[x]);
			}

			for (j = 0; j < 6; j++) {
				for (k = 0; k < 12; k++) {
					pDocFile->WriteBlockChar(Instruments[i].Samples[j][k]);
					pDocFile->WriteBlockChar(Instruments[i].SamplePitch[j][k]);
				}
			}

			pDocFile->WriteBlockInt((int)strlen(Instruments[i].Name));
			pDocFile->WriteBlock(&Instruments[i].Name, (int)strlen(Instruments[i].Name));
		}
	}

	pDocFile->FlushBlock();
}

void CFamiTrackerDoc::WriteBlock_Sequences(CDocumentFile *pDocFile)
{
	int i, x, Count = 0;

	// Sequences
	pDocFile->CreateBlock(FILE_BLOCK_SEQUENCES, 1);

	for (i = 0; i < MAX_SEQUENCES; i++) {
		if (Sequences[i].Count > 0)
			Count++;
	}

	pDocFile->WriteBlockInt(Count);

	for (i = 0; i < MAX_SEQUENCES; i++) {
		if (Sequences[i].Count > 0) {
			pDocFile->WriteBlockInt(i);
			pDocFile->WriteBlockChar(Sequences[i].Count);
			for (x = 0; x < Sequences[i].Count; x++) {
				pDocFile->WriteBlockChar(Sequences[i].Value[x]);
				pDocFile->WriteBlockChar(Sequences[i].Length[x]);
			}
		}
	}

	pDocFile->FlushBlock();
}

void CFamiTrackerDoc::WriteBlock_Frames(CDocumentFile *pDocFile)
{
	int i, x;

	pDocFile->CreateBlock(FILE_BLOCK_FRAMES, 1);
	pDocFile->WriteBlockInt(m_iFrameCount);
	pDocFile->WriteBlockInt(m_iChannelsAvaliable);

	for (i = 0; i < m_iFrameCount; i++) {
		for (x = 0; x < m_iChannelsAvaliable; x++) {
			pDocFile->WriteBlockChar(FrameList[i][x]);
		}
	}

	pDocFile->FlushBlock();
}

void CFamiTrackerDoc::WriteBlock_Patterns(CDocumentFile *pDocFile)
{
	int i, x, y, Items;

	pDocFile->CreateBlock(FILE_BLOCK_PATTERNS, 1);
	pDocFile->WriteBlockInt(m_iPatternLength);

	for (i = 0; i < m_iChannelsAvaliable; i++) {
		for (x = 0; x < MAX_PATTERN; x++) {
			Items = 0;

			for (y = 0; y < m_iPatternLength; y++) {
				if (ChannelData[i][x][y].Note != 0 || 
					ChannelData[i][x][y].ExtraStuff1 != 0 || 
					ChannelData[i][x][y].Vol != 0)
					Items++;
			}

			if (Items > 0) {
				pDocFile->WriteBlockInt(i);		// Write channel
				pDocFile->WriteBlockInt(x);		// Write pattern
				pDocFile->WriteBlockInt(Items);	// Number of items

				for (y = 0; y < m_iPatternLength; y++) {
					if (ChannelData[i][x][y].Note != 0 || 
						ChannelData[i][x][y].ExtraStuff1 != 0 || 
						ChannelData[i][x][y].Vol != 0) {
						pDocFile->WriteBlockChar(y);
						pDocFile->WriteBlockChar(ChannelData[i][x][y].Note);
						pDocFile->WriteBlockChar(ChannelData[i][x][y].Octave);
						pDocFile->WriteBlockChar(ChannelData[i][x][y].Instrument);
						pDocFile->WriteBlockChar(ChannelData[i][x][y].Vol);
						pDocFile->WriteBlockChar(ChannelData[i][x][y].ExtraStuff1);
						pDocFile->WriteBlockChar(ChannelData[i][x][y].ExtraStuff2);
					}
				}
			}
		}
	}

	pDocFile->FlushBlock();
}

void CFamiTrackerDoc::WriteBlock_DSamples(CDocumentFile *pDocFile)
{
	int Count = 0, i;

	pDocFile->CreateBlock(FILE_BLOCK_DSAMPLES, 1);
	
	for (i = 0; i < MAX_DSAMPLES; i++) {
		if (DSamples[i].SampleSize > 0)
			Count++;
	}

	pDocFile->WriteBlockChar(Count);

	for (i = 0; i < MAX_DSAMPLES; i++) {
		if (DSamples[i].SampleSize > 0) {
			pDocFile->WriteBlockChar(i);
			pDocFile->WriteBlockInt((int)strlen(DSamples[i].Name));
			pDocFile->WriteBlock(DSamples[i].Name, (int)strlen(DSamples[i].Name));
			pDocFile->WriteBlockInt(DSamples[i].SampleSize);
			pDocFile->WriteBlock(DSamples[i].SampleData, DSamples[i].SampleSize);
		}
	}

	pDocFile->FlushBlock();
}


BOOL CFamiTrackerDoc::SaveDocument(LPCTSTR lpszPathName)
{
	CDocumentFile DocumentFile;

	if (!DocumentFile.Open(lpszPathName, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Couldn't open file for saving! Check that it isn't write protected or shared with other applications.");
		return FALSE;
	}

	DocumentFile.BeginDocument();

	// Song parameters
	DocumentFile.CreateBlock(FILE_BLOCK_PARAMS, 1);
	DocumentFile.WriteBlockInt(m_iSongSpeed);
	DocumentFile.WriteBlockInt(m_iChannelsAvaliable);
	DocumentFile.WriteBlockInt(m_iMachine);
	DocumentFile.WriteBlockInt(m_iEngineSpeed);
	DocumentFile.FlushBlock();

	// Song info
	DocumentFile.CreateBlock(FILE_BLOCK_INFO, 1);
	DocumentFile.WriteBlock(m_strName, 32);
	DocumentFile.WriteBlock(m_strArtist, 32);
	DocumentFile.WriteBlock(m_strCopyright, 32);
	DocumentFile.FlushBlock();

	WriteBlock_Instruments(&DocumentFile);
	WriteBlock_Sequences(&DocumentFile);
	WriteBlock_Frames(&DocumentFile);
	WriteBlock_Patterns(&DocumentFile);
	WriteBlock_DSamples(&DocumentFile);

	DocumentFile.EndDocument();

	DocumentFile.Close();

	return TRUE;
}

bool CFamiTrackerDoc::ReadBlock_Instruments(CDocumentFile *pDocFile)
{
	int i, x, y, j, k;
	int Count, SeqCnt, Type;
	int Version = pDocFile->GetBlockVersion();
	
	Count = pDocFile->GetBlockInt();

	for (i = 0; i < Count; i++) {
		x = pDocFile->GetBlockInt();

		if (x > MAX_INSTRUMENTS || x < 0)
			return true;

		Instruments[x].Free = false;

		Type = pDocFile->GetBlockChar();

		//Instruments[x].AssignedSample = pDocFile->GetBlockInt();
		//pDocFile->GetBlockInt();

		SeqCnt = pDocFile->GetBlockInt();

		if (SeqCnt > MOD_COUNT)
			return true;

		for (y = 0; y < SeqCnt; y++) {
			
			Instruments[x].ModEnable[y] = pDocFile->GetBlockChar();
			Instruments[x].ModIndex[y] = pDocFile->GetBlockChar();
			
			/*
			Instruments[x].ModEnable[y] = pDocFile->GetBlockInt();
			Instruments[x].ModIndex[y] = pDocFile->GetBlockInt();
			*/
		}

		for (j = 0; j < 6; j++) {
			for (k = 0; k < 12; k++) {
				Instruments[x].Samples[j][k] = pDocFile->GetBlockChar();
				Instruments[x].SamplePitch[j][k] = pDocFile->GetBlockChar();
			}
		}

		y = pDocFile->GetBlockInt();
		
		if (y > 256)
			return true;

		pDocFile->GetBlock(&Instruments[x].Name, y);
		Instruments[x].Name[y] = 0;
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Sequences(CDocumentFile *pDocFile)
{
	int i, x, Count = 0, SeqCount, Index;
	int Version = pDocFile->GetBlockVersion();

	Count = pDocFile->GetBlockInt();

	for (i = 0; i < Count; i++) {
		Index		= pDocFile->GetBlockInt();
		SeqCount	= pDocFile->GetBlockChar();
		
		Sequences[Index].Count = SeqCount;
		
		for (x = 0; x < SeqCount; x++) {
			Sequences[Index].Value[x] = pDocFile->GetBlockChar();
			Sequences[Index].Length[x] = pDocFile->GetBlockChar();
		}
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Frames(CDocumentFile *pDocFile)
{
	int i, x;
	int Version = pDocFile->GetBlockVersion();

	m_iFrameCount			= pDocFile->GetBlockInt();
	m_iChannelsAvaliable	= pDocFile->GetBlockInt();

	for (i = 0; i < m_iFrameCount; i++) {
		for (x = 0; x < m_iChannelsAvaliable; x++) {
			FrameList[i][x] = pDocFile->GetBlockChar();
		}
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Patterns(CDocumentFile *pDocFile)
{
	unsigned char Item;

	unsigned int Channel, Pattern, i, Items;
	int Version = pDocFile->GetBlockVersion();

	m_iPatternLength = pDocFile->GetBlockInt();

	while (!pDocFile->BlockDone()) {
		Channel = pDocFile->GetBlockInt();
		Pattern = pDocFile->GetBlockInt();
		Items	= pDocFile->GetBlockInt();

		for (i = 0; i < Items; i++) {
			Item = pDocFile->GetBlockChar();

			if (Channel > 5 || Pattern > MAX_PATTERN || Item > MAX_PATTERN_LENGTH)
				return true;

			ChannelData[Channel][Pattern][Item].Note		= pDocFile->GetBlockChar();
			ChannelData[Channel][Pattern][Item].Octave		= pDocFile->GetBlockChar();
			ChannelData[Channel][Pattern][Item].Instrument	= pDocFile->GetBlockChar();
			ChannelData[Channel][Pattern][Item].Vol			= pDocFile->GetBlockChar();
			ChannelData[Channel][Pattern][Item].ExtraStuff1	= pDocFile->GetBlockChar();
			ChannelData[Channel][Pattern][Item].ExtraStuff2	= pDocFile->GetBlockChar();
		}
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_DSamples(CDocumentFile *pDocFile)
{
	int Count = 0, i, Item, Len, Size;
	int Version = pDocFile->GetBlockVersion();

	Count = pDocFile->GetBlockChar();
	
	for (i = 0; i < Count; i++) {
		Item	= pDocFile->GetBlockChar();
		Len		= pDocFile->GetBlockInt();
		pDocFile->GetBlock(DSamples[Item].Name, Len);
		DSamples[Item].Name[Len] = 0;
		Size	= pDocFile->GetBlockInt();
		DSamples[Item].SampleData = new char[Size];
		DSamples[Item].SampleSize = Size;
		pDocFile->GetBlock(DSamples[Item].SampleData, Size);
	}

	return false;
}

BOOL CFamiTrackerDoc::OpenDocument(LPCTSTR lpszPathName)
{
	CDocumentFile DocumentFile;
	char *BlockID;
	bool FileFinished = false;
	bool ErrorFlag = false;

	if (!DocumentFile.Open(lpszPathName, CFile::modeRead)) {
		theApp.GetMainWnd()->MessageBox("Couldn't open file! Check that other applications don't use the file.");
		return FALSE;
	}

	if (!DocumentFile.CheckValidity()) {
		AfxMessageBox("File is not a FamiTracker module (or damaged)!");
		DocumentFile.Close();
		return FALSE;
	}

	// From version 2.0, all files should be compatible (though individual blocks may not)
	if (DocumentFile.GetFileVersion() < 0x0200) {
		AfxMessageBox("File version is not supported!");
		DocumentFile.Close();
		return FALSE;
	}

	// Unload previous file
	CleanDocument();

	// File version checking
	DocumentFile.GetFileVersion();

	// Read all blocks
	while (!DocumentFile.Finished() && !FileFinished && !ErrorFlag) {
		DocumentFile.ReadBlock();
		BlockID = DocumentFile.GetBlockHeaderID();

		if (!strcmp(BlockID, FILE_BLOCK_PARAMS)) {
			m_iSongSpeed			= DocumentFile.GetBlockInt();
			m_iChannelsAvaliable	= DocumentFile.GetBlockInt();
			m_iMachine				= DocumentFile.GetBlockInt();
			m_iEngineSpeed			= DocumentFile.GetBlockInt();
		}
		else if (!strcmp(BlockID, FILE_BLOCK_INFO)) {
			DocumentFile.GetBlock(m_strName, 32);
			DocumentFile.GetBlock(m_strArtist, 32);
			DocumentFile.GetBlock(m_strCopyright, 32);
		}
		else if (!strcmp(BlockID, FILE_BLOCK_INSTRUMENTS)) {
			ErrorFlag = ReadBlock_Instruments(&DocumentFile);
		}
		else if (!strcmp(BlockID, FILE_BLOCK_SEQUENCES)) {
			ErrorFlag = ReadBlock_Sequences(&DocumentFile);
		}
		else if (!strcmp(BlockID, FILE_BLOCK_FRAMES)) {
			ErrorFlag = ReadBlock_Frames(&DocumentFile);
		}
		else if (!strcmp(BlockID, FILE_BLOCK_PATTERNS)) {
			ErrorFlag = ReadBlock_Patterns(&DocumentFile);
		}
		else if (!strcmp(BlockID, FILE_BLOCK_DSAMPLES)) {
			ErrorFlag = ReadBlock_DSamples(&DocumentFile);
		}
		else if (!strcmp(BlockID, "END")) {
			FileFinished = true;
		}
		else {
			// This shouldn't show up
			//AfxMessageBox("Unknown file block!");
		}
	}

	DocumentFile.Close();

	if (ErrorFlag) {
		AfxMessageBox("Couldn't load file properly!");
		CleanDocument();
		return FALSE;
	}

	m_bFileLoaded = true;

	return TRUE;
}

stInstrument *CFamiTrackerDoc::GetInstrument(int Index)
{
	if (Instruments[Index].Free == true)
		return NULL;

	return &Instruments[Index];
}

stDSample *CFamiTrackerDoc::GetDSample(int Index)
{
	return &DSamples[Index];
}

void CFamiTrackerDoc::GetSampleName(int Index, char *Name)
{
	strcpy(Name, DSamples[Index].Name);
}

stSequence *CFamiTrackerDoc::GetModifier(int Index)
{
	return &Sequences[Index];
}

stChanNote	*CFamiTrackerDoc::GetPatternData(int Pattern, int Nr, int Chan)
{
	return &ChannelData[Chan][FrameList[Pattern][Chan]][Nr];
}

bool CFamiTrackerDoc::IncreaseFrame(int PatternPos, int Channel)
{
	if (FrameList[PatternPos][Channel] < (MAX_PATTERN - 1)) {
		FrameList[PatternPos][Channel]++;
		SetModifiedFlag();
		return true;
	}

	return false;
}

bool CFamiTrackerDoc::DecreaseFrame(int PatternPos, int Channel)
{
	if (FrameList[PatternPos][Channel] > 0) {
		FrameList[PatternPos][Channel]--;
		SetModifiedFlag();
		return true;
	}

	return false;
}

void CFamiTrackerDoc::SetPatternCount(int Count)
{
	m_iFrameCount = Count;

	if (m_iFrameCount > MAX_FRAMES)
		m_iFrameCount = MAX_FRAMES;
	else if (m_iFrameCount < 0)
		m_iFrameCount = 0;
}

void CFamiTrackerDoc::SetSongSpeed(int Speed)
{
	m_iSongSpeed = Speed;
}

// DMC Stuff below

stDSample *CFamiTrackerDoc::GetFreeDSample()
{
	int i;

	for (i = 0; i < MAX_DSAMPLES; i++) {
		if (DSamples[i].SampleSize == 0) {
			return &DSamples[i];
		}
	}

	return NULL;
}

void CFamiTrackerDoc::RemoveDSample(int Index)
{
	if (DSamples[Index].SampleSize != 0) {
		delete [] DSamples[Index].SampleData;
		DSamples[Index].SampleSize = 0;
		SetModifiedFlag();
	}
}

void CFamiTrackerDoc::SetRowCount(int Index)
{	
	m_iPatternLength = Index;
}

int CFamiTrackerDoc::GetNote(int Channel, int RowPos, int PatternPos)
{
	return ChannelData[Channel][FrameList[PatternPos][Channel]][RowPos].Note;
}

int	CFamiTrackerDoc::GetNoteEffect(int Channel, int RowPos, int PatternPos)
{
	return ChannelData[Channel][FrameList[PatternPos][Channel]][RowPos].ExtraStuff1;
}

int	CFamiTrackerDoc::GetNoteEffectValue(int Channel, int RowPos, int PatternPos)
{
	return ChannelData[Channel][FrameList[PatternPos][Channel]][RowPos].ExtraStuff2;
}

stChanNote *CFamiTrackerDoc::GetNoteData(int Channel, int RowPos, int PatternPos)
{
	return &ChannelData[Channel][FrameList[PatternPos][Channel]][RowPos];
}

void CFamiTrackerDoc::SetNoteData(int Channel, int RowPos, int PatternPos, stChanNote *Note)
{
	memcpy(&ChannelData[Channel][FrameList[PatternPos][Channel]][RowPos], Note, sizeof(stChanNote));
}

void CFamiTrackerDoc::SetSongInfo(char *Name, char *Artist, char *Copyright)
{
	strcpy(m_strName, Name);
	strcpy(m_strArtist, Artist);
	strcpy(m_strCopyright, Copyright);
}

void CFamiTrackerDoc::WritePatternSymbol(int Pattern, int Channel, int Item)
{/*
	int Octave, Note, AsmNote;
	char *EndLine;

	if (Item == (m_iPatternLength - 1))
		EndLine = "";
	else
		EndLine = ",";

	stChanNote	*pChanData = &ChannelData[Channel][Pattern][Item];

	Note	= pChanData->Note;
	Octave	= pChanData->Octave;

	if (pChanData->Note == 0)
		AsmNote = 0;
	else if (pChanData->Note == HALT)
		AsmNote = 0x7F;
	else {
		if (Channel == 4) { // DPCM


			AsmNote = 0;

		}
		else
			AsmNote = (Note - 1) + (Octave * 12);

		if (pChanData->Instrument != MAX_INSTRUMENTS && pChanData->Instrument != SelectedInstrument) {
			// write instrument change as an command
			SelectedInstrument = pChanData->Instrument;
			Compile.WriteSymbol("$80,%i,", SelectedInstrument);
		}
	}

	switch (pChanData->ExtraStuff1) {
		case EF_SPEED:		Compile.WriteSymbol("$81,%i,", pChanData->ExtraStuff2); break;
		case EF_JUMP:		Compile.WriteSymbol("$82,%i,", pChanData->ExtraStuff2 + 1); break;
		case EF_SKIP:		Compile.WriteSymbol("$83,%i,", pChanData->ExtraStuff2); break;
		case EF_HALT:		Compile.WriteSymbol("$84,%i,", pChanData->ExtraStuff2); break;
		case EF_VOLUME:		Compile.WriteSymbol("$85,%i,", pChanData->ExtraStuff2); break;
		case EF_PORTAON:	Compile.WriteSymbol("$86,%i,", pChanData->ExtraStuff2 + 1); break;
		case EF_PORTAOFF:	Compile.WriteSymbol("$87,%i,", pChanData->ExtraStuff2); break;
		case EF_SWEEPUP: {
			// this need some extra work
			int shift	= pChanData->ExtraStuff2 & 0x07;
			int len		= (pChanData->ExtraStuff2 / 10) & 0x07;
			int Val		= 0x88 | (len << 4) | shift;
			Compile.WriteSymbol("$88,%i,", Val); 
			break;
			}
		case EF_SWEEPDOWN: {
			int shift	= pChanData->ExtraStuff2 & 0x07;
			int len		= (pChanData->ExtraStuff2 / 10) & 0x07;
			int Val		= 0x80 | (len << 4) | shift;
			Compile.WriteSymbol("$88,%i,", Val); 
			break;
			}
	}

//	if ((Item & 15) == 15)									// nesasm doesn't manage too big lines
//		Compile.WriteSymbol("%i\n %s ", AsmNote, SYM_STORE_BYTE);
//	else 
	Compile.WriteSymbol("%i%s", AsmNote, EndLine);*/
}

char *CFamiTrackerDoc::CreateNSF(CString FileName, bool KeepSymbols)
{
	/*
	//
	// Create a NSF from the module
	//
	// KeepSymbols - If true, copy assembly symbols file along with the NSF file
	//
	// Return - a string with some statistics.
	//

	CFile		File;
	char		FilePath[MAX_PATH], SymbolFile[MAX_PATH], SymbolKeepFile[MAX_PATH];
	char		BinFile[MAX_PATH], DriverFile[MAX_PATH];
	char		*CompileInfo;
	int			i, c, p;
	int			InstrumentsUsed = 0, SequencesUsed = 0;
	int			MaxPattern[5];
	int			DPCMPointer = 0x00;
	//int			SelectedInstrument;
	int			OrgPos;
	int			TickPeriod;

	TickPeriod		= m_iSongSpeed;
	CompileInfo		= new char[2048];
	CompileInfo[0]	= 0;

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (Instruments[i].Free == false)
			InstrumentsUsed = i + 1;
	}

	for (i = 0; i < MAX_SEQUENCES; i++) {
		if (Sequences[i].Count > 0)
			SequencesUsed = i + 1;
	}

	for (c = 0; c < m_iChannelsAvaliable; c++) {
		MaxPattern[c] = 0;
	}

	for (i = 0; i < m_iFrameCount; i++) {
		for (c = 0; c < m_iChannelsAvaliable; c++) {
			if (FrameList[i][c] > MaxPattern[c]) {
				MaxPattern[c] = FrameList[i][c] + 1;
			}
		}
	}

	if (InstrumentsUsed == 0 || SequencesUsed == 0) {
		sprintf(CompileInfo, "Error: There must be at least one instrument and sequence list");
		return CompileInfo;
	}

	strcpy(FilePath, GetPathName().GetString());

	for (i = (int)strlen(FilePath); i; i--) {
		if (FilePath[i] == '\\') {
			FilePath[i] = NULL;
			break;
		}
	}

	strcpy(SymbolFile, theApp.m_cAppPath);
	strcat(SymbolFile, "\\songdata.asm");
	strcpy(BinFile, theApp.m_cAppPath);
	strcat(BinFile, "\\songdata.bin");
	strcpy(DriverFile, theApp.m_cAppPath);
	strcat(DriverFile, "\\driver.bin");
	strcpy(SymbolKeepFile, FilePath);
	strcat(SymbolKeepFile, "\\musicdata.asm");
	
	if (!Compile.CreateSymbolFile(SymbolFile)) {
		strcpy(CompileInfo, "Error: Could not create music data file\r\n");
		return CompileInfo;
	}

	Compile.WriteSymbol("; FamiTracker\n; music symbol file : %s\n; \n\n", GetTitle());
	Compile.WriteSymbol(" .org $%04X\n\n", MUSIC_ORIGIN);
	Compile.WriteSymbol(" %s %i ; song speed\n", SYM_STORE_BYTE, TickPeriod + 1);
	Compile.WriteSymbol(" %s %i ; frames\n", SYM_STORE_BYTE, m_iFrameCount);
	Compile.WriteSymbol(" %s %i ; pattern length\n\n", SYM_STORE_BYTE, m_iPatternLength);
	Compile.WriteSymbol(" %s InstPointers ; instruments\n", SYM_STORE_WORD);
	Compile.WriteSymbol(" %s SequencePtrs ; sequences\n", SYM_STORE_WORD);
	Compile.WriteSymbol(" %s Frames ; frames\n", SYM_STORE_WORD);
	Compile.WriteSymbol(" %s DPCM ; dpcm samples\n\n", SYM_STORE_WORD);

	// Store pointers

	Compile.WriteSymbol("\nInstPointers: ; instrument pointers\n %s ", SYM_STORE_WORD);

	for (i = 0; i < InstrumentsUsed - 1; i++)
		Compile.WriteSymbol("inst%i, ", i);

	Compile.WriteSymbol("inst%i\n", i);
	Compile.WriteSymbol("\nSequencePtrs:	; sequence pointers\n %s ", SYM_STORE_WORD);

	for (i = 0; i < SequencesUsed - 1; i++)
		Compile.WriteSymbol("seq%i, ", i, i);

	Compile.WriteSymbol(" seq%i\n", i, i);
	Compile.WriteSymbol("\nFrames:   ; pattern frame pointers\n %s ", SYM_STORE_WORD);

	for (i = 0; i < m_iFrameCount - 1; i++)
		Compile.WriteSymbol("frame%i, ", i); 

	Compile.WriteSymbol("frame%i\n", i); 
	Compile.WriteSymbol("\nDPCM:\n %s ", SYM_STORE_BYTE);

	for (i = 0; i < MAX_DSAMPLES; i++) {
		if (DSamples[i].SampleSize != 0) {
			Compile.WriteSymbol("$%02X,$%02X,", DPCMPointer, DSamples[i].SampleSize / 0x10);
			DPCMPointer += ((DSamples[i].SampleSize / 0x40) + 1);
		}
		else
			Compile.WriteSymbol("$00,$00,");
	}

	Compile.WriteSymbol("$00\n\n");
	Compile.WriteSymbol("; Instruments\n");

	for (i = 0; i < InstrumentsUsed; i++) {
		Compile.WriteSymbol("inst%i: %s ", i, SYM_STORE_BYTE);
		for (c = 0; c < MOD_COUNT - 1; c++) {
			if (Instruments[i].ModEnable[c])
				Compile.WriteSymbol("%i,", (unsigned char)Instruments[i].ModIndex[c] + 1); 
			else
				Compile.WriteSymbol("0,"); 
		}
		if (Instruments[i].ModEnable[c])
			Compile.WriteSymbol("%i", (unsigned char)Instruments[i].ModIndex[c] + 1); 
		else
			Compile.WriteSymbol("0"); 

		Compile.WriteSymbol("\n");
	}

	Compile.WriteSymbol("; DPCM instrument definitions\n");

	for (i = 0; i < InstrumentsUsed; i++) {
		Compile.WriteSymbol("dpcm_inst%i: %s", i, SYM_STORE_BYTE);

		int o, n;

		for (o = 0; o < 6; o++) {
			for (n = 0; n < 12; n++) {
				if (Instruments[i].Samples[o][n] > 0)
					Compile.WriteSymbol("%i, ", (unsigned char)Instruments[i].Samples[o][n]);
			}
		}
	}	

	Compile.WriteSymbol("; Sequences\n");

	for (i = 0; i < SequencesUsed; i++) {

		Compile.WriteSymbol("seq%i: %s %i, ", i, SYM_STORE_BYTE, Sequences[i].Count); 

		int Val;

		for (c = 0; c < Sequences[i].Count; c++) {
			Val = (char)Sequences[i].Value[c];
			if (Val < 0)
				Val = 256 + Val;
			if (Sequences[i].Length[c] >= 0)
				Compile.WriteSymbol("%i,%i,", (char)Sequences[i].Length[c] + 1, Val); 
			else {
				// fix for the nsf player
				int len;
				len = (char)Sequences[i].Length[c] - 1;
				if ((-len) > (char)Sequences[i].Count)
					len = -(char)Sequences[i].Count;
				len = 256 + len;
				Compile.WriteSymbol("%i,%i,", len, Val); 
			}
		}

		Compile.WriteSymbol("0,0\n");	// end of list
	}

	Compile.WriteSymbol("; Frames\n");

	for (i = 0; i < m_iFrameCount; i++) {
		Compile.WriteSymbol("frame%i: %s ", i, SYM_STORE_WORD); 
		for (c = 0; c < m_iChannelsAvaliable - 1; c++)
			Compile.WriteSymbol("pat%ich%i,", FrameList[i][c], c); 

		Compile.WriteSymbol("pat%ich%i\n", FrameList[i][c], c); 
	}

	Compile.WriteSymbol("\n ; pattern data\n");

	for (p = 0; p < MAX_PATTERN; p++) {
		for (c = 0; c < m_iChannelsAvaliable; c++) {

			// Only write pattern if it has been addressed
			if (MaxPattern[c] >= p) {

				Compile.WriteSymbol("; Pattern %i, channel %i\n", p, c); 

				Compile.WriteSymbol("pat%ich%i: %s ", p, c, SYM_STORE_BYTE); 
				SelectedInstrument = MAX_INSTRUMENTS;

				for (i = 0; i < m_iPatternLength; i++) {
					WritePatternSymbol(p, c, i);
				}
			}
		}
	}

	OrgPos = 0xC000;
	int Samples = 0;

	// write dmc samples, they need to be aligned to $40 byte blocks
	for (i = 0; i < MAX_DSAMPLES; i++) {
		if (DSamples[i].SampleSize != 0) {

			Samples++;
			Compile.WriteSymbol(" .org $%04X\n", OrgPos);
			Compile.WriteSymbol("\n %s ", SYM_STORE_BYTE);

			for (c = 0; c < DSamples[i].SampleSize - 1; c++) {
				if ((c & 15) == 15)
					Compile.WriteSymbol("%i\n %s ", (unsigned char)DSamples[i].SampleData[c], SYM_STORE_BYTE);
				else
					Compile.WriteSymbol("%i,", (unsigned char)DSamples[i].SampleData[c]);
			}

			Compile.WriteSymbol("%i\n\n", (unsigned char)DSamples[i].SampleData[c]);
			
			OrgPos += ((DSamples[i].SampleSize / 0x40) + 1) * 0x40;
		}
	}

	Compile.CloseSymbolFile();
	
	if (!Compile.CompileSymbolFile(SymbolFile, BinFile)) {
		strcpy(CompileInfo, "Error: Could not compile music data\r\n");
		return CompileInfo;
	}

	int Speed;

	if (m_iEngineSpeed == 0)
		Speed = 1000000 / (m_iMachine == NTSC ? 60 : 50);
	else
		Speed = 1000000 / m_iEngineSpeed;

	Compile.SetSongInfo(m_strName, m_strArtist, m_strCopyright, Speed, m_iMachine);

	if (!Compile.ApplyDriver(BinFile, FileName)) {
		strcpy(CompileInfo, "Error: Could not apply sound driver\r\n");
		return CompileInfo;
	}

	if (KeepSymbols) {
		CopyFile(SymbolFile, SymbolKeepFile, FALSE);
	}

	DeleteFile(BinFile);
	DeleteFile(SymbolFile);

	sprintf(CompileInfo, "Wrote %s\r\n", FileName);
	sprintf(CompileInfo, "%s * Patterns used: %i, %i, %i, %i, %i\r\n", CompileInfo, MaxPattern[0], MaxPattern[1], MaxPattern[2], MaxPattern[3], MaxPattern[4]);
	sprintf(CompileInfo, "%s * Instruments used: %i\r\n", CompileInfo, InstrumentsUsed);
	sprintf(CompileInfo, "%s * Sequences used: %i\r\n", CompileInfo, SequencesUsed);
	sprintf(CompileInfo, "%s * Frames: %i\r\n", CompileInfo, m_iFrameCount);
	sprintf(CompileInfo, "%s * DPCM samples: %i, allocated size: %i bytes\r\n", CompileInfo, Samples, OrgPos - 0xC000);
	sprintf(CompileInfo, "%s * Music data size: %i bytes\r\n", CompileInfo, Compile.GetMusicSize());
	sprintf(CompileInfo, "%s * NSF speed: %04X\r\n", CompileInfo, Speed);

	if (KeepSymbols)
		sprintf(CompileInfo, "%sMusic data is in file %s\r\n", CompileInfo, SymbolKeepFile);

	if (Compile.ErrorFlag)
		sprintf(CompileInfo, "%s ! Errors encountered, NSF may not work !\r\n", CompileInfo, Compile.GetMusicSize());

	return CompileInfo;*/

	CCompile Compile;

	return Compile.CompileNSF(this, FileName, KeepSymbols);
}

void CFamiTrackerDoc::OnCloseDocument()
{	
	theApp.ShutDownSynth();

	CDocument::OnCloseDocument();
}
