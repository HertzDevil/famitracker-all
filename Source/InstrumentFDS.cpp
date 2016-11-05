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

#include <map>
#include <vector>
#include "stdafx.h"
#include "FamiTrackerDoc.h"
#include "Instrument.h"
#include "Compiler.h"
#include "Chunk.h"
#include "DocumentFile.h"

const char TEST_WAVE[] = {
	00, 01, 12, 22, 32, 36, 39, 39, 42, 47, 47, 50, 48, 51, 54, 58,
	54, 55, 49, 50, 52, 61, 63, 63, 59, 56, 53, 51, 48, 47, 41, 35,
	35, 35, 41, 47, 48, 51, 53, 56, 59, 63, 63, 61, 52, 50, 49, 55,
	54, 58, 54, 51, 48, 50, 47, 47, 42, 39, 39, 36, 32, 22, 12, 01
};

const int FIXED_FDS_INST_SIZE = 1 + 16 + 4 + 1;

// // //
const int CInstrumentFDS::SEQUENCE_TYPES[] = {SEQ_VOLUME, SEQ_ARPEGGIO, SEQ_PITCH, SEQ_HIPITCH, SEQ_DUTYCYCLE};

CInstrumentFDS::CInstrumentFDS() : CInstrument()
{
	memcpy(m_iSamples[0], TEST_WAVE, WAVE_SIZE);		// // //
	memset(m_iSamples[1], 0, WAVE_SIZE * (MAX_WAVE_COUNT - 1));
	memset(m_iModulation, 0, MOD_SIZE);

	m_iModulationSpeed = 0;
	m_iModulationDepth = 0;
	m_iModulationDelay = 0;
	m_bModulationEnable = true;

	m_pVolume = new CSequence();
	m_pArpeggio = new CSequence();
	m_pPitch = new CSequence();
	for (int i = 0; i < SEQUENCE_COUNT; i++) {		// // //
		m_iSeqEnable[i] = 0;
		m_iSeqIndex[i] = 0;
	}
	
	m_iWaveCount = 1;		// // //
}

CInstrumentFDS::~CInstrumentFDS()
{
	SAFE_RELEASE(m_pVolume);
	SAFE_RELEASE(m_pArpeggio);
	SAFE_RELEASE(m_pPitch);
}

CInstrument *CInstrumentFDS::Clone() const
{
	CInstrumentFDS *pNewInst = new CInstrumentFDS();

	// Copy parameters
	memcpy(pNewInst->m_iSamples, m_iSamples, WAVE_SIZE * MAX_WAVE_COUNT);		// // //
	memcpy(pNewInst->m_iModulation, m_iModulation, MOD_SIZE);
	pNewInst->m_iModulationDelay = m_iModulationDelay;
	pNewInst->m_iModulationDepth = m_iModulationDepth;
	pNewInst->m_iModulationSpeed = m_iModulationSpeed;
	pNewInst->m_iWaveCount = m_iWaveCount;		// // //

	// Copy sequences
	for (int i = 0; i < SEQUENCE_COUNT; i++) {		// // //
		pNewInst->SetSeqEnable(i, GetSeqEnable(i));
		pNewInst->SetSeqIndex(i, GetSeqIndex(i));
	}

	// Copy name
	pNewInst->SetName(GetName());

	return pNewInst;
}

void CInstrumentFDS::Setup()		// // //
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();

	for (int i = 0; i < SEQ_COUNT; ++i) {
		SetSeqEnable(i, 0);
		int Index = pDoc->GetFreeSequenceFDS(i);
		if (Index != -1)
			SetSeqIndex(i, Index);
	}
}

void CInstrumentFDS::StoreInstSequence(CInstrumentFile *pFile, CSequence *pSeq)
{
	// Store number of items in this sequence
	pFile->WriteInt(pSeq->GetItemCount());
	// Store loop point
	pFile->WriteInt(pSeq->GetLoopPoint());
	// Store release point (v4)
	pFile->WriteInt(pSeq->GetReleasePoint());
	// Store setting (v4)
	pFile->WriteInt(pSeq->GetSetting());
	// Store items
	for (unsigned i = 0; i < pSeq->GetItemCount(); ++i)
		pFile->WriteChar(pSeq->GetItem(i));
}

bool CInstrumentFDS::LoadInstSequence(CInstrumentFile *pFile, CSequence *pSeq)
{
	unsigned SeqCount = pFile->ReadInt();
	if (SeqCount > MAX_SEQUENCE_ITEMS)
		SeqCount = MAX_SEQUENCE_ITEMS;

	pSeq->Clear();
	pSeq->SetItemCount(SeqCount);
	pSeq->SetLoopPoint(pFile->ReadInt());
	pSeq->SetReleasePoint(pFile->ReadInt());
	pSeq->SetSetting(pFile->ReadInt());

	for (unsigned i = 0; i < SeqCount; ++i)
		pSeq->SetItem(i, pFile->ReadChar());

	return true;
}

void CInstrumentFDS::StoreSequence(CDocumentFile *pDocFile, CSequence *pSeq)
{
	// Store number of items in this sequence
	pDocFile->WriteBlockChar(pSeq->GetItemCount());
	// Store loop point
	pDocFile->WriteBlockInt(pSeq->GetLoopPoint());
	// Store release point (v4)
	pDocFile->WriteBlockInt(pSeq->GetReleasePoint());
	// Store setting (v4)
	pDocFile->WriteBlockInt(pSeq->GetSetting());
	// Store items
	for (unsigned int j = 0; j < pSeq->GetItemCount(); j++) {
		pDocFile->WriteBlockChar(pSeq->GetItem(j));
	}
}

bool CInstrumentFDS::LoadSequence(CDocumentFile *pDocFile, CSequence *pSeq)
{
	int SeqCount;
	unsigned int LoopPoint;
	unsigned int ReleasePoint;
	unsigned int Settings;

	SeqCount = (unsigned char)pDocFile->GetBlockChar();
	LoopPoint = pDocFile->GetBlockInt();
	ReleasePoint = pDocFile->GetBlockInt();
	Settings = pDocFile->GetBlockInt();

	ASSERT_FILE_DATA(SeqCount <= MAX_SEQUENCE_ITEMS);

	pSeq->Clear();
	pSeq->SetItemCount(SeqCount);
	pSeq->SetLoopPoint(LoopPoint);
	pSeq->SetReleasePoint(ReleasePoint);
	pSeq->SetSetting(Settings);

	for (int x = 0; x < SeqCount; ++x) {
		char Value = pDocFile->GetBlockChar();
		pSeq->SetItem(x, Value);
	}

	return true;
}

void CInstrumentFDS::Store(CDocumentFile *pDocFile)
{
	// Write wave
	pDocFile->WriteBlockInt(m_iWaveCount);		// // //
	for (int j = 0; j < m_iWaveCount; j++) {
		for (int i = 0; i < WAVE_SIZE; ++i) {
			pDocFile->WriteBlockChar(GetSample(j, i));
		}
	}

	// Write modulation table
	for (int i = 0; i < MOD_SIZE; ++i) {
		pDocFile->WriteBlockChar(GetModulation(i));
	}

	// Modulation parameters
	pDocFile->WriteBlockInt(GetModulationSpeed());
	pDocFile->WriteBlockInt(GetModulationDepth());
	pDocFile->WriteBlockInt(GetModulationDelay());

	// // // Sequences
	pDocFile->WriteBlockInt(SEQUENCE_COUNT);

	for (int i = 0; i < SEQUENCE_COUNT; i++) {
		pDocFile->WriteBlockChar(GetSeqEnable(i));
		pDocFile->WriteBlockChar(GetSeqIndex(i));
	}
}

bool CInstrumentFDS::Load(CDocumentFile *pDocFile)
{
	m_iWaveCount = 1;		// // //
	for (int i = 0; i < WAVE_SIZE; ++i) {
		SetSample(0, i, pDocFile->GetBlockChar());
	}

	for (int i = 0; i < MOD_SIZE; ++i) {
		SetModulation(i, pDocFile->GetBlockChar());
	}

	SetModulationSpeed(pDocFile->GetBlockInt());
	SetModulationDepth(pDocFile->GetBlockInt());
	SetModulationDelay(pDocFile->GetBlockInt());

	// hack to fix earlier saved files (remove this eventually)
/*
	if (pDocFile->GetBlockVersion() > 2) {
		LoadSequence(pDocFile, m_pVolume);
		LoadSequence(pDocFile, m_pArpeggio);
		if (pDocFile->GetBlockVersion() > 2)
			LoadSequence(pDocFile, m_pPitch);
	}
	else {
*/
	unsigned int a = pDocFile->GetBlockInt();
	unsigned int b = pDocFile->GetBlockInt();
	pDocFile->RollbackPointer(8);

	if (a < 256 && (b & 0xFF) != 0x00) {
	}
	else {
		CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();		// // //
		CSequence *vSeq;
		static const sequence_t FDSSEQTYPE[] = {SEQ_VOLUME, SEQ_ARPEGGIO, SEQ_PITCH};
		int id;
		for (int i = 0; i < sizeof(FDSSEQTYPE) / sizeof(sequence_t); i++) {
			if (FDSSEQTYPE[i] == SEQ_PITCH && pDocFile->GetBlockVersion() <= 2) break;
			id = 0;
			vSeq = NULL;
			do {
				vSeq = pDoc->GetSequenceFDS(id++, FDSSEQTYPE[i]);
			} while (vSeq->GetItemCount() > 0);
			ASSERT(vSeq != NULL);
			LoadSequence(pDocFile, vSeq);
			if (vSeq->GetItemCount() > 0) {
				SetSeqEnable(FDSSEQTYPE[i], 1);
				SetSeqIndex(FDSSEQTYPE[i], --id);
			}
			else {
				SetSeqEnable(FDSSEQTYPE[i], 0);
				SetSeqIndex(FDSSEQTYPE[i], 0);
			}
		}
	}

//	}

	// Older files was 0-15, new is 0-31
	if (pDocFile->GetBlockVersion() <= 3) {
		for (unsigned int i = 0; i < m_pVolume->GetItemCount(); ++i)
			m_pVolume->SetItem(i, m_pVolume->GetItem(i) * 2);
	}

	return true;
}

bool CInstrumentFDS::LoadNew(CDocumentFile *pDocFile)		// // //
{
	m_iWaveCount = pDocFile->GetBlockInt();		// // //
	ASSERT_FILE_DATA(m_iWaveCount >= 1 && m_iWaveCount <= MAX_WAVE_COUNT);
	for (int j = 0; j < m_iWaveCount; j++) {
		for (int i = 0; i < WAVE_SIZE; ++i) {
			SetSample(j, i, pDocFile->GetBlockChar());
		}
	}

	for (int i = 0; i < MOD_SIZE; ++i) {
		SetModulation(i, pDocFile->GetBlockChar());
	}

	SetModulationSpeed(pDocFile->GetBlockInt());
	SetModulationDepth(pDocFile->GetBlockInt());
	SetModulationDelay(pDocFile->GetBlockInt());

	int SeqCnt = pDocFile->GetBlockInt();

	ASSERT_FILE_DATA(SeqCnt < (SEQUENCE_COUNT + 1));

	SeqCnt = SEQUENCE_COUNT;//SEQ_COUNT;

	for (int i = 0; i < SeqCnt; i++) {
		SetSeqEnable(i, pDocFile->GetBlockChar());
		int Index = pDocFile->GetBlockChar();
		ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
		SetSeqIndex(i, Index);
	}

	return true;
}

void CInstrumentFDS::SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc)
{
	// Write wave
	for (int i = 0; i < WAVE_SIZE; ++i) {
		pFile->WriteChar(GetSample(0, i));
	}

	// Write modulation table
	for (int i = 0; i < MOD_SIZE; ++i) {
		pFile->WriteChar(GetModulation(i));
	}

	// Modulation parameters
	pFile->WriteInt(GetModulationSpeed());
	pFile->WriteInt(GetModulationDepth());
	pFile->WriteInt(GetModulationDelay());

	// Sequences
	StoreInstSequence(pFile, m_pVolume);
	StoreInstSequence(pFile, m_pArpeggio);
	StoreInstSequence(pFile, m_pPitch);
}

bool CInstrumentFDS::LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc)
{
	// Read wave
	for (int i = 0; i < WAVE_SIZE; ++i) {
		SetSample(0, i, pFile->ReadChar());
	}

	// Read modulation table
	for (int i = 0; i < MOD_SIZE; ++i) {
		SetModulation(i, pFile->ReadChar());
	}

	// Modulation parameters
	SetModulationSpeed(pFile->ReadInt());
	SetModulationDepth(pFile->ReadInt());
	SetModulationDelay(pFile->ReadInt());

	// Sequences
	LoadInstSequence(pFile, m_pVolume);
	LoadInstSequence(pFile, m_pArpeggio);
	LoadInstSequence(pFile, m_pPitch);

	if (iVersion <= 22) {
		for (unsigned int i = 0; i < m_pVolume->GetItemCount(); ++i)
			m_pVolume->SetItem(i, m_pVolume->GetItem(i) * 2);
	}

	return true;
}

int CInstrumentFDS::Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index)
{
	CStringA str;

	// Store wave
//	int Table = pCompiler->AddWavetable(m_iSamples);
//	int Table = 0;
//	pChunk->StoreByte(Table);

	// Store modulation table, two entries/byte
	for (int i = 0; i < 16; ++i) {
		char Data = GetModulation(i << 1) | (GetModulation((i << 1) + 1) << 3);
		pChunk->StoreByte(Data);
	}
	
	pChunk->StoreByte(GetModulationDelay());
	pChunk->StoreByte(GetModulationDepth());
	pChunk->StoreWord(GetModulationSpeed());

	// // // Store reference to wave
	CStringA waveLabel;
	waveLabel.Format(CCompiler::LABEL_WAVES, Index);
	pChunk->StoreReference(waveLabel);

	// Store sequences
	char Switch = 0;		// // //
	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		Switch = (Switch >> 1) | (GetSeqEnable(i) && (pDoc->GetSequence(SNDCHIP_FDS, GetSeqIndex(i), i)->GetItemCount() > 0) ? 0x10 : 0);
	}
	pChunk->StoreByte(Switch);

	// // //
	int size = FIXED_FDS_INST_SIZE + 2;
	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		if (GetSeqEnable(i) != 0 && (pDoc->GetSequence(SNDCHIP_FDS, GetSeqIndex(i), i)->GetItemCount() != 0)) {
			CStringA str;
			str.Format(CCompiler::LABEL_SEQ_FDS, GetSeqIndex(i) * SEQUENCE_COUNT + i);
			pChunk->StoreReference(str);
			size += 2;
		}
	}
	
	return size;
}

int CInstrumentFDS::StoreWave(CChunk *pChunk) const		// // //
{
	// Number of waves
//	pChunk->StoreByte(m_iWaveCount);

	// Pack samples
	for (int i = 0; i < m_iWaveCount; ++i) {
		for (int j = 0; j < WAVE_SIZE; j++) {
			pChunk->StoreByte(m_iSamples[i][j]);
		}
	}

	return m_iWaveCount * WAVE_SIZE;
}

bool CInstrumentFDS::IsWaveEqual(CInstrumentFDS *pInstrument)		// // //
{
	int Count = GetWaveCount();

	if (pInstrument->GetWaveCount() != Count)
		return false;

	for (int i = 0; i < Count; ++i) {
		for (int j = 0; j < WAVE_SIZE; ++j) {
			if (GetSample(i, j) != pInstrument->GetSample(i, j))
				return false;
		}
	}

	return true;
}

bool CInstrumentFDS::CanRelease() const
{
	if (GetSeqEnable(SEQ_VOLUME) != 0) {		// // //
		int index = GetSeqIndex(SEQ_VOLUME);
		return CFamiTrackerDoc::GetDoc()->GetSequence(SNDCHIP_FDS, index, SEQ_VOLUME)->GetReleasePoint() != -1;
	}
	return false;
}

unsigned char CInstrumentFDS::GetSample(int Wave, int Index) const		// // //
{
	ASSERT(Wave < MAX_WAVE_COUNT);
	ASSERT(Index < WAVE_SIZE);
	return m_iSamples[Wave][Index];
}

void CInstrumentFDS::SetSample(int Wave, int Index, int Sample)		// // //
{
	ASSERT(Wave < MAX_WAVE_COUNT);
	ASSERT(Index < WAVE_SIZE);
	m_iSamples[Wave][Index] = Sample;
	InstrumentChanged();
}

int CInstrumentFDS::GetModulation(int Index) const
{
	return m_iModulation[Index];
}

void CInstrumentFDS::SetModulation(int Index, int Value)
{
	m_iModulation[Index] = Value;
	InstrumentChanged();
}

int CInstrumentFDS::GetModulationSpeed() const
{
	return m_iModulationSpeed;
}

void CInstrumentFDS::SetModulationSpeed(int Speed)
{
	m_iModulationSpeed = Speed;
	InstrumentChanged();
}

int CInstrumentFDS::GetModulationDepth() const
{
	return m_iModulationDepth;
}

void CInstrumentFDS::SetModulationDepth(int Depth)
{
	m_iModulationDepth = Depth;
	InstrumentChanged();
}

int CInstrumentFDS::GetModulationDelay() const
{
	return m_iModulationDelay;
}

void CInstrumentFDS::SetModulationDelay(int Delay)
{
	m_iModulationDelay = Delay;
	InstrumentChanged();
}

CSequence* CInstrumentFDS::GetVolumeSeq() const
{
	return m_pVolume;
}

CSequence* CInstrumentFDS::GetArpSeq() const
{
	return m_pArpeggio;
}

CSequence* CInstrumentFDS::GetPitchSeq() const
{
	return m_pPitch;
}

bool CInstrumentFDS::GetModulationEnable() const
{
	return m_bModulationEnable;
}

void CInstrumentFDS::SetModulationEnable(bool Enable)
{
	m_bModulationEnable = Enable;
	InstrumentChanged();
}

int	CInstrumentFDS::GetSeqEnable(int Index) const		// // //
{
	return m_iSeqEnable[Index];
}

int	CInstrumentFDS::GetSeqIndex(int Index) const
{
	return m_iSeqIndex[Index];
}

void CInstrumentFDS::SetSeqEnable(int Index, int Value)
{
	if (m_iSeqEnable[Index] != Value)
		InstrumentChanged();		
	m_iSeqEnable[Index] = Value;
}

void CInstrumentFDS::SetSeqIndex(int Index, int Value)
{
	if (m_iSeqIndex[Index] != Value)
		InstrumentChanged();
	m_iSeqIndex[Index] = Value;
}

void CInstrumentFDS::SetWaveCount(int count)		// // //
{
	ASSERT(count <= MAX_WAVE_COUNT);
	m_iWaveCount = count;
	InstrumentChanged();
}

int CInstrumentFDS::GetWaveCount() const		// // //
{
	return m_iWaveCount;
}
