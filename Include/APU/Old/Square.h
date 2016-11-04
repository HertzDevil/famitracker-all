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

/*
 * Square wave generation
 *
 */

#ifndef _SQUARE_H_
#define _SQUARE_H_

#include "channel.h"
#include "envelope.h"

class CSquare : public CChannel, CEnvelope
{
public:
	CSquare(CMixer *pMixer, int ID);
	~CSquare();

	void	Reset();

	void	Write(uint16 Address, uint8 Value);
	void	WriteControl(uint8 Value);
	uint8	ReadControl();

	void	Process(uint32 Time);

	void	LengthCounterUpdate();
	void	SweepUpdate1();
	void	SweepUpdate2();
	void	EnvelopeUpdate();
	
private:
	uint8	Enabled;
	uint8	Volume;

	bool bInverted;

	uint8	DutyLength, DutyCycle;
	uint16	Wavelength;
	uint16	LengthCounter;
	uint8	ControlReg;

	uint8	WaveLow, WaveHigh;

	uint8	SweepEnabled, SweepRefresh, SweepMode, SweepShift;
	int16	SweepCounter, SweepResult;
	bool	SweepWritten;
	uint8	SweepRegister;

	uint16	Counter;

	int32	Value, LastValue;
};

#endif /* _SQUARE_H_ */