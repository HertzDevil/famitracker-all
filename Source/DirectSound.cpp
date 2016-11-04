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

//
// DirectSound Interface
//
// Written for my NSF player
//
// By Jonathan Liss 2003
// zxy965r@tninet.se
//
//
// Notes
//
// The dsound play buffer notification is used to keep playback in sync.
// It has been somewhat complicated, but now I think it works fine.
//
// Playback starts automatic when the first wait for sync occur, 
// it shouldn't be started manually before that.
//
//

#include "stdafx.h"
#include <cstdio>
#include "common.h"
#include "DirectSound.h"
#include "resource.h"

//#define SYNC_MESSAGES

enum {
	MSG_DX_ERROR_INIT,
	MSG_DX_ERROR_BLOCKS,
	MSG_DX_ERROR_SAMPLERATE,
	MSG_DX_ERROR_LENGTH,
	MSG_DX_ERROR_BUFFER,
	MSG_DX_ERROR_QUERY,
	MSG_DX_ERROR_NOTIFICATION,
	MSG_DX_ERROR_PLAY,
	MSG_DX_ERROR_LOCK,
	MSG_DX_ERROR_UNLOCK

};

static const char *MESSAGE_TITLE = "DirectX Error";

static const char *DX_MESSAGES[] = {"Error: DirectSound initialization failed!",
		   							"DirectSound::OpenChannel - A maximum of %i blocks is allowed!",
									"DirectSound::OpenChannel - Sample rate above %i kHz is not supported!",
									"DirectSound::OpenChannel - Buffer length above %i seconds is not supported!",
									"DirectSound::OpenChannel - Initialization failed: Could not create the buffer, following error was returned: ",
									"DirectSound::OpenChannel - Initialization failed: Could not query IID_IDirectSoundNotify!",
									"DirectSound::OpenChannel - Initialization failed: Could not set notification positions!",
									"Error: Could not start playback of sound buffer",
									"Error: Could not lock sound buffer",
									"Error: Could not unlock sound buffer"};

const int CDSound::MAX_BLOCKS = 16;

static CDSound *pObject;

static int CalculateBufferLenght(int BufferLen, int Samplerate, int Samplesize, int Channels)
{
	// Calculate size of the buffer, in bytes
	return ((Samplerate * BufferLen) / 1000) * (Samplesize / 8) * Channels;
}

static BOOL CALLBACK DSEnumCallback(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext)
{
	pObject->EnumerateCallback(lpGuid, lpcstrDescription, lpcstrModule, lpContext);
	return TRUE;
}

CDSound::CDSound()
{
	m_iDevices = 0;
	pObject = this;
}

bool CDSound::Init(HWND hWnd, HANDLE hNotification, int Device)
{
	HRESULT hRes;
	
	hNotificationHandle = hNotification;
	hWndTarget			= hWnd;

	hRes = DirectSoundCreate((LPCGUID)m_pGUIDs[Device], &lpDirectSound, NULL);

	if FAILED(hRes) {
		MessageBox(hWnd, DX_MESSAGES[MSG_DX_ERROR_INIT], MESSAGE_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	hRes = lpDirectSound->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);

	if FAILED(hRes) {
		MessageBox(hWnd, DX_MESSAGES[MSG_DX_ERROR_INIT], MESSAGE_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	
	return true;
}

void CDSound::Close()
{
	lpDirectSound->Release();
}

void CDSound::ClearEnumeration()
{
	for (unsigned int i = 0; i < m_iDevices; i++)
		delete [] m_pcDevice[i];

	m_iDevices = 0;
}

void CDSound::EnumerateCallback(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext)
{
	m_pcDevice[m_iDevices] = new char[strlen(lpcstrDescription) + 1];
	strcpy(m_pcDevice[m_iDevices], lpcstrDescription);

	if (lpGuid != NULL) {
		m_pGUIDs[m_iDevices] = new GUID;
		memcpy(m_pGUIDs[m_iDevices], lpGuid, sizeof(GUID));
	}
	else
		m_pGUIDs[m_iDevices] = NULL;

	m_iDevices++;
}

void CDSound::EnumerateDevices()
{
	if (m_iDevices != 0)
		ClearEnumeration();

	DirectSoundEnumerate(DSEnumCallback, NULL);
}

unsigned int CDSound::GetDeviceCount()
{
	return m_iDevices;
}

char *CDSound::GetDeviceName(int iDevice)
{
	return m_pcDevice[iDevice];
}

int CDSound::MatchDeviceID(char *Name)
{
	for (unsigned int i = 0; i < m_iDevices; i++) {
		if (!strcmp(Name, m_pcDevice[i]))
			return i;
	}

	return 0;
}

CDSoundChannel *CDSound::OpenChannel(int SampleRate, int SampleSize, int Channels, int BufferLength, int Blocks)
{
	// Open a new secondary buffer
	//

	DSBPOSITIONNOTIFY	PositionNotify[MAX_BLOCKS];
	WAVEFORMATEX		SoundFormat;
	DSBUFFERDESC		BufferDesc;
	HRESULT				hRes;

	CDSoundChannel		*Channel;
	int					SoundBufferSize;
	int					BlockSize;
	
	char				Text[256];

	if (Blocks > MAX_BLOCKS) {
		sprintf(Text, DX_MESSAGES[MSG_DX_ERROR_BLOCKS], MAX_BLOCKS);
		MessageBox(hWndTarget, Text, MESSAGE_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return NULL;
	}

	if (SampleRate > 96000) {
		sprintf(Text, DX_MESSAGES[MSG_DX_ERROR_SAMPLERATE], 96);
		MessageBox(hWndTarget, Text, MESSAGE_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return NULL;
	}

	if (BufferLength > 10000) {
		sprintf(Text, DX_MESSAGES[MSG_DX_ERROR_LENGTH], 10);
		MessageBox(hWndTarget, Text, MESSAGE_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return NULL;
	}

	if (Blocks == 0)
		return 0;

	// Adjust buffer length in case a buffer would end up in half samples

	while ((SampleRate * BufferLength / (Blocks * 1000) != (double)SampleRate * BufferLength / (Blocks * 1000)))
		BufferLength++;
 
	Channel = new CDSoundChannel;
	
	SoundBufferSize = CalculateBufferLenght(BufferLength, SampleRate, SampleSize, Channels);
	BlockSize		= SoundBufferSize / Blocks;
	
	Channel->BufferLength		= BufferLength;			// in ms
	Channel->SoundBufferSize	= SoundBufferSize;		// in bytes
	Channel->BlockSize			= BlockSize;			// in bytes
	Channel->Blocks				= Blocks;
	Channel->SampleSize			= SampleSize;
	Channel->SampleRate			= SampleRate;
	Channel->Channels			= Channels;

	Channel->CurrentWriteBlock	= 0;
	Channel->Underruns			= 0;

	Channel->hNotification		= hNotificationHandle;
	Channel->hWndTarget			= hWndTarget;

	memset(&SoundFormat, 0x00, sizeof(WAVEFORMATEX));
	SoundFormat.cbSize			= sizeof(WAVEFORMATEX);
	SoundFormat.nChannels		= Channels;
	SoundFormat.nSamplesPerSec	= SampleRate;
	SoundFormat.wBitsPerSample	= SampleSize;
	SoundFormat.nBlockAlign		= SoundFormat.nChannels * (SampleSize / 8);
	SoundFormat.nAvgBytesPerSec = SoundFormat.nSamplesPerSec * SoundFormat.nBlockAlign;
	SoundFormat.wFormatTag		= WAVE_FORMAT_PCM;

	memset(&BufferDesc, 0x00, sizeof(DSBUFFERDESC));
	BufferDesc.dwSize			= sizeof(DSBUFFERDESC);
	BufferDesc.dwBufferBytes	= SoundBufferSize;
	BufferDesc.dwFlags			= DSBCAPS_LOCSOFTWARE | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY;
	BufferDesc.lpwfxFormat		= &SoundFormat;

	for (int i = 0; i < Blocks; i++) {
		PositionNotify[i].dwOffset		= i * BlockSize;
		PositionNotify[i].hEventNotify	= hNotificationHandle;
	}
	
	hRes = lpDirectSound->CreateSoundBuffer(&BufferDesc, &Channel->lpDirectSoundBuffer, NULL);

	if FAILED(hRes) {
		char ErrText[256];

		strcpy(ErrText, DX_MESSAGES[MSG_DX_ERROR_BUFFER]);

		switch (hRes) {			
			case DSERR_ALLOCATED:		strcat(ErrText, "DSERR_ALLOCATED");			break;
			case DSERR_BADFORMAT:		strcat(ErrText, "DSERR_BADFORMAT");			break;
			case DSERR_BUFFERTOOSMALL:	strcat(ErrText, "DSERR_BUFFERTOOSMALL");	break;
			case DSERR_CONTROLUNAVAIL:	strcat(ErrText, "DSERR_CONTROLUNAVAIL");	break;
			case DSERR_DS8_REQUIRED:	strcat(ErrText, "DSERR_DS8_REQUIRED");		break;
			case DSERR_INVALIDCALL:		strcat(ErrText, "DSERR_INVALIDCALL");		break;
			case DSERR_INVALIDPARAM:	strcat(ErrText, "DSERR_INVALIDPARAM");		break;
			case DSERR_NOAGGREGATION:	strcat(ErrText, "DSERR_NOAGGREGATION");		break;
			case DSERR_OUTOFMEMORY:		strcat(ErrText, "DSERR_OUTOFMEMORY");		break;
			case DSERR_UNINITIALIZED:	strcat(ErrText, "DSERR_UNINITIALIZED");		break;
			case DSERR_UNSUPPORTED:		strcat(ErrText, "DSERR_UNSUPPORTED");		break;
				break;
		}

		MessageBox(hWndTarget, ErrText, MESSAGE_TITLE, MB_OK | MB_ICONEXCLAMATION);

		delete Channel;
		return NULL;
	}
	
	hRes = Channel->lpDirectSoundBuffer->QueryInterface(IID_IDirectSoundNotify, (void**)&Channel->lpDirectSoundNotify);

	if FAILED(hRes) {
		MessageBox(hWndTarget, DX_MESSAGES[MSG_DX_ERROR_QUERY], MESSAGE_TITLE, MB_OK | MB_ICONEXCLAMATION);
		delete Channel;
		return NULL;
	}

	hRes = Channel->lpDirectSoundNotify->SetNotificationPositions(Blocks, PositionNotify);

	if FAILED(hRes) {
		MessageBox(hWndTarget, DX_MESSAGES[MSG_DX_ERROR_NOTIFICATION], MESSAGE_TITLE, MB_OK | MB_ICONEXCLAMATION);
		delete Channel;
		return NULL;
	}
	
	Channel->Clear();
	
	return Channel;
}

void CDSound::CloseChannel(CDSoundChannel *Channel)
{
	Channel->lpDirectSoundBuffer->Release();
	Channel->lpDirectSoundNotify->Release();
}

// CDSoundChannel

void CDSoundChannel::Play()
{
	// Begin playback of buffer
	HRESULT hRes;
	hRes = lpDirectSoundBuffer->Play(NULL, NULL, DSBPLAY_LOOPING);

	if FAILED(hRes) {
		MessageBox(NULL, DX_MESSAGES[MSG_DX_ERROR_PLAY], MESSAGE_TITLE, 0);
		return;
	}
}

void CDSoundChannel::Stop()
{
	// Stop playback
	lpDirectSoundBuffer->Stop();
	Reset();
}

void CDSoundChannel::Pause()
{
	// Pause playback of buffer, doesn't reset cursors
	lpDirectSoundBuffer->Stop();
}

void CDSoundChannel::Clear()
{
	DWORD	*AudioPtr1;
	DWORD	*AudioPtr2;
	DWORD	AudioBytes1;
	DWORD	AudioBytes2;
	HRESULT	hRes;
	DWORD	Status;

	lpDirectSoundBuffer->GetStatus(&Status);

	// Clearing a buffer will stop playback.
	if (Status & DSBSTATUS_PLAYING)
		Stop();

	lpDirectSoundBuffer->SetCurrentPosition(0);

	hRes = lpDirectSoundBuffer->Lock(0, SoundBufferSize, (void**)&AudioPtr1, &AudioBytes1, (void**)&AudioPtr2, &AudioBytes2, 0);
	
	if FAILED(hRes) {
		MessageBox(hWndTarget, DX_MESSAGES[MSG_DX_ERROR_LOCK], MESSAGE_TITLE, MB_OK);
		return;
	}

	if (SampleSize == 8) {
		memset(AudioPtr1, 0x80, AudioBytes1);

		if (AudioPtr2)
			memset(AudioPtr2, 0x80, AudioBytes2);
	}
	else {
		memset(AudioPtr1, 0x0000, AudioBytes1);

		if (AudioPtr2)
			memset(AudioPtr2, 0x0000, AudioBytes2);
	}

	hRes = lpDirectSoundBuffer->Unlock((void*)AudioPtr1, AudioBytes1, (void*)AudioPtr2, AudioBytes2);

	if FAILED(hRes) {
		MessageBox(hWndTarget, DX_MESSAGES[MSG_DX_ERROR_UNLOCK], MESSAGE_TITLE, MB_OK);
		return;
	}

	lpDirectSoundBuffer->SetCurrentPosition(0);
}

void CDSoundChannel::WriteSoundBuffer(void *Buffer, unsigned int Samples)
{
	// Fill sound buffer
	//
	// Buffer	- Pointer to a buffer with samples
	// Samples	- Number of samples, in bytes
	//

	HRESULT	hRes;
	DWORD	*AudioPtr1;
	DWORD	*AudioPtr2;
	DWORD	AudioBytes1;
	DWORD	AudioBytes2;
	DWORD	CurrentBlock;

	if (BlockSize != Samples)
		return;

	CurrentBlock = CurrentWriteBlock;

	hRes = lpDirectSoundBuffer->Lock(CurrentWriteBlock * BlockSize, BlockSize, (void**)&AudioPtr1, &AudioBytes1, (void**)&AudioPtr2, &AudioBytes2, 0);

	// Could not lock
	if (FAILED(hRes))
		return;

	LastWriteBlock = CurrentWriteBlock;
	CurrentWriteBlock = (CurrentWriteBlock + 1) % Blocks;

	memcpy(AudioPtr1, Buffer, AudioBytes1);

	if (AudioPtr2)
		memcpy(AudioPtr2, (unsigned char*)Buffer + AudioBytes1, AudioBytes2);

	lpDirectSoundBuffer->Unlock((void*)AudioPtr1, AudioBytes1, (void*)AudioPtr2, AudioBytes2);
}

void CDSoundChannel::Reset()
{
	// Reset playback from the beginning of the buffer

	InSync		= false;
	StartSync	= true;
	SyncProblem	= false;
	ManualEvent = false;

	CurrentWriteBlock = 0;
	lpDirectSoundBuffer->SetCurrentPosition(0);
	ResetEvent(hNotification);
}

void CDSoundChannel::ResetNotification()
{
	ManualEvent = false;
	ResetEvent(hNotification);
}

void CDSoundChannel::SetNotification()
{
	ManualEvent = true;
	SetEvent(hNotification);
}

bool CDSoundChannel::IsInSync()
{
	return InSync;
}

int CDSoundChannel::GetBlock()
{
	return LastWriteBlock;
}

int CDSoundChannel::WaitForDirectSoundEvent()
{
	// Wait for a DirectSound event
	//
	// This part will wait until the read pointer has passed any
	// of the specified positions in the buffer.
	// To avoid a delay when sending messages to the player thread,
	// a message should also be sent to this thread.
	//
	// The meaing is simply to keep the playback speed in sync
	//
	// Return value is 1 if an manual event was set, and 0 if it was a
	// direct sound event
	//

	static bool Synced = false;

	DWORD	dwResult;
	DWORD	WriteBlock;

	DWORD PlayPos;
	DWORD WritePos;

	WriteBlock = GetWriteBlock();

	// First sync is special, skip notification when a song just have started
	// The buffer needs to be filled before synchronisation can start
	if (StartSync) {

#ifdef SYNC_MESSAGES
//		PrintDebug("start sync\n", CurrentWriteBlock, WriteBlock, Synced ? "sync" : "no sync");
#endif

		// Start playback
		Clear();
		Play();

		ResetNotification();		// Make sure no notifications is set at the beginning, that would mess the sync.

		CurrentWriteBlock = 0;

		StartSync	= false;
		Synced		= false;
	}
	else {
		if (CurrentWriteBlock == WriteBlock)
			Synced = true;							// synchronized
		else
			Synced = false;							// Not synchronized
	}

#ifdef SYNC_MESSAGES
//	PrintDebug("internal %i - dsound %i %s - ", CurrentWriteBlock, WriteBlock, Synced ? "sync" : "no sync");
#endif

	SyncProblem = false;

	// If not synced, quit and play immediately
	if (!Synced) {
		if (InSync == true) {
			SyncProblem = true;
#ifdef SYNC_MESSAGES
//			PrintDebug("sync problem - ");
#endif
		}
	}
	
	// Save sync flag to a global variable
	InSync = Synced;

	int Signaled = 0;

	if (WaitForSingleObject(hNotification, 0) == WAIT_OBJECT_0) {
		SetEvent(hNotification);
		Signaled = 1;
	}

#ifdef SYNC_MESSAGES
//	PrintDebug("event %i - ", Signaled);
#endif
	
	// The buffer is out of sync, do not wait for the notification
	if (!Synced) {

		// Reset any notifications that may have been set by direct sound
		ResetEvent(hNotification);

#ifdef SYNC_MESSAGES
//		PrintDebug("no event signal, block %i\n", GetWriteBlock());
#endif

		return 0;
	}

	// Loop until the direct sound write cursor and the fill block isn't the same
	while (WriteBlock == CurrentWriteBlock) {

		// Wait for the DirectSound signal of a part of the buffer has been played, or, another event like stop playing
		dwResult = WaitForSingleObject(hNotification, INFINITE);

		// Manual set event, the player thread has a task to do. Skip notification
		if (ManualEvent) {
			ManualEvent = false;
#ifdef SYNC_MESSAGES
//			PrintDebug("manual event signal\n");
#endif
			return 1;
		}

		WriteBlock = GetWriteBlock();

		if (WriteBlock == CurrentWriteBlock) {
#ifdef SYNC_MESSAGES
//			PrintDebug("bad event - ");
#endif
			Underruns++;
		}
	}

	// Event has been set.
	// It's now safe to write to the currently selected write block

#ifdef SYNC_MESSAGES
//	PrintDebug("event signal, block %i, expected %i\n", WriteBlock, (CurrentWriteBlock + 1) % Blocks);
#endif

	lpDirectSoundBuffer->GetCurrentPosition(&PlayPos, &WritePos);

	// Update directsound dialog
//	DSoundStatDlgSetBuf(PlayPos, WritePos, SoundBufferSize, BufferLength, Underruns);

	return 0;
}

/*

   The sound buffer is splitted into blocks (here eight)
   Notification positions is at the end of each block

  0    1    2    3    4    5    6    7
  |----|----|----|----|----|----|----|----|

       ^   ^
       |   |
       |  Play
       |   cursor
       |
      Fill
       block

	   One more block will be filled every time 
	   the play cursor has passed a new block.
	   Optimally, the fill cursor will always be
	   one block behind the play cursor
  
*/

int CDSoundChannel::GetWriteBlock()
{
	// Return the block where the write pos is

	DWORD PlayPos;
	DWORD WritePos;

	lpDirectSoundBuffer->GetCurrentPosition(&PlayPos, &WritePos);

	return (WritePos / BlockSize);
}

int CDSoundChannel::GetPlayBlock()
{
	// Return the block where the play pos is

	DWORD PlayPos;
	DWORD WritePos;

	lpDirectSoundBuffer->GetCurrentPosition(&PlayPos, &WritePos);

	return (PlayPos / BlockSize);
}

int CDSoundChannel::GetPlayPos()
{
	DWORD PlayPos;
	DWORD WritePos;

	lpDirectSoundBuffer->GetCurrentPosition(&PlayPos, &WritePos);

	return PlayPos;
}
