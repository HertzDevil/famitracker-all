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
#include "Sequence.h"
#include "DocumentFile.h"

CSequence::CSequence()
{
	Clear();
}

void CSequence::Clear()
{
	m_iItemCount = 0;
	m_iLoopPoint = -1;
	m_iReleasePoint = -1;

	for (int i = 0; i < MAX_SEQUENCE_ITEMS; i++) {
		m_cValues[i] = 0;
	}

	m_iPlaying = -1;
	m_iSetting = 0;
}

signed char CSequence::GetItem(int Index) const
{
	ASSERT(Index < MAX_SEQUENCE_ITEMS);

	return m_cValues[Index];
}

unsigned int CSequence::GetItemCount() const
{
	return m_iItemCount;
}

void CSequence::SetItem(int Index, signed char Value)
{
	ASSERT(Index < MAX_SEQUENCE_ITEMS);

	m_cValues[Index] = Value;
}
/*
signed char CSequence::GetReleaseItem(int Index)
{
	ASSERT(Index < MAX_SEQUENCE_ITEMS);

	return m_cReleaseValues[Index];
}

void CSequence::SetReleaseItem(int Index, signed char Value) 
{
	ASSERT(Index < MAX_SEQUENCE_ITEMS);

	m_cReleaseValues[Index] = Value;
}
*/
unsigned int CSequence::GetLoopPoint()
{
	return m_iLoopPoint;
}

unsigned int CSequence::GetReleasePoint()
{
	return m_iReleasePoint;
}

void CSequence::SetItemCount(unsigned int Count)
{
	ASSERT(Count < MAX_SEQUENCE_ITEMS);

	m_iItemCount = Count;
}

void CSequence::SetLoopPoint(int Point)
{
	ASSERT(Point < MAX_SEQUENCE_ITEMS);

	m_iLoopPoint = Point;

	if (m_iLoopPoint >= m_iReleasePoint)
		m_iLoopPoint = -1;
}

void CSequence::SetReleasePoint(int Point)
{
	ASSERT(Point < MAX_SEQUENCE_ITEMS);

	m_iReleasePoint = Point;

	if (m_iLoopPoint >= m_iReleasePoint)
		m_iLoopPoint = -1;
}

void CSequence::SetPlayPos(int Position)
{
	m_iPlaying = Position;
}

int	CSequence::GetPlayPos()
{
	int Ret = m_iPlaying;
	m_iPlaying = -1;
	return Ret;
}

void CSequence::SetSetting(int Setting)
{
	m_iSetting = Setting;
}

int CSequence::GetSetting()
{
	return m_iSetting;
}

void CSequence::Copy(const CSequence *pSeq)
{
	// Copy all values from pSeq
	m_iItemCount = pSeq->m_iItemCount;
	m_iLoopPoint = pSeq->m_iLoopPoint;
	m_iReleasePoint = pSeq->m_iReleasePoint;
	m_iSetting = pSeq->m_iSetting;

	memcpy(m_cValues, pSeq->m_cValues, MAX_SEQUENCE_ITEMS);
}

void CSequence::Store(CDocumentFile *pDocFile, int Index, int Type)
{
	int Count = GetItemCount();

	if (Count > 0) {
		// Store index
		pDocFile->WriteBlockInt(Index);
		// Store type of sequence
		pDocFile->WriteBlockInt(Type);
		// Store number of items in this sequence
		pDocFile->WriteBlockChar(Count);
		// Store loop point
		pDocFile->WriteBlockInt(GetLoopPoint());
		// Store release point (v4)
		pDocFile->WriteBlockInt(GetReleasePoint());
		// Store setting (v4)
		pDocFile->WriteBlockInt(GetSetting());
		// Store items
		for (int j = 0; j < Count; j++) {
			pDocFile->WriteBlockChar(GetItem(j));
		}
	}
}
