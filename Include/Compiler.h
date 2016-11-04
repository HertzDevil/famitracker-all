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

#pragma once

#include "PatternCompiler.h"
#include "Driver.h"

const unsigned int MAX_BANKS = 0x80;

class CDataBank {
public:
	CDataBank();
	~CDataBank();
	void			AllocateBank(unsigned char Origin);
	void			WriteByte(unsigned int Pointer, unsigned char Value);
	void			WriteShort(unsigned int Pointer, unsigned short Value);
	bool			Overflow(unsigned int iSize);
	unsigned char	*GetData();
	void			CopyData(unsigned char *pToArray);

private:
	unsigned char	*m_pData;
	unsigned int	m_iOrigin;
	unsigned int	m_iPointer;
};

// This is stored first in NSF data, 8 bytes
struct stTuneHeader {
	unsigned short	SongListOffset;
	unsigned short	InstrumentListOffset;
	unsigned short	DPCMInstListOffset;
	unsigned short	DPCMSamplesOffset;
	unsigned short	DividerNTSC;
	unsigned short	DividerPAL;
};

// One song header per track, 6 bytes
struct stSongHeader {
	unsigned short	FrameListOffset;
	unsigned char	FrameCount;
	unsigned char	PatternLength;
	unsigned char	Speed;
	unsigned char	Tempo;
};

class CCompiler
{
public:
	CCompiler();
	~CCompiler();

	void Export(CString FileName, CFamiTrackerDoc *pDoc, CEdit *pLogTxt);
	void ExportNSF(CString FileName, CFamiTrackerDoc *pDoc, CEdit *pLogTxt, bool EnablePAL);
	void ExportBIN(CString BIN_File, CString DPCM_File, CFamiTrackerDoc *pDoc, CEdit *pLogTxt);
	void ExportPRG(CString FileName, CFamiTrackerDoc *pDoc, CEdit *pLogTxt, bool EnablePAL);

private:
	void CreateHeader(CFamiTrackerDoc *pDoc, stNSFHeader *pHeader, bool EnablePAL);

	void AllocateSpace();
	void RemoveSpace();
	void ScanSong(CFamiTrackerDoc *pDoc);

	void CompileData(CFamiTrackerDoc *pDoc);
	void AssembleData(CFamiTrackerDoc *pDoc);

	// Data bank functions
	void AllocateNewBank(unsigned int iAddress);
	void SetMemoryPosition(unsigned int iAddress);
	void SetMemoryOffset(unsigned int iOffset);
	void SetInitialPosition(unsigned int iAddress);
	void SkipBytes(unsigned int iBytes);
	void StoreShort(unsigned short Value);
	void StoreByte(unsigned char Value);

	unsigned int GetCurrentOffset();

	// Song conversion functions
	void WriteHeader(stTuneHeader *pHeader, CFamiTrackerDoc *pDoc);
	void WriteTrackHeader(stSongHeader *m_stTrackHeader, unsigned int iAddress);

	void StoreSongs(CFamiTrackerDoc *pDoc);
	void CreateFrameList(CFamiTrackerDoc *pDoc);
	void StorePatterns(CFamiTrackerDoc *pDoc, unsigned int Track);

	void CreateInstrumentList(CFamiTrackerDoc *pDoc);
	void CreateSequenceList(CFamiTrackerDoc *pDoc);

	void CreateDPCMList(CFamiTrackerDoc *pDoc);
	void StoreDPCM(CFamiTrackerDoc *pDoc);

	bool IsPatternAddressed(CFamiTrackerDoc *pDoc, int Pattern, int Channel);

	int GetSampleIndex(int SampleNumber);
	bool IsSampleAccessed(int SampleNumber);

	// Debugging
	void WriteLog(char *text, ...);
	void Print(char *text, ...);

private:
	// Data
	unsigned char	*m_pData;					// Linear music data
	unsigned char	*m_pDPCM;					// DPCM samples (for both bank switched and linear)
	unsigned int	m_iDataPointer;
	unsigned int	m_iInitialPosition;
	bool			m_bBankSwitched;			// True for bank switched song
	unsigned int	m_iMasterHeaderAddr;

	// For bank switching
	CDataBank		m_DataBanks[MAX_BANKS];
	unsigned int	m_iSelectedBank;
	unsigned int	m_iAllocatedBanks;
	unsigned char	m_cSelectedBanks[8];		// There are 8 banks visible
	CDataBank		m_pSelectedBanks[8];

	unsigned int	m_iLoadAddress;
	unsigned int	m_iDriverLocation;
	bool			m_bErrorFlag;

	// Sequences and instruments
	unsigned int	m_iDPCMSize;
	unsigned int	m_iInstruments;
	unsigned int	m_iSequences;
	unsigned int	m_iAssignedInstruments[MAX_INSTRUMENTS];
	unsigned int	m_iSequenceAddresses[MAX_SEQUENCES][MOD_COUNT];
	
	// Patterns and frames
	unsigned short	m_iPatternAddresses[MAX_PATTERN][MAX_CHANNELS];
	unsigned char	m_iPatternBanks[MAX_PATTERN][MAX_CHANNELS];
	unsigned int	m_iFrameListAddress;
	CPattCompiler	m_oPatternCompiler;
	
	// Sample variables
	unsigned char	m_iDPCM_LookUp[MAX_INSTRUMENTS][OCTAVE_RANGE][NOTE_RANGE];
	bool			m_bSamplesAccessed[MAX_INSTRUMENTS][OCTAVE_RANGE][NOTE_RANGE];
	unsigned char	m_iSampleBank[MAX_DSAMPLES];
	unsigned int	m_iSamplesUsed;
	unsigned int	m_iSampleStart;

	// Other
	stTuneHeader	m_stTuneHeader;
	stSongHeader	m_stTrackHeader;
	CEdit			*m_pLogText;
};