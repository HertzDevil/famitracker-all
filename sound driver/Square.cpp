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
#include "apu.h"
#include "square.h"

// This is also shared by MMC5

CSquare::CSquare(CMixer *pMixer, int ID)
{
	memset(this, 0, sizeof(CSquare));

	Mixer		= pMixer;
	ChanId		= ID;
}

CSquare::~CSquare()
{
}

void CSquare::Reset()
{
	Counter = 0;
	DutyLength = DutyCycle = 0;
	LengthCounter = Wavelength = SweepCounter = 0;
	Enabled = ControlReg = Volume = Volume = 0;

	SweepResult = 0;
	SweepRefresh = SweepShift = 0;
	SweepEnabled = SweepMode = 0;

	EndFrame();
}

void CSquare::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
	case 0x00:
		DutyLength		= CAPU::DUTY_PULSE[(Value & 0xC0) >> 6];
		Looping			= (Value & 0x20);
		EnvelopeFix		= (Value & 0x10);
		Volume			= (Value & 0x0F);
		EnvelopeSpeed	= (Value & 0x0F) + 1;
		break;

	case 0x01:
		SweepRegister	= Value;
		SweepWritten	= true;
		break;

	case 0x02:
		Wavelength		= Value | (Wavelength & 0x0700);
		break;

	case 0x03:
		LengthCounter	= CAPU::LENGTH_TABLE[((Value & 0xF8) >> 3)] + 1;
		Wavelength		= ((Value & 0x07) << 8) | (Wavelength & 0xFF);
		DutyCycle		= 0;
		EnvelopeVolume	= 0x0F;

		if (ControlReg)
			Enabled = 1;

		break;
	}
}

void CSquare::WriteControl(uint8 Value)
{
	ControlReg = Value & 0x01;

	if (ControlReg == 0)
		Enabled = 0;
}

uint8 CSquare::ReadControl()
{
	return ((LengthCounter > 0) && (Enabled == 1));
}

void CSquare::Process(uint32 Time)
{
	if (!Wavelength) {
		FrameCycles += Time;
		return;
	}	

	bool Valid = (Wavelength > 7) && (Enabled != 0) && (LengthCounter > 0) && SweepResult < 0x800;

	while (Time >= Counter) {
		Time		-= Counter;
		FrameCycles += Counter;
		Counter		= Wavelength + 1;
		DutyCycle	= (DutyCycle + 1) & 0x0F;
		AddMixer((((DutyCycle >= DutyLength) && Valid) ? (EnvelopeFix ? Volume : EnvelopeVolume) : 0));
	}

	Counter -= (uint16)Time;
	FrameCycles += Time;
}

void CSquare::LengthCounterUpdate()
{
	if ((Looping == 0) && (LengthCounter > 0)) LengthCounter--;
}

void CSquare::SweepUpdate1()
{
	SweepResult = (Wavelength >> SweepShift);

	if (SweepMode)
		SweepResult = Wavelength - SweepResult;
	else
		SweepResult = Wavelength + SweepResult;

	if (SweepEnabled && (Wavelength > 0x07) && (SweepResult < 0x800) && (SweepShift > 0)) {
		if (SweepCounter <= 1) {
			SweepCounter	= SweepRefresh + 1;
			Wavelength		= SweepResult;
		}
		SweepCounter--;
	}

	if (SweepWritten) {
		SweepWritten	= false;
		SweepEnabled	= (SweepRegister & 0x80);	
		SweepRefresh	= (SweepRegister & 0x70) >> 4;
		SweepMode		= (SweepRegister & 0x08);		
		SweepShift		= (SweepRegister & 0x07);	
		SweepCounter	= SweepRefresh + 1;
	}
}

void CSquare::SweepUpdate2()
{
	SweepResult = (Wavelength >> SweepShift);

	if (SweepMode)
		SweepResult = Wavelength - SweepResult + 1;
	else
		SweepResult = Wavelength + SweepResult;

	if (SweepEnabled && Wavelength > 0x07 && SweepResult < 0x800 && SweepShift > 0) {
		if (SweepCounter <= 1) {
			SweepCounter	= SweepRefresh + 1;
			Wavelength		= SweepResult;
		}
		SweepCounter--;
	}

	if (SweepWritten) {
		SweepWritten	= false;
		SweepEnabled	= (SweepRegister & 0x80);	
		SweepRefresh	= (SweepRegister & 0x70) >> 4;
		SweepMode		= (SweepRegister & 0x08);		
		SweepShift		= (SweepRegister & 0x07);	
		SweepCounter	= SweepRefresh + 1;
	}
}

void CSquare::EnvelopeUpdate()
{
	DoEnvelope();
}
