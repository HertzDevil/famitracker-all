/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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

//
// Derived channels, 2A03
//

class CChannelHandler2A03 : public CChannelHandler {
public:
	CChannelHandler2A03();
	virtual void ProcessChannel();
	virtual void ResetChannel();

protected:
	virtual void HandleNoteData(stChanNote *pNoteData, int EffColumns);
	virtual void HandleCustomEffects(int EffNum, int EffParam);
	virtual bool HandleInstrument(int Instrument, bool Trigger, bool NewInstrument);
	virtual void HandleEmptyNote();
	virtual void HandleCut();
	virtual void HandleRelease();
	virtual void HandleNote(int Note, int Octave);

protected:
	unsigned char m_cSweep;			// Sweep, used by pulse channels

	bool	m_bManualVolume;		// Flag for Exx
	int		m_iInitVolume;			// Initial volume
	bool	m_bSweeping;			// Flag for HW sweep
	int		m_iSweep;
	int		m_iPostEffect;
	int		m_iPostEffectParam;
};

// Square 1
class CSquare1Chan : public CChannelHandler2A03 {
public:
	CSquare1Chan() : CChannelHandler2A03() { m_iDefaultDuty = 0; };
	virtual void RefreshChannel();
protected:
	virtual void ClearRegisters();
};

// Square 2
class CSquare2Chan : public CChannelHandler2A03 {
public:
	CSquare2Chan() : CChannelHandler2A03() { m_iDefaultDuty = 0; };
	virtual void RefreshChannel();
protected:
	virtual void ClearRegisters();
};

// Triangle
class CTriangleChan : public CChannelHandler2A03 {
public:
	CTriangleChan() : CChannelHandler2A03() {};
	virtual void RefreshChannel();
protected:
	virtual void ClearRegisters();
};

// Noise
class CNoiseChan : public CChannelHandler2A03 {
public:
	CNoiseChan();
	virtual void RefreshChannel();
protected:
	virtual void ClearRegisters();
	virtual void HandleNote(int Note, int Octave);
	virtual void SetupSlide(int Type, int EffParam);

	int TriggerNote(int Note);
};

// DPCM
class CDPCMChan : public CChannelHandler2A03 {
public:
	CDPCMChan(CSampleMem *pSampleMem);
	virtual void RefreshChannel();
protected:
	virtual void HandleNoteData(stChanNote *pNoteData, int EffColumns);
	virtual void HandleCustomEffects(int EffNum, int EffParam);
	virtual bool HandleInstrument(int Instrument, bool Trigger, bool NewInstrument);
	virtual void HandleEmptyNote();
	virtual void HandleCut();
	virtual void HandleRelease();
	virtual void HandleNote(int Note, int Octave);

	virtual void ClearRegisters();
private:
	// DPCM variables
	CSampleMem *m_pSampleMem;

	unsigned char m_cDAC;
	unsigned char m_iLoop;
	unsigned char m_iOffset;
	unsigned char m_iSampleLength;
	unsigned char m_iLoopOffset;
	unsigned char m_iLoopLength;
	int m_iRetrigger;
	int m_iRetriggerCntr;
	int m_iCustomPitch;
	bool m_bTrigger;
	bool m_bEnabled;
};
