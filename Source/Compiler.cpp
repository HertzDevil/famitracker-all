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

// Completely rewritten

#include "stdafx.h"
#include "FamiTracker.h"
#include "Compiler.h"
#include "Driver.h"

const unsigned int TUNE_PATTERN		= 0x00;
const unsigned int BANK_SIZE		= 0x10000;
const unsigned int DPCM_BANK_SIZE	= 0x4000;

const unsigned int PAGE_SIZE		= 0x1000;		// one NSF page, 4kB

CCompiler::CCompiler()
{
}

CCompiler::~CCompiler()
{
}

CFile *LogFile = NULL;

void CCompiler::WriteLog(char *text, ...)
{
	static char buf[256];
	static DWORD CharsWritten;
    va_list argp;
    va_start(argp, text);
    if (!text)
		return;
    vsprintf(buf, text, argp);
	if (!LogFile)
		return;
	LogFile->Write(buf, (UINT)strlen(buf));
}

void CCompiler::Print(char *text, ...)
{
	static char buf[256];
	static DWORD CharsWritten;
	CString WndTxt;
    va_list argp;
    va_start(argp, text);
    if (!text)
		return;
    vsprintf(buf, text, argp);

	int len = (int)strlen(buf);

	if (buf[len - 1] == '\n') {
		buf[len - 1] = '\r';
		buf[len] = '\n';
		buf[len + 1] = 0;
	}

	m_pLogText->SetSel(m_pLogText->GetWindowTextLength(), m_pLogText->GetWindowTextLength(), 0);
	m_pLogText->ReplaceSel(buf, 0);
	m_pLogText->RedrawWindow();
}

void CCompiler::CreateHeader(CFamiTrackerDoc *pDoc, stNSFHeader *pHeader, bool EnablePAL)
{
	// Fill the NSF header
	//
	// Speed will be the same for NTSC/PAL
	//

	int SpeedPAL, SpeedNTSC, Speed;

	Speed = pDoc->GetEngineSpeed();

	// If speed is default, write correct NTSC/PAL speed periods
	if (Speed == 0) {
		SpeedNTSC = 1000000 / 60;
		SpeedPAL = 1000000 / 50;
	}
	else {
		// else, set the same custom speed for both
		SpeedNTSC = SpeedPAL = 1000000 / Speed;
	}

	memset(pHeader, 0, 0x80);

	pHeader->Ident[0]	= 0x4E;
	pHeader->Ident[1]	= 0x45;
	pHeader->Ident[2]	= 0x53;
	pHeader->Ident[3]	= 0x4D;
	pHeader->Ident[4]	= 0x1A;

	pHeader->Version	= 0x01;
	pHeader->TotalSongs	= pDoc->GetTrackCount() + 1;
	pHeader->StartSong	= 1;
	pHeader->LoadAddr	= m_iLoadAddress;
	pHeader->InitAddr	= m_iDriverLocation;
	pHeader->PlayAddr	= m_iDriverLocation + 3;

	strcpy((char*)pHeader->SongName,	pDoc->GetSongName());
	strcpy((char*)pHeader->ArtistName,	pDoc->GetSongArtist());
	strcpy((char*)pHeader->Copyright,	pDoc->GetSongCopyright());

	pHeader->Speed_NTSC		= SpeedNTSC; //0x411A; // default ntsc speed

	for (char i = 0; i < 8; i++)
		pHeader->BankValues[i] = (m_bBankSwitched ? i : 0);

	pHeader->Speed_PAL		= SpeedPAL; //0x4E20; // default pal speed

	if (EnablePAL)
		pHeader->Flags		= 0x01;
	else
		pHeader->Flags		= 0x02;

	pHeader->SoundChip		= 0x00;		// this will be filled with nice stuff later...
	pHeader->Reserved[0]	= 0x00;
	pHeader->Reserved[1]	= 0x00;
	pHeader->Reserved[2]	= 0x00;
	pHeader->Reserved[3]	= 0x00;
}

// Create the NSF file
//
void CCompiler::ExportNSF(CString FileName, CFamiTrackerDoc *pDoc, CEdit *pLogTxt, bool EnablePAL)
{
	unsigned int iOffsetDriver;
	unsigned int iOffsetMusic;
	unsigned int iOffsetDPCM;

	bool bAlternativeMode;

	CFile OutputFile;
	stNSFHeader Header;

	// Clear progress
	m_pLogText = pLogTxt;
	m_pLogText->SetWindowText("");

	LogFile = NULL;

/*
	LogFile = new CFile;

	if (!LogFile->Open("C:\\nsflog.txt", CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Could not open log file!");
		return;
	}

*/

	// Build the music data
	CompileData(pDoc);

	if (!OutputFile.Open(FileName, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Could not open output file!");
		return;
	}
	
	if (!m_bBankSwitched) {
		if ((m_iDataPointer + 0x1000) > 0x4000)
			bAlternativeMode = true;
		else
			bAlternativeMode = false;
	}
	else
		bAlternativeMode = true;

	// Define memory areas
	//
	if (bAlternativeMode) {
		m_iDriverLocation	= DRIVER2_LOCATION;
		m_iLoadAddress		= m_iDriverLocation;

		iOffsetMusic		= DRIVER_SIZE;
		iOffsetDriver		= 0;
		iOffsetDPCM			= m_iSampleStart;
	}
	else {
		m_iDriverLocation	= DRIVER1_LOCATION;
		m_iLoadAddress		= m_iDriverLocation - m_iDataPointer;

		iOffsetMusic		= 0;
		iOffsetDriver		= m_iDataPointer;
		iOffsetDPCM			= m_iDataPointer + (0xC000 - DRIVER1_LOCATION);
	}

	CreateHeader(pDoc, &Header, EnablePAL);

	unsigned char *pFileSpace;
	unsigned int iCurrentSize;

	// remove this
	pFileSpace		= new unsigned char[0x20000];
	iCurrentSize	= 0;

	if (!m_bBankSwitched) {
		if (!bAlternativeMode)
			memcpy(pFileSpace + iOffsetDriver, DRIVER_MODE1, DRIVER_SIZE);
		else
			memcpy(pFileSpace + iOffsetDriver, DRIVER_MODE2, DRIVER_SIZE);

		memcpy(pFileSpace + iOffsetMusic, m_pData, m_iDataPointer);
		iCurrentSize = m_iDataPointer + DRIVER_SIZE;

		// Last two bytes of music code points to the start of music data
		pFileSpace[iOffsetDriver + DRIVER_SIZE - 2] = (m_iLoadAddress + iOffsetMusic) & 0xFF;
		pFileSpace[iOffsetDriver + DRIVER_SIZE - 1] = (m_iLoadAddress + iOffsetMusic) >> 8;

		if (m_iDPCMSize > 0) {
			memcpy(pFileSpace + iOffsetDPCM, m_pDPCM + m_iSampleStart - 0x4000, m_iDPCMSize);
			if (bAlternativeMode) {
				iCurrentSize = iOffsetDPCM + m_iDPCMSize;
			}
			else {
				iCurrentSize += ((0xC000 - DRIVER1_LOCATION) - DRIVER_SIZE) + m_iDPCMSize;
			}
		}
	}
	else {
		// Copy driver

		iCurrentSize = iOffsetMusic - DRIVER_SIZE;

		// The first banks
		for (int i = 0; i < 4; i++) {
			m_DataBanks[i].CopyData(pFileSpace + iCurrentSize);
			iCurrentSize += 0x1000;
		}

		// Empty space for DPCM samples
		iCurrentSize += 0x4000;

		// And the rest of the music data
		for (unsigned int i = 8; i < m_iAllocatedBanks; i++) {
			m_DataBanks[i].CopyData(pFileSpace + iCurrentSize);
			iCurrentSize += 0x1000;
		}

		if (m_iDPCMSize > 0) {
			memcpy(pFileSpace + iOffsetDPCM, m_pDPCM + m_iSampleStart - 0x4000, m_iDPCMSize);
		}

		// Driver
		memcpy(pFileSpace, DRIVER_MODE2, DRIVER_SIZE);

		pFileSpace[DRIVER_SIZE - 2] = (m_iLoadAddress + iOffsetMusic) & 0xFF;
		pFileSpace[DRIVER_SIZE - 1] = (m_iLoadAddress + iOffsetMusic) >> 8;
	}

	Print(" * NSF load address: $%04X\n", m_iLoadAddress);
	Print("Writing output file...\n");
	Print(" * Driver size: %i bytes\n", DRIVER_SIZE);

	if (m_bBankSwitched) {
		Print(" * NSF type: Bankswitched (%i banks)\n", m_iAllocatedBanks);
		Print(" * Song data size: %i bytes (%i%%)\n", m_iDataPointer, (100 * m_iDataPointer) / (0x80000 - DRIVER_SIZE - m_iDPCMSize));
	}
	else {
		Print(" * Song data size: %i bytes (%i%%)\n", m_iDataPointer, (100 * m_iDataPointer) / (0x8000 - DRIVER_SIZE - m_iDPCMSize));
		if (bAlternativeMode)
			Print(" * NSF type: Linear (mode 2)\n");
		else
			Print(" * NSF type: Linear (mode 1)\n");
	}

	// Write the file
	OutputFile.Write(&Header, sizeof(stNSFHeader));
	OutputFile.Write(pFileSpace, iCurrentSize);

	Print("Done, total file size: %i bytes\n", iCurrentSize + sizeof(stNSFHeader));

	// remove
	delete [] pFileSpace;

	// Done
	OutputFile.Close();

	if (LogFile)
		LogFile->Close();
}

void CCompiler::ExportBIN(CString BIN_File, CString DPCM_File, CFamiTrackerDoc *pDoc, CEdit *pLogTxt)
{
	CFile OutputFileBIN, OutputFileDPCM;

	// Clear progress
	m_pLogText = pLogTxt;
	m_pLogText->SetWindowText("");

	LogFile = NULL;

#ifdef _DEBUG_

	LogFile = new CFile;

	if (!LogFile->Open("C:\\nsflog.txt", CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Could not open log file!");
		return;
	}

#endif

	// Build the music data
	CompileData(pDoc);

	if (m_bBankSwitched) {
		Print("Error: Can't write bankswitched songs!\n");
		return;
	}

	if (!OutputFileBIN.Open(BIN_File, CFile::modeWrite | CFile::modeCreate) || 
		!OutputFileDPCM.Open(DPCM_File, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Could not open files!");
		OutputFileBIN.Close();
		OutputFileDPCM.Close();
		return;
	}

	Print("Writing output files...\n");
	Print(" * Music data size: %i\n", m_iDataPointer);
	Print(" * DPCM size: %i\n", m_iDPCMSize);
	Print(" * DPCM location: $%04X\n", m_iSampleStart + 0x8000);

	OutputFileBIN.Write(m_pData, m_iDataPointer);
	OutputFileDPCM.Write(m_pDPCM, m_iDPCMSize);

	Print("Done, total file sizes: %i, %i bytes\n", m_iDataPointer, m_iDPCMSize);

	// Done
	OutputFileBIN.Close();
	OutputFileDPCM.Close();
}

void CCompiler::ExportPRG(CString FileName, CFamiTrackerDoc *pDoc, CEdit *pLogTxt, bool EnablePAL)
{
	// This is basically a type 2 NSF with a caller
	unsigned int iOffsetDriver;
	unsigned int iOffsetMusic;
	unsigned int iOffsetDPCM;

//	bool bAlternativeMode;

	CFile OutputFile;
//	stNSFHeader Header;

	// Clear progress
	m_pLogText = pLogTxt;
	m_pLogText->SetWindowText("");

	LogFile = NULL;

	// Build the music data
	CompileData(pDoc);

	if (!OutputFile.Open(FileName, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Could not open output file!");
		return;
	}
	
	if (m_bBankSwitched) {
		AfxMessageBox("Error: Song is too big to fit!", 0, 0);
		return;
	}

	// Define memory areas
	//
	m_iDriverLocation	= DRIVER2_LOCATION;
	m_iLoadAddress		= m_iDriverLocation;

	iOffsetMusic		= DRIVER_SIZE;
	iOffsetDriver		= 0;
	iOffsetDPCM			= m_iSampleStart;

//	CreateHeader(pDoc, &Header, EnablePAL);

	unsigned char *pFileSpace;
	unsigned int iCurrentSize;

	// remove this
	pFileSpace		= new unsigned char[0x8000];
	iCurrentSize	= 0;

	memcpy(pFileSpace, DRIVER_MODE2, DRIVER_SIZE);
	memcpy(pFileSpace + DRIVER_SIZE, m_pData, m_iDataPointer);
	iCurrentSize = m_iDataPointer + DRIVER_SIZE;

	// Last two bytes of music code points to the start of music data
	pFileSpace[iOffsetDriver + DRIVER_SIZE - 2] = (m_iLoadAddress + iOffsetMusic) & 0xFF;
	pFileSpace[iOffsetDriver + DRIVER_SIZE - 1] = (m_iLoadAddress + iOffsetMusic) >> 8;

	if (m_iDPCMSize > 0) {
		memcpy(pFileSpace + iOffsetDPCM, m_pDPCM + m_iSampleStart - 0x4000, m_iDPCMSize);
		iCurrentSize = iOffsetDPCM + m_iDPCMSize;
	}

	// Finally add the caller to the end
	memcpy(pFileSpace + 0x8000 - NSF_CALLER_SIZE, NSF_CALLER_BIN, NSF_CALLER_SIZE);

	pFileSpace[0x7FD8] = (EnablePAL ? 0x01 : 0x00);
	pFileSpace[0x7FDA] = 0x00;	// init
	pFileSpace[0x7FDB] = 0x80;
	pFileSpace[0x7FF2] = 0x03;	// play
	pFileSpace[0x7FF3] = 0x80;

	Print("Writing output file...\n");
	Print(" * Driver size: %i bytes\n", DRIVER_SIZE);
	Print(" * Song data size: %i bytes (%i%%)\n", m_iDataPointer, (100 * m_iDataPointer) / (0x8000 - DRIVER_SIZE - m_iDPCMSize));

	// Write the file
	OutputFile.Write(pFileSpace, 0x8000);

	Print("Done, total file size: %i bytes\n", 0x8000);

	// remove
	delete [] pFileSpace;

	// Done
	OutputFile.Close();
}

void CCompiler::Export(CString FileName, CFamiTrackerDoc *pDoc, CEdit *pLogTxt)
{
	CFile ExportFile, SampleFile;

	m_pLogText = pLogTxt;
	m_pLogText->SetWindowText("");

	LogFile = new CFile;

	if (!LogFile->Open("C:\\nsflog.txt", CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Could not open log file!");
		return;
	}

	m_iAllocatedBanks = 0;
	m_bErrorFlag	= false;
	m_bBankSwitched = true;

	AssembleData(pDoc);

	// And dump into files
	if (!ExportFile.Open(FileName + ".bin", CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Could not open output file!");
		LogFile->Close();
		return;
	}

	if (!SampleFile.Open(FileName + ".dmc", CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Could not open output file!");
		LogFile->Close();
		return;
	}

	ExportFile.Write(m_pData, m_iDataPointer);
	SampleFile.Write(m_pDPCM, m_iDPCMSize);

	// All done!
	ExportFile.Close();
	SampleFile.Close();
	LogFile->Close();

	delete LogFile;
}

void CCompiler::CompileData(CFamiTrackerDoc *pDoc)
{
	// Assembles music data, first without bank switching and then with (if necessary)

	CString SavedString;

	m_pLogText->GetWindowText(SavedString);

	m_iAllocatedBanks = 0;

	//
	// First try, no bank switching
	//

	m_bErrorFlag	= false;
	m_bBankSwitched = false;
	AssembleData(pDoc);

	if (m_bErrorFlag) {

		//
		// Second try, bank switching
		//

		RemoveSpace();

		m_pLogText->SetWindowText(SavedString);		// clear progress text

		m_bErrorFlag	= false;
		m_bBankSwitched = true;
		AssembleData(pDoc);
	}
}

void CCompiler::AssembleData(CFamiTrackerDoc *pDoc)
{
	unsigned int i;

	// Begin log
	WriteLog("Famitracker NSF compiler log file\n");
	WriteLog("---------------------------------\n");

	Print("Building music data...\n");

	if (m_bBankSwitched) {
		// Always setup the 32 kB area
		for (i = 0; i < 8; i++)
			AllocateNewBank(i * PAGE_SIZE);

		SetInitialPosition(DRIVER_SIZE);
		SetMemoryPosition(DRIVER_SIZE);
	}
	else {
		SetInitialPosition(0);
		SetMemoryPosition(0);
	}

	// Allocate space
	AllocateSpace();

	// Get song parameters
	ScanSong(pDoc);

	m_iMasterHeaderAddr = GetCurrentOffset();

	// Save space for header
	SkipBytes(6 * 2);

	// Sequences
	CreateSequenceList(pDoc);
	// Instruments
	CreateInstrumentList(pDoc);
	// DPCM
	CreateDPCMList(pDoc);
	// Then loop through and store all songs
	StoreSongs(pDoc);
	// Store DPCMs
	StoreDPCM(pDoc);
	// Write the tune header
	WriteHeader(&m_stTuneHeader, pDoc);

	// And song is done

	// Finish log
	WriteLog("---------------------------------\n");
	WriteLog("NSF data size: %i bytes\n", m_iDataPointer);
	WriteLog("DPCM area size: %i bytes\n", m_iDPCMSize);
	WriteLog("---------- End of file ----------");
}

void CCompiler::AllocateSpace()
{
	m_pData = new unsigned char[BANK_SIZE];
//	m_iDataPointer = 0;

	m_pDPCM = new unsigned char[DPCM_BANK_SIZE];
	m_iDPCMSize = 0;

	memset(m_bSamplesAccessed, 0, MAX_INSTRUMENTS * OCTAVE_RANGE * NOTE_RANGE * sizeof(bool));
}

void CCompiler::RemoveSpace()
{
	delete [] m_pData;
	delete [] m_pDPCM;
}

void CCompiler::ScanSong(CFamiTrackerDoc *pDoc)
{
	// Scan song for what's needed
	unsigned int i;

	m_iInstruments = 0;

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (pDoc->IsInstrumentUsed(i)) {
			m_iAssignedInstruments[m_iInstruments++] = i;
		}
	}
}

// Bank handling //

void CCompiler::AllocateNewBank(unsigned int iAddress)
{
	// Allocates a new bank for the desired address
	unsigned int iBankSlot;

	if (!m_bBankSwitched)
		return;

	iBankSlot		= (iAddress & 0x7FFF) >> 12;
	m_iDataPointer	= iAddress & 0x7FFF;

	ASSERT(iBankSlot < MAX_BANKS);

	// Setup
	m_DataBanks[m_iAllocatedBanks].AllocateBank(iAddress);

	// Save this bank
	m_cSelectedBanks[iBankSlot] = m_iAllocatedBanks;

	m_iAllocatedBanks++;
}

void CCompiler::SetMemoryPosition(unsigned int iAddress)
{
	// Sets current memory position
	m_iDataPointer = iAddress /*+ m_iInitialPosition*/;
}

void CCompiler::SetMemoryOffset(unsigned int iOffset)
{
	m_iDataPointer = iOffset + m_iInitialPosition;
}

void CCompiler::SetInitialPosition(unsigned int iAddress)
{
	// Sets initial position in memory
	m_iInitialPosition = iAddress;
}

void CCompiler::SkipBytes(unsigned int iBytes)
{
	// Skips some bytes
	m_iDataPointer += iBytes;
}

void CCompiler::StoreByte(unsigned char Value)
{
	// Store a byte in memory

	if (m_bBankSwitched) {
		// Bankswitched space
		unsigned int iBank, iSlot;
		
//		m_iDataPointer += DRIVER_SIZE;

		// Find the bank used in this address
		iSlot = (m_iDataPointer & 0x7FFF) >> 12;
		iBank = m_cSelectedBanks[iSlot];

		m_DataBanks[iBank].WriteByte(m_iDataPointer, Value);

//		m_iDataPointer -= DRIVER_SIZE;

		m_iDataPointer++;
	}
	else {
		// Linear space

		if (m_bErrorFlag)
			return;

		ASSERT(m_iDataPointer < BANK_SIZE);
		m_pData[m_iDataPointer++] = Value;

		// Overflow omg! :O
		if (m_iDataPointer >= (0x8000 - DRIVER_SIZE))
			m_bErrorFlag = true;
	}
}

void CCompiler::StoreShort(unsigned short Value)
{
	// Store a short in memory
	StoreByte(Value & 0xFF);
	StoreByte(Value >> 8);
}

unsigned int CCompiler::GetCurrentOffset()
{
	// Returns current offset from initial position
	return m_iDataPointer - m_iInitialPosition;
}

// Headers //

void CCompiler::WriteHeader(stTuneHeader *pHeader, CFamiTrackerDoc *pDoc)
{
	// Writes the music header
	unsigned int SavedPointer = m_iDataPointer;

	int TicksPerSec, Machine, Tempo;

	TicksPerSec = pDoc->GetEngineSpeed();
	Machine		= pDoc->GetMachine();

	if (TicksPerSec == 0) {
		Tempo		= (pDoc->GetFrameRate() * 5) / 2;
		pHeader->DividerNTSC = FRAMERATE_NTSC * 60;
		pHeader->DividerPAL	= FRAMERATE_PAL * 60;
	}
	else {
		Tempo = (TicksPerSec * 5) / 2;
		pHeader->DividerNTSC = pHeader->DividerPAL = TicksPerSec * 60;
	}

	SetMemoryOffset(m_iMasterHeaderAddr);

	// Write header
	StoreShort(pHeader->SongListOffset);
	StoreShort(pHeader->InstrumentListOffset);
	StoreShort(pHeader->DPCMInstListOffset);
	StoreShort(pHeader->DPCMSamplesOffset);
	StoreShort(pHeader->DividerNTSC);
	StoreShort(pHeader->DividerPAL);

	m_iDataPointer = SavedPointer;
}

void CCompiler::WriteTrackHeader(stSongHeader *pTrackHeader, unsigned int iAddress)
{
	// Writes a song header
	unsigned int SavedPointer = m_iDataPointer;

	SetMemoryOffset(iAddress);

	StoreShort(pTrackHeader->FrameListOffset);
	StoreByte(pTrackHeader->FrameCount);
	StoreByte(pTrackHeader->PatternLength);
	StoreByte(pTrackHeader->Speed);
	StoreByte(pTrackHeader->Tempo);

	m_iDataPointer = SavedPointer;

	WriteLog(" -- Track header for track %i (@ %04X) --\n", NULL, iAddress);
	WriteLog("Frame list offset: %04X\n", pTrackHeader->FrameListOffset);
	WriteLog("Frame count: %i\n", pTrackHeader->FrameCount);
	WriteLog("Pattern length: %i\n", pTrackHeader->PatternLength);
	WriteLog("Song speed / tempo: %i / %i\n", pTrackHeader->Speed, pTrackHeader->Tempo);
}

// Song conversion functions //

bool CCompiler::IsPatternAddressed(CFamiTrackerDoc *pDoc, int Pattern, int Channel)
{
	// Scan the frame list to see if a pattern is accessed for that frame
	for (unsigned int i = 0; i < pDoc->GetFrameCount(); i++) {
		if (pDoc->GetPatternAtFrame(i, Channel) == Pattern)
			return true;
	}
	return false;
}

void CCompiler::StoreSongs(CFamiTrackerDoc *pDoc)
{
	/*
	 * Store patterns and frames for each song
	 * 
	 */

	unsigned int iSongCount = pDoc->GetTrackCount() + 1, i;
	unsigned int iFrameCount = pDoc->GetFrameCount();
	unsigned int iChannelCount = pDoc->GetAvailableChannels();
	unsigned int iHeaderList = GetCurrentOffset() + (iSongCount * 2);
	unsigned int iSavedPointer;

	m_stTuneHeader.SongListOffset = GetCurrentOffset();

	// Store track header pointers
	for (i = 0; i < iSongCount; i++)
		StoreShort(iHeaderList + i * 6);

	// Allocate space for track headers
	SkipBytes(iSongCount * 6);

	// Store track headers
	for (i = 0; i < iSongCount; i++) {
		WriteLog("---------------------------------------------------------\n");
		WriteLog(" Storing song %i\n", i);
		WriteLog("---------------------------------------------------------\n");
		pDoc->SelectTrack(i);

		iFrameCount = pDoc->GetFrameCount();
		iChannelCount = pDoc->GetAvailableChannels();

		// Allocate space for the frame list
		m_iFrameListAddress = GetCurrentOffset();		// Save the address to this list
		SkipBytes(iFrameCount * iChannelCount * 3 + iFrameCount * 2);

		// Store pattern data
		StorePatterns(pDoc, i);

		// Write the frame list
		iSavedPointer = GetCurrentOffset();
		SetMemoryOffset(m_iFrameListAddress);
		CreateFrameList(pDoc);
		SetMemoryOffset(iSavedPointer);

		// Fill a track header
		m_stTrackHeader.FrameListOffset	= m_iFrameListAddress;
		m_stTrackHeader.FrameCount		= pDoc->GetFrameCount();
		m_stTrackHeader.PatternLength	= pDoc->GetPatternLength();
		m_stTrackHeader.Speed			= pDoc->GetSongSpeed();
		m_stTrackHeader.Tempo			= pDoc->GetSongTempo();
		// and print it
		WriteTrackHeader(&m_stTrackHeader, iHeaderList + i * 6);
	}
}

void CCompiler::CreateFrameList(CFamiTrackerDoc *pDoc)
{
	/*
	 * Creates a frame list
	 *
	 * The pointer list is just pointing to each item in the frame list
	 * and the frame list holds the offset addresses for the patterns for all channels
	 *
	 * ---------------------
	 *  Frame entry pointers
	 *  $XXXX (4 bytes, offset to a frame entry)
	 *  ...
	 * ---------------------
	 *
	 * ---------------------
	 *  Frame entries
	 *  $XXXX * 4 (2 * 4 bytes, each pair is an offset to the pattern)
	 * ---------------------
	 *
	 */
	
	unsigned int	iFrameCount, iChannelCount, i, j, iOffset;
	unsigned short	iPattern, iPointer, iChannelItem;
	unsigned char	iBank;

	WriteLog(" -- Filling the frame list -- \n");

	iFrameCount		= pDoc->GetFrameCount();
	iChannelCount	= pDoc->GetAvailableChannels();
	iChannelItem	= iChannelCount * 3;
	iOffset			= (iFrameCount * 2) + GetCurrentOffset();

	// Setup addresses for the pointer list
	for (i = 0; i < iFrameCount; i++)
		StoreShort(iOffset + i * iChannelItem);

	// Store addresses to patterns
	for (i = 0; i < iFrameCount; i++) {
		WriteLog("Frame %i: ", i);

		for (j = 0; j < iChannelCount; j++) {
			iPattern	= pDoc->GetPatternAtFrame(i, j);
			iPointer	= m_iPatternAddresses[iPattern][j];
			iBank		= m_iPatternBanks[iPattern][j];

			// Zero will disable bank writes
			if (!m_bBankSwitched)
				iBank = 0;

			StoreShort(iPointer);
			StoreByte(iBank);
			WriteLog("%04X (%i) ", iPointer, iBank);

//			Print("frame %i(%i): %i\n", i, j, iBank);
		}

		WriteLog("\n");
	}
}

void CCompiler::StorePatterns(CFamiTrackerDoc *pDoc, unsigned int Track)
{
	/* 
	 * Store patterns and save references to them for the frame list
	 * 
	 */

	unsigned int	iSize, i, k, j, iPatternCount = 0;
	unsigned int	iTotalSize = 0, iTotalSizeCompressed = 0;	// for debugging
	unsigned int	iChannels;
	bool			bPatternsUsed[MAX_PATTERN][MAX_CHANNELS];

	iChannels = pDoc->GetAvailableChannels();

	WriteLog(" -- Pattern data -- \n");

	// Find out what patterns that are used
	for (i = 0; i < MAX_PATTERN; i++) {
		for (j = 0; j < pDoc->GetAvailableChannels(); j++)
			bPatternsUsed[i][j] = IsPatternAddressed(pDoc, i, j);
	}

	// Iterate through all patterns
	for (j = 0; j < iChannels; j++) {
		for (i = 0; i < MAX_PATTERN; i++) {
			// And choose only used ones
			if (bPatternsUsed[i][j]) {

				// Compile pattern data
				m_oPatternCompiler.CompileData(pDoc, Track, i, j, &m_iDPCM_LookUp);

				// Get size
				iSize = m_oPatternCompiler.GetDataSize();
				iTotalSize += iSize;

				int AbsoluteAddress = m_iDataPointer + /*DRIVER_SIZE +*/ 0x8000;

				// Check if current bank is full 
				if (m_bBankSwitched) {
					//if ((m_iDataPointer + iSize) >= 0x4000)
					if ((AbsoluteAddress + iSize) > 0xBFFF) {
						AllocateNewBank(0xB000);
					}
				}

				// Save the address for this pattern
				m_iPatternAddresses[i][j] = GetCurrentOffset();

				// Since only the last bank is switched, we need to store the bank
				// where this pattern ends to make sure it's completely available
				int iBankIndex = ((AbsoluteAddress + iSize) & 0x7FFF) >> 12;
				
				if (iBankIndex > 3)
					iBankIndex = 3;

				m_iPatternBanks[i][j] = m_cSelectedBanks[iBankIndex];

 				WriteLog("Pattern %02X, channel %i: { ", i, j);

				// And store it in the memory
				for (k = 0; k < iSize; k++) {
					StoreByte(m_oPatternCompiler.GetData(k));
					WriteLog("%02X ", m_oPatternCompiler.GetData(k));
				}

				WriteLog("}\n");

				iPatternCount++;
/*
				iSize = m_oPatternCompiler.GetCompressedDataSize();
				iTotalSizeCompressed += iSize;

				WriteLog("Pattern C %02X, channel %i: { ", i, j);
				for (k = 0; k < iSize; k++) {
					//WriteByte(m_oPatternCompiler.GetCompressedData(k));
					WriteLog("%02X ", m_oPatternCompiler.GetCompressedData(k));
				}
				WriteLog("}\n");
				*/
			}
		}
	}

	WriteLog(" ----------------- \n");
	WriteLog("Total size uncompressed: %i\n", iTotalSize);
	WriteLog("Total size compressed: %i\n", iTotalSizeCompressed);
	
	if (iTotalSize > 0)
		WriteLog("Ratio of compression: %i%%\n", 100 - (iTotalSizeCompressed * 100) / iTotalSize);

	Print(" * Song %i, %i patterns (%i bytes)\n", Track + 1, iPatternCount, iTotalSize);
}

void CCompiler::CreateSequenceList(CFamiTrackerDoc *pDoc)
{
	/*
	 * Create sequence lists
	 *
	 *
	 */

	unsigned int	i, j, k, Size = 0, StoredCount = 0, iLoopPoint;
	unsigned char	iItemCount;
	CSequence		*pSeq;
	
	WriteLog(" -- Sequences -- \n");

	for (i = 0; i < MAX_SEQUENCES; i++) {
		for (j = 0; j < MOD_COUNT; j++) {
			pSeq = pDoc->GetSequence(i, j);
			if (pSeq->GetItemCount() > 0) {
				StoredCount++;
				WriteLog("Sequence %i (%04X): ", i, m_iDataPointer);

				// Sequences cannot be stored on addresses where the low byte is zero
				// because that indicates no sequence
				if ((GetCurrentOffset() & 0xFF) == 0)
					StoreByte(0);

				// Save a pointer to this
				m_iSequenceAddresses[i][j] = GetCurrentOffset();

				// Store the sequence
				iItemCount	= pSeq->GetItemCount();
				iLoopPoint	= pSeq->GetLoopPoint();

				StoreByte((unsigned char)iItemCount);
				StoreByte((unsigned char)iLoopPoint);

				Size += 2;

				WriteLog("%02X %02X ", iItemCount, (unsigned char)iLoopPoint);
				WriteLog("{ ");

				for (k = 0; k < iItemCount; k++) {
					StoreByte(pSeq->GetItem(k));
					Size++;
					if (k == iLoopPoint)
						WriteLog("| ");
					WriteLog("%02X ", pSeq->GetItem(k));
				}
				switch (j) {
					case MOD_VOLUME:	WriteLog("} (Volume)\n");		break;
					case MOD_ARPEGGIO:	WriteLog("} (Arpeggio)\n");		break;
					case MOD_PITCH:		WriteLog("} (Pitch)\n");		break;
					case MOD_HIPITCH:	WriteLog("} (Hi Pitch)\n");		break;
					case MOD_DUTYCYCLE:	WriteLog("} (Duty cycle)\n");	break;
				}
			}
			else {
				// No sequence
				m_iSequenceAddresses[i][j] = 0;
			}
		}
	}

	Print(" * Sequences used: %i (%i bytes)\n", StoredCount, Size);
}

void CCompiler::CreateInstrumentList(CFamiTrackerDoc *pDoc)
{
	/*
	 * Create the instrument list
	 *
	 * Sequences must have been done before here
	 *
	 */

	CInstrument2A03 *pInst;
	unsigned int i, j, iIndex, iAddress, iOffset, iListPointer;	

	WriteLog(" -- Instrument list -- \n");

	m_stTuneHeader.InstrumentListOffset = GetCurrentOffset();
	
	iListPointer	= GetCurrentOffset();
	iOffset			= (m_iInstruments * 2) + GetCurrentOffset();

	// Setup addresses for the instruments
	for (i = 0; i < m_iInstruments; i++)
		StoreShort(iOffset + i * (MOD_COUNT * 2));

	// Store instruments
	for (i = 0; i < m_iInstruments; i++) {

		WriteLog("Instrument %i: { ", i);
		for (j = 0; j < MOD_COUNT; j++) {
			iIndex		= m_iAssignedInstruments[i];
			pInst		= (CInstrument2A03*)pDoc->GetInstrument(iIndex);
			iAddress	= (pInst->GetModEnable(j) == 0) ? 0 : m_iSequenceAddresses[pInst->GetModIndex(j)][j];

			StoreShort(iAddress);
			WriteLog("%04X ", iAddress);
		}

		// Print name to log file
		char Name[256];
		pInst->GetName(Name);
		WriteLog("} %s\n", Name);
	}

	Print(" * Instruments used: %i (%i bytes)\n", m_iInstruments, m_iInstruments * MOD_COUNT * 2);
}

int CCompiler::GetSampleIndex(int SampleNumber)
{
	// Returns a sample pos from the sample bank
	for (int i = 0; i < MAX_DSAMPLES; i++) {
		if (m_iSampleBank[i] == SampleNumber)
			return i;
		else if(m_iSampleBank[i] == 0xFF) {
			m_iSampleBank[i] = SampleNumber;
			m_iSamplesUsed++;
			return i;
		}
	}

	// Fail if getting here!!!
	return SampleNumber;
}

bool CCompiler::IsSampleAccessed(int SampleNumber)
{
	for (int i = 0; i < MAX_DSAMPLES; i++) {
		if (m_iSampleBank[i] == SampleNumber)
			return true;
	}

	return false;
}

void CCompiler::CreateDPCMList(CFamiTrackerDoc *pDoc)
{
	/*
	 * DPCM instrument list
	 *
	 */

	CInstrument2A03 *pInst;
	stChanNote		Note;

	unsigned char cSample, cSamplePitch;
	unsigned int i, j, k, Item = 1, iIndex, iSampleIndex;
	unsigned int TrackCount = pDoc->GetTrackCount() + 1;
	unsigned int Instrument = 0;

	m_iSamplesUsed = 0;

	WriteLog(" -- DPCM instrument list -- \n");

	// Scan the entire song to see what samples are used
	for (k = 0; k < TrackCount; k++) {
		for (i = 0; i < MAX_PATTERN; i++) {
			for (j = 0; j < pDoc->GetPatternLength(); j++) {
				pDoc->GetDataAtPattern(k, i, 4, j, &Note);
				if (Note.Note > 0) {
					if (Note.Instrument < 0x40)
						Instrument = Note.Instrument;
					m_bSamplesAccessed[Instrument][Note.Octave][Note.Note - 1] = true;
				}
			}
		}
	}

	// Clear the sample list
	memset(m_iSampleBank, 0xFF, MAX_DSAMPLES);
	
	m_stTuneHeader.DPCMInstListOffset = GetCurrentOffset() - 2;

	// Store definitions
	for (i = 0; i < m_iInstruments; i++) {
		iIndex	= m_iAssignedInstruments[i];
		pInst	= (CInstrument2A03*)pDoc->GetInstrument(iIndex);
		for (j = 0; j < OCTAVE_RANGE; j++) {
			for (k = 0; k < NOTE_RANGE; k++) {
				if ((pInst->GetSample(j, k) > 0) && m_bSamplesAccessed[iIndex][j][k]) {
					cSample			= pInst->GetSample(j, k) - 1;
					cSamplePitch	= pInst->GetSamplePitch(j, k);
					cSamplePitch	|= (cSamplePitch & 0x80) >> 1;	// Still the same
					iSampleIndex	= GetSampleIndex(cSample);
					// Save a reference to this item
					m_iDPCM_LookUp[i][j][k] = Item++;
					StoreByte(cSamplePitch);
					StoreByte(iSampleIndex << 1);
					WriteLog("Sample: %i (%i), pitch %i (instrument %i)\n", cSample, iSampleIndex, cSamplePitch, iIndex);
				}
				else
					// No instrument here
					m_iDPCM_LookUp[i][j][k] = 0;
			}
		}
	}
	
	// Allocate space for pointers
	m_stTuneHeader.DPCMSamplesOffset = GetCurrentOffset();
	SkipBytes(m_iSamplesUsed * 2);
}

void CCompiler::StoreDPCM(CFamiTrackerDoc *pDoc)
{
	/*
	 * DPCM sample list & DPCM sample bank
	 *
	 */ 

	CDSample *pDSample;

	unsigned char cSampleAddr, cSampleSize;
	unsigned int i, j, iListAddress, iAddedSamples = 0, iIndex;
	unsigned int iSampleAddress, iSize;
	unsigned int iDataPosition, iSavedPointer;
	bool Overflow = false;

	WriteLog(" -- Saving DPCM bank -- \n");
	
	if (m_bErrorFlag)
		return;

	iListAddress	= m_stTuneHeader.DPCMSamplesOffset;
	iDataPosition	= GetCurrentOffset() + DRIVER_SIZE;

	// Align to DPCM bank or nearest valid sample address
	if (iDataPosition > 0x4000 && !m_bBankSwitched)
		iSampleAddress = (((iDataPosition >> 6) + 1) << 6) - 0x4000;
	else
		iSampleAddress = 0x0000;

	// Get sample start address
	m_iSampleStart = 0x4000 + iSampleAddress;

	iSavedPointer = m_iDataPointer;
	SetMemoryOffset(iListAddress);

	// Store DPCM samples in a separate array
	for (i = 0; i < m_iSamplesUsed; i++) {
		if ((iIndex = m_iSampleBank[i]) == 0xFF || Overflow)
			break;
		pDSample	= pDoc->GetDSample(iIndex);
		iSize		= pDSample->SampleSize;
		if (iSize > 0) {
			if (iSampleAddress + iSize > 0x4000) {
				Overflow = true;
				//break;
				iSize = 0x4000 - iSampleAddress;
			}
			// Fill sample list
			cSampleAddr = iSampleAddress >> 6;
			cSampleSize = iSize >> 4;
			StoreByte(cSampleAddr);
			StoreByte(cSampleSize);

			iListAddress += 2;
			iAddedSamples++;

			// Move the sample
			for (j = 0; j < iSize; j++)
				m_pDPCM[iSampleAddress++] = pDoc->GetSampleData(iIndex, j);

			// Align
			if ((iSampleAddress & 0x3F) > 0)
				iSampleAddress += 0x40 - (iSampleAddress & 0x3F);

			WriteLog("Sample %i: address: %02X, size: %02X\n", iIndex, cSampleAddr, cSampleSize);
		}
	}

	m_iDataPointer	= iSavedPointer;
	m_iDPCMSize		= iSampleAddress;

	if (Overflow) {
		WriteLog("DPCM overflow\n");
		Print("Warning: DPCM overflow!\n");
	//	m_bErrorFlag = true;
	}

	Print(" * Samples located at: $%04X\n", m_iSampleStart + 0x8000);
	Print(" * DPCM samples used: %i (%i bytes)\n", m_iSamplesUsed, m_iDPCMSize);
}

/*
 * class CDataBank
 */

CDataBank::CDataBank()
{
	m_pData = NULL;
}

CDataBank::~CDataBank()
{
	if (!m_pData)
		delete [] m_pData;
}

void CDataBank::AllocateBank(unsigned char Origin)
{
	m_iPointer	= 0;
	m_iOrigin	= Origin;
	m_pData		= new unsigned char[0x1000];
	memset(m_pData, 0xFF, 0x1000);
}

void CDataBank::WriteByte(unsigned int Pointer, unsigned char Value)
{
	m_pData[Pointer & 0xFFF] = Value;
}

void CDataBank::WriteShort(unsigned int Pointer, unsigned short Value)
{
	WriteByte(Pointer, Value & 0xFF);
	WriteByte(Pointer + 1, Value >> 8);
	/*
	m_pData[m_iPointer++] = Value & 0xFF;
	m_pData[m_iPointer++] = Value >> 8;*/
}

bool CDataBank::Overflow(unsigned int iSize)
{
	return (m_iPointer + iSize >= 0x1000);
}

unsigned char *CDataBank::GetData()
{
	return m_pData;
}

void CDataBank::CopyData(unsigned char *pToArray)
{
	memcpy(pToArray, m_pData, 0x1000);
}