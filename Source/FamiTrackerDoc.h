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

// Get access to some APU constants
#include "apu/apu.h"

#ifdef _DEBUG
	#define ASSERT_FILE_DATA(Statement) ASSERT(Statement)
#else
	#define ASSERT_FILE_DATA(Statement) if (!(Statement)) return true
#endif

#define MIDI_NOTE(octave, note)		((octave + 2) * 12 + (note) - 1)
#define GET_OCTAVE(midi_note)		((midi_note) / 12)
#define GET_NOTE(midi_note)			((midi_note) % 12 + 1)

/*
 * Here are the constants that defines the limits in the tracker
 * change if needed (some might cause side effects)
 *
 */

// Maximum number of instruments to use
const int MAX_INSTRUMENTS			= 64;

// Maximum number of sequence lists
const int MAX_SEQUENCES				= 128;

// Maximum number of items for each sequence
const int MAX_SEQ_ITEMS				= 128;		

const int MAX_PATTERN				= 128;		// Maximum number of patterns per channel
const int MAX_FRAMES				= 128;		// Maximum number of frames
const int MAX_PATTERN_LENGTH		= 256;		// Maximum length of patterns (in rows)

const int MAX_DSAMPLES				= 64;		// Maximum number of dmc samples (this would be more limited by NES memory)

const int MAX_EFFECT_COLUMNS		= 4;		// Number of effect columns allowed

const int MAX_TEMPO					= 255;		// Max tempo
const int MIN_TEMPO					= 1;		// Min tempo

//const int MAX_CHANNELS				= 5 + 8;	// Number of avaliable channels (max)
const int MAX_CHANNELS				= 5 + 3 + 2 + 6 + 1 + 8 + 3;	// Number of avaliable channels (max) should not be used anymore!
																	// instead, check the channelsavailable variable

const int CHANNELS_DEFAULT			= 5;
const int CHANNELS_VRC6				= 3;
const int CHANNELS_VRC7				= 6;

const int OCTAVE_RANGE				= 8;
const int NOTE_RANGE				= 12;

const unsigned int MAX_TRACKS		= 64;

const unsigned int FRAMERATE_NTSC	= 60;
const unsigned int FRAMERATE_PAL	= 50;

// Instrument types
enum {
	INST_2A03 = 1,
	INST_VRC6,
	INST_VRC7,
	INST_FDS,
	INST_N106,
	INST_5B
};
/*
enum {				// Supported expansion chips
	CHIP_NONE = SNDCHIP_NONE,
	CHIP_VRC6 = SNDCHIP_VRC6,
	CHIP_VRC7 = SNDCHIP_VRC7,
	CHIP_FDS = SNDCHIP_FDS,
	CHIP_MMC5 = SNDCHIP_MMC5,
	CHIP_5B = SNDCHIP_5B
};
*/
// Screen update modes
enum {
	CHANGED_TRACK = 1,		// Changed track
	CHANGED_TRACKCOUNT,		// Update track count
	CHANGED_FRAMES,			// Redraw frame window
	CHANGED_HEADER,			// Redraw header
	CHANGED_ERASE,			// Also make sure background is erased
	CHANGED_CLEAR_SEL,		// Clear selection
	
	UPDATE_ENTIRE,			// Redraw entire document
	UPDATE_FAST,			// Fast document drawing
	UPDATE_EDIT,			// When document changed
	UPDATE_CURSOR,			// When cursor moved

	RELOAD_COLORS,			// Color scheme changed

	UPDATE_INSTRUMENTS,		// Instrument list has changed
};

// Shared with VRC6

enum {
	SEQ_VOLUME,
	SEQ_ARPEGGIO,
	SEQ_PITCH,
	SEQ_HIPITCH,		// todo: remove this eventually
	SEQ_DUTYCYCLE,

	SEQ_COUNT
};

// Channel effects
enum eEffects {
	EF_NONE = 0,
	EF_SPEED,
	EF_JUMP,
	EF_SKIP,
	EF_HALT,
	EF_VOLUME,
	EF_PORTAMENTO,
	EF_PORTAOFF,		// unused!!
	EF_SWEEPUP,
	EF_SWEEPDOWN,
	EF_ARPEGGIO,
	EF_VIBRATO,
	EF_TREMOLO,
	EF_PITCH,
	EF_DELAY,
	EF_DAC,
	EF_PORTA_UP,
	EF_PORTA_DOWN,
	EF_DUTY_CYCLE,
	EF_SAMPLE_OFFSET,
	EF_SLIDE_UP,
	EF_SLIDE_DOWN,
	EF_VOLUME_SLIDE,
	EF_NOTE_CUT,
	EF_RETRIGGER,

//	EF_DELAYED_VOLUME,

	EF_COUNT
};

const int EF_DPCM_PITCH = EF_SWEEPUP;
//const int EF_RETRIGGER = EF_SWEEPDOWN;

// Channel effect letters
const char EFF_CHAR[] = {'F',	// Speed
						 'B',	// Jump 
						 'D',	// Skip 
						 'C',	// Halt
						 'E',	// Volume
						 '3',	// Porta on
						 ' ',	// Porta off		// unused
						 'H',	// Sweep up
						 'I',	// Sweep down
						 '0',	// Arpeggio
						 '4',	// Vibrato
						 '7',	// Tremolo
						 'P',	// Pitch
						 'G',	// Note delay
						 'Z',	// DAC setting
						 '1',	// Portamento up
						 '2',	// Portamento down
						 'V',	// Duty cycle
						 'Y',	// Sample offset
						 'Q',	// Slide up
						 'R',	// Slide down
						 'A',	// Volume slide
						 'S',	// Note cut
						 'X',	// DPCM retrigger
		//				 'J',	// Delayed volume
};

enum eNotes {
	NONE = 0,
	C, Cs, D, Ds, E, F, Fs, G, Gs, A, As, B,
	RELEASE,	// Release, note release
	HALT,		// Halt, note cut (a ***)
};

enum eMachine {
	NTSC,
	PAL
};

enum {
	VIBRATO_OLD = 0,
	VIBRATO_NEW,
};

// Old sequence list, kept for compability
struct stSequence {
	unsigned int Count;
	signed char Length[MAX_SEQ_ITEMS];
	signed char Value[MAX_SEQ_ITEMS];
};

// DPCM sample
class CDSample {
public:
	// Empty constructor
	CDSample() : SampleSize(0), SampleData(NULL) {};
	// Copy constructor
	CDSample(CDSample &sample) 
		: SampleSize(sample.SampleSize) 
	{
//		SampleSize = sample.SampleSize;
		SampleData = new char[SampleSize];
		memcpy(SampleData, sample.SampleData, SampleSize);
		memcpy(Name, sample.Name, 256);
	};
	unsigned int SampleSize;
	char		*SampleData;
	char		Name[256];
};

#include "Sequence.h"
#include "PatternData.h"
#include "Instrument.h"
//#include "SampleBank.h"

class CInstrument;
class CTrackerChannel;
class CMainFrame;
class CDocumentFile;

// Use this when it's done
/*
class CPatternCell {
public:
	unsigned char	GetNote() { return m_cNote; };
	unsigned char	GetOctave() { return m_cOctave; };
	unsigned char	GetVolume() { return m_cVolume; };
	unsigned char	GetInstrument() { return m_cInstrument; };
	unsigned char	GetEffect(unsigned char Column) { return m_cEffNumber[MAX_EFFECT_COLUMNS]; };
	unsigned char	GetEffectParam(unsigned char Column) { return m_cEffParam[MAX_EFFECT_COLUMNS]; };
public:
	unsigned char	m_cNote;
	unsigned char	m_cOctave;
	unsigned char	m_cVolume;
	unsigned char	m_cInstrument;
	unsigned char	m_cEffNumber[MAX_EFFECT_COLUMNS];
	unsigned char	m_cEffParam[MAX_EFFECT_COLUMNS];

};
*/

//
// Pure virtual interface to famitracker document, to make it simple for custom exporter DLLs
// to interact with the document. Also, all the getters are const so that custom exporters
// cannot screw anything up =)
//

class CFamiTrackerDocInterface
{
public:
	virtual void			  GetNoteData(unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *Data) const = 0;
	virtual unsigned int	  GetFrameCount()			const = 0;
	virtual unsigned int	  GetPatternLength()		const = 0;
	virtual unsigned int	  GetSongSpeed()			const = 0;
	virtual CSequence const	  *GetSequence(int Index, int Type) const = 0;
	virtual int				  GetSequenceCount(int Index, int Type) const = 0;
	virtual int               GetInstrumentCount() const = 0;
	virtual CInstrument const *GetInstrument(int Instrument) const = 0;
	virtual unsigned int	GetNoteEffectType(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const = 0;
	virtual unsigned int	GetNoteEffectParam(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const = 0;
};

//
// I'll try to organize this class, things are quite messy right now!
//

class CFamiTrackerDoc : public CDocument, public CFamiTrackerDocInterface
{
protected: // create from serialization only
	CFamiTrackerDoc();
	DECLARE_DYNCREATE(CFamiTrackerDoc)

	int m_iVersion;
public:
	static const int CLASS_VERSION = 1;
	const int GetVersion() const { return m_iVersion; };

// Attributes
public:

//
// Public functions
//
public:
	// General functions

	// Sequences
	CSequence		*GetSequence(int Index, int Type);
	// Read only getter for exporter plugins
	CSequence const *GetSequence(int Index, int Type) const;
	int				GetSequenceCount(int Index, int Type) const;
	int				GetFreeSequence(int Type) const;

	CSequence		*GetSequenceVRC6(int Index, int Type);
	int				GetSequenceCountVRC6(int Index, int Type) const;
	int				GetFreeSequenceVRC6(int Type) const;

	void			ConvertSequence(stSequence *OldSequence, CSequence *NewSequence, int Type);

	// Instruments
	CInstrument		  *GetInstrument(int Instrument);
	// Read only getter for exporter plugins
	CInstrument const *GetInstrument(int Instrument) const;
	int				  GetInstrumentCount() const;
	bool			  IsInstrumentUsed(int Instrument) const;

	// Instrument management
	unsigned int	AddInstrument(const char *Name, int Type);				// Add a new instrument
	void			RemoveInstrument(unsigned int Index);					// Remove an instrument
	void			SetInstrumentName(unsigned int Index, char *Name);		// Set the name of an instrument
	void			GetInstrumentName(unsigned int Index, char *Name);		// Get the name of an instrument
	void			CloneInstrument(unsigned int Index);					// Create a copy of an instrument
	CInstrument		*CreateInstrument(int InstType);						// Creates a new instrument of InstType
	int				FindFreeInstrumentSlot();

	// DPCM samples
	int				GetSampleSize(unsigned int Sample);
	char			GetSampleData(unsigned int Sample, unsigned int Offset);

	// Document
	unsigned int	GetPatternLength()		const { return m_pSelectedTune->m_iPatternLength; };
	unsigned int	GetFrameCount()			const { return m_pSelectedTune->m_iFrameCount; };
	unsigned int	GetSongSpeed()			const { return m_pSelectedTune->m_iSongSpeed; };
	unsigned int	GetSongTempo()			const { return m_pSelectedTune->m_iSongTempo; };
	unsigned int	GetAvailableChannels()	const { return m_iChannelsAvailable; };

	void			SetFrameCount(unsigned int Count);
	void			SetPatternLength(unsigned int Length);
	void			SetSongSpeed(unsigned int Speed);					// Sets the speed of song
	void			SetSongTempo(unsigned int Tempo);

	void			ClearPatterns();

	// Track functions
//	void			SetTracks(unsigned int Tracks);
	void			SelectTrack(unsigned int Track);
	void			SelectTrackFast(unsigned int Track);
	void			AllocateSong(unsigned int Song);
	unsigned int	GetTrackCount();
	unsigned int	GetSelectedTrack();
	char			*GetTrackTitle(unsigned int Track) const;
	bool			AddTrack();
	void			RemoveTrack(unsigned int Track);
	void			SetTrackTitle(unsigned int Track, CString Title);
	void			MoveTrackUp(unsigned int Track);
	void			MoveTrackDown(unsigned int Track);

	void			SelectExpansionChip(unsigned char Chip);
	unsigned char	GetExpansionChip() const { return m_cExpansionChip; };

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

	int				GetFirstFreePattern(int Channel);

	int				GetChannelType(int Channel) const;
	int				GetChipType(int Channel) const;

	bool			ExpansionEnabled(int Chip) const;

	// Module properties
	int				GetVibratoStyle() const;
	void			SetVibratoStyle(int Style);

	// General
	bool			IsFileLoaded() const { return m_bFileLoaded; };
	unsigned int	GetFrameRate(void) const;

	// Pattern editing
	void			IncreasePattern(unsigned int PatternPos, unsigned int Channel, int Count);
	void			DecreasePattern(unsigned int PatternPos, unsigned int Channel, int Count);
	void			IncreaseInstrument(unsigned int Frame, unsigned int Channel, unsigned int Row);
	void			DecreaseInstrument(unsigned int Frame, unsigned int Channel, unsigned int Row);
	void			IncreaseVolume(unsigned int Frame, unsigned int Channel, unsigned int Row);
	void			DecreaseVolume(unsigned int Frame, unsigned int Channel, unsigned int Row);
	void			IncreaseEffect(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Index);
	void			DecreaseEffect(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Index);

	void			SetNoteData(unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *Data);
	void			GetNoteData(unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *Data) const;

	void			SetDataAtPattern(unsigned int Track, unsigned int Pattern, unsigned int Channel, unsigned int Row, stChanNote *Data);
	void			GetDataAtPattern(unsigned int Track, unsigned int Pattern, unsigned int Channel, unsigned int Row, stChanNote *Data) const;

	unsigned int	GetNoteEffectType(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const;
	unsigned int	GetNoteEffectParam(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const;

	bool			InsertNote(unsigned int Frame, unsigned int Channel, unsigned int Row);
	bool			DeleteNote(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Column);
	bool			RemoveNote(unsigned int Frame, unsigned int Channel, unsigned int Row);

	bool			IsTrackAdded(unsigned int Track) { return (m_pTunes != NULL); };


	// Instruments
	void			SaveInstrument(unsigned int Instrument, CString FileName);
	unsigned int 	LoadInstrument(CString FileName);

	void			ResetChannels();
	void			RegisterChannel(CTrackerChannel *pChannel, int ChannelType, int ChipType);
	CTrackerChannel	*GetChannel(int Index) const;

	int				GetSampleCount() const;

//
// Private functions
//
private:
	void			SwitchToTrack(unsigned int Track);

protected:

	// Channels
	CTrackerChannel	*m_pChannels[CHANNELS];
	int				m_iRegisteredChannels;
	int				m_iChannelTypes[CHANNELS];
	int				m_iChannelChip[CHANNELS];

	//
	// File management functions (load/save)
	//

	BOOL			SaveDocument(LPCSTR lpszPathName);
	BOOL			OpenDocument(LPCTSTR lpszPathName);

	BOOL			LoadOldFile(CFile *pOpenFile);
	BOOL			OpenDocumentNew(LPCTSTR lpszPathName);

	void			WriteBlock_Header(CDocumentFile *pDocFile);
	void			WriteBlock_Instruments(CDocumentFile *pDocFile);
	void			WriteBlock_Sequences(CDocumentFile *pDocFile);
	void			WriteBlock_Frames(CDocumentFile *pDocFile);
	void			WriteBlock_Patterns(CDocumentFile *pDocFile);
	void			WriteBlock_DSamples(CDocumentFile *pDocFile);
	void			WriteBlock_SequencesVRC6(CDocumentFile *pDocFile);

	bool			ReadBlock_Header(CDocumentFile *pDocFile);
	bool			ReadBlock_Instruments(CDocumentFile *pDocFile);
	bool			ReadBlock_Sequences(CDocumentFile *pDocFile);
	bool			ReadBlock_Frames(CDocumentFile *pDocFile);
	bool			ReadBlock_Patterns(CDocumentFile *pDocFile);
	bool			ReadBlock_DSamples(CDocumentFile *pDocFile);
	bool			ReadBlock_SequencesVRC6(CDocumentFile *pDocFile);

	void			ReorderSequences();
	void			ConvertSequences();


private:

	//
	// Document data
	//

	// Patterns and song data
	CPatternData	*m_pSelectedTune;
	CPatternData	*m_pTunes[MAX_TRACKS];

	unsigned int	m_iTracks, m_iTrack;						// Tracks and selected track

	CString			m_sTrackNames[MAX_TRACKS];

	// Instruments and sequences
	CInstrument		*m_pInstruments[MAX_INSTRUMENTS];
	stSequence		m_Sequences[MAX_SEQUENCES][SEQ_COUNT];		// Allocate one sequence-list for each effect
	CDSample		m_DSamples[MAX_DSAMPLES];					// The DPCM sample

	CSequence		m_Sequences2A03[MAX_SEQUENCES][SEQ_COUNT];
	CSequence		m_SequencesVRC6[MAX_SEQUENCES][SEQ_COUNT];

//	CSampleBank		m_SampleBank;

	// Module properties
	unsigned char	m_cExpansionChip;							// Expansion chip
	int				m_iVibratoStyle;							// 0 = old style, 1 = new style

	unsigned int	m_iChannelsAvailable;						// Number of channels used


	// NSF info
	char			m_strName[32];								// Song name
	char			m_strArtist[32];							// Song artist
	char			m_strCopyright[32];							// Song copyright

	unsigned int	m_iMachine;									// NTSC / PAL
	unsigned int	m_iEngineSpeed;								// Refresh rate

//	bool			m_bExpandDPCMArea;							// DPCM bank switching

	// Remove this eventually
	stSequence		Sequences[MAX_SEQUENCES];

	//
	// End of document data
	//

	bool			m_bFileLoaded;								// Is a file loaded?
	unsigned int	m_iFileVersion;

public:

	// these are still here...
	CDSample		*GetFreeDSample();
	CDSample		*GetDSample(unsigned int Index);
	void			RemoveDSample(unsigned int Index);
	void			GetSampleName(unsigned int Index, char *Name);

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
	virtual void DeleteContents();
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileSave();
	afx_msg void OnEditRemoveUnusedInstruments();
	afx_msg void OnEditRemoveUnusedPatterns();
};


