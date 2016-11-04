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

/*

 Document file version changes

 Ver 2.1
  - Made some additions to support multiple effect columns and prepared for more channels
  - Made some speed adjustments, increase one in speed effects if it's below 20
 Ver 2.0
  - Files are small


*/

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "DocumentFile.h"
#include "Compile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const char	*FILE_HEADER			= "FamiTracker Module";
const char	*FILE_ENDER				= "END";
const int	FILE_VER				= 0x0201;				// Current file version (2.01)
const int	COMPATIBLE_VER			= 0x0100;				// Compatible file version (1.0)

const char *FILE_BLOCK_PARAMS		= "PARAMS";
const char *FILE_BLOCK_INFO			= "INFO";
const char *FILE_BLOCK_INSTRUMENTS	= "INSTRUMENTS";
const char *FILE_BLOCK_SEQUENCES	= "SEQUENCES";
const char *FILE_BLOCK_FRAMES		= "FRAMES";
const char *FILE_BLOCK_PATTERNS		= "PATTERNS";
const char *FILE_BLOCK_DSAMPLES		= "DPCM SAMPLES";
const char *FILE_BLOCK_HEADER		= "HEADER";

struct stInstrumentImport {
	char	Name[256];
	bool	Free;
	int		ModEnable[MOD_COUNT];
	int		ModIndex[MOD_COUNT];
	int		AssignedSample;				// For DPCM
};

struct stSequenceImport {
	signed char Length[MAX_SEQ_ITEMS];
	signed char Value[MAX_SEQ_ITEMS];
	int			Count;
};

struct stDSampleImport {
	char	*SampleData;
	int		SampleSize;
	char	Name[256];
};

struct stChanNoteImport {
	int		Note;
	int		Octave;
	int		Vol;
	int		Instrument;
	int		ExtraStuff1;
	int		ExtraStuff2;
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
	CMainFrame	*MainFrame;
	POSITION	Pos = GetFirstViewPosition();

	int i, c, x;

	// Remove allocated memory
	for (i = 0; i < MAX_DSAMPLES; i++) {
		if (DSamples[i].SampleSize != 0) {
			DSamples[i].SampleSize = 0;
			delete [] DSamples[i].SampleData;
		}
	}

	// Reset variables
	m_iPatternLength		= 64;
	m_iChannelsAvailable	= CHANNELS_DEFAULT;
	m_iFrameCount			= 1;
	m_iSongSpeed			= 150;
	
	m_iMachine				= NTSC;
	m_iEngineSpeed			= 0;

	memset(m_strName, 0, 32);
	memset(m_strArtist, 0, 32);
	memset(m_strCopyright, 0, 32);

	for (c = 0; c < m_iChannelsAvailable; c++) {
		for (i = 0; i < MAX_FRAMES; i++) {
			FrameList[i][c] = 0;
		}
	}

	for (x = 0; x < m_iChannelsAvailable; x++) {
		for (c = 0; c < MAX_PATTERN; c++) {
			for (i = 0; i < MAX_PATTERN_LENGTH; i++) {
				ChannelData[x][c][i].Note		= 0;
				ChannelData[x][c][i].Octave		= 0;
				ChannelData[x][c][i].Instrument	= MAX_INSTRUMENTS;
				ChannelData[x][c][i].Vol		= 0x10;

				for (int n = 0; n < MAX_EFFECT_COLUMNS; n++) {
					ChannelData[x][c][i].EffNumber[n] = 0;
					ChannelData[x][c][i].EffParam[n] = 0;
				}
			}
		}
		m_iEffectColumns[x] = 0;
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

	MainFrame = (CMainFrame*)GetNextView(Pos)->GetParentFrame();

	MainFrame->ClearInstrumentList();

	m_bFileLoaded = false;
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

void CFamiTrackerDoc::IncreaseEffect(int Frame, int Channel, int Row, int Index)
{
	int Effect = ChannelData[Channel][FrameList[Frame][Channel]][Row].EffParam[Index] + 1;
	
	if (Effect > 255)
		Effect = 255;

	ChannelData[Channel][FrameList[Frame][Channel]][Row].EffParam[Index] = Effect;
}

void CFamiTrackerDoc::DecreaseEffect(int Frame, int Channel, int Row, int Index)
{
	int Effect = ChannelData[Channel][FrameList[Frame][Channel]][Row].EffParam[Index] - 1;
	
	if (Effect < 0)
		Effect = 0;

	ChannelData[Channel][FrameList[Frame][Channel]][Row].EffParam[Index] = Effect;
}

bool CFamiTrackerDoc::DeleteNote(int RowPos, int Frame, int Channel, int Column)
{
	stChanNote Note;

	Note = ChannelData[Channel][FrameList[Frame][Channel]][RowPos];

	if (Column == C_NOTE) {
		Note.Note		= 0;
		Note.Octave		= 0;
		Note.Instrument = MAX_INSTRUMENTS;
		Note.Vol		= 0x10;
	}
	else if (Column == C_INSTRUMENT1 || Column == C_INSTRUMENT2) {
		Note.Instrument = MAX_INSTRUMENTS;
	}
	else if (Column == C_VOLUME) {
		Note.Vol = 0x10;
	}
	else if (Column == C_EFF_NUM || Column == C_EFF_PARAM1 || Column == C_EFF_PARAM2) {
		Note.EffNumber[0]	= 0;
		Note.EffParam[0]	= 0;
	}
	else if (Column == C_EFF2_NUM || Column == C_EFF2_PARAM1 || Column == C_EFF2_PARAM2) {
		Note.EffNumber[1]	= 0;
		Note.EffParam[1]	= 0;
	}
	else if (Column == C_EFF3_NUM || Column == C_EFF3_PARAM1 || Column == C_EFF3_PARAM2) {
		Note.EffNumber[2]	= 0;
		Note.EffParam[2]	= 0;
	}
	else if (Column == C_EFF4_NUM || Column == C_EFF4_PARAM1 || Column == C_EFF4_PARAM2) {
		Note.EffNumber[3]	= 0;
		Note.EffParam[3]	= 0;
	}
	
	ChannelData[Channel][FrameList[Frame][Channel]][RowPos] = Note;

	SetModifiedFlag();

	return true;
}

bool CFamiTrackerDoc::InsertNote(int RowPos, int PatternPos, int Channel)
{
	stChanNote Note;
	int i;

	for (int n = 0; n < MAX_EFFECT_COLUMNS; n++) {
		Note.EffNumber[n] = 0;
		Note.EffParam[n] = 0;
	}

	Note.Note			= 0;
	Note.Octave			= 0;
	Note.Instrument		= MAX_INSTRUMENTS;
	Note.Vol			= 0x10;

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

	for (int n = 0; n < MAX_EFFECT_COLUMNS; n++) {
		Note.EffNumber[n] = 0;
		Note.EffParam[n] = 0;
	}

	Note.Note			= 0;
	Note.Octave			= 0;
	Note.Instrument		= MAX_INSTRUMENTS;
	Note.Vol			= 0x10;

	for (i = RowPos - 1; i < (m_iPatternLength - 1); i++) {
		ChannelData[Channel][FrameList[PatternPos][Channel]][i] = ChannelData[Channel][FrameList[PatternPos][Channel]][i + 1];
	}

	ChannelData[Channel][FrameList[PatternPos][Channel]][m_iPatternLength - 1] = Note;

	SetModifiedFlag();

	return true;
}

void CFamiTrackerDoc::GetNoteData(stChanNote *Data, int Channel, int RowPos, int Frame)
{
	for (int n = 0; n < MAX_EFFECT_COLUMNS; n++) {
		Data->EffNumber[n]	= ChannelData[Channel][FrameList[Frame][Channel]][RowPos].EffNumber[n];
		Data->EffParam[n]	= ChannelData[Channel][FrameList[Frame][Channel]][RowPos].EffParam[n];
	}

	Data->Instrument	= ChannelData[Channel][FrameList[Frame][Channel]][RowPos].Instrument;
	Data->Note			= ChannelData[Channel][FrameList[Frame][Channel]][RowPos].Note;
	Data->Octave		= ChannelData[Channel][FrameList[Frame][Channel]][RowPos].Octave;
	Data->Vol			= ChannelData[Channel][FrameList[Frame][Channel]][RowPos].Vol;
}

int CFamiTrackerDoc::AddInstrument(const char *Name)
{
	// Return a slot for the new added instrument
	POSITION Pos = GetFirstViewPosition();

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

void CFamiTrackerDoc::InsertModifierItem(unsigned int Index, unsigned int SelectedSequence)
{
	POSITION	Pos = GetFirstViewPosition();
	CMainFrame	*MainFrame = (CMainFrame*)GetNextView(Pos)->GetParentFrame();

	if (Index < Sequences[SelectedSequence].Count) {
		for (unsigned int i = Sequences[SelectedSequence].Count; i > (Index+1); i--) {
			Sequences[SelectedSequence].Length[i] = Sequences[SelectedSequence].Length[i - 1];
			Sequences[SelectedSequence].Value[i] = Sequences[SelectedSequence].Value[i - 1];
		}
		Index++;
	}

	Sequences[SelectedSequence].Length[Index]	= 0;
	Sequences[SelectedSequence].Value[Index]	= 0;
	Sequences[SelectedSequence].Count++;
}

void CFamiTrackerDoc::RemoveModifierItem(unsigned int Index, unsigned int SelectedSequence)
{
	POSITION	Pos = GetFirstViewPosition();
	CMainFrame	*MainFrame = (CMainFrame*)GetNextView(Pos)->GetParentFrame();

	if (Index < Sequences[SelectedSequence].Count) {
		for (unsigned int i = Index; i < Sequences[SelectedSequence].Count; i++) {
			Sequences[SelectedSequence].Length[i] = Sequences[SelectedSequence].Length[i + 1];
			Sequences[SelectedSequence].Value[i] = Sequences[SelectedSequence].Value[i + 1];
		}
	}

	Sequences[SelectedSequence].Count--;
}

BOOL CFamiTrackerDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	if (!m_bFileLoaded)
		return FALSE;

	return SaveDocument(lpszPathName);
}

BOOL CFamiTrackerDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

	int i, c, ReadCount, FileBlock;

	CFile	OpenFile;
	char	Text[256];
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

	if (Version >= 0x0200) {
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

	stInstrumentImport	ImportedInstruments;
	stSequenceImport	ImportedSequence;
	stDSampleImport		ImportedDSample;
	stChanNoteImport	ImportedNote;

	while (FileBlock != FB_EOF) {
		if (OpenFile.Read(&FileBlock, sizeof(int)) == 0)
			FileBlock = FB_EOF;

		switch (FileBlock) {
			case FB_CHANNELS:
				OpenFile.Read(&m_iChannelsAvailable, sizeof(int));
				break;

			case FB_SPEED:
				OpenFile.Read(&m_iSongSpeed, sizeof(int));
				m_iSongSpeed++;
				break;

			case FB_MACHINE:
				OpenFile.Read(&m_iMachine, sizeof(int));				
				break;

			case FB_ENGINESPEED:
				OpenFile.Read(&m_iEngineSpeed, sizeof(int));
				break;

			case FB_INSTRUMENTS:
				OpenFile.Read(&ReadCount, sizeof(int));
				if (ReadCount > MAX_INSTRUMENTS)
					ReadCount = MAX_INSTRUMENTS - 1;
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
				for (i = 0; i < ReadCount; i++) {
					OpenFile.Read(&ImportedSequence, sizeof(stSequenceImport));
					if (ImportedSequence.Count > 0 && ImportedSequence.Count < MAX_SEQ_ITEMS) {
						Sequences[i].Count = ImportedSequence.Count;
						memcpy(Sequences[i].Length, ImportedSequence.Length, ImportedSequence.Count);
						memcpy(Sequences[i].Value, ImportedSequence.Value, ImportedSequence.Count);
					}
				}
				break;

			case FB_PATTERN_ROWS:
				OpenFile.Read(&m_iFrameCount, sizeof(int));
				for (c = 0; c < m_iFrameCount; c++) {
					for (i = 0; i < m_iChannelsAvailable; i++)
						OpenFile.Read(&FrameList[c][i], sizeof(int));
				}
				break;

			case FB_PATTERNS:
				OpenFile.Read(&ReadCount, sizeof(int));
				OpenFile.Read(&m_iPatternLength, sizeof(int));
				for (int x = 0; x < m_iChannelsAvailable; x++) {
					for (c = 0; c < ReadCount; c++) {
						for (i = 0; i < m_iPatternLength; i++) {
							OpenFile.Read(&ImportedNote, sizeof(stChanNoteImport));
							ChannelData[x][c][i].EffNumber[0]	= ImportedNote.ExtraStuff1;
							ChannelData[x][c][i].EffParam[0]	= ImportedNote.ExtraStuff2;

							ChannelData[x][c][i].Instrument		= ImportedNote.Instrument;
							ChannelData[x][c][i].Note			= ImportedNote.Note;
							ChannelData[x][c][i].Octave			= ImportedNote.Octave;
							ChannelData[x][c][i].Vol			= 0;

							if (ChannelData[x][c][i].Note == 0)
								ChannelData[x][c][i].Instrument = MAX_INSTRUMENTS;
							if (ChannelData[x][c][i].Vol == 0)
								ChannelData[x][c][i].Vol = 0x10;
						}
					}
				}
				break;

			case FB_DSAMPLES:
				OpenFile.Read(&ReadCount, sizeof(int));
				for (i = 0; i < ReadCount; i++) {
					OpenFile.Read(&ImportedDSample, sizeof(stDSampleImport));
					if (ImportedDSample.SampleSize != 0 && ImportedDSample.SampleSize < 0x4000) {
						ImportedDSample.SampleData = new char[ImportedDSample.SampleSize];
						OpenFile.Read(ImportedDSample.SampleData, ImportedDSample.SampleSize);
					}
					strcpy(DSamples[i].Name, ImportedDSample.Name);
					DSamples[i].SampleSize = ImportedDSample.SampleSize;
					DSamples[i].SampleData = ImportedDSample.SampleData;
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
			
			default:
				FileBlock = FB_EOF;
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

void CFamiTrackerDoc::WriteBlock_Header(CDocumentFile *pDocFile)
{
	int i, Count = 0;

	// Header data
	pDocFile->CreateBlock(FILE_BLOCK_HEADER, 1);

	for (i = 0; i < m_iChannelsAvailable; i++) {
		pDocFile->WriteBlockChar(i);						// Channel type
		pDocFile->WriteBlockChar(m_iEffectColumns[i]);		// Effect columns
	}

	pDocFile->FlushBlock();
}

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
	unsigned int i, x, Count = 0;

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
	pDocFile->WriteBlockInt(m_iChannelsAvailable);

	for (i = 0; i < m_iFrameCount; i++) {
		for (x = 0; x < m_iChannelsAvailable; x++) {
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

	for (i = 0; i < m_iChannelsAvailable; i++) {
		for (x = 0; x < MAX_PATTERN; x++) {
			Items = 0;

			for (y = 0; y < m_iPatternLength; y++) {
				if (ChannelData[i][x][y].Note != 0 || 
					ChannelData[i][x][y].EffNumber[0] != 0 ||
					ChannelData[i][x][y].EffNumber[1] != 0 ||
					ChannelData[i][x][y].EffNumber[2] != 0 ||
					ChannelData[i][x][y].EffNumber[3] != 0 ||
					ChannelData[i][x][y].Vol < 0x10 ||
					ChannelData[i][x][y].Instrument != MAX_INSTRUMENTS)
					Items++;
			}

			if (Items > 0) {
				pDocFile->WriteBlockInt(i);		// Write channel
				pDocFile->WriteBlockInt(x);		// Write pattern
				pDocFile->WriteBlockInt(Items);	// Number of items

				for (y = 0; y < m_iPatternLength; y++) {
					if (ChannelData[i][x][y].Note != 0 || 
						ChannelData[i][x][y].EffNumber[0] != 0 ||
						ChannelData[i][x][y].EffNumber[1] != 0 ||
						ChannelData[i][x][y].EffNumber[2] != 0 ||
						ChannelData[i][x][y].EffNumber[3] != 0 ||
						ChannelData[i][x][y].Vol < 0x10 ||
						ChannelData[i][x][y].Instrument != MAX_INSTRUMENTS) {

						//pDocFile->WriteBlockChar(y);
						pDocFile->WriteBlockInt(y);
						pDocFile->WriteBlockChar(ChannelData[i][x][y].Note);
						pDocFile->WriteBlockChar(ChannelData[i][x][y].Octave);
						pDocFile->WriteBlockChar(ChannelData[i][x][y].Instrument);
						pDocFile->WriteBlockChar(ChannelData[i][x][y].Vol);
						/*
						pDocFile->WriteBlockChar(ChannelData[i][x][y].EffNumber[0]);
						pDocFile->WriteBlockChar((unsigned char)ChannelData[i][x][y].EffParam[0]);
						*/

						for (int n = 0; n < signed(m_iEffectColumns[i] + 1); n++) {
							pDocFile->WriteBlockChar(ChannelData[i][x][y].EffNumber[n]);
							pDocFile->WriteBlockChar(ChannelData[i][x][y].EffParam[n]);
						}
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
	DocumentFile.WriteBlockInt(m_iChannelsAvailable);
	DocumentFile.WriteBlockInt(m_iMachine);
	DocumentFile.WriteBlockInt(m_iEngineSpeed);
	DocumentFile.FlushBlock();

	// Song info
	DocumentFile.CreateBlock(FILE_BLOCK_INFO, 1);
	DocumentFile.WriteBlock(m_strName, 32);
	DocumentFile.WriteBlock(m_strArtist, 32);
	DocumentFile.WriteBlock(m_strCopyright, 32);
	DocumentFile.FlushBlock();

	WriteBlock_Header(&DocumentFile);
	WriteBlock_Instruments(&DocumentFile);
	WriteBlock_Sequences(&DocumentFile);
	WriteBlock_Frames(&DocumentFile);
	WriteBlock_Patterns(&DocumentFile);
	WriteBlock_DSamples(&DocumentFile);

	DocumentFile.EndDocument();

	DocumentFile.Close();

	return TRUE;
}

bool CFamiTrackerDoc::ReadBlock_Header(CDocumentFile *pDocFile)
{
	int i;

	for (i = 0; i < m_iChannelsAvailable; i++) {
		pDocFile->GetBlockChar();							// Channel type (unused)
		m_iEffectColumns[i] = pDocFile->GetBlockChar();		// Effect columns
	}

	return false;
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

		Type	= pDocFile->GetBlockChar();
		SeqCnt	= pDocFile->GetBlockInt();

		if (SeqCnt > MOD_COUNT)
			return true;

		for (y = 0; y < SeqCnt; y++) {
			Instruments[x].ModEnable[y] = pDocFile->GetBlockChar();
			Instruments[x].ModIndex[y] = pDocFile->GetBlockChar();			
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
	unsigned int Version = pDocFile->GetBlockVersion();
	int i, x;

	m_iFrameCount			= pDocFile->GetBlockInt();
	m_iChannelsAvailable	= pDocFile->GetBlockInt();

	for (i = 0; i < m_iFrameCount; i++) {
		for (x = 0; x < m_iChannelsAvailable; x++) {
			FrameList[i][x] = pDocFile->GetBlockChar();
		}
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Patterns(CDocumentFile *pDocFile)
{
	unsigned char Item;

	unsigned int Channel, Pattern, i, Items;
	unsigned int Version = pDocFile->GetBlockVersion();

	m_iPatternLength = pDocFile->GetBlockInt();

	while (!pDocFile->BlockDone()) {
		Channel = pDocFile->GetBlockInt();
		Pattern = pDocFile->GetBlockInt();
		Items	= pDocFile->GetBlockInt();

		for (i = 0; i < Items; i++) {
			if (m_iFileVersion == 0x0200)
				Item = pDocFile->GetBlockChar();
			else
				Item = pDocFile->GetBlockInt();

			if (Channel > 5 || Pattern > MAX_PATTERN || Item > MAX_PATTERN_LENGTH)
				return true;

			memset(&ChannelData[Channel][Pattern][Item], 0, sizeof(stChanNote));

			ChannelData[Channel][Pattern][Item].Note			= pDocFile->GetBlockChar();
			ChannelData[Channel][Pattern][Item].Octave			= pDocFile->GetBlockChar();
			ChannelData[Channel][Pattern][Item].Instrument		= pDocFile->GetBlockChar();
			ChannelData[Channel][Pattern][Item].Vol				= pDocFile->GetBlockChar();

			if (m_iFileVersion == 0x0200) {
				ChannelData[Channel][Pattern][Item].EffNumber[0]	= pDocFile->GetBlockChar();
				ChannelData[Channel][Pattern][Item].EffParam[0]		= (unsigned char)pDocFile->GetBlockChar();
			}
			else {
				for (int n = 0; n < signed(m_iEffectColumns[Channel] + 1); n++) {
					ChannelData[Channel][Pattern][Item].EffNumber[n]	= pDocFile->GetBlockChar();
					ChannelData[Channel][Pattern][Item].EffParam[n] 	= (unsigned char)pDocFile->GetBlockChar();
				}
			}
			
			if (ChannelData[Channel][Pattern][Item].Vol > 0x10)
				ChannelData[Channel][Pattern][Item].Vol &= 0x0F;

			if (m_iFileVersion == 0x0200) {
				if (ChannelData[Channel][Pattern][Item].EffNumber[0] == EF_SPEED && ChannelData[Channel][Pattern][Item].EffParam[0] < 20)
					ChannelData[Channel][Pattern][Item].EffParam[0]++;
				
				if (ChannelData[Channel][Pattern][Item].Vol == 0)
					ChannelData[Channel][Pattern][Item].Vol = 0x10;
				else {
					ChannelData[Channel][Pattern][Item].Vol--;
					ChannelData[Channel][Pattern][Item].Vol &= 0x0F;
				}

				if (ChannelData[Channel][Pattern][Item].Note == 0)
					ChannelData[Channel][Pattern][Item].Instrument = MAX_INSTRUMENTS;
			}
			
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
	m_iFileVersion = DocumentFile.GetFileVersion();

	// Read all blocks
	while (!DocumentFile.Finished() && !FileFinished && !ErrorFlag) {
		DocumentFile.ReadBlock();
		BlockID = DocumentFile.GetBlockHeaderID();

		if (!strcmp(BlockID, FILE_BLOCK_PARAMS)) {
			m_iSongSpeed			= DocumentFile.GetBlockInt();
			m_iChannelsAvailable	= DocumentFile.GetBlockInt();
			m_iMachine				= DocumentFile.GetBlockInt();
			m_iEngineSpeed			= DocumentFile.GetBlockInt();
			if (m_iFileVersion == 0x0200) {
				if (m_iSongSpeed < 20)
					m_iSongSpeed++;
			}
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
		else if (!strcmp(BlockID, FILE_BLOCK_HEADER)) {
			ErrorFlag = ReadBlock_Header(&DocumentFile);
		}
		else if (!strcmp(BlockID, "END")) {
			FileFinished = true;
		}
		else {
			// This shouldn't show up
			//AfxMessageBox("Unknown file block!");
		}

		ASSERT(ErrorFlag == false);
	}

	DocumentFile.Close();

	if (ErrorFlag) {
		AfxMessageBox("Couldn't load file properly!");
		CleanDocument();
		return FALSE;
	}

	m_bFileLoaded = true;

	m_iChannelsAvailable = 5;

	return TRUE;
}

stInstrument *CFamiTrackerDoc::GetInstrument(int Index)
{
	ASSERT(Index >= 0 && Index < MAX_INSTRUMENTS);

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

int	CFamiTrackerDoc::GetNoteEffect(int Channel, int RowPos, int PatternPos, int Index)
{
	return ChannelData[Channel][FrameList[PatternPos][Channel]][RowPos].EffNumber[Index];
}

int	CFamiTrackerDoc::GetNoteEffectValue(int Channel, int RowPos, int PatternPos, int Index)
{
	return ChannelData[Channel][FrameList[PatternPos][Channel]][RowPos].EffParam[Index];
}

stChanNote *CFamiTrackerDoc::GetNoteData(int Channel, int RowPos, int PatternPos)
{
	return &ChannelData[Channel][FrameList[PatternPos][Channel]][RowPos];
}

#define GET_CURRENT_PATTERN(x, y) FrameList[x][y]

void CFamiTrackerDoc::SetNoteData(unsigned int Channel, unsigned int RowPos, unsigned int PatternPos, stChanNote *Note)
{
	if (Channel > unsigned(m_iChannelsAvailable - 1))
		return;

	memcpy(&ChannelData[Channel][GET_CURRENT_PATTERN(PatternPos, Channel)][RowPos], Note, sizeof(stChanNote));
}

void CFamiTrackerDoc::SetSongInfo(char *Name, char *Artist, char *Copyright)
{
	strcpy(m_strName, Name);
	strcpy(m_strArtist, Artist);
	strcpy(m_strCopyright, Copyright);
}

void CFamiTrackerDoc::OnCloseDocument()
{	
	theApp.ShutDownSynth();

	CDocument::OnCloseDocument();
}

unsigned int CFamiTrackerDoc::GetFrameRate(void)
{
	if (m_iEngineSpeed == 0) {
		if (m_iMachine == NTSC)
			return 60;
		else
			return 50;
	}
	
	return m_iEngineSpeed;
}
