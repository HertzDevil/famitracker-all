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

#include <memory>
#include "apu/apu.h"

// VRC6 external sound chip emulation

void CVRC6_Pulse::Reset()
{
	DutyCycle = Volume = Gate = Enabled = 0;
	Frequency = FreqLow = FreqHigh = 0;
	Counter = 0;
	DutyCycleCounter = 0;
}

void CVRC6_Pulse::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
		case 0x00:
            Gate = Value & 0x80;
            DutyCycle = ((Value & 0x70) >> 4) + 1;
            Volume = Value & 0x0F;
			if (Gate)
				AddMixer(Volume);
  			break;
		case 0x01:
            FreqLow = Value;
            Frequency = FreqLow + (FreqHigh << 8);
			break;
		case 0x02:
            Enabled = (Value & 0x80);
            FreqHigh = (Value & 0x0F);
            Frequency = FreqLow + (FreqHigh << 8);
			break;
	}
}

void CVRC6_Pulse::Process(int Time)
{
	if (!Enabled || Frequency == 0) {
		FrameCycles += Time;
		return;
	}

	while (Time >= Counter) {
		Time		-= Counter;
		FrameCycles += Counter;
		Counter		= Frequency;
	
		DutyCycleCounter = (DutyCycleCounter + 1) & 0x0F;
		AddMixer((DutyCycleCounter >= DutyCycle || Gate) ? Volume : 0);
	}

	Counter -= (uint16)Time;
	FrameCycles += Time;
}

void CVRC6_Sawtooth::Reset()
{
	PhaseAccumulator = PhaseInput = Enabled = ResetReg = 0;
	Frequency = 0;
	FreqLow = FreqHigh = 0;
	Counter = 0;
}

void CVRC6_Sawtooth::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
		case 0x00:
            PhaseInput = (Value & 0x3F);
			break;

		case 0x01:
            FreqLow = Value;
            Frequency = FreqLow + (FreqHigh << 8);
			break;

		case 0x02:
            Enabled = (Value & 0x80);
            FreqHigh = (Value & 0x0F);
            Frequency = FreqLow + (FreqHigh << 8);
			break;
	}
}

void CVRC6_Sawtooth::Process(int Time)
{
	if (!Enabled || !Frequency) {
		FrameCycles += Time;
		return;
	}

	while (Time >= Counter) {
		Time		-= Counter;
		FrameCycles	+= Counter;
		Counter		= Frequency;

		PhaseAccumulator = (PhaseAccumulator + PhaseInput) & 0xFF;
		ResetReg++;

		if (ResetReg == 14) {
			PhaseAccumulator = 0;
			ResetReg = 0;
		}

		AddMixer(PhaseAccumulator >> 3);
	}

	Counter -= Time;
	FrameCycles += Time;
}

void CVRC6::Init(CMixer *pMixer)
{
	Mixer		= pMixer;
	Pulse1		= new CVRC6_Pulse(pMixer, CHANID_VRC6_PULSE1);
	Pulse2		= new CVRC6_Pulse(pMixer, CHANID_VRC6_PULSE2);
	Sawtooth	= new CVRC6_Sawtooth(pMixer, CHANID_VRC6_SAWTOOTH);
}

void CVRC6::Reset()
{
	Pulse1->Reset();
	Pulse2->Reset();
	Sawtooth->Reset();
}

void CVRC6::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
		case 0x9000:
		case 0x9001:
		case 0x9002:
			Pulse1->Write(Address & 3, Value);
			break;			
		case 0xA000:
		case 0xA001:
		case 0xA002:
			Pulse2->Write(Address & 3, Value);
			break;
		case 0xB000:
		case 0xB001:
		case 0xB002:
			Sawtooth->Write(Address & 3, Value);
			break;
	}
}

void CVRC6::EndFrame()
{
	Pulse1->EndFrame();
	Pulse2->EndFrame();
	Sawtooth->EndFrame();
}

void CVRC6::Process(int Time)
{
	Pulse1->Process(Time);
	Pulse2->Process(Time);
	Sawtooth->Process(Time);
}
