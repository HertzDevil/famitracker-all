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

#pragma once

const int MAX_SEQUENCE_ITEMS = 254;

/*
** This class is a pure virtual interface to CSequence, which can be used by custom exporters
*/
class CSequenceInterface
{
public:
	virtual signed char		GetItem(int Index) const = 0;
	virtual unsigned int	GetItemCount() const = 0;
};

class CDocumentFile;

/*
** This class is used to store instrument sequences
*/
class CSequence: public CSequenceInterface {
public:
	CSequence();
	void			Clear();
	signed char		GetItem(int Index) const;
	unsigned int	GetItemCount() const;
	unsigned int	GetLoopPoint();
	unsigned int	GetReleasePoint();
	void			SetItem(int Index, signed char Value);
	void			SetItemCount(unsigned int Count);
	void			SetLoopPoint(int Point);
	void			SetReleasePoint(int Point);

	void			Store(CDocumentFile *pDocFile, int Index, int Type);

	// Used by instrument editor
	void			SetPlayPos(int pos);
	int				GetPlayPos();

	void			SetSetting(int Setting);
	int				GetSetting();

	void			Copy(const CSequence *pSeq);

private:
	unsigned int	m_iItemCount;
	unsigned int	m_iLoopPoint;
	unsigned int	m_iReleasePoint;
//	unsigned int	m_iItemCountRelease;
	signed	 char	m_cValues[MAX_SEQUENCE_ITEMS];
	int				m_iSetting;
	int				m_iPlaying;
};
