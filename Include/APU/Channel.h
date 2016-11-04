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

#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include "mixer.h"

//
// This class is used to derive the audio channels
//

class CChannel {
public:
	CChannel() {
		m_iFrameCycles = 0;
		m_iLastValue   = 0;
	};

	// Begin a new audio frame
	inline void EndFrame() {
		m_iFrameCycles = 0;
	}

protected:
	inline void Mix(int32 Value) {
		if (m_iLastValue != Value) {
			m_pMixer->AddValue(m_iChanId, m_iChip, Value, m_iFrameCycles);
			m_iLastValue = Value;
		}
	};

	CMixer	*m_pMixer;			// The mixer
	uint32	m_iFrameCycles;		// Cycle counter, resets every new frame
	int32	m_iLastValue;		// Last value sent to mixer
	uint16	m_iChanId;			// This channels unique ID
	uint16	m_iChip;			// Chip

	// Variables used by channels
	uint8	m_iControlReg, m_iEnabled;
	uint16	m_iFrequency, m_iLengthCounter;
	uint32	m_iCounter;
};

class CExChannel {
public:
	CExChannel() {
		m_iFrameCycles = 0;
		m_iLastValue   = 0;
	};

	void Init(CMixer *pMixer, uint8 Chip, uint8 ID) {
		m_pMixer = pMixer;
		m_iChip = Chip;
		m_iChanId = ID;
	}

	inline void EndFrame() {
		m_iFrameCycles = 0;
	}

protected:
	inline void Mix(int32 Value) {
		int32 Delta = Value - m_iLastValue;
		if (Delta)
			m_pMixer->AddValue(m_iChanId, m_iChip, Delta, m_iFrameCycles);
		m_iLastValue = Value;
	}

	CMixer	*m_pMixer;
	uint32	m_iFrameCycles;		// Cycle counter, resets every new frame
	uint8	m_iChip;
	uint8	m_iChanId;
	int32	m_iLastValue;		// Last value sent to mixer
};

#endif /* _CHANNEL_H_ */