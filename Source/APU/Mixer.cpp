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

//
// This will mix and resample the APU audio using blip-buffer
//
// Mixing of internal audio relies on Blargg's findings
//

/*

 Mixing of external channles are based on my own research:

 VRC6 (Madara): 
	Pulse channels has the same amplitude as internal-
    pulse channels on equal volume levels.

 FDS: 
	Square wave @ v = $1F: 2.4V
	  			  v = $0F: 1.25V
	(internal square wave: 1.0V)

 MMC5 (just breed): 
	2A03 square @ v = $0F: 760mV (the cart attenuates internal channels a little)
	MMC5 square @ v = $0F: 900mV

 VRC7:
	2A03 Square  @ v = $0F: 300mV (the cart attenuates internal channels a lot)
	VRC7 Patch 5 @ v = $0F: 900mV
	Did some more tests and found patch 14 @ v=15 to be 13.77dB stronger than a 50% square @ v=15

 ---

 N106 & 5B are still unknown

*/

#include "../stdafx.h"
#include <memory>
#include "mixer.h"
#include "apu.h"
#include "emu2413.h"

//#define LINEAR_MIXING

static const double AMP_2A03 = 400.0;

static const float LEVEL_FALL_OFF_RATE	= 0.6f;
static const int   LEVEL_FALL_OFF_DELAY = 3;

CMixer::CMixer()
{
	memset(m_iChannels, 0, sizeof(int32) * CHANNELS);
	memset(m_fChannelLevels, 0, sizeof(float) * CHANNELS);
	memset(m_iChanLevelFallOff, 0, sizeof(uint32) * CHANNELS);
}

CMixer::~CMixer()
{
}

inline double CMixer::CalcPin1(double Val1, double Val2)
{
	// Mix the output of APU audio pin 1: square
	//

	if ((Val1 + Val2) > 0)
		return 95.88 / ((8128.0 / (Val1 + Val2)) + 100.0);

	return 0;
}

inline double CMixer::CalcPin2(double Val1, double Val2, double Val3)
{
	// Mix the output of APU audio pin 2: triangle, noise and DPCM
	//

	if ((Val1 + Val2 + Val3) > 0)
		return 159.79 / ((1.0 / ((Val1 / 8227.0) + (Val2 / 12241.0) + (Val3 / 22638.0))) + 100.0);

	return 0;
}

void CMixer::ExternalSound(int Chip)
{
	m_iExternalChip = Chip;
	UpdateSettings(m_iLowCut, m_iHighCut, m_iHighDamp, m_iOverallVol);
}

void CMixer::UpdateSettings(int LowCut,	int HighCut, int HighDamp, int OverallVol)
{
	float fVol = float(OverallVol) / 100.0f;

	if (m_iExternalChip & SNDCHIP_VRC7)
		// Decrease the internal audio when VRC7 is enabled to increase the headroom
		m_fDamping = 0.34f;
	else
		m_fDamping = 1.0f;

	// Blip-buffer filtering
	BlipBuffer.bass_freq(LowCut);

	Synth2A03.treble_eq(blip_eq_t(-HighDamp, HighCut, m_iSampleRate));
	SynthVRC6.treble_eq(blip_eq_t(-HighDamp, HighCut, m_iSampleRate));
	SynthMMC5.treble_eq(blip_eq_t(-HighDamp, HighCut, m_iSampleRate));
	SynthFDS.treble_eq(blip_eq_t(-HighDamp, HighCut, m_iSampleRate));
	SynthN106.treble_eq(blip_eq_t(-HighDamp, HighCut, m_iSampleRate));

	// Checked against hardware
	Synth2A03.volume(fVol * 1.0f * m_fDamping);
	SynthVRC6.volume(fVol * 3.98333f);
	SynthFDS.volume(fVol * 1.00f);
	SynthMMC5.volume(fVol * 1.18421f);
	
	// Not checked
	SynthN106.volume(fVol * 1.0f);

	m_iLowCut = LowCut;
	m_iHighCut = HighCut;
	m_iHighDamp = HighDamp;
	m_iOverallVol = OverallVol;
}

void CMixer::MixSamples(blip_sample_t *pBuffer, uint32 Count)
{
	// For VRC7
	BlipBuffer.mix_samples(pBuffer, Count);
}

uint32 CMixer::GetMixSampleCount(int t) const
{
	return BlipBuffer.count_samples(t);
}

bool CMixer::AllocateBuffer(unsigned int BufferLength, uint32 SampleRate, uint8 NrChannels)
{
	m_iSampleRate = SampleRate;
	BlipBuffer.sample_rate(SampleRate, (BufferLength * 1000 * 2) / SampleRate);
	return true;
}

void CMixer::SetClockRate(uint32 Rate)
{
	// Change the clockrate
	BlipBuffer.clock_rate(Rate);
}

void CMixer::ClearBuffer()
{
	BlipBuffer.clear();
}

int CMixer::SamplesAvail() const
{	
	return (int)BlipBuffer.samples_avail();
}

int CMixer::FinishBuffer(int t)
{
	BlipBuffer.end_frame(t);

	// Get channel levels for VRC7
	for (int i = 0; i < 6; ++i)
		StoreChannelLevel(CHANID_VRC7_CH1 + i, OPLL_getchanvol(i) >> 6);

	for (int i = 0; i < CHANNELS; ++i) {
		if (m_iChanLevelFallOff[i] > 0)
			m_iChanLevelFallOff[i]--;
		else {
			if (m_fChannelLevels[i] > 0) {
				m_fChannelLevels[i] -= LEVEL_FALL_OFF_RATE;
				if (m_fChannelLevels[i] < 0)
					m_fChannelLevels[i] = 0;
			}
		}
	}

	// Return number of samples available
	return BlipBuffer.samples_avail();
}

//
// Mixing
//

void CMixer::MixInternal(int Value, int Time)
{
	static double LastSum;
	double Sum, Delta;

#ifdef LINEAR_MIXING
	SumL = ((m_iChannels[CHANID_SQUARE1].Left + m_iChannels[CHANID_SQUARE2].Left) * 0.00752 + (0.00851 * m_iChannels[CHANID_TRIANGLE].Left + 0.00494 * m_iChannels[CHANID_NOISE].Left + 0.00335 * m_iChannels[CHANID_DPCM].Left)) * InternalVol;
	SumR = ((m_iChannels[CHANID_SQUARE1].Right + m_iChannels[CHANID_SQUARE2].Right) *  0.00752 + (0.00851 * m_iChannels[CHANID_TRIANGLE].Right + 0.00494 * m_iChannels[CHANID_NOISE].Right + 0.00335 * m_iChannels[CHANID_DPCM].Right)) * InternalVol;
#else
	Sum = CalcPin1(m_iChannels[CHANID_SQUARE1], m_iChannels[CHANID_SQUARE2]) + 
		  CalcPin2(m_iChannels[CHANID_TRIANGLE], m_iChannels[CHANID_NOISE], m_iChannels[CHANID_DPCM]);
#endif

	Delta = (Sum - LastSum) * AMP_2A03 /** m_dDamping*/;
	LastSum = Sum;

	if (Delta)
		Synth2A03.offset(Time, (int)Delta, &BlipBuffer);
}

void CMixer::MixN106(int Value, int Time)
{
	SynthN106.offset(Time, Value, &BlipBuffer);
}

void CMixer::MixFDS(int Value, int Time)
{
	SynthFDS.offset(Time, Value, &BlipBuffer);
}

void CMixer::MixVRC6(int Value, int Time)
{
	SynthVRC6.offset(Time, Value, &BlipBuffer);
}

void CMixer::MixMMC5(int Value, int Time)
{
	if (Value)
		SynthMMC5.offset(Time, Value, &BlipBuffer);
}

void CMixer::AddValue(int ChanID, int Chip, int Value, int AbsValue, int FrameCycles)
{
	// Add a new channel value to mix
	//

//	Channels[ChanID].Left  = int32(float(Value) * Channels[ChanID].VolLeft);
//	Channels[ChanID].Right = int32(float(Value) * Channels[ChanID].VolRight);

	StoreChannelLevel(ChanID, AbsValue);

	m_iChannels[ChanID] = Value;

	switch (Chip) {
		case SNDCHIP_NONE:
			MixInternal(Value, FrameCycles);
			break;
		case SNDCHIP_N106:
			MixN106(Value, FrameCycles);
			break;
		case SNDCHIP_FDS:
			MixFDS(Value, FrameCycles);
			break;
		case SNDCHIP_MMC5:
			MixMMC5(Value, FrameCycles);
			break;
		case SNDCHIP_VRC6:
			MixVRC6(Value, FrameCycles);
			break;
		case SNDCHIP_5B:
			break;
	}
}

int CMixer::ReadBuffer(int Size, void *Buffer, bool Stereo)
{
	return BlipBuffer.read_samples((blip_sample_t*)Buffer, Size);
}

int32 CMixer::GetChanOutput(uint8 Chan) const
{
	return (int32)m_fChannelLevels[Chan];
}

void CMixer::StoreChannelLevel(int Channel, int Value)
{
	int AbsVol = abs(Value);

	// Adjust channel levels for some channels
	if (Channel == CHANID_VRC6_SAWTOOTH)
		AbsVol = (AbsVol * 3) / 4;

	if (Channel == CHANID_DPCM)
		AbsVol /= 8;

	if (Channel == CHANID_FDS)
		AbsVol = AbsVol / 38;

	if (float(AbsVol) >= m_fChannelLevels[Channel]) {
		m_fChannelLevels[Channel] = float(AbsVol);
		m_iChanLevelFallOff[Channel] = LEVEL_FALL_OFF_DELAY;
	}
}
