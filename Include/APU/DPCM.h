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


/*
 * Delta modulation generation
 *
 */

#ifndef _DPCM_H_
#define _DPCM_H_

#include "channel.h"

class CEmulator;

class CDPCM : public CChannel {
public:
	CDPCM(CMixer *pMixer, CSampleMem *pSampleMem, int ID);
	~CDPCM();

	void	Reset();
	void	SetSpeed(int Speed);
	void	Write(uint16 Address, uint8 Value);
	void	WriteControl(uint8 Value);
	uint8	ReadControl();
	uint8	DidIRQ();
	void	Process(uint32 Time);
	void	Reload();

	uint8	GetSamplePos() { return  (m_iDMA_Address - (m_iDMA_LoadReg << 6 | 0x4000)) >> 6; };
	uint8	GetDeltaCounter() { return m_iDeltaCounter; };

private:
	static const uint16	DMC_FREQ_NTSC[];
	static const uint16	DMC_FREQ_PAL[];

	CEmulator *m_pEmulator;

	uint8	m_iBitDivider, m_iShiftReg;
	uint8	m_iPlayMode;
	uint8	m_iDeltaCounter;
	uint8	m_iSampleBuffer;

	uint16	m_iDMA_LoadReg, m_iDMA_Address;
	uint16	m_iDMA_Length, m_iDMA_LengthCounter;

	bool	m_bTriggeredIRQ, m_bSampleFilled, m_bSilenceFlag;

	uint16	m_iDMC_FreqTable[16];

	CSampleMem	*m_pSampleMem;
};

#endif /* _DPCM_H_ */