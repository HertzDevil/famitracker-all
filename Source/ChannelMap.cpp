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
#include "TrackerChannel.h"
#include "ChannelMap.h"

/*
 *  This class contains the expansion chip definitions & instruments.
 *
 */

CChannelMap::CChannelMap() :
	m_iAddedChips(0)
{
	SetupSoundChips();
}

CChannelMap::~CChannelMap()
{
	for (int i = 0; i < m_iAddedChips; ++i) {
		SAFE_RELEASE(m_pChipInst[i]);
	}
}

void CChannelMap::SetupSoundChips()
{
	// Add available chips
#ifdef _DEBUG
	// Under development
	AddChip(_T("Internal only (2A03/2A07)"), SNDCHIP_NONE, new CInstrument2A03());
	AddChip(_T("Konami VRC6"), SNDCHIP_VRC6, new CInstrumentVRC6());
	AddChip(_T("Konami VRC7"), SNDCHIP_VRC7, new CInstrumentVRC7());
	AddChip(_T("Nintendo FDS sound"), SNDCHIP_FDS, new CInstrumentFDS());
	AddChip(_T("Nintendo MMC5"), SNDCHIP_MMC5, new CInstrument2A03());
	AddChip(_T("Namco N106"), SNDCHIP_N106, new CInstrumentN106());
	AddChip(_T("Sunsoft 5B"), SNDCHIP_5B, new CInstrument5B());
#else
	// Ready for use
	AddChip(_T("Internal only (2A03/2A07)"), SNDCHIP_NONE, new CInstrument2A03());
	AddChip(_T("Konami VRC6"), SNDCHIP_VRC6, new CInstrumentVRC6());
	AddChip(_T("Konami VRC7"), SNDCHIP_VRC7, new CInstrumentVRC7());
	AddChip(_T("Nintendo FDS sound"), SNDCHIP_FDS, new CInstrumentFDS());
	AddChip(_T("Nintendo MMC5"), SNDCHIP_MMC5, new CInstrument2A03());
#endif
}

void CChannelMap::AddChip(LPCTSTR pName, int Ident, CInstrument *pInst)
{
	ASSERT(m_iAddedChips < CHIP_COUNT);

	m_pChipNames[m_iAddedChips] = pName;
	m_iChipIdents[m_iAddedChips] = Ident;
	m_pChipInst[m_iAddedChips] = pInst;
	m_iAddedChips++;
}

int CChannelMap::GetChipCount() const
{
	return m_iAddedChips;
}

LPCTSTR CChannelMap::GetChipName(int Index) const
{
	return m_pChipNames[Index];
}

int CChannelMap::GetChipIdent(int Index) const
{
	return m_iChipIdents[Index];
}

int	CChannelMap::GetChipIndex(int Ident) const
{
	for (int i = 0; i < m_iAddedChips; i++) {
		if (Ident == m_iChipIdents[i])
			return i;
	}
	return 0;
}

CInstrument* CChannelMap::GetChipInstrument(int Chip) const
{
	int Index = GetChipIndex(Chip);

	if (m_pChipInst[Index] == NULL)
		return NULL;

	return m_pChipInst[Index]->CreateNew();
}
/*
int CChannelMap::GetChannelType(int Channel) const
{
	ASSERT(m_iRegisteredChannels != 0);
	return m_iChannelTypes[Channel];
}

int CChannelMap::GetChipType(int Channel) const
{
	ASSERT(m_iRegisteredChannels != 0);
	ASSERT(Channel < m_iRegisteredChannels);
	return m_pChannels[Channel]->GetChip();
}

void CChannelMap::ResetChannels()
{
	// Clears all channels from the document
	m_iRegisteredChannels = 0;
}

void CChannelMap::RegisterChannel(CTrackerChannel *pChannel, int ChannelType, int ChipType)
{
	// Adds a channel to the document
	m_pChannels[m_iRegisteredChannels] = pChannel;
	m_iChannelTypes[m_iRegisteredChannels] = ChannelType;
	m_iChannelChip[m_iRegisteredChannels] = ChipType;
	m_iRegisteredChannels++;
}

CTrackerChannel *CChannelMap::GetChannel(int Index) const
{
	ASSERT(m_iRegisteredChannels != 0);
	ASSERT(m_pChannels[Index] != NULL);
	return m_pChannels[Index];
}
*/