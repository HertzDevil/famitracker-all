/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2009  Jonathan Liss
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

// CDocumentFile

class CDocumentFile : public CFile
{
public:
	CDocumentFile();
	~CDocumentFile();

	bool	Finished();

	// Write functions
	void	BeginDocument();
	void	EndDocument();

	void	CreateBlock(const char *ID, int Version);
	void	WriteBlock(void *Data, int Size);
	void	WriteBlockInt(int Value);
	void	WriteBlockChar(char Value);
	void	WriteString(CString String);
	void	FlushBlock();

	// Read functions
	bool	CheckValidity();
	int		GetFileVersion();

	void	ReadBlock();
	char	*GetBlockHeaderID();
	int		GetBlockInt();
	char	GetBlockChar();
	void	GetBlock(void *Buffer, int Size);
	int		GetBlockVersion();
	bool	BlockDone();

	int		GetBlockPos();
	int		GetBlockSize();

	CString	ReadString();

private:
	unsigned int	m_iFileVersion;
	bool			m_bFileDone;

	char			m_cBlockID[16];
	unsigned int	m_iBlockSize;
	unsigned int	m_iBlockVersion;
	char			*m_pBlockData;

	unsigned int	m_iBlockPointer;	
};
