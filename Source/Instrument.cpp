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


#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "Instrument.h"

/*
 * class CInstrument, base class for instruments
 *
 */

CInstrument::CInstrument() : m_iType(0)
{
	memset(m_cName, 0, 128);
}

void CInstrument::SetName(char *Name)
{
	strcpy(m_cName, Name);
}

void CInstrument::GetName(char *Name) const
{
	strcpy(Name, m_cName);
}

char *CInstrument::GetName()
{
	return m_cName;
}
