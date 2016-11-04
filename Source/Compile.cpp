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
// This class is used to create NSF files.
// Totally rewritten from version 0.2.3 (removed the assembly stuff)
//

#include "stdafx.h"
#include "FamiTracker.h"
#include "Compile.h"

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
** |  Instruments                    : 2 |
** |  DPCM instruments               : 2 |
** |  Sequences                      : 2 |
** |  Frames                         : 2 |
** |  DPCM samples                   : 2 |
** |-------------------------------------|
** | * Music data                        |
** |-------------------------------------|
** | Instruments pointers            : x |
** |-------------------------------------|
** | DPCM instruments pointers       : x |
** |-------------------------------------|
** | Sequence pointers               : x |
** |-------------------------------------|
** | Song frame list                 : x |
** |-------------------------------------|
** | DPCM rom addresses              : x |
** |-------------------------------------|
** | Instrument data                 : x |
** |-------------------------------------|
** | DPCM instrument data            : x |
** |-------------------------------------|
** | Sequence lists                  : x |
** |-------------------------------------|
** | Pattern pointers for frames     : x |
** |-------------------------------------|
** | Pattern data for every channel  : x |
** |-------------------------------------|
** | DPCM samples                    : x |
** '-------------------------------------'
*/

void CCompile::WriteToBank(unsigned char Value)
{
	if (m_iBankPointer > 0xFFFF) {
		m_iBankPointer++;
		return;
	}

	m_bPageDirty[(m_iBankPointer & 0x7FFF) >> 12] = true;
	m_pBank[m_iBankPointer & 0x7FFF] = Value;
	m_iBankPointer++;
}

void CCompile::StoreBankPosition(int Number, int Increase)
{
	m_iPointerOffsets[Number] = m_iBankPointer;
	m_iBankPointer += Increase;
}

void CCompile::WriteBankOffset(int Number, int Offset)
{
	unsigned int Pos = (m_iPointerOffsets[Number] + Offset) & 0x7FFF;
	m_pBank[Pos] = m_iBankPointer & 0xFF;
	m_pBank[Pos + 1] = (m_iBankPointer >> 8) & 0xFF;
}

void CCompile::CreatePRG(CString FileName, CFamiTrackerDoc *pDoc, bool ForcePAL)
{
	// Create a file that's ready to burn on an eprom
	unsigned short FileSize;
	
	CString Text;
	CFile File;

	FileSize = m_iBankPointer - m_iMusicDataAddr;

	Text.Format("Writing %s...", FileName);
	AddLog(Text);

	// Add the music driver
	memcpy(m_pBank /*+ 0x80*/, DRIVER_BIN, DRIVER_SIZE);

	// Apply nsf caller...
	memcpy(m_pBank + (0x8000 - NSF_CALLER_SIZE), NSF_CALLER_BIN, NSF_CALLER_SIZE);

	// ...and patch it
	m_pBank[0x7FC4] = (ForcePAL ? 0x01 : 0x00);
	m_pBank[0x7FC6] = 0x00;
	m_pBank[0x7FC7] = 0x80;
	m_pBank[0x7FD1] = 0x03;
	m_pBank[0x7FD2] = 0x80;

	// Write file and then return
	if (!File.Open(FileName, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Couldn't open file");
		delete [] m_pBank;
		return;
	}

	// Size will always be 32kB, fill one whole PRG
	FileSize = 0x8000;

	File.Write(m_pBank, FileSize);
	File.Close();

	delete [] m_pBank;

	Text.Format("Done (%i bytes)", FileSize);
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
		delete [] m_pBank;
		return;
	}

	BinFile.Write(m_pBank + (m_iMusicDataAddr - 0x8000), m_iRawMusicSize);
	if (m_iDPCMStart != 0)
		DmcFile.Write(m_pBank + (m_iDPCMStart - 0x8000), m_iRawSampleSize);
	else
		m_iRawSampleSize = 0;

	BinFile.Close();
	DmcFile.Close();

	delete [] m_pBank;

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

	if (pDoc->GetEngineSpeed() == 0)
		Speed = 1000000 / (pDoc->GetMachine() == NTSC ? 60 : 50);
	else
		Speed = 1000000 / pDoc->GetEngineSpeed();

	Text.Format(" * NSF speed: %i (%s)", Speed, (pDoc->GetMachine() == NTSC ? "NTSC" : "PAL"));
	AddLog(Text);

	// Create a header
	SetSongInfo(pDoc->GetSongName(), 
		pDoc->GetSongArtist(), 
		pDoc->GetSongCopyright(), 
		pDoc->GetEngineSpeed(), 
		pDoc->GetMachine(), 
		ForcePAL);

	// Add the music driver
	memcpy(m_pBank /*+ 0x80*/, DRIVER_BIN, DRIVER_SIZE);

	FileSize = m_iBankPointer - 0x8000;

	if (BankSwitch) {
		int Bank = 0, i, Size;
		unsigned char *OldBank = m_pBank;
		
		m_pBank = new unsigned char[0x8000];

		FileSize = 0;

		for (i = 0; i < 8; i++) {
			if (m_bPageDirty[i]) {
				if (((i + 1) * 0x1000) < int(m_iBankPointer & 0x7FFF))
					Size = 0x1000;
				else
					Size = (m_iBankPointer & 0x7FFF) - (i * 0x1000);
				memcpy(m_pBank + (Bank * 0x1000), OldBank + i * 0x1000, Size);
				Header.BankValues[i] = Bank++;
				FileSize += Size;
			}
		}

		delete [] OldBank;

		Text.Format("Banks %X %X %X %X %X %X %X %X...", Header.BankValues[0], Header.BankValues[1], Header.BankValues[2], Header.BankValues[3], Header.BankValues[4], Header.BankValues[5], Header.BankValues[6], Header.BankValues[7]);
		AddLog(Text);
	}

	// Apply header
//	memcpy(m_pBank, &Header, 0x80);

	Text.Format("Writing %s...", FileName);
	AddLog(Text);

	// Write file and then return
	if (!File.Open(FileName, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox("Couldn't open NSF output file");
		delete [] m_pBank;
		return;
	}

	// Make sure it's below 32kB
	FileSize &= 0x7FFF;

	File.Write(&Header, 0x80);
	File.Write(m_pBank, FileSize);

	File.Close();

	delete [] m_pBank;

	Text.Format("Done (%i bytes)", FileSize);
	AddLog(Text);
}

void CCompile::AccumulateZero()
{
	m_iZeroes++;
}

void CCompile::DispatchZeroes()
{
	if (m_iZeroes < 3) {
		for (unsigned int i = 0; i < m_iZeroes; i++)
			WriteToBank(0);
	}
	else {
		WriteToBank(0xFF);
		WriteToBank(m_iZeroes - 1);
	}

	m_iZeroes = 0;
}

void CCompile::BuildMusicData(int StartAddress, CFamiTrackerDoc *pDoc)
{
	const int NES_CHANNELS = 5;

	unsigned char TickPeriod;
	unsigned char InstrumentsUsed = 0;
	unsigned char SequencesUsed = 0;
	unsigned char DSamplesUsed = 0;
	unsigned char DPCM_LookUp[MAX_INSTRUMENTS][8][12];

	unsigned int MaxPattern[NES_CHANNELS];
	unsigned int i, j, k, c, x;
	unsigned int SelectedInstrument;
	unsigned int TicksPerSec;
	unsigned int SavedPointer, PointerBeforeDPCM, DPCMStart, TotalSize;

	unsigned int SequenceIndices[MAX_SEQUENCES * MOD_COUNT];
	bool SequencesAccessed[MAX_SEQUENCES * MOD_COUNT];

	unsigned short PattAddr[MAX_PATTERN][NES_CHANNELS];

	bool MusicDataFilled = false;

	m_iDPCMStart = 0xC000;
	m_iRawMusicSize = 0;
	m_iRawSampleSize = 0;

	LogText = "";

	AddLog("Building music data...");

	m_iMusicDataAddr = StartAddress & 0xFFFF;

	if (m_iMusicDataAddr < 0x8000)
		m_iMusicDataAddr += 0x8000;

	m_pBank			= new unsigned char[0x8000];
	m_iBankPointer	= m_iMusicDataAddr;

	for (i = 0; i < 8; i++)
		m_bPageDirty[i] = false;

	memset(MaxPattern, 0, sizeof(int) * NES_CHANNELS);
	memset(m_pBank, 0, 0x8000);

	memset(SequencesAccessed, 0, sizeof(bool) * MAX_SEQUENCES * MOD_COUNT);

	m_iZeroes = 0;

	TicksPerSec = pDoc->GetFrameRate();
	TickPeriod	= pDoc->GetSongSpeed();

	// Determine number of used instruments
	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (pDoc->IsInstrumentUsed(i))
			InstrumentsUsed = i + 1;
	}

	// Determine number of used sequences and store indices in a lookup table
	for (i = 0; i < MAX_SEQUENCES; i++) {
		for (x = 0; x < MOD_COUNT; x++) {
			if (pDoc->GetSequenceCount(i, x) > 0)
				SequenceIndices[i * MOD_COUNT + x] = SequencesUsed++;
			else
				SequenceIndices[i * MOD_COUNT + x] = 0;
		}
	}

	// Determine number of used dpcm samples
	for (i = 0; i < MAX_DSAMPLES; i++) {
		if (pDoc->GetSampleSize(i) > 0)
			DSamplesUsed = i + 1;
	}

	// Find all patterns that has been addressed to
	for (i = 0; i < unsigned(pDoc->GetFrameCount()); i++) {
		for (j = 0; j < unsigned(pDoc->GetAvailableChannels()); j++) {
			if (pDoc->GetPatternAtFrame(i, j) /*pDoc->FrameList[i][j]*/ > MaxPattern[j])
				MaxPattern[j] = pDoc->GetPatternAtFrame(i, j) + 1; //pDoc->FrameList[i][j] + 1;
		}
	}

	WriteToBank(TickPeriod);
	WriteToBank((TicksPerSec * 5) / 2);		// 60 / 24
	WriteToBank(pDoc->GetFrameCount());
	WriteToBank(pDoc->GetPatternLength() - 1);

	// Save offsets for later (and allocate space)
	StoreBankPosition(INSTLIST_POINTER,			2);
	StoreBankPosition(DPCM_INSTLIST_POINTER,	2);
	StoreBankPosition(SEQLIST_POINTER,			2);
	StoreBankPosition(FRAMELIST_POINTER,		2);
	StoreBankPosition(DPCMLIST_POINTER,			2);
	WriteToBank((TicksPerSec * 60) & 0xFF);
	WriteToBank((TicksPerSec * 60) >> 8);

	// Allocate space for instrument pointers
	WriteBankOffset(INSTLIST_POINTER, 0);
	StoreBankPosition(INST_POINTERS, InstrumentsUsed * 2);

	// Allocate space for DPCM instrument pointers
	WriteBankOffset(DPCM_INSTLIST_POINTER, 0);
	StoreBankPosition(DPCM_INST_POINTERS, InstrumentsUsed * 2);

	// Allocate space for sequence pointers
	WriteBankOffset(SEQLIST_POINTER, 0);
	StoreBankPosition(SEQ_POINTERS, SequencesUsed * 2);

	// Allocate space for frame pointers
	WriteBankOffset(FRAMELIST_POINTER, 0);
	StoreBankPosition(FRAME_POINTERS, pDoc->GetFrameCount() * 2);

	// Allocate space for DPCM sample pointers
	WriteBankOffset(DPCMLIST_POINTER, 0);
	StoreBankPosition(DPCM_POINTERS, DSamplesUsed * 2);
	
	// Store instruments
	for (i = 0; i < InstrumentsUsed; i++) {
		// Write pointer
		WriteBankOffset(INST_POINTERS, i * 2);
		for (c = 0; c < MOD_COUNT; c++) {
			if (pDoc->GetInstEffState(i, c)) {
				int Sequence = pDoc->GetInstEffIndex(i, c);
				int Index = SequenceIndices[Sequence * MOD_COUNT + c];
				WriteToBank(Index + 1);
				SequencesAccessed[Sequence * MOD_COUNT + c] = true;
			}
			else
				WriteToBank(0);
		}
	}

	// Store DPCM instrument definitions
	for (i = 0; i < InstrumentsUsed; i++) {

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
					WriteToBank(Sample);
					WriteToBank(SamplePitch);
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

			Index		= SequenceIndices[i * MOD_COUNT + x];
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

	// Fill addresses for frames
	SavedPointer = m_iBankPointer;
	for (i = 0; i < unsigned(pDoc->GetFrameCount()); i++) {
		WriteBankOffset(FRAME_POINTERS, i * 2);
		m_iBankPointer += pDoc->GetAvailableChannels() * 2;
	}

	m_iBankPointer = SavedPointer;

	// Allocate space for the frame list
	StoreBankPosition(FRAME_LIST, (pDoc->GetAvailableChannels() * pDoc->GetFrameCount()) * 2);

	// Now, store pattern data
	for (i = 0; i < MAX_PATTERN; i++) {
		for (j = 0; j < pDoc->GetAvailableChannels(); j++) {

			SelectedInstrument = MAX_INSTRUMENTS + 1;

			// Write pattern only if it has been addressed
			if (MaxPattern[j] >= i) {
				PattAddr[i][j] = m_iBankPointer;
				for (k = 0; k < pDoc->GetPatternLength(); k++) {

					unsigned char Note, Octave, Instrument, AsmNote, Volume;
					unsigned char Effect, EffParam;

					stChanNote NoteData;

					pDoc->GetDataAtPattern(i, j, k, &NoteData);

					Note		= NoteData.Note;
					Octave		= NoteData.Octave;
					Instrument	= NoteData.Instrument;
					Volume		= NoteData.Vol;

					if (Note == 0) {
						AsmNote = 0;
					}
					else if (Note == HALT) {
						AsmNote = 0x7F;
					}
					else {
						if (j == 4) {	// DPCM	
							int LookUp = DPCM_LookUp[Instrument][Octave][Note - 1];
							if (LookUp > 0)
								AsmNote = LookUp * 2;
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
						WriteToBank(0x80);
						WriteToBank(Instrument);
					}

					for (unsigned int n = 0; n < (pDoc->GetEffColumns(j) + 1); n++) {
						Effect		= NoteData.EffNumber[n];
						EffParam	= NoteData.EffParam[n];
						
						if (Effect > 0)
							DispatchZeroes();

						switch (Effect) {
							case EF_ARPEGGIO:
								WriteToBank(NSF_EFF_BEGIN + 0);
								WriteToBank(EffParam);
								break;
							case EF_PORTAON:
								WriteToBank(NSF_EFF_BEGIN + 1);
								WriteToBank(EffParam + 1);
								break;
							case EF_PORTAOFF:
								WriteToBank(NSF_EFF_BEGIN + 2);
								WriteToBank(EffParam);
								break;
							case EF_VIBRATO:
								WriteToBank(NSF_EFF_BEGIN + 3);
								WriteToBank(EffParam);
								break;
							case EF_TREMOLO:
								WriteToBank(NSF_EFF_BEGIN + 4);
								WriteToBank(EffParam);
								break;
							case EF_SPEED:
								WriteToBank(NSF_EFF_BEGIN + 5);
								WriteToBank(EffParam);
								break;
							case EF_JUMP:
								WriteToBank(NSF_EFF_BEGIN + 6);
								WriteToBank(EffParam + 1); 
								break;
							case EF_SKIP:
								WriteToBank(NSF_EFF_BEGIN + 7);
								WriteToBank(EffParam);
								break;
							case EF_HALT:
								WriteToBank(NSF_EFF_BEGIN + 8);
								WriteToBank(EffParam);
								break;
							case EF_VOLUME:
								WriteToBank(NSF_EFF_BEGIN + 9);
								WriteToBank(EffParam);
								break;
							case EF_SWEEPUP: {
								// this need some extra work
								unsigned char shift	= EffParam & 0x07;
								unsigned char len	= (EffParam >> 4) & 0x07;
								unsigned char Val	= 0x88 | (len << 4) | shift;
								WriteToBank(NSF_EFF_BEGIN + 10);
								WriteToBank(Val);
								break;
							}
							case EF_SWEEPDOWN: {
								unsigned char shift	= EffParam & 0x07;
								unsigned char len	= (EffParam >> 4) & 0x07;
								unsigned char Val	= 0x80 | (len << 4) | shift;
								WriteToBank(NSF_EFF_BEGIN + 10);
								WriteToBank(Val);
								break;
							}
							case EF_PITCH:
								if (j < 5) {
									WriteToBank(NSF_EFF_BEGIN + 11);
									WriteToBank(EffParam);
								}
								break;
						}
					}

					if (Volume < 0x10) {
						DispatchZeroes();
						WriteToBank(0x81);
						WriteToBank((Volume ^ 0x0F) & 0x0F);
					}

					if (AsmNote == 0)
						AccumulateZero();
					else {
						DispatchZeroes();
						WriteToBank(AsmNote);
					}

//					WriteToBank(AsmNote);
				}
				DispatchZeroes();
			}
		}
	}

	PointerBeforeDPCM = m_iBankPointer;

	// Write pattern addresses in the frame list
	for (i = 0; i < unsigned(pDoc->GetFrameCount()); i++) {
		for (j = 0; j < unsigned(pDoc->GetAvailableChannels()); j++) {
			m_iBankPointer = PattAddr[pDoc->GetPatternAtFrame(i, j)][j];
			WriteBankOffset(FRAME_LIST, ((i * pDoc->GetAvailableChannels()) + j) * 2);
		}
	}
	
	m_iBankPointer = PointerBeforeDPCM;

	m_iMusicDataEnd = PointerBeforeDPCM;
	m_iRawMusicSize = PointerBeforeDPCM - StartAddress;

	if (DSamplesUsed > 0) {

		if (m_iBankPointer < 0xC000)
			m_iBankPointer = 0xC000;
		else
			m_iBankPointer = ((m_iBankPointer / 0x40) + 1) << 6;

		m_iDPCMStart = m_iBankPointer;
		DPCMStart = m_iBankPointer;

		// Write DPCM samples
		for (i = 0; i < MAX_DSAMPLES; i++) {
			if (pDoc->GetSampleSize(i) > 0) {
				
				unsigned int SampleSize = pDoc->GetSampleSize(i);
				// Store position and length
				m_pBank[(m_iPointerOffsets[DPCM_POINTERS] + (i * 2)) & 0x7FFF]		= (m_iBankPointer - 0xC000) >> 6;
				m_pBank[(m_iPointerOffsets[DPCM_POINTERS] + (i * 2) + 1) & 0x7FFF]	= SampleSize >> 4;

				SavedPointer = m_iBankPointer;

				for (j = 0; j < SampleSize; j++) {
					WriteToBank(pDoc->GetSampleData(i, j));
				}

				unsigned short OldSize = SampleSize;
				unsigned short NewSize = (SampleSize >> 6) << 6;

				if (OldSize != NewSize) {
					NewSize += 0x40;
				}

				m_iBankPointer = SavedPointer + NewSize;
			}
		}
	}
	else
		m_iDPCMStart = 0;

	m_iMusicDataSize = m_iBankPointer - StartAddress;
	m_iRawSampleSize = m_iBankPointer - m_iDPCMStart;

	if (DSamplesUsed > 0)
		TotalSize = (PointerBeforeDPCM - 0x8000) + (m_iBankPointer - DPCMStart);
	else
		TotalSize = (PointerBeforeDPCM - 0x8000);

	// Music data is compiled, compile some text

	CString Text, Param;

	Text = " * Patterns used: ";

	for (i = 0; i < NES_CHANNELS; i++) {
		Param.Format("%i", MaxPattern[i]); 
		Text.Append(Param);
		if (i < (NES_CHANNELS - 1))
			Text.Append(", ");
	}

	AddLog(Text);

	Text.Format(" * Instruments/sequences used: %i / %i", InstrumentsUsed, SequencesUsed);
	AddLog(Text);

	Text.Format(" * Frames: %i", pDoc->GetFrameCount());
	AddLog(Text);

	if (DSamplesUsed > 0)
		Text.Format(" * DPCM samples: %i, allocated size: %i bytes", DSamplesUsed, m_iBankPointer - 0xC000);
	else
		Text.Format(" * DPCM samples: 0, allocated size: 0 bytes");

	AddLog(Text);

	Text.Format(" * Music data size: %i", (PointerBeforeDPCM - StartAddress));
	AddLog(Text);

	Text.Format(" * Driver: %s", DRIVER_ID);
	AddLog(Text);

	Text.Format(" * Total size: $%04X (%i bytes)", TotalSize, TotalSize);
	AddLog(Text);

	if (TotalSize > 0x8000) {
		AddLog(" ! Error: Music data size exceeded limit. NSF may not work properly !");
		AfxMessageBox("Error: Music data size exceeded limit. NSF may not work properly!");
	}
}

void CCompile::SetSongInfo(char *Name, char *Artist, char *Copyright, int Speed, int Machine, bool ForcePAL)
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
	Header.TotalSongs		= 1;
	Header.StartSong		= 1;
	Header.LoadAddr			= 0x8000;
	Header.InitAddr			= 0x8000;
	Header.PlayAddr			= 0x8003;

	strcpy((char*)&Header.SongName, Name);
	strcpy((char*)&Header.ArtistName, Artist);
	strcpy((char*)&Header.Copyright, Copyright);

	Header.Speed_NTSC		= SpeedNTSC; //0x411A; // default ntsc speed
	Header.BankValues[0]	= 0;
	Header.BankValues[1]	= 0;
	Header.BankValues[2]	= 0;
	Header.BankValues[3]	= 0;
	Header.BankValues[4]	= 0;
	Header.BankValues[5]	= 0;
	Header.BankValues[6]	= 0;
	Header.BankValues[7]	= 0;
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
