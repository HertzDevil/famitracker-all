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

#include "apu/apu.h"

//
// Base class
//
class CChannelHandler
{
public:
	void InitChannel(CAPU *pAPU, unsigned char *pVibTable, CFamiTrackerDoc *pDoc);
	void SetNoteTable(unsigned int *NoteLookupTable);
	void ProcessChannel();
	void KillChannel();
	void MakeSilent();
	
	virtual void PlayNote(stChanNote *NoteData, int EffColumns);
	virtual void RefreshChannel() = 0;

	void Arpeggiate(unsigned int Note);

private:
	void RunSequence(int Index, stSequence *pSequence);
	void LimitFreq();

protected:
	virtual	unsigned int	TriggerNote(int Note);
	virtual void			ClearRegisters() = 0;
	
	int					m_iChannelID;

	CAPU				*m_pAPU;
	CFamiTrackerDoc		*m_pDocument;

	unsigned int		*m_pNoteLookupTable;
	unsigned char		*m_pcVibTable;

	bool				m_bDelayEnabled;
	unsigned char		m_cDelayCounter;
	int					m_iDelayEffColumns;		// exterminate this somehow
	stChanNote			*m_DelayNote;

	bool				m_bEnabled;
	unsigned int		m_iFrequency, m_iLastFrequency;
	unsigned int		m_iFinePitch;

	// Effects
	unsigned char		m_cArpeggio, m_cArpVar;
	unsigned int		m_iVibratoDepth, m_iVibratoSpeed, m_iVibratoPhase;
	unsigned int		m_iTremoloDepth, m_iTremoloSpeed, m_iTremoloPhase;

	unsigned int		m_iOutVol, InitVol;
	unsigned int		Length;

	unsigned char		m_cSweep;
	unsigned char		m_cDutyCycle;

	unsigned int		m_iNote, m_iLastNote;
	unsigned int		m_iOctave;

	unsigned int		m_iPortaTo;
	unsigned int		m_iPortaSpeed;

	unsigned int		m_iVolume;

	int					m_iLastInstrument, m_iInstrument;

	int					ModEnable[MOD_COUNT];
	int					ModIndex[MOD_COUNT];
	int					ModDelay[MOD_COUNT];
	int					ModPointer[MOD_COUNT];
};

//
// Derived channels, 2A03
//

// Square 1
class CSquare1Chan : public CChannelHandler
{
public:
	CSquare1Chan() { m_iChannelID = 0; };
	void RefreshChannel();
protected:
	void ClearRegisters();
};

// Square 2
class CSquare2Chan : public CChannelHandler
{
public:
	CSquare2Chan() { m_iChannelID = 1; };
	void RefreshChannel();
protected:
	void ClearRegisters();
};

// Triangle
class CTriangleChan : public CChannelHandler
{
public:
	CTriangleChan() { m_iChannelID = 2; };
	void RefreshChannel();
protected:
	void ClearRegisters();
};

// Noise
class CNoiseChan : public CChannelHandler
{
public:
	CNoiseChan() { m_iChannelID = 3; };
	void RefreshChannel();
protected:
	void ClearRegisters();
	unsigned int TriggerNote(int Note);
};

// DPCM
class CDPCMChan : public CChannelHandler
{
public:
	CDPCMChan() { m_iChannelID = 4; };
	void RefreshChannel();
	void PlayNote(stChanNote *NoteData, int EffColumns);
	void SetSampleMem(CSampleMem *pSampleMem);
protected:
	void ClearRegisters();
private:
	CSampleMem *m_pSampleMem;
	unsigned char m_cDAC;
	bool m_bKeyRelease;
};
