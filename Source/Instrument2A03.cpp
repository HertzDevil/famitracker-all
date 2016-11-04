/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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

#include <vector>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "Instrument.h"
#include "Compiler.h"
#include "Chunk.h"
#include "DocumentFile.h"

// 2A03 instruments

const int CInstrument2A03::SEQUENCE_TYPES[] = {
	SEQ_VOLUME, 
	SEQ_ARPEGGIO, 
	SEQ_PITCH, 
	SEQ_HIPITCH, 
	SEQ_DUTYCYCLE
};

CInstrument2A03::CInstrument2A03()
{
	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		m_iSeqEnable[i] = 0;
		m_iSeqIndex[i] = 0;
	}

	for (int i = 0; i < OCTAVE_RANGE; ++i) {
		for (int j = 0; j < 12; ++j) {
			m_cSamples[i][j] = 0;
			m_cSamplePitch[i][j] = 0;
			m_cSampleLoopOffset[i][j] = 0;
			m_cSampleDelta[i][j] = -1;
		}
	}
}

CInstrument *CInstrument2A03::Clone() const
{
	CInstrument2A03 *pNew = new CInstrument2A03();

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		pNew->SetSeqEnable(i, GetSeqEnable(i));
		pNew->SetSeqIndex(i, GetSeqIndex(i));
	}

	for (int i = 0; i < OCTAVE_RANGE; ++i) {
		for (int j = 0; j < 12; ++j) {
			pNew->SetSample(i, j, GetSample(i, j));
			pNew->SetSamplePitch(i, j, GetSamplePitch(i, j));
		}
	}

	pNew->SetName(GetName());

	return pNew;
}

void CInstrument2A03::Setup()
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();

	// Select free sequences
	for (int i = 0; i < SEQ_COUNT; ++i) {
		SetSeqEnable(i, 0);
		int Slot = pDoc->GetFreeSequence(i);
		if (Slot != -1)
			SetSeqIndex(i, Slot);
	}
}

void CInstrument2A03::Store(CDocumentFile *pDocFile)
{
	pDocFile->WriteBlockInt(SEQUENCE_COUNT);

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		pDocFile->WriteBlockChar(GetSeqEnable(i));
		pDocFile->WriteBlockChar(GetSeqIndex(i));
	}

	for (int i = 0; i < OCTAVE_RANGE; ++i) {
		for (int j = 0; j < 12; ++j) {
			pDocFile->WriteBlockChar(GetSample(i, j));
			pDocFile->WriteBlockChar(GetSamplePitch(i, j));
			pDocFile->WriteBlockChar(GetSampleDeltaValue(i, j));
		}
	}
}

bool CInstrument2A03::Load(CDocumentFile *pDocFile)
{
	int Version = pDocFile->GetBlockVersion();

	int SeqCnt = pDocFile->GetBlockInt();
	ASSERT_FILE_DATA(SeqCnt < (SEQUENCE_COUNT + 1));

	for (int i = 0; i < SeqCnt; ++i) {
		SetSeqEnable(i, pDocFile->GetBlockChar());
		int Index = pDocFile->GetBlockChar();
		ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
		SetSeqIndex(i, Index);
	}

	int Octaves = (Version == 1) ? 6 : OCTAVE_RANGE;

	for (int i = 0; i < Octaves; ++i) {
		for (int j = 0; j < 12; ++j) {
			int Index = pDocFile->GetBlockChar();
			if (Index > MAX_DSAMPLES)
				Index = 0;
			SetSample(i, j, Index);
			SetSamplePitch(i, j, pDocFile->GetBlockChar());
			if (Version > 5) {
				char Value = pDocFile->GetBlockChar();
				if (Value != -1 && Value < 0)
					Value = -1;
				SetSampleDeltaValue(i, j, Value);
			}
		}
	}

	return true;
}

void CInstrument2A03::SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc)
{
	// Saves an 2A03 instrument
	// Current version 2.4

	// Sequences
	pFile->WriteChar(SEQUENCE_COUNT);

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		unsigned int Sequence = GetSeqIndex(i);
		if (GetSeqEnable(i)) {
			const CSequence *pSeq = pDoc->GetSequence(Sequence, i);
			pFile->WriteChar(1);
			pFile->WriteInt(pSeq->GetItemCount());
			pFile->WriteInt(pSeq->GetLoopPoint());
			pFile->WriteInt(pSeq->GetReleasePoint());
			pFile->WriteInt(pSeq->GetSetting());
			for (unsigned int j = 0; j < pSeq->GetItemCount(); j++) {
				pFile->WriteChar(pSeq->GetItem(j));
			}
		}
		else {
			pFile->WriteChar(0);
		}
	}

	unsigned int Count = 0;

	// Count assigned keys
	for (int i = 0; i < 8; ++i) {	// octaves
		for (int j = 0; j < 12; ++j) {	// notes
			Count += (GetSample(i, j) > 0) ? 1 : 0;
		}
	}
	pFile->WriteInt(Count);

	bool UsedSamples[MAX_DSAMPLES];
	memset(UsedSamples, 0, sizeof(bool) * MAX_DSAMPLES);

	// DPCM
	for (int i = 0; i < 8; ++i) {	// octaves
		for (int j = 0; j < 12; ++j) {	// notes
			if (GetSample(i, j) > 0) {
				unsigned char Index = i * 12 + j;
				unsigned char Sample = GetSample(i, j);
				pFile->WriteChar(Index);
				pFile->WriteChar(Sample);
				pFile->WriteChar(GetSamplePitch(i, j));
				pFile->WriteChar(GetSampleDeltaValue(i, j));
				UsedSamples[Sample - 1] = true;
			}
		}
	}

	int SampleCount = 0;

	// Count samples
	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (pDoc->IsSampleUsed(i) && UsedSamples[i])
			++SampleCount;
	}

	// Write the number
	pFile->WriteInt(SampleCount);

	// List of sample names
	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (pDoc->IsSampleUsed(i) && UsedSamples[i]) {
			const CDSample *pSample = pDoc->GetSample(i);
			pFile->WriteInt(i);
			const char *pName = pSample->GetName();
			int NameLen = strlen(pName);
			pFile->WriteInt(NameLen);
			pFile->Write(pName, NameLen);
			pFile->WriteInt(pSample->GetSize());
			pFile->Write(pSample->GetData(), pSample->GetSize());
		}
	}
}

bool CInstrument2A03::LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc)
{
	// Reads an FTI file
	//

	char SampleNames[MAX_DSAMPLES][256];
	stSequence OldSequence;

	// Sequences
	unsigned char SeqCount = pFile->ReadChar();

	// Loop through all instrument effects
	for (unsigned int i = 0; i < SeqCount; ++i) {

		unsigned char Enabled = pFile->ReadChar();
		if (Enabled == 1) {
			// Read the sequence
			int Count = pFile->ReadInt();
			if (Count < 0 || Count > MAX_SEQUENCE_ITEMS)
				return false;

			// Find a free sequence
			int Index = pDoc->GetFreeSequence(i);
			if (Index != -1) {
				CSequence *pSeq = pDoc->GetSequence((unsigned)Index, i);

				if (iVersion < 20) {
					OldSequence.Count = Count;
					for (int j = 0; j < Count; ++j) {
						OldSequence.Length[j] = pFile->ReadChar();
						OldSequence.Value[j] = pFile->ReadChar();
					}
					CFamiTrackerDoc::ConvertSequence(&OldSequence, pSeq, i);	// convert
				}
				else {
					pSeq->SetItemCount(Count);
					int LoopPoint = pFile->ReadInt();
					pSeq->SetLoopPoint(LoopPoint);
					if (iVersion > 20) {
						int ReleasePoint = pFile->ReadInt();
						pSeq->SetReleasePoint(ReleasePoint);
					}
					if (iVersion >= 23) {
						int Setting = pFile->ReadInt();
						pSeq->SetSetting(Setting);
					}
					for (int j = 0; j < Count; ++j) {
						char Val = pFile->ReadChar();
						pSeq->SetItem(j, Val);
					}
				}
				SetSeqEnable(i, true);
				SetSeqIndex(i, Index);
			}
		}
		else {
			SetSeqEnable(i, false);
			SetSeqIndex(i, 0);
		}
	}

	bool SamplesFound[MAX_DSAMPLES];
	memset(SamplesFound, 0, sizeof(bool) * MAX_DSAMPLES);

	unsigned int Count;
	pFile->Read(&Count, sizeof(int));

	// DPCM instruments
	for (unsigned int i = 0; i < Count; ++i) {
		unsigned char InstNote = pFile->ReadChar();
		int Octave = InstNote / 12;
		int Note = InstNote % 12;
		unsigned char Sample = pFile->ReadChar();
		unsigned char Pitch = pFile->ReadChar();
		unsigned char Delta;
		if (iVersion >= 24)
			Delta = pFile->ReadChar();
		else
			Delta = 0xFF;
		SetSamplePitch(Octave, Note, Pitch);
		SetSample(Octave, Note, Sample);
		SetSampleDeltaValue(Octave, Note, Delta);
	}

	// DPCM samples list
	bool bAssigned[OCTAVE_RANGE][NOTE_RANGE];
	memset(bAssigned, 0, sizeof(bool) * OCTAVE_RANGE * NOTE_RANGE);

	unsigned int SampleCount = pFile->ReadInt();
	for (unsigned int i = 0; i < SampleCount; ++i) {
		int Index = pFile->ReadInt();
		int Len = pFile->ReadInt();
		if (Index >= MAX_DSAMPLES || Len >= 256)
			return false;
		pFile->Read(SampleNames[Index], Len);
		SampleNames[Index][Len] = 0;
		int Size = pFile->ReadInt();
		char *SampleData = new char[Size];
		pFile->Read(SampleData, Size);
		bool Found = false;
		for (int j = 0; j < MAX_DSAMPLES; ++j) {
			CDSample *pSample = pDoc->GetSample(j);
			// Compare size and name to see if identical sample exists
			if (pSample->GetSize() == Size && !strcmp(pSample->GetName(), SampleNames[Index])) {
				Found = true;
				// Assign sample
				for (int o = 0; o < OCTAVE_RANGE; ++o) {
					for (int n = 0; n < NOTE_RANGE; ++n) {
						if (GetSample(o, n) == (Index + 1) && !bAssigned[o][n]) {
							SetSample(o, n, j + 1);
							bAssigned[o][n] = true;
						}
					}
				}
			}
		}

		if (!Found) {
			// Load sample			
			int FreeSample = pDoc->GetFreeSampleSlot();
			if (FreeSample != -1) {
				if ((pDoc->GetTotalSampleSize() + Size) <= MAX_SAMPLE_SPACE) {
					CDSample *pSample = pDoc->GetSample(FreeSample);
					pSample->SetName(SampleNames[Index]);
					pSample->SetData(Size, SampleData);
					// Assign it
					for (int o = 0; o < OCTAVE_RANGE; ++o) {
						for (int n = 0; n < NOTE_RANGE; ++n) {
							if (GetSample(o, n) == (Index + 1) && !bAssigned[o][n]) {
								SetSample(o, n, FreeSample + 1);
								bAssigned[o][n] = true;
							}
						}
					}
				}
				else {
					CString message;
					message.Format(IDS_OUT_OF_SAMPLEMEM_FORMAT, MAX_SAMPLE_SPACE / 1024);
					AfxMessageBox(message, MB_ICONERROR);
					SAFE_RELEASE_ARRAY(SampleData);
					return false;
				}
			}
			else {
				AfxMessageBox(IDS_OUT_OF_SLOTS, MB_ICONERROR);
				SAFE_RELEASE_ARRAY(SampleData);
				return false;
			}
		}
		else {
			SAFE_RELEASE_ARRAY(SampleData);
		}
	}

	return true;
}

int CInstrument2A03::Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index)
{
	int ModSwitch = 0;
	int StoredBytes = 0;

	for (unsigned int i = 0; i < SEQUENCE_COUNT; ++i) {
		const CSequence *pSequence = pDoc->GetSequence(unsigned(GetSeqIndex(i)), i);
		ModSwitch = (ModSwitch >> 1) | ((GetSeqEnable(i) && (pSequence->GetItemCount() > 0)) ? 0x10 : 0);
	}
	
	pChunk->StoreByte(ModSwitch);
	StoredBytes++;

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		const CSequence *pSequence = pDoc->GetSequence(unsigned(GetSeqIndex(i)), i);
		if (GetSeqEnable(i) != 0 && (pSequence->GetItemCount() != 0)) {
			CStringA str;
			str.Format(CCompiler::LABEL_SEQ_2A03, GetSeqIndex(i) * SEQUENCE_COUNT + i);
			pChunk->StoreReference(str);
			StoredBytes += 2;
		}
	}

	return StoredBytes;
}

bool CInstrument2A03::CanRelease() const
{
	if (GetSeqEnable(0) != 0) {
		int index = GetSeqIndex(SEQ_VOLUME);
		return CFamiTrackerDoc::GetDoc()->GetSequence(SNDCHIP_NONE, index, SEQ_VOLUME)->GetReleasePoint() != -1;
	}

	return false;
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
	if (m_iSeqEnable[Index] != Value)
		InstrumentChanged();
	m_iSeqEnable[Index] = Value;
}

void CInstrument2A03::SetSeqIndex(int Index, int Value)
{
	if (m_iSeqIndex[Index] != Value)
		InstrumentChanged();
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

bool CInstrument2A03::GetSampleLoop(int Octave, int Note) const
{
	return (m_cSamplePitch[Octave][Note] & 0x80) == 0x80;
}

char CInstrument2A03::GetSampleLoopOffset(int Octave, int Note) const
{
	return m_cSampleLoopOffset[Octave][Note];
}

char CInstrument2A03::GetSampleDeltaValue(int Octave, int Note) const
{
	return m_cSampleDelta[Octave][Note];
}

void CInstrument2A03::SetSample(int Octave, int Note, char Sample)
{
	m_cSamples[Octave][Note] = Sample;
	InstrumentChanged();
}

void CInstrument2A03::SetSamplePitch(int Octave, int Note, char Pitch)
{
	m_cSamplePitch[Octave][Note] = Pitch;
	InstrumentChanged();
}

void CInstrument2A03::SetSampleLoop(int Octave, int Note, bool Loop)
{
	m_cSamplePitch[Octave][Note] = (m_cSamplePitch[Octave][Note] & 0x7F) | (Loop ? 0x80 : 0);
	InstrumentChanged();
}

void CInstrument2A03::SetSampleLoopOffset(int Octave, int Note, char Offset)
{
	m_cSampleLoopOffset[Octave][Note] = Offset;
	InstrumentChanged();
}

void CInstrument2A03::SetSampleDeltaValue(int Octave, int Note, char Value)
{
	m_cSampleDelta[Octave][Note] = Value;
	InstrumentChanged();
}

bool CInstrument2A03::AssignedSamples() const
{
	// Returns true if there are assigned samples in this instrument	

	for (int i = 0; i < OCTAVE_RANGE; ++i) {
		for (int j = 0; j < NOTE_RANGE; ++j) {
			if (GetSample(i, j) != 0)
				return true;
		}
	}

	return false;
}
