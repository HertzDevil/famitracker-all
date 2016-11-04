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

//
// This is the player handling code
//
// 050617: This is ripped from my NSF player and hacked together (it may look a bit dirty)
//

#include "stdafx.h"
#include <cstdio>
#include <windows.h>
#include "player.h"
#include "player.h"
#include "apu.h"
#include "resource.h"

const int CPlayer::FRAME_CYCLES_PAL		= (1264 / 12) * 315;
const int CPlayer::FRAME_CYCLES_NTSC	= (1364 / 12) * 262;
const int CPlayer::MAX_FRAMES_INIT		= 10;					// Max amount of allowed time for song to init. If more is used, consider it hunged

extern bool DMC_Filter;

CPlayer::CPlayer()
{
	APU				= NULL;
	AccumBuffer		= NULL;
	BufferPtr		= 0;
}

CPlayer::~CPlayer()
{
}

bool CPlayer::Init(CSampleMem *pSampleMem)
{
	// Init a player
	//

	APU	= new CAPU;

	APU->Init(this, pSampleMem);

	PlaybackSpeed	= SPEED_NTSC;
	SpeedSetting	= 0;

	SetSpeed(SPEED_AUTO);

	return true;
}

void CPlayer::Shutdown()
{
	// Shutdown everything, player will close
	//

	APU->Shutdown();

	delete APU;
}

/////////////////////////////////////////////

bool CPlayer::SetupSound(uint32 BlockLength, uint32 SampleRate, uint32 SampleSize, uint32 Channels)
{
	BlockSamples = BlockLength;
	BufferPtr = 0;

	if (AccumBuffer) {
		delete [] AccumBuffer;
		AccumBuffer = NULL;
	}
	
	int Speed = SPEED_NTSC;

	if (!APU->AllocateBuffer(SampleRate, Channels, PlaybackSpeed))
		return false;
		
	AccumBuffer = new int32[BlockLength];
	
	// Update blip-buffer filtering 
//	APU->SetupMixer();

	return true;
}

void CPlayer::SetupMixer(int LowCut, int HighCut, int HighDamp)
{
	APU->SetupMixer(LowCut, HighCut, HighDamp);
}

//// General functions ///////////////////////////////////////////////////////////////////////////////////////////////

void CPlayer::SetSpeed(int Speed)
{
	SpeedSetting = Speed;

	SetupSpeed();
}

void CPlayer::SetupSpeed()
{
	// Setup the player speed, can be changed during playback
	//
	// If bit 2 of NTSC/PAL flags is set the player will restart the song
	//

	switch (SpeedSetting) {
		case SPEED_NTSC: PlaybackSpeed = SPEED_NTSC; break;
		case SPEED_PAL:	PlaybackSpeed = SPEED_PAL; break;
	}

	if (PlaybackSpeed == SPEED_NTSC)
		FrameCycles = FRAME_CYCLES_NTSC;

	else if (PlaybackSpeed == SPEED_PAL)
		FrameCycles = FRAME_CYCLES_PAL;

	APU->ChangeSpeed(PlaybackSpeed);
	APU->ProduceSamples(true);
}


bool CPlayer::InitSong()
{
	// Init the song
	//

	NeedSongInit = false;
	
	APU->Reset();
	
	return true;
}

void CPlayer::Write(uint16 Address, uint8 Value)
{
	if (Address == 0x4017) {
		APU->Write4017(Value);
	}
	else if (Address == 0x4015) {
		APU->WriteControl(Value);
	}
	else {
		APU->Write(Address, Value);
	}
}

void CPlayer::Reset()
{
	APU->Reset();
	
}

bool CPlayer::PlaySong(uint32 Cycles)
{
	// Called constantly when a song is playing
	//
	// Return false if some error occured
	//
	
	APU->Run(Cycles);

	return true;
}

void CPlayer::FlushAudioBuffer(int32 *Buffer, uint32 Samples)
{
	// Transfer the buffer of sound
	//
	// Buffer	- Pointer to a buffer containing 32 bit samples
	// Samples	- Amount of samples in the buffer
	//
	
	for (uint32 i = 0; i < Samples; i++) {
		
		AccumBuffer[BufferPtr++] = Buffer[i];
		
		if (BufferPtr == BlockSamples) {
			FlushSoundBuffer(AccumBuffer, BufferPtr);
			BufferPtr = 0;
		}
	}
}

int32 CPlayer::GetChanVol(uint8 Chan)
{
	return APU->GetVol(Chan);
}