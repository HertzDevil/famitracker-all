/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2010  Jonathan Liss
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

// Sunsoft 5B

#include <cmath>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "ChannelHandler.h"
#include "Channels5B.h"

void CChannelHandler5B::PlayChannelNote(stChanNote *NoteData, int EffColumns)
{
}

void CChannelHandler5B::ProcessChannel()
{
	// Default effects
	CChannelHandler::ProcessChannel();
}

void CChannelHandler5B::RefreshChannel()
{
}

void CChannelHandler5B::ClearRegisters()
{
}
/*
void CChannelHandler5B::RunSequence(int Index, CSequence *pSequence)
{
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Square 1 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void C5BChannel1::RefreshChannel()
{
}

void C5BChannel1::ClearRegisters()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Square 2 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void C5BChannel2::RefreshChannel()
{
}

void C5BChannel2::ClearRegisters()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Channel 3 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void C5BChannel3::RefreshChannel()
{
}

void C5BChannel3::ClearRegisters()
{
}
