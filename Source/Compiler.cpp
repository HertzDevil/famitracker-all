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

/*
   This file takes care of module to NSF data translation.
*/

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "Compiler.h"
#include "Driver.h"
#include "SoundGen.h"

//#define DISABLE_DPCM	// testing
//#define COMPRESSION

//const unsigned int TUNE_PATTERN		= 0x00;
const unsigned int BANK_SIZE		= 0x10000;
const unsigned int DPCM_BANK_SIZE	= 0x4000;

const unsigned int PAGE_SIZE		= 0x1000;		// size of NSF pages, 4kB

// Define channel maps. The important one is 4 (DPCM)
const int CHAN_ORDER_DEFAULT[]	= {0, 1, 2, 3, 4};
const int CHAN_ORDER_VRC6[]		= {0, 1, 2, 3, 5, 6, 7, 4};
const int CHAN_ORDER_MMC5[]		= {0, 1, 2, 3, 5, 6, 4};
const int CHAN_ORDER_VRC7[]		= {0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 4};
const int CHAN_ORDER_FDS[]		= {0, 1, 2, 3, 5, 4};

const char *LOGFILE = "D:\\nsflog.txt";


/**
 * class CDataBank
 *
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
	memset(m_pData, 0xAB, 0x1000);
}

void CDataBank::WriteByte(unsigned int Pointer, unsigned char Value)
{
	m_pData[Pointer & 0xFFF] = Value;
}

void CDataBank::WriteShort(unsigned int Pointer, unsigned short Value)
{
	WriteByte(Pointer, Value & 0xFF);
	WriteByte(Pointer + 1, Value >> 8);
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

/**
 * CCompiler
 *
 */

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

	pHeader->Speed_NTSC = SpeedNTSC; //0x411A; // default ntsc speed

	for (char i = 0; i < 8; i++) {
		pHeader->BankValues[i] = (m_bBankSwitched ? i : 0);
	}

	pHeader->Speed_PAL = SpeedPAL; //0x4E20; // default pal speed

	// Allow PAL or dual tunes only if no expansion chip is selected
	// Expansion chips weren't available in PAL areas
	if (pDoc->GetExpansionChip() == SNDCHIP_NONE) {
		if (EnablePAL)
			pHeader->Flags = 0x01;
		else
			pHeader->Flags = 0x02;
	}
	else {
		pHeader->Flags = 0x00;
	}

	// Expansion chip
	switch (pDoc->GetExpansionChip()) {
		case SNDCHIP_NONE:
			pHeader->SoundChip = 0x00;
			m_pChanOrder = CHAN_ORDER_DEFAULT;
			break;
		case SNDCHIP_VRC6:
			pHeader->SoundChip = SNDCHIP_VRC6;
			m_pChanOrder = CHAN_ORDER_VRC6;
			break;
		case SNDCHIP_MMC5:
			pHeader->SoundChip = SNDCHIP_MMC5;
			m_pChanOrder = CHAN_ORDER_MMC5;
			break;
		case SNDCHIP_VRC7:
			pHeader->SoundChip = SNDCHIP_VRC7;
			m_pChanOrder = CHAN_ORDER_VRC7;
			break;
		case SNDCHIP_FDS:
			pHeader->SoundChip = SNDCHIP_FDS;
			m_pChanOrder = CHAN_ORDER_FDS;
			break;
	}

	pHeader->Reserved[0] = 0x00;
	pHeader->Reserved[1] = 0x00;
	pHeader->Reserved[2] = 0x00;
	pHeader->Reserved[3] = 0x00;
}

// Create the NSF file
//
void CCompiler::ExportNSF(CString FileName, CFamiTrackerDoc *pDoc, CEdit *pLogTxt, bool EnablePAL)
{
	const char *pDriver;
	char *pCustomDriver;

	unsigned int iOffsetDriver;
	unsigned int iOffsetMusic;
	unsigned int iOffsetDPCM;

	bool bAlternativeMode;

	CFile OutputFile;
	stNSFHeader Header;

	m_pDocument = pDoc;

	// Clear progress
	m_pLogText = pLogTxt;
	m_pLogText->SetWindowText("");

	LogFile = NULL;

#ifdef _DEBUG
	LogFile = new CFile;

	if (!LogFile->Open(LOGFILE, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Could not open log file!");
		LogFile = NULL;
//		return;
	}
#endif

	// Build the music data
	CompileData(pDoc);

	if (!OutputFile.Open(FileName, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Could not open output file!");
		return;
	}
	
	if (!m_bBankSwitched) {
		//if ((m_iDataPointer + 0x1000) > 0x4000)
		if ((DRIVER1_LOCATION - m_iDataPointer) < 0x8000)
			bAlternativeMode = true;
		else
			bAlternativeMode = false;
	}
	else
		bAlternativeMode = true;

	if (pDoc->GetExpansionChip() != SNDCHIP_NONE)
		bAlternativeMode = true;

	// Define memory areas
	//
	if (bAlternativeMode || m_bBankSwitched) {
		m_iDriverLocation	= DRIVER2_LOCATION;
		m_iLoadAddress		= m_iDriverLocation;

		iOffsetMusic		= m_iDriverSize;
		iOffsetDriver		= 0;
		iOffsetDPCM			= m_iSampleStart;

		// Bank switched songs also use mode 2
		switch (pDoc->GetExpansionChip()) {
			case SNDCHIP_NONE: pDriver = DRIVER_MODE2; break;
			case SNDCHIP_VRC6: pDriver = DRIVER_VRC6; break;
			case SNDCHIP_MMC5: pDriver = DRIVER_MMC5; break;
			case SNDCHIP_VRC7: pDriver = DRIVER_VRC7; break;
			case SNDCHIP_FDS: pDriver = DRIVER_FDS; break;
		}
	}
	else {
		m_iDriverLocation	= DRIVER1_LOCATION;
		m_iLoadAddress		= m_iDriverLocation - m_iDataPointer;

		iOffsetMusic		= 0;
		iOffsetDriver		= m_iDataPointer;
		iOffsetDPCM			= m_iDataPointer + (0xC000 - DRIVER1_LOCATION);

		pDriver = DRIVER_MODE1;
	}

	// Copy the vibrato table, the stored one only works for new vibrato mode
	pCustomDriver = new char[m_iDriverSize];
	memcpy(pCustomDriver, pDriver, m_iDriverSize);
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();
	for (int i = 0; i < 256; ++i) {
		*(pCustomDriver + m_iDriverSize - 258 + i) = (char)pSoundGen->ReadVibTable(i);
	}

	CreateHeader(pDoc, &Header, EnablePAL);

	unsigned char *pFileSpace;
	unsigned int iCurrentSize;

	// remove this
	pFileSpace = new unsigned char[0x80000];
	memset(pFileSpace, 0, 0x80000);
	iCurrentSize = 0;

	if (!m_bBankSwitched) {
//		if (!bAlternativeMode)
//			memcpy(pFileSpace + iOffsetDriver, pDriver, m_iDriverSize);
//		else
			memcpy(pFileSpace + iOffsetDriver, pCustomDriver, m_iDriverSize);

		memcpy(pFileSpace + iOffsetMusic, m_pData, m_iDataPointer);
		iCurrentSize = m_iDataPointer + m_iDriverSize;

		// Last two bytes of music code points to the start of music data
		pFileSpace[iOffsetDriver + m_iDriverSize - 2] = (m_iLoadAddress + iOffsetMusic) & 0xFF;
		pFileSpace[iOffsetDriver + m_iDriverSize - 1] = (m_iLoadAddress + iOffsetMusic) >> 8;

		if (m_iDPCMSize > 0) {

			// Allocate DPCM space
			/*
			for (int i = 0; i < (m_iDPCMSize >> 12); i++)
				AllocateNewBank(0xC000 + (i << 12));
*/
			memcpy(pFileSpace + iOffsetDPCM, m_pDPCM + m_iSampleStart - 0x4000, m_iDPCMSize);

			if (bAlternativeMode) {
				iCurrentSize = iOffsetDPCM + m_iDPCMSize;
			}
			else {
				iCurrentSize += ((0xC000 - DRIVER1_LOCATION) - m_iDriverSize) + m_iDPCMSize;
			}
		}
	}
	else {
		// Copy driver
		iCurrentSize = iOffsetMusic - m_iDriverSize;

		// Copy all banks
		for (unsigned int i = 0; i < m_iAllocatedBanks; i++) {
			m_DataBanks[i].CopyData(pFileSpace + iCurrentSize);
			iCurrentSize += 0x1000;
		}

		// Copy DPCM
		if (m_iDPCMSize > 0) {
			// Set up header
			for (unsigned int i = 0; i < (m_iDPCMSize >> 12) + 1; i++)
				Header.BankValues[4 + i] = m_iAllocatedBanks + i ;

			memcpy(pFileSpace + iCurrentSize, m_pDPCM, m_iDPCMSize);
			iCurrentSize += m_iDPCMSize;
		}

		// Driver
		memcpy(pFileSpace, pCustomDriver, m_iDriverSize);

		pFileSpace[m_iDriverSize - 2] = (m_iLoadAddress + iOffsetMusic) & 0xFF;
		pFileSpace[m_iDriverSize - 1] = (m_iLoadAddress + iOffsetMusic) >> 8;
	}

	Print(" * NSF load address: $%04X\n", m_iLoadAddress);
	Print("Writing output file...\n");
	Print(" * Driver size: %i bytes\n", m_iDriverSize);

	if (m_bBankSwitched) {
		Print(" * NSF type: Bankswitched (%i banks)\n", m_iAllocatedBanks);
		Print(" * Song data size: %i bytes (%i%%)\n", m_iDataPointer, (100 * m_iDataPointer) / (0x80000 - m_iDriverSize - m_iDPCMSize));
	}
	else {
		Print(" * Song data size: %i bytes (%i%%)\n", m_iDataPointer, (100 * m_iDataPointer) / (0x8000 - m_iDriverSize - m_iDPCMSize));
		/*
#ifdef _DEBUG
		if (bAlternativeMode)
			Print(" * NSF type: Linear (driver @ $%04X)\n", DRIVER2_LOCATION);
		else
			Print(" * NSF type: Linear (driver @ $%04X)\n", DRIVER1_LOCATION);
#else*/
			Print(" * NSF type: Linear (driver @ $%04X)\n", m_iDriverLocation);
/*#endif*/
	}

	// Write the file
	OutputFile.Write(&Header, sizeof(stNSFHeader));
	OutputFile.Write(pFileSpace, iCurrentSize);

	Print("Done, total file size: %i bytes\n", iCurrentSize + sizeof(stNSFHeader));

	// remove
	delete [] pFileSpace;

	// Done
	OutputFile.Close();

#ifdef _DEBUG
	if (LogFile)
		LogFile->Close();
#endif
}

void CCompiler::ExportNES(CString FileName, CFamiTrackerDoc *pDoc, CEdit *pLogTxt, bool EnablePAL)
{
	// 32kb NROM, no CHR
	const char NES_HEADER[] = {0x4E, 0x45, 0x53, 0x1A, 0x02, 0x00, 0x00, 0x00, 
							   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	// This is basically a type 2 NSF with a caller
	unsigned int iOffsetDriver;
	unsigned int iOffsetMusic;
	unsigned int iOffsetDPCM;
	CFile OutputFile;

	m_pDocument = pDoc;

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
	
	if (m_bBankSwitched || (m_iDriverSize + m_iDataPointer + m_iDPCMSize) > 0x8000) {
		Print("Song is too big, aborted.\n");
		AfxMessageBox("Error: Song is too big to fit!", 0, 0);
		return;
	}

	// Define memory areas
	//
	m_iDriverLocation	= DRIVER2_LOCATION;
	m_iLoadAddress		= m_iDriverLocation;

	iOffsetMusic		= m_iDriverSize;
	iOffsetDriver		= 0;
	iOffsetDPCM			= m_iSampleStart;

	unsigned char *pFileSpace;
	unsigned int iCurrentSize;

	// remove this
	pFileSpace		= new unsigned char[0x8000];
	iCurrentSize	= 0;

	const char *pDriver;

	switch (pDoc->GetExpansionChip()) {
		case SNDCHIP_NONE: pDriver = DRIVER_MODE2; break;
		case SNDCHIP_VRC6: pDriver = DRIVER_VRC6; break;
		case SNDCHIP_MMC5: pDriver = DRIVER_MMC5; break;
		case SNDCHIP_VRC7: pDriver = DRIVER_VRC7; break;
		case SNDCHIP_FDS: pDriver = DRIVER_FDS; break;
	}

	memcpy(pFileSpace, pDriver, m_iDriverSize);
	memcpy(pFileSpace + m_iDriverSize, m_pData, m_iDataPointer);
	iCurrentSize = m_iDataPointer + m_iDriverSize;

	// Last two bytes of music code points to the start of music data
	pFileSpace[iOffsetDriver + m_iDriverSize - 2] = (m_iLoadAddress + iOffsetMusic) & 0xFF;
	pFileSpace[iOffsetDriver + m_iDriverSize - 1] = (m_iLoadAddress + iOffsetMusic) >> 8;

	if (m_iDPCMSize > 0) {
		memcpy(pFileSpace + iOffsetDPCM, m_pDPCM + m_iSampleStart - 0x4000, m_iDPCMSize);
		iCurrentSize = iOffsetDPCM + m_iDPCMSize;
	}

	// Add the caller to the end and patch it
	if (pDoc->GetExpansionChip() == SNDCHIP_VRC6) {
		memcpy(pFileSpace + 0x8000 - NSF_CALLER_SIZE, NSF_CALLER_BIN_VRC6, NSF_CALLER_SIZE);
		pFileSpace[0x7F2C] = (EnablePAL ? 0x01 : 0x00);
	}
	else {
		memcpy(pFileSpace + 0x8000 - NSF_CALLER_SIZE, NSF_CALLER_BIN, NSF_CALLER_SIZE);
		pFileSpace[0x7F22] = (EnablePAL ? 0x01 : 0x00);
	}

	/*
	pFileSpace[0x7FDA] = 0x00;	// init
	pFileSpace[0x7FDB] = 0x80;
	pFileSpace[0x7FF2] = 0x03;	// play
	pFileSpace[0x7FF3] = 0x80;
*/
	Print("Writing file...\n");
	Print(" * Driver size: %i bytes\n", m_iDriverSize);
	Print(" * Song data size: %i bytes (%i%%)\n", m_iDataPointer, (100 * m_iDataPointer) / (0x8000 - m_iDriverSize - m_iDPCMSize));

	if (pDoc->GetExpansionChip() == SNDCHIP_VRC6) {
		// VRC6 patch
		char head[16];
		memcpy(head, NES_HEADER, 16);
		head[6] = 0x80;
		head[7] = 0x10;
		OutputFile.Write(head, 0x10);
	}
	else {
		// Write NES header first
		OutputFile.Write(NES_HEADER, 0x10);
	}

	// Write the file
	OutputFile.Write(pFileSpace, 0x8000);

	Print("Done, total file size: %i bytes\n", 0x8000);

	// remove
	delete [] pFileSpace;

	// Done
	OutputFile.Close();
}

void CCompiler::ExportBIN(CString BIN_File, CString DPCM_File, CFamiTrackerDoc *pDoc, CEdit *pLogTxt)
{
	CFile OutputFileBIN, OutputFileDPCM;

	m_pDocument = pDoc;

	// Clear progress
	m_pLogText = pLogTxt;
	m_pLogText->SetWindowText("");

	LogFile = NULL;

#ifdef _DEBUG

	LogFile = new CFile;

	if (!LogFile->Open(LOGFILE, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Could not open log file!");
		LogFile = NULL;
		//return;
	}

#endif

	// Build the music data
	CompileData(pDoc);

	if (m_bBankSwitched) {
		Print("Error: Can't write bankswitched songs!\n");
		return;
	}

	if (!OutputFileBIN.Open(BIN_File, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Could not open music data file!");
		OutputFileBIN.Close();
		return;
	}

	if (DPCM_File.GetLength() != 0 && !OutputFileDPCM.Open(DPCM_File, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Could not open sample file!");
		OutputFileBIN.Close();
		OutputFileDPCM.Close();
		return;
	}

	Print("Writing output files...\n");
	Print(" * Music data size: %i\n", m_iDataPointer);
	Print(" * DPCM size: %i\n", m_iDPCMSize);
	Print(" * DPCM location: $%04X\n", m_iSampleStart + 0x8000);

	OutputFileBIN.Write(m_pData, m_iDataPointer);

	if (DPCM_File.GetLength() != 0)
		OutputFileDPCM.Write(m_pDPCM, m_iDPCMSize);

	Print("Done, total file sizes: %i, %i bytes\n", m_iDataPointer, m_iDPCMSize);

	// Done
	OutputFileBIN.Close();

	if (DPCM_File.GetLength() != 0)
		OutputFileDPCM.Close();

	if (LogFile)
		LogFile->Close();
}

void CCompiler::ExportPRG(CString FileName, CFamiTrackerDoc *pDoc, CEdit *pLogTxt, bool EnablePAL)
{
	// This is basically a type 2 NSF with a caller
	unsigned int iOffsetDriver;
	unsigned int iOffsetMusic;
	unsigned int iOffsetDPCM;

//	bool bAlternativeMode;

	m_pDocument = pDoc;

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

	iOffsetMusic		= m_iDriverSize;
	iOffsetDriver		= 0;
	iOffsetDPCM			= m_iSampleStart;

	unsigned char *pFileSpace;
	unsigned int iCurrentSize;

	// remove this
	pFileSpace		= new unsigned char[0x8000];
	iCurrentSize	= 0;
	//

	memcpy(pFileSpace, DRIVER_MODE2, m_iDriverSize);
	memcpy(pFileSpace + m_iDriverSize, m_pData, m_iDataPointer);
	iCurrentSize = m_iDataPointer + m_iDriverSize;

	// Last two bytes of music code points to the start of music data
	pFileSpace[iOffsetDriver + m_iDriverSize - 2] = (m_iLoadAddress + iOffsetMusic) & 0xFF;
	pFileSpace[iOffsetDriver + m_iDriverSize - 1] = (m_iLoadAddress + iOffsetMusic) >> 8;

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
	Print(" * Driver size: %i bytes\n", m_iDriverSize);
	Print(" * Song data size: %i bytes (%i%%)\n", m_iDataPointer, (100 * m_iDataPointer) / (0x8000 - m_iDriverSize - m_iDPCMSize));

	// Write the file
	OutputFile.Write(pFileSpace, 0x8000);

	Print("Done, total file size: %i bytes\n", 0x8000);

	// remove
	delete [] pFileSpace;

	// Done
	OutputFile.Close();
}

void CCompiler::ExportASM(CString FileName, CFamiTrackerDoc *pDoc, CEdit *pLogTxt)
{
//	CFile OutputFileBIN, OutputFileDPCM;

	m_pDocument = pDoc;

	// Clear progress
	m_pLogText = pLogTxt;
	m_pLogText->SetWindowText("");

	LogFile = NULL;

#ifdef _DEBUG

	LogFile = new CFile;

	if (!LogFile->Open(LOGFILE, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Could not open log file!");
		LogFile = NULL;
		//return;
	}

#endif

	// Build the music data
	CompileData(pDoc);

	if (m_bBankSwitched) {
		Print("Error: Can't write bankswitched songs!\n");
		return;
	}
/*
	if (!OutputFileBIN.Open(BIN_File, CFile::modeWrite | CFile::modeCreate) || 
		!OutputFileDPCM.Open(DPCM_File, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Could not open files!");
		OutputFileBIN.Close();
		OutputFileDPCM.Close();
		return;
	}
*/
	Print("Writing output files...\n");
	Print(" * Music data size: %i\n", m_iDataPointer);
	Print(" * DPCM size: %i\n", m_iDPCMSize);
	Print(" * DPCM location: $%04X\n", m_iSampleStart + 0x8000);
/*
	OutputFileBIN.Write(m_pData, m_iDataPointer);
	OutputFileDPCM.Write(m_pDPCM, m_iDPCMSize);
*/
	Print("Done, total file sizes: %i, %i bytes\n", m_iDataPointer, m_iDPCMSize);
/*
	// Done
	OutputFileBIN.Close();
	OutputFileDPCM.Close();
*/
	if (LogFile)
		LogFile->Close();
}


void CCompiler::Export(CString FileName, CFamiTrackerDoc *pDoc, CEdit *pLogTxt)
{
	CFile ExportFile, SampleFile;

	m_pDocument = pDoc;

	m_pLogText = pLogTxt;
	m_pLogText->SetWindowText("");

	LogFile = new CFile;

	if (!LogFile->Open(LOGFILE, CFile::modeWrite | CFile::modeCreate)) {
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

	// Determine driver size
	switch (pDoc->GetExpansionChip()) {
		case SNDCHIP_NONE:
			m_iDriverSize = DRIVER_SIZE;
			m_pChanOrder = CHAN_ORDER_DEFAULT;
			Print(" * No expansion chip\n");
			break;
		case SNDCHIP_VRC6:
			m_iDriverSize = DRIVER_SIZE_VRC6;
			m_pChanOrder = CHAN_ORDER_VRC6;
			Print(" * VRC6 expansion enabled\n");
			break;
		case SNDCHIP_MMC5:
			m_iDriverSize = DRIVER_SIZE_MMC5;
			m_pChanOrder = CHAN_ORDER_MMC5;
			Print(" * MMC5 expansion enabled\n");
			break;
		case SNDCHIP_VRC7:
			m_iDriverSize = DRIVER_SIZE_VRC7;
			m_pChanOrder = CHAN_ORDER_VRC7;
			Print(" * VRC7 expansion enabled\n");
			break;
		case SNDCHIP_FDS:
			m_iDriverSize = DRIVER_SIZE_FDS;
			m_pChanOrder = CHAN_ORDER_FDS;
			Print(" * FDS expansion enabled\n");
			break;
	}

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

	if (LogFile)
		LogFile->SeekToBegin();

	// Begin log
	WriteLog("Famitracker NSF compiler log file\r\n");
	WriteLog("---------------------------------\r\n");

	Print("Building music data...\n");

	if (m_bBankSwitched) {
		// (Always setup the 32 kB area)
		// Setup a 16 kB area, add space for DPCM dynamically
		for (i = 0; i < 4; i++)
			AllocateNewBank(i * PAGE_SIZE);

		SetInitialPosition(m_iDriverSize);
	}
	else {
		SetInitialPosition(0);
	}

	// Allocate space
	AllocateSpace();

	// Get song parameters
	ScanSong(pDoc);

	m_iMasterHeaderAddr = GetCurrentOffset();

	// Save space for header
	int HeaderSize = 6 * 2 + 1;

	if (pDoc->ExpansionEnabled(SNDCHIP_FDS))
		HeaderSize += 2;

	SkipBytes(HeaderSize);

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
	WriteLog("---------------------------------\r\n");
	WriteLog("NSF data size: %i bytes\r\n", m_iDataPointer);
	WriteLog("DPCM area size: %i bytes\r\n", m_iDPCMSize);
	WriteLog("---------- End of file ----------\r\n");
}

void CCompiler::AllocateSpace()
{
	m_pData = new unsigned char[BANK_SIZE];

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
		m_iAssignedInstruments[i] = -1;
	}

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

	WriteLog("Allocating bank at: %04X (slot %i) = %i\r\n", iAddress, iBankSlot, m_iAllocatedBanks);

	m_iAllocatedBanks++;
}


void CCompiler::SetMemoryOffset(unsigned int iOffset)
{
	m_iDataPointer = iOffset + m_iInitialPosition;
	ASSERT(m_iDataPointer < 0x8000);
}

void CCompiler::SetInitialPosition(unsigned int iAddress)
{
	// Sets initial position in memory, where music data will be stored
	ASSERT(iAddress < 0x8000);
	m_iInitialPosition = iAddress;
	m_iDataPointer = iAddress;
}

void CCompiler::SkipBytes(unsigned int iBytes)
{
	// Skips some bytes
	m_iDataPointer += iBytes;
	ASSERT(m_iDataPointer < 0x8000);
}

unsigned int CCompiler::GetCurrentOffset()
{
	// Returns current offset from start of music data (first byte of music data = 0)
	return m_iDataPointer - m_iInitialPosition;
}

unsigned int CCompiler::GetAbsoluteAddress()
{
	// Returns current absolute address in the ROM space
	// only useful when bankswitching is enabled
	return m_iDataPointer + 0x8000;
}


void CCompiler::StoreByte(unsigned char Value)
{
	// Store a byte in memory

	if (m_bBankSwitched) {
		// Bankswitched space
		unsigned int iBank, iSlot;
		
		// Find the bank used in this address
		iSlot = (m_iDataPointer & 0x7FFF) >> 12;
		iBank = m_cSelectedBanks[iSlot];

		m_DataBanks[iBank].WriteByte(m_iDataPointer++, Value);
	}
	else {
		// Linear space

		if (m_bErrorFlag)
			return;

		ASSERT(m_iDataPointer < BANK_SIZE);
		m_pData[m_iDataPointer++] = Value;

		// Overflow
		if (m_iDataPointer >= (0x8000 - m_iDriverSize))
			m_bErrorFlag = true;
	}
}

void CCompiler::StoreShort(unsigned short Value)
{
	// Store a short in memory
	StoreByte(Value & 0xFF);
	StoreByte(Value >> 8);
}

// Headers //

void CCompiler::WriteHeader(stTuneHeader *pHeader, CFamiTrackerDoc *pDoc)
{
	// Writes the music header
	unsigned int SavedPointer = m_iDataPointer;

	int TicksPerSec, Machine;

	TicksPerSec = pDoc->GetEngineSpeed();
	Machine		= pDoc->GetMachine();

	if (TicksPerSec == 0) {
		pHeader->DividerNTSC = FRAMERATE_NTSC * 60;
		pHeader->DividerPAL	= FRAMERATE_PAL * 60;
	}
	else {
		pHeader->DividerNTSC = TicksPerSec * 60;
		pHeader->DividerPAL = TicksPerSec * 60;
	}

	pHeader->Flags = (m_bBankSwitched ? 1 : 0) | ((m_pDocument->GetVibratoStyle() == VIBRATO_OLD) ? 2 : 0);

	SetMemoryOffset(m_iMasterHeaderAddr);

	// Write header
	StoreShort(pHeader->SongListOffset);
	StoreShort(pHeader->InstrumentListOffset);
	StoreShort(pHeader->DPCMInstListOffset);
	StoreShort(pHeader->DPCMSamplesOffset);
	StoreByte(pHeader->Flags);						// Flags

	// FDS table, only if FDS is enabled
	if (pDoc->ExpansionEnabled(SNDCHIP_FDS))
		StoreShort(pHeader->WaveTable);

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
	StoreByte(pTrackHeader->FrameBank);

	m_iDataPointer = SavedPointer;

	WriteLog(" -- Track header for track %i (@ %04X) --\r\n", NULL, iAddress);
	WriteLog("Frame list offset: %04X\r\n", pTrackHeader->FrameListOffset);
	WriteLog("Frame count: %i\r\n", pTrackHeader->FrameCount);
	WriteLog("Pattern length: %i\r\n", pTrackHeader->PatternLength);
	WriteLog("Song speed / tempo: %i / %i\r\n", pTrackHeader->Speed, pTrackHeader->Tempo);
	WriteLog("Frame bank: %i\r\n\r\n", pTrackHeader->FrameBank);
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

	const int TRACK_HEADER_SIZE = 7;		// sizeof dosen't work down to single bytes on structs

	m_stTuneHeader.SongListOffset = GetCurrentOffset();

	// Store track header pointers
	for (i = 0; i < iSongCount; i++)
		StoreShort(iHeaderList + i * TRACK_HEADER_SIZE);

	// Allocate space for track headers
	SkipBytes(iSongCount * TRACK_HEADER_SIZE);

	int SelectedTrack = pDoc->GetSelectedTrack();

//	iSongCount = 15;

	iChannelCount = pDoc->GetAvailableChannels();

	int FrameRowSize = iChannelCount * (m_bBankSwitched ? 3 : 2);

	// Store track headers
	for (i = 0; i < iSongCount; i++) {
		WriteLog("---------------------------------------------------------\r\n");
		WriteLog(" Storing song %i\r\n", i);
		WriteLog("---------------------------------------------------------\r\n");
		pDoc->SelectTrackFast(i);

		iFrameCount = pDoc->GetFrameCount();
//		iChannelCount = pDoc->GetAvailableChannels();
		
		if (m_bBankSwitched) {
			int FrameListSize = (iFrameCount * 2) + (iFrameCount * FrameRowSize);
			if ((GetAbsoluteAddress() + FrameListSize) > 0xBFFF) {
				AllocateNewBank(0xB000);
			}
		}

		// Allocate space for the frame list
		m_iFrameListAddress = GetCurrentOffset();		// Save the address to this list
		m_iFrameBank = m_cSelectedBanks[3];				// Also only used if in bank 3

		if (!m_bBankSwitched)
			m_iFrameBank = 0;

		WriteLog("Frame start: %04X\r\n", GetAbsoluteAddress());

		SkipBytes(iFrameCount * 2);					// pointer list
		SkipBytes(iFrameCount * FrameRowSize);	// frame list

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
		m_stTrackHeader.FrameBank		= m_iFrameBank;
		// and print it
		WriteTrackHeader(&m_stTrackHeader, iHeaderList + i * TRACK_HEADER_SIZE);
	}

	pDoc->SelectTrack(SelectedTrack);
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
	
	unsigned int	iFrameCount, iChannelCount, i, j, iOffset, iChan;
	unsigned short	iPattern, iPointer, iChannelItem;
	unsigned char	iBank;
	unsigned int	iSize = 0;

	WriteLog(" -- Filling the frame list -- \r\n");

	iFrameCount		= pDoc->GetFrameCount();
	iChannelCount	= pDoc->GetAvailableChannels();
	iChannelItem	= iChannelCount * (m_bBankSwitched ? 3 : 2);
	iOffset			= (iFrameCount * 2) + GetCurrentOffset();

	WriteLog("Frame pointer list at: (offset) %04X (absolute) %04X\r\n", GetCurrentOffset(), GetAbsoluteAddress());

	int SavedBank = m_cSelectedBanks[3];

	m_cSelectedBanks[3] = m_iFrameBank;

	// Setup addresses for the pointer list
	for (i = 0; i < iFrameCount; i++)
		StoreShort(iOffset + i * iChannelItem);

	iSize = iFrameCount * 2;

	// Store addresses to patterns
	for (i = 0; i < iFrameCount; i++) {
		WriteLog("Frame %02X %04X: ", i, GetAbsoluteAddress());
		// Pattern pointers
		for (j = 0; j < iChannelCount; j++) {
			iChan = m_pChanOrder[j];
			iPattern = pDoc->GetPatternAtFrame(i, iChan);
			iPointer = m_iPatternAddresses[iPattern][iChan];
			StoreShort(iPointer);
			WriteLog("%04X ", iPointer);
			iSize += 2;
		}
		// Banks
		if (m_bBankSwitched) {
			WriteLog("( ");
			for (j = 0; j < iChannelCount; j++) {
				iChan = m_pChanOrder[j];
				iPattern = pDoc->GetPatternAtFrame(i, iChan);
				iBank = m_iPatternBanks[iPattern][iChan];
				StoreByte(iBank);
				WriteLog("%i ", iBank);
				iSize += 1;
			}
			WriteLog(")");
		}
		WriteLog("\r\n");
	}

	m_cSelectedBanks[3] = SavedBank;

	Print(" * Frame count: %i (%i bytes)\n", iFrameCount, iSize);
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

	WriteLog(" -- Pattern data -- \r\n");

	// Find out which of the patterns that are used
	for (i = 0; i < MAX_PATTERN; i++) {
		for (j = 0; j < pDoc->GetAvailableChannels(); j++)
			bPatternsUsed[i][j] = IsPatternAddressed(pDoc, i, j);
	}

	// Iterate through all patterns
	for (i = 0; i < MAX_PATTERN; i++) {
		for (j = 0; j < iChannels; j++) {
			// And choose only used ones
			if (bPatternsUsed[i][j]) {

				// Compile pattern data
				m_oPatternCompiler.CompileData(pDoc, Track, i, j, &m_iDPCM_LookUp, m_iAssignedInstruments);

				// Todo: handle empty patterns, should only be stored once
				/*
				if (m_oPatternCompiler.EmptyPattern()) {
				}
				else {
				}
				*/

				// Get size
#ifndef COMPRESSION
				iSize = m_oPatternCompiler.GetDataSize();
				iTotalSize += iSize;
#else
				iSize = m_oPatternCompiler.GetCompressedDataSize();
				iTotalSizeCompressed += iSize;
#endif
				// Check if current bank is full
				if (m_bBankSwitched) {
					int Addr = GetAbsoluteAddress();
					if ((Addr + iSize) > 0xBFFF) {
						AllocateNewBank(0xB000);
					}
				}

				// Save the start address of this pattern
				m_iPatternAddresses[i][j] = GetCurrentOffset();

				if (m_bBankSwitched) {
					// Since only the last bank is switched, we need to store the bank
					// where this pattern ends to make sure it's completely available
					int iBankIndex = ((GetAbsoluteAddress() + iSize) & 0x7FFF) >> 12;
					int iBank = m_cSelectedBanks[iBankIndex];

					if (iBank < 3)
						iBank = 0;

					m_iPatternBanks[i][j] = iBank; 
				}
				else
					m_iPatternBanks[i][j] = 0;

				WriteLog("Pattern O %02X, (%04X [%i]) channel %i: { ", i, GetAbsoluteAddress(), m_iPatternBanks[i][j], j);

				// And store it in the memory
				for (k = 0; k < iSize; k++) {
#ifndef COMPRESSION
					StoreByte(m_oPatternCompiler.GetData(k));
#else
					StoreByte(m_oPatternCompiler.GetCompressedData(k));
#endif
					WriteLog("%02X ", m_oPatternCompiler.GetData(k));
				}

				WriteLog("}\r\n");

				iPatternCount++;
				/*
				iSize = m_oPatternCompiler.GetCompressedDataSize();
				iTotalSizeCompressed += iSize;

				WriteLog("Pattern C %02X, (%04X [%i]) channel %i: { ", i, GetAbsoluteAddress(), m_iPatternBanks[i][j], j);
#ifdef COMPRESSION
				for (k = 0; k < iSize; k++) {
					StoreByte(m_oPatternCompiler.GetCompressedData(k));
					WriteLog("%02X ", m_oPatternCompiler.GetCompressedData(k));
				}
#endif
				WriteLog("}\n");
				*/
			}
		}
	}

	WriteLog(" ----------------- \r\n");
	WriteLog("Total size uncompressed: %i\r\n", iTotalSize);
	WriteLog("Total size compressed: %i\r\n", iTotalSizeCompressed);
	
	if (iTotalSize > 0)
		WriteLog("Ratio of compression: %i%%\r\n", 100 - (iTotalSizeCompressed * 100) / iTotalSize);

	//Print(" * Song %i, %i patterns (%i bytes) [%i bytes, %i%%]\n", Track + 1, iPatternCount, iTotalSizeCompressed, iTotalSize, 100 - (iTotalSizeCompressed * 100) / iTotalSize);
	Print(" * Song %i, %i patterns (%i bytes)\r\n", Track + 1, iPatternCount, iTotalSize);
}

unsigned int CCompiler::StoreSequence(CFamiTrackerDoc *pDoc, CSequence *pSeq)
{
	unsigned int Address = 0;
	int iItemCount, iLoopPoint, iReleasePoint;
	int k;

	if (pSeq->GetItemCount() > 0) {
		// Sequences cannot be stored on addresses where the low byte is zero
		// because that indicates no sequence
		// Todo: is this still valid?? 
		// 2010-01-03: ?? check this
		if ((GetCurrentOffset() & 0xFF) == 0)
			StoreByte(0);

		// Save a pointer to this
		Address = GetCurrentOffset();

		// Store the sequence
		iItemCount	  = pSeq->GetItemCount();
		iLoopPoint	  = pSeq->GetLoopPoint();
		iReleasePoint = pSeq->GetReleasePoint();

		if (iReleasePoint != -1)
			iReleasePoint += 1;
		else
			iReleasePoint = 0;

		StoreByte((unsigned char)iItemCount);
		StoreByte((unsigned char)iLoopPoint);
		StoreByte((unsigned char)iReleasePoint);

		WriteLog("%02X %02X %02X ", iItemCount, (unsigned char)iLoopPoint, (unsigned char)iReleasePoint);
		WriteLog("{ ");

		for (k = 0; k < iItemCount; k++) {
			StoreByte(pSeq->GetItem(k));
			if (k == iLoopPoint)
				WriteLog("| ");
			WriteLog("%02X ", pSeq->GetItem(k));
		}
	}

	// Return 0 if no sequence is used
	return Address;
}

void CCompiler::CreateSequenceList(CFamiTrackerDoc *pDoc)
{
	/*
	 * Create sequence lists
	 *
	 *
	 */

	unsigned int i, j, Size = 0, StoredCount = 0;
	CSequence	 *pSeq;
	
	WriteLog(" -- Sequences -- \r\n");

	for (i = 0; i < MAX_SEQUENCES; i++) {
		for (j = 0; j < SEQ_COUNT; j++) {
			pSeq = pDoc->GetSequence(i, j);

			if (pSeq->GetItemCount() > 0) {
				WriteLog("Sequence %i (%04X): ", i, m_iDataPointer);
				m_iSequenceAddresses[i][j] = StoreSequence(pDoc, pSeq);
				StoredCount++;
				Size += 3 + pSeq->GetItemCount();

				switch (j) {
					case SEQ_VOLUME:	WriteLog("} (Volume)\r\n");		break;
					case SEQ_ARPEGGIO:	WriteLog("} (Arpeggio)\r\n");		break;
					case SEQ_PITCH:		WriteLog("} (Pitch)\r\n");		break;
					case SEQ_HIPITCH:	WriteLog("} (Hi Pitch)\r\n");		break;
					case SEQ_DUTYCYCLE:	WriteLog("} (Duty cycle)\r\n");	break;
				}
			}
			else
				m_iSequenceAddresses[i][j] = 0;
		}
	}

	if (pDoc->ExpansionEnabled(SNDCHIP_VRC6)) {
		WriteLog(" -- Sequences VRC6 -- \r\n");
		for (i = 0; i < MAX_SEQUENCES; i++) {
			for (j = 0; j < SEQ_COUNT; j++) {
				pSeq = pDoc->GetSequenceVRC6(i, j);

				if (pSeq->GetItemCount() > 0) {
					WriteLog("Sequence %i (%04X): ", i, m_iDataPointer);
					m_iSequenceAddressesVRC6[i][j] = StoreSequence(pDoc, pSeq);
					StoredCount++;
					Size += 3 + pSeq->GetItemCount();

					switch (j) {
						case SEQ_VOLUME:	WriteLog("} (Volume)\r\n");		break;
						case SEQ_ARPEGGIO:	WriteLog("} (Arpeggio)\r\n");		break;
						case SEQ_PITCH:		WriteLog("} (Pitch)\r\n");		break;
						case SEQ_HIPITCH:	WriteLog("} (Hi Pitch)\r\n");		break;
						case SEQ_DUTYCYCLE:	WriteLog("} (Duty cycle)\r\n");	break;
					}
				}
				else
					m_iSequenceAddressesVRC6[i][j] = 0;
			}
		}
	}

	if (pDoc->ExpansionEnabled(SNDCHIP_FDS)) {
		memset(m_iSequenceAddressesFDS, 0, sizeof(int) * MAX_INSTRUMENTS * SEQ_COUNT);
		WriteLog(" -- Sequences FDS -- \r\n");
		for (i = 0; i < MAX_INSTRUMENTS; i++) {
			CInstrumentFDS *pInst = (CInstrumentFDS*)pDoc->GetInstrument(i);
			if (pInst && pInst->GetType() == INST_FDS) {
				pSeq = pInst->GetVolumeSeq();
				if (pSeq->GetItemCount() > 0) {
					WriteLog("Sequence %i (%04X): ", i, m_iDataPointer);
					m_iSequenceAddressesFDS[i][0] = StoreSequence(pDoc, pSeq);
					StoredCount++;
					Size += 3 + pSeq->GetItemCount();
				}
				pSeq = pInst->GetArpSeq();
				if (pSeq->GetItemCount() > 0) {
					WriteLog("Sequence %i (%04X): ", i, m_iDataPointer);
					m_iSequenceAddressesFDS[i][1] = StoreSequence(pDoc, pSeq);
					StoredCount++;
					Size += 3 + pSeq->GetItemCount();
				}
				pSeq = pInst->GetPitchSeq();
				if (pSeq->GetItemCount() > 0) {
					WriteLog("Sequence %i (%04X): ", i, m_iDataPointer);
					m_iSequenceAddressesFDS[i][2] = StoreSequence(pDoc, pSeq);
					StoredCount++;
					Size += 3 + pSeq->GetItemCount();
				}
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
	 * Sequences must have been done before running this
	 *
	 */

	CInstrument *pInstrument;
	unsigned int i, iIndex, iOffset, iListPointer, StoredBytes = 0;	

	WriteLog(" -- Instrument list -- \r\n");

	m_stTuneHeader.InstrumentListOffset = GetCurrentOffset();
	
	iListPointer = GetCurrentOffset();
	iOffset		 = (m_iInstruments * 2) + GetCurrentOffset();

	m_iWaveTables = 0;

	int Offset = 0;

	// Setup addresses for the instruments
	for (i = 0; i < m_iInstruments; i++) {
		StoreShort(iOffset + Offset);
		int Index = m_iAssignedInstruments[i];
		pInstrument = pDoc->GetInstrument(Index);
		Offset += pInstrument->CompileSize(this);
	}

	// Store instruments
	for (i = 0; i < m_iInstruments; i++) {
		WriteLog("Instrument %i: ", i);		
		iIndex = m_iAssignedInstruments[i];
		pInstrument = pDoc->GetInstrument(iIndex);
		StoredBytes += pInstrument->Compile(this, i);
		
		// Print name to log file
		char Name[256];
		pInstrument->GetName(Name);
		WriteLog("} %s\r\n", Name);
	}

	Print(" * Instruments used: %i (%i bytes)\n", m_iInstruments, StoredBytes);

	// Handle expansion chips

	if (pDoc->ExpansionEnabled(SNDCHIP_FDS)) {
		// Write FDS waves
		WriteLog(" -- FDS wave lists -- \r\n");
		m_stTuneHeader.WaveTable = GetCurrentOffset() - 1;
		// Write waves
		for (i = 0; i < (unsigned)m_iWaveTables; ++i) {
			WriteLog("Storing table %i {", i);
			for (int j = 0; j < 64; ++j) {
				StoreByte(*(m_iWaveTable[i] + j));
				WriteLog("%i%s", *(m_iWaveTable[i] + j), (j < 63) ? ", " : "");
			}
			WriteLog("}\r\n");
		}
		Print(" * FDS waves used: %i (%i bytes)\n", m_iWaveTables, m_iWaveTables * 64);
	}
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

	WriteLog(" -- DPCM instrument list -- \r\n");

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
				cSample	= pInst->GetSample(j, k)/* - 1*/;
				if ((/*pInst->GetSample(j, k)*/ cSample > 0) && m_bSamplesAccessed[iIndex][j][k] && m_pDocument->GetSampleSize(cSample - 1) > 0) {
					cSample -= 1;
					cSamplePitch	= pInst->GetSamplePitch(j, k);
					cSamplePitch	|= (cSamplePitch & 0x80) >> 1;	// Still the same
					iSampleIndex	= GetSampleIndex(cSample);
					// Save a reference to this item
					m_iDPCM_LookUp[i][j][k] = Item++;
					StoreByte(cSamplePitch);
					StoreByte(iSampleIndex << 1);
					WriteLog("Sample: %i (%i), pitch %i (instrument %i)\r\n", cSample, iSampleIndex, cSamplePitch, iIndex);
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

	WriteLog(" -- Saving DPCM bank -- \r\n");
	
	if (m_bErrorFlag)
		return;

	iListAddress	= m_stTuneHeader.DPCMSamplesOffset;
	iDataPosition	= GetCurrentOffset() + m_iDriverSize;

	// Align to DPCM bank or closest valid sample address
	if (iDataPosition > 0x4000 && !m_bBankSwitched)
		iSampleAddress = (((iDataPosition >> 6) + 1) << 6) - 0x4000;
	else
		iSampleAddress = 0x0000;

	// Get sample start address
	m_iSampleStart = 0x4000 + iSampleAddress;

	iSavedPointer = m_iDataPointer;
	SetMemoryOffset(iListAddress);

	m_iDPCMSize = 0;

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

			m_iDPCMSize += iSize;

			// Align
			if ((iSampleAddress & 0x3F) > 0) {
				m_iDPCMSize += 0x40 - (iSampleAddress & 0x3F);
				iSampleAddress += 0x40 - (iSampleAddress & 0x3F);
			}

			WriteLog("Sample %i: address: %02X, size: %02X\r\n", iIndex, cSampleAddr, cSampleSize);
		}
	}

	m_iDataPointer	= iSavedPointer;
	m_iDPCMSize		= m_iSamplesUsed > 0 ? /*iSampleAddress*/ m_iDPCMSize : 0;

	if (Overflow) {
		WriteLog("DPCM overflow\r\n");
		Print("Warning: DPCM overflow!\n");

		if (!m_bBankSwitched)
			m_bErrorFlag = true;
	}

	Print(" * Samples located at: $%04X\n", m_iSampleStart + 0x8000);
	Print(" * DPCM samples used: %i (%i bytes)\n", m_iSamplesUsed, m_iDPCMSize);
}

unsigned int CCompiler::GetSequenceAddress(int Sequence, int Type) const
{
	return m_iSequenceAddresses[Sequence][Type];
}

unsigned int CCompiler::GetSequenceAddressVRC6(int Sequence, int Type) const
{
	return m_iSequenceAddressesVRC6[Sequence][Type];
}

int CCompiler::AddWavetable(unsigned char Wave[64])
{
	// Find equal existing waves
	for (int i = 0; i < m_iWaveTables; ++i) {
		if (!memcmp(Wave, m_iWaveTable[i], 64))
			return i;
	}
	// Allocate new wave
	m_iWaveTable[m_iWaveTables] = new unsigned char[64];
	memcpy(m_iWaveTable[m_iWaveTables], Wave, 64);
	m_iWaveTables++;
	return m_iWaveTables - 1;
}

unsigned int CCompiler::GetSequenceAddressFDS(int Instrument, int Type) const
{
	return m_iSequenceAddressesFDS[Instrument][Type];
}
