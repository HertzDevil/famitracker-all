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

#ifndef _VRC7_H_
#define _VRC7_H_

#include "external.h"
#include "emu2413.h"

class CVRC7 : public CExternal {
public:
	CVRC7(CMixer *pMixer);
	~CVRC7();
	void Shutdown();
	void Reset();
	void SetSampleSpeed(uint32 SampleRate, double ClockRate);
	void Write(uint16 Address, uint8 Value);
	void EndFrame();
	void Process(uint32 Time);

private:
	OPLL	*OPLLInt;
	uint32	m_iFrameCycles;
	uint32	m_iMaxSamples;

	double	Div;
	int16	*m_pBuffer;
	uint32	m_iBufferPtr;

	uint8	m_iSoundReg;
};


#endif /* _VRC7_H_ */