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

#pragma once

#include "apu/apu.h"

const int MAX_VOL = 0x7F;
const int VOL_SHIFT = 3;

//
// Base class
//
class CChannelHandler {
public:
	CChannelHandler() { m_bEnabled = false; m_iInstrument = m_iLastInstrument = 0; };

	// Virtual functions
	virtual void PlayNote(stChanNote *NoteData, int EffColumns) = 0; // Plays a note
	virtual void ProcessChannel() = 0;								 // Run the instrument
	virtual void RefreshChannel() = 0;								 // Update channel registers
	virtual void SetNoteTable(unsigned int *NoteLookupTable);

	// Public functions
	void InitChannel(CAPU *pAPU, int *pVibTable, CFamiTrackerDoc *pDoc);
	void KillChannel();
	void MakeSilent();
	void Arpeggiate(unsigned int Note);

protected:
	// Protected virtual functions
	virtual	unsigned int	TriggerNote(int Note);
	virtual void			ClearRegisters() = 0;		// this must be overloaded
	void	RunNote(int Octave, int Note);
	void	SetupSlide(int Type, int EffParam);

	bool CheckNote(stChanNote *pNoteData, int InstrumentType);

	bool CheckCommonEffects(unsigned char EffCmd, unsigned char EffParam);
	bool HandleDelay(stChanNote *NoteData, int EffColumns);

	void LimitFreq();

	int GetVibrato();
	int GetTremolo();

	int	m_iChannelID;		// Channel ID

	CAPU *m_pAPU;
	CFamiTrackerDoc	*m_pDocument;

	unsigned int *m_pNoteLookupTable;
	int *m_pcVibTable;

	// Delay variables
	bool				m_bDelayEnabled;
	unsigned char		m_cDelayCounter;
	unsigned int		m_iDelayEffColumns;		
	stChanNote			*m_DelayNote;

	// General variables
	bool				m_bEnabled;
	unsigned int		m_iFrequency, m_iLastFrequency;
	unsigned int		m_iNote;
	char				m_iVolume, m_iVolSlide;

	// Effects
	unsigned int		m_iVibratoDepth, m_iVibratoSpeed, m_iVibratoPhase;
	unsigned int		m_iTremoloDepth, m_iTremoloSpeed, m_iTremoloPhase;

	unsigned char		m_iEffect;	// arpeggio & portamento
	unsigned char		m_cArpeggio, m_cArpVar;
	unsigned int		m_iPortaTo, m_iPortaSpeed;

	unsigned int		m_iFinePitch;

	// other crap
	unsigned int		m_iOutVol, InitVol;
	unsigned int		Length;

	unsigned int		m_iLastInstrument, m_iInstrument;

	unsigned char		m_iDefaultDuty;
};



//
// Derived channels, 5B
//
#if 0
class CChannelHandler5B : public CChannelHandler {
public:
	void PlayNote(stChanNote *NoteData, int EffColumns);
	void ProcessChannel();
protected:
	void RunSequence(int Index, CSequence *pSequence);

	unsigned char m_cSweep;
	unsigned char m_cDutyCycle, m_iDefaultDuty;

	int ModEnable[MOD_COUNT];
	int	ModIndex[MOD_COUNT];
	int	ModDelay[MOD_COUNT];
	int	ModPointer[MOD_COUNT];
};

// Channel 1
class C5BChannel1 : public CChannelHandler5B {
public:
	CMMC5Square1Chan() { m_iDefaultDuty = 0; m_iChannelID = 5; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
};

// Channel 2
class C5BChannel2 : public CChannelHandler5B {
public:
	CMMC5Square2Chan() { m_iDefaultDuty = 0; m_iChannelID = 6; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
};

// Channel 3
class C5BChannel3 : public CChannelHandler5B {
public:
	CMMC5Square3Chan() { m_iDefaultDuty = 0; m_iChannelID = 7; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
};

#endif