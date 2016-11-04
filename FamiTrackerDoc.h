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


#pragma once

const int MAX_INSTRUMENTS		= 64;		// Maximum number of instruments to use
const int MAX_SEQUENCES			= 128;		// Maximum number of sequence lists
const int MAX_SEQ_ITEMS			= 64;		// Maximum number of items in a sequence
const int MAX_PATTERN			= 100;		// Maximum number of patterns per channel
const int MAX_FRAMES			= 100;		// Maximum number of frames
const int MAX_PATTERN_LENGTH	= 128;		// Maximum length of patterns
const int MAX_DSAMPLES			= 32;		// Maximum number of dmc samples (this would be limited by NES memory)

const int MAX_TEMPO = 60;
const int MIN_TEMPO = 0;

enum eModifiers {
	MOD_VOLUME,
	MOD_ARPEGGIO,
	MOD_PITCH,
	MOD_HIPITCH,
	MOD_DUTYCYCLE,
	MOD_COUNT
};

enum eEffects {
	EF_NONE = 0,
	EF_SPEED,		// Axx
	EF_JUMP,		// Bxx
	EF_SKIP,		// Cxx
	EF_HALT,		// Dxx
	EF_VOLUME,		// Exx
	EF_PORTAON,		// Fxx
	EF_PORTAOFF,	// Gxx
	EF_SWEEPUP,		// Hxy
	EF_SWEEPDOWN,	// Ixy
};

const char EFF_CHAR[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I'};

enum eNotes {
	C = 1,
	Cb,
	D,
	Db,
	E,
	F,
	Fb,
	G,
	Gb,
	A,
	Ab,
	B,
	DEL,	// Used by editor, delete note
	HALT,	// Stop channel
};

enum eMachine {
	NTSC,
	PAL
};

struct stInstrument {
	char	Name[256];
	bool	Free;
	int		ModEnable[MOD_COUNT];
	int		ModIndex[MOD_COUNT];
	char	Samples[6][12];				// Samples
	char	SamplePitch[6][12];			// Play pitch
};

struct stSequence {
	signed char Length[MAX_SEQ_ITEMS];
	signed char Value[MAX_SEQ_ITEMS];
	int			Count;
};

struct stDSample {
	char	*SampleData;
	int		SampleSize;
	char	Name[256];
};

class CMainFrame;
class CDocumentFile;

class CFamiTrackerDoc : public CDocument
{
protected: // create from serialization only
	CFamiTrackerDoc();
	DECLARE_DYNCREATE(CFamiTrackerDoc)

//	CCompile		Compile;
//	int				SelectedInstrument;

// Attributes
public:
	// General functions

	// Instruments
	int				AddInstrument(const char *Name);			// Add a new instrument
	void			RemoveInstrument(int Index);				// Remove an instrument
	void			InstrumentName(int Index, char *Name);		// Set the name of an instrument
	void			GetInstName(int Instrument, char *Name);	// Get the name of an instrument

	// Sequence list
	void			InsertModifierItem(int Index, int SelectedSequence);
	void			RemoveModifierItem(int Index, int SelectedSequence);

	// General document

	void			SetPatternCount(int Count);					// ???
	void			SetRowCount(int Index);						// Amount of rows / pattern
	void			SetSongSpeed(int Speed);					// Sets the speed of song

	void			SetSongInfo(char *Name, char *Artist, char *Copyright);

	bool			IncreaseFrame(int PatternPos, int Channel);
	bool			DecreaseFrame(int PatternPos, int Channel);

	void			IncreaseInstrument(int Frame, int Channel, int Row);
	void			DecreaseInstrument(int Frame, int Channel, int Row);
	void			IncreaseEffect(int Frame, int Channel, int Row);
	void			DecreaseEffect(int Frame, int Channel, int Row);
	void			IncreaseVolume(int Frame, int Channel, int Row);
	void			DecreaseVolume(int Frame, int Channel, int Row);

	// Document edit functions

	stChanNote		*GetPatternData(int Pattern, int Nr, int Chan);
	stDSample		*GetFreeDSample();

	stInstrument	*GetInstrument(int Index);
	stSequence		*GetModifier(int Index);
	stDSample		*GetDSample(int Index);
	void			RemoveDSample(int Index);

	bool			DeleteNote(int RowPos, int PatternPos, int Channel, int Column);
	bool			InsertNote(int RowPos, int PatternPos, int Channel);
	bool			RemoveNote(int RowPos, int PatternPos, int Channel);
	void			PlayNote(int Channel, int RowPos, int Frame);

	stChanNote		*GetNoteData(int Channel, int RowPos, int PatternPos);
	void			SetNoteData(int Channel, int RowPos, int PatternPos, stChanNote *Note);
	int				GetNoteEffect(int Channel, int RowPos, int Pattern);
	int				GetNoteEffectValue(int Channel, int RowPos, int Pattern);
	int				GetNote(int Channel, int RowPos, int Pattern);
	void			GetNoteData(stChanNote *Data, int Channel, int RowPos, int Frame);

	void			GetSampleName(int Index, char *Name);

	// General

	char			*CreateNSF(CString FileName, bool KeepSymbols);

	void			WritePatternSymbol(int Pattern, int Channel, int Item);

	// Document data (will be stored/read when opening/saving)

	int				m_iPatternLength;			// Amount of rows in one pattern
	int				m_iFrameCount;				// Number of frames
	int				m_iChannelsAvaliable;		// Number of channels used
	int				m_iSongSpeed;				// Song speed

	int				m_iMachine;					// NTSC / PAL
	int				m_iEngineSpeed;				// Refresh rate

	char			m_strName[32];				// Song name
	char			m_strArtist[32];			// Song artist
	char			m_strCopyright[32];			// Song copyright

	// End of document data

	bool			m_bFileLoaded;				// Is file loaded?

	// Document data

	stInstrument	Instruments[MAX_INSTRUMENTS];
	stSequence		Sequences[MAX_SEQUENCES];
	stDSample		DSamples[MAX_DSAMPLES];

	stChanNote		ChannelData[5][MAX_PATTERN][MAX_PATTERN_LENGTH];
	int				FrameList[MAX_FRAMES][5];					//	<- fix

// Operations
public:

// Overrides
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~CFamiTrackerDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void CleanDocument();

	BOOL SaveDocument(LPCSTR lpszPathName);
	BOOL OpenDocument(LPCTSTR lpszPathName);

	void WriteBlock_Instruments(CDocumentFile *pDocFile);
	void WriteBlock_Sequences(CDocumentFile *pDocFile);
	void WriteBlock_Frames(CDocumentFile *pDocFile);
	void WriteBlock_Patterns(CDocumentFile *pDocFile);
	void WriteBlock_DSamples(CDocumentFile *pDocFile);

	bool ReadBlock_Instruments(CDocumentFile *pDocFile);
	bool ReadBlock_Sequences(CDocumentFile *pDocFile);
	bool ReadBlock_Frames(CDocumentFile *pDocFile);
	bool ReadBlock_Patterns(CDocumentFile *pDocFile);
	bool ReadBlock_DSamples(CDocumentFile *pDocFile);

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
};


