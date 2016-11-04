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

// 2A03 instruments

CInstrument2A03::CInstrument2A03() : 
	m_iPitchOption(0)
{
	for (int i = 0; i < SEQ_COUNT; ++i) {
		m_iSeqEnable[i] = 0;
		m_iSeqIndex[i] = 0;
	}

	for (int i = 0; i < OCTAVE_RANGE; ++i) {
		for (int j = 0; j < 12; ++j) {
			m_cSamples[i][j] = 0;
			m_cSamplePitch[i][j] = 0;
			m_cSampleLoopOffset[i][j] = 0;
		}
	}
}

CInstrument *CInstrument2A03::Clone()
{
	CInstrument2A03 *pNew = new CInstrument2A03();

	for (int i = 0; i < SEQ_COUNT; i++) {
		pNew->SetSeqEnable(i, GetSeqEnable(i));
		pNew->SetSeqIndex(i, GetSeqIndex(i));
	}

	for (int i = 0; i < OCTAVE_RANGE; i++) {
		for (int j = 0; j < 12; j++) {
			pNew->SetSample(i, j, GetSample(i, j));
			pNew->SetSamplePitch(i, j, GetSamplePitch(i, j));
		}
	}

	pNew->SetName(GetName());

	return pNew;
}

void CInstrument2A03::Store(CDocumentFile *pDocFile)
{
	int i, j;

	pDocFile->WriteBlockInt(SEQ_COUNT);

	for (i = 0; i < SEQ_COUNT; i++) {
		pDocFile->WriteBlockChar(GetSeqEnable(i));
		pDocFile->WriteBlockChar(GetSeqIndex(i));
	}

	for (i = 0; i < OCTAVE_RANGE; i++) {
		for (j = 0; j < 12; j++) {
			pDocFile->WriteBlockChar(GetSample(i, j));
			pDocFile->WriteBlockChar(GetSamplePitch(i, j));
		}
	}
}

bool CInstrument2A03::Load(CDocumentFile *pDocFile)
{
	int SeqCnt, i, j;
	int Octaves = OCTAVE_RANGE;
	int Index;
	int Version = pDocFile->GetBlockVersion();

	SeqCnt = pDocFile->GetBlockInt();
	ASSERT_FILE_DATA(SeqCnt < (SEQ_COUNT + 1));

	for (i = 0; i < SeqCnt; i++) {
		SetSeqEnable(i, pDocFile->GetBlockChar());
		Index = pDocFile->GetBlockChar();
		ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
		SetSeqIndex(i, Index);
	}

	if (Version == 1)
		Octaves = 6;

	for (i = 0; i < Octaves; i++) {
		for (j = 0; j < 12; j++) {
			Index = pDocFile->GetBlockChar();
			if (Index >= MAX_DSAMPLES)
				Index = 0;
			SetSample(i, j, Index);
			SetSamplePitch(i, j, pDocFile->GetBlockChar());
		}
	}

	return true;
}

void CInstrument2A03::SaveFile(CFile *pFile)
{
	// Saves an 2A03 instrument
	// Current version 2.2

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();

	// Sequences
	unsigned char SeqCount = SEQ_COUNT;
	pFile->Write(&SeqCount, sizeof(char));

	for (int i = 0; i < SEQ_COUNT; i++) {
		int Sequence = GetSeqIndex(i);

		if (GetSeqEnable(i)) {
			CSequence *pSeq = pDoc->GetSequence(Sequence, i);
			char Enabled = 1;
			int ItemCount = pSeq->GetItemCount();
			int LoopPoint = pSeq->GetLoopPoint();
			int ReleasePoint = pSeq->GetReleasePoint();
			int Setting = pSeq->GetSetting();
			pFile->Write(&Enabled, sizeof(char));
			pFile->Write(&ItemCount, sizeof(int));
			pFile->Write(&LoopPoint, sizeof(int));
			pFile->Write(&ReleasePoint, sizeof(int));
			pFile->Write(&Setting, sizeof(int));
			for (unsigned int j = 0; j < pSeq->GetItemCount(); j++) {
				int Value = pSeq->GetItem(j);
				pFile->Write(&Value, sizeof(char));
			}
		}
		else {
			char Enabled = 0;
			pFile->Write(&Enabled, sizeof(char));
		}
	}

	unsigned int Count = 0;

	// Count assigned keys
	for (int i = 0; i < 6; i++) {	// octaves
		for (int j = 0; j < 12; j++) {	// notes
			if (GetSample(i, j) > 0)
				Count++;
		}
	}

	pFile->Write(&Count, sizeof(int));

	bool UsedSamples[MAX_DSAMPLES];
	memset(UsedSamples, 0, sizeof(bool) * MAX_DSAMPLES);

	// DPCM
	for (int i = 0; i < 6; i++) {	// octaves
		for (int j = 0; j < 12; j++) {	// notes
			if (GetSample(i, j) > 0) {
				unsigned char Index = i * 12 + j, Sample, Pitch;
				pFile->Write(&Index, sizeof(char));
				Sample = GetSample(i, j);
				Pitch = GetSamplePitch(i, j);
				pFile->Write(&Sample, sizeof(char));
				pFile->Write(&Pitch, sizeof(char));
				UsedSamples[GetSample(i, j) - 1] = true;
			}
		}
	}

	int SampleCount = 0;

	// Count samples
	for (int i = 0; i < MAX_DSAMPLES; i++) {
		if (pDoc->GetSampleSize(i) > 0 && UsedSamples[i])
			SampleCount++;
	}

	// Write the number
	pFile->Write(&SampleCount, sizeof(int));

	// List of sample names, the samples itself won't be stored
	for (int i = 0; i < MAX_DSAMPLES; i++) {
		if (pDoc->GetSampleSize(i) > 0 && UsedSamples[i]) {
			CDSample *pSample = pDoc->GetDSample(i);
			int Len = (int)strlen(pSample->Name);
			pFile->Write(&i, sizeof(int));
			pFile->Write(&Len, sizeof(int));
			pFile->Write(pSample->Name, Len);
			pFile->Write(&pSample->SampleSize, sizeof(int));
			pFile->Write(pSample->SampleData, pSample->SampleSize);
		}
	}
}

bool CInstrument2A03::LoadFile(CFile *pFile, int iVersion)
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();
	char SampleNames[MAX_DSAMPLES][256];
	stSequence OldSequence;

	// Sequences
	unsigned char SeqCount;
	pFile->Read(&SeqCount, sizeof(char));

	// Loop through all instrument effects
	for (int l = 0; l < SeqCount; l++) {

		unsigned char Enabled;
		pFile->Read(&Enabled, sizeof(char));

		if (Enabled == 1) {
			// Read the sequence
			int Count;
			pFile->Read(&Count, sizeof(int));

			// Find a free sequence
			int Index = pDoc->GetFreeSequence(l);
			CSequence *pSeq = pDoc->GetSequence(Index, l);

			if (iVersion < 20) {
				OldSequence.Count = Count;
				for (int k = 0; k < Count; k++) {
					pFile->Read(&OldSequence.Length[k], sizeof(char));
					pFile->Read(&OldSequence.Value[k], sizeof(char));
				}
				pDoc->ConvertSequence(&OldSequence, pSeq, l);	// convert
			}
			else {
				pSeq->SetItemCount(Count);
				int LoopPoint;
				int Setting;
				pFile->Read(&LoopPoint, sizeof(int));
				pSeq->SetLoopPoint(LoopPoint);
				if (iVersion > 20) {
					int ReleasePoint;
					pFile->Read(&ReleasePoint, sizeof(int));
					pSeq->SetReleasePoint(ReleasePoint);
				}
				if (iVersion >= 23) {
					pFile->Read(&Setting, sizeof(int));
					pSeq->SetSetting(Setting);
				}
				for (int k = 0; k < Count; k++) {
					char Val;
					pFile->Read(&Val, sizeof(char));
					pSeq->SetItem(k, Val);
				}
			}
			SetSeqEnable(l, true);
			SetSeqIndex(l, Index);
		}
		else {
			SetSeqEnable(l, false);
			SetSeqIndex(l, 0);
		}
	}

	bool SamplesFound[MAX_DSAMPLES];
	memset(SamplesFound, 0, sizeof(bool) * MAX_DSAMPLES);

	unsigned int Count;
	unsigned int j;
	pFile->Read(&Count, sizeof(int));

	// DPCM instruments
	for (j = 0; j < Count; j++) {
		int Note, Octave;
		unsigned char InstNote, Sample, Pitch;
		pFile->Read(&InstNote, sizeof(char));
		Octave = InstNote / 12;
		Note = InstNote % 12;
		pFile->Read(&Sample, sizeof(char));
		pFile->Read(&Pitch, sizeof(char));
		SetSamplePitch(Octave, Note, Pitch);
		SetSample(Octave, Note, Sample);
	}

	// DPCM samples list
	unsigned int SampleCount;
	bool Found;
	int Size;
	char *SampleData;

	pFile->Read(&SampleCount, sizeof(int));

	for (unsigned int l = 0; l < SampleCount; l++) {
		int Index, Len;
		pFile->Read(&Index, sizeof(int));
		pFile->Read(&Len, sizeof(int));
		pFile->Read(SampleNames[Index], Len);
		SampleNames[Index][Len] = 0;
		pFile->Read(&Size, sizeof(int));
		SampleData = new char[Size];
		pFile->Read(SampleData, Size);
		Found = false;
		for (int m = 0; m < MAX_DSAMPLES; m++) {
			CDSample *pSample = pDoc->GetDSample(m);
			if (pSample->SampleSize > 0 && !strcmp(pSample->Name, SampleNames[Index])) {
				Found = true;
				// Assign sample
				for (int n = 0; n < (6 * 12); n++) {
					if (GetSample(n / 6, j % 13) == (Index + 1))
						SetSample(n / 6, j % 13, m + 1);
				}
			}
		}
		if (!Found) {
			// Load sample
			int FreeSample;
			for (FreeSample = 0; FreeSample < MAX_DSAMPLES; FreeSample++) {
				if (pDoc->GetSampleSize(FreeSample) == 0)
					break;
			}

			if (FreeSample < MAX_DSAMPLES) {
				CDSample *Sample = pDoc->GetDSample(FreeSample);
				strcpy(Sample->Name, SampleNames[Index]);
				Sample->SampleSize = Size;
				Sample->SampleData = SampleData;
				// Assign it
				for (int n = 0; n < (6 * 12); n++) {
					if (GetSample(n / 6, n % 13) == (Index + 1))
						SetSample(n / 6, n % 13, FreeSample + 1);
				}
			}
			else {
				theApp.DisplayError(IDS_OUT_OF_SLOTS);
			}
		}
		else
			delete [] SampleData;
	}

	return true;
}

int CInstrument2A03::CompileSize(CCompiler *pCompiler)
{
	int Size = 1;
	CFamiTrackerDoc *pDoc = pCompiler->GetDocument();

	for (int i = 0; i < SEQ_COUNT; ++i) {
		if (GetSeqEnable(i)) {
			CSequence *pSeq = pDoc->GetSequence(GetSeqIndex(i), i);
			if (pSeq->GetItemCount() > 0)
				Size += 2;
		}
	}

	return Size;
}

int CInstrument2A03::Compile(CCompiler *pCompiler, int Index)
{
	int ModSwitch;
	int StoredBytes = 0;
	int iAddress;
	int i;

	CFamiTrackerDoc *pDoc = pCompiler->GetDocument();

	pCompiler->WriteLog("2A03 {");
	ModSwitch = 0;
	
	for (i = 0; i < SEQ_COUNT; ++i) {
		ModSwitch = (ModSwitch >> 1) | ((GetSeqEnable(i) && (pDoc->GetSequence(GetSeqIndex(i), i)->GetItemCount() > 0)) ? 0x10 : 0);
	}
	
	pCompiler->StoreByte(ModSwitch);
	pCompiler->WriteLog("%02X ", ModSwitch);
	StoredBytes++;

	for (i = 0; i < SEQ_COUNT; ++i) {
		iAddress = (GetSeqEnable(i) == 0) ? 0 : pCompiler->GetSequenceAddress(GetSeqIndex(i), i);
		if (iAddress > 0) {
			pCompiler->StoreShort(iAddress);
			pCompiler->WriteLog("%04X ", iAddress);
			StoredBytes += 2;
		}
	}

	return StoredBytes;
}

int	CInstrument2A03::GetSeqEnable(int Index) const
{
	return m_iSeqEnable[Index];
}

int	CInstrument2A03::GetSeqIndex(int Index) const
{
	return m_iSeqIndex[Index];
}

void CInstrument2A03::SetSeqEnable(int Index, int Value)
{
	m_iSeqEnable[Index] = Value;
}

void CInstrument2A03::SetSeqIndex(int Index, int Value)
{
	m_iSeqIndex[Index] = Value;
}

char CInstrument2A03::GetSample(int Octave, int Note) const
{
	return m_cSamples[Octave][Note];
}

char CInstrument2A03::GetSamplePitch(int Octave, int Note) const
{
	return m_cSamplePitch[Octave][Note];
}

char CInstrument2A03::GetSampleLoopOffset(int Octave, int Note) const
{
	return m_cSampleLoopOffset[Octave][Note];
}

void CInstrument2A03::SetSample(int Octave, int Note, char Sample)
{
	m_cSamples[Octave][Note] = Sample;
}

void CInstrument2A03::SetSamplePitch(int Octave, int Note, char Pitch)
{
	m_cSamplePitch[Octave][Note] = Pitch;
}

void CInstrument2A03::SetSampleLoopOffset(int Octave, int Note, char Offset)
{
	m_cSampleLoopOffset[Octave][Note] = Offset;
}

bool CInstrument2A03::AssignedSamples() const
{
	// Returns true if there are assigned samples in this instrument

	int i, j;
	
	for (i = 0; i < OCTAVE_RANGE; i++) {
		for (j = 0; j < 12; j++) {
			if (GetSample(i, j) != 0)
				return true;
		}
	}

	return false;
}

void CInstrument2A03::SetPitchOption(int Option)
{
	m_iPitchOption = Option;
}

int CInstrument2A03::GetPitchOption() const
{
	return m_iPitchOption;
}
