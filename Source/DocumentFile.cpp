/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2006  Jonathan Liss
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
#include "DocumentFile.h"

static const char			*FILE_HEADER	= "FamiTracker Module";
static const char			*FILE_ENDER		= "END";
static const int			FILE_VER		= 0x0203;				// Current file version (2.00)
static const int			COMPATIBLE_VER	= 0x0100;				// Compatible file version (1.0)

static const unsigned int	MAX_BLOCK_SIZE	= 0x80000;				// 256 kB for now (should be enough)

// CDocumentFile

CDocumentFile::CDocumentFile()
{
	m_pBlockData = NULL;
}

CDocumentFile::~CDocumentFile()
{
	if (m_pBlockData) {
		delete [] m_pBlockData;
		m_pBlockData = NULL;
	}
}

bool CDocumentFile::Finished()
{
	return m_bFileDone;
}

void CDocumentFile::BeginDocument()
{
	Write(FILE_HEADER,	int(strlen(FILE_HEADER)));
	Write(&FILE_VER,	sizeof(int));
}

void CDocumentFile::EndDocument()
{
	Write(FILE_ENDER, int(strlen(FILE_ENDER)));
}

void CDocumentFile::CreateBlock(const char *ID, int Version)
{
	memset(m_cBlockID, 0, 16);
	strcpy(m_cBlockID, ID);

	m_iBlockPointer = 0;
	m_iBlockSize	= 0;
	m_iBlockVersion = Version & 0xFFFF;

	m_pBlockData = new char[MAX_BLOCK_SIZE];
	memset(m_pBlockData, 0, MAX_BLOCK_SIZE);
}

void CDocumentFile::WriteBlock(void *Data, int Size)
{
	if (!m_pBlockData || m_iBlockPointer >= MAX_BLOCK_SIZE)
		return;

	memcpy(m_pBlockData + m_iBlockPointer, Data, Size);
	m_iBlockPointer += Size;
}

void CDocumentFile::WriteBlockInt(int Value)
{
	if (!m_pBlockData || m_iBlockPointer >= MAX_BLOCK_SIZE)
		return;

	memcpy(m_pBlockData + m_iBlockPointer, &Value, sizeof(int));
	m_iBlockPointer += sizeof(int);
}

void CDocumentFile::WriteBlockChar(char Value)
{
	if (!m_pBlockData || m_iBlockPointer >= MAX_BLOCK_SIZE)
		return;

	memcpy(m_pBlockData + m_iBlockPointer, &Value, sizeof(char));
	m_iBlockPointer += sizeof(char);
}

void CDocumentFile::FlushBlock()
{
	if (!m_pBlockData)
		return;

	Write(m_cBlockID, 16);
	Write(&m_iBlockVersion, sizeof(int));
	Write(&m_iBlockPointer, sizeof(int));
	Write(m_pBlockData, m_iBlockPointer);

	delete [] m_pBlockData;
	m_pBlockData = NULL;
}

bool CDocumentFile::CheckValidity()
{
	char Buffer[256];

	Read(Buffer, int(strlen(FILE_HEADER)));

	if (memcmp(Buffer, FILE_HEADER, strlen(FILE_HEADER)) != 0)
		return FALSE;

	Read(&m_iFileVersion, sizeof(int));

	m_bFileDone = false;

	return true;
}

int CDocumentFile::GetFileVersion()
{
	return m_iFileVersion & 0xFFFF;
}

void CDocumentFile::ReadBlock()
{
	int BytesRead;

	m_iBlockPointer = 0;
	
	memset(m_cBlockID, 0, 16);

	BytesRead = Read(m_cBlockID, 16);
	Read(&m_iBlockVersion, sizeof(int));
	Read(&m_iBlockSize, sizeof(int));

	if (m_pBlockData) {
		delete [] m_pBlockData;
		m_pBlockData = NULL;
	}

	m_pBlockData = new char[m_iBlockSize];

	Read(m_pBlockData, m_iBlockSize);

	if (strcmp(m_cBlockID, FILE_ENDER) == 0)
		m_bFileDone = true;

	if (BytesRead == 0)
		m_bFileDone = true;
}

char *CDocumentFile::GetBlockHeaderID()
{
	return m_cBlockID;
}

int CDocumentFile::GetBlockVersion()
{
	return m_iBlockVersion;
}

int CDocumentFile::GetBlockInt()
{
	int Value;

	memcpy(&Value, m_pBlockData + m_iBlockPointer, sizeof(int));
	m_iBlockPointer += sizeof(int);

	return Value;
}

char CDocumentFile::GetBlockChar()
{
	int Value;

	memcpy(&Value, m_pBlockData + m_iBlockPointer, sizeof(char));
	m_iBlockPointer += sizeof(char);

	return Value;
}

void CDocumentFile::GetBlock(void *Buffer, int Size)
{
	ASSERT(Size > 0 && Size < MAX_BLOCK_SIZE);
	ASSERT(Buffer != NULL);

	memcpy(Buffer, m_pBlockData + m_iBlockPointer, Size);
	m_iBlockPointer += Size;
}

bool CDocumentFile::BlockDone()
{
	return (m_iBlockPointer >= m_iBlockSize);
}