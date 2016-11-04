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

#include <memory>
#include "apu.h"
#include "square.h"

// This is also shared with MMC5

const uint8 CSquare::DUTY_TABLE[4][16] = {
	{0, 1, 1, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0},
	{0, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0},
	{0, 1, 1, 1,  1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0},
	{1, 0, 0, 0,  0, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1}
};

CSquare::CSquare(CMixer *pMixer, int ID) : CChannel(pMixer, ID, SNDCHIP_NONE)
{
}

CSquare::~CSquare()
{
}

void CSquare::Reset()
{
	m_iEnabled = m_iControlReg = 0;
	m_iCounter = m_iEnvelopeCounter = 0;

	m_iSweepCounter = 1;
	m_iSweepPeriod = 1;

	Write(0, 0);
	Write(1, 0);
	Write(2, 0);
	Write(3, 0);

	SweepUpdate(false);

	EndFrame();
}

void CSquare::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
	case 0x00:
		m_iDutyLength	 = Value >> 6;
		m_iFixedVolume	 = (Value & 0x0F);
		m_iLooping	 	 = (Value & 0x20);
		m_iEnvelopeFix	 = (Value & 0x10);
		m_iEnvelopeSpeed = (Value & 0x0F) + 1;
		break;
	case 0x01:
		m_iSweepEnabled  = (Value & 0x80);
		m_iSweepPeriod	 = ((Value >> 4) & 0x07) + 1;
		m_iSweepMode	 = (Value & 0x08);		
		m_iSweepShift	 = (Value & 0x07);
		m_bSweepWritten	 = true;
		break;
	case 0x02:
		m_iFrequency = Value | (m_iFrequency & 0x0700);
		break;
	case 0x03:
		m_iFrequency = ((Value & 0x07) << 8) | (m_iFrequency & 0xFF);
		m_iLengthCounter = CAPU::LENGTH_TABLE[((Value & 0xF8) >> 3)] + 1;
		m_iDutyCycle = 0;
		m_iEnvelopeVolume = 0x0F;
		if (m_iControlReg)
			m_iEnabled = 1;
		break;
	}
}

void CSquare::WriteControl(uint8 Value)
{
	m_iControlReg = Value & 0x01;

	if (m_iControlReg == 0)
		m_iEnabled = 0;
}

uint8 CSquare::ReadControl()
{
	return ((m_iLengthCounter > 0) && (m_iEnabled == 1));
}

void CSquare::Process(uint32 Time)
{
	if (!m_iFrequency) {
		m_iFrameCycles += Time;
		return;
	}

	bool Valid = (m_iFrequency > 7) && (m_iEnabled != 0) && (m_iLengthCounter > 0) && m_iSweepResult < 0x800;

	while (Time >= m_iCounter) {
		Time			-= m_iCounter;
		m_iFrameCycles	+= m_iCounter;
		m_iCounter		= m_iFrequency + 1;
		uint8 Volume = m_iEnvelopeFix ? m_iFixedVolume : m_iEnvelopeVolume;
		if (Valid && Volume > 0)
			Mix(DUTY_TABLE[m_iDutyLength][m_iDutyCycle] * Volume);
		m_iDutyCycle = (m_iDutyCycle + 1) & 0x0F;
	}

	m_iCounter -= Time;
	m_iFrameCycles += Time;
}

void CSquare::LengthCounterUpdate()
{
	if ((m_iLooping == 0) && (m_iLengthCounter > 0)) m_iLengthCounter--;
}

void CSquare::SweepUpdate(bool First)
{
	m_iSweepResult = (m_iFrequency >> m_iSweepShift);

	if (m_iSweepMode)
		//m_iSweepResult = m_iFrequency - m_iSweepResult - (First ? 0 : 1);
		m_iSweepResult = (m_iFrequency + (m_iSweepResult ^ 0x7FF) + 1) & 0x7FF;

	else
		m_iSweepResult = m_iFrequency + m_iSweepResult;

	if (--m_iSweepCounter == 0) {
		m_iSweepCounter = m_iSweepPeriod;
		if (m_iSweepEnabled && (m_iFrequency > 0x07) && (m_iSweepResult < 0x800) && (m_iSweepShift > 0))
			m_iFrequency = m_iSweepResult;
	}

	if (m_bSweepWritten) {
		m_bSweepWritten = false;
		m_iSweepCounter = m_iSweepPeriod;
	}
}

void CSquare::EnvelopeUpdate()
{
	if (--m_iEnvelopeCounter < 0) {
		m_iEnvelopeCounter = m_iEnvelopeSpeed;
		if (!m_iEnvelopeFix) {
			if (m_iLooping)
				m_iEnvelopeVolume = (m_iEnvelopeVolume - 1) & 0x0F;
			else if (m_iEnvelopeVolume > 0)
				m_iEnvelopeVolume--;
		}
	}
}

