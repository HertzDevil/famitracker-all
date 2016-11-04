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

#include "stdafx.h"
#include "FamiTracker.h"
#include <memory>
#include "common.h"
#include "apu/apu.h"

const int32 AMPLIFY = 2;
const uint32 OPL_CLOCK = 3579545;

void CVRC7::Init(CMixer *pMixer)
{
	m_pMixer = pMixer;
	m_pBuffer = NULL;

	OPLL_init(OPL_CLOCK, 44100);

	OPLLInt = OPLL_new();

	OPLL_reset(OPLLInt);
	OPLL_reset_patch(OPLLInt, 1);
	OPLLInt->masterVolume = 70;
}

void CVRC7::Shutdown()
{
	delete [] m_pBuffer;
}

void CVRC7::Reset()
{
	m_iBufferPtr = 0;
	m_iFrameCycles = 0;
}

void CVRC7::SetSampleSpeed(uint32 SampleRate, double ClockRate)
{
	OPLL_setClock(OPL_CLOCK, SampleRate);

	m_iMaxSamples = SampleRate / 60 + 400;		// remove 60
	
	if (m_pBuffer) 
		delete [] m_pBuffer;

	m_pBuffer = new int16[m_iMaxSamples];
	memset(m_pBuffer, 0, sizeof(int16) * m_iMaxSamples);
}

void CVRC7::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
		case 0x9010:
			m_iSoundReg = Value;
			break;
		case 0x9030:
			OPLL_writeReg(OPLLInt, m_iSoundReg, Value);
			TRACE("VRC7: %02X: %02X\n", m_iSoundReg, Value);
			break;
	}
}

void CVRC7::EndFrame()
{
	uint32 WantSamples = m_pMixer->GetMixSampleCount(m_iFrameCycles);

	while (m_iBufferPtr < WantSamples)
		m_pBuffer[m_iBufferPtr++] = OPLL_calc(OPLLInt) * AMPLIFY;

	m_pMixer->MixSamples((blip_sample_t*)m_pBuffer, WantSamples);

	m_iBufferPtr -= WantSamples;
	m_iFrameCycles = 0;
}

void CVRC7::Process(uint32 Time)
{
	//  this cannot run in sync
	m_iFrameCycles += Time;
}
