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

//
// The NES APU (2A03/2A07) (Audio Processing Unit) emulation core
//
// Written by Jonathan Liss 2002 - 2003
//
// Briefly about how the audio stream is handled
//
//  To set up sound, call AllocateBuffer with desired sample rate, stereo and speed (NTSC/PAL).
//  At every end of frame, call EndFrame. It will then call the parent to flush the buffer with about one frame
//  worth of sound.
//  The sound is rendered in 32 bit signed format.
//  Speed may be changed during playback. No buffers needs to be reallocated.
//
// Mail: zxy965r@tninet.se
//
// Todo:
//

#include <cstdio>
#include <memory>
#include "APU/apu.h"
#include "APU/square.h"
#include "APU/triangle.h"
#include "APU/noise.h"
#include "APU/dpcm.h"

const int		CAPU::SEQUENCER_PERIOD		= 7458;
//const int		CAPU::SEQUENCER_PERIOD_PAL	= 7458;		// ????
const uint32	CAPU::BASE_FREQ_NTSC		= 1789773; // 772.7272...
const uint32	CAPU::BASE_FREQ_PAL			= 1662607;
const uint8		CAPU::FRAME_RATE_NTSC		= 60;
const uint8		CAPU::FRAME_RATE_PAL		= 50;

const uint8		CAPU::LENGTH_TABLE[]		= {0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06,
											   0xA0, 0x08, 0x3C, 0x0A, 0x0E, 0x0C, 0x1A, 0x0E,
											   0x0C, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
											   0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E};

CAPU::CAPU(ICallback *pCallback, CSampleMem *pSampleMem)
{	
	m_pMixer	= new CMixer();

	m_pMixer->Init();

	SquareCh1	= new CSquare(m_pMixer, CHANID_SQUARE1);
	SquareCh2	= new CSquare(m_pMixer, CHANID_SQUARE2);
	TriangleCh	= new CTriangle(m_pMixer, CHANID_TRIANGLE);
	NoiseCh		= new CNoise(m_pMixer, CHANID_NOISE);
	DPCMCh		= new CDPCM(m_pMixer, pSampleMem, CHANID_DPCM);

	VRC6.Init(m_pMixer);
	//MMC5.Init(m_pMixer);
	FDS.Init(m_pMixer);
	N106.Init(m_pMixer);
	VRC7.Init(m_pMixer);

	MMC5 = new CMMC5(m_pMixer);

	DPCMCh->SetSpeed(SPEED_NTSC);

	m_pParent = pCallback;

	FrameCycles = 0;
	m_iNextStopTime = 0;
	SoundBuffer = NULL;
}

CAPU::~CAPU()
{
}

inline void CAPU::Clock_240Hz()
{
	// 240Hz Frame counter (1/4 frame)
	//

	SquareCh1->EnvelopeUpdate();
	SquareCh2->EnvelopeUpdate();
	NoiseCh->EnvelopeUpdate();
	TriangleCh->LinearCounterUpdate();
}

inline void CAPU::Clock_120Hz()
{
	// 120Hz Frame counter (1/2 frame)
	//

	SquareCh1->SweepUpdate(true);
	SquareCh2->SweepUpdate(false);

	SquareCh1->LengthCounterUpdate();
	SquareCh2->LengthCounterUpdate();
	TriangleCh->LengthCounterUpdate();
	NoiseCh->LengthCounterUpdate();
}

inline void CAPU::Clock_60Hz()
{
	// 60Hz Frame counter (1/1 frame)
	//

	// No IRQs are generated for NSFs
}

inline void CAPU::ClockSequence()
{
	// The frame sequencer
	//

	if (FrameMode == 0) {
		FrameSequence = (FrameSequence + 1) % 4;
		switch (FrameSequence) {
			case 0: Clock_240Hz(); break;
			case 1: Clock_240Hz(); Clock_120Hz(); break;
			case 2: Clock_240Hz(); break;
			case 3: Clock_240Hz(); Clock_120Hz(); Clock_60Hz(); break;
		}
	}
	else {
		FrameSequence = (FrameSequence + 1) % 5;
		switch (FrameSequence) {
			case 0: Clock_240Hz(); Clock_120Hz(); break;
			case 1: Clock_240Hz(); break;
			case 2: Clock_240Hz(); Clock_120Hz(); break;
			case 3: Clock_240Hz(); break;
			case 4: break;
		}
	}
}

void CAPU::Shutdown()
{
	// Delete allocated memory
	//

	VRC7.Shutdown();

	delete SquareCh1;
	delete SquareCh2;
	delete TriangleCh;
	delete NoiseCh;
	delete DPCMCh;
	
	m_pMixer->Shutdown();

	delete m_pMixer;
}

void CAPU::Process()
{
	// The main APU emulation
	//
	// The amount of cycles that will be emulated is added by CAPU::AddCycles
	//
	
	static uint32 Time;
	uint32 ClockCycles;

	if (m_iNextStopTime < m_iCurrentTime)
		return;

	ClockCycles = m_iNextStopTime - m_iCurrentTime;

	if (m_iExternalSoundChip & SNDCHIP_VRC6)
		VRC6.Process(ClockCycles);
	if (m_iExternalSoundChip & SNDCHIP_MMC5)
		MMC5->Process(ClockCycles);
	if (m_iExternalSoundChip & SNDCHIP_FDS)
		FDS.Process(ClockCycles);
	if (m_iExternalSoundChip & SNDCHIP_N106)
		N106.Process(ClockCycles);
	if (m_iExternalSoundChip & SNDCHIP_VRC7)
		VRC7.Process(ClockCycles);

	while (m_iNextStopTime > m_iCurrentTime) {
		ClockCycles = m_iNextStopTime - m_iCurrentTime;

		if (ClockCycles > FrameClock)
			Time = FrameClock;
//		else if (ClockCycles > 500)
//			Time = 500;
		else
			Time = ClockCycles;

		SquareCh1->Process(Time);
		SquareCh2->Process(Time);
		TriangleCh->Process(Time);
		NoiseCh->Process(Time);
		DPCMCh->Process(Time);

		FrameCycles		+= Time;
		m_iCurrentTime	+= Time;
		FrameClock		-= Time;

		if (FrameClock < 1) {
			FrameClock += SEQUENCER_PERIOD;
			ClockSequence();
		}
	}
}

// End of audio frame, flush the buffer if enough samples has been produced, and start a new frame
void CAPU::EndFrame()
{
	// The APU will always output audio in 32 bit signed format
	
	SquareCh1->EndFrame();
	SquareCh2->EndFrame();
	TriangleCh->EndFrame();
	NoiseCh->EndFrame();
	DPCMCh->EndFrame();

	if (m_iExternalSoundChip & SNDCHIP_VRC6)
		VRC6.EndFrame();
	if (m_iExternalSoundChip & SNDCHIP_MMC5)
		MMC5->EndFrame();
	if (m_iExternalSoundChip & SNDCHIP_FDS)
		FDS.EndFrame();
	if (m_iExternalSoundChip & SNDCHIP_N106)
		N106.EndFrame();	
	if (m_iExternalSoundChip & SNDCHIP_VRC7)
		VRC7.EndFrame();

//	m_pMixer->FinishBuffer(FrameCycles);
//	m_pMixer->FlushBuffer(m_pSoundInterface, SoundBufferSamples, m_bStereoEnabled);

	int ReadSamples, SamplesAvail;
	
	SamplesAvail = m_pMixer->FinishBuffer(FrameCycles);
	ReadSamples	= m_pMixer->ReadBuffer(SamplesAvail, SoundBuffer, m_bStereoEnabled);
	m_pParent->FlushBuffer(SoundBuffer, ReadSamples /*<< SampleSizeShift*/);

	FrameCycles = 0;
	m_iCurrentTime = 0;
}

void CAPU::Reset()
{
	// Reset APU
	//
	
	m_iNextStopTime = 0;
	m_iCurrentTime	= 0;

	FrameCycles		= 0;
	FrameClock		= SEQUENCER_PERIOD;
	FrameSequence	= 0;
	FrameMode		= 0;
	
	SquareCh1->Reset();
	SquareCh2->Reset();
	TriangleCh->Reset();
	NoiseCh->Reset();
	DPCMCh->Reset();
	
	m_pMixer->ClearBuffer();

	VRC6.Reset();
	MMC5->Reset();
	FDS.Reset();
	N106.Reset();
	VRC7.Reset();
}

void CAPU::SetupMixer(int LowCut, int HighCut, int HighDamp, int Volume) const
{
	m_pMixer->UpdateSettings(LowCut, HighCut, HighDamp, Volume);
}

void CAPU::SetExternalSound(uint8 Chip)
{
	m_iExternalSoundChip = Chip;
	m_pMixer->ExternalSound(Chip);
}

void CAPU::ChangeMachine(int Machine)
{
	// Allow to change speed on the fly
	//

	if (Machine == MACHINE_NTSC) {
		DPCMCh->SetSpeed(MACHINE_NTSC);
		m_pMixer->SetClockRate((uint32)BASE_FREQ_NTSC);
	}
	else if (Machine == MACHINE_PAL) {
		DPCMCh->SetSpeed(MACHINE_PAL);
		m_pMixer->SetClockRate((uint32)BASE_FREQ_PAL);
	}
}

void CAPU::SetSoundInterface(ISoundInterface *pSoundInterface)
{
	//m_pSoundInterface = pSoundInterface;
}

bool CAPU::SetupSound(int SampleRate, int NrChannels, int Machine)
{
	// Allocate a sound buffer
	//
	// Returns false if a buffer couldn't be allocated
	//
	
	uint32 BaseFreq;

	SoundBufferSamples	= uint32(SampleRate / FRAME_RATE_PAL);			// Samples / frame. Allocate for PAL, since it's more
	m_bStereoEnabled	= (NrChannels == 2) ? true : false;

	DPCMCh->SetSpeed(Machine);
	BaseFreq = (Machine == MACHINE_NTSC) ? (uint32)BASE_FREQ_NTSC : (uint32)BASE_FREQ_PAL;

	BufferPointer		= 0;
	SoundBufferSize		= SoundBufferSamples * NrChannels;			// Total amount of samples to allocate
	SampleSizeShift		= (NrChannels == 2) ? 1 : 0;

	m_pMixer->AllocateBuffer(SoundBufferSamples, SampleRate, BaseFreq, NrChannels);

	if (SoundBuffer != NULL) {
		delete [] SoundBuffer;
		SoundBuffer = NULL;
	}
	
	SoundBuffer = new int16[SoundBufferSize << 1];
	
	if (SoundBuffer == NULL)
		return false;
	
	VRC7.SetSampleSpeed(SampleRate, (Machine == MACHINE_NTSC ? BASE_FREQ_NTSC : BASE_FREQ_PAL));

	return true;
}

void CAPU::Halt()
{
	//m_pSoundInterface->StopSound();
}

void CAPU::SetNextTime(uint32 Cycles)
{
	m_iNextStopTime = Cycles;
}

void CAPU::Write(uint16 Address, uint8 Value)
{
	// Data was written to an APU register
	//

	Process();

	if (Address == 0x4015) {
		Write4015(Value);
		return;
	}
	else if (Address == 0x4017) {
		Write4017(Value);
		return;
	}

	switch (Address & 0x1C) {
		case 0x00: SquareCh1->Write(Address & 0x03, Value); break;
		case 0x04: SquareCh2->Write(Address & 0x03, Value); break;
		case 0x08: TriangleCh->Write(Address & 0x03, Value); break;
		case 0x0C: NoiseCh->Write(Address & 0x03, Value); break;
		case 0x10: DPCMCh->Write(Address & 0x03, Value); break;
	}
}

void CAPU::Write4017(uint8 Value)
{
	// The $4017 Control port
	//

	Process();

	// Reset counter
	FrameSequence = 0;

	// Mode 1
	if (Value & 0x80) {
		FrameMode = 1;
		// Immediately run all units		
		Clock_240Hz();
		Clock_120Hz();
		Clock_60Hz();
	}
	// Mode 0
	else
		FrameMode = 0;

	// IRQs are not generated when playing NSFs
}

void CAPU::Write4015(uint8 Value)
{
	//  Sound Control ($4015)
	//

	Process();

	SquareCh1->WriteControl(Value);
	SquareCh2->WriteControl(Value >> 1);
	TriangleCh->WriteControl(Value >> 2);
	NoiseCh->WriteControl(Value >> 3);
	DPCMCh->WriteControl(Value >> 4);
}

uint8 CAPU::Read4015()
{
	// Sound Control ($4015)
	//

	uint8 RetVal;

	Process();

	RetVal = SquareCh1->ReadControl();
	RetVal |= SquareCh2->ReadControl() << 1;
	RetVal |= TriangleCh->ReadControl() << 2;
	RetVal |= NoiseCh->ReadControl() << 3;
	RetVal |= DPCMCh->ReadControl() << 4;
	RetVal |= DPCMCh->DidIRQ() << 7;
	
	return RetVal;
}

void CAPU::ExternalWrite(uint16 Address, uint8 Value)
{
	// Data was written to an external sound chip (this doesn't really belong in the APU)
	//

	Process();

	if (m_iExternalSoundChip & SNDCHIP_VRC6)
		VRC6.Write(Address, Value);
	if (m_iExternalSoundChip & SNDCHIP_MMC5)
		MMC5->Write(Address, Value);
	if (m_iExternalSoundChip & SNDCHIP_FDS)
		FDS.Write(Address, Value);
	if (m_iExternalSoundChip & SNDCHIP_N106)
		N106.Write(Address, Value);
	if (m_iExternalSoundChip & SNDCHIP_VRC7)
		VRC7.Write(Address, Value);
}

uint8 CAPU::ExternalRead(uint16 Address)
{
	// Data read from an external chip
	//

	uint8 Value = 0;

	Process();

	if (!m_iExternalSoundChip)
		Value = Address >> 8;

	if (m_iExternalSoundChip & SNDCHIP_N106 && !Value)
		Value = N106.Read(Address);
	if (m_iExternalSoundChip & SNDCHIP_FDS && !Value)
		Value = FDS.Read(Address);

	return Value;
}

// Expansion for famitracker

int32 CAPU::GetVol(uint8 Chan)
{
	return m_pMixer->GetChanOutput(Chan);
}

uint8 CAPU::GetSamplePos()
{
	return DPCMCh->GetSamplePos();
}

uint8 CAPU::GetDeltaCounter()
{
	return DPCMCh->GetDeltaCounter();
}