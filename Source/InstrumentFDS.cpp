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

const char TEST_WAVE[] = {
	00, 01, 12, 22, 32, 36, 39, 39, 42, 47, 47, 50, 48, 51, 54, 58,
	54, 55, 49, 50, 52, 61, 63, 63, 59, 56, 53, 51, 48, 47, 41, 35,
	35, 35, 41, 47, 48, 51, 53, 56, 59, 63, 63, 61, 52, 50, 49, 55,
	54, 58, 54, 51, 48, 50, 47, 47, 42, 39, 39, 36, 32, 22, 12, 01};

const int FIXED_FDS_INST_SIZE = 1 + 16 + 4 + 1;

CInstrumentFDS::CInstrumentFDS() : CInstrument(), m_bChanged(false)
{
	memcpy(m_iSamples, TEST_WAVE, 64);	
	memset(m_iModulation, 0, 32);

	m_iModulationFreq = 0;
	m_iModulationDepth = 0;
	m_iModulationDelay = 0;

	m_pVolume = new CSequence();
	m_pArpeggio = new CSequence();
	m_pPitch = new CSequence();
}

CInstrumentFDS::~CInstrumentFDS()
{
	delete m_pVolume;
	delete m_pArpeggio;
	delete m_pPitch;
}

CInstrument *CInstrumentFDS::Clone()
{
	CInstrumentFDS *pNewInst = new CInstrumentFDS();

	// Copy parameters
	memcpy(pNewInst->m_iSamples, m_iSamples, 64);
	memcpy(pNewInst->m_iModulation, m_iModulation, 32);
	pNewInst->m_iModulationDelay = m_iModulationDelay;
	pNewInst->m_iModulationDepth = m_iModulationDepth;
	pNewInst->m_iModulationFreq = m_iModulationFreq;

	// Copy sequences
	pNewInst->m_pVolume->Copy(m_pVolume);
	pNewInst->m_pArpeggio->Copy(m_pArpeggio);
	pNewInst->m_pPitch->Copy(m_pPitch);

	// Copy name
	pNewInst->SetName(GetName());

	return pNewInst;
}


void CInstrumentFDS::StoreInstSequence(CFile *pFile, CSequence *pSeq)
{
	int i;
	// Store number of items in this sequence
	i = pSeq->GetItemCount();
	pFile->Write(&i, 4);
	// Store loop point
	i = pSeq->GetLoopPoint();
	pFile->Write(&i, 4);
	// Store release point (v4)
	i = pSeq->GetReleasePoint();
	pFile->Write(&i, 4);
	// Store setting (v4)
	i = pSeq->GetSetting();
	pFile->Write(&i, 4);
	// Store items
	for (unsigned int j = 0; j < pSeq->GetItemCount(); j++) {
		char c;
		c = pSeq->GetItem(j);
		pFile->Write(&c, 1);
	}
}

bool CInstrumentFDS::LoadInstSequence(CFile *pFile, CSequence *pSeq)
{
	int SeqCount;
	int LoopPoint;
	int ReleasePoint;
	int Settings;

	pFile->Read(&SeqCount, 4);
	pFile->Read(&LoopPoint, 4);
	pFile->Read(&ReleasePoint, 4);
	pFile->Read(&Settings, 4);

	if (SeqCount >= MAX_SEQUENCE_ITEMS)
		SeqCount = MAX_SEQUENCE_ITEMS - 1;

	ASSERT_FILE_DATA(SeqCount <= MAX_SEQUENCE_ITEMS);

	pSeq->Clear();
	pSeq->SetItemCount(SeqCount);
	pSeq->SetLoopPoint(LoopPoint);
	pSeq->SetReleasePoint(ReleasePoint);
	pSeq->SetSetting(Settings);

	for (int x = 0; x < SeqCount; x++) {
		char Value;// = pDocFile->GetBlockChar();
		pFile->Read(&Value, 1);
		pSeq->SetItem(x, Value);
	}

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

	SeqCount  = (unsigned char)pDocFile->GetBlockChar();
	LoopPoint = pDocFile->GetBlockInt();
	ReleasePoint = pDocFile->GetBlockInt();
	Settings = pDocFile->GetBlockInt();

	if (SeqCount >= MAX_SEQUENCE_ITEMS)
		SeqCount = MAX_SEQUENCE_ITEMS - 1;

	ASSERT_FILE_DATA(SeqCount <= MAX_SEQUENCE_ITEMS);

	pSeq->Clear();
	pSeq->SetItemCount(SeqCount);
	pSeq->SetLoopPoint(LoopPoint);
	pSeq->SetReleasePoint(ReleasePoint);
	pSeq->SetSetting(Settings);

	for (int x = 0; x < SeqCount; x++) {
		char Value = pDocFile->GetBlockChar();
		pSeq->SetItem(x, Value);
	}

	return true;
}

void CInstrumentFDS::Store(CDocumentFile *pDocFile)
{
	int i;

	// Write wave
	for (i = 0; i < 64; i++) {
		pDocFile->WriteBlockChar(GetSample(i));
	}

	// Write modulation table
	for (i = 0; i < 32; i++) {
		pDocFile->WriteBlockChar(GetModulation(i));
	}

	// Modulation parameters
	pDocFile->WriteBlockInt(GetModulationFreq());
	pDocFile->WriteBlockInt(GetModulationDepth());
	pDocFile->WriteBlockInt(GetModulationDelay());

	StoreSequence(pDocFile, m_pVolume);
	StoreSequence(pDocFile, m_pArpeggio);
	StoreSequence(pDocFile, m_pPitch);
}

bool CInstrumentFDS::Load(CDocumentFile *pDocFile)
{
	int i;
	
	for (i = 0; i < 64; i++) {
		SetSample(i, pDocFile->GetBlockChar());
	}

	for (i = 0; i < 32; i++) {
		SetModulation(i, pDocFile->GetBlockChar());
	}

	SetModulationFreq(pDocFile->GetBlockInt());
	SetModulationDepth(pDocFile->GetBlockInt());
	SetModulationDelay(pDocFile->GetBlockInt());

	// hack to fix earlier saved files
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
		LoadSequence(pDocFile, m_pVolume);
		LoadSequence(pDocFile, m_pArpeggio);
		if (pDocFile->GetBlockVersion() > 2)
			LoadSequence(pDocFile, m_pPitch);
	}

//	}

	// Older files was 0-15, new is 0-31
	if (pDocFile->GetBlockVersion() <= 3) {
		for (unsigned int i = 0; i < m_pVolume->GetItemCount(); ++i)
			m_pVolume->SetItem(i, m_pVolume->GetItem(i) * 2);
	}

	return true;
}

void CInstrumentFDS::SaveFile(CFile *pFile)
{
	int i;

	// Write wave
	for (i = 0; i < 64; i++) {
		char sample = GetSample(i);
		pFile->Write(&sample, 1);
	}

	// Write modulation table
	for (i = 0; i < 32; i++) {
		char mod = GetModulation(i);
		pFile->Write(&mod, 1);
	}

	// Modulation parameters
	int data;
	data = GetModulationFreq();
	pFile->Write(&data, sizeof(int));
	data = GetModulationDepth();
	pFile->Write(&data, sizeof(int));
	data = GetModulationDelay();
	pFile->Write(&data, sizeof(int));

	// Sequences
	StoreInstSequence(pFile, m_pVolume);
	StoreInstSequence(pFile, m_pArpeggio);
	StoreInstSequence(pFile, m_pPitch);
}

bool CInstrumentFDS::LoadFile(CFile *pFile, int iVersion)
{
	int i;

	// Write wave
	for (i = 0; i < 64; i++) {
		char sample;
		pFile->Read(&sample, 1);
		SetSample(i, sample);
	}

	// Write modulation table
	for (i = 0; i < 32; i++) {
		char mod;
		pFile->Read(&mod, 1);
		SetModulation(i, mod);
	}

	// Modulation parameters
	int data;
	pFile->Read(&data, sizeof(int));
	SetModulationFreq(data);
	pFile->Read(&data, sizeof(int));
	SetModulationDepth(data);
	pFile->Read(&data, sizeof(int));
	SetModulationDelay(data);

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

int CInstrumentFDS::CompileSize(CCompiler *pCompiler)
{
	int size = FIXED_FDS_INST_SIZE;
	size += (m_pVolume->GetItemCount() > 0 ? 2 : 0);
	size += (m_pArpeggio->GetItemCount() > 0 ? 2 : 0);
	size += (m_pPitch->GetItemCount() > 0 ? 2 : 0);
	return size;
}

int CInstrumentFDS::Compile(CCompiler *pCompiler, int Index)
{
	char Data;

	pCompiler->WriteLog("FDS {");

	// Store wave
	int Table = pCompiler->AddWavetable(m_iSamples);
	pCompiler->StoreByte(Table);	// waveform

	pCompiler->WriteLog("%i", Table);

	// Store modulation table, two entries/byte
	for (int i = 0; i < 16; ++i) {
		Data = GetModulation(i << 1) | (GetModulation((i << 1) + 1) << 3);
		pCompiler->StoreByte(Data);
	}

	pCompiler->StoreByte(GetModulationDelay());
	pCompiler->StoreByte(GetModulationDepth());
	pCompiler->StoreShort(GetModulationFreq());

	// Store sequences
	char Switch = (m_pVolume->GetItemCount() > 0 ? 1 : 0) | (m_pArpeggio->GetItemCount() > 0 ? 2 : 0) | (m_pPitch->GetItemCount() > 0 ? 4 : 0);

	pCompiler->StoreByte(Switch);

	// Volume
	if (Switch & 1)
		pCompiler->StoreShort(pCompiler->GetSequenceAddressFDS(Index, 0));

	// Arpeggio
	if (Switch & 2)
		pCompiler->StoreShort(pCompiler->GetSequenceAddressFDS(Index, 1));
	
	// Pitch
	if (Switch & 4)
		pCompiler->StoreShort(pCompiler->GetSequenceAddressFDS(Index, 2));

	return CompileSize(pCompiler);
}

bool CInstrumentFDS::HasChanged()
{
	bool Val = m_bChanged;
	m_bChanged = false;
	return Val;
}

unsigned char CInstrumentFDS::GetSample(int Index) const
{
	return m_iSamples[Index];
}

void CInstrumentFDS::SetSample(int Index, int Sample)
{
	m_iSamples[Index] = Sample;
	m_bChanged = true;
}

int CInstrumentFDS::GetModulation(int Index) const
{
	return m_iModulation[Index];
}

void CInstrumentFDS::SetModulation(int Index, int Value)
{
	m_iModulation[Index] = Value;
	m_bChanged = true;
}

int CInstrumentFDS::GetModulationFreq() const
{
	return m_iModulationFreq;
}

void CInstrumentFDS::SetModulationFreq(int Freq)
{
	m_iModulationFreq = Freq;
	m_bChanged = true;
}

int CInstrumentFDS::GetModulationDepth() const
{
	return m_iModulationDepth;
}

void CInstrumentFDS::SetModulationDepth(int Depth)
{
	m_iModulationDepth = Depth;
	m_bChanged = true;
}

int CInstrumentFDS::GetModulationDelay() const
{
	return m_iModulationDelay;
}

void CInstrumentFDS::SetModulationDelay(int Delay)
{
	m_iModulationDelay = Delay;
	m_bChanged = true;
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
