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

#ifndef _DSOUND_H_
#define _DSOUND_H_

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

class CDSoundChannel
{
	friend class CDSound;

	public:
		void Play();
		void Stop();
		void Pause();
		void Clear();
		void Reset();
		void WriteSoundBuffer(void *Buffer, uint32 Samples);
		bool IsInSync();

		void SetNotification();
		void ResetNotification();
		int	 WaitForDirectSoundEvent();

		int GetWriteBlock();
		int GetPlayBlock();

		int GetBlockSize()		{ return BlockSize;		};
		int GetBlockSamples()	{ return BlockSize >> ((SampleSize >> 3) - 1); };
		int GetBlocks()			{ return Blocks;		};
		int	GetBufferLength()	{ return BufferLength;	};
		int GetSampleSize()		{ return SampleSize;	};
		int	GetSampleRate()		{ return SampleRate;	};
		int GetChannels()		{ return Channels;		};

		int GetBlock();

		int GetPlayPos();

		bool IsStartSync() { return StartSync; };

		int		Underruns;

	private:
		LPDIRECTSOUNDBUFFER		lpDirectSoundBuffer;
		LPDIRECTSOUNDNOTIFY		lpDirectSoundNotify;

		HANDLE	hNotification;
		HWND	hWndTarget;

		int		SampleSize;
		int		SampleRate;
		int		Blocks;
		int		Channels;
		int		BufferLength;			// in ms
		int		LastWriteBlock;

		bool	InSync;
		bool	StartSync;
		bool	SyncProblem;
		bool	ManualEvent;

		uint32	SoundBufferSize;		// in bytes
		uint32	BlockSize;				// in bytes
		uint8	CurrentWriteBlock;
};

class CDSound
{
	public:
		bool Init(HWND hWnd, HANDLE hNotification);

		CDSoundChannel *OpenChannel(int SampleRate, int SampleSize, int Channels, int BufferLength, int Blocks);
//		CDSoundChannel *OpenChannel(stSoundSettings *Settings);
		void			CloseChannel(CDSoundChannel *Channel);

	private:

		static const int MAX_BLOCKS;

		HWND			hWndTarget;
		HANDLE			hNotificationHandle;
		LPDIRECTSOUND	lpDirectSound;

};

#endif /* _DSOUND_H_ */