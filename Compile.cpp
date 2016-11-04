/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005  Jonathan Liss
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
// Compile.cpp : implementation file
//
// This class is used to produce NSF files.
// Parts of this code is originally from an 6502 assmebler I worked on.
//

#include "stdafx.h"
#include "FamiTracker.h"
#include "Compile.h"

const unsigned char NSF_EFFECTS[] = {0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89};

int WrittenOrigin = 0;

// CCompile

CCompile::CCompile()
{
}

CCompile::~CCompile()
{
}

// CCompile member functions

bool CCompile::CreateSymbolFile(char *Filename)
{
	if (!File.Open(Filename, CFile::modeCreate | CFile::modeWrite))
		return false;

	ErrorFlag = false;

	return true;
}

void CCompile::WriteSymbol(char *Text, ...)
{
	char	Buf[512];
    va_list argp;
    
	va_start(argp, Text);
    
	if (!Text)
		return;

    vsprintf(Buf, Text, argp);

	File.Write(Buf, unsigned int(strlen(Buf)));
}

void CCompile::WriteSymbolRaw(char *Data, int Size)
{
	File.Write(Data, Size);
}

void CCompile::CloseSymbolFile()
{
	File.Close();
}

char *CCompile::CompileNSF(CFamiTrackerDoc *pDoc, CString FileName, bool KeepSymbols)
{
	//
	// Create a NSF from the module
	//
	// KeepSymbols - If true, copy assembly symbols file along with the NSF file
	//
	// Return - a string with some statistics.
	//

	CFile		File;
	char		FilePath[MAX_PATH], SymbolFile[MAX_PATH], SymbolKeepFile[MAX_PATH];
	char		BinFile[MAX_PATH], DriverFile[MAX_PATH];
	char		*CompileInfo;
	int			i, c, p;
	int			InstrumentsUsed = 0, SequencesUsed = 0;
	int			MaxPattern[5];
	int			DPCMPointer = 0x00;
	//int			SelectedInstrument;
	//int			OrgPos;
	int			TickPeriod;

	TickPeriod		= pDoc->m_iSongSpeed;
	CompileInfo		= new char[2048];
	CompileInfo[0]	= 0;

	AddedSamples	= 0;

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (pDoc->Instruments[i].Free == false) {
			InstrumentsUsed = i + 1;
		}
	}

	for (i = 0; i < MAX_SEQUENCES; i++) {
		if (pDoc->Sequences[i].Count > 0)
			SequencesUsed = i + 1;
	}

	for (c = 0; c < pDoc->m_iChannelsAvaliable; c++) {
		MaxPattern[c] = 0;
	}

	for (i = 0; i < pDoc->m_iFrameCount; i++) {
		for (c = 0; c < pDoc->m_iChannelsAvaliable; c++) {
			if (pDoc->FrameList[i][c] > MaxPattern[c]) {
				MaxPattern[c] = pDoc->FrameList[i][c] + 1;
			}
		}
	}

	if (InstrumentsUsed == 0 || SequencesUsed == 0) {
		sprintf(CompileInfo, "Error: There must be at least one instrument and sequence list");
		return CompileInfo;
	}

	strcpy(FilePath, pDoc->GetPathName().GetString());

	for (i = (int)strlen(FilePath); i; i--) {
		if (FilePath[i] == '\\') {
			FilePath[i] = NULL;
			break;
		}
	}

	strcpy(SymbolFile,		theApp.m_cAppPath);
	strcat(SymbolFile,		"\\songdata.asm");
	strcpy(BinFile,			theApp.m_cAppPath);
	strcat(BinFile,			"\\songdata.bin");
	strcpy(DriverFile,		theApp.m_cAppPath);
	strcat(DriverFile,		"\\driver.bin");
	strcpy(SymbolKeepFile,	FilePath);
	strcat(SymbolKeepFile,	"\\musicdata.asm");
	
	if (!CreateSymbolFile(SymbolFile)) {
		strcpy(CompileInfo, "Error: Could not create music data file\r\n");
		return CompileInfo;
	}

	WriteSymbol("; FamiTracker\n; music symbol file : %s\n; \n\n", pDoc->GetTitle());
	WriteSymbol(" .org $%04X\n\n", MUSIC_ORIGIN);
	WriteSymbol(" %s %i ; song speed\n", SYM_STORE_BYTE, TickPeriod + 1);
	WriteSymbol(" %s %i ; frames\n", SYM_STORE_BYTE, pDoc->m_iFrameCount);
	WriteSymbol(" %s %i ; pattern length\n\n", SYM_STORE_BYTE, pDoc->m_iPatternLength);
	WriteSymbol(" %s InstPointers ; instruments\n", SYM_STORE_WORD);
	WriteSymbol(" %s InstPointersDPCM ; instruments\n", SYM_STORE_WORD);
	WriteSymbol(" %s SequencePtrs ; sequences\n", SYM_STORE_WORD);
	WriteSymbol(" %s Frames ; frames\n", SYM_STORE_WORD);
	WriteSymbol(" %s DPCM ; dpcm samples\n\n", SYM_STORE_WORD);

	// Store pointers

	WriteSymbol("\nInstPointers: ; instrument pointers\n %s ", SYM_STORE_WORD);

	for (i = 0; i < InstrumentsUsed - 1; i++)
		WriteSymbol("inst%i, ", i);

	WriteSymbol("inst%i\n", i);

	bool Stored = false, WasAvaliable;

	WriteSymbol("\nInstPointersDPCM: ; instrument pointers for DPCM");

	for (i = 0; i < InstrumentsUsed; i++) {
		WasAvaliable = false;
		for (int o = 0; o < 6; o++) {
			for (int n = 0; n < 12; n++) {
				if (pDoc->Instruments[i].Samples[o][n] > 0) {
					if (!Stored) {
						WriteSymbol("\n %s ", SYM_STORE_WORD);
						Stored = true;
					}
					else
						WriteSymbol(",");
					WriteSymbol("dpcm_inst%i", i);
					n = 12; o = 6;
					WasAvaliable = true;
				}
			}
		}
		if (!WasAvaliable) {
			if (!Stored) {
				WriteSymbol("\n %s ", SYM_STORE_WORD);
				Stored = true;
			}
			else
				WriteSymbol(",");
			WriteSymbol("0");
		}
	}

	WriteSymbol("\nSequencePtrs:	; sequence pointers\n %s ", SYM_STORE_WORD);

	for (i = 0; i < SequencesUsed - 1; i++)
		WriteSymbol("seq%i, ", i, i);

	WriteSymbol(" seq%i\n", i, i);
	WriteSymbol("\nFrames:   ; pattern frame pointers\n %s ", SYM_STORE_WORD);

	for (i = 0; i < pDoc->m_iFrameCount - 1; i++)
		WriteSymbol("frame%i, ", i); 

	WriteSymbol("frame%i\n", i); 
	WriteSymbol("\nDPCM:\n %s ", SYM_STORE_BYTE);

	for (i = 0; i < MAX_DSAMPLES; i++) {
		if (pDoc->DSamples[i].SampleSize != 0) {
			WriteSymbol("$%02X,$%02X,", DPCMPointer, pDoc->DSamples[i].SampleSize / 0x10);
			DPCMPointer += ((pDoc->DSamples[i].SampleSize / 0x40) + 1);
		}
		else
			WriteSymbol("$00,$00,");
	}

	WriteSymbol("$00\n\n");
	WriteSymbol("; Instruments\n");

	for (i = 0; i < InstrumentsUsed; i++) {
		WriteSymbol("inst%i: %s ", i, SYM_STORE_BYTE);
		NSF_WriteInstruments(pDoc->Instruments + i);
	}

	WriteSymbol("\n; DPCM instrument definitions\n");

	for (i = 0; i < InstrumentsUsed; i++) {
		NSF_WriteDPCMInst(i, pDoc->Instruments + i);
	}

	WriteSymbol("\n; Sequences\n");

	for (i = 0; i < SequencesUsed; i++) {
//		WriteSymbol("seq%i: %s %i, ", i, SYM_STORE_BYTE, pDoc->Sequences[i].Count); 
		WriteSymbol("%s %i\n", SYM_STORE_BYTE, pDoc->Sequences[i].Count); 
		WriteSymbol(" seq%i: %s ", i, SYM_STORE_BYTE); 
		NSF_WriteSequence(pDoc->Sequences + i);
	}

	WriteSymbol("\n; Frames\n");

	for (i = 0; i < pDoc->m_iFrameCount; i++) {
		WriteSymbol("frame%i: %s ", i, SYM_STORE_WORD); 
		
		for (c = 0; c < pDoc->m_iChannelsAvaliable - 1; c++)
			WriteSymbol("pat%ich%i,", pDoc->FrameList[i][c], c); 

		WriteSymbol("pat%ich%i\n", pDoc->FrameList[i][c], c); 
	}

	WriteSymbol("\n ; pattern data\n");

	SelectedInstrument = MAX_INSTRUMENTS;

	for (p = 0; p < MAX_PATTERN; p++) {
		for (c = 0; c < pDoc->m_iChannelsAvaliable; c++) {

			// Only write pattern if it has been addressed
			if (MaxPattern[c] >= p) {

				WriteSymbol("; Pattern %i, channel %i\n", p, c); 

				WriteSymbol("pat%ich%i: %s ", p, c, SYM_STORE_BYTE); 
				SelectedInstrument = MAX_INSTRUMENTS;

				for (i = 0; i < pDoc->m_iPatternLength; i++) {
					//WritePatternSymbol(p, c, i);
					NSF_WritePattern(pDoc, p, c, i);
				}
			}
		}
	}

	WriteSymbol("\n\n"); 

	SampleOrgPos = 0xC000;
	AddedSamples = 0;

	WriteSymbol(" .org $%04X\n", SampleOrgPos);
	WriteSymbol(" .segment \"DPCM\"\n", SampleOrgPos);

	// write dmc samples, they need to be aligned to $40 byte blocks
	for (i = 0; i < MAX_DSAMPLES; i++) {
		NSF_WriteDPCM(pDoc->DSamples + i);
		SampleOrgPos += pDoc->DSamples[i].SampleSize;
	}

	CloseSymbolFile();
	
	if (!CompileSymbolFile(SymbolFile, BinFile)) {
		strcpy(CompileInfo, "Error: Could not compile music data\r\n");
		return CompileInfo;
	}

	int Speed;

	if (pDoc->m_iEngineSpeed == 0)
		Speed = 1000000 / (pDoc->m_iMachine == NTSC ? 60 : 50);
	else
		Speed = 1000000 / pDoc->m_iEngineSpeed;

	SetSongInfo(pDoc->m_strName, pDoc->m_strArtist, pDoc->m_strCopyright, Speed, pDoc->m_iMachine);

	if (!ApplyDriver(BinFile, FileName)) {
		strcpy(CompileInfo, "Error: Could not apply sound driver\r\n");
		return CompileInfo;
	}

	if (KeepSymbols) {
		CopyFile(SymbolFile, SymbolKeepFile, FALSE);
	}

	DeleteFile(BinFile);
	DeleteFile(SymbolFile);

	sprintf(CompileInfo, "Wrote %s\r\n", FileName);
	sprintf(CompileInfo, "%s * Patterns used: %i, %i, %i, %i, %i\r\n", CompileInfo, MaxPattern[0], MaxPattern[1], MaxPattern[2], MaxPattern[3], MaxPattern[4]);
	sprintf(CompileInfo, "%s * Instruments used: %i\r\n", CompileInfo, InstrumentsUsed);
	sprintf(CompileInfo, "%s * Sequences used: %i\r\n", CompileInfo, SequencesUsed);
	sprintf(CompileInfo, "%s * Frames: %i\r\n", CompileInfo, pDoc->m_iFrameCount);
	sprintf(CompileInfo, "%s * DPCM samples: %i, allocated size: %i bytes\r\n", CompileInfo, AddedSamples, SampleOrgPos - 0xC000);
	sprintf(CompileInfo, "%s * Music data size: %i bytes\r\n", CompileInfo, GetMusicSize());
	sprintf(CompileInfo, "%s * NSF speed: %04X\r\n", CompileInfo, Speed);

	if (KeepSymbols)
		sprintf(CompileInfo, "%sMusic data is in file %s\r\n", CompileInfo, SymbolKeepFile);

	if (ErrorFlag)
		sprintf(CompileInfo, "%s ! Errors encountered, NSF may not work !\r\n", CompileInfo, GetMusicSize());

	return CompileInfo;

}

void CCompile::NSF_WriteDPCM(stDSample *pDSample)
{
	unsigned char v;
	int c;

	if (pDSample->SampleSize != 0) {
		AddedSamples++;
		WriteSymbol("\n %s ", SYM_STORE_BYTE);

		for (c = 0; c < ((pDSample->SampleSize / 0x40) + 1) * 0x40; c++) {
			if (c < pDSample->SampleSize)
				v = (unsigned char)pDSample->SampleData[c];
			else
				v = 0;

			if ((c & 15) == 15)
				WriteSymbol("%i\n %s ", v, SYM_STORE_BYTE);
			else
				WriteSymbol("%i,", v);
		}

		WriteSymbol("%i\n\n", (unsigned char)pDSample->SampleData[c]);		
	}
}

void CCompile::NSF_WriteSequence(stSequence *pSequence)
{
	int Val, c;

	for (c = 0; c < pSequence->Count; c++) {
		Val = (char)pSequence->Value[c];
		if (Val < 0)
			Val = 256 + Val;
		if (pSequence->Length[c] >= 0)
			WriteSymbol("%i,%i,", (char)pSequence->Length[c] + 1, Val); 
		else {
			// fix for the nsf player
			int len;
			len = (char)pSequence->Length[c] - 1;
			if ((-len) > (char)pSequence->Count)
				len = -(char)pSequence->Count;
			len = 256 + len;
			WriteSymbol("%i,%i,", len, Val); 
		}
	}

	WriteSymbol("0,0\n");	// end of list
}

void CCompile::NSF_WriteInstruments(stInstrument *pInstrument)
{
	int c;

	for (c = 0; c < MOD_COUNT - 1; c++) {
		if (pInstrument->ModEnable[c])
			WriteSymbol("%i,", (unsigned char)pInstrument->ModIndex[c] + 1); 
		else
			WriteSymbol("0,"); 
	}
	if (pInstrument->ModEnable[c])
		WriteSymbol("%i", (unsigned char)pInstrument->ModIndex[c] + 1); 
	else
		WriteSymbol("0"); 

	WriteSymbol("\n");
}

void CCompile::NSF_WriteDPCMInst(int Index, stInstrument *pInstrument)
{
	int o, n, i = 1;
	bool First = true;

	for (o = 0; o < 6; o++) {
		for (n = 0; n < 12; n++) {
			if (pInstrument->Samples[o][n] > 0) {
				if (!First) {
					WriteSymbol(",");
				}
				else {
					WriteSymbol("dpcm_inst%i: %s ", Index, SYM_STORE_BYTE);
				}
				First = false;
				WriteSymbol("%i, %i", (unsigned char)pInstrument->Samples[o][n] - 1, (unsigned char)pInstrument->SamplePitch[o][n]);
				DPCM_LookUp[Index][o][n] = i++;
			}
			else
				DPCM_LookUp[Index][o][n] = 0;
		}
	}

	if (!First)
		WriteSymbol("\n");
}

void CCompile::NSF_WritePattern(CFamiTrackerDoc *pDoc, int Pattern, int Channel, int Item)
{
	int Octave, Note, AsmNote;
	char *EndLine;

	if (Item == (pDoc->m_iPatternLength - 1))
		EndLine = "";
	else
		EndLine = ",";

	stChanNote	*pChanData = &pDoc->ChannelData[Channel][Pattern][Item];

	Note	= pChanData->Note;
	Octave	= pChanData->Octave;

	if (pChanData->Note == 0)
		AsmNote = 0;
	else if (pChanData->Note == HALT)
		AsmNote = 0x7F;
	else {
		if (Channel == 4) { // DPCM
			int LookUp = DPCM_LookUp[pChanData->Instrument][Octave][Note - 1];
			if (LookUp > 0)
				AsmNote = LookUp * 2;
			else
				AsmNote = 0;
		}
		else
			AsmNote = (Note - 1) + (Octave * 12);

		if (pChanData->Instrument != MAX_INSTRUMENTS && pChanData->Instrument != SelectedInstrument) {
			// write instrument change as an command
			SelectedInstrument = pChanData->Instrument;
			WriteSymbol("$80,%i,", SelectedInstrument);
		}
	}

	switch (pChanData->ExtraStuff1) {
		case EF_SPEED:		WriteSymbol("$%X,%i,", NSF_EFFECTS[0], pChanData->ExtraStuff2);	 break;
		case EF_JUMP:		WriteSymbol("$%X,%i,", NSF_EFFECTS[1], pChanData->ExtraStuff2 + 1); break;
		case EF_SKIP:		WriteSymbol("$%X,%i,", NSF_EFFECTS[2], pChanData->ExtraStuff2);		break;
		case EF_HALT:		WriteSymbol("$%X,%i,", NSF_EFFECTS[3], pChanData->ExtraStuff2);		break;
		case EF_VOLUME:		WriteSymbol("$%X,%i,", NSF_EFFECTS[4], pChanData->ExtraStuff2);		break;
		case EF_PORTAON:	WriteSymbol("$%X,%i,", NSF_EFFECTS[5], pChanData->ExtraStuff2 + 1); break;
		case EF_PORTAOFF:	WriteSymbol("$%X,%i,", NSF_EFFECTS[6], pChanData->ExtraStuff2);		break;
		case EF_SWEEPUP: {
			// this need some extra work
			int shift	= pChanData->ExtraStuff2 & 0x07;
			int len		= (pChanData->ExtraStuff2 / 10) & 0x07;
			int Val		= 0x88 | (len << 4) | shift;
			WriteSymbol("$%X,%i,", NSF_EFFECTS[7], Val); 
			break;
		}
		case EF_SWEEPDOWN: {
			int shift	= pChanData->ExtraStuff2 & 0x07;
			int len		= (pChanData->ExtraStuff2 / 10) & 0x07;
			int Val		= 0x80 | (len << 4) | shift;
			WriteSymbol("$%X,%i,", NSF_EFFECTS[7], Val); 
			break;
		}
	}

	if (pChanData->Vol > 0) {
		WriteSymbol("$81,%i,", ((pChanData->Vol - 1) ^ 0x0F) & 0x0F);
	}

	WriteSymbol("%i%s", AsmNote, EndLine);
}

void CCompile::SetSongInfo(char *Name, char *Artist, char *Copyright, int Speed, int Machine)
{
	// Fill the NSF header
	//
	// Speed will be the same for NTSC/PAL
	//

	memset(&Header, 0, 0x80);

	Header.Ident[0]			= 0x4E;
	Header.Ident[1]			= 0x45;
	Header.Ident[2]			= 0x53;
	Header.Ident[3]			= 0x4D;
	Header.Ident[4]			= 0x1A;

	Header.Version			= 0x01;
	Header.TotalSongs		= 1;
	Header.StartSong		= 1;
	Header.LoadAddr			= 0x8080;
	Header.InitAddr			= 0x8080;
	Header.PlayAddr			= 0x8083;

	strcpy((char*)&Header.SongName, Name);
	strcpy((char*)&Header.ArtistName, Artist);
	strcpy((char*)&Header.Copyright, Copyright);

	Header.Speed_NTSC		= Speed; //0x411A; // default ntsc speed
	Header.BankValues[0]	= 0;
	Header.BankValues[1]	= 0;
	Header.BankValues[2]	= 0;
	Header.BankValues[3]	= 0;
	Header.BankValues[4]	= 0;
	Header.BankValues[5]	= 0;
	Header.BankValues[6]	= 0;
	Header.BankValues[7]	= 0;
	Header.Speed_PAL		= Speed; //0x4E20; // default pal speed
	Header.Flags			= 0x02 | (Machine == NTSC ? 0 : 1);		// set both dual and the specific machine
	Header.SoundChip		= 0x00;
	Header.Reserved[0]		= 0x00;
	Header.Reserved[1]		= 0x00;
	Header.Reserved[2]		= 0x00;
	Header.Reserved[3]		= 0x00;
}

bool CCompile::ApplyDriver(char *BinFile, CString OutFile)
{
	char *MusicCode = new char[0x8000];
	int	 MusicSize;

	// Copy the driver code
	memcpy(MusicCode + 0x80, DRIVER_BIN, DRIVER_SIZE);

	// Fill the header
	memcpy(MusicCode, &Header, 0x80);

	if (!File.Open(BinFile, CFile::modeRead)) {
		delete [] MusicCode;
		return false;
	}

	MusicSize = /*Origin*/ WrittenOrigin - (0x8000 + DRIVER_SIZE);

	File.Seek(DRIVER_SIZE, 0x0000);
	File.Read(MusicCode + DRIVER_SIZE, MusicSize);
	File.Close();

	if (!File.Open(OutFile, CFile::modeWrite | CFile::modeCreate)) {
		delete [] MusicCode;
		return false;
	}

	File.Write(MusicCode, MusicSize + DRIVER_SIZE);
	File.Close();

	delete [] MusicCode;

	return true;
}

int CCompile::GetMusicSize()
{
	return SongSize - (0x8000 + DRIVER_SIZE);
}

bool CCompile::CompileSymbolFile(char *InFile, char *OutFile)
{
	char	Word[256];
	bool	FileDone = false;

	if (!File.Open(InFile, CFile::modeRead))
		return false;

	CodeBank = new char[0x8000];

	int len;

	Origin = 0;
	LabelStack = 0;

	for (int i = 0; i < 256; i++) {
		Labels[i].Used = false;
	}

	FillAdresses	= true;
	FileDone		= false;
	Origin			= 0x8000 + DRIVER_SIZE;
	SongSize		= 0x8000 + DRIVER_SIZE;

	while (!FileDone) {

		len = GetWord(Word);

		if (File.GetLength() == File.GetPosition())
			FileDone = true;

		if (len > 0)
			CheckStatement(Word);
	}

	File.SeekToBegin();

	FillAdresses	= false;
	FileDone		= false;
	Origin			= 0x8000 + DRIVER_SIZE;
	SongSize		= 0x8000 + DRIVER_SIZE;

	while (!FileDone) {

		len = GetWord(Word);

		if (File.GetLength() == File.GetPosition())
			FileDone = true;

		if (len > 0)
			CheckStatement(Word);
	}

	File.Close();

	if (!File.Open(OutFile, CFile::modeCreate | CFile::modeWrite))
		return false;

	File.Write(CodeBank, /*0x8000*/ SongSize - 0x8000);
	File.Close();

	delete [] CodeBank;

	return true;
}

int CCompile::GetWord(char *Word)
{
	int Len = 0;
	char c = 0;
	int IsEof = 1;

	memset(Word, 0, 256);

	while (Len < 256 && IsEof > 0) {
		IsEof = File.Read(&c, 1);
		switch (c) {
			case ' ':
			case '\t':
				if (Len == 0)
					Len = 0;
				else
					return Len;
				break;
			case '\r':
				break;
			case '\n':
				return Len;
			case ';':
				while (c != '\n' && IsEof > 0)
					IsEof = File.Read(&c, 1);
				return Len;
			default:
				Word[Len] = c;
				if (Len++ == 256)
					return Len;
				if (c == ',')
					return Len;
		}
	}

	return 0;
}

void CCompile::CheckStatement(char *Word)
{
	bool	Done;
	char	Param[256];
	int		ParamLen;
	int		Value;

	if (Word[0] == '.') {

		Done = false;

		if (strcmp(Word, SYM_STORE_BYTE) == 0) {
			// Store a byte
			// Loop until all bytes is read
			while(Done == false) {
				ParamLen = GetWord(Param);

				// Check if more than one byte, splitted with ','

				if (Param[ParamLen - 1] == ',')
					Param[ParamLen - 1] = 0;
				else
					Done = true;

				Value = GetValidValue(Param);

				if (Value == -1) {
					AbortError("byte value not valid!\n");
				}
				else {
					// Must be in range of one byte
					if (Value < 0 || Value > 255 || ValueSize == 16)
						// Too big
						AbortError("byte value overflow!\n");
					else {
						// Add it
						WriteBank(Value);
					}
				}
			}
		}	
		else if (strcmp(Word, SYM_STORE_WORD) == 0) {
			// Store a word
			while(Done == false) {
				ParamLen = GetWord(Param);
				
				// Check if more than one, splitted with ','

				if (Param[ParamLen - 1] == ',')
					Param[ParamLen - 1] = 0;
				else
					Done = true;

				Value = GetValidValue(Param);
				
				if (Value == -1) {
					AbortError("word value not valid!\n");
				}
				else {
					// Must be in range of one word
					if (Value < 0 || Value > 65535)
						// Too big
						AbortError("word value overflow!\n");
					else {
						// Add it, storing the low byte first
						WriteBank(Value & 0xFF);
						WriteBank(Value >> 8);
					}
				}
			}
		}
		else if (strcmp(Word, SYM_ORIGIN) == 0) {
			// Set origin
			GetWord(Param);
			Value = GetValidValue(Param); //ConvertValue(Param);
			if (Value == 0xC000)
				SongSize = Origin;
			Origin = Value;
		}
		else if (strcmp(Word, SYM_INCLUDE_BIN) == 0) {
			// Include binary file
			ParamLen = GetWord(Param);
			Param[ParamLen - 1] = 0;
			IncludeBinary(Param + 1);
		}
		else if (strcmp(Word, SYM_SEGMENT) == 0) {
			// Just to skip the error message
			// do nothing
			GetWord(Param);
		}
		else if (strcmp(Word, ".bank") == 0) {
			GetWord(Param);
			// do nothing
		}
	}
	else
	{
		// If it wasn't,
		// check if a label

		ParamLen = (int)strlen(Word);

		if (Word[ParamLen - 1] == ':')
		{
			if (FillAdresses == true)
			{
				Labels[LabelStack].Address = Origin;
				strcpy(Labels[LabelStack].Name, Word);
				Labels[LabelStack].Name[ParamLen - 1] = 0;
				LabelStack++;
			}
		}
		else
		{
			// Syntax error
			//
			AbortError("syntax error, unknown directive!\n");
		}
	}
}

void CCompile::WriteBank(char c)
{
	if (!FillAdresses)
		CodeBank[Origin & 0x7FFF] = c;

	WrittenOrigin = Origin;

	Origin++;
	SongSize++;
}

int CCompile::GetValidValue(char *param)
{
	// This translates the string in param
	// to a valid value, -1 if there was an error
	// If it was a label then the label address is returned
	//

	unsigned int x;
	bool FoundIt = false;
	int Value = 0;

	ValueSize = 0;

	if (param[0] == '$')
	{
		// Hexadecimal
		//

		param++;

		for (x = 0; x < strlen(param); x++)
			if ((param[x] < '0') || (param[x] > '9'))
				if ((param[x] < 'A') || (param[x] > 'F'))
				{
					AbortError("invalid hex value!\n");
					return -1;
				}
		if (strlen(param) > 4)
		{
			AbortError("overflow!\n");
			return -1;
		}

		sscanf(param, "%X", &Value);

	}
	else if (param[0] == '%')
	{
		// Binary
		//
		param++;

		// Translate binary to decimal

		for (x = (int)strlen(param) - 1; x; x--)
		{
			if (param[x] == '1') 
			{
				Value |= 1 << (8 - (x + 1));
			}
			else if (param[x] != '0') 
			{
				AbortError("invalid binary value!\n");
				return -1;
			}
		}
	}
	else if (param[0] >= '0' && param[0] <= '9')
	{
		// Decimal
		//

		for (x = 0; x < strlen(param); x++)
			if (param[x] < '0' || param[x] > '9')
				AbortError("invalid decimal value!\n");

		sscanf(param, "%d", &Value);
	}
	else
	{
		// Label, constant or something
		//

		// First check the labels
		for (int x = 0; x < LabelStack; x++)
		{
			if (FillAdresses == true)
			{
				// Return 0 if we are not sure
				Value = 0;
				ValueSize = 16;				// Labels are always 16bit 
				FoundIt = true;
				break;
			}
			else
			{
				if (strcmp(Labels[x].Name, param) == 0)
				{
					// Found it
					Value = Labels[x].Address;
					ValueSize = 16;			// Labels are always 16bit 
					FoundIt = true;
					break;
				}
			}
		}
	}

	// Only accepts unsigned 16bit now
	if (Value < 0 || Value > 65535)
	{
		AbortError("overflow!\n");
		return -1;
	}

	// Get the bit size of the value (if not already setted)
	if ((Value > 0xFF) && (ValueSize == 0))
		ValueSize = 16;
	else 
		ValueSize = 8;

	return Value;
}

void CCompile::IncludeBinary(char *Filename)
{
	CFile	File;
	int		Size;
	char	*ReadBlock;

	File.Open(Filename, CFile::modeRead);

	Size = (int)File.GetLength();

	ReadBlock = new char[Size];

	File.Read(ReadBlock, Size);

	for (int i = 0; i < Size; i++) {
		WriteBank(ReadBlock[i]);
	}

	delete [] ReadBlock;

	File.Close();
}

void CCompile::AbortError(char *Text)
{
	ErrorFlag = true;
}