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
#include "triangle.h"

CTriangle::CTriangle(CMixer *pMixer, int ID)
{
	Mixer = pMixer;
	ChanId = ID;
}

CTriangle::~CTriangle()
{
}

void CTriangle::Reset()
{
	Enabled		= 0;
	Counter		= 0;
	ControlReg	= 0;
	LengthCounter = 0;
	LinearCounter = 0;
	StepGenStep = 1;

	Value = 0;
	LastValue = 0;

	EndFrame();
	AddMixer(0);
}

void CTriangle::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
		case 0x00:
			LinearLoad = (Value & 0x7F);
			Loop = (Value & 0x80);
			break;
		case 0x01:
			break;
		case 0x02:
			Wavelength = Value | (Wavelength & 0x0700);
			break;
		case 0x03:
			Wavelength = ((Value & 0x07) << 8) | (Wavelength & 0xFF);
			LengthCounter = CAPU::LENGTH_TABLE[(Value & 0xF8) >> 3];
			Halt = 1;
			if (ControlReg)
				Enabled = 1;
			break;
	}
}

void CTriangle::WriteControl(uint8 Value)
{
	ControlReg = Value;
	if (Value == 0) Enabled = 0;
}

uint8 CTriangle::ReadControl()
{
	return ((LengthCounter > 0) && (Enabled == 1));
}

void CTriangle::Process(int Time)
{
	// The triangle will be skipped if a wavelength less than 2 is used
	// It takes to much CPU and it wouldn't be possible to hear anyway
	//

	if (!LinearCounter || !LengthCounter || !Enabled || Wavelength < 2) {
		FrameCycles += Time;
		return;
	}

	while (Time >= Counter) {
		Time		-= Counter;
		FrameCycles += Counter;
		Counter		= Wavelength + 1;

		StepGen = (StepGen + 1) & 0x1F;
		AddMixer((StepGen & 0x0F) ^ ((StepGen & 0x10) - ((StepGen >> 4) & 0x01)));
	}
	
	Counter -= Time;
	FrameCycles += Time;
}

void CTriangle::LengthCounterUpdate()
{
	if ((Loop == 0) && (LengthCounter > 0)) LengthCounter--;
}

void CTriangle::LinearCounterUpdate()
{
	if (Halt == 1)
		LinearCounter = LinearLoad;
	else
		if (LinearCounter > 0)
			LinearCounter--;

	if (Loop == 0)
		Halt = 0;
}

