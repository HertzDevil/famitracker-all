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

#ifndef _PLAYER_H_
#define _PLAYER_H_

#include <cstdio>

typedef unsigned	char		uint8;
typedef unsigned	short		uint16;
typedef unsigned	long		uint32;
typedef unsigned	__int64		uint64;
typedef signed		char		int8;
typedef signed		short		int16;
typedef signed		long		int32;
typedef signed		__int64		int64;

#define SAMPLES_IN_BYTES(x) (x << SampleSizeShift)

const int SPEED_AUTO	= 0;
const int SPEED_NTSC	= 1;
const int SPEED_PAL		= 2;

class CAPU;
/*
// class for simulating CPU memory, used by the DPCM channel
class CSampleMem
{
	public:
		uint8	Read(uint16 Address) {
			if (!Mem)
				return 0;
			return (Mem[Address - 0xC000]);
		};

		void	SetMem(char *Ptr, int Size) {
			Mem = (uint8*)Ptr;
			MemSize = Size;
		};
	private:
		uint8	*Mem;
		uint16	MemSize;
};
*/
class CPlayer
{
	public:
		CPlayer();
		~CPlayer();

		bool	Init(CSampleMem *);
		void	Shutdown();

		void	SetSpeed(int Speed);

		bool	PlaySong(uint32 Cycles);

		bool	SetupSound(uint32 BufferLength, uint32 SampleRate, uint32 SampleSize, uint32 Channels);
		void	SetupMixer(int LowCut, int HighCut, int HighDamp);

		void	Write(uint16 Addr, uint8 Value);
		void	Reset();

		void	FlushAudioBuffer(int32 *Buffer, uint32 Samples);

		int32	GetChanVol(uint8 Chan);

	private:
		static const int FRAME_CYCLES_PAL;
		static const int FRAME_CYCLES_NTSC;
		static const int MAX_FRAMES_INIT;
		
		void			SetupSpeed();
		bool			InitSong();

		// Sound components
		bool			StereoEnabled;						// If stereo is enabled
		int32			*AccumBuffer;
		uint32			BufferPtr;
		uint32			BlockSamples;

		// Player settings
		bool			FileLoaded;							// A file is properly loaded
		unsigned int	FileSize;							// File size
		char			FilePath[MAX_PATH];

		unsigned char	CurrentSong;						// Current selected song
		bool			NeedSongInit;						// When a new song has been selected, flag to run the init routine
		int				PlaybackSpeed;						// NTSC / PAL
		int				SpeedSetting;						// NTSC / PAL / Auto
		uint32			FrameCycles, FrameCyclesLeft;		// Clock cycles per frame, and counter

		// NES hardware stuff
		CAPU			*APU;								// Pointer to the APU object
};

void FlushSoundBuffer(int32 *Buffer, uint32 Samples);

#endif /* _PLAYER_H_ */
