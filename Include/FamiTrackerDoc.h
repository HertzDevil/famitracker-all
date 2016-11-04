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

const int MAX_EFFECT_COLUMNS	= 4;		// Number of effect columns allowed

const int MAX_TEMPO				= 255;		// Max tempo
const int MIN_TEMPO				= 1;		// Min tempo

const int MAX_CHANNELS			= 5;		// Number of avaliable channels (the internal ones right now)

const int CHANNELS_DEFAULT		= 5;
const int CHANNELS_VRC6			= 3;

const int OCTAVE_RANGE			= 8;

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
	EF_PITCH
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
						 '7',	// Tremolo
						 'P'};	// Pitch

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
	char	Samples[OCTAVE_RANGE][12];				// Samples
	char	SamplePitch[OCTAVE_RANGE][12];			// Play pitch/loop
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

//
// I'll try to organize this class, things are quite messy right now!
//

class CFamiTrackerDoc : public CDocument
{
protected: // create from serialization only
	CFamiTrackerDoc();
	DECLARE_DYNCREATE(CFamiTrackerDoc)

// Attributes
public:

//
// Public functions
//
public:
	// General functions

	//
	// Abstraction access functions
	//
	
	// Sequences
	stSequence		*GetSequence(int Index, int Type);
	int				GetSequenceCount(int Index, int Type);

	// Instruments
	stInstrument	*GetInstrument(int Instrument);
	bool			IsInstrumentUsed(int Instrument);
	bool			GetInstEffState(int Instrument, int Effect);
	int				GetInstEffIndex(int Instrument, int Effect);
	int				GetInstDPCM(int Instrument, int Note, int Octave);
	int				GetInstDPCMPitch(int Instrument, int Note, int Octave);
	void			SetInstEffect(int Instrument, int Effect, int Sequence, bool Enabled);
	void			SetInstDPCMPitch(int Instrument, int Note, int Octave, int Pitch);
	void			SetInstDPCM(int Instrument, int Note, int Octave, int Sample);

	// Instrument management
	unsigned int	AddInstrument(const char *Name);						// Add a new instrument
	void			RemoveInstrument(unsigned int Index);					// Remove an instrument
	void			SetInstrumentName(unsigned int Index, char *Name);		// Set the name of an instrument
	void			GetInstrumentName(unsigned int Index, char *Name);		// Get the name of an instrument

	// DPCM samples
	int				GetSampleSize(unsigned int Sample);
	char			GetSampleData(unsigned int Sample, unsigned int Offset);

	// Document
	unsigned int	GetPatternLength()		const { return m_iPatternLength; };
	unsigned int	GetFrameCount()			const { return m_iFrameCount; };
	unsigned int	GetSongSpeed()			const { return m_iSongSpeed; };
	unsigned int	GetAvailableChannels()	const { return m_iChannelsAvailable; };

	void			SetFrameCount(unsigned int Count);
	void			SetPatternLength(unsigned int Length);
	void			SetSongSpeed(unsigned int Speed);					// Sets the speed of song

	void			SetSongInfo(char *Name, char *Artist, char *Copyright);
	char			*GetSongName()			 { return m_strName; };
	char			*GetSongArtist()		 { return m_strArtist; };
	char			*GetSongCopyright()		 { return m_strCopyright; };

	unsigned int	GetEngineSpeed()		const { return m_iEngineSpeed; };
	unsigned int	GetMachine()			const { return m_iMachine; };

	void			SetEngineSpeed(unsigned int Speed);
	void			SetMachine(unsigned int Machine);

	unsigned int	GetEffColumns(unsigned int Channel) const; 
	void			SetEffColumns(unsigned int Channel, unsigned int Columns);

	unsigned int	GetPatternAtFrame(unsigned int Frame, unsigned int Channel) const;
	void			SetPatternAtFrame(unsigned int Frame, unsigned int Channel, unsigned int Pattern);

	// General
	bool			IsFileLoaded()			const { return m_bFileLoaded; };
	unsigned int	GetFrameRate(void) const;

	// Pattern editing
	void			IncreasePattern(unsigned int PatternPos, unsigned int Channel);
	void			DecreasePattern(unsigned int PatternPos, unsigned int Channel);
	void			IncreaseInstrument(unsigned int Frame, unsigned int Channel, unsigned int Row);
	void			DecreaseInstrument(unsigned int Frame, unsigned int Channel, unsigned int Row);
	void			IncreaseVolume(unsigned int Frame, unsigned int Channel, unsigned int Row);
	void			DecreaseVolume(unsigned int Frame, unsigned int Channel, unsigned int Row);
	void			IncreaseEffect(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Index);
	void			DecreaseEffect(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Index);

	void			SetNoteData(unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *Data);
	void			GetNoteData(unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *Data) const;

	void			SetDataAtPattern(unsigned int Pattern, unsigned int Channel, unsigned int Row, stChanNote *Data);
	void			GetDataAtPattern(unsigned int Pattern, unsigned int Channel, unsigned int Row, stChanNote *Data) const;

	unsigned int	GetNoteEffectType(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const;
	unsigned int	GetNoteEffectParam(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const;

	bool			InsertNote(unsigned int Frame, unsigned int Channel, unsigned int Row);
	bool			DeleteNote(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Column);
	bool			RemoveNote(unsigned int Frame, unsigned int Channel, unsigned int Row);

protected:

	//
	// File management functions (load/save)
	//

	void			CleanDocument();

	BOOL			SaveDocument(LPCSTR lpszPathName);
	BOOL			OpenDocument(LPCTSTR lpszPathName);

	BOOL			LoadOldFile(CFile OpenFile);
	BOOL			LoadNewFile(CFile OpenFile);

	void			WriteBlock_Header(CDocumentFile *pDocFile);
	void			WriteBlock_Instruments(CDocumentFile *pDocFile);
	void			WriteBlock_Sequences(CDocumentFile *pDocFile);
	void			WriteBlock_Frames(CDocumentFile *pDocFile);
	void			WriteBlock_Patterns(CDocumentFile *pDocFile);
	void			WriteBlock_DSamples(CDocumentFile *pDocFile);

	bool			ReadBlock_Header(CDocumentFile *pDocFile);
	bool			ReadBlock_Instruments(CDocumentFile *pDocFile);
	bool			ReadBlock_Sequences(CDocumentFile *pDocFile);
	bool			ReadBlock_Frames(CDocumentFile *pDocFile);
	bool			ReadBlock_Patterns(CDocumentFile *pDocFile);
	bool			ReadBlock_DSamples(CDocumentFile *pDocFile);

	void			ReorderSequences();

private:

	//
	// Document data
	//
	stChanNote		m_PatternData[MAX_CHANNELS][MAX_PATTERN][MAX_PATTERN_LENGTH];	// The patterns
	stSequence		m_Sequences[MAX_SEQUENCES][MOD_COUNT];		// Allocate one sequence-list for each effect
	stInstrument	m_Instruments[MAX_INSTRUMENTS];				// Instruments
	stDSample		m_DSamples[MAX_DSAMPLES];					// The DPCM sample

	unsigned int	m_iEffectColumns[MAX_CHANNELS];				// Effect columns enabled
	unsigned int	m_iFrameList[MAX_FRAMES][MAX_CHANNELS];		// List of the patterns assigned to frames

	unsigned int	m_iPatternLength;							// Amount of rows in one pattern
	unsigned int	m_iFrameCount;								// Number of frames
	unsigned int	m_iChannelsAvailable;						// Number of channels used
	unsigned int	m_iSongSpeed;								// Song speed

	char			m_strName[32];								// Song name
	char			m_strArtist[32];							// Song artist
	char			m_strCopyright[32];							// Song copyright

	unsigned int	m_iMachine;									// NTSC / PAL
	unsigned int	m_iEngineSpeed;								// Refresh rate

	// Remove this eventually
	stSequence		Sequences[MAX_SEQUENCES];

	//
	// End of document data
	//

	bool			m_bFileLoaded;								// Is file loaded?
	unsigned int	m_iFileVersion;

public:

	// these are still here...
	stDSample		*GetFreeDSample();
	stDSample		*GetDSample(int Index);
	void			RemoveDSample(int Index);
	void			GetSampleName(int Index, char *Name);

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

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
};


