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

#ifndef _DSOUND_H_
#define _DSOUND_H_

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#define EVENT_FLAG	1
#define NO_SYNC		2
#define	IN_SYNC		3

class CDSoundChannel {
friend class CDSound;

public:
	CDSoundChannel();
	~CDSoundChannel();

	void Play();
	void Stop();
	void Pause();
	void Clear();
	void Reset();
	void WriteSoundBuffer(void *Buffer, unsigned int Samples);
	int	 WaitForDirectSoundEvent();
	bool IsPlaying() const;

	int GetBlockSize()		{ return BlockSize;		};
	int GetBlockSamples()	{ return BlockSize >> ((SampleSize >> 3) - 1); };
	int GetBlocks()			{ return Blocks;		};
	int	GetBufferLength()	{ return BufferLength;	};
	int GetSampleSize()		{ return SampleSize;	};
	int	GetSampleRate()		{ return SampleRate;	};
	int GetChannels()		{ return Channels;		};

private:
	int GetWriteBlock();
	int GetPlayBlock();

private:
	LPDIRECTSOUNDBUFFER	lpDirectSoundBuffer;
	LPDIRECTSOUNDNOTIFY	lpDirectSoundNotify;

	HANDLE	hEventList[2];
	HWND	hWndTarget;

	int		SampleSize, SampleRate, Channels;
	int		BufferLength, Blocks;			// buffer length in ms
	int		LastWriteBlock;

	unsigned int	SoundBufferSize;		// in bytes
	unsigned int	BlockSize;				// in bytes
	unsigned char	CurrentWriteBlock;
};

class CDSound {
public:
	CDSound();
	~CDSound();

	bool Init(HWND hWnd, HANDLE hNotification, int Device);
	void Close();

	CDSoundChannel	*OpenChannel(int SampleRate, int SampleSize, int Channels, int BufferLength, int Blocks);
	void			CloseChannel(CDSoundChannel *Channel);

	void			EnumerateDevices();
	void			ClearEnumeration();
	void			EnumerateCallback(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext);
	unsigned int	GetDeviceCount();
	char			*GetDeviceName(int iDevice);
	int				MatchDeviceID(char *Name);
	GUID			GetDeviceID(int iDevice);

private:
	const static unsigned int MAX_DEVICES = 256;
	static const int MAX_BLOCKS;

	HWND			hWndTarget;
	HANDLE			hNotificationHandle;
	LPDIRECTSOUND	lpDirectSound;

	unsigned int	m_iDevices;
	char			*m_pcDevice[MAX_DEVICES];
	GUID			*m_pGUIDs[MAX_DEVICES];
};

#endif /* _DSOUND_H_ */