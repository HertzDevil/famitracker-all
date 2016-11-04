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

#pragma once

#include "driver.h"
#include "FamiTrackerDoc.h"

enum {
	INSTLIST_POINTER = 0,
	DPCM_INSTLIST_POINTER,
	SEQLIST_POINTER,
	FRAMELIST_POINTER,
	DPCMLIST_POINTER,

	INST_POINTERS,
	DPCM_INST_POINTERS,
	SEQ_POINTERS,
	FRAME_POINTERS,
	DPCM_POINTERS,

	FRAME_LIST,

	OFFSET_COUNT
};

// CCompile command target

class CCompile : public CObject
{
public:
	CCompile();
	virtual ~CCompile();

	void	Clean();

	void	CreateNSF(CString FileName, CFamiTrackerDoc *pDoc, bool BankSwitch, bool ForcePAL);
	void	CreateBIN(CString FileName, CFamiTrackerDoc *pDoc);

	void	BuildMusicData(int StartAddress, CFamiTrackerDoc *pDoc);

	void	WriteToBank(unsigned char Value);
	void	StoreBankPosition(int Number, int Increase);
	void	WriteBankOffset(int Number, int Offset);

	void	SetSongInfo(char *Name, char *Artist, char *Copyright, int Speed, int Machine, bool ForcePAL);

	CString	GetLogOutput();

private:
	void	AddLog(CString Text);

	unsigned char	*m_pBank;
	unsigned int	m_iBankPointer;
	unsigned short	m_iMusicDataAddr, m_iMusicDataSize;

	unsigned int	m_iPointerOffsets[OFFSET_COUNT];
	bool			m_bPageDirty[8];

	CString			LogText;

	stNSFHeader Header;

};

