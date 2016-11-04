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
#include "DocumentFile.h"

//
// This class is based on CFile and has some simple extensions to create and read FTM files
//

// No unicode allowed here

// Class constants
const unsigned int CDocumentFile::FILE_VER		 = 0x0420;			// Current file version (4.20)
const unsigned int CDocumentFile::COMPATIBLE_VER = 0x0100;			// Compatible file version (1.0)

const char *CDocumentFile::FILE_HEADER_ID = "FamiTracker Module";
const char *CDocumentFile::FILE_END_ID	  = "END";

const unsigned int CDocumentFile::MAX_BLOCK_SIZE = 0x80000;
const unsigned int CDocumentFile::BLOCK_SIZE = 0x10000;

// CDocumentFile

CDocumentFile::CDocumentFile() : 
	m_pBlockData(NULL),
	m_cBlockID(new char[16])
{
}

CDocumentFile::~CDocumentFile()
{
	SAFE_RELEASE_ARRAY(m_pBlockData);
	SAFE_RELEASE_ARRAY(m_cBlockID);
}

bool CDocumentFile::Finished() const
{
	return m_bFileDone;
}

void CDocumentFile::BeginDocument()
{
	Write(FILE_HEADER_ID, int(strlen(FILE_HEADER_ID)));
	Write(&FILE_VER, sizeof(int));
}

void CDocumentFile::EndDocument()
{
	Write(FILE_END_ID, int(strlen(FILE_END_ID)));
}

void CDocumentFile::CreateBlock(const char *ID, int Version)
{
	memset(m_cBlockID, 0, 16);
	strcpy(m_cBlockID, ID);

	m_iBlockPointer = 0;
	m_iBlockSize	= 0;
	m_iBlockVersion = Version & 0xFFFF;

	m_iMaxBlockSize = BLOCK_SIZE;

	m_pBlockData = new char[m_iMaxBlockSize];

	ASSERT(m_pBlockData != NULL);
}

void CDocumentFile::ReallocateBlock()
{
	int OldSize = m_iMaxBlockSize;
	m_iMaxBlockSize += BLOCK_SIZE;
	char *pData = new char[m_iMaxBlockSize];
	ASSERT(pData != NULL);
	memcpy(pData, m_pBlockData, OldSize);
	SAFE_RELEASE_ARRAY(m_pBlockData);
	m_pBlockData = pData;
}

void CDocumentFile::WriteBlock(const void *Data, unsigned int Size)
{
	ASSERT(m_pBlockData != NULL);

	unsigned int WriteSize, WritePtr = 0;
	char *pData = (char*)Data;

	// Allow block to grow in size
	while (Size > 0) {
		if (Size > BLOCK_SIZE)
			WriteSize = BLOCK_SIZE;
		else
			WriteSize = Size;

		if ((m_iBlockPointer + WriteSize) >= m_iMaxBlockSize)
			ReallocateBlock();

		memcpy(m_pBlockData + m_iBlockPointer, pData + WritePtr, WriteSize);
		m_iBlockPointer += WriteSize;
		Size -= WriteSize;
		WritePtr += WriteSize;
	}
}

void CDocumentFile::WriteBlockInt(int Value)
{
	WriteBlock(&Value, sizeof(int));
}

void CDocumentFile::WriteBlockChar(char Value)
{
	WriteBlock(&Value, sizeof(char));
}

void CDocumentFile::WriteString(CString String)
{
	int len = String.GetLength();

	for (int i = 0; i < len; ++i)
		WriteBlockChar(String.GetAt(i));

	// End of string
	WriteBlockChar(0);
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
	// Checks if loaded file is valid

	char Buffer[256];

	// Check ident string
	Read(Buffer, int(strlen(FILE_HEADER_ID)));

	if (memcmp(Buffer, FILE_HEADER_ID, strlen(FILE_HEADER_ID)) != 0)
		return FALSE;

	// Read file version
	Read(Buffer, 4);
	m_iFileVersion = (Buffer[3] << 24) | (Buffer[2] << 16) | (Buffer[1] << 8) | Buffer[0];

	m_bFileDone = false;

	return true;
}

unsigned int CDocumentFile::GetFileVersion() const
{
	return m_iFileVersion & 0xFFFF;
}

bool CDocumentFile::ReadBlock()
{
	int BytesRead;

	m_iBlockPointer = 0;
	
	memset(m_cBlockID, 0, 16);

	BytesRead = Read(m_cBlockID, 16);
	Read(&m_iBlockVersion, sizeof(int));
	Read(&m_iBlockSize, sizeof(int));

	if (m_iBlockSize > 1000000) {
		memset(m_cBlockID, 0, 16);
		return true;
	}

	SAFE_RELEASE_ARRAY(m_pBlockData);
	m_pBlockData = new char[m_iBlockSize];

	Read(m_pBlockData, m_iBlockSize);

	if (strcmp(m_cBlockID, FILE_END_ID) == 0)
		m_bFileDone = true;

	if (BytesRead == 0)
		m_bFileDone = true;

	return false;
}

char *CDocumentFile::GetBlockHeaderID() const
{
	return m_cBlockID;
}

int CDocumentFile::GetBlockVersion() const
{
	return m_iBlockVersion;
}

void CDocumentFile::RollbackPointer(int count)
{
	m_iBlockPointer -= count;
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
	char Value;
	memcpy(&Value, m_pBlockData + m_iBlockPointer, sizeof(char));
	m_iBlockPointer += sizeof(char);
	return Value;
}

CString CDocumentFile::ReadString()
{
	char str[1024], c;
	int str_ptr = 0;

	while (c = GetBlockChar())
		str[str_ptr++] = c;

	str[str_ptr++] = 0;

	return CString(str);
}

void CDocumentFile::GetBlock(void *Buffer, int Size)
{
	ASSERT(Size < MAX_BLOCK_SIZE);
	ASSERT(Buffer != NULL);

	memcpy(Buffer, m_pBlockData + m_iBlockPointer, Size);
	m_iBlockPointer += Size;
}

bool CDocumentFile::BlockDone() const
{
	return (m_iBlockPointer >= m_iBlockSize);
}

int CDocumentFile::GetBlockPos() const
{
	return m_iBlockPointer;
}

int CDocumentFile::GetBlockSize() const
{
	return m_iBlockSize;
}
