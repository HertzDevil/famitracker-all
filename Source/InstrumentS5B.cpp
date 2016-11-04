/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2012  Jonathan Liss
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
#include "FamiTrackerDoc.h"
#include "Instrument.h"
#include "DocumentFile.h"

const int CInstrumentS5B::SEQUENCE_TYPES[] = {SEQ_VOLUME, SEQ_ARPEGGIO, SEQ_PITCH, SEQ_HIPITCH, SEQ_DUTYCYCLE};

CInstrumentS5B::CInstrumentS5B()
{
	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		m_iSeqEnable[i] = 0;
		m_iSeqIndex[i] = 0;
	}
}

CInstrument *CInstrumentS5B::Clone() const
{
	CInstrumentS5B *pNew = new CInstrumentS5B();

	for (int i = 0; i < SEQUENCE_COUNT; i++) {
		pNew->SetSeqEnable(i, GetSeqEnable(i));
		pNew->SetSeqIndex(i, GetSeqIndex(i));
	}

	pNew->SetName(GetName());

	return pNew;
}

void CInstrumentS5B::Setup()
{
}

void CInstrumentS5B::Store(CDocumentFile *pDocFile)
{
	pDocFile->WriteBlockInt(SEQUENCE_COUNT);

	for (int i = 0; i < SEQUENCE_COUNT; i++) {
		pDocFile->WriteBlockChar(GetSeqEnable(i));
		pDocFile->WriteBlockChar(GetSeqIndex(i));
	}
}

bool CInstrumentS5B::Load(CDocumentFile *pDocFile)
{
	int SeqCnt = pDocFile->GetBlockInt();

	ASSERT_FILE_DATA(SeqCnt < (SEQUENCE_COUNT + 1));

	SeqCnt = SEQUENCE_COUNT;

	for (int i = 0; i < SeqCnt; i++) {
		SetSeqEnable(i, pDocFile->GetBlockChar());
		int Index = pDocFile->GetBlockChar();
		ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
		SetSeqIndex(i, Index);
	}

	return true;
}

/*void CInstrumentS5B::SaveFile(CFile *pFile, CFamiTrackerDoc *pDoc)
{
	AfxMessageBox(_T("Saving 5B instruments is not yet supported"));
}*/

void CInstrumentS5B::SaveFile(CFile *pFile, CFamiTrackerDoc *pDoc)
{
	//AfxMessageBox(_T("Saving 5B instruments is not yet supported"));
	unsigned char SeqCount = SEQUENCE_COUNT;
	pFile->Write(&SeqCount, sizeof(char));

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		int Sequence = GetSeqIndex(i);
		if (GetSeqEnable(i)) {
			CSequence *pSeq = pDoc->GetSequence(SNDCHIP_S5B, Sequence, i);

			char Enabled = 1;
			int ItemCount = pSeq->GetItemCount();
			int LoopPoint = pSeq->GetLoopPoint();
			int ReleasePoint = pSeq->GetReleasePoint();
			int Setting	= pSeq->GetSetting();
			
			pFile->Write(&Enabled, sizeof(char));
			pFile->Write(&ItemCount, sizeof(int));
			pFile->Write(&LoopPoint, sizeof(int));
			pFile->Write(&ReleasePoint, sizeof(int));
			pFile->Write(&Setting, sizeof(int));

			for (int j = 0; j < ItemCount; ++j) {
				signed char Value = pSeq->GetItem(j);
				pFile->Write(&Value, sizeof(char));
			}
		}
		else {
			char Enabled = 0;
			pFile->Write(&Enabled, sizeof(char));
		}
	}
}

/*bool CInstrumentS5B::LoadFile(CFile *pFile, int iVersion, CFamiTrackerDoc *pDoc)
{
	return false;
}*/

bool CInstrumentS5B::LoadFile(CFile *pFile, int iVersion, CFamiTrackerDoc *pDoc)
{
	// Sequences
	unsigned char SeqCount;
	unsigned char Enabled;
	int Count, Index;
	int LoopPoint, ReleasePoint, Setting;

	pFile->Read(&SeqCount, sizeof(char));

	// Loop through all instrument effects
	for (int i = 0; i < SeqCount; ++i) {
		pFile->Read(&Enabled, sizeof(char));
		if (Enabled == 1) {
			// Read the sequence

			pFile->Read(&Count, sizeof(int));
			Index = pDoc->GetFreeSequenceS5B(i);

			CSequence *pSeq = pDoc->GetSequence(SNDCHIP_S5B, Index, i);

			pSeq->SetItemCount(Count);
			pFile->Read(&LoopPoint, sizeof(int));
			pSeq->SetLoopPoint(LoopPoint);
			if (iVersion > 20) {
				pFile->Read(&ReleasePoint, sizeof(int));
				pSeq->SetReleasePoint(ReleasePoint);
			}
			if (iVersion >= 22) {
				pFile->Read(&Setting, sizeof(int));
				pSeq->SetSetting(Setting);
			}
			for (int j = 0; j < Count; ++j) {
				char Val;
				pFile->Read(&Val, sizeof(char));
				pSeq->SetItem(j, Val);
			}
			SetSeqEnable(i, true);
			SetSeqIndex(i, Index);
		}
		else {
			SetSeqEnable(i, false);
			SetSeqIndex(i, 0);
		}
	}

	return true;
}

int CInstrumentS5B::Compile(CChunk *pChunk, int Index)
{
	return 0;
}

bool CInstrumentS5B::CanRelease() const
{
	return false; // TODO
}

int	CInstrumentS5B::GetSeqEnable(int Index) const
{
	return m_iSeqEnable[Index];
}

int	CInstrumentS5B::GetSeqIndex(int Index) const
{
	return m_iSeqIndex[Index];
}

void CInstrumentS5B::SetSeqEnable(int Index, int Value)
{
	m_iSeqEnable[Index] = Value;
}

void CInstrumentS5B::SetSeqIndex(int Index, int Value)
{
	m_iSeqIndex[Index] = Value;
}
