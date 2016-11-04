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

#include <memory>
#include "APU/apu.h"
#include "APU/square.h"

// This is also shared with MMC5

const uint8	CSquare::DUTY_PULSE[] = {0x0E, 0x0C, 0x08, 0x04};

CSquare::CSquare(CMixer *pMixer, int ID)
{
//	memset(this, 0, sizeof(CSquare));

	m_pMixer = pMixer;
	m_iChanId = ID;
	m_iChip = SNDCHIP_NONE;
}

CSquare::~CSquare()
{
}

void CSquare::Reset()
{
	m_iEnabled = m_iControlReg = 0;
	m_iCounter = m_iEnvelopeCounter = 0;

	SweepCounter = 1;
	SweepPeriod = 1;

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
		m_iDutyLength = DUTY_PULSE[(Value & 0xC0) >> 6];
		m_iFixedVolume = (Value & 0x0F);
		m_iLooping = (Value & 0x20);
		m_iEnvelopeFix = (Value & 0x10);
		m_iEnvelopeSpeed = (Value & 0x0F) + 1;
		if (m_iDutyLength == 4) {
			m_iDutyLength = 0x0C;
			m_bInvert = true;
		}
		else
			m_bInvert = false;
		break;
	case 0x01:
		SweepEnabled = (Value & 0x80);
		SweepPeriod	 = ((Value >> 4) & 0x07) + 1;
		SweepMode	 = (Value & 0x08);		
		SweepShift	 = (Value & 0x07);
		SweepWritten = true;
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

	bool Valid = (m_iFrequency > 7) && (m_iEnabled != 0) && (m_iLengthCounter > 0) && SweepResult < 0x800;

	while (Time >= m_iCounter) {
		Time			-= m_iCounter;
		m_iFrameCycles	+= m_iCounter;
		m_iCounter		= m_iFrequency + 1;
		m_iDutyCycle	= (m_iDutyCycle + 1) & 0x0F;
		if (m_bInvert)
			Mix((((m_iDutyCycle >= m_iDutyLength) && Valid) ? 0 : (m_iEnvelopeFix ? m_iFixedVolume : m_iEnvelopeVolume)));
		else
			Mix((((m_iDutyCycle >= m_iDutyLength) && Valid) ? (m_iEnvelopeFix ? m_iFixedVolume : m_iEnvelopeVolume) : 0));
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
	SweepResult = (m_iFrequency >> SweepShift);

	if (SweepMode)
		SweepResult = m_iFrequency - SweepResult - (First ? 0 : 1);
	else
		SweepResult = m_iFrequency + SweepResult;

	if (--SweepCounter == 0) {
		SweepCounter = SweepPeriod;
		if (SweepEnabled && (m_iFrequency > 0x07) && (SweepResult < 0x800) && (SweepShift > 0))
			m_iFrequency = SweepResult;
	}

	if (SweepWritten) {
		SweepWritten = false;
		SweepCounter = SweepPeriod;
	}
}
/*
void CSquare::SweepUpdate2()
{
	SweepResult = (m_iFrequency >> SweepShift);

	if (SweepMode)
		SweepResult = m_iFrequency - SweepResult + 1;
	else
		SweepResult = m_iFrequency + SweepResult;

	if (SweepEnabled && m_iFrequency > 0x07 && SweepResult < 0x800 && SweepShift > 0) {
		if (SweepCounter <= 1) {
			SweepCounter	= SweepRefresh + 1;
			m_iFrequency	= SweepResult;
		}
		SweepCounter--;
	}

	if (SweepWritten) {
		SweepWritten	= false;
		SweepEnabled	= (SweepRegister & 0x80);	
		SweepRefresh	= (SweepRegister & 0x70) >> 4;
		SweepMode		= (SweepRegister & 0x08);		
		SweepShift		= (SweepRegister & 0x07);	
		SweepCounter	= SweepRefresh + 1;
	}
}
*/
void CSquare::EnvelopeUpdate()
{
	if (--m_iEnvelopeCounter < 1) {
		m_iEnvelopeCounter += m_iEnvelopeSpeed;
		if (!m_iEnvelopeFix) {
			if (m_iLooping)
				m_iEnvelopeVolume = (m_iEnvelopeVolume - 1) & 0x0F;
			else if (m_iEnvelopeVolume > 0)
				m_iEnvelopeVolume--;
		}
	}
}

