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

/*
 * Random noise generation
 *
 */

#ifndef _NOISE_H_
#define _NOISE_H_

#include "channel.h"

class CNoise : public CChannel
{
public:
	CNoise(CMixer *pMixer, int ID);
	~CNoise();

	void	Write(uint16 Address, uint8 Value);
	void	WriteControl(uint8 Value);
	uint8	ReadControl();

	void	Reset();

	void	Process(int Time);

	void	LengthCounterUpdate();
	void	EnvelopeUpdate();

private:
	uint8	Enabled, Looping, Volume, SampleRate;
	int32	Value, LastValue, EnvelopeCounter;
	uint16	Samples, Wavelength, LengthCounter, ShiftReg;
	uint8	EnvelopeFix, EnvelopeSpeed;
	uint8	ShiftTwice, LastRand;

	int16	Counter;

	uint8	ControlReg;
};

#endif /* _NOISE_H_ */