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
#include "TrackerChannel.h"
#include "MainFrm.h"
#include "DocumentFile.h"
#include "Settings.h"
#include "SoundGen.h"
#include "ChannelMap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define GET_PATTERN(Frame, Channel) m_pSelectedTune->GetFramePattern(Frame, Channel)

// Defaults when creating new modules
const char* CFamiTrackerDoc::DEFAULT_TRACK_NAME = "New song";
const int	CFamiTrackerDoc::DEFAULT_ROW_COUNT = 64;

const char* CFamiTrackerDoc::NEW_INST_NAME = "New instrument";

// File I/O constants
const char *FILE_HEADER				= "FamiTracker Module";
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

// FTI instruments files
const char INST_HEADER[] = "FTI";
const char INST_VERSION[] = "2.3";

/* 
	Instrument version history
	 * 2.1 - Release points for sequences in 2A03 & VRC6
	 * 2.2 - FDS volume sequences goes from 0-31 instead of 0-15
	 * 2.3 - Support for release points & extra setting in sequences, 2A03 & VRC6
*/

// Structures for loading older versions of files

struct stInstrumentImport {
	char Name[256];
	bool Free;
	int	 ModEnable[SEQ_COUNT];
	int	 ModIndex[SEQ_COUNT];
	int	 AssignedSample;				// For DPCM
};

struct stSequenceImport {
	signed char Length[64];	// locked to 64
	signed char Value[64];
	int	Count;
};

struct stDSampleImport {
	char *SampleData;
	int	 SampleSize;
	char Name[256];
};

struct stChanNoteImport {
	int	Note;
	int	Octave;
	int	Vol;
	int	Instrument;
	int	ExtraStuff1;
	int	ExtraStuff2;
};

enum {
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

//
// CDSample
//

CDSample::CDSample() 
	: SampleSize(0), SampleData(NULL) 
{
	memset(Name, 0, 256);
}

CDSample::CDSample(int Size, char *pData) 
	: SampleSize(Size), SampleData(pData)
{
	if (SampleData == NULL)
		SampleData = new char[Size];

	memset(Name, 0, 256);
}

CDSample::CDSample(CDSample &sample) 
	: SampleSize(sample.SampleSize), SampleData(new char[sample.SampleSize])
{
	memcpy(SampleData, sample.SampleData, SampleSize);
	strncpy(Name, sample.Name, 256);
}

CDSample::~CDSample()
{
	if (SampleData != NULL) {
		delete [] SampleData;
		SampleData = NULL;
	}
}

void CDSample::Copy(const CDSample *pDSample) 
{
	SAFE_RELEASE_ARRAY(SampleData);

	SampleSize = pDSample->SampleSize;
	SampleData = new char[SampleSize];

	memcpy(SampleData, pDSample->SampleData, SampleSize);
	strcpy(Name, pDSample->Name);
}

void CDSample::Allocate(int iSize, char *pData)
{
	SAFE_RELEASE_ARRAY(SampleData);

	SampleData = new char[iSize];
	SampleSize = iSize;

	if (pData != NULL)
		memcpy(SampleData, pData, iSize);
}

//
// CFamiTrackerDoc
//

IMPLEMENT_DYNCREATE(CFamiTrackerDoc, CDocument)

BEGIN_MESSAGE_MAP(CFamiTrackerDoc, CDocument)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_CLEANUP_REMOVEUNUSEDINSTRUMENTS, OnEditRemoveUnusedInstruments)
	ON_COMMAND(ID_CLEANUP_REMOVEUNUSEDPATTERNS, OnEditRemoveUnusedPatterns)
END_MESSAGE_MAP()

//
// Convert an instrument type to sound chip
//
static int GetChipFromInstrument(int Type)
{
	switch (Type) {
		case INST_2A03:
			return SNDCHIP_NONE;
		case INST_VRC6:
			return SNDCHIP_VRC6;
		case INST_VRC7:
			return SNDCHIP_VRC7;
		case INST_S5B:
			return SNDCHIP_S5B;
		case INST_FDS:
			return SNDCHIP_FDS;
		case INST_N106:
			return SNDCHIP_N106;
	}

	return SNDCHIP_NONE;
}

// CFamiTrackerDoc construction/destruction

CFamiTrackerDoc::CFamiTrackerDoc() 
	: m_iVersion(CLASS_VERSION), m_bFileLoaded(false), m_iRegisteredChannels(0)
{
	// Initialize object

	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		m_DSamples[i].SampleSize = 0;
		m_DSamples[i].SampleData = NULL;
	}

	// Clear pointer arrays
	memset(m_pTunes, 0, sizeof(CPatternData*) * MAX_TRACKS);
	memset(m_pInstruments, 0, sizeof(CInstrument*) * MAX_INSTRUMENTS);
	memset(m_pSequences2A03, 0, sizeof(CSequence*) * MAX_SEQUENCES * SEQ_COUNT);
	memset(m_pSequencesVRC6, 0, sizeof(CSequence*) * MAX_SEQUENCES * SEQ_COUNT);
	memset(m_pSequencesN106, 0, sizeof(CSequence*) * MAX_SEQUENCES * SEQ_COUNT);

	// Register this object to the sound generator
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	if (pSoundGen)
		pSoundGen->AssignDocument(this);
}

CFamiTrackerDoc::~CFamiTrackerDoc()
{
	// Clean up

	// DPCM samples
	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (m_DSamples[i].SampleData != NULL) {
			delete [] m_DSamples[i].SampleData;
			m_DSamples[i].SampleData = NULL;
			m_DSamples[i].SampleSize = 0;
		}
	}

	// Patterns
	for (int i = 0; i < MAX_TRACKS; ++i) {
		SAFE_RELEASE(m_pTunes[i]);
	}

	// Instruments
	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		SAFE_RELEASE(m_pInstruments[i]);
	}

	// Sequences
	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		for (int j = 0; j < SEQ_COUNT; ++j) {
			SAFE_RELEASE(m_pSequences2A03[i][j]);
			SAFE_RELEASE(m_pSequencesVRC6[i][j]);
			SAFE_RELEASE(m_pSequencesN106[i][j]);
		}
	}
}

//
// Overrides
//

BOOL CFamiTrackerDoc::OnNewDocument()
{
	// Called by the GUI to create a new file

	// This calls DeleteContents
	if (!CDocument::OnNewDocument())
		return FALSE;

	CreateEmpty();

	return TRUE;
}

BOOL CFamiTrackerDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	// This function is called by the GUI to load a file

	DeleteContents();

	// Load file
	if (!OpenDocument(lpszPathName)) {
		// Loading failed, create new document
		GetDocTemplate()->OpenDocumentFile(NULL);
		// and tell doctemplate that loading failed
		return FALSE;
	}

	// Update main frame
	SelectExpansionChip(m_iExpansionChip);

	theApp.GetSoundGenerator()->LoadMachineSettings(m_iMachine, m_iEngineSpeed);

//	SetupAutoSave();

	// Remove modified flag
	SetModifiedFlag(FALSE);

	return TRUE;
}

BOOL CFamiTrackerDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	// This function is called by the GUI to save the file

	if (!m_bFileLoaded)
		return FALSE;

	if (!SaveDocument(lpszPathName))
		return FALSE;

	// Reset modified flag
	SetModifiedFlag(FALSE);

	return TRUE;
}

void CFamiTrackerDoc::OnCloseDocument()
{	
	// Document object is about to be deleted

	// Remove itself from sound generator
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	if (pSoundGen)
		pSoundGen->RemoveDocument();

	CDocument::OnCloseDocument();
}

void CFamiTrackerDoc::DeleteContents()
{
	// Current document is being unloaded, clear and reset variables and memory
	// Delete everything because the current object is being reused in SDI

	// Mark file as unloaded
	m_csLoadedLock.Lock();
	m_bFileLoaded = false;
	m_csLoadedLock.Unlock();

	UpdateAllViews(NULL, CLOSE_DOCUMENT);

	// Make sure player is stopped
	theApp.StopPlayer();

	// DPCM samples
	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (m_DSamples[i].SampleSize != NULL) {
			delete [] m_DSamples[i].SampleData;
			m_DSamples[i].SampleData = NULL;
			m_DSamples[i].SampleSize = 0;
		}
	}

	// Instruments
	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		SAFE_RELEASE(m_pInstruments[i]);
	}

	// Clear sequences
	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		m_TmpSequences[i].Count = 0;
		for (int j = 0; j < SEQ_COUNT; ++j) {
			SAFE_RELEASE(m_pSequences2A03[i][j]);
			SAFE_RELEASE(m_pSequencesVRC6[i][j]);
			SAFE_RELEASE(m_pSequencesN106[i][j]);
		}
	}

	// Clear tracks
	m_iTracks = 0;
	m_iTrack = 0;

	// Delete all patterns
	for (int i = 0; i < MAX_TRACKS; ++i) {
		SAFE_RELEASE(m_pTunes[i]);
		m_sTrackNames[i] = "";
	}

	// Clear song info
	memset(m_strName, 0, 32);
	memset(m_strArtist, 0, 32);
	memset(m_strCopyright, 0, 32);

	// Reset variables to default
	m_iMachine			 = DEFAULT_MACHINE_TYPE;
	m_iEngineSpeed		 = 0;
	m_iExpansionChip	 = SNDCHIP_NONE;
	m_iVibratoStyle		 = VIBRATO_OLD;
	m_iChannelsAvailable = CHANNELS_DEFAULT;

	// Used for loading older files
	memset(m_Sequences, 0, sizeof(stSequence) * MAX_SEQUENCES * SEQ_COUNT);

	// Auto save
//	ClearAutoSave();

	// Remove modified flag
	SetModifiedFlag(FALSE);

	CDocument::DeleteContents();
}

void CFamiTrackerDoc::SetModifiedFlag(BOOL bModified)
{
	// Trigger auto-save in 10 seconds
	/*
	if (bModified)
		m_iAutoSaveCounter = 10;
		*/

	CDocument::SetModifiedFlag(bModified);
}

void CFamiTrackerDoc::CreateEmpty()
{
	// Allocate first song
	SwitchToTrack(0);

	// and select 2A03 only
	SelectExpansionChip(SNDCHIP_NONE);

	// Auto-select new style vibrato for new modules
	m_iVibratoStyle = VIBRATO_NEW;

//	SetupAutoSave();

	SetModifiedFlag(FALSE);

	// Document is avaliable
	m_csLoadedLock.Lock();
	m_bFileLoaded = true;
	m_csLoadedLock.Unlock();	
}

//
// Messages
//

void CFamiTrackerDoc::OnFileSave()
{
	if (GetPathName().GetLength() == 0)
		OnFileSaveAs();
	else
		CDocument::OnFileSave();
}

void CFamiTrackerDoc::OnFileSaveAs()
{
	// Overloaded in order to save the ftm-path
	CString newName = GetPathName();

	if (!AfxGetApp()->DoPromptFileName(newName, AFX_IDS_SAVEFILE, OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, FALSE, GetDocTemplate()))
		return;

	theApp.GetSettings()->SetPath(newName, PATH_FTM);
	
	DoSave(newName);
}


void CFamiTrackerDoc::OnEditRemoveUnusedPatterns()
{
	POSITION	Pos = GetFirstViewPosition();
	CMainFrame	*pMainFrame = (CMainFrame*)GetNextView(Pos)->GetParentFrame();

	// Removes unused patterns in the module
	// 
	// All tracks and patterns are scanned
	//

	bool bRemove;

	if (AfxMessageBox(IDS_REMOVE_PATTERNS, MB_YESNO | MB_ICONINFORMATION) == IDNO)
		return;

	for (unsigned int i = 0; i <= m_iTracks; ++i) {
		for (unsigned int c = 0; c < m_iChannelsAvailable; ++c) {
			for (unsigned int p = 0; p < MAX_PATTERN; ++p) {
				bRemove = true;
				// Check if pattern is used in frame list
				unsigned int FrameCount = m_pTunes[i]->GetFrameCount();
				for (unsigned int f = 0; f < FrameCount; ++f) {
					bRemove = (m_pTunes[i]->GetFramePattern(f, c) == p) ? false : bRemove;
				}
				if (bRemove)
					m_pTunes[i]->ClearPattern(c, p);
			}
		}
	}

	UpdateAllViews(NULL, CHANGED_PATTERN);
}

void CFamiTrackerDoc::OnEditRemoveUnusedInstruments()
{
	POSITION	Pos = GetFirstViewPosition();
	CMainFrame	*pMainFrame = (CMainFrame*)GetNextView(Pos)->GetParentFrame();

	// Removes unused instruments in the module
	// 
	// All tracks and patterns are scanned
	//

	if (AfxMessageBox(IDS_REMOVE_INSTRUMENTS, MB_YESNO | MB_ICONINFORMATION) == IDNO)
		return;

	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (IsInstrumentUsed(i)) {
			bool Used = false;
			for (unsigned int j = 0; j <= m_iTracks; ++j) {
				for (unsigned int Channel = 0; Channel < m_iChannelsAvailable; ++Channel) {
					for (unsigned int Frame = 0; Frame < m_pTunes[j]->GetFrameCount(); ++Frame) {
						unsigned int Pattern = m_pTunes[j]->GetFramePattern(Frame, Channel);
						for (unsigned int Row = 0; Row < m_pTunes[j]->GetPatternLength(); ++Row) {
							stChanNote *pNote = m_pTunes[j]->GetPatternData(Channel, Pattern, Row);
							if (pNote->Note != NONE && pNote->Note != HALT && pNote->Note != RELEASE && pNote->Instrument == i)
								Used = true;
						}
					}
				}
			}
			if (!Used)
				RemoveInstrument(i);
		}
	}

	// Also remove unused sequences
	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		for (int j = 0; j < SEQ_COUNT; ++j) {
			// Scan through all 2A03 instruments
			if (GetSequence(i, j)->GetItemCount() > 0) {
				bool Used = false;
				for (int k = 0; k < MAX_INSTRUMENTS; ++k) {
					if (IsInstrumentUsed(k)) {
						CInstrument2A03 *pInst = (CInstrument2A03*)GetInstrument(k);
						if (pInst->GetType() == INST_2A03) {
							if (pInst->GetSeqIndex(j) == i) {
								Used = true;
								break;
							}
						}
					}
				}
				if (!Used)
					GetSequence(i, j)->Clear();
			}
			// Scan through all VRC6 instruments
			if (GetSequenceVRC6(i, j)->GetItemCount() > 0) {
				bool Used = false;
				for (int k = 0; k < MAX_INSTRUMENTS; ++k) {
					if (IsInstrumentUsed(k)) {
						CInstrumentVRC6 *pInst = (CInstrumentVRC6*)GetInstrument(k);
						if (pInst->GetType() == INST_VRC6) {
							if (pInst->GetSeqIndex(j) == i) {
								Used = true;
								break;
							}
						}
					}
				}
				if (!Used)
					GetSequenceVRC6(i, j)->Clear();
			}
		}
	}

	// Update instrument list
	UpdateAllViews(NULL, UPDATE_INSTRUMENTS);
}

// CFamiTrackerDoc serialization (never used)

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

// Functions for compability with older file versions

void CFamiTrackerDoc::ReorderSequences()
{
	int Keepers[SEQ_COUNT] = {0, 0, 0, 0, 0};
	int Indices[MAX_SEQUENCES][SEQ_COUNT];
	int Index;

	memset(Indices, 0xFF, MAX_SEQUENCES * SEQ_COUNT * sizeof(int));

	CInstrument2A03	*pInst;

	// Organize sequences
	for (int i = 0; i < MAX_INSTRUMENTS; i++) {
		if (m_pInstruments[i] != NULL) {
			pInst = (CInstrument2A03*)m_pInstruments[i];
			for (int x = 0; x < SEQ_COUNT; x++) {
				if (pInst->GetSeqEnable(x)) {
					Index = pInst->GetSeqIndex(x);
					if (Indices[Index][x] >= 0 && Indices[Index][x] != -1) {
						pInst->SetSeqIndex(x, Indices[Index][x]);
					}
					else {
						memcpy(&m_Sequences[Keepers[x]][x], &m_TmpSequences[Index], sizeof(stSequence));
						for (unsigned int j = 0; j < m_Sequences[Keepers[x]][x].Count; j++) {
							switch (x) {
								case SEQ_VOLUME: LIMIT(m_Sequences[Keepers[x]][x].Value[j], 15, 0); break;
								case SEQ_DUTYCYCLE: LIMIT(m_Sequences[Keepers[x]][x].Value[j], 3, 0); break;
							}
						}
						Indices[Index][x] = Keepers[x];
						pInst->SetSeqIndex(x, Keepers[x]);
						Keepers[x]++;
					}
				}
				else
					pInst->SetSeqIndex(x, 0);
			}
		}
	}
}

void CFamiTrackerDoc::ConvertSequences()
{
	int i, j, k;
	int iLength, ValPtr, Count, Value, Length;
	stSequence	*pSeq;
	CSequence	*pNewSeq;

	// This function is used to convert the old type sequences to new type

	for (i = 0; i < MAX_SEQUENCES; ++i) {
		for (j = 0; j < /*MAX_SEQUENCE_ITEMS*/ SEQ_COUNT; ++j) {
			pSeq = &m_Sequences[i][j];
			if (pSeq->Count > 0 && pSeq->Count < MAX_SEQUENCE_ITEMS) {
				pNewSeq = GetSequence(i, j);
				
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
						for (int l = 0; l < Length + 1 && ValPtr < MAX_SEQUENCE_ITEMS; l++) {
							if ((j == SEQ_PITCH || j == SEQ_HIPITCH) && l > 0)
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


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Document store functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CFamiTrackerDoc::SaveDocument(LPCTSTR lpszPathName) const
{
	CDocumentFile DocumentFile;
	CFileException ex;
	TCHAR TempPath[MAX_PATH], TempFile[MAX_PATH];

	if (GetExpansionChip() & SNDCHIP_N106) {
		AfxMessageBox("Saving Namco modules is not yet supported");
		return FALSE;
	}
	if (GetExpansionChip() & SNDCHIP_S5B) {
		AfxMessageBox("Saving Sunsoft modules is not yet supported");
		return FALSE;
	}

	// First write to a temp file (if saving fails, the original is not destroyed)
	GetTempPath(MAX_PATH, TempPath);
	GetTempFileName(TempPath, _T("FTM"), 0, TempFile);

	if (!DocumentFile.Open(TempFile, CFile::modeWrite | CFile::modeCreate, &ex)) {
		// Could not open file
		TCHAR szCause[255];
		CString strFormatted;
		ex.GetErrorMessage(szCause, 255);
		strFormatted.LoadString(IDS_SAVE_ERROR_REASON);
		strFormatted += szCause;
		AfxMessageBox(strFormatted, MB_OK | MB_ICONERROR);
		return FALSE;
	}

	DocumentFile.BeginDocument();

	if (!WriteBlocks(&DocumentFile)) {
		// The save process failed, delete temp file
		DocumentFile.Close();
		DeleteFile(TempFile);
		// Display error
		CString	ErrorMsg;
		ErrorMsg.LoadString(IDS_SAVE_ERROR);
		AfxMessageBox(ErrorMsg, MB_OK | MB_ICONERROR);
		return FALSE;
	}

	DocumentFile.EndDocument();

	DocumentFile.Close();

	// Everything is done and the program cannot crash at this point
	// Replace the original
	if (!MoveFileEx(TempFile, lpszPathName, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
		// Display message if saving failed
		CString	ErrorMsg;
		TCHAR	*lpMsgBuf;
		ErrorMsg.LoadString(IDS_SAVE_ERROR_REASON);
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
		ErrorMsg.Append(lpMsgBuf);
		AfxMessageBox(ErrorMsg, MB_OK | MB_ICONERROR);
		LocalFree(lpMsgBuf);
		// Remove temp file
		DeleteFile(TempFile);
		return FALSE;
	}

	return TRUE;
}

bool CFamiTrackerDoc::WriteBlocks(CDocumentFile *pDocFile) const
{
	if (!WriteBlock_Parameters(pDocFile))
		return false;
	if (!WriteBlock_SongInfo(pDocFile))
		return false;
	if (!WriteBlock_Header(pDocFile))
		return false;
	if (!WriteBlock_Instruments(pDocFile))
		return false;
	if (!WriteBlock_Sequences(pDocFile))
		return false;
	if (!WriteBlock_Frames(pDocFile))
		return false;
	if (!WriteBlock_Patterns(pDocFile))
		return false;
	if (!WriteBlock_DSamples(pDocFile))
		return false;

	if (m_iExpansionChip & SNDCHIP_VRC6) {
		if (!WriteBlock_SequencesVRC6(pDocFile))
			return false;
	}

	return true;
}

bool CFamiTrackerDoc::WriteBlock_Parameters(CDocumentFile *pDocFile) const
{
	// Song parameters
	pDocFile->CreateBlock(FILE_BLOCK_PARAMS, 4);
	
	CMainFrame *pMainFrm = (CMainFrame*)theApp.GetMainWnd();
	int Highlight = pMainFrm->GetHighlightRow();
	int SecondHighlight = pMainFrm->GetSecondHighlightRow();

	pDocFile->WriteBlockChar(m_iExpansionChip);		// ver 2 change
	pDocFile->WriteBlockInt(m_iChannelsAvailable);
	pDocFile->WriteBlockInt(m_iMachine);
	pDocFile->WriteBlockInt(m_iEngineSpeed);
	pDocFile->WriteBlockInt(m_iVibratoStyle);		// ver 3 change
	pDocFile->WriteBlockInt(Highlight);				// ver 4 change
	pDocFile->WriteBlockInt(SecondHighlight);

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_SongInfo(CDocumentFile *pDocFile) const
{
	// Song info
	pDocFile->CreateBlock(FILE_BLOCK_INFO, 1);
	
	pDocFile->WriteBlock(m_strName, 32);
	pDocFile->WriteBlock(m_strArtist, 32);
	pDocFile->WriteBlock(m_strCopyright, 32);

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_Header(CDocumentFile *pDocFile) const
{
	/* 
	 *  Header data
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
	for (unsigned int i = 0; i <= m_iTracks; ++i) {
		pDocFile->WriteString(m_sTrackNames[i]);
	}

	for (unsigned int i = 0; i < m_iChannelsAvailable; ++i) {
		// Channel type
		pDocFile->WriteBlockChar(m_iChannelTypes[i]);	
		for (unsigned int j = 0; j <= m_iTracks; ++j) {
			ASSERT(m_pTunes[j] != NULL);
//			AllocateSong(j);
			// Effect columns
			pDocFile->WriteBlockChar(m_pTunes[j]->GetEffectColumnCount(i));
		}
	}

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_Instruments(CDocumentFile *pDocFile) const
{
	// A bug in v0.3.0 causes a crash if this is not 2, so change only when that ver is obsolete!
	const int BLOCK_VERSION = 2;
	// If FDS is used then version must be at least 4 or recent files won't load
	int Version = BLOCK_VERSION;

	// Fix for FDS instruments
	if (m_iExpansionChip & SNDCHIP_FDS)
		Version = 4;

	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (m_pInstruments[i] != NULL) {
			if (m_pInstruments[i]->GetType() == INST_FDS)
				Version = 4;
		}
	}

	int Count = 0;
	char Name[256], Type;

	// Instruments block
	pDocFile->CreateBlock(FILE_BLOCK_INSTRUMENTS, Version);

	// Count number of instruments
	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (m_pInstruments[i] != NULL)
			Count++;
	}

	pDocFile->WriteBlockInt(Count);

	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		// Only write instrument if it's used
		if (m_pInstruments[i] != NULL) {

			Type = m_pInstruments[i]->GetType();

			// Write index and type
			pDocFile->WriteBlockInt(i);
			pDocFile->WriteBlockChar(Type);

			// Store the instrument
			m_pInstruments[i]->Store(pDocFile);

			// Store the name
			m_pInstruments[i]->GetName(Name);
			pDocFile->WriteBlockInt((int)strlen(Name));
			pDocFile->WriteBlock(Name, (int)strlen(Name));			
		}
	}

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_Sequences(CDocumentFile *pDocFile) const
{
	/* 
	 * Store 2A03 sequences
	 */ 

	// Sequences, version 6
	pDocFile->CreateBlock(FILE_BLOCK_SEQUENCES, 6);

	int Count = 0;

	// Count number of used sequences
	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		for (int j = 0; j < SEQ_COUNT; ++j) {
			if (GetSequenceItemCount(i, j) > 0)
				Count++;
		}
	}

	pDocFile->WriteBlockInt(Count);

	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		for (int j = 0; j < SEQ_COUNT; ++j) {
			Count = GetSequenceItemCount(i, j);
			if (Count > 0) {
				const CSequence *pSeq = reinterpret_cast<const CSequence*>(GetSequence(i, j));
				// Store index
				pDocFile->WriteBlockInt(i);
				// Store type of sequence
				pDocFile->WriteBlockInt(j);
				// Store number of items in this sequence
				pDocFile->WriteBlockChar(Count);
				// Store loop point
				pDocFile->WriteBlockInt(pSeq->GetLoopPoint());
				// Store items
				for (int k = 0; k < Count; ++k) {
					pDocFile->WriteBlockChar(pSeq->GetItem(k));
				}
			}
		}
	}

	// v6
	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		for (int j = 0; j < SEQ_COUNT; ++j) {
			Count = GetSequenceItemCount(i, j);
			if (Count > 0) {
				const CSequence *pSeq = reinterpret_cast<const CSequence*>(GetSequence(i, j));
				// Store release point
				pDocFile->WriteBlockInt(pSeq->GetReleasePoint());
				// Store setting
				pDocFile->WriteBlockInt(pSeq->GetSetting());
			}
		}
	}

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_SequencesVRC6(CDocumentFile *pDocFile) const
{
	/* 
	 * Store VRC6 sequences
	 */ 

	// Sequences, version 6
	pDocFile->CreateBlock(FILE_BLOCK_SEQUENCES_VRC6, 6);

	int Count = 0;

	// Count number of used sequences
	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		for (int j = 0; j < SEQ_COUNT; ++j) {
			if (GetSequenceItemCountVRC6(i, j) > 0)
				Count++;
		}
	}

	// Write it
	pDocFile->WriteBlockInt(Count);

	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		for (int j = 0; j < SEQ_COUNT; ++j) {
			Count = GetSequenceItemCountVRC6(i, j);
			if (Count > 0) {
				const CSequence *pSeq = GetSequenceVRC6(i, j);
				// Store index
				pDocFile->WriteBlockInt(i);
				// Store type of sequence
				pDocFile->WriteBlockInt(j);
				// Store number of items in this sequence
				pDocFile->WriteBlockChar(Count);
				// Store loop point
				pDocFile->WriteBlockInt(pSeq->GetLoopPoint());
				// Store items
				for (int k = 0; k < Count; ++k) {
					pDocFile->WriteBlockChar(pSeq->GetItem(k));
				}
			}
		}
	}

	// v6
	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		for (int j = 0; j < SEQ_COUNT; ++j) {
			Count = GetSequenceItemCountVRC6(i, j);
			if (Count > 0) {
				const CSequence *pSeq = GetSequenceVRC6(i, j);
				// Store release point
				pDocFile->WriteBlockInt(pSeq->GetReleasePoint());
				// Store setting
				pDocFile->WriteBlockInt(pSeq->GetSetting());
			}
		}
	}

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_Frames(CDocumentFile *pDocFile) const
{
//	unsigned int i, x, y;

	/* Store frame count
	 *
	 * 1. Number of channels (5 for 2A03 only)
	 * 2. 
	 * 
	 */ 

	pDocFile->CreateBlock(FILE_BLOCK_FRAMES, 3);

	for (unsigned i = 0; i <= m_iTracks; ++i) {
		CPatternData *pTune = m_pTunes[i];

		pDocFile->WriteBlockInt(pTune->GetFrameCount());
		pDocFile->WriteBlockInt(pTune->GetSongSpeed());
		pDocFile->WriteBlockInt(pTune->GetSongTempo());
		pDocFile->WriteBlockInt(pTune->GetPatternLength());

		for (unsigned int j = 0; j < pTune->GetFrameCount(); ++j) {
			for (unsigned k = 0; k < m_iChannelsAvailable; ++k) {
				pDocFile->WriteBlockChar((unsigned char)pTune->GetFramePattern(j, k));
			}
		}
	}

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_Patterns(CDocumentFile *pDocFile) const
{
	/*
	 * Version changes: 
	 *
	 *  2: Support multiple tracks
	 *  3: Changed portamento effect
	 *  4: Switched portamento effects for VRC7 (1xx & 2xx), adjusted Pxx for FDS
	 *  5: Adjusted FDS octave
	 *
	 */ 

#ifdef TRANSPOSE_FDS
	pDocFile->CreateBlock(FILE_BLOCK_PATTERNS, 5);
#else
	pDocFile->CreateBlock(FILE_BLOCK_PATTERNS, 4);
#endif

	for (unsigned t = 0; t <= m_iTracks; ++t) {
		for (unsigned i = 0; i < m_iChannelsAvailable; ++i) {
			for (unsigned x = 0; x < MAX_PATTERN; ++x) {
				unsigned Items = 0;

				// Save all rows
				unsigned int PatternLen = MAX_PATTERN_LENGTH;
				//unsigned int PatternLen = m_pTunes[t]->GetPatternLength();
				
				// Get the number of items in this pattern
				for (unsigned y = 0; y < PatternLen; ++y) {
					if (!m_pTunes[t]->IsCellFree(i, x, y))
						Items++;
				}

				if (Items > 0) {
					pDocFile->WriteBlockInt(t);		// Write track
					pDocFile->WriteBlockInt(i);		// Write channel
					pDocFile->WriteBlockInt(x);		// Write pattern
					pDocFile->WriteBlockInt(Items);	// Number of items

					for (unsigned y = 0; y < PatternLen; y++) {
						if (!m_pTunes[t]->IsCellFree(i, x, y)) {
							pDocFile->WriteBlockInt(y);

							pDocFile->WriteBlockChar(m_pTunes[t]->GetNote(i, x, y));
							pDocFile->WriteBlockChar(m_pTunes[t]->GetOctave(i, x, y));
							pDocFile->WriteBlockChar(m_pTunes[t]->GetInstrument(i, x, y));
							pDocFile->WriteBlockChar(m_pTunes[t]->GetVolume(i, x, y));

							int EffColumns = (m_pTunes[t]->GetEffectColumnCount(i) + 1);

							for (int n = 0; n < EffColumns; n++) {
								pDocFile->WriteBlockChar(m_pTunes[t]->GetEffect(i, x, y, n));
								pDocFile->WriteBlockChar(m_pTunes[t]->GetEffectParam(i, x, y, n));
							}
						}
					}
				}
			}
		}
	}

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_DSamples(CDocumentFile *pDocFile) const
{
	int Count = 0;

	pDocFile->CreateBlock(FILE_BLOCK_DSAMPLES, 1);

	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (m_DSamples[i].SampleSize > 0)
			Count++;
	}

	// Write sample count
	pDocFile->WriteBlockChar(Count);

	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (m_DSamples[i].SampleSize > 0) {
			// Write sample
			pDocFile->WriteBlockChar(i);
			pDocFile->WriteBlockInt((int)strlen(m_DSamples[i].Name));
			pDocFile->WriteBlock(m_DSamples[i].Name, (int)strlen(m_DSamples[i].Name));
			pDocFile->WriteBlockInt(m_DSamples[i].SampleSize);
			pDocFile->WriteBlock(m_DSamples[i].SampleData, m_DSamples[i].SampleSize);
		}
	}

	return pDocFile->FlushBlock();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Document load functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CFamiTrackerDoc::OpenDocument(LPCTSTR lpszPathName)
{
	CFileException ex;
	CDocumentFile  OpenFile;
	unsigned int   iVersion;
	bool		   bForceBackup = false;

	// Delete loaded document
//	DeleteContents();

	// Open file
	if (!OpenFile.Open(lpszPathName, CFile::modeRead | CFile::shareDenyWrite, &ex)) {
		TCHAR   szCause[255];
		CString strFormatted;
		ex.GetErrorMessage(szCause, 255);
		strFormatted = _T("Could not open file.\n\n");
		strFormatted += szCause;
		AfxMessageBox(strFormatted);
		OnNewDocument();
		return FALSE;
	}

	// Check if empty file
	if (OpenFile.GetLength() == 0) {
		// Setup default settings (TODO: call this from onnewdocument)
		CreateEmpty();
		return TRUE;
	}

	// Read header ID and version
	if (!OpenFile.CheckValidity()) {
		AfxMessageBox(IDS_FILE_VALID_ERROR, MB_ICONERROR);
		return FALSE;
	}

	iVersion = OpenFile.GetFileVersion();

	if (iVersion < 0x0200) {
		// Older file version
		if (iVersion < CDocumentFile::COMPATIBLE_VER) {
			AfxMessageBox(IDS_FILE_VERSION_ERROR, MB_ICONERROR);
			return FALSE;
		}

		if (!OpenDocumentOld(&OpenFile))
			return FALSE;

		// Create a backup of this file, since it's an old version 
		// and something might go wrong when converting
		bForceBackup = true;

		// Auto-select old style vibrato for old files
		m_iVibratoStyle = VIBRATO_OLD;
	}
	else if (iVersion >= 0x0200) {
		// New file version

		// Try to open file, create new if it fails
		if (!OpenDocumentNew(OpenFile))
			return FALSE;

		// Backup if files was of an older version
		bForceBackup = m_iFileVersion < CDocumentFile::FILE_VER;
	}

#ifdef WIP
	// Force backups if compiled as beta
//	bForceBackup = true;
#endif

	// File backup
	if (bForceBackup || theApp.GetSettings()->General.bBackups) {
		CString BakName;
		BakName.Format(_T("%s.bak"), lpszPathName);
		CopyFile(lpszPathName, BakName.GetBuffer(), FALSE);
	}

	// File is loaded
	m_csLoadedLock.Lock();
	m_bFileLoaded= true;
	m_csLoadedLock.Unlock();

	return TRUE;
}

/**
 * This function reads the old obsolete file version. 
 */
BOOL CFamiTrackerDoc::OpenDocumentOld(CFile *pOpenFile)
{
	unsigned int i, c, ReadCount, FileBlock;

	FileBlock = 0;

	// Only single track files
	SwitchToTrack(0);

	m_iVibratoStyle = VIBRATO_OLD;

	stInstrumentImport	ImportedInstruments;
	stSequenceImport	ImportedSequence;
	stDSampleImport		ImportedDSample;
	stChanNoteImport	ImportedNote;

	while (FileBlock != FB_EOF) {
		if (pOpenFile->Read(&FileBlock, sizeof(int)) == 0)
			FileBlock = FB_EOF;

		unsigned int Speed, FrameCount, Pattern, PatternLength;

		switch (FileBlock) {
			case FB_CHANNELS:
				pOpenFile->Read(&m_iChannelsAvailable, sizeof(int));
				break;

			case FB_SPEED:
				pOpenFile->Read(&Speed, sizeof(int));
				m_pSelectedTune->SetSongSpeed(Speed + 1);
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
						for (int j = 0; j < SEQ_COUNT; j++) {
							pInst->SetSeqEnable(j, ImportedInstruments.ModEnable[j]);
							pInst->SetSeqIndex(j, ImportedInstruments.ModIndex[j]);
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
				}
				break;

			case FB_SEQUENCES:
				pOpenFile->Read(&ReadCount, sizeof(int));
				for (i = 0; i < ReadCount; i++) {
					pOpenFile->Read(&ImportedSequence, sizeof(stSequenceImport));
					if (ImportedSequence.Count > 0 && ImportedSequence.Count < MAX_SEQUENCE_ITEMS) {
						m_TmpSequences[i].Count = ImportedSequence.Count;
						memcpy(m_TmpSequences[i].Length, ImportedSequence.Length, ImportedSequence.Count);
						memcpy(m_TmpSequences[i].Value, ImportedSequence.Value, ImportedSequence.Count);
					}
				}
				break;

			case FB_PATTERN_ROWS:
				pOpenFile->Read(&FrameCount, sizeof(int));
				m_pSelectedTune->SetFrameCount(FrameCount);
				for (c = 0; c < FrameCount; c++) {
					for (i = 0; i < m_iChannelsAvailable; i++) {
						pOpenFile->Read(&Pattern, sizeof(int));
						m_pSelectedTune->SetFramePattern(c, i, Pattern);
					}
				}
				break;

			case FB_PATTERNS:
				pOpenFile->Read(&ReadCount, sizeof(int));
				pOpenFile->Read(&PatternLength, sizeof(int));
				m_pSelectedTune->SetPatternLength(PatternLength);
				for (unsigned int x = 0; x < m_iChannelsAvailable; x++) {
					for (c = 0; c < ReadCount; c++) {
						for (i = 0; i < PatternLength; i++) {
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

	pOpenFile->Close();

	return TRUE;
}

/**
 *  This function opens the most recent file version
 *
 */
BOOL CFamiTrackerDoc::OpenDocumentNew(CDocumentFile &DocumentFile)
{
	const char *BlockID;
	bool FileFinished = false;
	bool ErrorFlag = false;

#ifdef _DEBUG
	int _msgs_ = 0;
#endif

	// File version checking
	m_iFileVersion = DocumentFile.GetFileVersion();

	// From version 2.0, all files should be compatible (though individual blocks may not)
	if (m_iFileVersion < 0x0200) {
		AfxMessageBox(IDS_FILE_VERSION_ERROR, MB_ICONERROR);
		DocumentFile.Close();
		return FALSE;
	}

	// File version is too new
	if (m_iFileVersion > CDocumentFile::FILE_VER) {
		AfxMessageBox(IDS_FILE_VERSION_TOO_NEW, MB_ICONERROR);
		DocumentFile.Close();
		return false;
	}

	if (m_iFileVersion < 0x0210) {
		// This has to be done for older files
		SwitchToTrack(0);
	}

	// Read all blocks
	while (!DocumentFile.Finished() && !FileFinished && !ErrorFlag) {
		ErrorFlag = DocumentFile.ReadBlock();
		BlockID = DocumentFile.GetBlockHeaderID();

		if (!strcmp(BlockID, FILE_BLOCK_PARAMS)) {
			if (DocumentFile.GetBlockVersion() == 1) {
				m_pSelectedTune->SetSongSpeed(DocumentFile.GetBlockInt());
			}
			else
				m_iExpansionChip = DocumentFile.GetBlockChar();

			m_iChannelsAvailable	= DocumentFile.GetBlockInt();
			m_iMachine				= DocumentFile.GetBlockInt();
			m_iEngineSpeed			= DocumentFile.GetBlockInt();

			if (m_iMachine != NTSC && m_iMachine != PAL)
				m_iMachine = NTSC;

			if (DocumentFile.GetBlockVersion() > 2)
				m_iVibratoStyle = DocumentFile.GetBlockInt();
			else
				m_iVibratoStyle = VIBRATO_OLD;

			CMainFrame *pMainFrm = (CMainFrame*)theApp.GetMainWnd();
			if (pMainFrm != NULL) {
				if (DocumentFile.GetBlockVersion() > 3) {
					int Highlight = DocumentFile.GetBlockInt();
					int SecondHighlight = DocumentFile.GetBlockInt();
					pMainFrm->SetHighlightRow(Highlight);
					pMainFrm->SetSecondHighlightRow(SecondHighlight);
				}
				else {
					pMainFrm->SetHighlightRow(4);
					pMainFrm->SetSecondHighlightRow(16);
				}
			}

			// This is strange. Sometimes expansion chip is set to 0xFF in files
			if (m_iChannelsAvailable == 5)
				m_iExpansionChip = 0;

//			m_cExpansionChip = Chip;
//			SelectExpansionChip(m_cExpansionChip);

			if (m_iFileVersion == 0x0200) {
				int Speed = m_pSelectedTune->GetSongSpeed();
				if (Speed < 20)
					m_pSelectedTune->SetSongSpeed(Speed + 1);
			}

			if (DocumentFile.GetBlockVersion() == 1) {
				if (m_pSelectedTune->GetSongSpeed() > 19) {
					m_pSelectedTune->SetSongTempo(m_pSelectedTune->GetSongSpeed());
					m_pSelectedTune->SetSongSpeed(6);
				}
				else {
					m_pSelectedTune->SetSongTempo(m_iMachine == NTSC ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL);
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
				AfxMessageBox(_T("Unknown file block!"));
#endif
			if (DocumentFile.IsFileIncomplete())
				ErrorFlag = true;
		}
	}

	DocumentFile.Close();

	if (ErrorFlag) {
		AfxMessageBox(IDS_FILE_LOAD_ERROR, MB_ICONERROR);
		DeleteContents();
		return FALSE;
	}

	if (m_iFileVersion <= 0x0201)
		ReorderSequences();

	if (m_iFileVersion < 0x0300)
		ConvertSequences();

	// Always start with track 1
	SwitchToTrack(0);

	return TRUE;
}

bool CFamiTrackerDoc::ReadBlock_Header(CDocumentFile *pDocFile)
{
	int Version = pDocFile->GetBlockVersion();

	if (Version == 1) {
		// Single track
		SwitchToTrack(0);
		for (unsigned int i = 0; i < m_iChannelsAvailable; ++i) {
			// Channel type (unused)
			pDocFile->GetBlockChar();
			// Effect columns
			m_pSelectedTune->SetEffectColumnCount(i, pDocFile->GetBlockChar());
		}
	}
	else if (Version >= 2) {
		// Multiple tracks
		m_iTracks = pDocFile->GetBlockChar();

		ASSERT_FILE_DATA(m_iTracks <= MAX_TRACKS);

		// Add tracks to document
		for (unsigned i = 0; i <= m_iTracks; ++i) {
			AllocateSong(i);
		}

		// Track names
		if (Version >= 3) {
			for (unsigned i = 0; i <= m_iTracks; ++i) {
				m_sTrackNames[i] = pDocFile->ReadString();
			}
		}

		for (unsigned i = 0; i < m_iChannelsAvailable; ++i) {
			unsigned char ChannelType = pDocFile->GetBlockChar();					// Channel type (unused)
			for (unsigned j = 0; j <= m_iTracks; ++j) {
				m_pTunes[j]->SetEffectColumnCount(i, pDocFile->GetBlockChar());		// Effect columns
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
	
	int Version = pDocFile->GetBlockVersion();

	// Number of instruments
	int Count = pDocFile->GetBlockInt();
	ASSERT_FILE_DATA(Count <= MAX_INSTRUMENTS);

	for (int i = 0; i < Count; ++i) {
		// Instrument index
		int index = pDocFile->GetBlockInt();
		ASSERT_FILE_DATA(index <= MAX_INSTRUMENTS);

		// Read instrument type and create an instrument
		int Type = pDocFile->GetBlockChar();
		CInstrument *pInst = CreateInstrument(Type);
		ASSERT_FILE_DATA(pInst != NULL);

		// Load the instrument
		ASSERT_FILE_DATA(pInst->Load(pDocFile));

		// Read name
		unsigned int size = pDocFile->GetBlockInt();
		ASSERT_FILE_DATA(size < 256);
		char Name[256];
		pDocFile->GetBlock(Name, size);
		Name[size] = 0;
		pInst->SetName(Name);

		// Store instrument
		m_pInstruments[index] = pInst;
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Sequences(CDocumentFile *pDocFile)
{
	unsigned int i, x, Count = 0, Index, Type;
	unsigned int LoopPoint, ReleasePoint = -1, Settings = 0;
	unsigned char SeqCount;
	int Version = pDocFile->GetBlockVersion();
	char Value, Length;

	Count = pDocFile->GetBlockInt();
	ASSERT_FILE_DATA(Count < (MAX_SEQUENCES * SEQ_COUNT));

	if (Version == 1) {
		for (i = 0; i < Count; i++) {
			Index		= pDocFile->GetBlockInt();
			SeqCount	= pDocFile->GetBlockChar();
			ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
			ASSERT_FILE_DATA(SeqCount < MAX_SEQUENCE_ITEMS);
			m_TmpSequences[Index].Count = SeqCount;
			for (x = 0; x < SeqCount; x++) {
				m_TmpSequences[Index].Value[x] = pDocFile->GetBlockChar();
				m_TmpSequences[Index].Length[x] = pDocFile->GetBlockChar();
			}
		}
	}
	else if (Version == 2) {
		for (i = 0; i < Count; i++) {
			Index		= pDocFile->GetBlockInt();
			Type		= pDocFile->GetBlockInt();
			SeqCount	= pDocFile->GetBlockChar();
			ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
			ASSERT_FILE_DATA(Type < SEQ_COUNT);
			ASSERT_FILE_DATA(SeqCount < MAX_SEQUENCE_ITEMS);
			m_Sequences[Index][Type].Count = SeqCount;
			for (x = 0; x < SeqCount; x++) {
				Value = pDocFile->GetBlockChar();
				Length = pDocFile->GetBlockChar();
				m_Sequences[Index][Type].Value[x] = Value;
				m_Sequences[Index][Type].Length[x] = Length;
			}
		}
	}
	else if (Version >= 3) {
		int Indices[MAX_SEQUENCES * SEQ_COUNT];
		int Types[MAX_SEQUENCES * SEQ_COUNT];

		for (i = 0; i < Count; ++i) {
			Index	  = pDocFile->GetBlockInt();
			Type	  = pDocFile->GetBlockInt();
			SeqCount  = pDocFile->GetBlockChar();
			LoopPoint = pDocFile->GetBlockInt();

			// Work-around for some older files
			if (LoopPoint == SeqCount)
				LoopPoint = -1;

			Indices[i] = Index;
			Types[i] = Type;

			ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
			ASSERT_FILE_DATA(Type < SEQ_COUNT);
//			ASSERT_FILE_DATA(SeqCount <= MAX_SEQUENCE_ITEMS);

			CSequence *pSeq = GetSequence(Index, Type);

			pSeq->Clear();
			pSeq->SetItemCount(SeqCount < MAX_SEQUENCE_ITEMS ? SeqCount : MAX_SEQUENCE_ITEMS);
			pSeq->SetLoopPoint(LoopPoint);

			if (Version == 4) {
				ReleasePoint = pDocFile->GetBlockInt();
				Settings = pDocFile->GetBlockInt();
				pSeq->SetReleasePoint(ReleasePoint);
				pSeq->SetSetting(Settings);
			}

			for (x = 0; x < SeqCount; ++x) {
				Value = pDocFile->GetBlockChar();
				if (x <= MAX_SEQUENCE_ITEMS)
					pSeq->SetItem(x, Value);
			}
		}

		if (Version == 5) {
			// Version 5 saved the release points incorrectly, this is fixed in ver 6
			for (int i = 0; i < MAX_SEQUENCES; ++i) {
				for (int x = 0; x < SEQ_COUNT; ++x) {
					ReleasePoint = pDocFile->GetBlockInt();
					Settings = pDocFile->GetBlockInt();
					if (GetSequenceItemCount(i, x) > 0) {
						CSequence *pSeq = GetSequence(i, x);
						pSeq->SetReleasePoint(ReleasePoint);
						pSeq->SetSetting(Settings);
					}
				}
			}
		}
		else if (Version >= 6) {
			// Read release points correctly stored
			for (i = 0; i < Count; ++i) {
				ReleasePoint = pDocFile->GetBlockInt();
				Settings = pDocFile->GetBlockInt();
				Index = Indices[i];
				Type = Types[i];
				CSequence *pSeq = GetSequence(Index, Type);
				pSeq->SetReleasePoint(ReleasePoint);
				pSeq->SetSetting(Settings);
			}
		}

	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_SequencesVRC6(CDocumentFile *pDocFile)
{
	unsigned int i, j, Count = 0, Index, Type;
	unsigned int LoopPoint, ReleasePoint, Settings;
	unsigned char SeqCount;
	int Version = pDocFile->GetBlockVersion();
	char Value;

	Count = pDocFile->GetBlockInt();
	ASSERT_FILE_DATA(Count < MAX_SEQUENCES);

	if (Version < 4) {
		for (i = 0; i < Count; i++) {
			Index	  = pDocFile->GetBlockInt();
			Type	  = pDocFile->GetBlockInt();
			SeqCount  = pDocFile->GetBlockChar();
			LoopPoint = pDocFile->GetBlockInt();
//			if (SeqCount > MAX_SEQUENCE_ITEMS)
//				SeqCount = MAX_SEQUENCE_ITEMS;
			ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
			ASSERT_FILE_DATA(Type < SEQ_COUNT);
//			ASSERT_FILE_DATA(SeqCount <= MAX_SEQUENCE_ITEMS);
			CSequence *pSeq = GetSequenceVRC6(Index, Type);
			pSeq->Clear();
			pSeq->SetItemCount(SeqCount < MAX_SEQUENCE_ITEMS ? SeqCount : MAX_SEQUENCE_ITEMS);
			pSeq->SetLoopPoint(LoopPoint);
			for (j = 0; j < SeqCount; ++j) {
				Value = pDocFile->GetBlockChar();
				if (j <= MAX_SEQUENCE_ITEMS)
					pSeq->SetItem(j, Value);
			}
		}
	}
	else {
		int Indices[MAX_SEQUENCES];
		int Types[MAX_SEQUENCES];

		for (i = 0; i < Count; i++) {
			Index	  = pDocFile->GetBlockInt();
			Type	  = pDocFile->GetBlockInt();
			SeqCount  = pDocFile->GetBlockChar();
			LoopPoint = pDocFile->GetBlockInt();

			Indices[i] = Index;
			Types[i] = Type;
/*
			if (SeqCount >= MAX_SEQUENCE_ITEMS)
				SeqCount = MAX_SEQUENCE_ITEMS - 1;
*/
			ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
			ASSERT_FILE_DATA(Type < SEQ_COUNT);
//			ASSERT_FILE_DATA(SeqCount <= MAX_SEQUENCE_ITEMS);

			CSequence *pSeq = GetSequenceVRC6(Index, Type);

			pSeq->Clear();
			pSeq->SetItemCount(SeqCount);
			pSeq->SetLoopPoint(LoopPoint);

			if (Version == 4) {
				ReleasePoint = pDocFile->GetBlockInt();
				Settings = pDocFile->GetBlockInt();
				pSeq->SetReleasePoint(ReleasePoint);
				pSeq->SetSetting(Settings);
			}

			for (j = 0; j < SeqCount; ++j) {
				Value = pDocFile->GetBlockChar();
				if (j <= MAX_SEQUENCE_ITEMS)
					pSeq->SetItem(j, Value);
			}
		}

		if (Version == 5) {
			// Version 5 saved the release points incorrectly, this is fixed in ver 6
			for (int i = 0; i < MAX_SEQUENCES; ++i) {
				for (int j = 0; j < SEQ_COUNT; ++j) {
					ReleasePoint = pDocFile->GetBlockInt();
					Settings = pDocFile->GetBlockInt();
					if (GetSequenceItemCountVRC6(i, j) > 0) {
						CSequence *pSeq = GetSequenceVRC6(i, j);
						pSeq->SetReleasePoint(ReleasePoint);
						pSeq->SetSetting(Settings);
					}
				}
			}
		}
		else if (Version >= 6) {
			for (i = 0; i < Count; i++) {
				ReleasePoint = pDocFile->GetBlockInt();
				Settings = pDocFile->GetBlockInt();
				Index = Indices[i];
				Type = Types[i];
				CSequence *pSeq = GetSequenceVRC6(Index, Type);
				pSeq->SetReleasePoint(ReleasePoint);
				pSeq->SetSetting(Settings);
			}
		}
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Frames(CDocumentFile *pDocFile)
{
	unsigned int Version = pDocFile->GetBlockVersion();

	if (Version == 1) {
		unsigned int FrameCount = pDocFile->GetBlockInt();
		m_pSelectedTune->SetFrameCount(FrameCount);
		m_iChannelsAvailable = pDocFile->GetBlockInt();
		ASSERT_FILE_DATA(FrameCount <= MAX_FRAMES);
		ASSERT_FILE_DATA(m_iChannelsAvailable <= MAX_CHANNELS);
		for (unsigned i = 0; i < FrameCount; ++i) {
			for (unsigned j = 0; j < m_iChannelsAvailable; ++j) {
				unsigned Pattern = (unsigned)pDocFile->GetBlockChar();
				ASSERT_FILE_DATA(Pattern < MAX_FRAMES);
				m_pSelectedTune->SetFramePattern(i, j, Pattern);
			}
		}
	}
	else if (Version > 1) {

		for (unsigned y = 0; y <= m_iTracks; ++y) {
			unsigned int FrameCount = pDocFile->GetBlockInt();
			unsigned int Speed = pDocFile->GetBlockInt();
			
			ASSERT_FILE_DATA(FrameCount > 0 && FrameCount <= MAX_FRAMES);
			ASSERT_FILE_DATA(Speed > 0);

			m_pTunes[y]->SetFrameCount(FrameCount);

			if (Version == 3) {
				unsigned int Tempo = pDocFile->GetBlockInt();
				ASSERT_FILE_DATA(Tempo > 0);
				m_pTunes[y]->SetSongTempo(Tempo);
				m_pTunes[y]->SetSongSpeed(Speed);
			}
			else {
				if (Speed < 20) {
					unsigned int Tempo = (m_iMachine == NTSC) ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL;
					m_pTunes[y]->SetSongTempo(Tempo);
					m_pTunes[y]->SetSongSpeed(Speed);
				}
				else {
					m_pTunes[y]->SetSongTempo(Speed);
					m_pTunes[y]->SetSongSpeed(DEFAULT_SPEED);
				}
			}

			unsigned PatternLength = (unsigned)pDocFile->GetBlockInt();
			ASSERT_FILE_DATA(PatternLength > 0 && PatternLength <= MAX_PATTERN_LENGTH);

			m_pTunes[y]->SetPatternLength(PatternLength);
			
			for (unsigned i = 0; i < FrameCount; ++i) {
				for (unsigned j = 0; j < m_iChannelsAvailable; ++j) {
					// Read pattern index
					unsigned Pattern = (unsigned char)pDocFile->GetBlockChar();
					ASSERT_FILE_DATA(Pattern < MAX_PATTERN);
					m_pTunes[y]->SetFramePattern(i, j, Pattern);
				}
			}
		}
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Patterns(CDocumentFile *pDocFile)
{
	unsigned int Version = pDocFile->GetBlockVersion();

	if (Version == 1) {
		int PatternLen = pDocFile->GetBlockInt();
		ASSERT_FILE_DATA(PatternLen <= MAX_PATTERN_LENGTH);
		m_pSelectedTune->SetPatternLength(PatternLen);
	}

	while (!pDocFile->BlockDone()) {
		unsigned Track;
		if (Version > 1)
			Track = pDocFile->GetBlockInt();
		else if (Version == 1)
			Track = 0;

		unsigned Channel = pDocFile->GetBlockInt();
		unsigned Pattern = pDocFile->GetBlockInt();
		unsigned Items	= pDocFile->GetBlockInt();

		if (Channel > MAX_CHANNELS)
			return false;

		ASSERT_FILE_DATA(Track < MAX_TRACKS);
		ASSERT_FILE_DATA(Channel < MAX_CHANNELS);
		ASSERT_FILE_DATA(Pattern < MAX_PATTERN);
		ASSERT_FILE_DATA((Items - 1) < MAX_PATTERN_LENGTH);

		SwitchToTrack(Track);

		for (unsigned i = 0; i < Items; ++i) {
			unsigned Row;
			if (m_iFileVersion == 0x0200)
				Row = pDocFile->GetBlockChar();
			else
				Row = pDocFile->GetBlockInt();

			ASSERT_FILE_DATA(Row < MAX_PATTERN_LENGTH);

			stChanNote *Note = m_pSelectedTune->GetPatternData(Channel, Pattern, Row);
			memset(Note, 0, sizeof(stChanNote));

			Note->Note		 = pDocFile->GetBlockChar();
			Note->Octave	 = pDocFile->GetBlockChar();
			Note->Instrument = pDocFile->GetBlockChar();
			Note->Vol		 = pDocFile->GetBlockChar();

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

				stChanNote *Note = m_pSelectedTune->GetPatternData(Channel, Pattern, Row);

				Note->EffNumber[0]	= EffectNumber;
				Note->EffParam[0]	= EffectParam;
			}
			else {
				for (int n = 0; n < (m_pSelectedTune->GetEffectColumnCount(Channel) + 1); ++n) {
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

					Note->EffNumber[n]	= EffectNumber;
					Note->EffParam[n] 	= EffectParam;
				}
			}

			if (Note->Vol > 0x10)
				Note->Vol &= 0x0F;

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
			}

			if (Version == 3) {
				// Fix for VRC7 portamento
				if (GetExpansionChip() == SNDCHIP_VRC7 && Channel > 4) {
					for (int n = 0; n < MAX_EFFECT_COLUMNS; ++n) {
						switch (Note->EffNumber[n]) {
							case EF_PORTA_DOWN:
								Note->EffNumber[n] = EF_PORTA_UP;
								break;
							case EF_PORTA_UP:
								Note->EffNumber[n] = EF_PORTA_DOWN;
								break;
						}
					}
				}
				// FDS pitch effect fix
				else if (GetExpansionChip() == SNDCHIP_FDS && Channel == 5) {
					for (int n = 0; n < MAX_EFFECT_COLUMNS; ++n) {
						switch (Note->EffNumber[n]) {
							case EF_PITCH:
								if (Note->EffParam[n] != 0x80)
									Note->EffParam[n] = (0x100 - Note->EffParam[n]) & 0xFF;
								break;
						}
					}
				}
			}
#ifdef TRANSPOSE_FDS
			if (Version < 5) {
				// FDS octave
				if (GetExpansionChip() == SNDCHIP_FDS && Channel > 4 && Note->Octave < 7) {
					Note->Octave++;
				}
			}
#endif
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

bool CFamiTrackerDoc::ImportFile(LPCTSTR lpszPathName, bool bIncludeInstruments)
{
	// TODO: copy messages in this function to string table

	// Import a module as new sub tune
	CFamiTrackerDoc *pImported = new CFamiTrackerDoc();

	// Load into a new document
	if (!pImported->OpenDocument(lpszPathName)) {
		delete pImported;
		return false;
	}

	if (pImported->GetExpansionChip() != GetExpansionChip()) {
		AfxMessageBox(_T("Imported file must be of the same expansion chip type as current file"));
		delete pImported;
		return false;
	}

	// Add a new track to this document
	if (!AddTrack()) {
		delete pImported;
		return false;
	}

	// Imports single tracks only at the moment!

	int NewTrack = GetTrackCount() - 1;

	//SelectTrackFast(NewTrack);
	m_pSelectedTune = m_pTunes[NewTrack];

	// Import first track only at the moment
	int ImportTrack = 0;
	stChanNote data;

	// Copy parameters
	SetPatternLength(pImported->GetPatternLength());
	SetFrameCount(pImported->GetFrameCount());
	SetSongTempo(pImported->GetSongTempo());
	SetSongSpeed(pImported->GetSongSpeed());

	// Copy track name
	SetTrackTitle(NewTrack, pImported->GetTrackTitle(ImportTrack));

	int InstrumentTable[MAX_INSTRUMENTS];
	int SamplesTable[MAX_DSAMPLES];
	int SequenceTable[MAX_SEQUENCES][SEQ_COUNT];
	int SequenceTableVRC6[MAX_SEQUENCES][SEQ_COUNT];

	memset(InstrumentTable, 0, sizeof(int) * MAX_INSTRUMENTS);
	memset(SamplesTable, 0, sizeof(int) * MAX_DSAMPLES);
	memset(SequenceTable, 0, sizeof(int) * MAX_SEQUENCES * SEQ_COUNT);
	memset(SequenceTableVRC6, 0, sizeof(int) * MAX_SEQUENCES * SEQ_COUNT);

	if (bIncludeInstruments) {

		// Check instrument count
		if (GetInstrumentCount() + pImported->GetInstrumentCount() >= MAX_INSTRUMENTS) {
			m_pSelectedTune = m_pTunes[m_iTrack];
			RemoveTrack(NewTrack);
			delete pImported;
			AfxMessageBox(_T("Can't import file, number of instruments will exceed maximum allowed count."), MB_ICONERROR);
			return false;
		}

		// Copy sequences
		for (int s = 0; s < MAX_SEQUENCES; ++s) {
			for (int t = 0; t < SEQ_COUNT; ++t) {
				// 2A03
				if (pImported->GetSequenceItemCount(s, t) > 0) {
					CSequence *pImportSeq = pImported->GetSequence(s, t);
					int index = GetFreeSequence(t);
					CSequence *pSeq = GetSequence(index, t);
					pSeq->Copy(pImportSeq);
					// Save a reference to this sequence
					SequenceTable[s][t] = index;
				}
				// VRC6
				if (pImported->GetSequenceItemCountVRC6(s, t) > 0) {
					CSequence *pImportSeq = pImported->GetSequenceVRC6(s, t);
					int index = GetFreeSequenceVRC6(t);
					CSequence *pSeq = GetSequenceVRC6(index, t);
					pSeq->Copy(pImportSeq);
					// Save a reference to this sequence
					SequenceTableVRC6[s][t] = index;
				}
			}
		}

		bool bOutOfSampleSpace = false;

		// Copy DPCM samples
		for (int i = 0; i < MAX_DSAMPLES; ++i) {
			CDSample *pImportDSample = pImported->GetDSample(i);
			if (pImportDSample->SampleSize > 0) {
				int Index = GetFreeDSample();
				if (Index != -1) {
					CDSample *pDSample = GetDSample(Index);
					pDSample->Copy(pImportDSample);
					// Save a reference to this DPCM sample
					SamplesTable[i] = Index;
				}
				else
					bOutOfSampleSpace = true;
			}
		}

		if (bOutOfSampleSpace) {
			AfxMessageBox(_T("Could not import all samples, out of sample slots!"), MB_ICONEXCLAMATION);
		}

		// Copy instruments
		for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
			if (pImported->IsInstrumentUsed(i)) {
				CInstrument *pInst = pImported->GetInstrument(i)->Clone();
				// Update references
				switch (pInst->GetType()) {
					case INST_2A03: 
						{
							CInstrument2A03 *pInstrument = (CInstrument2A03*)pInst;
							// Update sequence references
							for (int t = 0; t < SEQ_COUNT; ++t) {
								if (pInstrument->GetSeqEnable(t)) {
									pInstrument->SetSeqIndex(t, SequenceTable[pInstrument->GetSeqIndex(t)][t]);
								}
							}
							// Update DPCM samples
							for (int o = 0; o < OCTAVE_RANGE; ++o) {
								for (int n = 0; n < NOTE_RANGE; ++n) {
									int Sample = pInstrument->GetSample(o, n);
									if (Sample != 0) {
										pInstrument->SetSample(o, n, SamplesTable[Sample - 1] + 1);
									}
								}
							}
						}
						break;
					case INST_VRC6: 
						{
							CInstrumentVRC6 *pInstrument = (CInstrumentVRC6*)pInst;
							// Update sequence references
							for (int t = 0; t < SEQ_COUNT; ++t) {
								if (pInstrument->GetSeqEnable(t)) {
									pInstrument->SetSeqIndex(t, SequenceTableVRC6[pInstrument->GetSeqIndex(t)][t]);
								}
							}
						}
						break;
				}
				// Update samples
				int Index = AddInstrument(pInst);
				// Save a reference to this instrument
				InstrumentTable[i] = Index;
			}
		}

	}

	// Copy frames
	for (unsigned int f = 0; f < pImported->GetFrameCount(); ++f) {
		for (unsigned int c = 0; c < GetAvailableChannels(); ++c) {
			SetPatternAtFrame(f, c, pImported->GetPatternAtFrame(f, c));
		}
	}

	// Copy patterns
	for (unsigned int p = 0; p < MAX_PATTERN; ++p) {
		for (unsigned int c = 0; c < GetAvailableChannels(); ++c) {
			for (unsigned int r = 0; r < pImported->GetPatternLength(); ++r) {
				// Get note
				pImported->GetDataAtPattern(ImportTrack, p, c, r, &data);
				// Translate instrument number
				if (data.Instrument != MAX_INSTRUMENTS && bIncludeInstruments) {
					data.Instrument = InstrumentTable[data.Instrument];
				}
				// Store
				SetDataAtPattern(NewTrack, p, c, r, &data);
			}
		}
	}

	// Effect columns
	for (unsigned int c = 0; c < GetAvailableChannels(); ++c) {
		SetEffColumns(c, pImported->GetEffColumns(c));
	}

	delete pImported;

	m_pSelectedTune = m_pTunes[m_iTrack];

	// Rebuild instrument list
	SetModifiedFlag();
	UpdateAllViews(NULL, UPDATE_INSTRUMENTS);
	UpdateAllViews(NULL, CHANGED_PATTERN);

	return true;
}

// End of file load/save

// DMC Stuff

CDSample *CFamiTrackerDoc::GetDSample(unsigned int Index)
{
	ASSERT(Index < MAX_DSAMPLES);
	return &m_DSamples[Index];
}

void CFamiTrackerDoc::GetSampleName(unsigned int Index, char *Name) const
{
	ASSERT(Index < MAX_DSAMPLES);
	ASSERT(m_DSamples[Index].SampleSize > 0);
	strcpy(Name, m_DSamples[Index].Name);
}

int CFamiTrackerDoc::GetFreeDSample() const
{
	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (m_DSamples[i].SampleSize == 0) {
			return i;
		}
	}
	// Out of free samples
	return -1;
}

void CFamiTrackerDoc::RemoveDSample(unsigned int Index)
{
	ASSERT(Index < MAX_DSAMPLES);

	if (m_DSamples[Index].SampleSize != 0) {
		SAFE_RELEASE_ARRAY(m_DSamples[Index].SampleData);
		m_DSamples[Index].SampleSize = 0;
		SetModifiedFlag();
	}
}

int CFamiTrackerDoc::GetTotalSampleSize() const
{
	// Return total size of all loaded samples
	int Size = 0;
	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		Size += m_DSamples[i].SampleSize;
	}
	return Size;

}

// ---------------------------------------------------------------------------------------------------------
// Document access functions
// ---------------------------------------------------------------------------------------------------------

void CFamiTrackerDoc::SetSongInfo(const char *Name, const char *Artist, const char *Copyright)
{
	ASSERT(Name != NULL && Artist != NULL && Copyright != NULL);

	// Check if strings actually changed
	if (strcmp(Name, m_strName) || strcmp(Artist, m_strArtist) || strcmp(Copyright, m_strCopyright))
		SetModifiedFlag();

	strncpy(m_strName, Name, 32);
	strncpy(m_strArtist, Artist, 32);
	strncpy(m_strCopyright, Copyright, 32);

	// Limit strings to 31 characters to make room for null-termination
	m_strName[31] = 0;
	m_strArtist[31] = 0;
	m_strCopyright[31] = 0;
}

// Sequences

CSequence *CFamiTrackerDoc::GetSequence(int Chip, int Index, int Type)
{
	ASSERT(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	switch (Chip) {
		case SNDCHIP_NONE: 
			return GetSequence(Index, Type);
		case SNDCHIP_VRC6: 
			return GetSequenceVRC6(Index, Type);
		case SNDCHIP_N106: 
			return GetSequenceN106(Index, Type);
	}

	return NULL;
}

CSequence *CFamiTrackerDoc::GetSequence(int Index, int Type)
{
	ASSERT(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	if (m_pSequences2A03[Index][Type] == NULL)
		m_pSequences2A03[Index][Type] = new CSequence();

	return m_pSequences2A03[Index][Type];
}

CSequenceInterface const *CFamiTrackerDoc::GetSequence(int Index, int Type) const
{
	ASSERT(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);
	return dynamic_cast<CSequenceInterface const *>(m_pSequences2A03[Index][Type]);
}

int CFamiTrackerDoc::GetSequenceItemCount(int Index, int Type) const
{
	ASSERT(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);
	
	if (m_pSequences2A03[Index][Type] == NULL)
		return 0;

	return m_pSequences2A03[Index][Type]->GetItemCount();
}

int CFamiTrackerDoc::GetFreeSequence(int Type) const
{
	// Return a free sequence slot
	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		if (GetSequenceItemCount(i, Type) == 0)
			return i;
	}
	return 0;
}

int CFamiTrackerDoc::GetSequenceCount(int Type) const
{
	// Return number of allocated sequences of Type
	int Count = 0;
	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		if (GetSequenceItemCount(i, Type) > 0)
			++Count;
	}
	return Count;
}

// VRC6

CSequence *CFamiTrackerDoc::GetSequenceVRC6(int Index, int Type)
{
	ASSERT(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	if (m_pSequencesVRC6[Index][Type] == NULL)
		m_pSequencesVRC6[Index][Type] = new CSequence();

	return m_pSequencesVRC6[Index][Type];
}

CSequence *CFamiTrackerDoc::GetSequenceVRC6(int Index, int Type) const
{
	ASSERT(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);
	return m_pSequencesVRC6[Index][Type];
}

int CFamiTrackerDoc::GetSequenceItemCountVRC6(int Index, int Type) const
{
	ASSERT(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	if (m_pSequencesVRC6[Index][Type] == NULL)
		return 0;

	return m_pSequencesVRC6[Index][Type]->GetItemCount();
}

int CFamiTrackerDoc::GetFreeSequenceVRC6(int Type) const
{
	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		if (GetSequenceItemCountVRC6(i, Type) == 0)
			return i;
	}
	return 0;
}

// N106

CSequence *CFamiTrackerDoc::GetSequenceN106(int Index, int Type)
{
	ASSERT(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	if (m_pSequencesN106[Index][Type] == NULL)
		m_pSequencesN106[Index][Type] = new CSequence();

	return m_pSequencesN106[Index][Type];
}

CSequence *CFamiTrackerDoc::GetSequenceN106(int Index, int Type) const
{
	ASSERT(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);
	return m_pSequencesN106[Index][Type];
}

int CFamiTrackerDoc::GetSequenceItemCountN106(int Index, int Type) const
{
	ASSERT(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	if (m_pSequencesN106[Index][Type] == NULL)
		return 0;

	return m_pSequencesN106[Index][Type]->GetItemCount();
}

int CFamiTrackerDoc::GetFreeSequenceN106(int Type) const
{
	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		if (GetSequenceItemCountN106(i, Type) == 0)
			return i;
	}
	return 0;
}


//
// Instruments
//

CInstrument *CFamiTrackerDoc::GetInstrument(int Index)
{
	// This may return a NULL pointer
	ASSERT(Index >= 0 && Index < MAX_INSTRUMENTS);
	return m_pInstruments[Index];
}

CInstrument2A03Interface const *CFamiTrackerDoc::Get2A03Instrument(int Instrument) const
{
	// This may return a NULL pointer
	ASSERT(Instrument >= 0 && Instrument < MAX_INSTRUMENTS);
	return dynamic_cast<CInstrument2A03Interface const *>(m_pInstruments[Instrument]);
}

int CFamiTrackerDoc::GetInstrumentCount() const
{
	int count = 0;
	for(int i = 0; i < MAX_INSTRUMENTS; ++i)
	{
		if( IsInstrumentUsed( i ) ) ++count;
	}
	return count;
}

bool CFamiTrackerDoc::IsInstrumentUsed(int Index) const
{
	ASSERT(Index >= 0 && Index < MAX_INSTRUMENTS);
	return !(m_pInstruments[Index] == NULL);
}

CInstrument *CFamiTrackerDoc::CreateInstrument(int InstType)
{
	// Creates a new instrument of selected type
	switch (InstType) {
		case INST_2A03: return new CInstrument2A03();
		case INST_VRC6: return new CInstrumentVRC6(); 
		case INST_VRC7: return new CInstrumentVRC7();
		case INST_N106:	return new CInstrumentN106();
		case INST_FDS: return new CInstrumentFDS();
		case INST_S5B: return new CInstrumentS5B();
	}

	return NULL;
}

int CFamiTrackerDoc::FindFreeInstrumentSlot()
{
	// Returns a free instrument slot, or -1 if no free slots exists
	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (m_pInstruments[i] == NULL)
			return i;
	}
	return -1;
}

int CFamiTrackerDoc::AddInstrument(CInstrument *pInst)
{
	// Add an existing instrument to instrument list

	int Slot = FindFreeInstrumentSlot();

	if (Slot == -1)
		return -1;

	m_pInstruments[Slot] = pInst;

	SetModifiedFlag();
	UpdateAllViews(NULL, UPDATE_ENTIRE);

	return Slot;
}

int CFamiTrackerDoc::AddInstrument(const char *Name, int ChipType)
{
	// Adds a new instrument to the module
	int Slot = FindFreeInstrumentSlot();

	if (Slot == -1)
		return -1;

	m_pInstruments[Slot] = theApp.GetChannelMap()->GetChipInstrument(ChipType);

	if (m_pInstruments[Slot] == NULL) {
#ifdef _DEBUG
		MessageBox(NULL, _T("(TODO) add instrument definitions for this chip"), _T("Stop"), MB_OK);
#endif
		return -1;
	}

	// TODO: move this to instrument classes
	
	switch (ChipType) {
		case SNDCHIP_NONE:
		case SNDCHIP_MMC5:
			for (int i = 0; i < SEQ_COUNT; ++i) {
				((CInstrument2A03*)m_pInstruments[Slot])->SetSeqEnable(i, 0);
				((CInstrument2A03*)m_pInstruments[Slot])->SetSeqIndex(i, GetFreeSequence(i));
			}
			break;
		case SNDCHIP_VRC6:
			for (int i = 0; i < SEQ_COUNT; ++i) {
				((CInstrumentVRC6*)m_pInstruments[Slot])->SetSeqEnable(i, 0);
				((CInstrumentVRC6*)m_pInstruments[Slot])->SetSeqIndex(i, GetFreeSequenceVRC6(i));
			}
			break;
	}

	m_pInstruments[Slot]->SetName(Name);

	SetModifiedFlag();
	UpdateAllViews(NULL, UPDATE_ENTIRE);

	return Slot;
}

void CFamiTrackerDoc::RemoveInstrument(unsigned int Index)
{
	// Removes an instrument from the module

	ASSERT(Index < MAX_INSTRUMENTS);
	
	if (m_pInstruments[Index] == NULL)
		return;

	SAFE_RELEASE(m_pInstruments[Index]);

	SetModifiedFlag();
	UpdateAllViews(NULL, UPDATE_ENTIRE);
}

int CFamiTrackerDoc::CloneInstrument(unsigned int Index)
{
	ASSERT(Index < MAX_INSTRUMENTS);

	if (!IsInstrumentUsed(Index))
		return -1;

	int Slot = FindFreeInstrumentSlot();

	if (Slot != -1) {
		m_pInstruments[Slot] = GetInstrument(Index)->Clone();
		SetModifiedFlag();
		//UpdateAllViews(NULL, UPDATE_INSTRUMENTS);
	}

	return Slot;
}

void CFamiTrackerDoc::GetInstrumentName(unsigned int Index, char *Name) const
{
	ASSERT(Index < MAX_INSTRUMENTS);
	ASSERT(m_pInstruments[Index] != NULL);

	m_pInstruments[Index]->GetName(Name);
}

void CFamiTrackerDoc::SetInstrumentName(unsigned int Index, const char *Name)
{
	ASSERT(Index < MAX_INSTRUMENTS);
	ASSERT(m_pInstruments[Index] != NULL);

	if (m_pInstruments[Index] != NULL) {
		if (strcmp(m_pInstruments[Index]->GetName(), Name) != 0) {
			m_pInstruments[Index]->SetName(Name);
			SetModifiedFlag();
		}
	}
}

int CFamiTrackerDoc::GetInstrumentType(unsigned int Index) const
{
	ASSERT(Index < MAX_INSTRUMENTS);

	if (!IsInstrumentUsed(Index))
		return INST_NONE;

	return m_pInstruments[Index]->GetType();
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

	if (m_pSelectedTune->GetFrameCount()!= Count) {
		m_pSelectedTune->SetFrameCount(Count);
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::SetPatternLength(unsigned int Length)
{ 
	ASSERT(Length <= MAX_PATTERN_LENGTH);

	if (m_pSelectedTune->GetPatternLength() != Length) {
		m_pSelectedTune->SetPatternLength(Length);
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::SetSongSpeed(unsigned int Speed)
{
	ASSERT(Speed <= MAX_TEMPO);
	
	if (m_pSelectedTune->GetSongSpeed() != Speed) {
		m_pSelectedTune->SetSongSpeed(Speed);
		SetModifiedFlag();
	}
}

void CFamiTrackerDoc::SetSongTempo(unsigned int Tempo)
{
	ASSERT(Tempo <= MAX_TEMPO);

	if (m_pSelectedTune->GetSongTempo() != Tempo) {
		m_pSelectedTune->SetSongTempo(Tempo);
		SetModifiedFlag();
	}
}

unsigned int CFamiTrackerDoc::GetEffColumns(int Track, unsigned int Channel) const
{
	ASSERT(Channel < MAX_CHANNELS);
	return m_pTunes[Track]->GetEffectColumnCount(Channel);
}

unsigned int CFamiTrackerDoc::GetEffColumns(unsigned int Channel) const
{
	ASSERT(Channel < MAX_CHANNELS);
	return m_pSelectedTune->GetEffectColumnCount(Channel);
}

void CFamiTrackerDoc::SetEffColumns(unsigned int Channel, unsigned int Columns)
{
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Columns < MAX_EFFECT_COLUMNS);

	GetChannel(Channel)->SetColumnCount(Columns);
	m_pSelectedTune->SetEffectColumnCount(Channel, Columns);

	SetModifiedFlag();
	// Erase background to calculate new pattern editor width
	UpdateAllViews(NULL, CHANGED_ERASE);	// TODO: fix this
}

void CFamiTrackerDoc::SetEngineSpeed(unsigned int Speed)
{
	ASSERT(Speed <= 800); // hardcoded at the moment, TODO: fix this
	ASSERT(Speed >= 10 || Speed == 0);

	m_iEngineSpeed = Speed;
	SetModifiedFlag();
}

void CFamiTrackerDoc::SetMachine(unsigned int Machine)
{
	ASSERT(Machine == PAL || Machine == NTSC);
	m_iMachine = Machine;
	SetModifiedFlag();
}

unsigned int CFamiTrackerDoc::GetPatternAtFrame(int Track, unsigned int Frame, unsigned int Channel) const
{
	ASSERT(Frame < MAX_FRAMES && Channel < MAX_CHANNELS);
	return m_pTunes[Track]->GetFramePattern(Frame, Channel);
}

unsigned int CFamiTrackerDoc::GetPatternAtFrame(unsigned int Frame, unsigned int Channel) const
{
	ASSERT(Frame < MAX_FRAMES && Channel < MAX_CHANNELS);
	return m_pSelectedTune->GetFramePattern(Frame, Channel);
}

void CFamiTrackerDoc::SetPatternAtFrame(unsigned int Frame, unsigned int Channel, unsigned int Pattern)
{
	ASSERT(Frame < MAX_FRAMES && Channel < MAX_CHANNELS && Pattern < MAX_PATTERN);
	m_pSelectedTune->SetFramePattern(Frame, Channel, Pattern);
//	SetModifiedFlag();
}

unsigned int CFamiTrackerDoc::GetFrameRate(void) const
{
	if (m_iEngineSpeed == 0)
		return (m_iMachine == NTSC) ? CAPU::FRAME_RATE_NTSC : CAPU::FRAME_RATE_PAL;
	
	return m_iEngineSpeed;
}

//
// Pattern editing
//

void CFamiTrackerDoc::IncreasePattern(unsigned int Frame, unsigned int Channel, int Count)
{
	ASSERT(Frame < MAX_FRAMES && Channel < MAX_CHANNELS);

	int Current = m_pSelectedTune->GetFramePattern(Frame, Channel);

	// Selects the next channel pattern
	if ((Current + Count) < (MAX_PATTERN - 1)) {
		m_pSelectedTune->SetFramePattern(Frame, Channel, Current + Count);
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
	else {
		m_pSelectedTune->SetFramePattern(Frame, Channel, MAX_PATTERN - 1);
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
}

void CFamiTrackerDoc::DecreasePattern(unsigned int Frame, unsigned int Channel, int Count)
{
	ASSERT(Frame < MAX_FRAMES && Channel < MAX_CHANNELS);

	int Current = m_pSelectedTune->GetFramePattern(Frame, Channel);

	// Selects the previous channel pattern
	if (Current > Count) {
		m_pSelectedTune->SetFramePattern(Frame, Channel, Current - Count);
		SetModifiedFlag();
		UpdateAllViews(NULL);
	}
	else {
		m_pSelectedTune->SetFramePattern(Frame, Channel, 0);
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

	if (Vol < 16) {
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
	memcpy(Data, m_pTunes[Track]->GetPatternData(Channel, Pattern, Row), sizeof(stChanNote));
}

unsigned int CFamiTrackerDoc::GetNoteEffectType(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);
	ASSERT(Index < MAX_EFFECT_COLUMNS);

	return m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->EffNumber[Index];
}

unsigned int CFamiTrackerDoc::GetNoteEffectParam(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);
	ASSERT(Index < MAX_EFFECT_COLUMNS);

	return m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->EffParam[Index];
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

	Note.Note		= 0;
	Note.Octave		= 0;
	Note.Instrument	= MAX_INSTRUMENTS;
	Note.Vol		= 0x10;

	for (unsigned int i = m_pSelectedTune->GetPatternLength() - 1; i > Row; i--) {
		memcpy(m_pSelectedTune->GetPatternData(Channel, m_pSelectedTune->GetFramePattern(Frame, Channel), i), 
			m_pSelectedTune->GetPatternData(Channel, m_pSelectedTune->GetFramePattern(Frame, Channel), i - 1), 
			sizeof(stChanNote));
	}

	*m_pSelectedTune->GetPatternData(Channel, m_pSelectedTune->GetFramePattern(Frame, Channel), Row) = Note;

	SetModifiedFlag();

	return true;
}

bool CFamiTrackerDoc::DeleteNote(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Column)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	stChanNote *Note = m_pSelectedTune->GetPatternData(Channel, m_pSelectedTune->GetFramePattern(Frame, Channel), Row);

	if (Column == C_NOTE) {
		Note->Note = 0;
		Note->Octave = 0;
		Note->Instrument = MAX_INSTRUMENTS;
		Note->Vol = 0x10;
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


bool CFamiTrackerDoc::ClearRow(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	stChanNote *Note = m_pSelectedTune->GetPatternData(Channel, m_pSelectedTune->GetFramePattern(Frame, Channel), Row);

	Note->Note = 0;
	Note->Octave = 0;
	Note->Instrument = MAX_INSTRUMENTS;
	Note->Vol = 0x10;
	
	SetModifiedFlag();

	return true;
}

bool CFamiTrackerDoc::RemoveNote(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	stChanNote Note;

	for (int n = 0; n < MAX_EFFECT_COLUMNS; n++) {
		Note.EffNumber[n] = 0;
		Note.EffParam[n] = 0;
	}

	Note.Note = 0;
	Note.Octave = 0;
	Note.Instrument = MAX_INSTRUMENTS;
	Note.Vol = 0x10;

	for (unsigned int i = Row - 1; i < (m_pSelectedTune->GetPatternLength() - 1); i++) {
		memcpy(m_pSelectedTune->GetPatternData(Channel, m_pSelectedTune->GetFramePattern(Frame, Channel), i), 
			m_pSelectedTune->GetPatternData(Channel, m_pSelectedTune->GetFramePattern(Frame, Channel), i + 1),
			sizeof(stChanNote));
	}

	*m_pSelectedTune->GetPatternData(Channel, m_pSelectedTune->GetFramePattern(Frame, Channel), m_pSelectedTune->GetPatternLength() - 1) = Note;

	SetModifiedFlag();

	return true;
}

// Track functions

void CFamiTrackerDoc::SelectTrack(unsigned int Track)
{
	ASSERT(Track < MAX_TRACKS);
	SwitchToTrack(Track);
	UpdateAllViews(0, CHANGED_TRACK);
}

void CFamiTrackerDoc::SwitchToTrack(unsigned int Track)
{
	// Select a new track, initialize if it wasn't
	// TODO: should this fail if track didn't exist?
	m_iTrack = Track;
	AllocateSong(Track);
	m_pSelectedTune = m_pTunes[Track];
}

char *CFamiTrackerDoc::GetTrackTitle(unsigned int Track) const
{
	return (char*)m_sTrackNames[Track].GetString();
}

bool CFamiTrackerDoc::AddTrack()
{
	if (m_iTracks >= (MAX_TRACKS - 1))
		return false;

	AllocateSong(++m_iTracks);

	SetModifiedFlag();
	UpdateAllViews(NULL, CHANGED_TRACKCOUNT);

	return true;
}

void CFamiTrackerDoc::RemoveTrack(unsigned int Track)
{
	ASSERT(m_iTracks > 0);
	ASSERT(m_pTunes[Track] != NULL);

	delete m_pTunes[Track];

	// Move down all other tracks
	for (unsigned int i = Track; i < m_iTracks; ++i) {
		m_sTrackNames[i] = m_sTrackNames[i + 1];
		m_pTunes[i] = m_pTunes[i + 1];
	}

	m_pTunes[m_iTracks] = NULL;

	--m_iTracks;

	if (m_iTrack > m_iTracks)
		m_iTrack = m_iTracks;	// Last track was removed

	SwitchToTrack(m_iTrack);

	SetModifiedFlag();
	UpdateAllViews(NULL, CHANGED_TRACKCOUNT);
}

void CFamiTrackerDoc::SetTrackTitle(unsigned int Track, CString Title)
{
	if (m_sTrackNames[Track] == Title)
		return;

	m_sTrackNames[Track] = Title;

	SetModifiedFlag();
	UpdateAllViews(NULL, CHANGED_TRACKCOUNT);
}

void CFamiTrackerDoc::MoveTrackUp(unsigned int Track)
{
	CString Temp = m_sTrackNames[Track];
	m_sTrackNames[Track] = m_sTrackNames[Track - 1];
	m_sTrackNames[Track - 1] = Temp;

	CPatternData *pTemp = m_pTunes[Track];
	m_pTunes[Track] = m_pTunes[Track - 1];
	m_pTunes[Track - 1] = pTemp;

	SetModifiedFlag();
	UpdateAllViews(NULL, CHANGED_TRACKCOUNT);
}

void CFamiTrackerDoc::MoveTrackDown(unsigned int Track)
{
	CString Temp = m_sTrackNames[Track];
	m_sTrackNames[Track] = m_sTrackNames[Track + 1];
	m_sTrackNames[Track + 1] = Temp;

	CPatternData *pTemp = m_pTunes[Track];
	m_pTunes[Track] = m_pTunes[Track + 1];
	m_pTunes[Track + 1] = pTemp;

	SetModifiedFlag();
	UpdateAllViews(NULL, CHANGED_TRACKCOUNT);
}

void CFamiTrackerDoc::AllocateSong(unsigned int Song)
{
	// Allocate a new song if not already done
	if (m_pTunes[Song] == NULL) {
		int Tempo = (m_iMachine == NTSC) ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL;
		m_pTunes[Song] = new CPatternData(DEFAULT_ROW_COUNT, DEFAULT_SPEED, Tempo);
		m_sTrackNames[Song] = DEFAULT_TRACK_NAME;
	}

//	SetModifiedFlag();
}

unsigned int CFamiTrackerDoc::GetTrackCount() const
{
	return m_iTracks + 1;
}

unsigned int CFamiTrackerDoc::GetSelectedTrack() const
{
	// TODO: remove
	return m_iTrack;
}

void CFamiTrackerDoc::SelectExpansionChip(unsigned char Chip)
{
	// Store the chip
	m_iExpansionChip = Chip;

	// Register the channels
	theApp.GetSoundGenerator()->RegisterChannels(Chip); 

	m_iChannelsAvailable = GetChannelCount();

	SetModifiedFlag();
	UpdateAllViews(NULL, CHANGED_CHANNEL_COUNT);
}

bool CFamiTrackerDoc::ExpansionEnabled(int Chip) const
{
	// Returns true if a specified chip is enabled
	return (GetExpansionChip() & Chip) == Chip; 
}	

// Instrument functions

void CFamiTrackerDoc::SaveInstrument(unsigned int Instrument, CString FileName)
{
	// Saves an instrument to a file
	CFile InstrumentFile(FileName, CFile::modeCreate | CFile::modeWrite);
	CInstrument *pInstrument = GetInstrument(Instrument);

	ASSERT(pInstrument != NULL);
	
	if (InstrumentFile.m_hFile == CFile::hFileNull) {
		AfxMessageBox(IDS_INVALID_INST_FILE, MB_ICONERROR);
		return;
	}

	// Write header
	InstrumentFile.Write(INST_HEADER, (UINT)strlen(INST_HEADER));
	InstrumentFile.Write(INST_VERSION, (UINT)strlen(INST_VERSION));

	// Write type
	char InstType = pInstrument->GetType();
	InstrumentFile.Write(&InstType, sizeof(char));

	// Write name
	char Name[256];
	pInstrument->GetName(Name);
	int NameSize = (int)strlen(Name);
	InstrumentFile.Write(&NameSize, sizeof(int));
	InstrumentFile.Write(Name, NameSize);

	// Write instrument data
	pInstrument->SaveFile(&InstrumentFile, this);

	InstrumentFile.Close();
}

int CFamiTrackerDoc::LoadInstrument(CString FileName)
{
	int Slot = FindFreeInstrumentSlot();

	if (Slot == -1) {
		AfxMessageBox(IDS_INST_LIMIT, MB_ICONERROR);
		return -1;
	}

	// Open file
	CFile InstrumentFile(FileName, CFile::modeRead);

	if (InstrumentFile.m_hFile == CFile::hFileNull) {
		AfxMessageBox(IDS_INVALID_INST_FILE, MB_ICONERROR);
		return -1;
	}

	// Signature
	char Text[256];
	InstrumentFile.Read(Text, (UINT)strlen(INST_HEADER));
	Text[strlen(INST_HEADER)] = 0;

	if (strcmp(Text, INST_HEADER) != 0) {
		AfxMessageBox(IDS_INSTRUMENT_FILE_FAIL, MB_ICONERROR);
		InstrumentFile.Close();
		return -1;
	}

	// Version
	InstrumentFile.Read(Text, (UINT)strlen(INST_VERSION));

	int iInstMaj, iInstMin;

	sscanf(Text, "%i.%i", &iInstMaj, &iInstMin);
	int iInstVer = iInstMaj * 10 + iInstMin;
	
	sscanf(INST_VERSION, "%i.%i", &iInstMaj, &iInstMin);
	int iCurrentVer = iInstMaj * 10 + iInstMin;

	if (iInstVer > iCurrentVer) {
		AfxMessageBox(_T("File version not supported!"), MB_OK);
		InstrumentFile.Close();
		return -1;
	}

	theApp.GetSoundGenerator()->LockDocument();

	// Type
	char InstType;
	InstrumentFile.Read(&InstType, sizeof(char));

	if (!InstType)
		InstType = INST_2A03;

	CInstrument *pInstrument = CreateInstrument(InstType);
	m_pInstruments[Slot] = pInstrument;

	// Name
	unsigned int NameLen;
	InstrumentFile.Read(&NameLen, sizeof(int));

	if (NameLen >= 256) {
		theApp.GetSoundGenerator()->UnlockDocument();
		AfxMessageBox(_T("Operation failed!"), MB_OK);
		m_pInstruments[Slot] = NULL;
		SAFE_RELEASE(pInstrument);
		return -1;
	}

	InstrumentFile.Read(Text, NameLen);
	Text[NameLen] = 0;

	pInstrument->SetName(Text);

	if (!pInstrument->LoadFile(&InstrumentFile, iInstVer, this)) {
		theApp.GetSoundGenerator()->UnlockDocument();
		AfxMessageBox(_T("Operation failed!"), MB_OK);
		m_pInstruments[Slot] = NULL;
		SAFE_RELEASE(pInstrument);
		return -1;
	}

	theApp.GetSoundGenerator()->UnlockDocument();

	return Slot;
}


void CFamiTrackerDoc::ConvertSequence(stSequence *OldSequence, CSequence *NewSequence, int Type)
{
	// This function is used to convert old version sequences (used by older file versions)
	// to the current version

	if (OldSequence->Count > 0 && OldSequence->Count < MAX_SEQUENCE_ITEMS) {

		// Save a pointer to this
		int iLoopPoint = -1;
		int iLength = 0;
		int ValPtr = 0;

		// Store the sequence
		int Count = OldSequence->Count;

		for (int k = 0; k < Count; ++k) {
			int Value	= OldSequence->Value[k];
			int Length	= OldSequence->Length[k];

			if (Length < 0) {
				iLoopPoint = 0;
				for (int l = signed(OldSequence->Count) + Length - 1; l < signed(OldSequence->Count) - 1; l++)
					iLoopPoint += (OldSequence->Length[l] + 1);
			}
			else {
				for (int l = 0; l < Length + 1; l++) {
					if ((Type == SEQ_PITCH || Type == SEQ_HIPITCH) && l > 0)
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
	for (int i = 0; i < MAX_PATTERN; ++i) {
		if (!m_pSelectedTune->IsPatternInUse(Channel, i) && m_pSelectedTune->IsPatternEmpty(Channel, i))
			return i;

		// Changed v0.3.7, patterns can't be used earlier
		//if (m_pSelectedTune->IsPatternFree(Channel, i))
		//	return i;
	}

	return 0;
}

void CFamiTrackerDoc::ClearPatterns()
{
	m_pSelectedTune->ClearEverything();
}

// Channel interface, these functions must be synchronized!!!

int CFamiTrackerDoc::GetChannelType(int Channel) const
{
	ASSERT(m_iRegisteredChannels != 0);
	ASSERT(Channel < m_iRegisteredChannels);
	return m_iChannelTypes[Channel];
}

int CFamiTrackerDoc::GetChipType(int Channel) const
{
	ASSERT(m_iRegisteredChannels != 0);
	ASSERT(Channel < m_iRegisteredChannels);
	return m_pChannels[Channel]->GetChip();
}

int CFamiTrackerDoc::GetChannelCount() const
{
	return m_iRegisteredChannels;
}

void CFamiTrackerDoc::ResetChannels()
{
	// Clears all channels from the document
	m_iRegisteredChannels = 0;
}

void CFamiTrackerDoc::RegisterChannel(CTrackerChannel *pChannel, int ChannelType, int ChipType)
{
	// Adds a channel to the document
	m_pChannels[m_iRegisteredChannels] = pChannel;
	m_iChannelTypes[m_iRegisteredChannels] = ChannelType;
	m_iChannelChip[m_iRegisteredChannels] = ChipType;
	m_iRegisteredChannels++;
}

CTrackerChannel *CFamiTrackerDoc::GetChannel(int Index) const
{
	ASSERT(m_iRegisteredChannels != 0 && Index < m_iRegisteredChannels);
	ASSERT(m_pChannels[Index] != NULL);
	return m_pChannels[Index];
}

// DPCM samples

int CFamiTrackerDoc::GetSampleCount() const
{
	int Count = 0;
	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (m_DSamples[i].SampleSize > 0)
			++Count;
	}
	return Count;
}

// Vibrato functions

int CFamiTrackerDoc::GetVibratoStyle() const
{
	return m_iVibratoStyle;
}

void CFamiTrackerDoc::SetVibratoStyle(int Style)
{
	m_iVibratoStyle = Style;
	theApp.GetSoundGenerator()->GenerateVibratoTable(Style);
}

// Attributes

CString CFamiTrackerDoc::GetFileTitle() const 
{
	// Return file name without extension
	CString FileName = GetTitle();

	// Remove extension
	if (FileName.Right(4).CompareNoCase(_T(".ftm")) == 0)
		return FileName.Left(FileName.GetLength() - 4);

	return FileName;
}

bool CFamiTrackerDoc::IsFileLoaded()
{
	bool bResult;
	m_csLoadedLock.Lock();
	bResult = m_bFileLoaded;
	m_csLoadedLock.Unlock();
	return bResult;
}

// Auto-save (experimental)
/*
void CFamiTrackerDoc::SetupAutoSave()
{
	TCHAR TempPath[MAX_PATH], TempFile[MAX_PATH];

	GetTempPath(MAX_PATH, TempPath);
	GetTempFileName(TempPath, _T("Aut"), 21587, TempFile);

	// Check if file exists
	CFile file;
	if (file.Open(TempFile, CFile::modeRead)) {
		file.Close();
		if (AfxMessageBox(_T("It might be possible to recover last document, do you want to try?"), MB_YESNO) == IDYES) {
			OpenDocument(TempFile);
			SelectExpansionChip(m_iExpansionChip);
		}
		else {
			DeleteFile(TempFile);
		}
	}

	TRACE("Doc: Allocated file for auto save: ");
	TRACE(TempFile);
	TRACE("\n");

	m_sAutoSaveFile = TempFile;
}

void CFamiTrackerDoc::ClearAutoSave()
{
	if (m_sAutoSaveFile.GetLength() == 0)
		return;

	DeleteFile(m_sAutoSaveFile);

	m_sAutoSaveFile = _T("");
	m_iAutoSaveCounter = 0;

	TRACE("Doc: Removed auto save file\n");
}

void CFamiTrackerDoc::AutoSave()
{
	// Autosave
	if (!m_iAutoSaveCounter || !m_bFileLoaded || m_sAutoSaveFile.GetLength() == 0)
		return;

	m_iAutoSaveCounter--;

	if (m_iAutoSaveCounter == 0) {
		TRACE("Doc: Performing auto save\n");
		SaveDocument(m_sAutoSaveFile);
	}
}
*/

