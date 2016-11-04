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
// The NES APU (Audio Processing Unit) emulation core
//
// Written by Jonathan Liss 2002 - 2003
//
//

#include "stdafx.h"
#include <cstdio>
#include <memory>
#include "FamiTracker.h"
#include "apu/apu.h"
#include "apu/square.h"
#include "apu/triangle.h"
#include "apu/noise.h"
#include "apu/dpcm.h"

const int		CAPU::SEQUENCER_PERIOD		= 14915;
const double	CAPU::BASE_FREQ_NTSC		= 1789772.7272;
const double	CAPU::BASE_FREQ_PAL			= 1662607.0;
const uint8		CAPU::FRAME_RATE_NTSC		= 60;
const uint8		CAPU::FRAME_RATE_PAL		= 50;

const uint8		CAPU::DUTY_PULSE[]			= {0x0E, 0x0C, 0x08, 0x04};

const uint8		CAPU::LENGTH_TABLE[]		= {0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06,
											   0xA0, 0x08, 0x3C, 0x0A, 0x0E, 0x0C, 0x1A, 0x0E,
											   0x0C, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
											   0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E};

const uint16	CAPU::NOISE_FREQ[]			= {0x04, 0x08, 0x10, 0x20, 0x40, 0x60, 0x80, 0xA0, 
											   0xCA, 0xFE, 0x17C, 0x1FC, 0x2FA, 0x3F8, 0x7F2, 0xFE4};

const uint16	CAPU::DMC_FREQ_NTSC[]		= {428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 85, 72, 54};
const uint16	CAPU::DMC_FREQ_PAL[]		= {398, 354, 316, 298, 266, 236, 210, 199, 177, 149, 132, 119, 99, 79, 67, 50};

CAPU::CAPU() 
{
}

CAPU::~CAPU() 
{
}

inline void CAPU::Clock_240Hz()
{
	// 240Hz Framecounter (1/4 frame)
	//

	SquareCh1->EnvelopeUpdate();
	SquareCh2->EnvelopeUpdate();
	NoiseCh->EnvelopeUpdate();

	TriangleCh->LinearCounterUpdate();
}

inline void CAPU::Clock_120Hz()
{
	// 120Hz Framecounter (1/2 frame)
	//

	SquareCh1->SweepUpdate1();
	SquareCh2->SweepUpdate2();

	SquareCh1->LengthCounterUpdate();
	SquareCh2->LengthCounterUpdate();
	TriangleCh->LengthCounterUpdate();
	NoiseCh->LengthCounterUpdate();

}

inline void CAPU::Clock_60Hz()
{
	// 60Hz Framecounter (1/1 frame)
	//

	// No IRQs are generated for NSFs
}

inline void CAPU::ClockSequence()
{
	// The frame sequencer
	//

	if (FrameMode == 0) {
		switch (FrameSequence) {
			case 0: Clock_240Hz(); break;
			case 1: Clock_240Hz(); Clock_120Hz(); break;
			case 2: Clock_240Hz(); break;
			case 3: Clock_240Hz(); Clock_120Hz(); Clock_60Hz(); break;
		}
		FrameSequence = (FrameSequence + 1) % 4;
	}
	else {
		switch (FrameSequence) {
			case 0: Clock_240Hz(); Clock_120Hz(); break;
			case 1: Clock_240Hz(); break;
			case 2: Clock_240Hz(); Clock_120Hz(); break;
			case 3: Clock_240Hz(); break;
			case 4: break;
		}
		FrameSequence = (FrameSequence + 1) % 5;
	}
}

bool CAPU::Init(ICallback *pCallback, CSampleMem *pSampleMem)
{
	// Init APU code
	//

	Parent		= pCallback;
	
	Mixer		= new CMixer();

	SquareCh1	= new CSquare(Mixer, CHANID_SQUARE1);
	SquareCh2	= new CSquare(Mixer, CHANID_SQUARE2);
	TriangleCh	= new CTriangle(Mixer, CHANID_TRIANGLE);
	NoiseCh		= new CNoise(Mixer, CHANID_NOISE);
	DPCMCh		= new CDPCM(Mixer, CHANID_DPCM);
	
	VRC6		= new CVRC6;

	SoundBuffer	= NULL;
	
	DPCMCh->Init(pSampleMem);
	DPCMCh->SetSpeed(SPEED_NTSC);

	VRC6->Init(Mixer);
/*
	MMC5.Init(Mixer);
	N106.Init(Mixer);
	FDS.Init(Mixer);
	VRC7.Init(Mixer);
*/
	Mixer->Init();

	FrameCycles = 0;

	return true;
}

void CAPU::Shutdown()
{
	// Delete allocated memory
	//

	delete SquareCh1;
	delete SquareCh2;
	delete TriangleCh;
	delete NoiseCh;
	delete DPCMCh;

	delete VRC6;

	delete Mixer;

	ReleaseBuffer();
}

void CAPU::Process()
{
	// The main APU emulation
	//
	// The amount of cycles that will be emulated is added from CAPU::AddCycles
	//
	
	static uint32 Time;

	ClockCycles <<= 1;

	while (ClockCycles > 0 && !HaltEmulation) {

		if (ClockCycles > FrameClock)
			Time = FrameClock;
		else
			Time = ClockCycles;

		ClockCycles -= Time;
		FrameClock	-= Time;

		if (FrameClock <= 0) {
			FrameClock += SEQUENCER_PERIOD;
			ClockSequence();
		}

		SquareCh1->Process(Time >> 1);
		SquareCh2->Process(Time >> 1);
		TriangleCh->Process(Time >> 1);
		NoiseCh->Process(Time >> 1);
		DPCMCh->Process(Time >> 1);

		switch (ExternalSoundChip) {
			case SNDCHIP_VRC6:	VRC6->Process(Time >> 1);	break;
		/*
			case SNDCHIP_MMC5:	MMC5.Process(ClockCycles);	break;
			case SNDCHIP_N106:	N106.Process(ClockCycles);	break;
			case SNDCHIP_FDS:	FDS.Process(ClockCycles);	break;
			case SNDCHIP_VRC7:	VRC7.Process(ClockCycles);	break;
		*/
		}

		FrameCycles += (Time >> 1);

		if (FrameCycles >= FrameLength)
			EndFrame();
	}
}

void CAPU::Run(uint32 Cycles)
{
	ClockCycles += Cycles;
	Process();
}

void CAPU::EndFrame()
{
	// End of audio frame, flush the buffer if enough samples has been produced, and start a new frame
	//

	// The APU will always output audio in 32 bit signed format (not anymore)

	// Update (060122-0039): Changed it to always output all available samples

	int ReadSamples, SamplesAvail;
	
	SamplesAvail = Mixer->FinishBuffer(FrameCycles);

	ReadSamples	= Mixer->ReadBuffer(SamplesAvail, SoundBuffer, StereoEnabled);

	Parent->FlushBuffer(SoundBuffer, ReadSamples /*<< SampleSizeShift*/);

	SquareCh1->EndFrame();
	SquareCh2->EndFrame();
	TriangleCh->EndFrame();
	NoiseCh->EndFrame();
	DPCMCh->EndFrame();

	switch (ExternalSoundChip) {
		case SNDCHIP_VRC6:	VRC6->EndFrame(); break;
/*
		case SNDCHIP_MMC5:	MMC5.EndFrame(); break;
		case SNDCHIP_N106:	N106.EndFrame(); break;
		case SNDCHIP_FDS:	FDS.EndFrame();	 break;
		case SNDCHIP_VRC7:	VRC7.EndFrame(); break;
*/
	}

	FrameCycles = 0;
}

void CAPU::Reset()
{
	// Reset APU
	//
	
	memset(SoundRegs, 0x00, 0x15);

	ClockCycles		= 0;
	FrameCycles		= 0;
	ControlReg		= 0;

	FrameClock		= SEQUENCER_PERIOD;
	FrameSequence	= 0;
	FrameMode		= 0;
	
	HaltEmulation	= false;

	SquareCh1->Reset();
	SquareCh2->Reset();
	TriangleCh->Reset();
	NoiseCh->Reset();
	DPCMCh->Reset();
	
	ClearBuffer();
	Mixer->ClearBuffer();
	
	switch (ExternalSoundChip) {
		case SNDCHIP_VRC6:	VRC6->Reset(); break;
		/*
		case SNDCHIP_MMC5:	MMC5.Reset(); break;
		case SNDCHIP_N106:	N106.Reset(); break;
		case SNDCHIP_FDS:	FDS.Reset();  break;
		case SNDCHIP_VRC7:	VRC6.Reset(); break;
		*/
	}
}

void CAPU::SetupMixer(int LowCut, int HighCut, int HighDamp, int Volume)
{
	Mixer->UpdateSettings(LowCut, HighCut, HighDamp, Volume);
}

void CAPU::SetExternalSound(uint8 Chip)
{
	ExternalSoundChip = Chip;
	Mixer->ExternalSound(Chip);
}

void CAPU::ChangeSpeed(int Speed)
{
	// Allow to change speed on the fly
	//

	if (Speed == SPEED_NTSC) {
		DPCMCh->SetSpeed(SPEED_NTSC);
		Mixer->SetClockRate((uint32)BASE_FREQ_NTSC);
		FrameLength = int32(BASE_FREQ_NTSC / 60);
	}
	else if (Speed == SPEED_PAL) {
		DPCMCh->SetSpeed(SPEED_PAL);
		Mixer->SetClockRate((uint32)BASE_FREQ_PAL);
		FrameLength = int32(BASE_FREQ_PAL / 50);
	}

}

bool CAPU::AllocateBuffer(int SampleRate, int NrChannels, int Speed)
{
	// Allocate a sound buffer
	//
	// Returns false if a buffer couldn't be allocated
	//
	
	uint32 BaseFreq;

	BufferPointer		= 0;
	SoundBufferSamples	= (SampleRate / (FRAME_RATE_PAL - 1));			// Samples / frame. Allocate for PAL, since it's more
	SoundBufferSize		= SoundBufferSamples * NrChannels;			// Total amount of samples to allocate

	SampleSizeShift		= (NrChannels == 2) ? 1 : 0;
	StereoEnabled		= (NrChannels == 2) ? true : false;

	if (Speed == SPEED_NTSC) {
		DPCMCh->SetSpeed(SPEED_NTSC);
		BaseFreq = (uint32)BASE_FREQ_NTSC;
	}
	else if (Speed == SPEED_PAL) {
		DPCMCh->SetSpeed(SPEED_PAL);
		BaseFreq = (uint32)BASE_FREQ_PAL;
	}
	else
		return false;

	Mixer->AllocateBuffer(SoundBufferSamples, SampleRate, BaseFreq);

	if (SoundBuffer != NULL) {
		delete [] SoundBuffer;
		SoundBuffer = NULL;
	}
	
	SoundBuffer = new int16[SoundBufferSize << 1];
	
	if (SoundBuffer == NULL) {
//		DisplayError("CAPU: Could not allocate memory for soundbuffer!");
		return false;
	}
	
//	VRC7.SetSampleSpeed(SampleRate, (Speed == SPEED_NTSC ? BASE_FREQ_NTSC : BASE_FREQ_PAL));

	ClearBuffer();

	return true;
}

void CAPU::ClearBuffer()
{
	// Clear the buffer
	//

//	assert(SoundBuffer != NULL);

	BufferPointer = 0;

	memset(SoundBuffer, 0, SoundBufferSize);
	Mixer->ClearBuffer();
}

void CAPU::ReleaseBuffer()
{
	if (SoundBuffer == NULL)
		return;

	delete [] SoundBuffer;
	SoundBuffer = NULL;
}

void CAPU::Halt()
{
	HaltEmulation = true;
}

void CAPU::AddCycles(uint32 Cycles)
{
	ClockCycles += Cycles;
}

void CAPU::Write(uint16 Address, uint8 Value)
{
	// Data was written to an APU register
	//

	Process();

	SoundRegs[Address - 0x4000] = Value;

	switch (Address)
	{
		case 0x4000:
		case 0x4001:
		case 0x4002:
		case 0x4003:
			SquareCh1->Write(Address & 0x03, Value);
			break;
		case 0x4004:
		case 0x4005:
		case 0x4006:
		case 0x4007:
			SquareCh2->Write(Address & 0x03, Value);
			break;
		case 0x4008:
		case 0x4009:
		case 0x400A:
		case 0x400B:
			TriangleCh->Write(Address & 0x03, Value);
			break;
		case 0x400C:
		case 0x400D:
		case 0x400E:
		case 0x400F:
			NoiseCh->Write(Address & 0x03, Value);
			break;
		case 0x4010:
		case 0x4011:
		case 0x4012:
		case 0x4013:
			DPCMCh->Write(Address & 0x03, Value);
			break;
		case 0x4015:
			WriteControl(Value);
			break;
		case 0x4017:
			Write4017(Value);
			break;
	}

}

void CAPU::Write4017(uint8 Value)
{
	// The $4017 control port
	//

	Process();

	FrameSequence = 0;

	if (Value & 0x80) 
		FrameMode = 1;
	else
		FrameMode = 0;

	FrameClock = SEQUENCER_PERIOD;
	ClockSequence();

	// IRQs are not generated when playing NSFs
}

void CAPU::ExternalWrite(uint16 Address, uint8 Value)
{
	// Data was written to an external sound chip (this doesn't really belong in the APU)
	//

	Process();

	switch (ExternalSoundChip) {
		case SNDCHIP_VRC6:	VRC6->Write(Address, Value); break;
/*		case SNDCHIP_MMC5:	MMC5.Write(Address, Value); break;
		case SNDCHIP_N106:	N106.Write(Address, Value); break;
		case SNDCHIP_FDS:	FDS.Write(Address, Value);	break;
		case SNDCHIP_VRC7:	VRC7.Write(Address, Value); break;
		*/
	}
}

uint8 CAPU::ExternalRead(uint16 Address)
{
	// Data read from an external chip
	//

	Process();
	switch (ExternalSoundChip) {
		case SNDCHIP_VRC6: break;
/*
		case SNDCHIP_MMC5: break;
		case SNDCHIP_N106: return N106.Read(Address);
		case SNDCHIP_FDS: break;
		case SNDCHIP_VRC7: break;
	*/}

	return 0;
}

void CAPU::WriteControl(uint8 Value)
{
	//  Sound Control ($4015)
	//

	Process();

	ControlReg = Value;

	SquareCh1->WriteControl(Value & 0x01);
	SquareCh2->WriteControl((Value >> 1) & 0x01);
	TriangleCh->WriteControl((Value >> 2) & 0x01);
	NoiseCh->WriteControl((Value >> 3) & 0x01);
	DPCMCh->WriteControl((Value >> 4) & 0x01);
}

uint8 CAPU::ReadControl()
{
	// Sound Control ($4015)
	//

	uint8 RetVal = 0;

	Process();

	RetVal |= SquareCh1->ReadControl();
	RetVal |= (SquareCh2->ReadControl() << 1);
	RetVal |= (TriangleCh->ReadControl() << 2);
	RetVal |= (NoiseCh->ReadControl() << 3);
	RetVal |= (DPCMCh->ReadControl() << 4);
	RetVal |= (DPCMCh->DidIRQ() << 7);
	
	return RetVal;
}

int32 CAPU::GetVol(uint8 Chan)
{
	return Mixer->GetChanOutput(Chan);
}

bool CAPU::IRQ()
{
	return DPCMCh->IRQ();
}
