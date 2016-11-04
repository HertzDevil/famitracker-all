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

//
// Cleaning is required after the multisong nsf compiler addition
//

//
// This class is used to create NSF files.
//

#include "stdafx.h"
#include "FamiTracker.h"
#include "Compile.h"

#define GET_BANK(x) ((x - 0x8000) >> 12)

// Endian fix (nothing is needed for the 6502)
#define FIX_NES_ADDRESS(x) (x)

struct stSongDataHeader {
	unsigned char	cTickPeriod;
	unsigned char	cFrameRate;
	unsigned char	cFrameCount;
	unsigned char	cPatternLength;
	unsigned short	sAdrInstrPointer;
	unsigned short	sAdrDPCMInstrPointer;
	unsigned short	sAdrSequencePointer;
	unsigned short	sAdrDPCMListPointer;
	unsigned short	sAdrSongListPointer;
	unsigned short	sFrameRate;
};

const unsigned char NSF_EFF_BEGIN = 0x82;

// CCompile

CCompile::CCompile()
{
}

CCompile::~CCompile()
{
}

// CCompile member functions

CString CCompile::GetLogOutput()
{
	return LogText;
}

void CCompile::AddLog(CString Text)
{
	LogText += Text + "\r\n";
}

/*
** NSF music data structure
**
** .-------------------------------------.
** | * Header data                       |
** |-------------------------------------|
** | Song speed                      : 1 |
** | Frames                          : 1 |
** | Pattern length                  : 1 |
** | Ticks / minute                  : 2 |
** |-------------------------------------|
** | Pointers to:                        |
** |  Instruments                    : 2 | ------
** |  DPCM instruments               : 2 |      |
** |  Sequences                      : 2 |      |
** |  Frames                         : 2 |      |
** |  DPCM samples                   : 2 | -|   |
** |-------------------------------------|  |   |
** | * Music data                        |  |   |
** |-------------------------------------|  |   |
** | Instruments pointers            : x | <-------|
** |-------------------------------------|  |   |  |
** | DPCM instruments pointers       : x | <    |  |
** |-------------------------------------|  |   |  |
** | Sequence pointers               : x | <    |  |
** |-------------------------------------|  |   |  |
** | Song pointers                   : x | <    |  |
** |-------------------------------------|  |   |  |
** | Song frame lists                : x | <    |  |
** |-------------------------------------|  |   |  |
** | DPCM rom addresses              : x | <----|  |
** |-------------------------------------|         |
** | Instrument data                 : x | <-------|
** |-------------------------------------| 
** | DPCM instrument data            : x |
** |-------------------------------------|
** | Sequence lists                  : x |
** |-------------------------------------|
** | Pattern pointers for frames     : x |
** |-------------------------------------|
** | Pattern data for all channels   : x |
** |-------------------------------------|
** | DPCM samples                    : x | (at $C000)
** '-------------------------------------'
*/

void CCompile::WriteToBank(unsigned char Value)
{
	// Store one byte
	//

	ASSERT(m_iBankPointer <= 0xFFFF);

	int BankNumber = (m_iBankPointer - 0x8000) >> 12;
	int BankAddr = m_iBankPointer & 0x0FFF;

	m_bPageDirty[(m_iBankPointer & 0x7FFF) >> 12] = true;

	*(m_pBanks[BankNumber] + BankAddr) = Value;

	m_iBankPointer++;
}

void CCompile::WriteAddress(unsigned int Address, char Value)
{
	ASSERT(Address <= 0xFFFF);

	int BankNumber = (Address - 0x8000) >> 12;
	int BankAddr = Address & 0x0FFF;

	m_bPageDirty[BankNumber] = true;
	*(m_pBanks[BankNumber] + BankAddr) = Value;
}

void CCompile::TransferData(unsigned int Address, char *From, unsigned int Size)
{
	for (unsigned int i = 0; i < Size; i++) {
		WriteAddress(i + Address, From[i]);
	}
}

void CCompile::AllocateNewBank(unsigned int Address)
{
	int BankNumber = (Address - 0x8000) >> 12;
	char *Data;

	ASSERT(BankNumber < MAX_BANKS);

	Data = new char[0x1000];

	memset(Data, 0xFF, 0x1000);

	m_stBanks[m_iAllocatedBanks].Data = Data;
	m_cSelectedBanks[BankNumber] = m_iAllocatedBanks;
	m_pBanks[BankNumber] = Data;

	m_iAllocatedBanks++;
	m_iBankPointer = Address;
}

void CCompile::StoreBankPosition(int Index, int Increase)
{
	m_iPointerOffsets[Index] = m_iBankPointer;
	m_iBankPointer += Increase;
}

void CCompile::WriteBankOffset(int Number, int Offset)
{
	unsigned int Pos = (m_iPointerOffsets[Number] + Offset);
	*(m_pBanks[GET_BANK(Pos)] + (Pos & 0xFFF)) = m_iBankPointer & 0xFF;
	Pos++;
	*(m_pBanks[GET_BANK(Pos)] + (Pos & 0xFFF)) = (m_iBankPointer >> 8) & 0xFF;
}

void CCompile::WriteOffsetWord(int Number, int Offset, unsigned short Value)
{
	unsigned int Pos = (m_iPointerOffsets[Number] + Offset);
	*(m_pBanks[GET_BANK(Pos)] + (Pos & 0xFFF)) = Value & 0xFF;
	Pos++;
	*(m_pBanks[GET_BANK(Pos)] + (Pos & 0xFFF)) = (Value >> 8) & 0xFF;

}

void CCompile::WriteOffsetByte(int Number, int Offset, unsigned char Value)
{
	unsigned int Pos = (m_iPointerOffsets[Number] + Offset);
	*(m_pBanks[GET_BANK(Pos)] + (Pos & 0xFFF)) = Value & 0xFF;
}

void CCompile::Cleanup()
{
	for (unsigned int i = 0; i < m_iAllocatedBanks; i++)
		delete [] m_stBanks[i].Data;
}

void CCompile::CreatePRG(CString FileName, CFamiTrackerDoc *pDoc, bool ForcePAL)
{
	// Create a file that's ready to burn on an eprom
	unsigned short FileSize;
	
	CString Text;
	CFile File;

	if (m_bBankSwitced)
		theApp.DoMessageBox("It's not possible to create bankswitched PRG files! Click OK to continue", 0, 0);

	if (m_iBankPointer > (0x10000 - NSF_CALLER_SIZE))
		theApp.DoMessageBox("The song is too big for fitting in a PRG file! Click OK to continue (the song may not work properly)", 0, 0);

	FileSize = m_iBankPointer - m_iMusicDataAddr;

	Text.Format("Writing %s...", FileName);
	AddLog(Text);

	// Add the music driver
//	memcpy(m_pBank /*+ 0x80*/, DRIVER_BIN, DRIVER_SIZE);
	memcpy(m_stBanks[0].Data, DRIVER_BIN, DRIVER_SIZE);

	// Apply nsf caller...
//	memcpy(m_pBank + (0x8000 - NSF_CALLER_SIZE), NSF_CALLER_BIN, NSF_CALLER_SIZE);
	memcpy(m_stBanks[7].Data + (0x1000 - NSF_CALLER_SIZE), NSF_CALLER_BIN, NSF_CALLER_SIZE);

	m_stBanks[7].Data[0xFD6] = (ForcePAL ? 0x01 : 0x00);
	m_stBanks[7].Data[0xFDA] = 0x00;	// init
	m_stBanks[7].Data[0xFDB] = 0x80;
	m_stBanks[7].Data[0xFF2] = 0x03;	// play
	m_stBanks[7].Data[0xFF3] = 0x80;

/*	// ...and patch it
	m_pBank[0x7FC4] = (ForcePAL ? 0x01 : 0x00);
	m_pBank[0x7FC6] = 0x00;
	m_pBank[0x7FC7] = 0x80;
	m_pBank[0x7FD1] = 0x03;
	m_pBank[0x7FD2] = 0x80;
*/
	// Write file and then return
	if (!File.Open(FileName, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Couldn't open file");
//		delete [] m_pBank;
		return;
	}

	// Size will always be 32kB, fill one whole PRG
	FileSize = 0x8000;

	// Write all banks in order
	for (unsigned int i = 0; i < 8 /*m_iAllocatedBanks*/; i++)
		File.Write(m_stBanks[i].Data, 0x1000);

//	File.Write(m_pBank, FileSize);
	File.Close();

//	delete [] m_pBank;

	Text.Format("Done (%i bytes)", FileSize);
	AddLog(Text);
	Text.Format("Burn this file on a 32 kB (E)EPROM");
	AddLog(Text);
}

void CCompile::CreateBIN(CString FileName, CString SampleFile, CFamiTrackerDoc *pDoc)
{
	// Only create the binary song data, don't apply driver & header
	unsigned short FileSize;
	
	CString Text;
	CFile BinFile, DmcFile;

	FileSize = m_iBankPointer - m_iMusicDataAddr;

	Text.Format("Writing %s...", FileName);
	AddLog(Text);

	// Write file and then return
	if (!BinFile.Open(FileName, CFile::modeWrite | CFile::modeCreate) ||
		!DmcFile.Open(SampleFile, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Couldn't open file(s)");
//		delete [] m_pBank;
		return;
	}

		/*
	for (unsigned int i = 0; i < m_iAllocatedBanks; i++) {
		unsigned int Addr = ((i * 0x1000) + 0x8000);
		if (Addr < m_iBankPointer) {
			if (Addr < (m_iRawMusicSize + 0x8000)) {
				BinFile.Write(m_stBanks[i].Data, 0x1000);
			}
		}
		if (Addr >= m_iDPCMStart) {
			if (Addr < (m_iRawSampleSize + 0xC000)) {
				DmcFile.Write(m_stBanks[i].Data, m_iRawSampleSize);
			}
		}
	}*/

	char *Map = new char[0x8000];

	for (unsigned int i = 0; i < m_iAllocatedBanks; i++) {
		memcpy(Map + 0x1000 * i, m_stBanks[i].Data, 0x1000);
	}

	BinFile.Write(Map + (m_iMusicDataAddr - 0x8000), m_iRawMusicSize);
		
	if (m_iDPCMStart != 0)
		DmcFile.Write(Map + (m_iDPCMStart - 0x8000), m_iRawSampleSize);
	else
		m_iRawSampleSize = 0;

	delete [] Map;

	BinFile.Close();
	DmcFile.Close();

//	delete [] m_pBank;

	Text.Format(" - Music address: $%04X (%i bytes)", m_iMusicDataAddr, m_iRawMusicSize);
	AddLog(Text);
	Text.Format(" - Samples address: $%04X (%i bytes)", m_iDPCMStart, m_iRawSampleSize);
	AddLog(Text);
	Text.Format("Done (%i bytes)", FileSize);
	AddLog(Text);
}

void CCompile::CreateNSF(CString FileName, CFamiTrackerDoc *pDoc, bool BankSwitch, bool ForcePAL)
{
	unsigned short FileSize;
	unsigned int Speed;
	
	CString Text;
	CFile File;

	// First do some preparations

	if (pDoc->GetEngineSpeed() == 0)
		Speed = 1000000 / (pDoc->GetMachine() == NTSC ? 60 : 50);
	else
		Speed = 1000000 / pDoc->GetEngineSpeed();

	Text.Format(" * NSF speed: %i (%s)", Speed, (pDoc->GetMachine() == NTSC ? "NTSC" : "PAL"));
	AddLog(Text);

	// Setup the header
	SetSongInfo(pDoc->GetSongName(), pDoc->GetSongArtist(), pDoc->GetSongCopyright(), pDoc->GetEngineSpeed(), pDoc->GetMachine(), ForcePAL, pDoc->GetTrackCount());

	// Add music driver to the first bank
	memcpy(m_pBanks[0], DRIVER_BIN, DRIVER_SIZE);

	FileSize = m_iBankPointer - 0x8000;

	if (m_bBankSwitced) {
		// Setup a plain bank map when bankswitching is used
		for (unsigned int i = 0; i < 8; i++)
			Header.BankValues[i] = i;
	}
	else if (BankSwitch) {
		int Bank = 0, i, Size;

		FileSize = 0;

		for (i = 0; i < 8; i++) {
			if (m_bPageDirty[i]) {
				if (((i + 1) * 0x1000) < int(m_iBankPointer & 0x7FFF))
					Size = 0x1000;
				else
					Size = (m_iBankPointer & 0x7FFF) - (i * 0x1000);
				Header.BankValues[i] = Bank++;
				FileSize += Size;
			}
		}

		Text.Format("Banks %X %X %X %X %X %X %X %X...", Header.BankValues[0], Header.BankValues[1], Header.BankValues[2], Header.BankValues[3], Header.BankValues[4], Header.BankValues[5], Header.BankValues[6], Header.BankValues[7]);
		AddLog(Text);
	}

	// Now open and write the file

	Text.Format("Writing %s...", FileName);
	AddLog(Text);

	// Write file and then return
	if (!File.Open(FileName, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Couldn't open NSF output file");
		return;
	}

	// Calculate filesize
	FileSize = m_iAllocatedBanks * 0x1000 + 0x80;

	File.Write(&Header, 0x80);

	// Write banks in order (all banks are 4 kB)
	for (unsigned int i = 0; i < m_iAllocatedBanks; i++) {
		if (BankSwitch) {
			if (m_bPageDirty[i])
				File.Write(m_stBanks[i].Data, 0x1000);
		}
		else {
			if (((i * 0x1000) + 0x8000) < m_iBankPointer || m_bBankSwitced)
				File.Write(m_stBanks[i].Data, 0x1000);
		}
	}

	File.Close();

	Cleanup();

	Text.Format("Done (%i bytes)", FileSize);
	AddLog(Text);
}

void CCompile::ScanSong(CFamiTrackerDoc *pDoc)
{
	unsigned int i, j;
	
	m_cInstrumentsUsed	= 0;
	m_cDSamplesUsed		= 0;
	m_cSequencesUsed	= 0;

	// Determine number of used instruments
	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (pDoc->IsInstrumentUsed(i))
			m_cInstrumentsUsed = i + 1;
	}

	// Determine number of used sequences and store indices in a lookup table
	for (i = 0; i < MAX_SEQUENCES; i++) {
		for (j = 0; j < MOD_COUNT; j++) {
			if (pDoc->GetSequenceCount(i, j) > 0)
				m_iSequenceIndices[i * MOD_COUNT + j] = m_cSequencesUsed++;
			else
				m_iSequenceIndices[i * MOD_COUNT + j] = 0;
		}
	}

	// Determine number of used dpcm samples
	for (i = 0; i < MAX_DSAMPLES; i++) {
		if (pDoc->GetSampleSize(i) > 0)
			m_cDSamplesUsed = i + 1;
	}
}

void CCompile::ScanPattern(CFamiTrackerDoc *pDoc)
{
//	unsigned int i, j;
/*
	memset(m_iMaxPattern, 0, sizeof(int) * MAX_CHANNELS);

	// Find all patterns that has been addressed to
	for (i = 0; i < pDoc->GetFrameCount(); i++) {
		for (j = 0; j < pDoc->GetAvailableChannels(); j++) {
			if (pDoc->GetPatternAtFrame(i, j) > m_iMaxPattern[j])
				m_iMaxPattern[j] = pDoc->GetPatternAtFrame(i, j) + 1;
		}
	}
	*/
}

void CCompile::BuildMusicData(int StartAddress, bool BankSwitched, CFamiTrackerDoc *pDoc)
{
	unsigned int i, j, c, x;
	unsigned int TicksPerSec, Machine;
	unsigned int SavedPointer, PointerBeforeDPCM, DPCMStart, TotalSize;
	unsigned int Song;

	unsigned char	DPCM_LookUp[MAX_INSTRUMENTS][8][12];
	unsigned short	PattAddr[MAX_PATTERN][MAX_CHANNELS];
	unsigned char	PattBank[MAX_PATTERN][MAX_CHANNELS];
	bool			SequencesAccessed[MAX_SEQUENCES * MOD_COUNT];

	bool MusicDataFilled = false;
	bool OverflowFlag = false;

	unsigned int NumChannels = pDoc->GetAvailableChannels();
	unsigned int NumSongs	= pDoc->GetTrackCount() + 1;
	
	CPatternCompiler PatternCompiler;

	m_iAllocatedBanks	= 0;
	m_bBankSwitced		= BankSwitched;

	m_iDPCMStart		= 0xC000;
	m_iRawMusicSize		= 0;
	m_iRawSampleSize	= 0;

	LogText = "";

	AddLog("Building music data...");

	m_iMusicDataAddr = StartAddress & 0xFFFF;

	if (m_iMusicDataAddr < 0x8000)
		m_iMusicDataAddr += 0x8000;

	// Setup the 32 kB area
	for (i = 0; i < 8; i++)
		AllocateNewBank(0x8000 + i * 0x1000);

	m_iBankPointer	= m_iMusicDataAddr;

	for (i = 0; i < 8; i++)
		m_bPageDirty[i] = false;

	memset(SequencesAccessed, 0, sizeof(bool) * MAX_SEQUENCES * MOD_COUNT);

	unsigned char Tempo;
	unsigned short DividerNTSC, DividerPAL;

	TicksPerSec = pDoc->GetEngineSpeed();
	Machine		= pDoc->GetMachine();

	if (TicksPerSec == 0) {
		Tempo		= (pDoc->GetFrameRate() * 5) / 2;
		DividerNTSC = FRAMERATE_NTSC * 60;
		DividerPAL	= FRAMERATE_PAL * 60;
	}
	else {
		Tempo = (TicksPerSec * 5) / 2;
		DividerNTSC = DividerPAL = TicksPerSec * 60;
	}

	ScanSong(pDoc);

	//
	// Write song header
	//

	// !											    !
	// ! This is absolute and has to be stored IN ORDER !
	// !												!

//	WriteToBank((TicksPerSec * 5) / 2);		// 60 / 24
	WriteToBank(Tempo);

	// Save offsets for later (and allocate space)
	StoreBankPosition(INSTLIST_POINTER, 2);
	StoreBankPosition(DPCM_INSTLIST_POINTER, 2);
	StoreBankPosition(SEQLIST_POINTER, 2);
	StoreBankPosition(DPCMLIST_POINTER,	2);
	StoreBankPosition(SONGLIST_POINTER, 2);
	/*
	WriteToBank((TicksPerSec * 60) & 0xFF);
	WriteToBank((TicksPerSec * 60) >> 8);
	*/
	WriteToBank(DividerNTSC & 0xFF);
	WriteToBank(DividerNTSC >> 8);
	WriteToBank(DividerPAL & 0xFF);
	WriteToBank(DividerPAL >> 8);

	// !						 !
	// ! End of absoulte storage !
	// !						 !

	// Allocate space for instrument pointers
	WriteBankOffset(INSTLIST_POINTER, 0);
	StoreBankPosition(INST_POINTERS, m_cInstrumentsUsed * 2);

	// Allocate space for DPCM instrument pointers
	WriteBankOffset(DPCM_INSTLIST_POINTER, 0);
	StoreBankPosition(DPCM_INST_POINTERS, m_cInstrumentsUsed * 2);

	// Allocate space for sequence pointers
	WriteBankOffset(SEQLIST_POINTER, 0);
	StoreBankPosition(SEQ_POINTERS, m_cSequencesUsed * 2);

	// Allocate space for DPCM sample pointers
	WriteBankOffset(DPCMLIST_POINTER, 0);
	StoreBankPosition(DPCM_POINTERS, m_cDSamplesUsed * 2);

	// Allocate space for song table (points further to sets of frames)
	WriteBankOffset(SONGLIST_POINTER, 0);
	StoreBankPosition(SONG_POINTERS, NumSongs * 2);

	//
	// Write song data
	//

	//StoreBankPosition(FRAMELIST_POINTER, 2);

	// Store instruments
	for (i = 0; i < m_cInstrumentsUsed; i++) {
		// Write pointer
		WriteBankOffset(INST_POINTERS, i * 2);
		for (c = 0; c < MOD_COUNT; c++) {
			if (pDoc->GetInstEffState(i, c)) {
				int Sequence = pDoc->GetInstEffIndex(i, c);
				int Index = m_iSequenceIndices[Sequence * MOD_COUNT + c];
				WriteToBank(Index + 1);
				SequencesAccessed[Sequence * MOD_COUNT + c] = true;
			}
			else
				WriteToBank(0);
		}
	}

	// Store DPCM instrument definitions
	for (i = 0; i < m_cInstrumentsUsed; i++) {

		unsigned char Sample, SamplePitch;
		int o, n, Item = 1;
		bool First = true;

		for (o = 0; o < OCTAVE_RANGE; o++) {
			for (n = 0; n < 12; n++) {
				if (pDoc->GetInstDPCM(i, n, o) > 0) {
					if (First) {
						WriteBankOffset(DPCM_INST_POINTERS, i * 2);
						First = false;
					}
					Sample		= pDoc->GetInstDPCM(i, n, o) - 1;
					SamplePitch = pDoc->GetInstDPCMPitch(i, n, o);
					if (SamplePitch & 0x80)
						SamplePitch = (SamplePitch & 0x0F) | 0x40;	// I know this was stupid
					WriteToBank(Sample);
					WriteToBank(SamplePitch);
					// Save a reference to this item
					DPCM_LookUp[i][o][n] = Item++;
				}
				else {
					DPCM_LookUp[i][o][n] = 0;
				}
			}
		}
	}

	// Store sequences
	for (i = 0; i < MAX_SEQUENCES; i++) {
		for (x = 0; x < MOD_COUNT; x++) {
			signed short Value, Length;
			signed short Count;
			stSequence *Sequence;
			int Index;

			Index		= m_iSequenceIndices[i * MOD_COUNT + x];
			Sequence	= pDoc->GetSequence(i, x);
			
			if (Sequence->Count > 0 && SequencesAccessed[i * MOD_COUNT + x]) {

				WriteToBank(Sequence->Count);
				// Save a pointer to this list
				WriteBankOffset(SEQ_POINTERS, Index * 2);

				Count = (signed short)Sequence->Count;

				for (c = 0; c < unsigned(Count); c++) {
					Value	= Sequence->Value[c];
					Length	= Sequence->Length[c];
					if (Value < 0)
						Value += 256;
					if (Length >= 0)
						WriteToBank(Length + 1);
					else {
						// fix for the nsf driver
						Length--;
						if (-Length > Count)
							Length = -Count;
						Length += 256;
						WriteToBank((unsigned char)Length);
					}

					WriteToBank((unsigned char)Value);
				}
				WriteToBank(0);
				WriteToBank(0);
			}
		}
	}

	/*
	 * Each song consists of a frame list and the patterns
	 *
	 * Instruments and sequences are shared
	 *
	 */

	int PrevSong = pDoc->GetSelectedTrack();

	for (Song = 0; Song < NumSongs; Song++) {

		// Switch to current song for storage
		pDoc->SelectTrack(Song);
		ScanPattern(pDoc);

		unsigned int NumFrames = pDoc->GetFrameCount();

		// Allocate space for frames and store it in the song list
		WriteBankOffset(SONG_POINTERS, Song * 2);

		WriteToBank(NumFrames);
		WriteToBank(pDoc->GetPatternLength() - 1);
		WriteToBank(pDoc->GetSongSpeed());

		StoreBankPosition(FRAME_POINTERS, NumFrames * 2);

		// Fill addresses for frames
		SavedPointer = m_iBankPointer;

		for (i = 0; i < NumFrames; i++) {
			WriteBankOffset(FRAME_POINTERS, i * 2);
			m_iBankPointer += NumChannels * 4;
		}

		m_iBankPointer = SavedPointer;

		// Allocate space for the frame list (2 bytes for pattern and 1 for bank)
		StoreBankPosition(FRAME_LIST, NumChannels * NumFrames * 4);

		for (i = 0; i < MAX_PATTERN; i++) {
			for (j = 0; j < NumChannels; j++) {
				
				bool FrameIsAccessed = false;
			
				// See if this pattern should be stored
				for (unsigned int k = 0; k < NumFrames && !FrameIsAccessed; k++) {
					if (pDoc->GetPatternAtFrame(k, j) == i)
						FrameIsAccessed = true;
				}

				if (FrameIsAccessed) {

					// Assemble pattern
					PatternCompiler.CompileData(pDoc, i, j, &DPCM_LookUp);

					if ((m_iBankPointer + PatternCompiler.PatternSize()) > 0xBFFF) {
						if (m_bBankSwitced) {
							// If bankswitched, allocate new bank
							AllocateNewBank(0xB000);
						}
						else {
							if ((m_iBankPointer + PatternCompiler.PatternSize()) > 0xFFFF) {
								// Song is too big
								OverflowFlag = true;
							}
						}
					}

					if (!OverflowFlag) {
						PattAddr[i][j] = m_iBankPointer;
						PattBank[i][j] = m_cSelectedBanks[GET_BANK(m_iBankPointer)];

						TransferData(m_iBankPointer, PatternCompiler.PatternData(), PatternCompiler.PatternSize());
						m_iBankPointer += PatternCompiler.PatternSize();
					}

					PatternCompiler.Cleanup();
				}
			}
		}

		PointerBeforeDPCM = m_iBankPointer;

		// Write pattern addresses in the frame list
		for (i = 0; i < NumFrames; i++) {
			for (j = 0; j < NumChannels; j++) {
				int CurPattern = pDoc->GetPatternAtFrame(i, j);
				WriteOffsetWord(FRAME_LIST, ((i * NumChannels) + j) * 4, PattAddr[CurPattern][j]);
				WriteOffsetByte(FRAME_LIST, ((i * NumChannels) + j) * 4 + 2, PattBank[CurPattern][j]);
				WriteOffsetByte(FRAME_LIST, ((i * NumChannels) + j) * 4 + 3, GET_BANK(PattAddr[CurPattern][j]));
			}
		}

		m_iBankPointer = PointerBeforeDPCM;
	}

	pDoc->SelectTrack(PrevSong);

	m_iMusicDataEnd = PointerBeforeDPCM;
	m_iRawMusicSize = PointerBeforeDPCM - StartAddress;

	if (m_cDSamplesUsed > 0) {
		// Align to DPCM bank or nearest valid sample address
		if (m_iBankPointer < 0xC000)
			m_iBankPointer = 0xC000;
		else
			m_iBankPointer = ((m_iBankPointer >> 6) + 1) << 6;

		m_iDPCMStart = m_iBankPointer;
		DPCMStart = m_iBankPointer;

		// Write DPCM samples
		for (i = 0; i < MAX_DSAMPLES; i++) {
			unsigned int SampleSize = pDoc->GetSampleSize(i);
			if (SampleSize > 0 && !OverflowFlag && PatternCompiler.IsSampleAccessed(i)) {				
				if ((SampleSize + m_iBankPointer) > 0xFFFF) {
					OverflowFlag = true;
					break;
				}

				// Store position and length
				WriteAddress((m_iPointerOffsets[DPCM_POINTERS] + (i * 2)), (m_iBankPointer - 0xC000) >> 6);
				WriteAddress((m_iPointerOffsets[DPCM_POINTERS] + (i * 2) + 1), SampleSize >> 4);

				for (j = 0; j < SampleSize; j++)
					WriteToBank(pDoc->GetSampleData(i, j));

				m_iBankPointer += 0x40 - (m_iBankPointer & 0x3F);
			}
		}
	}
	else
		m_iDPCMStart = 0;

	m_iMusicDataSize = m_iBankPointer - StartAddress;
	m_iRawSampleSize = m_iBankPointer - m_iDPCMStart;

	if (m_cDSamplesUsed > 0)
		TotalSize = (PointerBeforeDPCM - 0x8000) + (m_iBankPointer - DPCMStart);
	else
		TotalSize = (PointerBeforeDPCM - 0x8000);

	// Music data is compiled, compile some text

	CString Text, Param;

	Text.Format(" * Instruments/sequences used: %i / %i", m_cInstrumentsUsed, m_cSequencesUsed);
	AddLog(Text);

	if (m_cDSamplesUsed > 0)
		Text.Format(" * DPCM samples: %i, allocated size: %i bytes", m_cDSamplesUsed, m_iBankPointer - 0xC000);
	else
		Text.Format(" * DPCM samples: 0, allocated size: 0 bytes");

	AddLog(Text);

	Text.Format(" * Music data size: %i", (PointerBeforeDPCM - StartAddress));
	AddLog(Text);

	Text.Format(" * Driver: %s", DRIVER_ID);
	AddLog(Text);

	Text.Format(" * Total size: %i bytes", m_iAllocatedBanks * 0x1000);
	AddLog(Text);

	if (OverflowFlag) {
		AddLog(" ! Error: Music data size exceeded limit, NSF may not work properly !");
		AfxMessageBox("Error: Music data size exceeded limit, NSF may not work properly! Try creating a bank switched NSF instead.");
	}
}

void CCompile::SetSongInfo(char *Name, char *Artist, char *Copyright, int Speed, int Machine, bool ForcePAL, int Songs)
{
	// Fill the NSF header
	//
	// Speed will be the same for NTSC/PAL
	//

	int SpeedPAL, SpeedNTSC;

	// If speed is default, write correct NTSC/PAL speed periods
	if (Speed == 0) {
		SpeedNTSC = 1000000 / 60;
		SpeedPAL = 1000000 / 50;
	}
	else {
		// else, set the same custom speed for both
		SpeedNTSC = SpeedPAL = 1000000 / Speed;
	}

	memset(&Header, 0, 0x80);

	Header.Ident[0]			= 0x4E;
	Header.Ident[1]			= 0x45;
	Header.Ident[2]			= 0x53;
	Header.Ident[3]			= 0x4D;
	Header.Ident[4]			= 0x1A;

	Header.Version			= 0x01;
	Header.TotalSongs		= Songs + 1;
	Header.StartSong		= 1;
	Header.LoadAddr			= 0x8000;
	Header.InitAddr			= 0x8000;
	Header.PlayAddr			= 0x8003;

	strcpy((char*)&Header.SongName, Name);
	strcpy((char*)&Header.ArtistName, Artist);
	strcpy((char*)&Header.Copyright, Copyright);

	Header.Speed_NTSC		= SpeedNTSC; //0x411A; // default ntsc speed

	for (char i = 0; i < 8; i++)
		Header.BankValues[i] = 0;

	Header.Speed_PAL		= SpeedPAL; //0x4E20; // default pal speed

	if (ForcePAL)
		Header.Flags		= 0x01;
	else
		Header.Flags		= 0x02;

	Header.SoundChip		= 0x00;
	Header.Reserved[0]		= 0x00;
	Header.Reserved[1]		= 0x00;
	Header.Reserved[2]		= 0x00;
	Header.Reserved[3]		= 0x00;
}

// CPatternCompiler ///////////////////////////////////////////////////////////////////////////////////////////

CPatternCompiler::CPatternCompiler()
{
	memset(m_bDSamplesAccessed, 0, sizeof(bool) * MAX_DSAMPLES);
}

void CPatternCompiler::Write(char Value)
{
	ASSERT(m_iPointer < 0x800);
	m_cpTempData[m_iPointer++] = Value;
}

void CPatternCompiler::AccumulateZero()
{
	m_iZeroes++;
}

void CPatternCompiler::DispatchZeroes()
{
	if (m_iZeroes < 3) {
		for (unsigned int i = 0; i < m_iZeroes; i++)
			Write(0);
	}
	else {
		Write(0xFF);
		Write(m_iZeroes - 1);
	}

	m_iZeroes = 0;
}

void CPatternCompiler::Cleanup()
{
	delete [] m_cpTempData;
}

void CPatternCompiler::CompileData(CFamiTrackerDoc *pDoc, int Pattern, int Channel, unsigned char (*DPCM_LookUp)[MAX_INSTRUMENTS][8][12])
{
	unsigned int MemUsed = 0;
	unsigned int SelectedInstrument, k;

	m_cpTempData	= new char[0x800];	// I doubt more than 2 kb is needed
	m_iPointer		= 0;
	m_iZeroes		= 0;

	SelectedInstrument = MAX_INSTRUMENTS + 1;

	for (k = 0; k < pDoc->GetPatternLength(); k++) {

		unsigned char Note, Octave, Instrument, AsmNote, Volume;
		unsigned char Effect, EffParam;

		stChanNote NoteData;

		pDoc->GetDataAtPattern(Pattern, Channel, k, &NoteData);

		Note		= NoteData.Note;
		Octave		= NoteData.Octave;
		Instrument	= NoteData.Instrument;
		Volume		= NoteData.Vol;

		// Check for delays, it must come first!
		for (unsigned int n = 0; n < (pDoc->GetEffColumns(Channel) + 1); n++) {
			Effect		= NoteData.EffNumber[n];
			EffParam	= NoteData.EffParam[n];
			if (Effect == EF_DELAY && EffParam > 0) {
				DispatchZeroes();
				Write(NSF_EFF_BEGIN + 12);
				Write(EffParam);
			}
		}

		// DPCM requires instruments every time
		if (Channel == 4 && Instrument == MAX_INSTRUMENTS && Note != HALT && Note != 0) {
			Instrument = SelectedInstrument;
			DispatchZeroes();
			Write(0x80);
			Write(Instrument);
		}

		if (Note == 0) {
			AsmNote = 0;
		}
		else if (Note == HALT) {
			AsmNote = 0x7F;
		}
		else {
			if (Channel == 4) {	// DPCM	
				int LookUp = (*DPCM_LookUp)[Instrument][Octave][Note - 1];
				if (LookUp > 0) {
					AsmNote = LookUp * 2;
					int Sample = pDoc->GetInstrument(Instrument)->Samples[Octave][Note - 1] - 1;
					m_bDSamplesAccessed[Sample] = true;
				}
				else
					AsmNote = 0;
			}
			else {
				AsmNote = (Note - 1) + (Octave * 12);
			}
		}

		if (Instrument != MAX_INSTRUMENTS && Instrument != SelectedInstrument && Note != HALT) {
			// write instrument change
			SelectedInstrument = Instrument;
			DispatchZeroes();
			Write(0x80);
			Write(Instrument);
		}
		
		for (unsigned int n = 0; n < (pDoc->GetEffColumns(Channel) + 1); n++) {
			Effect		= NoteData.EffNumber[n];
			EffParam	= NoteData.EffParam[n];
			
			if (Effect > 0)
				DispatchZeroes();

			switch (Effect) {
				case EF_ARPEGGIO:
					Write(NSF_EFF_BEGIN + 0);
					Write(EffParam);
					break;
				case EF_PORTAON:
					Write(NSF_EFF_BEGIN + 1);
					Write(EffParam + 1);
					break;
				case EF_PORTAOFF:
					Write(NSF_EFF_BEGIN + 2);
					Write(EffParam);
					break;
				case EF_VIBRATO:
					Write(NSF_EFF_BEGIN + 3);
					Write(EffParam);
					break;
				case EF_TREMOLO:
					Write(NSF_EFF_BEGIN + 4);
					Write(EffParam);
					break;
				case EF_SPEED:
					Write(NSF_EFF_BEGIN + 5);
					Write(EffParam);
					break;
				case EF_JUMP:
					Write(NSF_EFF_BEGIN + 6);
					Write(EffParam + 1);
					break;
				case EF_SKIP:
					Write(NSF_EFF_BEGIN + 7);
					Write(EffParam);
					break;
				case EF_HALT:
					Write(NSF_EFF_BEGIN + 8);
					Write(EffParam);
					break;
				case EF_VOLUME:
					Write(NSF_EFF_BEGIN + 9);
					Write(EffParam);
					break;
				case EF_SWEEPUP: {
					// this need some extra work
					unsigned char Shift = EffParam & 0x07;
					unsigned char Period = ((EffParam >> 4) & 0x07) << 4;
					unsigned char Sweep = 0x88 | Shift | Period;

					/*
					unsigned char shift	= EffParam & 0x07;
					unsigned char len	= (EffParam >> 4) & 0x07;
					unsigned char Val	= 0x88 | (len << 4) | shift;
					*/
					Write(NSF_EFF_BEGIN + 10);
					Write(Sweep);
					break;
				}
				case EF_SWEEPDOWN: {
					unsigned char Shift = EffParam & 0x07;
					unsigned char Period = ((EffParam >> 4) & 0x07) << 4;
					unsigned char Sweep = 0x80 | Shift | Period;
					/*
					unsigned char shift	= EffParam & 0x07;
					unsigned char len	= (EffParam >> 4) & 0x07;
					unsigned char Val	= 0x80 | (len << 4) | shift;
					*/
					Write(NSF_EFF_BEGIN + 10);
					Write(Sweep);
					break;
				}
				case EF_PITCH:
					if (Channel < 5) {
						Write(NSF_EFF_BEGIN + 11);
						Write(EffParam);
					}
					break;
					/*
				case EF_DELAY:
					Write(NSF_EFF_BEGIN + 12);
					Write(EffParam);
					break;
					*/
				case EF_DAC:
					if (Channel == 4) {
						Write(NSF_EFF_BEGIN + 13);
						Write(EffParam & 0x7F);
					}
					break;
			}
		}

		if (Volume < 0x10) {
			DispatchZeroes();
			Write(0x81);
			Write((Volume ^ 0x0F) & 0x0F);
		}

		if (AsmNote == 0)
			AccumulateZero();
		else {
			DispatchZeroes();
			Write(AsmNote);
		}
	}

	DispatchZeroes();
}
