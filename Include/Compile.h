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

const unsigned int MAX_BANKS = 0x20;

enum {
	INSTLIST_POINTER = 0,
	DPCM_INSTLIST_POINTER,
	SEQLIST_POINTER,
	FRAMELIST_POINTER,
	DPCMLIST_POINTER,
	SONGLIST_POINTER,

	SONG_POINTERS,

	INST_POINTERS,
	DPCM_INST_POINTERS,
	SEQ_POINTERS,
	FRAME_POINTERS,
	DPCM_POINTERS,

	FRAME_LIST,

	OFFSET_COUNT
};

struct stBank {
	char *Data;
	unsigned int Origin;
};


class CPatternCompiler {
public:
	CPatternCompiler();

	void	CompileData(CFamiTrackerDoc *pDoc, int Pattern, int Channel, unsigned char (*DPCM_LookUp)[MAX_INSTRUMENTS][8][12]);
	void	Cleanup();
	int		PatternSize() { return m_iPointer; };
	char	*PatternData() { return m_cpTempData; };
	bool	IsSampleAccessed(unsigned int Index) { return m_bDSamplesAccessed[Index]; };

private:
	void	AccumulateZero();
	void	DispatchZeroes();
	void	Write(char Value);

	unsigned int	m_iPointer; 
	unsigned int	m_iZeroes;
	char			*m_cpTempData;

	bool			m_bDSamplesAccessed[OCTAVE_RANGE * NOTE_RANGE];
};

// CCompile command target

class CCompile : public CObject
{
public:
	CCompile();
	virtual			~CCompile();

	void			Clean();

	void			CreateNSF(CString FileName, CFamiTrackerDoc *pDoc, bool BankSwitch, bool ForcePAL);
	void			CreateBIN(CString FileName, CString SampleFile, CFamiTrackerDoc *pDoc);
	void			CreatePRG(CString FileName, CFamiTrackerDoc *pDoc, bool ForcePAL);

	void			BuildMusicData(int StartAddress, bool BankSwitched, CFamiTrackerDoc *pDoc);
	
	void			SetSongInfo(char *Name, char *Artist, char *Copyright, int Speed, int Machine, bool ForcePAL, int Songs);

	CString			GetLogOutput();

private:
	void			AddLog(CString Text);
	void			AccumulateZero();
	void			DispatchZeroes();

	void			WriteAddress(unsigned int Address, char Value);
	void			WriteToBank(unsigned char Value);
	void			TransferData(unsigned int Address, char *From, unsigned int Size);
	void			AllocateNewBank(unsigned int Address);
	void			StoreBankPosition(int Number, int Increase);
	void			WriteBankOffset(int Number, int Offset);
	void			Cleanup();

	void			WriteOffsetWord(int Number, int Offset, unsigned short Value);
	void			WriteOffsetByte(int Number, int Offset, unsigned char Value);

	void			ScanSong(CFamiTrackerDoc *pDoc);
	void			ScanPattern(CFamiTrackerDoc *pDoc);

	unsigned int	m_iMusicDataEnd, m_iDPCMStart;

	unsigned int	m_iBankPointer;
	unsigned short	m_iMusicDataAddr, m_iMusicDataSize;

	unsigned short	m_iRawMusicSize, m_iRawSampleSize;

	unsigned int	m_iPointerOffsets[OFFSET_COUNT];
	bool			m_bPageDirty[8];

	CString			LogText;

	stNSFHeader		Header;

	unsigned char	m_cInstrumentsUsed;
	unsigned char	m_cSequencesUsed;
	unsigned char	m_cDSamplesUsed;
	unsigned int	m_iSequenceIndices[MAX_SEQUENCES * MOD_COUNT];
	//unsigned int	m_iMaxPattern[MAX_CHANNELS];

	unsigned short	m_sFramePointers[MAX_TRACKS];

	// The new banking

	char			m_cSelectedBanks[8];		// There are 8 banks visible
	char			*m_pBanks[8];
	stBank			m_stBanks[MAX_BANKS];
	unsigned int	m_iAllocatedBanks;
	bool			m_bBankSwitced;
};

