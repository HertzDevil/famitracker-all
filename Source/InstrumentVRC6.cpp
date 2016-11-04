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

/*
 * class CInstrumentVRC6
 *
 */

CInstrumentVRC6::CInstrumentVRC6()
{
	for (int i = 0; i < SEQ_COUNT; i++) {
		m_iSeqEnable[i] = 0;
		m_iSeqIndex[i] = 0;
	}	
}

CInstrument *CInstrumentVRC6::Clone()
{
	CInstrumentVRC6 *pNew = new CInstrumentVRC6();

	for (int i = 0; i < SEQ_COUNT; i++) {
		pNew->SetSeqEnable(i, GetSeqEnable(i));
		pNew->SetSeqIndex(i, GetSeqIndex(i));
	}

	return pNew;
}

void CInstrumentVRC6::Store(CDocumentFile *pDocFile)
{
	pDocFile->WriteBlockInt(SEQ_COUNT);

	for (int i = 0; i < SEQ_COUNT; i++) {
		pDocFile->WriteBlockChar(GetSeqEnable(i));
		pDocFile->WriteBlockChar(GetSeqIndex(i));
	}
}

bool CInstrumentVRC6::Load(CDocumentFile *pDocFile)
{
	int i, Index;
	int SeqCnt = pDocFile->GetBlockInt();

	ASSERT_FILE_DATA(SeqCnt < (SEQ_COUNT + 1));

	SeqCnt = SEQ_COUNT;

	for (i = 0; i < SeqCnt; i++) {
		SetSeqEnable(i, pDocFile->GetBlockChar());
		Index = pDocFile->GetBlockChar();
		ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
		SetSeqIndex(i, Index);
	}

	return true;
}

void CInstrumentVRC6::SaveFile(CFile *pFile)
{
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
}

bool CInstrumentVRC6::LoadFile(CFile *pFile, int iVersion)
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.GetFirstDocument();

	// Sequences
	unsigned char SeqCount;
	pFile->Read(&SeqCount, sizeof(char));

	stSequence OldSequence;

	// Loop through all instrument effects
	for (int l = 0; l < SeqCount; l++) {
		unsigned char Enabled;
		pFile->Read(&Enabled, sizeof(char));
		if (Enabled == 1) {
			// Read the sequence
			int Count;
			pFile->Read(&Count, sizeof(int));
			int Index;
			Index = pDoc->GetFreeSequenceVRC6(l);
			CSequence *pSeq = pDoc->GetSequenceVRC6(Index, l);
//			for (int j = 0; j < MAX_SEQUENCES; j++) {
				// Find a free sequence
//				if (m_SequencesVRC6[j][l].GetItemCount() == 0) {
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
				pFile->Read(&LoopPoint, sizeof(int));
				pSeq->SetLoopPoint(LoopPoint);
				if (iVersion > 20) {
					int ReleasePoint;
					pFile->Read(&ReleasePoint, sizeof(int));
					pSeq->SetReleasePoint(ReleasePoint);
				}
				if (iVersion >= 22) {
					int Setting;
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
//			}
		}
		else {
			SetSeqEnable(l, false);
			SetSeqIndex(l, 0);
		}
	}

	return true;
}

int CInstrumentVRC6::CompileSize(CCompiler *pCompiler)
{
	int Size = 1;
	CFamiTrackerDoc *pDoc = pCompiler->GetDocument();

	for (int i = 0; i < SEQ_COUNT; ++i)
		if (GetSeqEnable(i) && pDoc->GetSequenceVRC6(GetSeqIndex(i), i)->GetItemCount() > 0)
			Size += 2;

	return Size;
}

int CInstrumentVRC6::Compile(CCompiler *pCompiler, int Index)
{
	int ModSwitch;
	int StoredBytes = 0;
	int iAddress;
	int i;

	pCompiler->WriteLog("VRC6 {");
	ModSwitch = 0;

	for (i = 0; i < SEQ_COUNT; i++) {
		ModSwitch = (ModSwitch >> 1) | (GetSeqEnable(i) ? 0x10 : 0);
	}

	pCompiler->StoreByte(ModSwitch);
	pCompiler->WriteLog("%02X ", ModSwitch);
	StoredBytes++;

	for (i = 0; i < SEQ_COUNT; i++) {
		iAddress = (GetSeqEnable(i) == 0) ? 0 : pCompiler->GetSequenceAddressVRC6(GetSeqIndex(i), i);
		if (iAddress > 0) {
			pCompiler->StoreShort(iAddress);
			pCompiler->WriteLog("%04X ", iAddress);
			StoredBytes += 2;
		}
	}
	
	return StoredBytes;
}

int	CInstrumentVRC6::GetSeqEnable(int Index)
{
	return m_iSeqEnable[Index];
}

int	CInstrumentVRC6::GetSeqIndex(int Index)
{
	return m_iSeqIndex[Index];
}

void CInstrumentVRC6::SetSeqEnable(int Index, int Value)
{
	m_iSeqEnable[Index] = Value;
}

void CInstrumentVRC6::SetSeqIndex(int Index, int Value)
{
	m_iSeqIndex[Index] = Value;
}
