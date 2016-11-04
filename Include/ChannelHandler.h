/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2007  Jonathan Liss
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

//
// Base class
//
class CChannelHandler {
public:
	CChannelHandler() { m_bEnabled = false; };

	// Virtual functions
	virtual void PlayNote(stChanNote *NoteData, int EffColumns) = 0; // Plays a note
	virtual void ProcessChannel() = 0;								 // Run the instrument
	virtual void RefreshChannel() = 0;								 // Update channel registers
	virtual void SetNoteTable(unsigned int *NoteLookupTable);

	// Public functions
	void InitChannel(CAPU *pAPU, unsigned char *pVibTable, CFamiTrackerDoc *pDoc);
	void KillChannel();
	void MakeSilent();
	void Arpeggiate(unsigned int Note);

protected:
	// Protected virtual functions
	virtual	unsigned int	TriggerNote(int Note);
	virtual void			ClearRegisters() = 0;		// this must be overloaded

	bool CheckCommonEffects(unsigned char EffCmd, unsigned char EffParam);
	bool HandleDelay(stChanNote *NoteData, int EffColumns);

	void LimitFreq();

	int	m_iChannelID;		// Channel ID

	CAPU *m_pAPU;
	CFamiTrackerDoc	*m_pDocument;

	unsigned int *m_pNoteLookupTable;
	unsigned char *m_pcVibTable;

	// Delay variables
	bool				m_bDelayEnabled;
	unsigned char		m_cDelayCounter;
	unsigned int		m_iDelayEffColumns;		// exterminate this somehow
	stChanNote			*m_DelayNote;

	// General variables
	bool				m_bEnabled;
	unsigned int		m_iFrequency, m_iLastFrequency;
	unsigned int		m_iNote/*, m_iLastNote*/;
	unsigned int		m_iVolume;

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
};

//
// Derived channels, 2A03
//

class CChannelHandler2A03 : public CChannelHandler {
public:
	void PlayNote(stChanNote *NoteData, int EffColumns);
	void ProcessChannel();
	//void SetNoteTable(unsigned int *NoteLookupTable);
protected:
	void RunSequence(int Index, CSequence *pSequence);
//	unsigned int TriggerNote(int Note);

	unsigned char m_cSweep;
	unsigned char m_cDutyCycle, m_iDefaultDuty;

	int ModEnable[MOD_COUNT];
	int	ModIndex[MOD_COUNT];
	int	ModDelay[MOD_COUNT];
	int	ModPointer[MOD_COUNT];
};

// Square 1
class CSquare1Chan : public CChannelHandler2A03 {
public:
	CSquare1Chan() { m_iDefaultDuty = 0; m_iChannelID = 0; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
};

// Square 2
class CSquare2Chan : public CChannelHandler2A03 {
public:
	CSquare2Chan() { m_iDefaultDuty = 0; m_iChannelID = 1; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
};

// Triangle
class CTriangleChan : public CChannelHandler2A03 {
public:
	CTriangleChan() { m_iChannelID = 2; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
};

// Noise
class CNoiseChan : public CChannelHandler2A03 {
public:
	CNoiseChan() { m_iDefaultDuty = 0; m_iChannelID = 3; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
	unsigned int TriggerNote(int Note);
};

// DPCM
class CDPCMChan : public CChannelHandler2A03 {
public:
	CDPCMChan() { m_iChannelID = 4; m_bEnabled = false; };
	void RefreshChannel();
	void PlayNote(stChanNote *NoteData, int EffColumns);
	void SetSampleMem(CSampleMem *pSampleMem);
protected:
	void ClearRegisters();
private:
	CSampleMem *m_pSampleMem;
	unsigned char m_cDAC;
	unsigned char m_iLoop;
	bool m_bKeyRelease;
};

//
// Derived channels, VRC6
//

class CChannelHandlerVRC6 : public CChannelHandler {
public:
	void ProcessChannel();
	void PlayNote(stChanNote *NoteData, int EffColumns);
protected:
	void RunSequence(int Index, CSequence *pSequence);

	unsigned char m_cDutyCycle;

	int ModEnable[MOD_COUNT];
	int	ModIndex[MOD_COUNT];
	int	ModDelay[MOD_COUNT];
	int	ModPointer[MOD_COUNT];
};

class CVRC6Square1 : public CChannelHandlerVRC6 {
public:
	CVRC6Square1() { m_iChannelID = 5; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
private:
};

class CVRC6Square2 : public CChannelHandlerVRC6 {
public:
	CVRC6Square2() { m_iChannelID = 6; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
private:
};

class CVRC6Sawtooth : public CChannelHandlerVRC6 {
public:
	CVRC6Sawtooth() { m_iChannelID = 7; m_bEnabled = false; };
	void RefreshChannel();
protected:
	void ClearRegisters();
private:
};