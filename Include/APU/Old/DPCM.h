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

/*
 * Delta modulation generation
 *
 */

#ifndef _DPCM_H_
#define _DPCM_H_

#include "channel.h"

class CDPCM : public CChannel
{
public:
	CDPCM(CMixer *pMixer, int ID);
	~CDPCM();

	void	Reset();
	void	Init(CSampleMem *pSampleMem);
	void	SetSpeed(int Speed);

	void	Write(uint16 Address, uint8 Value);
	void	WriteControl(uint8 Value);
	uint8	ReadControl();
	uint8	DidIRQ();
	
	void	Process(int Time);
	void	Reload();

	bool	IRQ() { bool ret = (TriggeredIRQ != 0); TriggeredIRQ = 0; return ret; };

private:
	uint8	ControlReg;

	uint8	SampleBuf;
	bool	SampleFilled;
	uint8	SilenceFlag;

	uint8	Enabled, TriggeredIRQ, PlayMode, Divider;
	int8	PCM;
	int32	OutValue, LastOutValue, Counter;
	uint32	Frequency;
	uint8	ShiftReg, DeltaCounter, DAC_LSB;
	uint16	DMA_LoadReg, DMA_LoadRegCnt;
	uint16	DMA_Length, DMA_LengthCounter;

	uint16	DMC_Freq[16];

	CSampleMem *SampleMem;

};

#endif /* _DPCM_H_ */