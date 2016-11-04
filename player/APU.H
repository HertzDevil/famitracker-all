/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005  Jonathan Liss
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

#ifndef _APU_H_
#define _APU_H_

#include "common.h"
#include "mixer.h"

const int SNDCHIP_NONE	= 0;
const int SNDCHIP_VRC6	= 1;			// Konami VRCVI
const int SNDCHIP_VRC7	= 2;			// Konami VRCVII
const int SNDCHIP_FDS	= 4;			// Famicom Disk Sound
const int SNDCHIP_MMC5	= 8;			// Nintendo MMC5
const int SNDCHIP_N106	= 16;			// Namco N-106
const int SNDCHIP_FME07	= 32;			// Sunsoft FME-07

class ICallback;
class CSampleMem;

class CSquare;
class CTriangle;
class CNoise;
class CDPCM;

class CAPU
{
	public:
		CAPU();
		~CAPU();

		bool					Init(ICallback *pCallback, CSampleMem *pSampleMem);
		void					Shutdown();
		void					Halt();
		void					Reset();
		void					Process();
		void					AddCycles(uint32 Cycles);
		void					EndFrame();
		
		void					Run(uint32 Cycles);

		uint8					ReadControl();
		void					Write4017(uint8 Value);
		void					WriteControl(uint8 Value);
		void					Write(uint16 Address, uint8 Value);

		void					SetExternalSound(uint8 Chip);
		void					ExternalWrite(uint16 Address, uint8 Value);
		uint8					ExternalRead(uint16 Address);
		
		void					ChangeSpeed(int Speed);

		bool					AllocateBuffer(int SampleRate, int NrChannels, int Speed);
		void					ReleaseBuffer();
		void					ClearBuffer();

		void					SetupMixer(int LowCut, int HighCut, int HighDamp);

		int32					GetVol(uint8 Chan);

		static const uint8		DUTY_PULSE[];
		static const uint8		LENGTH_TABLE[];
		static const uint16		NOISE_FREQ[];
		static const uint16		DMC_FREQ_NTSC[];
		static const uint16		DMC_FREQ_PAL[];

		static const double		BASE_FREQ_NTSC;
		static const double		BASE_FREQ_PAL;

		static const uint8		FRAME_RATE_NTSC;
		static const uint8		FRAME_RATE_PAL;

	private:
		static const int		SEQUENCER_PERIOD;
		
		inline void				Clock_240Hz();
		inline void				Clock_120Hz();
		inline void				Clock_60Hz();
		inline void				ClockSequence();
		
		CMixer					*Mixer;

		CSquare					*SquareCh1;
		CSquare					*SquareCh2;
		CTriangle				*TriangleCh;
		CNoise					*NoiseCh;
		CDPCM					*DPCMCh;
		
		ICallback				*Parent;
		//CPU						*CPU;

		uint8					ExternalSoundChip;					// External sound chip, if used

		uint8					SoundRegs[0x17];					// Internal APU regs ($4000-$4013)
		uint8					ControlReg;							// The $4015 reg

		uint32					FramePeriod;						// Cycles per frame
		uint32					FrameCycles;						// Cycles emulated from start of frame
		int32					ClockCycles;						// Cycles to emulate
		int32					FrameClock;							// Clock for frame sequencer
		uint8					FrameSequence;						// Frame sequence
		uint8					FrameMode;							// 4 or 5-steps frame sequence

		uint32					SampleSizeShift;					// To convert samples to bytes
		uint32					SoundBufferSize;					// Size of buffer, counting int32s
		uint32					SoundBufferSamples;					// Size of buffer, in samples
		uint32					BufferPointer;						// Fill pos in buffer
		int32					*SoundBuffer;						// Sound transfer buffer
		bool					StereoEnabled;						// If stereo is enabled

		bool					HaltEmulation;						// Stop emulation
};

#endif /* _APU_H_ */