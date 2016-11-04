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
#include "dpcm.h"

const uint16 CDPCM::DMC_FREQ_NTSC[] = {428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 85, 72, 54};
const uint16 CDPCM::DMC_FREQ_PAL[]	= {398, 354, 316, 298, 266, 236, 210, 199, 177, 149, 132, 119, 99, 79, 67, 50};

CDPCM::CDPCM(CMixer *pMixer, CSampleMem *pSampleMem, int ID) : CChannel(pMixer, ID, SNDCHIP_NONE), m_pSampleMem(pSampleMem)
{
}

CDPCM::~CDPCM()
{
}

void CDPCM::Reset()
{
	m_iEnabled = 0;
	m_iCounter = m_iFrequency = 0;

	m_iBitDivider = m_iShiftReg = 0;
	m_iDMA_LoadReg = m_iDMA_Address = 0;
	m_iDMA_Length = m_iDMA_LengthCounter = 0;
	
	m_bTriggeredIRQ = m_bSampleFilled = false;

	// loaded with 0 on power-up.
	m_iDeltaCounter = 0;

	EndFrame();
}

void CDPCM::SetSpeed(int Speed)
{
	for (int i = 0; i < 16; ++i)
		m_iDMC_FreqTable[i] = (Speed == MACHINE_NTSC) ? DMC_FREQ_NTSC[i] : DMC_FREQ_PAL[i];
}

void CDPCM::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
		case 0x00:
			m_iPlayMode = (Value & 0xC0);
			m_iFrequency = m_iDMC_FreqTable[(Value & 0x0F)];
			if ((Value & 0x80) == 0x00) 
				m_bTriggeredIRQ = false;
			break;
		case 0x01:
			m_iDeltaCounter = Value & 0x7F;
			Mix(m_iDeltaCounter);
			break;
		case 0x02:
			m_iDMA_LoadReg = Value;
			break;
		case 0x03:
			m_iDMA_Length = Value;
			break;
	}
}

void CDPCM::WriteControl(uint8 Value)
{
	if ((Value & 1) == 0) {
		m_iEnabled = 0;
		m_iDMA_LengthCounter = 0;
	}
	else {
		if (m_iEnabled == 0)
			Reload();

		m_iEnabled = 1;
	}

	m_bTriggeredIRQ = false;
}

uint8 CDPCM::ReadControl()
{
	return m_iEnabled ? 1 : 0;
}

uint8 CDPCM::DidIRQ()
{
	return m_bTriggeredIRQ ? 1 : 0;
}

void CDPCM::Reload()
{
    m_iDMA_Address		 = m_iDMA_LoadReg << 6 | 0x4000;
    m_iDMA_LengthCounter = (m_iDMA_Length << 4) + 1;
	m_iBitDivider		 = 8;
	m_iCounter			 = m_iFrequency;

	m_bSampleFilled		 = false;
	m_bSilenceFlag		 = false;

//	CheckSample(DMA_LoadRegCnt | 0x8000, DMA_LengthCounter, Frequency);
}

void CDPCM::Process(uint32 Time)
{
	if (!m_iEnabled || m_bSilenceFlag) {
		m_iFrameCycles += Time;
		return;
	}

	while (Time > m_iCounter) {
		Time		   -= m_iCounter;
		m_iFrameCycles += m_iCounter;
		m_iCounter	   = m_iFrequency;

		// DMA reader
		// Check if a new byte should be fetched
		if (!m_bSampleFilled && m_iDMA_LengthCounter > 0) {

//			m_iSampleBuffer	= m_pEmulator->ReadMemory(m_iDMA_Address | 0x8000);
			m_iSampleBuffer = m_pSampleMem->Read(m_iDMA_Address | 0x8000);
			m_bSampleFilled = true;
//			m_pEmulator->ConsumeCycles(4);

			m_iDMA_Address = (m_iDMA_Address + 1) & 0x7FFF;
			m_iDMA_LengthCounter--;

			if (!m_iDMA_LengthCounter) {
				switch (m_iPlayMode) {
					case 0x00:	// Stop
						m_iEnabled = 0;
						break;
					case 0x40:	// Loop
					case 0xC0:
						Reload();
						break;
					case 0x80:	// Stop and do IRQ (not when an NSF is playing)
						m_bTriggeredIRQ = true;
						m_iEnabled = 0;
						break;
				}
			}
		}

		// Output unit
		if (!m_iBitDivider) {
			if (m_bSampleFilled) {
				m_iBitDivider = 8;
				m_iShiftReg = m_iSampleBuffer;
				m_bSampleFilled  = false;
			}
			else {
				m_bSilenceFlag = true;
			}
		}

		if (!m_bSilenceFlag) {
			if ((m_iShiftReg & 1) == 1) {
				if (m_iDeltaCounter < 126)
					m_iDeltaCounter += 2;
			}
			else {
				if (m_iDeltaCounter > 1)
					m_iDeltaCounter -= 2;
			}

			m_iShiftReg >>= 1;
			m_iBitDivider--;
		}

		Mix(m_iDeltaCounter);
	}

	m_iCounter -= Time;
	m_iFrameCycles += Time;
}
