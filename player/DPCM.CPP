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

#include "stdafx.h"
#include "apu.h"
#include "dpcm.h"

CDPCM::CDPCM(CMixer *pMixer, int ID)
{
	Mixer = pMixer;
	ChanId = ID;
}

CDPCM::~CDPCM()
{
}

void CDPCM::Init(CSampleMem *pSampleMem)
{
	SampleMem = pSampleMem;
}

void CDPCM::Reset()
{

	ControlReg = 0;

	PCM = 0;
	ShiftReg = DeltaCounter = DAC_LSB = 0;
	OutValue = Counter = 0;
	Frequency = 0;
	DMA_LoadReg = DMA_LoadRegCnt = 0;
	DMA_Length = DMA_LengthCounter = 0;
	Enabled = TriggeredIRQ = PlayMode = Divider = 0;
	
	EndFrame();
}

void CDPCM::SetSpeed(int Speed)
{
	int i;

	if (Speed == SPEED_NTSC)
		for (i = 0; i < 16; i++)
			DMC_Freq[i] = CAPU::DMC_FREQ_NTSC[i];

	else if (Speed == SPEED_PAL)
		for (i = 0; i < 16; i++)
			DMC_Freq[i] = CAPU::DMC_FREQ_PAL[i];
}

void CDPCM::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
		case 0x00:
			PlayMode = (Value & 0xC0);
			Frequency = DMC_Freq[(Value & 0x0F)];
			if ((Value & 0x80) == 0x00) 
				TriggeredIRQ = 0;
			break;
		case 0x01:
			//if ((Value & 0x80) == 0)							// fix
			//	DeltaCounter = Value & 0x7F;
			DeltaCounter = Value & 0x7F;
			AddMixer(DeltaCounter);
			break;
		case 0x02:
			DMA_LoadReg = Value;
			break;
		case 0x03:
			DMA_Length = Value;
			break;
	}
}

void CDPCM::WriteControl(uint8 Value)
{
	if (Value == 0x00) {
		Enabled = 0;
		DMA_LengthCounter = 0;
	}
	else {
		if (Enabled == 0)
			Reload();

		Enabled = 1;
	}

	TriggeredIRQ = 0;
}

uint8 CDPCM::ReadControl()
{
	if (Enabled == 1)
		return 1;

	return 0;
}

uint8 CDPCM::DidIRQ()
{
	if (TriggeredIRQ == 1) 
		return 1;

	return 0;
}

void CDPCM::Reload()
{
    DMA_LoadRegCnt		= (DMA_LoadReg << 6) | 0x4000;
    DMA_LengthCounter	= (DMA_Length << 4) + 1;
	Divider				= 0;
	Counter				= Frequency;

	SampleFilled		= false;
	SilenceFlag			= false;
}

void CDPCM::Process(int Time)
{
	static int LastDelta;

	if (!Enabled) {
		FrameCycles += Time;
		return;
	}

	while (Time > Counter) {
		Time		-= Counter;
		FrameCycles += Counter;
		Counter		= Frequency;

		// DMA reader
		// Check if a new byte should be fetched
		if (!SampleFilled) {

			SampleFilled	= true;
			SampleBuf		= SampleMem->Read((DMA_LoadRegCnt++) | 0x8000);
			//SampleBuf		= CPU->GetByte((DMA_LoadRegCnt++) | 0x8000);
//			CPU->AddCycles(4);

			DMA_LengthCounter--;

			if (!DMA_LengthCounter) {
				switch (PlayMode) {
					case 0x00:	// Stop
						Enabled = 0;
						SilenceFlag = true;
						break;
					case 0x40:
					case 0xC0:
						Reload();
						break;
					case 0x80:	// Stop and do IRQ (not when an NSF is playing)
						Enabled = 0;
						SilenceFlag = true;
						TriggeredIRQ = true;
						break;
				}
			}
		}

		// Output unit
		if (!Divider) {
			Divider			= 8;
			ShiftReg		= SampleBuf;
			SampleFilled	= false;
		}

		if (!SilenceFlag) {
			if (ShiftReg & 0x01) {
//				if (LastDelta == 1 || !DMC_Filter) {		// This doesn't exist on a real NES
					if (DeltaCounter < 126)
						DeltaCounter += 2;
					else if (DeltaCounter == 126)
						DeltaCounter++;
//				}
//				LastDelta = 1;
			}
			else {
//				if (LastDelta == 2 || !DMC_Filter) {
					if (DeltaCounter > 1)
						DeltaCounter -= 2;
					else if (DeltaCounter == 2)
						DeltaCounter--;
//				}
//				LastDelta = 2;
			}
		}

		ShiftReg >>= 1;
		Divider--;		

		AddMixer(DeltaCounter);
	}

	Counter -= Time;
	FrameCycles += Time;
}
