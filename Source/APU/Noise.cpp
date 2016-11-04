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

#include "stdafx.h"
#include "apu/apu.h"
#include "apu/noise.h"

CNoise::CNoise(CMixer *pMixer, int ID)
{
	Mixer = pMixer;
	ChanId = ID;
}

CNoise::~CNoise()
{
}

void CNoise::Reset()
{
	EnvelopeCounter = 0;
	LengthCounter = 0;
	ControlReg = 0;
	ShiftReg = 1;
	Enabled = 0;
	Counter = 0;
	Wavelength = 4;

	Value = 0;

	EndFrame();
}

void CNoise::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
	case 0x00:
		Looping				= (Value & 0x20);
		EnvelopeFix			= (Value & 0x10);
		if (!EnvelopeFix)
			EnvelopeSpeed	= (Value & 0x0F) + 1;
		else
			Volume			= (Value & 0x0F);
		break;

	case 0x01:
		break;

	case 0x02:
		Wavelength = CAPU::NOISE_FREQ[Value & 0x0F];
		SampleRate = (Value & 0x80) ? 8 : 13;
		break;

	case 0x03:
		LengthCounter = CAPU::LENGTH_TABLE[((Value & 0xF8) >> 3)] + 1;
		if (!EnvelopeFix)
			Volume = 0x0F;
		if (ControlReg)
			Enabled = 1;
		break;
	}
}

void CNoise::WriteControl(uint8 Value)
{
	ControlReg = Value;

	if (Value == 0) 
		Enabled = 0;
}

uint8 CNoise::ReadControl()
{
	return ((LengthCounter > 0) && (Enabled == 1));
}

void CNoise::Process(int Time)
{
	bool Output = Enabled && (LengthCounter > 0);
	static uint8 LastSample;	// I guess this wouldn't be allowed if there was more than one instance of CNoise

	while (Time >= Counter) {
		Time		-= Counter;
		FrameCycles += Counter;
		Counter		= Wavelength;
		
		ShiftReg = (((ShiftReg << 14) ^ (ShiftReg << SampleRate)) & 0x4000) | (ShiftReg >> 1);

		if (Output && LastSample != (ShiftReg & 0x01)) {
			LastSample = (ShiftReg & 0x01);
			AddMixer((LastSample && Output) ? Volume : 0);
		}
	}

	Counter -= Time;
	FrameCycles += Time;
}

void CNoise::LengthCounterUpdate()
{
	if ((Looping == 0) && (LengthCounter > 0)) LengthCounter--;
}

void CNoise::EnvelopeUpdate()
{
	if (--EnvelopeCounter < 1) {
		EnvelopeCounter += EnvelopeSpeed;
		if (!EnvelopeFix) {
			if (Looping)
				Volume = (Volume - 1) & 0x0F;
			else if (Volume > 0)
				Volume--;
		}
	}
}
