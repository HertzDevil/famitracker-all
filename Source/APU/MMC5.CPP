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
#include "MMC5.h"
#include "square.h"

// MMC5 external sound

CMMC5::CMMC5(CMixer *pMixer) : 
	m_pEXRAM(new uint8[0x400]),
	m_pSquare1(new CMMCSquare(pMixer, CHANID_MMC5_SQUARE1)),
	m_pSquare2(new CMMCSquare(pMixer, CHANID_MMC5_SQUARE2))
{
}

CMMC5::~CMMC5()
{
	if (m_pSquare1)
		delete m_pSquare1;

	if (m_pSquare2)
		delete m_pSquare2;

	if (m_pEXRAM)
		delete [] m_pEXRAM;
}

void CMMC5::Reset()
{
	m_pSquare1->Reset();
	m_pSquare2->Reset();

	m_pSquare1->Write(0x01, 0x08);
	m_pSquare2->Write(0x01, 0x08);
}

void CMMC5::Write(uint16 Address, uint8 Value)
{
	if (Address >= 0x5C00 && Address <= 0x5FF5) {
		m_pEXRAM[Address & 0x3FF] = Value;
		return;
	}

	switch (Address) {
		// Channel 1
		case 0x5000:
			m_pSquare1->Write(0, Value);
			break;
		case 0x5002:
			m_pSquare1->Write(2, Value);
			break;
		case 0x5003:
			m_pSquare1->Write(3, Value);
			break;
		// Channel 2
		case 0x5004:
			m_pSquare2->Write(0, Value);
			break;
		case 0x5006:
			m_pSquare2->Write(2, Value);
			break;
		case 0x5007:
			m_pSquare2->Write(3, Value);
			break;
		// Channel 3... (doesn't exist)
		// Control
		case 0x5015:
			m_pSquare1->WriteControl(Value & 1);
			m_pSquare2->WriteControl((Value >> 1) & 1);
			break;
		// Hardware multiplier
		case 0x5205:
			m_iMulLow = Value;
			break;
		case 0x5206:
			m_iMulHigh = Value;
			break;
	}
}

uint16 CMMC5::Read(uint16 Address)
{
	if (Address >= 0x5C00 && Address <= 0x5FF5)
		return m_pEXRAM[Address & 0x3FF];
	
	switch (Address) {
		case 0x5205:
			return (m_iMulLow * m_iMulHigh) & 0xFF;
		case 0x5206:
			return (m_iMulLow * m_iMulHigh) >> 8;
	}

	return -1;
}

void CMMC5::EndFrame()
{
	m_pSquare1->EndFrame();
	m_pSquare2->EndFrame();
}

void CMMC5::Process(uint32 Time)
{
	m_pSquare1->Process(Time);
	m_pSquare2->Process(Time);
}

void CMMC5::LengthCounterUpdate()
{
	m_pSquare1->LengthCounterUpdate();
	m_pSquare2->LengthCounterUpdate();
}

void CMMC5::EnvelopeUpdate()
{
	m_pSquare1->EnvelopeUpdate();
	m_pSquare2->EnvelopeUpdate();
}

// MMC5 Square

CMMCSquare::~CMMCSquare()
{
}

void CMMCSquare::Reset()
{
	m_iEnabled = m_iControlReg = 0;
	m_iCounter = m_iEnvelopeCounter = 0;

	Write(0, 0);
	Write(1, 0);
	Write(2, 0);
	Write(3, 0);

	EndFrame();
}

void CMMCSquare::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
	case 0x00:
		m_iDutyLength = Value >> 6;
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

void CMMCSquare::WriteControl(uint8 Value)
{
	m_iControlReg = Value & 0x01;

	if (m_iControlReg == 0)
		m_iEnabled = 0;
}

uint8 CMMCSquare::ReadControl()
{
	return ((m_iLengthCounter > 0) && (m_iEnabled == 1));
}

void CMMCSquare::Process(uint32 Time)
{
	if (!m_iFrequency) {
		m_iFrameCycles += Time;
		return;
	}

	bool Valid = (m_iFrequency > 7) && (m_iEnabled != 0) && (m_iLengthCounter > 0);

	while (Time >= m_iCounter) {
		Time			-= m_iCounter;
		m_iFrameCycles	+= m_iCounter;
		m_iCounter		= m_iFrequency + 1;
		m_iDutyCycle	= (m_iDutyCycle + 1) & 0x0F;

		Mix(Valid ? (CSquare::DUTY_TABLE[m_iDutyLength][m_iDutyCycle] * (m_iEnvelopeFix ? m_iFixedVolume : m_iEnvelopeVolume)) : 0);
	}

	m_iCounter -= Time;
	m_iFrameCycles += Time;
}

void CMMCSquare::LengthCounterUpdate()
{
	if ((m_iLooping == 0) && (m_iLengthCounter > 0)) m_iLengthCounter--;
}

void CMMCSquare::EnvelopeUpdate()
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
