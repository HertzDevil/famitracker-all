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
#include "common.h"
#include "APU/APU.h"
#include "APU/FDS.h"

//
// This was made out of Disch's FDS documentation
//

const int8 SWEEP_BIAS[] = {0, 1, 2, 4, 0, -4, -2, -1};

void CFDS::Init(CMixer *pMixer)
{
	CExChannel::Init(pMixer, SNDCHIP_FDS, CHANID_FDS);
}

void CFDS::Reset()
{
	m_iVolumeEnvCounter = 0;
	m_iModTableAddress = 0;
	m_iEnvelopeSpeed = 0xFF;
	m_iFrequency = 0;
	m_iWaveGenCounter = 0;
	m_iModAlt = 0;

	m_iWaveReadPointer = 0;

	// Counters
	m_iWaveGenCounter = 0;
	m_iVolumeEnvCounter = 0;
	m_iSweepEnvCounter = 0;
	m_iModCounter = 0;

	memset(m_iWaveTable, 0, 64);

	EndFrame();
}

void CFDS::Write(uint16 Address, uint8 Value)
{
	static int i;

	if (Address >= 0x4040 && Address < 0x4080) {
		// Wave table
		if (m_iWriteMode)
			m_iWaveTable[Address & 0x3F] = Value & 0x3F;
	}
	else {
		switch (Address) {
			case 0x4080:
				m_iVolumeEnvDisable	= Value & 0x80;
				m_iVolumeEnvMode	= Value & 0x40;
				m_iVolumeEnvSpeed	= Value & 0x3F;
				break;
			case 0x4082:
				m_iFrequency = (m_iFrequency & 0x0F00) | Value;
				break;
			case 0x4083:
				m_iFrequency = (m_iFrequency & 0xFF) | ((Value & 0x0F) << 8);
				m_iDisabled = Value & 0x80;
				m_iEnvelopeDisable = Value & 0x40;
				break;
			case 0x4084:
				m_iSweepEnvDisable = Value & 0x80;
				m_iSweepEnvMode = Value & 0x40;
				m_iSweepEnvSpeed = Value & 0x3F;
				break;
			case 0x4085:
				m_iSweepBias = (Value & 0x7F) | (Value & 0x40) << 1; // extend sign bit
				m_iModTableAddress = 0;
				break;
			case 0x4086:
				m_iModFrequency	= (m_iModFrequency & 0x0F00) | Value;
				break;
			case 0x4087:
				m_iModFrequency	= (m_iModFrequency & 0xFF) | ((Value & 0x0F) << 8);
				m_iModDisable = Value & 0x80; 
				break;
			case 0x4088:
				memcpy(m_iModTable, m_iModTable + 2, sizeof(uint8) * 62);
				m_iModTable[62] = Value & 0x07;
				m_iModTable[63] = Value & 0x07;
				break;
			case 0x4089:
				m_iWriteMode = Value & 0x80;
				m_iMasterVolume = Value & 0x03;
				break;
			case 0x408A:
				m_iEnvelopeSpeed = Value;
				break;
			case 0x4090:	// read-only
			case 0x4092:	// read-only
				break;
		}
	}
}

uint8 CFDS::Read(uint16 Address)
{
	if (Address >= 0x4040 && Address < 0x4080) {
		// Wave table
		return m_iWaveTable[Address & 0x3F] | 0x40;
	}

	switch (Address) {
		case 0x4090:
			return m_iVolumeEnvSpeed | 0x40;
		case 0x4092:
			return m_iSweepEnvSpeed | 0x40;
	}

	return 0;
}

void CFDS::EndFrame()
{
	m_iFrameCycles = 0;
}

void CFDS::Process(uint32 Time)
{
	uint32	TimePart;
	int32	OutSample;
	int16	temp;

	while (Time > 0) {

		TimePart = Time;

		// Find the lowest needed time for next step
		if (m_iEnvelopeSpeed > 0) {
			// Volume envelope
			if (!m_iVolumeEnvDisable && TimePart > m_iVolumeEnvCounter && m_iVolumeEnvCounter > 0 && m_iVolumeEnvSpeed > 0)
				TimePart = m_iVolumeEnvCounter;
			// Sweep envelope
			if (!m_iSweepEnvDisable && TimePart > m_iSweepEnvCounter && m_iSweepEnvCounter > 0 && m_iSweepEnvSpeed > 0)
				TimePart = m_iSweepEnvCounter;
		}

		// Modulation counter
		if (TimePart > m_iModCounter && m_iModCounter > 0 && !m_iModDisable && m_iModFrequency > 0)
			TimePart = m_iModCounter;

		// Main unit counter
		if (TimePart > m_iWaveGenCounter && m_iWaveGenCounter > 0 && !m_iDisabled && !m_iWriteMode && (m_iFrequency + m_iModAlt) > 0 && m_iFrequency > 0)
			TimePart = m_iWaveGenCounter;

		m_iFrameCycles += TimePart;
		Time -= TimePart;

		if (!m_iEnvelopeDisable && m_iEnvelopeSpeed > 0) {

			// Volume envelope
			if (!m_iVolumeEnvDisable && m_iVolumeEnvSpeed > 0) {
				if (m_iVolumeEnvCounter > 0)
					m_iVolumeEnvCounter -= (uint16)TimePart;
				if (!m_iVolumeEnvCounter) {
					m_iVolumeEnvCounter = (m_iEnvelopeSpeed << 3) * (m_iVolumeEnvSpeed + 1);
					if (m_iVolumeEnvMode) {
						if (m_iVolumeGain < 0x20)
							m_iVolumeGain++;
					}
					else {
						if (m_iVolumeGain > 0x00)
							m_iVolumeGain--;
					}
				}
			}
			else
				m_iVolumeGain = m_iVolumeEnvSpeed;
			
			// Sweep envelope
			if (!m_iSweepEnvDisable && m_iSweepEnvSpeed > 0) {
				if (m_iSweepEnvCounter > 0)
					m_iSweepEnvCounter -= (uint16)TimePart;
				if (!m_iSweepEnvCounter) {
					m_iSweepEnvCounter = (m_iEnvelopeSpeed << 3) * (m_iSweepEnvSpeed + 1);
					if (m_iSweepEnvMode) {
						if (m_iSweepGain < 0x20)
							m_iSweepGain++;
					}
					else {
						if (m_iSweepGain > 0x00)
							m_iSweepGain--;
					}
				}
			}
			else
				m_iSweepGain = m_iSweepEnvSpeed;

		}

		// Frequency modulation
		if (!m_iModDisable && m_iModFrequency > 0) {

			if (m_iModCounter > 0)
				m_iModCounter -= (uint16)TimePart;

			if (!m_iModCounter) {
				m_iModCounter += 65536 / m_iModFrequency;

				if (m_iModTable[m_iModTableAddress] == 4)
					m_iSweepBias = 0;
				
				m_iSweepBias += SWEEP_BIAS[m_iModTable[m_iModTableAddress]];
				m_iSweepBias = (m_iSweepBias & 0x3F) | (m_iSweepBias & 0x40) << 1;

				m_iModTableAddress = (m_iModTableAddress + 1) & 0x3F;

				temp = (m_iSweepBias * m_iSweepGain);

				if(temp & 0x0F) {
				//	temp /= 16;
					temp >>= 4;
					if(m_iSweepBias < 0)
						temp -= 1;
					else
						temp += 2;
				}
				else
					temp /= 16;

				if(temp > 193)
					temp -= 258;  // not a typo... for some reason the wraps are inconsistent
				if(temp < -64)
					temp += 256;
					
				m_iModAlt = (m_iFrequency * temp) / 256;
			}
		}
		else {
			m_iModAlt = 0;
		}

		// Wave generation
		if (!m_iDisabled && !m_iWriteMode && m_iFrequency > 0 && (m_iFrequency + m_iModAlt) > 0) {

			if (m_iWaveGenCounter > 0)
				m_iWaveGenCounter -= TimePart;

			if (!m_iWaveGenCounter) {
				m_iWaveGenCounter += 65536 / (m_iFrequency + m_iModAlt);
				OutSample = m_iWaveTable[m_iWaveReadPointer] * m_iVolumeGain;
				m_iWaveReadPointer = (m_iWaveReadPointer + 1) & 0x3F;

				switch (m_iMasterVolume) {
					case 1: OutSample = (OutSample * 20) / 30; break;
					case 2: OutSample = (OutSample * 15) / 30; break;
					case 3: OutSample = (OutSample * 12) / 30; break;
				}

				Mix(OutSample);
			}
		}
	}
}
