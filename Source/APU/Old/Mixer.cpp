/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2007  Jonathan Liss
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

#include "stdafx.h"
#include "apu/mixer.h"
#include "apu/apu.h"
#include "blip_buffer.h"

//#define LINEAR_MIXING

const int MAX_VOL		= 200;

const int OVERALL_AMP	= 1;
const int INTRNAL_AMP	= 750;
const int VRC6_AMP		= 4;
const int VRC7_AMP		= 2;
const int MMC5_AMP		= 8;
const int FDS_AMP		= 1;
const int N106_AMP		= 1;

static Blip_Buffer	BlipBuffer;
static Blip_Synth<blip_high_quality, -2> BlipSynth;

CMixer::CMixer()
{
	m_bVolRead = false;
}

CMixer::~CMixer()
{
}

inline double CMixer::CalcPin1(double Val1, double Val2)
{
	// Mix the APU audio pin 1 output: the square channels
	//

	if ((Val1 + Val2) > 0)
		return 95.88 / ((8128.0 / (Val1 + Val2)) + 100.0);

	return 0;
}

inline double CMixer::CalcPin2(double Val1, double Val2, double Val3)
{
	// Mix the APU audio pin 2 output: triangle, noise and DPCM
	//

	if ((Val1 + Val2 + Val3) > 0)
		return 159.79 / ((1.0 / ((Val1 / 8227.0 ) + (Val2 / 12241.0) + (Val3 / 22638.0))) + 100.0);

	return 0;
}

bool CMixer::Init()
{
	memset(Channels, 0, sizeof(stChanVal) * CHANNELS);
	memset(ExternalSampleBuffer, 0, sizeof(int32) * 1000);

	ExternalSamples = 0;

	return true;
}

void CMixer::ExternalSound(int Chip)
{
	ExternalChip = Chip;
}

void CMixer::SetChannelVolume(int ChanID, double VolLeft, double VolRight)
{
	Channels[ChanID].VolLeft	= VolLeft;
	Channels[ChanID].VolRight	= VolRight;
}

void CMixer::UpdateSettings(int LowCut, int HighCut, int HighDamp, int OverallVol)
{
	BlipBuffer.bass_freq(LowCut);
	BlipSynth.treble_eq(blip_eq_t(-HighDamp, HighCut, m_iSampleRate));

	float Vol = float(OverallVol) / 100.0f;

	BlipSynth.volume(0.005f * Vol);

	InternalVol = INTRNAL_AMP;
	VRC6Vol = VRC6_AMP * 2;
	MasterVol = OVERALL_AMP;
}

void CMixer::AddSample(int ChanID, int Value)
{
	switch (ChanID) {
		case CHANID_VRC7:
			ExternalSampleBuffer[ExternalSamples] = Value * (int32)VRC7Vol;
			break;
	}
	ExternalSamples++;
}

void CMixer::AddValue(int ChanID, int Value, int FrameCycles)
{
	// Add a new channel output value to mix
	//
	// I only got accurate data for mixing the NES internal channels,
	// the rest of the channles are just added together
	//

	static double LastSumL, LastSumR, LastSum;
	double /*SumL, SumR,*/ Sum;
	int DeltaL, DeltaR;
	int AbsVol;

	AbsVol = abs(Value);

	if (ChanID == CHANID_VRC6_SAWTOOTH) {
		AbsVol /= 2;
	}

	if (AbsVol >= Channels[ChanID].VolMeasure) {
		Channels[ChanID].VolMeasure = AbsVol;
		Channels[ChanID].VolFallOff = 3;
	}

	Channels[ChanID].Mono	= Value;

//	if (!StereoEnabled) {
#ifdef LINEAR_MIXING
		Sum = ((Channels[CHANID_SQUARE1].Mono + Channels[CHANID_SQUARE2].Mono) * 0.00752 + (0.00851 * Channels[CHANID_TRIANGLE].Mono + 0.00494 * Channels[CHANID_NOISE].Mono + 0.00335 * Channels[CHANID_DPCM].Mono));
#else /* LINEAR_MIXING */
		Sum = (CalcPin1(Channels[CHANID_SQUARE1].Mono, Channels[CHANID_SQUARE2].Mono) + CalcPin2(Channels[CHANID_TRIANGLE].Mono, Channels[CHANID_NOISE].Mono, Channels[CHANID_DPCM].Mono)) * InternalVol;
#endif /* LINEAR_MIXING */

		// External channels are mixed linear
		switch (ExternalChip) {
			case SNDCHIP_VRC6: Sum += (Channels[CHANID_VRC6_PULSE1].Mono + Channels[CHANID_VRC6_PULSE2].Mono + Channels[CHANID_VRC6_SAWTOOTH].Mono) * VRC6Vol; break;
				/*
			case SNDCHIP_VRC7: Sum += Channels[CHANID_VRC7].Mono * VRC7Vol; break;
			case SNDCHIP_FDS: Sum += (Channels[CHANID_FDS].Mono) * FDSVol; break;
			case SNDCHIP_MMC5: Sum += (Channels[CHANID_MMC5_SQUARE1].Mono + Channels[CHANID_MMC5_SQUARE2].Mono) * MMC5Vol; break;
			case SNDCHIP_N106: Sum += (Channels[CHANID_N106_CHAN1].Mono + Channels[CHANID_N106_CHAN2].Mono + Channels[CHANID_N106_CHAN3].Mono + Channels[CHANID_N106_CHAN4].Mono + Channels[CHANID_N106_CHAN5].Mono + Channels[CHANID_N106_CHAN6].Mono + Channels[CHANID_N106_CHAN7].Mono + Channels[CHANID_N106_CHAN8].Mono) * N106Vol; break;
			case SNDCHIP_FME07:
				break;
				*/
		}

		Sum *= MasterVol / 2;
		DeltaL = int(Sum - LastSum);
		DeltaR = DeltaL;
		LastSum = Sum;
		/*
	}
	else {

#ifdef LINEAR_MIXING
		SumL = ((Channels[CHANID_SQUARE1].Left + Channels[CHANID_SQUARE2].Left) * 0.00752 + (0.00851 * Channels[CHANID_TRIANGLE].Left + 0.00494 * Channels[CHANID_NOISE].Left + 0.00335 * Channels[CHANID_DPCM].Left)) * InternalVol;
		SumR = ((Channels[CHANID_SQUARE1].Right + Channels[CHANID_SQUARE2].Right) *  0.00752 + (0.00851 * Channels[CHANID_TRIANGLE].Right + 0.00494 * Channels[CHANID_NOISE].Right + 0.00335 * Channels[CHANID_DPCM].Right)) * InternalVol;
#else /* LINEAR_MIXING *//*
		SumL = (CalcPin1(Channels[CHANID_SQUARE1].Left, Channels[CHANID_SQUARE2].Left) + CalcPin2(Channels[CHANID_TRIANGLE].Left, Channels[CHANID_NOISE].Left, Channels[CHANID_DPCM].Left)) * InternalVol;
		SumR = (CalcPin1(Channels[CHANID_SQUARE1].Right, Channels[CHANID_SQUARE2].Right) + CalcPin2(Channels[CHANID_TRIANGLE].Right, Channels[CHANID_NOISE].Right, Channels[CHANID_DPCM].Right)) * InternalVol;
#endif /* LINEAR_MIXING */
/*
		// External channels are mixed linear
		switch (ExternalChip) {
			case SNDCHIP_VRC6:
				SumL += (Channels[CHANID_VRC6_PULSE1].Left + Channels[CHANID_VRC6_PULSE2].Left + Channels[CHANID_VRC6_SAWTOOTH].Left) * VRC6Vol;
				SumR += (Channels[CHANID_VRC6_PULSE1].Right + Channels[CHANID_VRC6_PULSE2].Right + Channels[CHANID_VRC6_SAWTOOTH].Right) * VRC6Vol;
				break;
			case SNDCHIP_VRC7:
				SumL += Channels[CHANID_VRC7].Left * VRC7Vol;
				SumR += Channels[CHANID_VRC7].Right * VRC7Vol;
				break;
			case SNDCHIP_FDS:
				SumL += (Channels[CHANID_FDS].Left) * FDSVol;
				SumR += (Channels[CHANID_FDS].Right) * FDSVol;
				break;
			case SNDCHIP_MMC5:
				SumL += (Channels[CHANID_MMC5_SQUARE1].Left + Channels[CHANID_MMC5_SQUARE2].Left) * MMC5Vol;
				SumR += (Channels[CHANID_MMC5_SQUARE1].Right + Channels[CHANID_MMC5_SQUARE2].Right) * MMC5Vol;
				break;
			case SNDCHIP_N106:
				SumL += (Channels[CHANID_N106_CHAN1].Left + Channels[CHANID_N106_CHAN2].Left + Channels[CHANID_N106_CHAN3].Left + Channels[CHANID_N106_CHAN4].Left + Channels[CHANID_N106_CHAN5].Left + Channels[CHANID_N106_CHAN6].Left + Channels[CHANID_N106_CHAN7].Left + Channels[CHANID_N106_CHAN8].Left) * N106Vol;
				SumR += (Channels[CHANID_N106_CHAN1].Right + Channels[CHANID_N106_CHAN2].Right + Channels[CHANID_N106_CHAN3].Right + Channels[CHANID_N106_CHAN4].Right + Channels[CHANID_N106_CHAN5].Right + Channels[CHANID_N106_CHAN6].Right + Channels[CHANID_N106_CHAN7].Right + Channels[CHANID_N106_CHAN8].Right) * N106Vol;
				break;
			case SNDCHIP_FME07:
				break;
		}

		SumL *= MasterVol;
		SumR *= MasterVol;

		DeltaL = int(SumL - LastSumL);
		DeltaR = int(SumR - LastSumR);
	
		LastSumL = SumL;
		LastSumR = SumR;
	}
*/
	if (DeltaL + DeltaR)
		AddDelta(DeltaL, DeltaR, FrameCycles);
}

bool CMixer::AllocateBuffer(unsigned int BufferLength, uint32 SampleRate, uint32 ClockRate)
{
	BlipBuffer.sample_rate(SampleRate, BufferLength);
	BlipBuffer.clock_rate(ClockRate);

	BlipSynth.volume(0.005f);
	BlipSynth.output(&BlipBuffer);

	m_iSampleRate = SampleRate;

	return true;
}

void CMixer::SetClockRate(int Rate)
{
	// Change the clockrate
	BlipBuffer.clock_rate(Rate);
}

void CMixer::AddDelta(int AmpLeft, int AmpRight, int Time)
{
	BlipSynth.offset(Time, (AmpLeft + AmpRight) >> 1, &BlipBuffer);
}

void CMixer::ClearBuffer()
{
	BlipBuffer.clear();
}

int CMixer::SamplesAvail()
{
	return (int)BlipBuffer.samples_avail();
}

int CMixer::FinishBuffer(int t)
{
	// Finishes the blip_buffer buffer, returns samples avaliable

	BlipBuffer.end_frame(t);

	for (int i = 0; i < CHANNELS; i++) {
		if (Channels[i].VolFallOff > 0) {
			Channels[i].VolFallOff--;
		}
		else {
			Channels[i].VolFallOff = 1;
			if (Channels[i].VolMeasure > 0) {
				if (i == 4) {
					Channels[i].VolMeasure -= 8;
					if (Channels[i].VolMeasure < 0)
						Channels[i].VolMeasure = 0;
				}
				else
					Channels[i].VolMeasure -= 1;
				if (Channels[i].VolMeasure < 0)
					Channels[i].VolMeasure = 0;
			}
		}
	}

	return BlipBuffer.samples_avail();
}

int CMixer::ReadBuffer(int Size, void *Buffer, bool Stereo)
{
	return BlipBuffer.read_samples((blip_sample_t*)Buffer, Size);
}

int32 CMixer::GetChanOutput(uint8 Chan)
{
	int32 Ret = Channels[Chan].VolMeasure;
	m_bVolRead = true;
	return Ret;
}