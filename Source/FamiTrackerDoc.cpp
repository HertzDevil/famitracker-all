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

 Document file version changes

 Ver 4.0
  - Header block, added song names

 Ver 3.0
  - Sequences are stored in the way they are represented in the instrument editor
  - Added separate speed and tempo settings
  - Changed automatic portamento to 3xx and added 1xx & 2xx portamento

 Ver 2.1
  - Made some additions to support multiple effect columns and prepared for more channels
  - Made some speed adjustments, increase speed effects by one if it's below 20

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
#include "..\include\famitrackerdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define GET_PATTERN(Frame, Channel) m_pSelectedTune->m_iFrameList[Frame][Channel]

#ifdef _DEBUG
	#define ASSERT_FILE_DATA(Statement) ASSERT(Statement)
#else
	#define ASSERT_FILE_DATA(Statement) if (!(Statement)) return true
#endif

const char	*FILE_HEADER			= "FamiTracker Module";
const char	*FILE_ENDER				= "END";
const int	FILE_VER				= 0x0400;				// Current file version (4.00)
const int	COMPATIBLE_VER			= 0x0100;				// Compatible file version (1.0)

const char *FILE_BLOCK_PARAMS		= "PARAMS";
const char *FILE_BLOCK_INFO			= "INFO";
const char *FILE_BLOCK_INSTRUMENTS	= "INSTRUMENTS";
const char *FILE_BLOCK_SEQUENCES	= "SEQUENCES";
const char *FILE_BLOCK_FRAMES		= "FRAMES";
const char *FILE_BLOCK_PATTERNS		= "PATTERNS";
const char *FILE_BLOCK_DSAMPLES		= "DPCM SAMPLES";
const char *FILE_BLOCK_HEADER		= "HEADER";

// VRC6
const char *FILE_BLOCK_SEQUENCES_VRC6 = "SEQUENCES_VRC6";

// Instruments
const char INST_HEADER[] = "FTI";
const char INST_VERSION[] = "2.0";

struct stInstrumentImport {
	char	Name[256];
	bool	Free;
	int		ModEnable[MOD_COUNT];
	int		ModIndex[MOD_COUNT];
	int		AssignedSample;				// For DPCM
};

struct stSequenceImport {
	signed char Length[64];	// locked to 64
	signed char Value[64];
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
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
END_MESSAGE_MAP()


// CFamiTrackerDoc construction/destruction

CFamiTrackerDoc::CFamiTrackerDoc()
{
	unsigned int i;
	m_bFileLoaded = false;

	m_SampleBank.InitBank();

	for (i = 0; i < MAX_DSAMPLES; i++)
		m_DSamples[i].SampleSize = 0;

	for (i = 0; i < MAX_TRACKS; i++)
		m_pTunes[i] = NULL;

	for (i = 0; i < MAX_INSTRUMENTS; i++)
		m_pInstruments[i] = NULL;
}

CFamiTrackerDoc::~CFamiTrackerDoc()
{
}

BOOL CFamiTrackerDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// Start with a single song
	SwitchToTrack(0);

	m_sTrackNames[0] = "New song";

	// and 2A03
	SelectExpansionChip(CHIP_NONE);

	// Document is avaliable
	m_bFileLoaded = true;

	theApp.SetDocumentLoaded(true);

	return TRUE;
}

void CFamiTrackerDoc::DeleteContents()
{
	unsigned int i;

	// Make sure player is stopped
	theApp.StopPlayer();

	// File unloaded
	m_bFileLoaded = false;

	// Remove/reset allocated memory

	// DPCM sample
	m_SampleBank.ClearSamples();

	for (i = 0; i < MAX_DSAMPLES; i++) {
		if (m_DSamples[i].SampleSize != 0) {
			delete [] m_DSamples[i].SampleData;
		}
		m_DSamples[i].SampleSize = 0;
	}

	// Instruments
	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (m_pInstruments[i]) {
			delete m_pInstruments[i];
			m_pInstruments[i] = NULL;
		}

		/*
		memset(m_Instruments + i, 0, sizeof(stInstrument));
		m_Instruments[i].SetFree(true);
		*/
	}

	// Sequences
	for (i = 0; i < MAX_SEQUENCES; i++) {
		Sequences[i].Count = 0;
		for (int j = 0; j < MOD_COUNT; j++) {
			m_NewSequences[i][j].Clear();
			m_SequencesVRC6[i][j].Clear();
		}
	}

	memset(m_Sequences, 0, sizeof(stSequence) * MAX_SEQUENCES * MOD_COUNT);

	// Clear tracks
	m_iTracks = 0;
	m_iTrack = 0;

	// Delete all patterns
	for (i = 0; i < MAX_TRACKS; i++) {
		if (m_pTunes[i] != NULL)
			delete m_pTunes[i];
		m_sTrackNames[i] = "";
		m_pTunes[i] = NULL;
	}

	// Reset variables to default
	m_cExpansionChip = CHIP_NONE;
	m_iChannelsAvailable = CHANNELS_DEFAULT;

	// Clear song info
	memset(m_strName, 0, 32);
	memset(m_strArtist, 0, 32);
	memset(m_strCopyright, 0, 32);

	// Clear variables
	m_iMachine		= NTSC;
	m_iEngineSpeed	= 0;

	// Update view
//	UpdateAllViews(NULL, CHANGED_CLEAR);

	CDocument::DeleteContents();
}

void CFamiTrackerDoc::OnFileSave()
{
	if (GetPathName().GetLength() == 0)
		OnFileSaveAs();
	else
		CDocument::OnFileSave();
}

void CFamiTrackerDoc::OnFileSaveAs()
{
	// Overloaded this for separate saving of the ftm-path
	CFileDialog FileDialog(FALSE, "ftm", "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "FamiTracker files (*.ftm)|*.ftm|All files (*.*)|*.*||", theApp.GetMainWnd(), 0);

	FileDialog.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_FTM);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	theApp.m_pSettings->SetPath(FileDialog.GetPathName(), PATH_FTM);

	CString Title = FileDialog.GetFileTitle() + "." + FileDialog.GetFileExt();

	OnSaveDocument(FileDialog.GetPathName());

	SetPathName(FileDialog.GetPathName());
	SetTitle(Title);
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File load / save routines
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerDoc::ReorderSequences()
{
	int Keepers[MOD_COUNT] = {0, 0, 0, 0, 0};
	int Indices[MAX_SEQUENCES][MOD_COUNT];
	int Index;

	memset(Indices, 0xFF, MAX_SEQUENCES * MOD_COUNT * sizeof(int));

	CInstrument2A03	*pInst;

	// Organize sequences
	for (int i = 0; i < MAX_INSTRUMENTS; i++) {
		if (m_pInstruments[i] != NULL) {
			pInst = (CInstrument2A03*)m_pInstruments[i];
			for (int x = 0; x < MOD_COUNT; x++) {
				if (pInst->GetModEnable(x)) {
					Index = pInst->GetModIndex(x);
					if (Indices[Index][x] >= 0 && Indices[Index][x] != -1) {
						pInst->SetModIndex(x, Indices[Index][x]);
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
						pInst->SetModIndex(x, Keepers[x]);
						Keepers[x]++;
					}
				}
				else
					pInst->SetModIndex(x, 0);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Document store functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*** File format description ***

0000: "FamiTracker Module"					id string
000x: Version								int, version number
000x: Start of blocks

 {FILE_BLOCK_PARAMS, 2}
  Expansion chip							char
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

BOOL CFamiTrackerDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	if (!m_bFileLoaded)
		return FALSE;

	return SaveDocument(lpszPathName);
}

void CFamiTrackerDoc::WriteBlock_Header(CDocumentFile *pDocFile)
{
	unsigned int i, j, Count = 0;

	/* Header data
 	 *
	 *  Store song count and then for each channel: 
	 *  channel type and number of effect columns
	 *
	 */

	// Version 3 adds song names

	// Header data
	pDocFile->CreateBlock(FILE_BLOCK_HEADER, 3);

	// Write number of tracks
	pDocFile->WriteBlockChar(m_iTracks);

	// Ver 3, store track names
	for (i = 0; i <= m_iTracks; i++) {
		pDocFile->WriteString(m_sTrackNames[i]);
	}

	for (i = 0; i < m_iChannelsAvailable; i++) {
		pDocFile->WriteBlockChar(i);			// Channel type
		for (j = 0; j <= m_iTracks; j++) {
			AllocateSong(j);
			pDocFile->WriteBlockChar(m_pTunes[j]->m_iEffectColumns[i]);		// Effect columns
		}
	}

	pDocFile->FlushBlock();
}

void CFamiTrackerDoc::WriteBlock_Instruments(CDocumentFile *pDocFile)
{
	const int BLOCK_VERSION = 2;

	int i, x, j, k;
	int Count = 0;
	char Name[256], Type;

	CInstrument/*2A03*/ *pInst;

	/* Store instruments
	 * 
	 */ 

	// Instruments
	pDocFile->CreateBlock(FILE_BLOCK_INSTRUMENTS, BLOCK_VERSION);

	// Count number of instruments
	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (m_pInstruments[i] != NULL)
			Count++;
	}

	pDocFile->WriteBlockInt(Count);

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		// Only write instrument if it's used
		if (m_pInstruments[i] != NULL) {

			Type = m_pInstruments[i]->GetType();

			// Write index and type
			pDocFile->WriteBlockInt(i);
			pDocFile->WriteBlockChar(Type);

			switch (Type) {
				case INST_2A03: 
				{
					pInst = /*(CInstrument2A03*)*/m_pInstruments[i];
					CInstrument2A03 *pInstrument = (CInstrument2A03*)pInst;
					pDocFile->WriteBlockInt(MOD_COUNT);
					for (x = 0; x < MOD_COUNT; x++) {
						pDocFile->WriteBlockChar(pInstrument->GetModEnable(x));
						pDocFile->WriteBlockChar(pInstrument->GetModIndex(x));
					}
					for (j = 0; j < OCTAVE_RANGE; j++) {
						for (k = 0; k < 12; k++) {
							pDocFile->WriteBlockChar(pInstrument->GetSample(j, k));
							pDocFile->WriteBlockChar(pInstrument->GetSamplePitch(j, k));
						}
					}
				}
				break;
				case INST_VRC6: 
				{
					pInst = m_pInstruments[i];
					CInstrumentVRC6 *pInstrument = (CInstrumentVRC6*)pInst;
					pDocFile->WriteBlockInt(MOD_COUNT);
					for (x = 0; x < MOD_COUNT; x++) {
						pDocFile->WriteBlockChar(pInstrument->GetModEnable(x));
						pDocFile->WriteBlockChar(pInstrument->GetModIndex(x));
					}
				}
				break;
			}

			// Store the name
			m_pInstruments[i]->GetName(Name);
			pDocFile->WriteBlockInt((int)strlen(Name));
			pDocFile->WriteBlock(Name, (int)strlen(Name));			
		}
	}

	pDocFile->FlushBlock();
}

void CFamiTrackerDoc::WriteBlock_Sequences(CDocumentFile *pDocFile)
{
	unsigned int i, j, x, Count = 0;

	/* Store sequences
	 * 
	 */ 

	// Sequences, version 3
	pDocFile->CreateBlock(FILE_BLOCK_SEQUENCES, 3);

	// Count number of used sequences
	for (i = 0; i < MAX_SEQUENCES; i++) {
		for (x = 0; x < MOD_COUNT; x++) {
			if (m_NewSequences[i][x].GetItemCount() > 0)
				Count++;
		}
	}

	// Write it
	pDocFile->WriteBlockInt(Count);

	for (i = 0; i < MAX_SEQUENCES; i++) {
		for (x = 0; x < MOD_COUNT; x++) {
			Count = m_NewSequences[i][x].GetItemCount();
			if (Count > 0) {
				// Store index
				pDocFile->WriteBlockInt(i);
				// Store type of sequence
				pDocFile->WriteBlockInt(x);
				// Store number of items in this sequence
				pDocFile->WriteBlockChar(Count);
				// Store loop point
				pDocFile->WriteBlockInt(m_NewSequences[i][x].GetLoopPoint());
				for (j = 0; j < Count; j++) {
					pDocFile->WriteBlockChar(m_NewSequences[i][x].GetItem(j));
				}
			}
		}
	}

	pDocFile->FlushBlock();
}

void CFamiTrackerDoc::WriteBlock_Frames(CDocumentFile *pDocFile)
{
	unsigned int i, x, y;

	/* Store frame count
	 *
	 * 1. Number of channels (5 for 2A03 only)
	 * 2. 
	 * 
	 */ 

	pDocFile->CreateBlock(FILE_BLOCK_FRAMES, 3);

//	pDocFile->WriteBlockInt(m_iChannelsAvailable);

	for (y = 0; y <= m_iTracks; y++) {

		pDocFile->WriteBlockInt(m_pTunes[y]->m_iFrameCount);
		pDocFile->WriteBlockInt(m_pTunes[y]->m_iSongSpeed);
		pDocFile->WriteBlockInt(m_pTunes[y]->m_iSongTempo);
		pDocFile->WriteBlockInt(m_pTunes[y]->m_iPatternLength);

		for (i = 0; i < m_pTunes[y]->m_iFrameCount; i++) {
			for (x = 0; x < m_iChannelsAvailable; x++) {
				pDocFile->WriteBlockChar((unsigned char)m_pTunes[y]->m_iFrameList[i][x]);
			}
		}
	}

	pDocFile->FlushBlock();
}

void CFamiTrackerDoc::WriteBlock_Patterns(CDocumentFile *pDocFile)
{
	/*
	 * Version changes: 2: Support multiple tracks
	 *					3: Changed portamento effect
	 */ 

	unsigned int i, t, x, y, Items;

	pDocFile->CreateBlock(FILE_BLOCK_PATTERNS, 3);

	for (t = 0; t <= m_iTracks; t++) {
		for (i = 0; i < m_iChannelsAvailable; i++) {
			for (x = 0; x < MAX_PATTERN; x++) {
				Items = 0;

				for (y = 0; y < m_pTunes[t]->m_iPatternLength; y++) {
					if (m_pTunes[t]->IsCellFree(i, x, y))
						Items++;
				}

				if (Items > 0) {
					pDocFile->WriteBlockInt(t);		// Write track
					pDocFile->WriteBlockInt(i);		// Write channel
					pDocFile->WriteBlockInt(x);		// Write pattern
					pDocFile->WriteBlockInt(Items);	// Number of items

					for (y = 0; y < m_pTunes[t]->m_iPatternLength; y++) {
						if (m_pTunes[t]->IsCellFree(i, x, y)) {
							pDocFile->WriteBlockInt(y);

							pDocFile->WriteBlockChar(m_pTunes[t]->GetNote(i, x, y));
							pDocFile->WriteBlockChar(m_pTunes[t]->GetOctave(i, x, y));
							pDocFile->WriteBlockChar(m_pTunes[t]->GetInstrument(i, x, y));
							pDocFile->WriteBlockChar(m_pTunes[t]->GetVolume(i, x, y));

							for (unsigned int n = 0; n < (m_pTunes[t]->m_iEffectColumns[i] + 1); n++) {
								pDocFile->WriteBlockChar(m_pTunes[t]->GetEffect(i, x, y, n));
								pDocFile->WriteBlockChar(m_pTunes[t]->GetEffectParam(i, x, y, n));
							}
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

	m_SampleBank.GetCount();

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

void CFamiTrackerDoc::WriteBlock_SequencesVRC6(CDocumentFile *pDocFile)
{
	unsigned int i, j, x, Count = 0;

	/* Store VRC6 sequences
	 * 
	 */ 

	// Sequences, version 1
	pDocFile->CreateBlock(FILE_BLOCK_SEQUENCES_VRC6, 1);

	// Count number of used sequences
	for (i = 0; i < MAX_SEQUENCES; i++) {
		for (x = 0; x < MOD_COUNT; x++) {
			if (m_SequencesVRC6[i][x].GetItemCount() > 0)
				Count++;
		}
	}

	// Write it
	pDocFile->WriteBlockInt(Count);

	for (i = 0; i < MAX_SEQUENCES; i++) {
		for (x = 0; x < MOD_COUNT; x++) {
			Count = m_SequencesVRC6[i][x].GetItemCount();
			if (Count > 0) {
				// Store index
				pDocFile->WriteBlockInt(i);
				// Store type of sequence
				pDocFile->WriteBlockInt(x);
				// Store number of items in this sequence
				pDocFile->WriteBlockChar(Count);
				// Store loop point
				pDocFile->WriteBlockInt(m_SequencesVRC6[i][x].GetLoopPoint());
				for (j = 0; j < Count; j++) {
					pDocFile->WriteBlockChar(m_SequencesVRC6[i][x].GetItem(j));
				}
			}
		}
	}

	pDocFile->FlushBlock();
}


BOOL CFamiTrackerDoc::SaveDocument(LPCTSTR lpszPathName)
{
	CDocumentFile DocumentFile;

	char TempPath[MAX_PATH], TempFile[MAX_PATH];

	// First write to a temp file (if it would fail, the original is not destroyed)
	GetTempPath(MAX_PATH, TempPath);
	GetTempFileName(TempPath, "NEW", 0, TempFile);

	//if (!DocumentFile.Open(lpszPathName, CFile::modeWrite | CFile::modeCreate)) {
	if (!DocumentFile.Open(TempFile, CFile::modeWrite | CFile::modeCreate)) {
		theApp.DisplayError(IDS_SAVE_ERROR);
		return FALSE;
	}

	DocumentFile.BeginDocument();

	// Song parameters
	DocumentFile.CreateBlock(FILE_BLOCK_PARAMS, 2);
	DocumentFile.WriteBlockChar(m_cExpansionChip);		// ver 2 change
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

	if (m_cExpansionChip & CHIP_VRC6) {
		WriteBlock_SequencesVRC6(&DocumentFile);
	}

	DocumentFile.EndDocument();

	DocumentFile.Close();

	// Everything is done, replace the original

	// Delete old file (if exists)
	DeleteFile(lpszPathName);

	// Move in new file
	if (!MoveFile(TempFile, lpszPathName)) {
		CComBSTR	ErrorMsg;
		CString		FinalMsg;
		char		*lpMsgBuf;

		ErrorMsg.LoadString(IDS_SAVE_ERROR_REASON);
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
		FinalMsg.Append(CW2CT(ErrorMsg));
		FinalMsg.Append(lpMsgBuf);
		theApp.DoMessageBox(FinalMsg, MB_OK | MB_ICONERROR, 0);
		LocalFree(lpMsgBuf);

		return FALSE;
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Document load functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CFamiTrackerDoc::LoadOldFile(CFile *pOpenFile)
{
	unsigned int i, c, ReadCount, FileBlock;

	FileBlock = 0;

	// Only single track files
	SwitchToTrack(0);

	stInstrumentImport	ImportedInstruments;
	stSequenceImport	ImportedSequence;
	stDSampleImport		ImportedDSample;
	stChanNoteImport	ImportedNote;

	while (FileBlock != FB_EOF) {
		if (pOpenFile->Read(&FileBlock, sizeof(int)) == 0)
			FileBlock = FB_EOF;

		switch (FileBlock) {
			case FB_CHANNELS:
				pOpenFile->Read(&m_iChannelsAvailable, sizeof(int));
				break;

			case FB_SPEED:
				pOpenFile->Read(&m_pSelectedTune->m_iSongSpeed, sizeof(int));
				m_pSelectedTune->m_iSongSpeed++;
				break;

			case FB_MACHINE:
				pOpenFile->Read(&m_iMachine, sizeof(int));				
				break;

			case FB_ENGINESPEED:
				pOpenFile->Read(&m_iEngineSpeed, sizeof(int));
				break;

			case FB_INSTRUMENTS:
				pOpenFile->Read(&ReadCount, sizeof(int));
				if (ReadCount > MAX_INSTRUMENTS)
					ReadCount = MAX_INSTRUMENTS - 1;
				for (i = 0; i < ReadCount; i++) {
					pOpenFile->Read(&ImportedInstruments, sizeof(stInstrumentImport));
					if (ImportedInstruments.Free == false) {
						CInstrument2A03 *pInst = new CInstrument2A03();
						for (int j = 0; j < MOD_COUNT; j++) {
							pInst->SetModEnable(j, ImportedInstruments.ModEnable[j]);
							pInst->SetModIndex(j, ImportedInstruments.ModIndex[j]);
						}
						pInst->SetName(ImportedInstruments.Name);

						if (ImportedInstruments.AssignedSample > 0) {
							int Pitch = 0;
							for (int y = 0; y < 6; y++) {
								for (int x = 0; x < 12; x++) {
									pInst->SetSample(y, x, ImportedInstruments.AssignedSample);
									pInst->SetSamplePitch(y, x, Pitch);
									Pitch = (Pitch + 1) % 16;
								}
							}
						}

						m_pInstruments[i] = pInst;
					}
					/*
					m_Instruments[i].IsFree() = ImportedInstruments.Free;
					memcpy(&m_Instruments[i].ModEnable, &ImportedInstruments.ModEnable, 4 * MOD_COUNT);
					memcpy(&m_Instruments[i].ModIndex, &ImportedInstruments.ModIndex, 4 * MOD_COUNT);
					strcpy(m_Instruments[i].Name, ImportedInstruments.Name);
					memset(m_Instruments[i].SamplePitch, 0, 5 * 11);
					memset(m_Instruments[i].Samples, 0, 5 * 11);
					*/
					/*
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
					*/
				}
				break;

			case FB_SEQUENCES:
				pOpenFile->Read(&ReadCount, sizeof(int));
				for (i = 0; i < ReadCount; i++) {
					pOpenFile->Read(&ImportedSequence, sizeof(stSequenceImport));
					if (ImportedSequence.Count > 0 && ImportedSequence.Count < MAX_SEQ_ITEMS) {
						Sequences[i].Count = ImportedSequence.Count;
						memcpy(Sequences[i].Length, ImportedSequence.Length, ImportedSequence.Count);
						memcpy(Sequences[i].Value, ImportedSequence.Value, ImportedSequence.Count);
					}
				}
				break;

			case FB_PATTERN_ROWS:
				pOpenFile->Read(&m_pSelectedTune->m_iFrameCount, sizeof(int));
				for (c = 0; c < m_pSelectedTune->m_iFrameCount; c++) {
					for (i = 0; i < m_iChannelsAvailable; i++)
						pOpenFile->Read(&m_pSelectedTune->m_iFrameList[c][i], sizeof(int));
				}
				break;

			case FB_PATTERNS:
				pOpenFile->Read(&ReadCount, sizeof(int));
				pOpenFile->Read(&m_pSelectedTune->m_iPatternLength, sizeof(int));
				for (unsigned int x = 0; x < m_iChannelsAvailable; x++) {
					for (c = 0; c < ReadCount; c++) {
						for (i = 0; i < m_pSelectedTune->m_iPatternLength; i++) {
							pOpenFile->Read(&ImportedNote, sizeof(stChanNoteImport));

							if (ImportedNote.ExtraStuff1 == EF_PORTAOFF) {
								ImportedNote.ExtraStuff1 = EF_PORTAMENTO;
								ImportedNote.ExtraStuff2 = 0;
							}
							else if (ImportedNote.ExtraStuff1 == EF_PORTAMENTO) {
								if (ImportedNote.ExtraStuff2 < 0xFF)
									ImportedNote.ExtraStuff2++;
							}

							stChanNote *Note;

							Note = m_pSelectedTune->GetPatternData(x, c, i);
							
							Note->EffNumber[0]	= ImportedNote.ExtraStuff1;
							Note->EffParam[0]	= ImportedNote.ExtraStuff2;

							Note->Instrument	= ImportedNote.Instrument;
							Note->Note			= ImportedNote.Note;
							Note->Octave		= ImportedNote.Octave;
							Note->Vol			= 0;

							if (Note->Note == 0)
								Note->Instrument = MAX_INSTRUMENTS;
							if (Note->Vol == 0)
								Note->Vol = 0x10;

							/*
							m_pSelectedTune->m_stPatternData[x][c][i].EffNumber[0]	= ImportedNote.ExtraStuff1;
							m_pSelectedTune->m_stPatternData[x][c][i].EffParam[0]	= ImportedNote.ExtraStuff2;

							m_pSelectedTune->m_stPatternData[x][c][i].Instrument	= ImportedNote.Instrument;
							m_pSelectedTune->m_stPatternData[x][c][i].Note			= ImportedNote.Note;
							m_pSelectedTune->m_stPatternData[x][c][i].Octave		= ImportedNote.Octave;
							m_pSelectedTune->m_stPatternData[x][c][i].Vol			= 0;

							if (m_pSelectedTune->m_stPatternData[x][c][i].Note == 0)
								m_pSelectedTune->m_stPatternData[x][c][i].Instrument = MAX_INSTRUMENTS;
							if (m_pSelectedTune->m_stPatternData[x][c][i].Vol == 0)
								m_pSelectedTune->m_stPatternData[x][c][i].Vol = 0x10;
*/
						}
					}
				}
				break;

			case FB_DSAMPLES:
				memset(m_DSamples, 0, sizeof(CDSample) * MAX_DSAMPLES);
				pOpenFile->Read(&ReadCount, sizeof(int));
				for (i = 0; i < ReadCount; i++) {
					pOpenFile->Read(&ImportedDSample, sizeof(stDSampleImport));
					if (ImportedDSample.SampleSize != 0 && ImportedDSample.SampleSize < 0x4000) {
						ImportedDSample.SampleData = new char[ImportedDSample.SampleSize];
						pOpenFile->Read(ImportedDSample.SampleData, ImportedDSample.SampleSize);
					}
					else
						ImportedDSample.SampleData = NULL;
					memcpy(m_DSamples[i].Name, ImportedDSample.Name, 256);
					m_DSamples[i].SampleSize = ImportedDSample.SampleSize;
					m_DSamples[i].SampleData = ImportedDSample.SampleData;
				}
				break;

			case FB_SONGNAME:
				pOpenFile->Read(m_strName, sizeof(char) * 32);
				break;

			case FB_SONGARTIST:
				pOpenFile->Read(m_strArtist, sizeof(char) * 32);
				break;
		
			case FB_SONGCOPYRIGHT:
				pOpenFile->Read(m_strCopyright, sizeof(char) * 32);
				break;
			
			default:
				FileBlock = FB_EOF;
		}
	}

	ReorderSequences();
	ConvertSequences();

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
	CComBSTR		ErrorMsg;

	CMainFrame	*MainFrame;
	POSITION	Pos = GetFirstViewPosition();

	MainFrame = (CMainFrame*)GetNextView(Pos)->GetParentFrame();

	if (!OpenFile.Open(lpszPathName, CFile::modeRead)) {
		theApp.DisplayError(IDS_FILE_OPEN_ERROR);
		OnNewDocument();
		return FALSE;
	}

	// Read header to validate file

	OpenFile.Read(Text, int(strlen(FILE_HEADER)));

	if (memcmp(Text, FILE_HEADER, strlen(FILE_HEADER)) != 0) {
		theApp.DisplayError(IDS_FILE_VALID_ERROR);
		OnNewDocument();
		return FALSE;
	}

	OpenFile.Read(&Version, sizeof(int));

	if (Version < 0x0200) {
		if (Version < COMPATIBLE_VER) {
			theApp.DisplayError(IDS_FILE_VERSION_ERROR);
			return FALSE;
		}

		if (!LoadOldFile(&OpenFile)) {
			OpenFile.Close();
			return FALSE;
		}

		OpenFile.Close();

		// Create a backup of this file, since it's an old version
		if (theApp.m_pSettings->General.bBackups) {
			char BackupFile[MAX_PATH];
			strcpy(BackupFile, lpszPathName);
			strcat(BackupFile, ".bak");
			CopyFile(lpszPathName, BackupFile, FALSE);
		}

		theApp.SetDocumentLoaded(true);
		UpdateAllViews(NULL);

		return TRUE;
	}
	else if (Version >= 0x0200) {
		OpenFile.Close();

		if (!OpenDocument(lpszPathName))
			OnNewDocument();

		theApp.SetDocumentLoaded(true);

		return TRUE;
	}

	theApp.SetDocumentLoaded(false);
	UpdateAllViews(NULL);
	UpdateAllViews(NULL, CHANGED_CLEAR_SEL);

	return FALSE;
}

BOOL CFamiTrackerDoc::OpenDocument(LPCTSTR lpszPathName)
{
	CDocumentFile DocumentFile;
	char *BlockID;
	bool FileFinished = false;
	bool ErrorFlag = false;

#ifdef _DEBUG
	int _msgs_ = 0;
#endif

	if (!DocumentFile.Open(lpszPathName, CFile::modeRead)) {
		theApp.DisplayError(IDS_FILE_OPEN_ERROR);
		return FALSE;
	}

	if (!DocumentFile.CheckValidity()) {
		theApp.DisplayError(IDS_FILE_VALID_ERROR);
		DocumentFile.Close();
		return FALSE;
	}

	// From version 2.0, all files should be compatible (though individual blocks may not)
	if (DocumentFile.GetFileVersion() < 0x0200) {
		theApp.DisplayError(IDS_FILE_VERSION_ERROR);
		DocumentFile.Close();
		return FALSE;
	}
	
	if (DocumentFile.GetFileVersion() < 0x0210) {
		// This has to be done for older files
		SwitchToTrack(0);
	}

	// File version checking
	m_iFileVersion = DocumentFile.GetFileVersion();

	// Read all blocks
	while (!DocumentFile.Finished() && !FileFinished && !ErrorFlag) {
		DocumentFile.ReadBlock();
		BlockID = DocumentFile.GetBlockHeaderID();

		if (!strcmp(BlockID, FILE_BLOCK_PARAMS)) {
			if (DocumentFile.GetBlockVersion() == 1) {
				m_pSelectedTune->m_iSongSpeed = DocumentFile.GetBlockInt();
			}
			else
				m_cExpansionChip = DocumentFile.GetBlockChar();

			m_iChannelsAvailable	= DocumentFile.GetBlockInt();
			m_iMachine				= DocumentFile.GetBlockInt();
			m_iEngineSpeed			= DocumentFile.GetBlockInt();

			// This is strange. Sometimes expansion chip is set to 0xFF in files
			if (m_iChannelsAvailable == 5)
				m_cExpansionChip = 0;

			SelectExpansionChip(m_cExpansionChip);

			if (m_iFileVersion == 0x0200) {
				if (m_pSelectedTune->m_iSongSpeed < 20)
					m_pSelectedTune->m_iSongSpeed++;
			}

			if (DocumentFile.GetBlockVersion() == 1) {
				if (m_pSelectedTune->m_iSongSpeed > 19) {
					m_pSelectedTune->m_iSongTempo = m_pSelectedTune->m_iSongSpeed;
					m_pSelectedTune->m_iSongSpeed = 6;
				}
				else {
					if (m_iMachine == NTSC)
						m_pSelectedTune->m_iSongTempo = 150;
					else
						m_pSelectedTune->m_iSongTempo = 125;
				}
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
		else if (!strcmp(BlockID, FILE_BLOCK_SEQUENCES_VRC6)) {
			ErrorFlag = ReadBlock_SequencesVRC6(&DocumentFile);
		}
		else if (!strcmp(BlockID, "END")) {
			FileFinished = true;
		}
		else {
			// This shouldn't show up in release (debug only)
#ifdef _DEBUG
			_msgs_++;
			if (_msgs_ < 5)
				AfxMessageBox("Unknown file block!");
#endif
		}

		//ASSERT(ErrorFlag == false);
	}

	DocumentFile.Close();

	if (ErrorFlag) {
		theApp.DisplayError(IDS_FILE_LOAD_ERROR);
		DeleteContents();
		return FALSE;
	}

	// Make sure a backup is taken if it's an old version
	if (m_iFileVersion < FILE_VER && theApp.m_pSettings->General.bBackups) {
		CString BakName;
		BakName.Format("%s.bak", lpszPathName);
		CopyFile(lpszPathName, BakName.GetBuffer(), FALSE);
	}

	if (m_iFileVersion <= 0x0201)
		ReorderSequences();

	if (m_iFileVersion < 0x0300)
		ConvertSequences();

	// Always start with track 1
	SwitchToTrack(0);

	// Everything done
	m_bFileLoaded = true;

//	m_iChannelsAvailable = 5;

	return TRUE;
}

bool CFamiTrackerDoc::ReadBlock_Header(CDocumentFile *pDocFile)
{
	int Version = pDocFile->GetBlockVersion();

	if (Version == 1) {
/*
		m_pTunes[0] = new CTune;
		m_pTunes[0]->ClearPattern(m_iChannelsAvailable);
*/
		SwitchToTrack(0);

		for (unsigned int i = 0; i < m_iChannelsAvailable; i++) {
			pDocFile->GetBlockChar();							// Channel type (unused)
			m_pSelectedTune->m_iEffectColumns[i] = pDocFile->GetBlockChar();		// Effect columns
		}

	}
	else if (Version >= 2) {

		unsigned int i, j, TrackCount;

		TrackCount = pDocFile->GetBlockChar();

		// Add tracks to document
		for (i = 0; i <= TrackCount; i++) {
			//SwitchToTrack(i);
			AllocateSong(i);
		}

		if (Version == 3) {
			for (i = 0; i <= TrackCount; i++) {
				m_sTrackNames[i] = pDocFile->ReadString();
			}
		}

		m_iTracks = TrackCount;

		for (i = 0; i < m_iChannelsAvailable; i++) {
			pDocFile->GetBlockChar();							// Channel type (unused)
			for (j = 0; j <= TrackCount; j++) {
				m_pTunes[j]->m_iEffectColumns[i] = pDocFile->GetBlockChar();		// Effect columns
			}
		}
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
	char Name[256];
	CInstrument/*2A03*/ *pInst;

	// Number of instruments
	Count = pDocFile->GetBlockInt();

	ASSERT_FILE_DATA(Count < MAX_INSTRUMENTS);

	for (i = 0; i < Count; i++) {
		// instrument index
		x = pDocFile->GetBlockInt();
		ASSERT_FILE_DATA(x <= MAX_INSTRUMENTS);

		// Instrument type
		Type = pDocFile->GetBlockChar();

		switch (Type) {
			case INST_2A03: 
			{
				pInst = new CInstrument2A03();
				CInstrument2A03 *pInstrument = (CInstrument2A03*)pInst;

				SeqCnt = pDocFile->GetBlockInt();
				ASSERT_FILE_DATA(SeqCnt < (MOD_COUNT + 1));

				for (y = 0; y < SeqCnt; y++) {
					pInstrument->SetModEnable(y, pDocFile->GetBlockChar());
					Index = pDocFile->GetBlockChar();
					ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
					pInstrument->SetModIndex(y, Index);
				}

				if (Version == 1)
					Octaves = 6;
				else if (Version == 2)
					Octaves = OCTAVE_RANGE;

				for (j = 0; j < Octaves; j++) {
					for (k = 0; k < 12; k++) {
						Index = pDocFile->GetBlockChar();
						if (Index >= MAX_DSAMPLES)
							Index = 0;
						pInstrument->SetSample(j, k, Index);
						pInstrument->SetSamplePitch(j, k, pDocFile->GetBlockChar());
					}
				}
			}
			break;
			case INST_VRC6: 
			{
				pInst = new CInstrumentVRC6();
				CInstrumentVRC6 *pInstrument = (CInstrumentVRC6*)pInst;

				SeqCnt = pDocFile->GetBlockInt();
				ASSERT_FILE_DATA(SeqCnt < (MOD_COUNT + 1));

				SeqCnt = MOD_COUNT;

				for (y = 0; y < SeqCnt; y++) {
					pInstrument->SetModEnable(y, pDocFile->GetBlockChar());
					Index = pDocFile->GetBlockChar();
					ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
					pInstrument->SetModIndex(y, Index);
				}
			}
			break;
		}

		// Read name
		y = pDocFile->GetBlockInt();
		ASSERT_FILE_DATA(y < 256);

		pDocFile->GetBlock(Name, y);
		Name[y] = 0;
		pInst->SetName(Name);

		// Store instrument
		m_pInstruments[x] = pInst;
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Sequences(CDocumentFile *pDocFile)
{
	unsigned int i, x, Count = 0, Index, Type;
	unsigned int LoopPoint;
	unsigned char SeqCount;
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
			/*
			if (Index > MAX_SEQUENCES)
				return false;
			*/
			ASSERT_FILE_DATA(Index < MAX_SEQUENCES);

//			if (Index >= MAX_SEQUENCES)
//				Index = (MAX_SEQUENCES - 1);

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
	else if (Version == 3) {
		for (i = 0; i < Count; i++) {
			Index	  = pDocFile->GetBlockInt();
			Type	  = pDocFile->GetBlockInt();
			SeqCount  = pDocFile->GetBlockChar();
			LoopPoint = pDocFile->GetBlockInt();

			ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
			ASSERT_FILE_DATA(Type < MOD_COUNT);
			ASSERT_FILE_DATA(SeqCount <= MAX_SEQUENCE_ITEMS);

			m_NewSequences[Index][Type].Clear();
			m_NewSequences[Index][Type].SetItemCount(SeqCount);
			m_NewSequences[Index][Type].SetLoopPoint(LoopPoint);
			
			for (x = 0; x < SeqCount; x++) {
				Value = pDocFile->GetBlockChar();
				m_NewSequences[Index][Type].SetItem(x, Value);
			}
		}
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Frames(CDocumentFile *pDocFile)
{
	unsigned int Version = pDocFile->GetBlockVersion();
	unsigned int i, x, y, Frame;

	if (Version == 1) {

		m_pSelectedTune->m_iFrameCount = pDocFile->GetBlockInt();
		m_iChannelsAvailable = pDocFile->GetBlockInt();

		ASSERT_FILE_DATA(m_pSelectedTune->m_iFrameCount <= MAX_FRAMES);
		ASSERT_FILE_DATA(m_iChannelsAvailable <= MAX_CHANNELS);

		for (i = 0; i < m_pSelectedTune->m_iFrameCount; i++) {
			for (x = 0; x < m_iChannelsAvailable; x++) {
				Frame = pDocFile->GetBlockChar();
				ASSERT_FILE_DATA(Frame < MAX_FRAMES);
				m_pSelectedTune->m_iFrameList[i][x] = Frame;
			}
		}

	}
	else if (Version > 1){
/*
		m_iChannelsAvailable = pDocFile->GetBlockInt();
		ASSERT_FILE_DATA(m_iChannelsAvailable <= MAX_CHANNELS);
*/
		for (y = 0; y <= m_iTracks; y++) {

			m_pTunes[y]->m_iFrameCount = pDocFile->GetBlockInt();
			//m_pTunes[y]->m_iSongSpeed = pDocFile->GetBlockInt();
			int Speed = pDocFile->GetBlockInt();
			
			if (Version == 3) {
				m_pTunes[y]->m_iSongTempo = pDocFile->GetBlockInt();
				m_pTunes[y]->m_iSongSpeed = Speed;
			}
			else {
				if (Speed < 20) {
					if (m_iMachine == NTSC)
						m_pTunes[y]->m_iSongTempo = 150;
					else if (m_iMachine == PAL)
						m_pTunes[y]->m_iSongTempo = 125;
					m_pTunes[y]->m_iSongSpeed = Speed;
				}
				else {
					m_pTunes[y]->m_iSongTempo = Speed;
					m_pTunes[y]->m_iSongSpeed = 6;
				}
			}

			m_pTunes[y]->m_iPatternLength = pDocFile->GetBlockInt();

			ASSERT_FILE_DATA(m_pTunes[y]->m_iFrameCount <= MAX_FRAMES);
			ASSERT_FILE_DATA(m_pTunes[y]->m_iPatternLength <= MAX_PATTERN_LENGTH);

			for (i = 0; i < m_pTunes[y]->m_iFrameCount; i++) {
				for (x = 0; x < m_iChannelsAvailable; x++) {
					Frame = pDocFile->GetBlockChar();
					ASSERT_FILE_DATA(Frame < MAX_FRAMES);
					m_pTunes[y]->m_iFrameList[i][x] = Frame;
				}
			}
		}
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Patterns(CDocumentFile *pDocFile)
{
	unsigned char Item;

	unsigned int Channel, Pattern, i, Items, Track;
	unsigned int Version = pDocFile->GetBlockVersion();

	if (Version == 1) {
		m_pSelectedTune->m_iPatternLength = pDocFile->GetBlockInt();
		ASSERT_FILE_DATA(m_pSelectedTune->m_iPatternLength <= MAX_PATTERN_LENGTH);
	}

	while (!pDocFile->BlockDone()) {
		if (Version > 1)
			Track = pDocFile->GetBlockInt();
		else if (Version == 1)
			Track = 0;

		Channel = pDocFile->GetBlockInt();
		Pattern = pDocFile->GetBlockInt();
		Items	= pDocFile->GetBlockInt();

		ASSERT_FILE_DATA(Track < MAX_TRACKS);
		ASSERT_FILE_DATA(Channel < MAX_CHANNELS);
		ASSERT_FILE_DATA(Pattern < MAX_PATTERN);
		ASSERT_FILE_DATA((Items - 1) < MAX_PATTERN_LENGTH);

		SwitchToTrack(Track);

		for (i = 0; i < Items; i++) {
			if (m_iFileVersion == 0x0200)
				Item = pDocFile->GetBlockChar();
			else
				Item = pDocFile->GetBlockInt();

			ASSERT_FILE_DATA(Item < MAX_PATTERN_LENGTH);

			stChanNote *Note = m_pSelectedTune->GetPatternData(Channel, Pattern, Item);
			memset(Note, 0, sizeof(stChanNote));
//			memset(&m_pSelectedTune->m_stPatternData[Channel][Pattern][Item], 0, sizeof(stChanNote));

			Note->Note		= pDocFile->GetBlockChar();
			Note->Octave		= pDocFile->GetBlockChar();
			Note->Instrument	= pDocFile->GetBlockChar();
			Note->Vol		= pDocFile->GetBlockChar();

/*
			m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].Note		= pDocFile->GetBlockChar();
			m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].Octave		= pDocFile->GetBlockChar();
			m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].Instrument	= pDocFile->GetBlockChar();
			m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].Vol		= pDocFile->GetBlockChar();
*/

			if (m_iFileVersion == 0x0200) {
				unsigned char EffectNumber, EffectParam;
				EffectNumber = pDocFile->GetBlockChar();
				EffectParam = pDocFile->GetBlockChar();
				if (Version < 3) {
					if (EffectNumber == EF_PORTAOFF) {
						EffectNumber = EF_PORTAMENTO;
						EffectParam = 0;
					}
					else if (EffectNumber == EF_PORTAMENTO) {
						if (EffectParam < 0xFF)
							EffectParam++;
					}
				}

				stChanNote *Note = m_pSelectedTune->GetPatternData(Channel, Pattern, Item);

				Note->EffNumber[0]	= EffectNumber;
				Note->EffParam[0]	= EffectParam;

				/*
				m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].EffNumber[0]	= EffectNumber;
				m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].EffParam[0]	= EffectParam;
				*/
			}
			else {
				for (unsigned int n = 0; n < (m_pSelectedTune->m_iEffectColumns[Channel] + 1); n++) {
					unsigned char EffectNumber, EffectParam;
					EffectNumber = pDocFile->GetBlockChar();
					EffectParam = pDocFile->GetBlockChar();
					if (Version < 3) {
						if (EffectNumber == EF_PORTAOFF) {
							EffectNumber = EF_PORTAMENTO;
							EffectParam = 0;
						}
						else if (EffectNumber == EF_PORTAMENTO) {
							if (EffectParam < 0xFF)
								EffectParam++;
						}
					}
//					stChanNote *Note = m_pSelectedTune->GetPatternData(Channel, Pattern, Item);
					Note->EffNumber[n]	= EffectNumber;
					Note->EffParam[n] 	= EffectParam;
					/*
					m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].EffNumber[n]	= EffectNumber;
					m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].EffParam[n] 	= EffectParam;
					*/
				}
			}

			if (Note->Vol > 0x10)
				Note->Vol &= 0x0F;

				/*
			if (m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].Vol > 0x10)
				m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].Vol &= 0x0F;
				*/

			// Specific for version 2.0
			if (m_iFileVersion == 0x0200) {

				if (Note->EffNumber[0] == EF_SPEED && Note->EffParam[0] < 20)
					Note->EffParam[0]++;
				
				if (Note->Vol == 0)
					Note->Vol = 0x10;
				else {
					Note->Vol--;
					Note->Vol &= 0x0F;
				}

				if (Note->Note == 0)
					Note->Instrument = MAX_INSTRUMENTS;

				/*
				if (m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].EffNumber[0] == EF_SPEED && m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].EffParam[0] < 20)
					m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].EffParam[0]++;
				
				if (m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].Vol == 0)
					m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].Vol = 0x10;
				else {
					m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].Vol--;
					m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].Vol &= 0x0F;
				}

				if (m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].Note == 0)
					m_pSelectedTune->m_stPatternData[Channel][Pattern][Item].Instrument = MAX_INSTRUMENTS;
					*/
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
	ASSERT_FILE_DATA(Count <= MAX_DSAMPLES);
	
	memset(m_DSamples, 0, sizeof(CDSample) * MAX_DSAMPLES);

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

bool CFamiTrackerDoc::ReadBlock_SequencesVRC6(CDocumentFile *pDocFile)
{
	unsigned int i, x, Count = 0, Index, Type;
	unsigned int LoopPoint;
	unsigned char SeqCount;
	int Version = pDocFile->GetBlockVersion();
	char Value;

	Count = pDocFile->GetBlockInt();
	ASSERT_FILE_DATA(Count < MAX_SEQUENCES);

	for (i = 0; i < Count; i++) {
		Index	  = pDocFile->GetBlockInt();
		Type	  = pDocFile->GetBlockInt();
		SeqCount  = pDocFile->GetBlockChar();
		LoopPoint = pDocFile->GetBlockInt();

		ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
		ASSERT_FILE_DATA(Type < MOD_COUNT);
		ASSERT_FILE_DATA(SeqCount <= MAX_SEQUENCE_ITEMS);

		m_SequencesVRC6[Index][Type].Clear();
		m_SequencesVRC6[Index][Type].SetItemCount(SeqCount);
		m_SequencesVRC6[Index][Type].SetLoopPoint(LoopPoint);
		
		for (x = 0; x < SeqCount; x++) {
			Value = pDocFile->GetBlockChar();
			m_SequencesVRC6[Index][Type].SetItem(x, Value);
		}
	}

	return false;
}


void CFamiTrackerDoc::ConvertSequences()
{
	int i, j, k;

	int iLength, ValPtr, Count, Value, Length;

	stSequence	*pSeq;
	CSequence	*pNewSeq;

	for (i = 0; i < MAX_SEQUENCES; i++) {
		for (j = 0; j < MAX_SEQ_ITEMS; j++) {

			pSeq = &m_Sequences[i][j];
			pNewSeq = &m_NewSequences[i][j];

			if (pSeq->Count > 0 && pSeq->Count < MAX_SEQ_ITEMS) {

				// Save a pointer to this
				int iLoopPoint = -1;
				iLength = 0;
				ValPtr = 0;

				// Store the sequence
				Count = pSeq->Count;
				for (k = 0; k < Count; k++) {
					Value	= pSeq->Value[k];
					Length	= pSeq->Length[k];

					if (Length < 0) {
						iLoopPoint = 0;
						for (int l = signed(pSeq->Count) + Length - 1; l < signed(pSeq->Count) - 1; l++)
							iLoopPoint += (pSeq->Length[l] + 1);
					}
					else {
						for (int l = 0; l < Length + 1; l++) {
							if ((j == MOD_PITCH || j == MOD_HIPITCH) && l > 0)
								pNewSeq->SetItem(ValPtr++, 0);
							else
								pNewSeq->SetItem(ValPtr++, (unsigned char)Value);
							iLength++;
						}
					}

				}

				if (iLoopPoint != -1) {
					if (iLoopPoint > iLength)
						iLoopPoint = iLength;
					iLoopPoint = iLength - iLoopPoint;
				}

				pNewSeq->SetItemCount(ValPtr);
				pNewSeq->SetLoopPoint(iLoopPoint);
			}

		}
	}

}

// End of file load/save

CDSample *CFamiTrackerDoc::GetDSample(unsigned int Index)
{
	ASSERT(Index < MAX_DSAMPLES);
	return &m_DSamples[Index];
}

void CFamiTrackerDoc::GetSampleName(unsigned int Index, char *Name)
{
	ASSERT(Index < MAX_DSAMPLES);
	ASSERT(m_DSamples[Index].SampleSize > 0);
	strcpy(Name, m_DSamples[Index].Name);
}

// DMC Stuff below

CDSample *CFamiTrackerDoc::GetFreeDSample()
{
	int i;

	for (i = 0; i < MAX_DSAMPLES; i++) {
		if (m_DSamples[i].SampleSize == 0) {
			return &m_DSamples[i];
		}
	}

	return NULL;
}

void CFamiTrackerDoc::RemoveDSample(unsigned int Index)
{
	ASSERT(Index < MAX_DSAMPLES);

	if (m_DSamples[Index].SampleSize != 0) {
		delete [] m_DSamples[Index].SampleData;
		m_DSamples[Index].SampleSize = 0;
		m_DSamples[Index].SampleData = NULL;
		SetModifiedFlag();
	}
}

void CFamiTrackerDoc::OnCloseDocument()
{	
	theApp.SetDocumentLoaded(false);
	CDocument::OnCloseDocument();
}

// ---------------------------------------------------------------------------------------------------------
// Document access functions
// ---------------------------------------------------------------------------------------------------------

void CFamiTrackerDoc::SetSongInfo(char *Name, char *Artist, char *Copyright)
{
	ASSERT(Name != NULL && Artist != NULL && Copyright != NULL);

	strcpy(m_strName, Name);
	strcpy(m_strArtist, Artist);
	strcpy(m_strCopyright, Copyright);

	SetModifiedFlag();
}

// Sequences

CSequence *CFamiTrackerDoc::GetSequence(int Index, int Type)
{
	ASSERT(Index >= 0 && Index < MAX_SEQUENCES);
	ASSERT(Type >= 0 && Type < MOD_COUNT);

	return &m_NewSequences[Index][Type];
	//return &m_Sequences[Index][Type];
}

int CFamiTrackerDoc::GetSequenceCount(int Index, int Type)
{
	ASSERT(Index >= 0 && Index < MAX_SEQUENCES);
	ASSERT(Type >= 0 && Type < MOD_COUNT);

	return m_NewSequences[Index][Type].GetItemCount();
}

CSequence *CFamiTrackerDoc::GetSequenceVRC6(int Index, int Type)
{
	ASSERT(Index >= 0 && Index < MAX_SEQUENCES);
	ASSERT(Type >= 0 && Type < MOD_COUNT);

	return &m_SequencesVRC6[Index][Type];
}

int CFamiTrackerDoc::GetSequenceCountVRC6(int Index, int Type)
{
	ASSERT(Index >= 0 && Index < MAX_SEQUENCES);
	ASSERT(Type >= 0 && Type < MOD_COUNT);

	return m_SequencesVRC6[Index][Type].GetItemCount();
}

//
// Instruments
//

CInstrument *CFamiTrackerDoc::GetInstrument(int Instrument)
{
	ASSERT(Instrument >= 0 && Instrument < MAX_INSTRUMENTS);

	if (m_pInstruments[Instrument] == NULL)
		return NULL;

	return m_pInstruments[Instrument];
}

bool CFamiTrackerDoc::IsInstrumentUsed(int Instrument)
{
	ASSERT(Instrument >= 0 && Instrument < MAX_INSTRUMENTS);

	return !(m_pInstruments[Instrument] == NULL);
}

unsigned int CFamiTrackerDoc::AddInstrument(const char *Name, int Type)
{
	unsigned int i;

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (m_pInstruments[i] == NULL)
			break;
	}

	if (i == MAX_INSTRUMENTS)
		return -1;

	switch (Type) {
		case CHIP_NONE:
		case CHIP_MMC5:
			m_pInstruments[i] = new CInstrument2A03();
			break;
		case CHIP_VRC6:
			m_pInstruments[i] = new CInstrumentVRC6();
			break;
		case CHIP_VRC7:
			m_pInstruments[i] = new CInstrumentVRC7();
			break;
#ifdef DEBUG
		default:
			MessageBox(NULL, "(todo) add instrument definitions for this chip", "Stop", MB_OK);
#endif
	}

	switch (Type) {
		case CHIP_NONE:
		case CHIP_MMC5:
			for (unsigned int c = 0; c < MOD_COUNT; c++) {
				((CInstrument2A03*)m_pInstruments[i])->SetModEnable(c, 0);
				((CInstrument2A03*)m_pInstruments[i])->SetModIndex(c, 0);
			}
			break;
	}

	m_pInstruments[i]->SetName((char*)Name);

	SetModifiedFlag();
	UpdateAllViews(NULL, UPDATE_ENTIRE);

	return i;
}

void CFamiTrackerDoc::RemoveInstrument(unsigned int Index)
{
	ASSERT(Index < MAX_INSTRUMENTS);
	
	if (m_pInstruments[Index] == NULL)
		return;

	delete m_pInstruments[Index];
	m_pInstruments[Index] = NULL;

	SetModifiedFlag();
	UpdateAllViews(NULL, UPDATE_ENTIRE);
}

void CFamiTrackerDoc::GetInstrumentName(unsigned int Index, char *Name)
{
	ASSERT(Index < MAX_INSTRUMENTS);
	m_pInstruments[Index]->GetName(Name);
}

void CFamiTrackerDoc::SetInstrumentName(unsigned int Index, char *Name)
{
	ASSERT(Index < MAX_INSTRUMENTS);

	if (m_pInstruments[Index] == NULL)
		return;

	m_pInstruments[Index]->SetName(Name);

	SetModifiedFlag();
}

//
// DPCM samples
//

int CFamiTrackerDoc::GetSampleSize(unsigned int Sample)
{
	ASSERT(Sample < MAX_DSAMPLES);

	return m_DSamples[Sample].SampleSize;
}

char CFamiTrackerDoc::GetSampleData(unsigned int Sample, unsigned int Offset)
{
	ASSERT(Sample < MAX_DSAMPLES);

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

	if (m_pSelectedTune->m_iFrameCount != Count) {
		m_pSelectedTune->m_iFrameCount = Count;
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::SetPatternLength(unsigned int Length)
{ 
	ASSERT(Length <= MAX_PATTERN_LENGTH);

	if (m_pSelectedTune->m_iPatternLength != Length) {
		m_pSelectedTune->m_iPatternLength = Length;
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::SetSongSpeed(unsigned int Speed)
{
	ASSERT(Speed <= MAX_TEMPO);
	
	if (m_pSelectedTune->m_iSongSpeed != Speed) {
		m_pSelectedTune->m_iSongSpeed = Speed;
		SetModifiedFlag();
	}
}

void CFamiTrackerDoc::SetSongTempo(unsigned int Tempo)
{
	ASSERT(Tempo <= MAX_TEMPO);

	if (m_pSelectedTune->m_iSongTempo != Tempo) {
		m_pSelectedTune->m_iSongTempo = Tempo;
		SetModifiedFlag();
	}
}

unsigned int CFamiTrackerDoc::GetEffColumns(unsigned int Channel) const
{
	ASSERT(Channel < MAX_CHANNELS);
	return m_pSelectedTune->m_iEffectColumns[Channel];
}

void CFamiTrackerDoc::SetEffColumns(unsigned int Channel, unsigned int Columns)
{
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Columns < MAX_EFFECT_COLUMNS);

	m_pSelectedTune->m_iEffectColumns[Channel] = Columns;

	SetModifiedFlag();
	UpdateAllViews(NULL, CHANGED_ERASE);	// Erase background to calculate new pattern editor width
}

void CFamiTrackerDoc::SetEngineSpeed(unsigned int Speed)
{
	ASSERT(Speed <= 800); // hardcoded for now
	ASSERT(Speed >= 10 || Speed == 0);

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

	return m_pSelectedTune->m_iFrameList[Frame][Channel];
}

void CFamiTrackerDoc::SetPatternAtFrame(unsigned int Frame, unsigned int Channel, unsigned int Pattern)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Pattern < MAX_PATTERN);
	m_pSelectedTune->m_iFrameList[Frame][Channel] = Pattern;
	SetModifiedFlag();
}

unsigned int CFamiTrackerDoc::GetFrameRate(void) const
{
	if (m_iEngineSpeed == 0) {
		if (m_iMachine == NTSC)
			return FRAMERATE_NTSC;
		else
			return FRAMERATE_PAL;
	}
	
	return m_iEngineSpeed;
}

//
// Pattern editing
//

void CFamiTrackerDoc::IncreasePattern(unsigned int Frame, unsigned int Channel, int Count)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);

	// Selects the next channel pattern
	if ((m_pSelectedTune->m_iFrameList[Frame][Channel] + Count) < (MAX_PATTERN - 1)) {
		m_pSelectedTune->m_iFrameList[Frame][Channel] += Count;
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
	else {
		m_pSelectedTune->m_iFrameList[Frame][Channel] = (MAX_PATTERN - 1);
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::DecreasePattern(unsigned int Frame, unsigned int Channel, int Count)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);

	// Selects the previous channel pattern
	if (m_pSelectedTune->m_iFrameList[Frame][Channel] > Count) {
		m_pSelectedTune->m_iFrameList[Frame][Channel] -= Count;
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
	else {
		m_pSelectedTune->m_iFrameList[Frame][Channel] = 0;
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::IncreaseInstrument(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	unsigned int Inst = m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->Instrument;

	if (Inst < MAX_INSTRUMENTS) {
		m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->Instrument = Inst + 1;
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::DecreaseInstrument(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	unsigned int Inst = m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->Instrument;

	if (Inst > 0) {
		m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->Instrument = Inst - 1;
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::IncreaseVolume(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	unsigned int Vol = m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->Vol;

	if (Vol < 0x10) {
		m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->Vol = Vol + 1;
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::DecreaseVolume(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	unsigned int Vol = m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->Vol;

	if (Vol > 1) {
		m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->Vol = Vol - 1;
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

	unsigned int Effect = m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->EffParam[Index];
	
	if (Effect < 256) {
		m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->EffParam[Index] = Effect + 1;
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

	unsigned int Effect = m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->EffParam[Index];

	if (Effect > 0) {
		m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->EffParam[Index] = Effect - 1;
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
	memcpy(m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row), Data, sizeof(stChanNote));
	SetModifiedFlag();
}

void CFamiTrackerDoc::GetNoteData(unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *Data) const
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	unsigned int Pattern = GET_PATTERN(Frame, Channel);

	// Sets the notes of the pattern
//	memcpy(Data, &m_pSelectedTune->m_stPatternData[Channel][Pattern][Row], sizeof(stChanNote));
	memcpy(Data, m_pSelectedTune->GetPatternData(Channel, Pattern, Row), sizeof(stChanNote));
}

void CFamiTrackerDoc::SetDataAtPattern(unsigned int Track, unsigned int Pattern, unsigned int Channel, unsigned int Row, stChanNote *Data)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Pattern < MAX_PATTERN);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	// Set a note to a direct pattern
	memcpy(m_pTunes[Track]->GetPatternData(Channel, Pattern, Row), Data, sizeof(stChanNote));
	SetModifiedFlag();
}

void CFamiTrackerDoc::GetDataAtPattern(unsigned int Track, unsigned int Pattern, unsigned int Channel, unsigned int Row, stChanNote *Data) const
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Pattern < MAX_PATTERN);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	// Get note from a direct pattern
	//memcpy(Data, &m_pTunes[Track]->m_stPatternData[Channel][Pattern][Row], sizeof(stChanNote));
	memcpy(Data, m_pTunes[Track]->GetPatternData(Channel, Pattern, Row), sizeof(stChanNote));
}

unsigned int CFamiTrackerDoc::GetNoteEffectType(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);
	ASSERT(Index < MAX_EFFECT_COLUMNS);

	return m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->EffNumber[Index];
	//return m_pSelectedTune->m_stPatternData[Channel][GET_PATTERN(Frame, Channel)][Row].EffNumber[Index];
}

unsigned int CFamiTrackerDoc::GetNoteEffectParam(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);
	ASSERT(Index < MAX_EFFECT_COLUMNS);

	return m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->EffParam[Index];
//	return m_pSelectedTune->m_stPatternData[Channel][GET_PATTERN(Frame, Channel)][Row].EffParam[Index];
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

	for (unsigned int i = m_pSelectedTune->m_iPatternLength - 1; i > Row; i--) {
		memcpy(m_pSelectedTune->GetPatternData(Channel, m_pSelectedTune->m_iFrameList[Frame][Channel], i), m_pSelectedTune->GetPatternData(Channel, m_pSelectedTune->m_iFrameList[Frame][Channel], i - 1), sizeof(stChanNote));
	}

	*m_pSelectedTune->GetPatternData(Channel, m_pSelectedTune->m_iFrameList[Frame][Channel], Row) = Note;

	SetModifiedFlag();

	return true;
}

bool CFamiTrackerDoc::DeleteNote(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Column)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	stChanNote *Note;

//	Note = m_pSelectedTune->m_stPatternData[Channel][m_pSelectedTune->m_iFrameList[Frame][Channel]][Row];
	Note = m_pSelectedTune->GetPatternData(Channel, m_pSelectedTune->m_iFrameList[Frame][Channel], Row);

	if (Column == C_NOTE) {
		Note->Note		= 0;
		Note->Octave		= 0;
		Note->Instrument = MAX_INSTRUMENTS;
		Note->Vol		= 0x10;
	}
	else if (Column == C_INSTRUMENT1 || Column == C_INSTRUMENT2) {
		Note->Instrument = MAX_INSTRUMENTS;
	}
	else if (Column == C_VOLUME) {
		Note->Vol = 0x10;
	}
	else if (Column == C_EFF_NUM || Column == C_EFF_PARAM1 || Column == C_EFF_PARAM2) {
		Note->EffNumber[0]	= 0;
		Note->EffParam[0]	= 0;
	}
	else if (Column == C_EFF2_NUM || Column == C_EFF2_PARAM1 || Column == C_EFF2_PARAM2) {
		Note->EffNumber[1]	= 0;
		Note->EffParam[1]	= 0;
	}
	else if (Column == C_EFF3_NUM || Column == C_EFF3_PARAM1 || Column == C_EFF3_PARAM2) {
		Note->EffNumber[2]	= 0;
		Note->EffParam[2]	= 0;
	}
	else if (Column == C_EFF4_NUM || Column == C_EFF4_PARAM1 || Column == C_EFF4_PARAM2) {
		Note->EffNumber[3]	= 0;
		Note->EffParam[3]	= 0;
	}
	
	SetModifiedFlag();

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

	for (i = Row - 1; i < (m_pSelectedTune->m_iPatternLength - 1); i++) {
		memcpy(m_pSelectedTune->GetPatternData(Channel, m_pSelectedTune->m_iFrameList[Frame][Channel], i), 
			m_pSelectedTune->GetPatternData(Channel, m_pSelectedTune->m_iFrameList[Frame][Channel], i + 1),
			sizeof(stChanNote));
	}

	*m_pSelectedTune->GetPatternData(Channel, m_pSelectedTune->m_iFrameList[Frame][Channel], m_pSelectedTune->m_iPatternLength - 1) = Note;

	SetModifiedFlag();

	return true;
}

void CFamiTrackerDoc::SelectTrack(unsigned int Track)
{
	ASSERT(Track < MAX_TRACKS);
	SwitchToTrack(Track);
	UpdateAllViews(0, CHANGED_TRACK);
}

void CFamiTrackerDoc::SelectTrackFast(unsigned int Track)
{
	// Called by the compiler only
	m_iTrack = Track;
	m_pSelectedTune = m_pTunes[Track];
}

void CFamiTrackerDoc::SwitchToTrack(unsigned int Track)
{
	// Select a new track, initialize if it wasn't

	m_iTrack = Track;
	AllocateSong(Track);
	m_pSelectedTune = m_pTunes[Track];
}

void CFamiTrackerDoc::AllocateSong(unsigned int Song)
{
	if (m_pTunes[Song] == NULL) {
		m_pTunes[Song] = new CPatternData;
		m_pTunes[Song]->Init(64, 1, 6, 150);
		//m_pTunes[Song]->ClearPattern(m_iChannelsAvailable);
		m_sTrackNames[Song] = "New song";
	}
}

unsigned int CFamiTrackerDoc::GetTrackCount() 
{
	return m_iTracks;
}

unsigned int CFamiTrackerDoc::GetSelectedTrack() 
{
	return m_iTrack;
}

void CFamiTrackerDoc::SelectExpansionChip(unsigned char Chip)
{
	int i = 0;

	m_cExpansionChip = Chip;
	m_iChannelsAvailable = CHANNELS_DEFAULT;

	m_iChannelTypes[i++] = CHANID_SQUARE1;
	m_iChannelTypes[i++] = CHANID_SQUARE2;
	m_iChannelTypes[i++] = CHANID_TRIANGLE;
	m_iChannelTypes[i++] = CHANID_NOISE;
	m_iChannelTypes[i++] = CHANID_DPCM;
/*
	m_iChannelTypes[i++] = CHANID_VRC6_PULSE1;
	m_iChannelTypes[i++] = CHANID_VRC6_PULSE2;
	m_iChannelTypes[i++] = CHANID_VRC6_SAWTOOTH;

	m_iChannelTypes[i++] = CHANID_MMC5_SQUARE1;
	m_iChannelTypes[i++] = CHANID_MMC5_SQUARE2;
*/
	switch (Chip) {
		//case SNDCHIP_FDS:	m_iChannelsAvailable += 1; break;
		//case SNDCHIP_FME07: m_iChannelsAvailable += 1; break;
		//case SNDCHIP_N106:	m_iChannelsAvailable += 8; break;

		case CHIP_VRC6:
			m_iChannelsAvailable += 3; 
			m_iChannelTypes[i++] = CHANID_VRC6_PULSE1;
			m_iChannelTypes[i++] = CHANID_VRC6_PULSE2;
			m_iChannelTypes[i++] = CHANID_VRC6_SAWTOOTH;
			break;

		case CHIP_VRC7:
			m_iChannelsAvailable += 6; 
			break;

		case CHIP_MMC5:	
			m_iChannelsAvailable += 2; 
			m_iChannelTypes[i++] = CHANID_MMC5_SQUARE1;
			m_iChannelTypes[i++] = CHANID_MMC5_SQUARE2;
			break;

	}


	theApp.SetSoundChip(Chip);
}

void CFamiTrackerDoc::StoreInstrument_VRC6(CFile *pFile, CInstrument *pInst)
{
	CInstrumentVRC6 *pInstrument = (CInstrumentVRC6*)pInst;

	// Sequences
	unsigned char SeqCount = MOD_COUNT;
	pFile->Write(&SeqCount, sizeof(char));

	for (int i = 0; i < MOD_COUNT; i++) {
		int Sequence = pInstrument->GetModIndex(i);

		if (pInstrument->GetModEnable(i)) {
			char Enabled = 1;
			pFile->Write(&Enabled, sizeof(char));

			int ItemCount = m_SequencesVRC6[Sequence][i].GetItemCount();
			pFile->Write(&ItemCount, sizeof(int));
			int LoopPoint = m_SequencesVRC6[Sequence][i].GetLoopPoint();
			pFile->Write(&LoopPoint, sizeof(int));
			for (unsigned int j = 0; j < m_SequencesVRC6[Sequence][i].GetItemCount(); j++) {
				int Value = m_SequencesVRC6[Sequence][i].GetItem(j);
				pFile->Write(&Value, sizeof(char));
			}
		}
		else {
			char Enabled = 0;
			pFile->Write(&Enabled, sizeof(char));
		}
	}
}

void CFamiTrackerDoc::StoreInstrument_2A03(CFile *pFile, CInstrument *pInst)
{
	CInstrument2A03 *pInstrument = (CInstrument2A03*)pInst;

	// Sequences
	unsigned char SeqCount = MOD_COUNT;
	pFile->Write(&SeqCount, sizeof(char));

	for (int i = 0; i < MOD_COUNT; i++) {
		int Sequence = pInstrument->GetModIndex(i);

		if (pInstrument->GetModEnable(i)) {
			char Enabled = 1;
			pFile->Write(&Enabled, sizeof(char));
			int ItemCount = m_NewSequences[Sequence][i].GetItemCount();
			pFile->Write(&ItemCount, sizeof(int));
			int LoopPoint = m_NewSequences[Sequence][i].GetLoopPoint();
			pFile->Write(&LoopPoint, sizeof(int));
			for (unsigned int j = 0; j < m_NewSequences[Sequence][i].GetItemCount(); j++) {
				int Value = m_NewSequences[Sequence][i].GetItem(j);
				pFile->Write(&Value, sizeof(char));
			}
		}
		else {
			char Enabled = 0;
			pFile->Write(&Enabled, sizeof(char));
		}
	}

	unsigned int Count = 0;

	// Count assigned keys
	for (int i = 0; i < 6; i++) {	// octaves
		for (int j = 0; j < 12; j++) {	// notes
			if (pInstrument->GetSample(i, j) > 0)
				Count++;
		}
	}

	pFile->Write(&Count, sizeof(int));

	bool UsedSamples[MAX_DSAMPLES];
	memset(UsedSamples, 0, sizeof(bool) * MAX_DSAMPLES);

	// DPCM
	for (int i = 0; i < 6; i++) {	// octaves
		for (int j = 0; j < 12; j++) {	// notes
			if (pInstrument->GetSample(i, j) > 0) {
				unsigned char Index = i * 12 + j, Sample, Pitch;
				pFile->Write(&Index, sizeof(char));
				Sample = pInstrument->GetSample(i, j);
				Pitch = pInstrument->GetSamplePitch(i, j);
				pFile->Write(&Sample, sizeof(char));
				pFile->Write(&Pitch, sizeof(char));
				UsedSamples[pInstrument->GetSample(i, j) - 1] = true;
			}
		}
	}

	int SampleCount = 0;

	// Count samples
	for (int i = 0; i < MAX_DSAMPLES; i++) {
		if (m_DSamples[i].SampleSize > 0 && UsedSamples[i])
			SampleCount++;
	}

	// Write the number
	pFile->Write(&SampleCount, sizeof(int));

	// List of sample names, the samples itself won't be stored
	for (int i = 0; i < MAX_DSAMPLES; i++) {
		if (m_DSamples[i].SampleSize > 0 && UsedSamples[i]) {
			int Len = (int)strlen(m_DSamples[i].Name);
			pFile->Write(&i, sizeof(int));
			pFile->Write(&Len, sizeof(int));
			pFile->Write(m_DSamples[i].Name, Len);
			pFile->Write(&m_DSamples[i].SampleSize, sizeof(int));
			pFile->Write(m_DSamples[i].SampleData, m_DSamples[i].SampleSize);
		}
	}
}

void CFamiTrackerDoc::SaveInstrument(unsigned int Instrument, CString FileName)
{
	CFile InstrumentFile(FileName, CFile::modeCreate | CFile::modeWrite);
	CInstrument *pInstrument = m_pInstruments[Instrument];

	ASSERT(pInstrument != NULL);

	// Write header
	InstrumentFile.Write(INST_HEADER, (UINT)strlen(INST_HEADER));
	InstrumentFile.Write(INST_VERSION, (UINT)strlen(INST_VERSION));

	// Type	
	char InstType;

	InstType = pInstrument->GetType();

	InstrumentFile.Write(&InstType, sizeof(char));

	char Name[256];
	pInstrument->GetName(Name);

	// Name
	int NameSize = (int)strlen(Name);
	InstrumentFile.Write(&NameSize, sizeof(int));
	InstrumentFile.Write(Name, NameSize);

	switch (InstType) {
		case INST_2A03:
			StoreInstrument_2A03(&InstrumentFile, pInstrument);
			break;
		case INST_VRC6:
			StoreInstrument_VRC6(&InstrumentFile, pInstrument);
			break;
	}

	InstrumentFile.Close();
}

void CFamiTrackerDoc::LoadInstrument_2A03(CFile *pFile, CInstrument *pInst, int iInstVer)
{
	CInstrument2A03 *pInstrument = (CInstrument2A03*)pInst;
	char SampleNames[MAX_DSAMPLES][256];

	// Sequences
	unsigned char SeqCount;
	pFile->Read(&SeqCount, sizeof(char));

	stSequence OldSequence;

	// Loop through all instrument effects
	for (int l = 0; l < SeqCount; l++) {

		unsigned char Enabled;
		pFile->Read(&Enabled, sizeof(char));

		if (Enabled == 1) {
			// Read the sequence
			int Count;
			pFile->Read(&Count, sizeof(int));
			for (int j = 0; j < MAX_SEQUENCES; j++) {
				// Find a free sequence
				if (m_NewSequences[j][l].GetItemCount() == 0) {
					if (iInstVer < 20) {
						OldSequence.Count = Count;
						for (int k = 0; k < Count; k++) {
							pFile->Read(&OldSequence.Length[k], sizeof(char));
							pFile->Read(&OldSequence.Value[k], sizeof(char));
						}
						ConvertSequence(&OldSequence, &m_NewSequences[j][l], l);	// convert
					}
					else {
						m_NewSequences[j][l].SetItemCount(Count);
						int LoopPoint;
						pFile->Read(&LoopPoint, sizeof(int));
						m_NewSequences[j][l].SetLoopPoint(LoopPoint);
						for (int k = 0; k < Count; k++) {
							char Val;
							pFile->Read(&Val, sizeof(char));
							m_NewSequences[j][l].SetItem(k, Val);
						}
					}
					pInstrument->SetModEnable(l, true);
					pInstrument->SetModIndex(l, j);
					break;
				}
			}
		}
		else {
			pInstrument->SetModEnable(l, false);
			pInstrument->SetModIndex(l, 0);
		}
	}

	bool SamplesFound[MAX_DSAMPLES];
	memset(SamplesFound, 0, sizeof(bool) * MAX_DSAMPLES);

	unsigned int Count;
	unsigned int j;
	pFile->Read(&Count, sizeof(int));

	// DPCM instruments
	for (j = 0; j < Count; j++) {
		int Note, Octave;
		unsigned char InstNote, Sample, Pitch;
		pFile->Read(&InstNote, sizeof(char));
		Octave = InstNote / 12;
		Note = InstNote % 12;
		pFile->Read(&Sample, sizeof(char));
		pFile->Read(&Pitch, sizeof(char));
		pInstrument->SetSamplePitch(Octave, Note, Pitch);
		pInstrument->SetSample(Octave, Note, Sample);
	}

	// DPCM samples list

	unsigned int SampleCount;

	pFile->Read(&SampleCount, sizeof(int));

	bool Found;
	int Size;
	char *SampleData;

	for (unsigned int l = 0; l < SampleCount; l++) {
		int Index, Len;
		pFile->Read(&Index, sizeof(int));
		pFile->Read(&Len, sizeof(int));
		pFile->Read(SampleNames[Index], Len);
		SampleNames[Index][Len] = 0;
		pFile->Read(&Size, sizeof(int));
		SampleData = new char[Size];
		pFile->Read(SampleData, Size);
		Found = false;
		for (int m = 0; m < MAX_DSAMPLES; m++) {
			if (m_DSamples[m].SampleSize > 0 && !strcmp(m_DSamples[m].Name, SampleNames[Index])) {
				Found = true;
				// Assign sample
				for (int n = 0; n < (6 * 12); n++) {
					if (pInstrument->GetSample(n / 6, j % 13) == (Index + 1))
						pInstrument->SetSample(n / 6, j % 13, m + 1);
				}
			}
		}
		if (!Found) {
			// Load sample
			int FreeSample;
			for (FreeSample = 0; FreeSample < MAX_DSAMPLES; FreeSample++) {
				if (m_DSamples[FreeSample].SampleSize == 0)
					break;
			}

			if (FreeSample < MAX_DSAMPLES) {
				CDSample *Sample = m_DSamples + FreeSample;
				strcpy(Sample->Name, SampleNames[Index]);
				Sample->SampleSize = Size;
				Sample->SampleData = SampleData;
				// Assign it
				for (int n = 0; n < (6 * 12); n++) {
					if (pInstrument->GetSample(n / 6, n % 13) == (Index + 1))
						pInstrument->SetSample(n / 6, n % 13, FreeSample + 1);
				}
			}
			else {
				theApp.DisplayError(IDS_OUT_OF_SLOTS);
				//AfxMessageBox("Out of sample slots");
			}
		}
		else
			delete [] SampleData;
	}
}


void CFamiTrackerDoc::LoadInstrument_VRC6(CFile *pFile, CInstrument *pInst, int iInstVer)
{
	CInstrumentVRC6 *pInstrument = (CInstrumentVRC6*)pInst;

	// Sequences
	unsigned char SeqCount;
	pFile->Read(&SeqCount, sizeof(char));

	stSequence OldSequence;

	// Loop through all instrument effects
	for (int l = 0; l < SeqCount; l++) {
		unsigned char Enabled;
		pFile->Read(&Enabled, sizeof(char));
		if (Enabled == 1) {
			// Read the sequence
			int Count;
			pFile->Read(&Count, sizeof(int));
			for (int j = 0; j < MAX_SEQUENCES; j++) {
				// Find a free sequence
				if (m_SequencesVRC6[j][l].GetItemCount() == 0) {
					if (iInstVer < 20) {
						OldSequence.Count = Count;
						for (int k = 0; k < Count; k++) {
							pFile->Read(&OldSequence.Length[k], sizeof(char));
							pFile->Read(&OldSequence.Value[k], sizeof(char));
						}
						ConvertSequence(&OldSequence, &m_SequencesVRC6[j][l], l);	// convert
					}
					else {
						m_SequencesVRC6[j][l].SetItemCount(Count);
						int LoopPoint;
						pFile->Read(&LoopPoint, sizeof(int));
						m_SequencesVRC6[j][l].SetLoopPoint(LoopPoint);
						for (int k = 0; k < Count; k++) {
							char Val;
							pFile->Read(&Val, sizeof(char));
							m_SequencesVRC6[j][l].SetItem(k, Val);
						}
					}
					pInstrument->SetModEnable(l, true);
					pInstrument->SetModIndex(l, j);
					break;
				}
			}
		}
		else {
			pInstrument->SetModEnable(l, false);
			pInstrument->SetModIndex(l, 0);
		}
	}
}

unsigned int CFamiTrackerDoc::LoadInstrument(CString FileName)
{
	CInstrument *pInstrument;
	unsigned int i, NameLen;
	char Text[256];

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (m_pInstruments[i] == NULL)
			break;
	}

	if (i == MAX_INSTRUMENTS)
		return -1;

	// Open file
	CFile InstrumentFile(FileName, CFile::modeRead);

	// Signature
	InstrumentFile.Read(Text, (UINT)strlen(INST_HEADER));

	Text[strlen(INST_HEADER)] = 0;

	if (strcmp(Text, INST_HEADER) != 0) {
		theApp.DisplayError(IDS_INSTRUMENT_FILE_FAIL);
		InstrumentFile.Close();
		return -1;
	}

	// Version
	InstrumentFile.Read(Text, (UINT)strlen(INST_VERSION));

	int iInstMaj, iInstMin, iInstVer, iCurrentVer;

	sscanf(Text, "%i.%i", &iInstMaj, &iInstMin);
	iInstVer = iInstMaj * 10 + iInstMin;
	
	sscanf(INST_VERSION, "%i.%i", &iInstMaj, &iInstMin);
	iCurrentVer = iInstMaj * 10 + iInstMin;

	if (iInstVer > iCurrentVer) {
		theApp.DoMessageBox("File version not supported!", MB_OK, 0);
		return -1;
	}

	// Type
	char InstType;
	InstrumentFile.Read(&InstType, sizeof(char));

	if (!InstType)
		InstType = INST_2A03;

	switch (InstType) {
		case INST_2A03:
			pInstrument = new CInstrument2A03();
			break;
		case INST_VRC6:
			pInstrument = new CInstrumentVRC6();
			break;
	}

	m_pInstruments[i] = pInstrument;

	// Name
	InstrumentFile.Read(&NameLen, sizeof(int));
	InstrumentFile.Read(Text, NameLen);
	Text[NameLen] = 0;

	pInstrument->SetName(Text);
	//m_Instruments[i].SetFree(false);

	switch (InstType) {
		case INST_2A03:
			LoadInstrument_2A03(&InstrumentFile, pInstrument, iInstVer);
			break;
		case INST_VRC6:
			LoadInstrument_VRC6(&InstrumentFile, pInstrument, iInstVer);
			break;
	}

	return i;
}


void CFamiTrackerDoc::ConvertSequence(stSequence *OldSequence, CSequence *NewSequence, int Type)
{
	int iLength, ValPtr, Count, Value, Length, k;

	if (OldSequence->Count > 0 && OldSequence->Count < MAX_SEQ_ITEMS) {

		// Save a pointer to this
		int iLoopPoint = -1;
		iLength = 0;
		ValPtr = 0;

		// Store the sequence
		Count = OldSequence->Count;

		for (k = 0; k < Count; k++) {
			Value	= OldSequence->Value[k];
			Length	= OldSequence->Length[k];

			if (Length < 0) {
				iLoopPoint = 0;
				for (int l = signed(OldSequence->Count) + Length - 1; l < signed(OldSequence->Count) - 1; l++)
					iLoopPoint += (OldSequence->Length[l] + 1);
			}
			else {
				for (int l = 0; l < Length + 1; l++) {
					if ((Type == MOD_PITCH || Type == MOD_HIPITCH) && l > 0)
						NewSequence->SetItem(ValPtr++, 0);
					else
						NewSequence->SetItem(ValPtr++, (unsigned char)Value);
					iLength++;
				}
			}

		}

		if (iLoopPoint != -1) {
			if (iLoopPoint > iLength)
				iLoopPoint = iLength;
			iLoopPoint = iLength - iLoopPoint;
		}

		NewSequence->SetItemCount(ValPtr);
		NewSequence->SetLoopPoint(iLoopPoint);
	}
}

int CFamiTrackerDoc::GetFirstFreePattern(int Channel)
{
	for (int i = 0; i < MAX_PATTERN; i++) {
		if (m_pSelectedTune->IsPatternFree(Channel, i))
			return i;
	}
	return 0;
}

char *CFamiTrackerDoc::GetTrackTitle(unsigned int Track) const
{
	return (char*)m_sTrackNames[Track].GetString();
}

bool CFamiTrackerDoc::AddTrack()
{
	if (m_iTracks >= (MAX_TRACKS - 1))
		return false;

	m_iTracks++;
	m_sTrackNames[m_iTracks] = "New song";

	if (m_pTunes[m_iTracks] == NULL) {
		m_pTunes[m_iTracks] = new CPatternData;
		m_pTunes[m_iTracks]->Init(64, 1, 6, 150);
	}

	return true;
}

void CFamiTrackerDoc::RemoveTrack(unsigned int Track)
{
	unsigned int i;

	ASSERT(m_iTracks > 0);

	delete m_pTunes[Track];

	for (i = Track; i < m_iTracks; i++) {
		m_sTrackNames[i] = m_sTrackNames[i + 1];
		m_pTunes[i] = m_pTunes[i + 1];
	}

	m_pTunes[m_iTracks] = NULL;

	m_iTracks--;
}

void CFamiTrackerDoc::SetTrackTitle(unsigned int Track, CString Title)
{
	m_sTrackNames[Track] = Title;
}

void CFamiTrackerDoc::MoveTrackUp(unsigned int Track)
{
	CString Temp;
	Temp = m_sTrackNames[Track];
	m_sTrackNames[Track] = m_sTrackNames[Track - 1];
	m_sTrackNames[Track - 1] = Temp;

	CPatternData *pTemp;
	pTemp = m_pTunes[Track];
	m_pTunes[Track] = m_pTunes[Track - 1];
	m_pTunes[Track - 1] = pTemp;
}

void CFamiTrackerDoc::MoveTrackDown(unsigned int Track)
{
	CString Temp;
	Temp = m_sTrackNames[Track];
	m_sTrackNames[Track] = m_sTrackNames[Track + 1];
	m_sTrackNames[Track + 1] = Temp;

	CPatternData *pTemp;
	pTemp = m_pTunes[Track];
	m_pTunes[Track] = m_pTunes[Track + 1];
	m_pTunes[Track + 1] = pTemp;
}

void CFamiTrackerDoc::ClearPatterns()
{
	m_pSelectedTune->ClearEverything();
	m_pSelectedTune->m_iFrameCount = 1;
}

int CFamiTrackerDoc::GetChannelType(int Channel)
{
	return m_iChannelTypes[Channel];
}

/////////////////////////////////

void CSequence::Clear()
{
	m_iItemCount = 0;
	m_iLoopPoint = -1;

	for (int i = 0; i < MAX_SEQUENCE_ITEMS; i++) {
		m_cValues[i] = 0;
	}
}

signed char CSequence::GetItem(int Index)
{
	if (Index >= MAX_SEQUENCE_ITEMS)
		return 0;

	return m_cValues[Index];
}

unsigned int CSequence::GetItemCount()
{
	return m_iItemCount;
}

void CSequence::SetItem(int Index, signed char Value)
{
	if (Index >= MAX_SEQUENCE_ITEMS)
		return;

	m_cValues[Index] = Value;
}

unsigned int CSequence::GetLoopPoint()
{
	return m_iLoopPoint;
}

void CSequence::SetItemCount(unsigned int Count)
{
	if (Count > MAX_SEQUENCE_ITEMS)
		Count = MAX_SEQUENCE_ITEMS - 1;

	m_iItemCount = Count;
}

void CSequence::SetLoopPoint(int Point)
{
	if (Point > MAX_SEQUENCE_ITEMS)
		Point = MAX_SEQUENCE_ITEMS - 1;

	m_iLoopPoint = Point;
}
