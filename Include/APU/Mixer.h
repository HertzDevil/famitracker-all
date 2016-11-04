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

#ifndef _MIXER_H_
#define _MIXER_H_

#include "SoundInterface.h"
#include "blip_buffer.h"

enum CHAN_IDS {
	CHANID_SQUARE1,
	CHANID_SQUARE2,
	CHANID_TRIANGLE,
	CHANID_NOISE,
	CHANID_DPCM,

	CHANID_VRC6_PULSE1,
	CHANID_VRC6_PULSE2,
	CHANID_VRC6_SAWTOOTH,

	CHANID_MMC5_SQUARE1,
	CHANID_MMC5_SQUARE2,
	CHANID_MMC5_VOICE,

	CHANID_N106_CHAN1,
	CHANID_N106_CHAN2,
	CHANID_N106_CHAN3,
	CHANID_N106_CHAN4,
	CHANID_N106_CHAN5,
	CHANID_N106_CHAN6,
	CHANID_N106_CHAN7,
	CHANID_N106_CHAN8,

	CHANID_FDS,

	CHANID_VRC7_CH1,
	CHANID_VRC7_CH2,
	CHANID_VRC7_CH3,
	CHANID_VRC7_CH4,
	CHANID_VRC7_CH5,
	CHANID_VRC7_CH6,

	CHANID_5B_CH1,
	CHANID_5B_CH2,
	CHANID_5B_CH3,

	CHANNELS
};
/*
struct stMixerSettings {
	int m_iMaster;
	int m_iInternal;
	int m_iVRC6;
	int m_iVRC7;
	int m_iMMC5;
	int m_iFDS;
	int m_iN106;
	int m_iFME07;
	int m_iChannels[CHANNELS];
	int m_iChannelPan[CHANNELS];
};
*/
class CMixer
{
	public:
		CMixer();
		~CMixer();

		bool	Init();
		void	Shutdown();
		void	ExternalSound(int Chip);
		void	AddValue(int ChanID, int Chip, int Value, int FrameCycles);
		void	UpdateSettings(int LowCut,	int HighCut, int HighDamp, int OverallVol);

		bool	AllocateBuffer(unsigned int Size, uint32 SampleRate, uint32 ClockRate, uint8 NrChannels);
		void	SetClockRate(int Rate);
		void	ClearBuffer();
		int		FinishBuffer(int t);
		int		SamplesAvail();

		void	MixSamples(blip_sample_t *pBuffer, uint32 Count);
		uint32	GetMixSampleCount(int t);

		void	AddSample(int ChanID, int Value);

		int		ReadBuffer(int Size, void *Buffer, bool Stereo);

		int32	GetChanOutput(uint8 Chan);

	private:
		inline double CalcPin1(double Val1, double Val2);
		inline double CalcPin2(double Val1, double Val2, double Val3);

		void MixInternal(int Value, int Time);
		void MixN106(int Value, int Time);
		void MixFDS(int Value, int Time);
		void MixVRC6(int Value, int Time);
		void MixMMC5(int Value, int Time);

		// Blip buffer synths
		Blip_Synth<blip_good_quality, -500>		Synth2A03;
		Blip_Synth<blip_good_quality, /*-170*/ -500>		SynthVRC6;
		Blip_Synth<blip_good_quality, -130>		SynthMMC5;	

		Blip_Synth<blip_good_quality, -1600>	SynthN106;
		Blip_Synth<blip_good_quality, -3500>	SynthFDS;

		Blip_Buffer		BlipBuffer;

		int32			*m_pSampleBuffer;

		int32			Channels[CHANNELS];
		uint8			ExternalChip;
		bool			StereoEnabled;

		uint32			m_iSampleRate;

		int32			ChannelLevels[CHANNELS];
		uint32			ChanLevelFallOff[CHANNELS];
};

#endif /* _MIXER_H_ */