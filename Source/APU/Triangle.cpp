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

#include "apu.h"
#include "triangle.h"

const uint8 CTriangle::TRIANGLE_WAVE[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 
	15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
};

CTriangle::CTriangle(CMixer *pMixer, int ID) : CChannel(pMixer, ID, SNDCHIP_NONE)
{
}

CTriangle::~CTriangle()
{
}

void CTriangle::Reset()
{
	m_iEnabled = m_iControlReg = 0;
	m_iCounter = m_iLengthCounter = 0;

	Write(0, 0);
	Write(1, 0);
	Write(2, 0);
	Write(3, 0);

	EndFrame();

	Mix(0);
}

void CTriangle::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
		case 0x00:
			m_iLinearLoad = (Value & 0x7F);
			m_iLoop = (Value & 0x80);
			break;
		case 0x01:
			break;
		case 0x02:
			m_iFrequency = Value | (m_iFrequency & 0x0700);
			break;
		case 0x03:
			m_iFrequency = ((Value & 0x07) << 8) | (m_iFrequency & 0xFF);
			m_iLengthCounter = CAPU::LENGTH_TABLE[(Value & 0xF8) >> 3];
			m_iHalt = 1;
			if (m_iControlReg)
				m_iEnabled = 1;
			break;
	}
}

void CTriangle::WriteControl(uint8 Value)
{
	m_iControlReg = Value & 1;
	
	if (m_iControlReg == 0)
		m_iEnabled = 0;
}

uint8 CTriangle::ReadControl()
{
	return ((m_iLengthCounter > 0) && (m_iEnabled == 1));
}

void CTriangle::Process(uint32 Time)
{
	// Triangle skips if a wavelength less than 2 is used
	// It takes to much CPU and it wouldn't be possible to hear anyway
	//

	if (!m_iLinearCounter || !m_iLengthCounter || !m_iEnabled) {
		m_iFrameCycles += Time;
		return;
	}
	else if (m_iFrequency <= 1) {
		// Frequency is too high to be audible
		Mix(7);
		m_iStepGen = 7;
		m_iFrameCycles += Time;
		return;
	}

	while (Time >= m_iCounter) {
		Time		   -= m_iCounter;
		m_iFrameCycles += m_iCounter;
		m_iCounter	   = m_iFrequency + 1;
		Mix(TRIANGLE_WAVE[m_iStepGen]);
		m_iStepGen	   = (m_iStepGen + 1) & 0x1F;
	}
	
	m_iCounter -= Time;
	m_iFrameCycles += Time;
}

void CTriangle::LengthCounterUpdate()
{
	if ((m_iLoop == 0) && (m_iLengthCounter > 0)) 
		m_iLengthCounter--;
}

void CTriangle::LinearCounterUpdate()
{
	/*
		1.  If the halt flag is set, the linear counter is reloaded with the counter reload value, 
			otherwise if the linear counter is non-zero, it is decremented.

		2.  If the control flag is clear, the halt flag is cleared. 
	*/

	if (m_iHalt == 1)
		m_iLinearCounter = m_iLinearLoad;
	else
		if (m_iLinearCounter > 0)
			m_iLinearCounter--;

	if (m_iLoop == 0)
		m_iHalt = 0;
}

