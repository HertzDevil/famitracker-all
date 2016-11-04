/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2006  Jonathan Liss
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

#include "common.h"

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

	CHANID_VRC7,

	CHANNELS
};

struct stChanVal {
	double Left, Right, Mono;
	double VolLeft, VolRight;
	double Stereo;
	int32 VolMeasure, VolFallOff;
};

class CMixer
{
	public:
		CMixer();
		~CMixer();

		bool	Init();
		void	ExternalSound(int Chip);
		void	SetChannelVolume(int ChanID, double VolLeft, double VolRight);
		void	AddValue(int ChanID, int Value, int FrameCycles);
		void	UpdateSettings(int LowCut, int HighCut, int HighDamp);

		bool	AllocateBuffer(unsigned int Size, uint32 SampleRate, uint32 ClockRate);
		void	SetClockRate(int Rate);
		void	ClearBuffer();
		int		FinishBuffer(int t);
		int		ReadBuffer(int Size, void *Buffer, bool Stereo);
		int		SamplesAvail();

		void	AddSample(int ChanID, int Value);

		int32	GetChanOutput(uint8 Chan);

	private:
		inline double	CalcPin1(double Val1, double Val2);
		inline double	CalcPin2(double Val1, double Val2, double Val3);

		void			AddDelta(int AmpLeft, int AmpRight, int Time);

		bool			m_bVolRead;
		int				m_iSampleRate;

		stChanVal		Channels[CHANNELS];
		
		uint32			ExternalSamples;
		int32			ExternalSampleBuffer[1000];

		uint8			ExternalChip;
		double			MasterVol;
		double			InternalVol,
						MMC5Vol,
						VRC6Vol,
						VRC7Vol,
						FDSVol,
						N106Vol;
};

#endif /* _MIXER_H_ */