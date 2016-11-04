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

const int MAX_INSTRUMENTS		= 64;		// Maximum number of instruments to use
const int MAX_SEQUENCES			= 128;		// Maximum number of sequence lists
const int MAX_SEQ_ITEMS			= 64;		// Maximum number of items in a sequence
const int MAX_PATTERN			= 64;		// Maximum number of patterns per channel
const int MAX_FRAMES			= 64;		// Maximum number of frames
const int MAX_PATTERN_LENGTH	= 256;		// Maximum length of patterns (in rows)
const int MAX_DSAMPLES			= 32;		// Maximum number of dmc samples (this would be limited by NES memory)

const int MAX_EFFECT_COLUMNS	= 4;

const int MAX_TEMPO = 255;
const int MIN_TEMPO = 1;

const int MAX_CHANNELS = 5;

const int CHANNELS_DEFAULT	= 5;
const int CHANNELS_VRC6		= 3;
const int CHANNELS_FDS		= 1;
const int CHANNELS_N106		= 8;

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
	EF_SPEED,
	EF_JUMP,
	EF_SKIP,
	EF_HALT,
	EF_VOLUME,
	EF_PORTAON,
	EF_PORTAOFF,
	EF_SWEEPUP,
	EF_SWEEPDOWN,
	EF_ARPEGGIO,
	EF_VIBRATO,
	EF_TREMOLO,
};

const char EFF_CHAR[] = {'F',	// Speed
						 'B',	// Jump 
						 'D',	// Skip 
						 'C',	// Halt
						 'E',	// Volume
						 '1',	// Porta on
						 '2',	// Porta off
						 'H',	// Sweep up
						 'I',	// Sweep down
						 '0',	// Arpeggio
						 '4',	// Vibrato
						 '7'};	// Tremolo

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
	unsigned int Count;
	signed char Length[MAX_SEQ_ITEMS];
	signed char Value[MAX_SEQ_ITEMS];
};

struct stDSample {
	unsigned int SampleSize;
	char		*SampleData;
	char		Name[256];
};

struct stChanNote {
	int		Note;
	int		Octave;
	int		Vol;
	int		Instrument;
	int		EffNumber[MAX_EFFECT_COLUMNS];
	int		EffParam[MAX_EFFECT_COLUMNS];
};

class CMainFrame;
class CDocumentFile;

class CFamiTrackerDoc : public CDocument
{
protected: // create from serialization only
	CFamiTrackerDoc();
	DECLARE_DYNCREATE(CFamiTrackerDoc)

// Attributes
public:
	// General functions

	// Instruments
	int				AddInstrument(const char *Name);			// Add a new instrument
	void			RemoveInstrument(int Index);				// Remove an instrument
	void			InstrumentName(int Index, char *Name);		// Set the name of an instrument
	void			GetInstName(int Instrument, char *Name);	// Get the name of an instrument

	// Sequence list
	void			InsertModifierItem(unsigned int Index, unsigned int SelectedSequence);
	void			RemoveModifierItem(unsigned int Index, unsigned int SelectedSequence);

	// General document
	void			SetPatternCount(int Count);					// ???
	void			SetRowCount(int Index);						// Amount of rows / pattern
	void			SetSongSpeed(int Speed);					// Sets the speed of song

	void			SetSongInfo(char *Name, char *Artist, char *Copyright);

	bool			IncreaseFrame(int PatternPos, int Channel);
	bool			DecreaseFrame(int PatternPos, int Channel);

	void			IncreaseInstrument(int Frame, int Channel, int Row);
	void			DecreaseInstrument(int Frame, int Channel, int Row);
	void			IncreaseEffect(int Frame, int Channel, int Row, int Index);
	void			DecreaseEffect(int Frame, int Channel, int Row, int Index);
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

	stChanNote		*GetNoteData(int Channel, int RowPos, int PatternPos);
	void			SetNoteData(unsigned int Channel, unsigned int RowPos, unsigned int PatternPos, stChanNote *Note);
	int				GetNoteEffect(int Channel, int RowPos, int Pattern, int Index);
	int				GetNoteEffectValue(int Channel, int RowPos, int Pattern, int Index);
	int				GetNote(int Channel, int RowPos, int Pattern);
	void			GetNoteData(stChanNote *Data, int Channel, int RowPos, int Frame);

	void			GetSampleName(int Index, char *Name);

	// Document data (will be stored/read when opening/saving)

	int				m_iPatternLength;			// Amount of rows in one pattern
	int				m_iFrameCount;				// Number of frames
	int				m_iChannelsAvailable;		// Number of channels used
	int				m_iSongSpeed;				// Song speed

	int				m_iMachine;					// NTSC / PAL
	int				m_iEngineSpeed;				// Refresh rate

	char			m_strName[32];				// Song name
	char			m_strArtist[32];			// Song artist
	char			m_strCopyright[32];			// Song copyright

	// End of document data

	bool			m_bFileLoaded;				// Is file loaded?
	unsigned int	m_iFileVersion;

	// Document data
	stInstrument	Instruments[MAX_INSTRUMENTS];
	stSequence		Sequences[MAX_SEQUENCES];
	stDSample		DSamples[MAX_DSAMPLES];

	stChanNote		ChannelData[MAX_CHANNELS][MAX_PATTERN][MAX_PATTERN_LENGTH];
	unsigned int	FrameList[MAX_FRAMES][MAX_CHANNELS];

	unsigned int	m_iEffectColumns[MAX_CHANNELS];

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

	void WriteBlock_Header(CDocumentFile *pDocFile);
	void WriteBlock_Instruments(CDocumentFile *pDocFile);
	void WriteBlock_Sequences(CDocumentFile *pDocFile);
	void WriteBlock_Frames(CDocumentFile *pDocFile);
	void WriteBlock_Patterns(CDocumentFile *pDocFile);
	void WriteBlock_DSamples(CDocumentFile *pDocFile);

	bool ReadBlock_Header(CDocumentFile *pDocFile);
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
	unsigned int GetFrameRate(void);
};


