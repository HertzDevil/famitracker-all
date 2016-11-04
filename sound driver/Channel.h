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

#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include "mixer.h"

//
// This class is used to derive the audio channels
//

class CChannel
{
public:
	CChannel() {
		FrameCycles = 0;
		LastValue	= 0;
	};

	// Begin a new audio frame
	inline void EndFrame() {
		FrameCycles = 0;
	}

protected:
	inline void AddMixer(int32 Value) {
		if (LastValue == Value)
			return;
		Mixer->AddValue(ChanId, Value, FrameCycles);
		LastValue = Value;
	};

	CMixer		*Mixer;			// The mixer
	uint32		FrameCycles;	// Cycle counter, resets every new frame
	int32		LastValue;		// Last value sent to mixer
	uint16		ChanId;			// This channels unique ID
};

#endif /* _CHANNEL_H_ */