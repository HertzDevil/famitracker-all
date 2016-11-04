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

#define GET_PATTERN(Frame, Channel) m_iFrameList[Frame][Channel]

#ifdef _DEBUG
	#define ASSERT_FILE_DATA(Statement) ASSERT(Statement)
#else
	#define ASSERT_FILE_DATA(Statement) if (!(Statement)) return true
#endif

const char	*FILE_HEADER			= "FamiTracker Module";
const char	*FILE_ENDER				= "END";
const int	FILE_VER				= 0x0203;				// Current file version (2.03)
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
	m_bFileLoaded = false;

	for (unsigned int i = 0; i < MAX_DSAMPLES; i++)
		m_DSamples[i].SampleSize = 0;
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

	int i;
	unsigned int x, c;

	// Remove allocated memory
	for (i = 0; i < MAX_DSAMPLES; i++) {
		if (m_DSamples[i].SampleSize != 0) {
			delete [] m_DSamples[i].SampleData;
			m_DSamples[i].SampleSize = 0;
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
			m_iFrameList[i][c] = 0;
		}
	}

	for (x = 0; x < m_iChannelsAvailable; x++) {
		for (c = 0; c < MAX_PATTERN; c++) {
			for (i = 0; i < MAX_PATTERN_LENGTH; i++) {
				m_PatternData[x][c][i].Note		= 0;
				m_PatternData[x][c][i].Octave		= 0;
				m_PatternData[x][c][i].Instrument	= MAX_INSTRUMENTS;
				m_PatternData[x][c][i].Vol		= 0x10;

				for (int n = 0; n < MAX_EFFECT_COLUMNS; n++) {
					m_PatternData[x][c][i].EffNumber[n] = 0;
					m_PatternData[x][c][i].EffParam[n] = 0;
				}
			}
		}
		m_iEffectColumns[x] = 0;
	}

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		memset(m_Instruments + i, 0, sizeof(stInstrument));
		m_Instruments[i].Free = true;
	}

	for (i = 0; i < MAX_SEQUENCES; i++) {
		Sequences[i].Count = 0;
	}

	memset(m_Sequences, 0, sizeof(stSequence) * MAX_SEQUENCES * MOD_COUNT);

	for (i = 0; i < MAX_DSAMPLES; i++) {
		m_DSamples[i].SampleSize = 0;
	}

	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetNextView(Pos));

	MainFrame = static_cast<CMainFrame*>(pView->GetParentFrame());

	MainFrame->ClearInstrumentList();
	MainFrame->CloseInstrumentSettings();

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

//
// File load / save routines
//

void CFamiTrackerDoc::ReorderSequences()
{
	int Keepers[MOD_COUNT] = {0, 0, 0, 0, 0};
	int Indices[MAX_SEQUENCES][MOD_COUNT];
	int Index;

	memset(Indices, 0xFF, MAX_SEQUENCES * MOD_COUNT * sizeof(int));

	// Organize sequences
	for (int i = 0; i < MAX_INSTRUMENTS; i++) {
		if (!m_Instruments[i].Free) {
			for (int x = 0; x < MOD_COUNT; x++) {
				if (m_Instruments[i].ModEnable[x]) {
					Index = m_Instruments[i].ModIndex[x];
					if (Indices[Index][x] >= 0 && Indices[Index][x] != -1) {
						m_Instruments[i].ModIndex[x] = Indices[Index][x];
					}
					else {
						memcpy(&m_Sequences[Keepers[x]][x], &Sequences[Index], sizeof(stSequence));
						for (unsigned int j = 0; j < m_Sequences[Keepers[x]][x].Count; j++) {
							switch (x) {
								case MOD_VOLUME: LIMIT(m_Sequences[Keepers[x]][x].Value[j], 15, 0); break;
								case MOD_DUTYCYCLE: LIMIT(m_Sequences[Keepers[x]][x].Value[j], 3, 0); break;
							}
						}
						Indices[Index][x] = Keepers[x];
						m_Instruments[i].ModIndex[x] = Keepers[x];
						Keepers[x]++;
					}
				}
				else
					m_Instruments[i].ModIndex[x] = 0;
			}
		}
	}
}

BOOL CFamiTrackerDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	if (!m_bFileLoaded)
		return FALSE;

	/***************************/
	//AfxMessageBox("Saving is disabled in this development version!");
	//return FALSE;
	/***************************/

	return SaveDocument(lpszPathName);
}

BOOL CFamiTrackerDoc::LoadOldFile(CFile OpenFile)
{
	unsigned int i, c, ReadCount, FileBlock;

	m_bFileLoaded = false;

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
					m_Instruments[i].Free = ImportedInstruments.Free;
					memcpy(&m_Instruments[i].ModEnable, &ImportedInstruments.ModEnable, 4 * MOD_COUNT);
					memcpy(&m_Instruments[i].ModIndex, &ImportedInstruments.ModIndex, 4 * MOD_COUNT);
					strcpy(m_Instruments[i].Name, ImportedInstruments.Name);
					memset(m_Instruments[i].SamplePitch, 0, 5 * 11);
					memset(m_Instruments[i].Samples, 0, 5 * 11);
					if (ImportedInstruments.AssignedSample > 0) {
						int Pitch = 0;
						for (int y = 0; y < 6; y++) {
							for (int x = 0; x < 12; x++) {
								m_Instruments[i].Samples[y][x] = ImportedInstruments.AssignedSample;
								m_Instruments[i].SamplePitch[y][x] = Pitch;
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
						OpenFile.Read(&m_iFrameList[c][i], sizeof(int));
				}
				break;

			case FB_PATTERNS:
				OpenFile.Read(&ReadCount, sizeof(int));
				OpenFile.Read(&m_iPatternLength, sizeof(int));
				for (unsigned int x = 0; x < m_iChannelsAvailable; x++) {
					for (c = 0; c < ReadCount; c++) {
						for (i = 0; i < m_iPatternLength; i++) {
							OpenFile.Read(&ImportedNote, sizeof(stChanNoteImport));
							m_PatternData[x][c][i].EffNumber[0]	= ImportedNote.ExtraStuff1;
							m_PatternData[x][c][i].EffParam[0]	= ImportedNote.ExtraStuff2;

							m_PatternData[x][c][i].Instrument	= ImportedNote.Instrument;
							m_PatternData[x][c][i].Note			= ImportedNote.Note;
							m_PatternData[x][c][i].Octave		= ImportedNote.Octave;
							m_PatternData[x][c][i].Vol			= 0;

							if (m_PatternData[x][c][i].Note == 0)
								m_PatternData[x][c][i].Instrument = MAX_INSTRUMENTS;
							if (m_PatternData[x][c][i].Vol == 0)
								m_PatternData[x][c][i].Vol = 0x10;
						}
					}
				}
				break;

			case FB_DSAMPLES:
				memset(m_DSamples, 0, sizeof(stDSample) * MAX_DSAMPLES);
				OpenFile.Read(&ReadCount, sizeof(int));
				for (i = 0; i < ReadCount; i++) {
					OpenFile.Read(&ImportedDSample, sizeof(stDSampleImport));
					if (ImportedDSample.SampleSize != 0 && ImportedDSample.SampleSize < 0x4000) {
						ImportedDSample.SampleData = new char[ImportedDSample.SampleSize];
						OpenFile.Read(ImportedDSample.SampleData, ImportedDSample.SampleSize);
					}
					else
						ImportedDSample.SampleData = NULL;
					memcpy(m_DSamples[i].Name, ImportedDSample.Name, 256);
					m_DSamples[i].SampleSize = ImportedDSample.SampleSize;
					m_DSamples[i].SampleData = ImportedDSample.SampleData;
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

	ReorderSequences();

	// File is loaded

	m_bFileLoaded = true;

	return TRUE;
}

BOOL CFamiTrackerDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

	CFile			OpenFile;
	char			Text[256];
	unsigned int	Version;

	CMainFrame	*MainFrame;
	POSITION	Pos = GetFirstViewPosition();

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

	if (Version < 0x0200) {
		if (Version < COMPATIBLE_VER) {
			MessageBox(0, "File version is not supported!", "Error", 0);
			return FALSE;
		}

		if (!LoadOldFile(OpenFile)) {
			OpenFile.Close();
			return FALSE;
		}

		OpenFile.Close();

		// Create a backup of this file, since it's an old version
		char BackupFile[MAX_PATH];
		strcpy(BackupFile, lpszPathName);
		strcat(BackupFile, ".bak");
		CopyFile(lpszPathName, BackupFile, FALSE);

		return TRUE;
	}
	else if (Version >= 0x0200) {
		OpenFile.Close();
		return OpenDocument(lpszPathName);
	}

	return FALSE;
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
	unsigned int i, Count = 0;

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
	const int BLOCK_VERSION = 2;

	int i, x, j, k;
	int Count = 0;

	// Instruments
	pDocFile->CreateBlock(FILE_BLOCK_INSTRUMENTS, BLOCK_VERSION);

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (!m_Instruments[i].Free)
			Count++;
	}

	pDocFile->WriteBlockInt(Count);

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		// Only write instrument if it's used
		if (m_Instruments[i].Free == false) {
			pDocFile->WriteBlockInt(i);
			pDocFile->WriteBlockChar(1);
			
			pDocFile->WriteBlockInt(MOD_COUNT);
			for (x = 0; x < MOD_COUNT; x++) {
				pDocFile->WriteBlockChar(m_Instruments[i].ModEnable[x]);
				pDocFile->WriteBlockChar(m_Instruments[i].ModIndex[x]);
			}

			for (j = 0; j < OCTAVE_RANGE; j++) {
				for (k = 0; k < 12; k++) {
					pDocFile->WriteBlockChar(m_Instruments[i].Samples[j][k]);
					pDocFile->WriteBlockChar(m_Instruments[i].SamplePitch[j][k]);
				}
			}

			pDocFile->WriteBlockInt((int)strlen(m_Instruments[i].Name));
			pDocFile->WriteBlock(&m_Instruments[i].Name, (int)strlen(m_Instruments[i].Name));
		}
	}

	pDocFile->FlushBlock();
}

void CFamiTrackerDoc::WriteBlock_Sequences(CDocumentFile *pDocFile)
{
	unsigned int i, j, x, Count = 0;

	// Sequences, version 2
	pDocFile->CreateBlock(FILE_BLOCK_SEQUENCES, 2);

	for (i = 0; i < MAX_SEQUENCES; i++) {
		for (x = 0; x < MOD_COUNT; x++) {
			if (m_Sequences[i][x].Count > 0)
				Count++;
		}
	}

	pDocFile->WriteBlockInt(Count);

	for (i = 0; i < MAX_SEQUENCES; i++) {
		for (x = 0; x < MOD_COUNT; x++) {
			if (m_Sequences[i][x].Count > 0) {
				pDocFile->WriteBlockInt(i);
				pDocFile->WriteBlockInt(x);
				pDocFile->WriteBlockChar(m_Sequences[i][x].Count);
				for (j = 0; j < m_Sequences[i][x].Count; j++) {
					pDocFile->WriteBlockChar(m_Sequences[i][x].Value[j]);
					pDocFile->WriteBlockChar(m_Sequences[i][x].Length[j]);
				}
			}
		}
	}

	pDocFile->FlushBlock();
}

void CFamiTrackerDoc::WriteBlock_Frames(CDocumentFile *pDocFile)
{
	unsigned int i, x;

	pDocFile->CreateBlock(FILE_BLOCK_FRAMES, 1);
	pDocFile->WriteBlockInt(m_iFrameCount);
	pDocFile->WriteBlockInt(m_iChannelsAvailable);

	for (i = 0; i < m_iFrameCount; i++) {
		for (x = 0; x < m_iChannelsAvailable; x++) {
			pDocFile->WriteBlockChar(m_iFrameList[i][x]);
		}
	}

	pDocFile->FlushBlock();
}

void CFamiTrackerDoc::WriteBlock_Patterns(CDocumentFile *pDocFile)
{
	unsigned int i, x, y, Items;

	pDocFile->CreateBlock(FILE_BLOCK_PATTERNS, 1);
	pDocFile->WriteBlockInt(m_iPatternLength);

	for (i = 0; i < m_iChannelsAvailable; i++) {
		for (x = 0; x < MAX_PATTERN; x++) {
			Items = 0;

			for (y = 0; y < m_iPatternLength; y++) {
				if (m_PatternData[i][x][y].Note != 0 || 
					m_PatternData[i][x][y].EffNumber[0] != 0 ||
					m_PatternData[i][x][y].EffNumber[1] != 0 ||
					m_PatternData[i][x][y].EffNumber[2] != 0 ||
					m_PatternData[i][x][y].EffNumber[3] != 0 ||
					m_PatternData[i][x][y].Vol < 0x10 ||
					m_PatternData[i][x][y].Instrument != MAX_INSTRUMENTS)
					Items++;
			}

			if (Items > 0) {
				pDocFile->WriteBlockInt(i);		// Write channel
				pDocFile->WriteBlockInt(x);		// Write pattern
				pDocFile->WriteBlockInt(Items);	// Number of items

				for (y = 0; y < m_iPatternLength; y++) {
					if (m_PatternData[i][x][y].Note != 0 || 
						m_PatternData[i][x][y].EffNumber[0] != 0 ||
						m_PatternData[i][x][y].EffNumber[1] != 0 ||
						m_PatternData[i][x][y].EffNumber[2] != 0 ||
						m_PatternData[i][x][y].EffNumber[3] != 0 ||
						m_PatternData[i][x][y].Vol < 0x10 ||
						m_PatternData[i][x][y].Instrument != MAX_INSTRUMENTS) {

						pDocFile->WriteBlockInt(y);
						pDocFile->WriteBlockChar(m_PatternData[i][x][y].Note);
						pDocFile->WriteBlockChar(m_PatternData[i][x][y].Octave);
						pDocFile->WriteBlockChar(m_PatternData[i][x][y].Instrument);
						pDocFile->WriteBlockChar(m_PatternData[i][x][y].Vol);

						for (int n = 0; n < signed(m_iEffectColumns[i] + 1); n++) {
							pDocFile->WriteBlockChar(m_PatternData[i][x][y].EffNumber[n]);
							pDocFile->WriteBlockChar(m_PatternData[i][x][y].EffParam[n]);
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
		if (m_DSamples[i].SampleSize > 0)
			Count++;
	}

	pDocFile->WriteBlockChar(Count);

	for (i = 0; i < MAX_DSAMPLES; i++) {
		if (m_DSamples[i].SampleSize > 0) {
			pDocFile->WriteBlockChar(i);
			pDocFile->WriteBlockInt((int)strlen(m_DSamples[i].Name));
			pDocFile->WriteBlock(m_DSamples[i].Name, (int)strlen(m_DSamples[i].Name));
			pDocFile->WriteBlockInt(m_DSamples[i].SampleSize);
			pDocFile->WriteBlock(m_DSamples[i].SampleData, m_DSamples[i].SampleSize);
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
			// This shouldn't show up in release
#ifdef _DEBUG
			AfxMessageBox("Unknown file block!");
#endif
		}

		//ASSERT(ErrorFlag == false);
	}

	DocumentFile.Close();

	if (ErrorFlag) {
		AfxMessageBox("Couldn't load file properly!");
		CleanDocument();
		return FALSE;
	}

	// Make sure a backup is taken if it's an old version
	if (m_iFileVersion < 0x0203) {
		CString BakName;
		BakName.Format("%s.bak", lpszPathName);
		CopyFile(lpszPathName, BakName.GetBuffer(), FALSE);
	}

	if (m_iFileVersion <= 0x0201)
		ReorderSequences();

	m_bFileLoaded = true;

	m_iChannelsAvailable = 5;

	return TRUE;
}

bool CFamiTrackerDoc::ReadBlock_Header(CDocumentFile *pDocFile)
{
	for (unsigned int i = 0; i < m_iChannelsAvailable; i++) {
		pDocFile->GetBlockChar();							// Channel type (unused)
		m_iEffectColumns[i] = pDocFile->GetBlockChar();		// Effect columns
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Instruments(CDocumentFile *pDocFile)
{
	/*
	 * Version changes: 2 - Extended DPCM octave range
	 */
	
	unsigned int SeqCnt, y, x, Index;
	int i, j, k;
	int Count, Type;
	int Version = pDocFile->GetBlockVersion();
	int Octaves;
	
	Count = pDocFile->GetBlockInt();

	for (i = 0; i < Count; i++) {		
		x = pDocFile->GetBlockInt();
		ASSERT_FILE_DATA(x < (MAX_INSTRUMENTS + 1));

		Type	= pDocFile->GetBlockChar();
		SeqCnt	= pDocFile->GetBlockInt();
		ASSERT_FILE_DATA(SeqCnt < (MOD_COUNT + 1));

		for (y = 0; y < SeqCnt; y++) {
			m_Instruments[x].ModEnable[y] = pDocFile->GetBlockChar();
			Index = pDocFile->GetBlockChar();
			ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
			m_Instruments[x].ModIndex[y] = Index;
		}

		if (Version == 1) {
			Octaves = 6;
		}
		else if (Version == 2) {
			Octaves = OCTAVE_RANGE;
		}

		for (j = 0; j < Octaves; j++) {
			for (k = 0; k < 12; k++) {
				Index = pDocFile->GetBlockChar();
				//ASSERT_FILE_DATA(Index < MAX_DSAMPLES);
				if (Index >= MAX_DSAMPLES)
					Index = 0;
				m_Instruments[x].Samples[j][k] = Index;
				m_Instruments[x].SamplePitch[j][k] = pDocFile->GetBlockChar();
			}
		}

		y = pDocFile->GetBlockInt();
		ASSERT_FILE_DATA(y < 256);

		pDocFile->GetBlock(&m_Instruments[x].Name, y);
		m_Instruments[x].Name[y] = 0;

		m_Instruments[x].Free = false;
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Sequences(CDocumentFile *pDocFile)
{
	int i, x, Count = 0, SeqCount, Index, Type;
	int Version = pDocFile->GetBlockVersion();
	char Value, Length;

	Count = pDocFile->GetBlockInt();
	ASSERT_FILE_DATA(Count < MAX_SEQUENCES);

	if (Version == 1) {
		for (i = 0; i < Count; i++) {
			Index		= pDocFile->GetBlockInt();
			SeqCount	= pDocFile->GetBlockChar();
			ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
			ASSERT_FILE_DATA(SeqCount < MAX_SEQ_ITEMS);
			
			Sequences[Index].Count = SeqCount;
			
			for (x = 0; x < SeqCount; x++) {
				Sequences[Index].Value[x] = pDocFile->GetBlockChar();
				Sequences[Index].Length[x] = pDocFile->GetBlockChar();
			}
		}
	}
	else if (Version == 2) {
		for (i = 0; i < Count; i++) {
			Index		= pDocFile->GetBlockInt();
			Type		= pDocFile->GetBlockInt();
			SeqCount	= pDocFile->GetBlockChar();
			ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
			ASSERT_FILE_DATA(Type < MOD_COUNT);
			ASSERT_FILE_DATA(SeqCount < MAX_SEQ_ITEMS);
			
			m_Sequences[Index][Type].Count = SeqCount;
			
			for (x = 0; x < SeqCount; x++) {
				Value = pDocFile->GetBlockChar();
				Length = pDocFile->GetBlockChar();
				m_Sequences[Index][Type].Value[x] = Value;
				m_Sequences[Index][Type].Length[x] = Length;
			}
		}
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Frames(CDocumentFile *pDocFile)
{
	unsigned int Version = pDocFile->GetBlockVersion();
	unsigned int i, x, Frame;

	m_iFrameCount			= pDocFile->GetBlockInt();
	m_iChannelsAvailable	= pDocFile->GetBlockInt();

	ASSERT_FILE_DATA(m_iFrameCount < MAX_FRAMES);
	ASSERT_FILE_DATA(m_iChannelsAvailable <= MAX_CHANNELS);

	for (i = 0; i < m_iFrameCount; i++) {
		for (x = 0; x < m_iChannelsAvailable; x++) {
			Frame = pDocFile->GetBlockChar();
			ASSERT_FILE_DATA(Frame < MAX_FRAMES);
			m_iFrameList[i][x] = Frame;
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
	ASSERT_FILE_DATA(m_iPatternLength <= MAX_PATTERN_LENGTH);

	while (!pDocFile->BlockDone()) {
		Channel = pDocFile->GetBlockInt();
		Pattern = pDocFile->GetBlockInt();
		Items	= pDocFile->GetBlockInt();

		ASSERT_FILE_DATA(Channel < MAX_CHANNELS);
		ASSERT_FILE_DATA(Pattern < MAX_PATTERN);
		ASSERT_FILE_DATA((Items - 1) < MAX_PATTERN_LENGTH);

		for (i = 0; i < Items; i++) {
			if (m_iFileVersion == 0x0200)
				Item = pDocFile->GetBlockChar();
			else
				Item = pDocFile->GetBlockInt();

			ASSERT_FILE_DATA(Item < MAX_PATTERN_LENGTH);

			memset(&m_PatternData[Channel][Pattern][Item], 0, sizeof(stChanNote));

			m_PatternData[Channel][Pattern][Item].Note			= pDocFile->GetBlockChar();
			m_PatternData[Channel][Pattern][Item].Octave		= pDocFile->GetBlockChar();
			m_PatternData[Channel][Pattern][Item].Instrument	= pDocFile->GetBlockChar();
			m_PatternData[Channel][Pattern][Item].Vol			= pDocFile->GetBlockChar();

			if (m_iFileVersion == 0x0200) {
				m_PatternData[Channel][Pattern][Item].EffNumber[0]	= pDocFile->GetBlockChar();
				m_PatternData[Channel][Pattern][Item].EffParam[0]	= (unsigned char)pDocFile->GetBlockChar();
			}
			else {
				for (int n = 0; n < signed(m_iEffectColumns[Channel] + 1); n++) {
					m_PatternData[Channel][Pattern][Item].EffNumber[n]	= pDocFile->GetBlockChar();
					m_PatternData[Channel][Pattern][Item].EffParam[n] 	= (unsigned char)pDocFile->GetBlockChar();
				}
			}
			
			if (m_PatternData[Channel][Pattern][Item].Vol > 0x10)
				m_PatternData[Channel][Pattern][Item].Vol &= 0x0F;

			if (m_iFileVersion == 0x0200) {
				if (m_PatternData[Channel][Pattern][Item].EffNumber[0] == EF_SPEED && m_PatternData[Channel][Pattern][Item].EffParam[0] < 20)
					m_PatternData[Channel][Pattern][Item].EffParam[0]++;
				
				if (m_PatternData[Channel][Pattern][Item].Vol == 0)
					m_PatternData[Channel][Pattern][Item].Vol = 0x10;
				else {
					m_PatternData[Channel][Pattern][Item].Vol--;
					m_PatternData[Channel][Pattern][Item].Vol &= 0x0F;
				}

				if (m_PatternData[Channel][Pattern][Item].Note == 0)
					m_PatternData[Channel][Pattern][Item].Instrument = MAX_INSTRUMENTS;
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
	ASSERT_FILE_DATA(Count < MAX_DSAMPLES);
	
	memset(m_DSamples, 0, sizeof(stDSample) * MAX_DSAMPLES);

	for (i = 0; i < Count; i++) {
		Item	= pDocFile->GetBlockChar();
		Len		= pDocFile->GetBlockInt();
		ASSERT_FILE_DATA(Item < MAX_DSAMPLES);
		ASSERT_FILE_DATA(Len < 256);
		pDocFile->GetBlock(m_DSamples[Item].Name, Len);
		m_DSamples[Item].Name[Len] = 0;
		Size	= pDocFile->GetBlockInt();
		ASSERT_FILE_DATA(Size < 0x8000);
		m_DSamples[Item].SampleData = new char[Size];
		m_DSamples[Item].SampleSize = Size;
		pDocFile->GetBlock(m_DSamples[Item].SampleData, Size);
	}

	return false;
}

stDSample *CFamiTrackerDoc::GetDSample(int Index)
{
	return &m_DSamples[Index];
}

void CFamiTrackerDoc::GetSampleName(int Index, char *Name)
{
	strcpy(Name, m_DSamples[Index].Name);
}

// DMC Stuff below

stDSample *CFamiTrackerDoc::GetFreeDSample()
{
	int i;

	for (i = 0; i < MAX_DSAMPLES; i++) {
		if (m_DSamples[i].SampleSize == 0) {
			return &m_DSamples[i];
		}
	}

	return NULL;
}

void CFamiTrackerDoc::RemoveDSample(int Index)
{
	if (m_DSamples[Index].SampleSize != 0) {
		delete [] m_DSamples[Index].SampleData;
		m_DSamples[Index].SampleSize = 0;
		SetModifiedFlag();
	}
}

void CFamiTrackerDoc::OnCloseDocument()
{	
	theApp.ShutDownSynth();

	CDocument::OnCloseDocument();
}

// ---------------------------------------------------------------------------------------------------------
// Document access functions
// ---------------------------------------------------------------------------------------------------------

// ASSERT is <3

void CFamiTrackerDoc::SetSongInfo(char *Name, char *Artist, char *Copyright)
{
	ASSERT(Name != NULL && Artist != NULL && Copyright != NULL);

	strcpy(m_strName, Name);
	strcpy(m_strArtist, Artist);
	strcpy(m_strCopyright, Copyright);

	SetModifiedFlag();
}

// Sequences

stSequence *CFamiTrackerDoc::GetSequence(int Index, int Type)
{
	ASSERT(Index >= 0 && Index < MAX_SEQUENCES);
	ASSERT(Type >= 0 && Type < MOD_COUNT);

	return &m_Sequences[Index][Type];
}

int CFamiTrackerDoc::GetSequenceCount(int Index, int Type)
{
	ASSERT(Index >= 0 && Index < MAX_SEQUENCES);
	ASSERT(Type >= 0 && Type < MOD_COUNT);

	return m_Sequences[Index][Type].Count;
}

//
// Instruments
//

stInstrument *CFamiTrackerDoc::GetInstrument(int Instrument)
{
	ASSERT(Instrument >= 0 && Instrument < MAX_INSTRUMENTS);

	if (m_Instruments[Instrument].Free == true)
		return NULL;

	return &m_Instruments[Instrument];
}

bool CFamiTrackerDoc::IsInstrumentUsed(int Instrument)
{
	ASSERT(Instrument >= 0 && Instrument < MAX_INSTRUMENTS);

	return !m_Instruments[Instrument].Free;
}

bool CFamiTrackerDoc::GetInstEffState(int Instrument, int Effect)
{
	// Returns the state of an effect in an instrument

	ASSERT(Instrument >= 0 && Instrument < MAX_INSTRUMENTS);
	ASSERT(Effect >= 0 && Effect < MOD_COUNT);

	return (m_Instruments[Instrument].ModEnable[Effect] == 1);
}

int CFamiTrackerDoc::GetInstEffIndex(int Instrument, int Effect)
{
	// Returns the state of an effect in an instrument

	ASSERT(Instrument >= 0 && Instrument < MAX_INSTRUMENTS);
	ASSERT(Effect >= 0 && Effect < MOD_COUNT);

	return m_Instruments[Instrument].ModIndex[Effect];
}

int CFamiTrackerDoc::GetInstDPCM(int Instrument, int Note, int Octave)
{
	ASSERT(Instrument >= 0 && Instrument < MAX_INSTRUMENTS);
	ASSERT(Note >= 0 && Note < 12);
	ASSERT(Octave >= 0 && Octave < OCTAVE_RANGE);

	return m_Instruments[Instrument].Samples[Octave][Note];
}

int CFamiTrackerDoc::GetInstDPCMPitch(int Instrument, int Note, int Octave)
{
	ASSERT(Instrument >= 0 && Instrument < MAX_INSTRUMENTS);
	ASSERT(Note >= 0 && Note < 12);
	ASSERT(Octave >= 0 && Octave < OCTAVE_RANGE);

	return m_Instruments[Instrument].SamplePitch[Octave][Note];
}

void CFamiTrackerDoc::SetInstEffect(int Instrument, int Effect, int Sequence, bool Enabled)
{
	ASSERT(Instrument >= 0 && Instrument < MAX_INSTRUMENTS);
	ASSERT(Effect >= 0 && Effect < MOD_COUNT);
	ASSERT(Sequence >= 0 && Sequence < MAX_SEQUENCES);

	m_Instruments[Instrument].ModEnable[Effect] = Enabled;
	m_Instruments[Instrument].ModIndex[Effect] = Sequence;

	SetModifiedFlag();
}

void CFamiTrackerDoc::SetInstDPCMPitch(int Instrument, int Note, int Octave, int Pitch)
{
	ASSERT(Instrument >= 0 && Instrument < MAX_INSTRUMENTS);
	ASSERT(Note >= 0 && Note < 12);
	ASSERT(Octave >= 0 && Octave < OCTAVE_RANGE);

	m_Instruments[Instrument].SamplePitch[Octave][Note] = Pitch;

	SetModifiedFlag();
}

void CFamiTrackerDoc::SetInstDPCM(int Instrument, int Note, int Octave, int Sample)
{
	ASSERT(Instrument >= 0 && Instrument < MAX_INSTRUMENTS);
	ASSERT(Note >= 0 && Note < 12);
	ASSERT(Octave >= 0 && Octave < OCTAVE_RANGE);
	ASSERT(Sample >= 0 && Sample < MAX_DSAMPLES);

	m_Instruments[Instrument].Samples[Octave][Note] = Sample;

	SetModifiedFlag();
}

unsigned int CFamiTrackerDoc::AddInstrument(const char *Name)
{
	unsigned int i;

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (m_Instruments[i].Free)
			break;
	}

	if (i == MAX_INSTRUMENTS) {
		theApp.GetMainWnd()->MessageBox("You cannot add more instruments, maximum number of allowed instuments are reached.");
		return -1;
	}

	strcpy(m_Instruments[i].Name, Name);
	m_Instruments[i].Free = false;
	
	for (unsigned int c = 0; c < MOD_COUNT; c++) {
		m_Instruments[i].ModEnable[c] = 0;
		m_Instruments[i].ModIndex[c] = 0;
	}

	SetModifiedFlag();

	return i;
}

void CFamiTrackerDoc::RemoveInstrument(unsigned int Index)
{
	ASSERT(Index < MAX_INSTRUMENTS);
	
	if (m_Instruments[Index].Free == true)
		return;

	memset(m_Instruments + Index, 0, sizeof(stInstrument));

	m_Instruments[Index].Free = true;

	SetModifiedFlag();
}

void CFamiTrackerDoc::GetInstrumentName(unsigned int Index, char *Name)
{
	ASSERT(Index < MAX_INSTRUMENTS);
	strcpy(Name, m_Instruments[Index].Name);
}

void CFamiTrackerDoc::SetInstrumentName(unsigned int Index, char *Name)
{
	ASSERT(Index < MAX_INSTRUMENTS);

	if (m_Instruments[Index].Free == true)
		return;

	strcpy(m_Instruments[Index].Name, Name);

	SetModifiedFlag();
}

//
// DPCM samples
//

int CFamiTrackerDoc::GetSampleSize(unsigned int Sample)
{
	ASSERT(Sample >= 0 && Sample < MAX_DSAMPLES);

	return m_DSamples[Sample].SampleSize;
}

char CFamiTrackerDoc::GetSampleData(unsigned int Sample, unsigned int Offset)
{
	ASSERT(Sample >= 0 && Sample < MAX_DSAMPLES);

	if (Offset >= m_DSamples[Sample].SampleSize)
		return 0;

	return m_DSamples[Sample].SampleData[Offset];
}

//
// General document
//

void CFamiTrackerDoc::SetFrameCount(unsigned int Count)
{
	ASSERT(Count <= MAX_FRAMES);

	if (m_iFrameCount != Count)
		SetModifiedFlag();

	m_iFrameCount = Count;

	UpdateAllViews(NULL);
}

void CFamiTrackerDoc::SetPatternLength(unsigned int Length)
{ 
	ASSERT(Length <= MAX_PATTERN_LENGTH);

	if (m_iPatternLength != Length)
		SetModifiedFlag();

	m_iPatternLength = Length;

	UpdateAllViews(NULL);
}

void CFamiTrackerDoc::SetSongSpeed(unsigned int Speed)
{
	ASSERT(Speed <= MAX_TEMPO);
	
	if (m_iSongSpeed != Speed)
		SetModifiedFlag();

	m_iSongSpeed = Speed;
}

unsigned int CFamiTrackerDoc::GetEffColumns(unsigned int Channel) const
{
	ASSERT(Channel < MAX_CHANNELS);
	return m_iEffectColumns[Channel];
}

void CFamiTrackerDoc::SetEffColumns(unsigned int Channel, unsigned int Columns)
{
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Columns < MAX_EFFECT_COLUMNS);

	m_iEffectColumns[Channel] = Columns;

	SetModifiedFlag();
	UpdateAllViews(NULL);
}

void CFamiTrackerDoc::SetEngineSpeed(unsigned int Speed)
{
	ASSERT(Speed < 200); // hardcoded for now
	m_iEngineSpeed = Speed;
}

void CFamiTrackerDoc::SetMachine(unsigned int Machine)
{
	ASSERT(Machine == PAL || Machine == NTSC);
	m_iMachine = Machine;
}

unsigned int CFamiTrackerDoc::GetPatternAtFrame(unsigned int Frame, unsigned int Channel) const
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);

	return m_iFrameList[Frame][Channel];
}

void CFamiTrackerDoc::SetPatternAtFrame(unsigned int Frame, unsigned int Channel, unsigned int Pattern)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Pattern < MAX_PATTERN);

	m_iFrameList[Frame][Channel] = Pattern;

	SetModifiedFlag();
	UpdateAllViews(NULL);
}

unsigned int CFamiTrackerDoc::GetFrameRate(void) const
{
	if (m_iEngineSpeed == 0) {
		if (m_iMachine == NTSC)
			return 60;
		else
			return 50;
	}
	
	return m_iEngineSpeed;
}

//
// Pattern editing
//

void CFamiTrackerDoc::IncreasePattern(unsigned int Frame, unsigned int Channel)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);

	// Selects the next channel pattern
	if (m_iFrameList[Frame][Channel] < (MAX_PATTERN - 1)) {
		m_iFrameList[Frame][Channel]++;
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::DecreasePattern(unsigned int Frame, unsigned int Channel)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);

	// Selects the previous channel pattern
	if (m_iFrameList[Frame][Channel] > 0) {
		m_iFrameList[Frame][Channel]--;
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::IncreaseInstrument(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	unsigned int Inst = m_PatternData[Channel][GET_PATTERN(Frame, Channel)][Row].Instrument;

	if (Inst < MAX_INSTRUMENTS) {
		m_PatternData[Channel][GET_PATTERN(Frame, Channel)][Row].Instrument = Inst + 1;
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::DecreaseInstrument(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	unsigned int Inst = m_PatternData[Channel][GET_PATTERN(Frame, Channel)][Row].Instrument;

	if (Inst > 0) {
		m_PatternData[Channel][GET_PATTERN(Frame, Channel)][Row].Instrument = Inst - 1;
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::IncreaseVolume(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	unsigned int Vol = m_PatternData[Channel][GET_PATTERN(Frame, Channel)][Row].Vol;

	if (Vol < 0x10) {
		m_PatternData[Channel][GET_PATTERN(Frame, Channel)][Row].Vol = Vol + 1;
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::DecreaseVolume(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	unsigned int Vol = m_PatternData[Channel][GET_PATTERN(Frame, Channel)][Row].Vol;

	if (Vol > 1) {
		m_PatternData[Channel][GET_PATTERN(Frame, Channel)][Row].Vol = Vol - 1;
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::IncreaseEffect(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Index)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);
	ASSERT(Index < MAX_EFFECT_COLUMNS);

	unsigned int Effect = m_PatternData[Channel][GET_PATTERN(Frame, Channel)][Row].EffParam[Index];
	
	if (Effect < 256) {
		m_PatternData[Channel][GET_PATTERN(Frame, Channel)][Row].EffParam[Index] = Effect + 1;
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::DecreaseEffect(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Index)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);
	ASSERT(Index < MAX_EFFECT_COLUMNS);

	unsigned int Effect = m_PatternData[Channel][GET_PATTERN(Frame, Channel)][Row].EffParam[Index];

	if (Effect > 0) {
		m_PatternData[Channel][GET_PATTERN(Frame, Channel)][Row].EffParam[Index] = Effect - 1;
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::SetNoteData(unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *Data)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	// Get notes from the pattern
	memcpy(&m_PatternData[Channel][GET_PATTERN(Frame, Channel)][Row], Data, sizeof(stChanNote));

	SetModifiedFlag();
	UpdateAllViews(NULL);
}


void CFamiTrackerDoc::GetNoteData(unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *Data) const
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	// Sets the notes of the pattern
	memcpy(Data, &m_PatternData[Channel][GET_PATTERN(Frame, Channel)][Row], sizeof(stChanNote));
}

void CFamiTrackerDoc::SetDataAtPattern(unsigned int Pattern, unsigned int Channel, unsigned int Row, stChanNote *Data)
{
	ASSERT(Pattern < MAX_PATTERN);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	// Set a note to a direct pattern
	memcpy(&m_PatternData[Channel][Pattern][Row], Data, sizeof(stChanNote));

	SetModifiedFlag();
	UpdateAllViews(NULL);
}

void CFamiTrackerDoc::GetDataAtPattern(unsigned int Pattern, unsigned int Channel, unsigned int Row, stChanNote *Data) const
{
	ASSERT(Pattern < MAX_PATTERN);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	// Get note from a direct pattern
	memcpy(Data, &m_PatternData[Channel][Pattern][Row], sizeof(stChanNote));
}

unsigned int CFamiTrackerDoc::GetNoteEffectType(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);
	ASSERT(Index < MAX_EFFECT_COLUMNS);

	return m_PatternData[Channel][GET_PATTERN(Frame, Channel)][Row].EffNumber[Index];
}

unsigned int CFamiTrackerDoc::GetNoteEffectParam(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);
	ASSERT(Index < MAX_EFFECT_COLUMNS);

	return m_PatternData[Channel][GET_PATTERN(Frame, Channel)][Row].EffParam[Index];
}

bool CFamiTrackerDoc::InsertNote(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	stChanNote Note;

	for (int n = 0; n < MAX_EFFECT_COLUMNS; n++) {
		Note.EffNumber[n] = 0;
		Note.EffParam[n] = 0;
	}

	Note.Note			= 0;
	Note.Octave			= 0;
	Note.Instrument		= MAX_INSTRUMENTS;
	Note.Vol			= 0x10;

	for (unsigned int i = m_iPatternLength - 1; i > Row; i--) {
		m_PatternData[Channel][m_iFrameList[Frame][Channel]][i] = m_PatternData[Channel][m_iFrameList[Frame][Channel]][i - 1];
	}

	m_PatternData[Channel][m_iFrameList[Frame][Channel]][Row] = Note;

	SetModifiedFlag();
	UpdateAllViews(NULL);

	return true;
}

bool CFamiTrackerDoc::DeleteNote(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Column)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	stChanNote Note;

	Note = m_PatternData[Channel][m_iFrameList[Frame][Channel]][Row];

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
	
	m_PatternData[Channel][m_iFrameList[Frame][Channel]][Row] = Note;

	SetModifiedFlag();
	UpdateAllViews(NULL);

	return true;
}

bool CFamiTrackerDoc::RemoveNote(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	stChanNote Note;
	unsigned int i;

	for (int n = 0; n < MAX_EFFECT_COLUMNS; n++) {
		Note.EffNumber[n] = 0;
		Note.EffParam[n] = 0;
	}

	Note.Note			= 0;
	Note.Octave			= 0;
	Note.Instrument		= MAX_INSTRUMENTS;
	Note.Vol			= 0x10;

	for (i = Row - 1; i < (m_iPatternLength - 1); i++) {
		m_PatternData[Channel][m_iFrameList[Frame][Channel]][i] = m_PatternData[Channel][m_iFrameList[Frame][Channel]][i + 1];
	}

	m_PatternData[Channel][m_iFrameList[Frame][Channel]][m_iPatternLength - 1] = Note;

	SetModifiedFlag();
	UpdateAllViews(NULL);

	return true;
}
