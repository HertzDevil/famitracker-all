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
#include "noise.h"

const uint16 CNoise::NOISE_FREQ[] = 
	{0x04, 0x08, 0x10, 0x20, 0x40, 0x60, 0x80, 0xA0, 
	 0xCA, 0xFE, 0x17C, 0x1FC, 0x2FA, 0x3F8, 0x7F2, 0xFE4};

CNoise::CNoise(CMixer *pMixer, int ID)
{
	m_pMixer = pMixer;
	m_iChanId = ID;
	m_iChip = SNDCHIP_NONE;
}

CNoise::~CNoise()
{
}

void CNoise::Reset()
{
	m_iEnabled = m_iControlReg = 0;
	m_iCounter = m_iLengthCounter = 0;
	
	m_iShiftReg = 1;

	Write(0, 0);
	Write(1, 0);
	Write(2, 0);
	Write(3, 0);

	EndFrame();
}

void CNoise::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
	case 0x00:
		m_iLooping = (Value & 0x20);
		m_iEnvelopeFix = (Value & 0x10);
		m_iFixedVolume = (Value & 0x0F);
		m_iEnvelopeSpeed = (Value & 0x0F) + 1;
		break;
	case 0x01:
		break;
	case 0x02:
		m_iFrequency = NOISE_FREQ[Value & 0x0F];
		m_iSampleRate = (Value & 0x80) ? 8 : 13;
		break;
	case 0x03:
		m_iLengthCounter = CAPU::LENGTH_TABLE[((Value & 0xF8) >> 3)] + 1;
		m_iEnvelopeVolume = 0x0F;
		if (m_iControlReg)
			m_iEnabled = 1;
		break;
	}
}

void CNoise::WriteControl(uint8 Value)
{
	m_iControlReg = Value & 1;

	if (m_iControlReg == 0) 
		m_iEnabled = 0;
}

uint8 CNoise::ReadControl()
{
	return ((m_iLengthCounter > 0) && (m_iEnabled == 1));
}

void CNoise::Process(uint32 Time)
{
	bool Output = m_iEnabled && (m_iLengthCounter > 0);
	static uint8 LastSample;	// I think this wouldn't be allowed if there was more than one instance of CNoise

	while (Time >= m_iCounter) {
		Time			-= m_iCounter;
		m_iFrameCycles	+= m_iCounter;
		m_iCounter		= m_iFrequency;
		
		m_iShiftReg = (((m_iShiftReg << 14) ^ (m_iShiftReg << m_iSampleRate)) & 0x4000) | (m_iShiftReg >> 1);
		
		if (Output && LastSample != (m_iShiftReg & 0x01)) {
			LastSample = (m_iShiftReg & 0x01);
			Mix((LastSample && Output) ? (m_iEnvelopeFix ? m_iFixedVolume : m_iEnvelopeVolume) : 0);
		}
	}

	m_iCounter -= Time;
	m_iFrameCycles += Time;
}

void CNoise::LengthCounterUpdate()
{
	if ((m_iLooping == 0) && (m_iLengthCounter > 0)) 
		m_iLengthCounter--;
}

void CNoise::EnvelopeUpdate()
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
