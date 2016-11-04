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

CInstrument5B::CInstrument5B()
{
	
}

CInstrument *CInstrument5B::Clone()
{
	return NULL;
}

void CInstrument5B::Store(CDocumentFile *pDocFile)
{
}

bool CInstrument5B::Load(CDocumentFile *pDocFile)
{
	return false;
}

void CInstrument5B::SaveFile(CFile *pFile)
{
	AfxMessageBox("Saving 5B instruments is not yet supported");
}

bool CInstrument5B::LoadFile(CFile *pFile, int iVersion)
{
	return false;
}

int CInstrument5B::CompileSize(CCompiler *pCompiler)
{
	return 0;
}

int CInstrument5B::Compile(CCompiler *pCompiler, int Index)
{
	return 0;
}
