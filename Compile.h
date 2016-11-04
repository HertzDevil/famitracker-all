/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005  Jonathan Liss
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

#include "driver.h"
#include "FamiTrackerDoc.h"

const char SYM_STORE_BYTE[]		= ".byte";	// .db
const char SYM_STORE_WORD[]		= ".word";	// .dw
const char SYM_ORIGIN[]			= ".org";
const char SYM_INCLUDE_BIN[]	= ".incbin";
const char SYM_SEGMENT[]		= ".segment";

struct stLabel
{
	int Address;
	char Name[256];
	bool Used;
};

// CCompile command target

class CCompile : public CObject
{
public:
	CCompile();
	virtual ~CCompile();

	char	*CompileNSF(CFamiTrackerDoc *pDoc, CString FileName, bool KeepSymbols);

	void	NSF_WritePattern(CFamiTrackerDoc *pDoc, int Pattern, int Channel, int Item);
	void	NSF_WriteSequence(stSequence *pSequence);
	void	NSF_WriteInstruments(stInstrument *pInst);
	void	NSF_WriteDPCMInst(int Index, stInstrument *pInstrument);
	void	NSF_WriteDPCM(stDSample *pDSample);

	bool	CreateSymbolFile(char *Filename);
	void	WriteSymbol(char *Text, ...);
	void	WriteSymbolRaw(char *Data, int Size);
	void	CloseSymbolFile();

	void	SetSongInfo(char *Name, char *Artist, char *Copyright, int Speed, int Machine);
	bool	ApplyDriver(char *BinFile, CString OutFile);

	bool	CompileSymbolFile(char *InFile, char *OutFile);

	int		GetMusicSize();

	bool	ErrorFlag;

private:
	int		AddedSamples;
	int		SampleOrgPos;
	int		SelectedInstrument;

	char	DPCM_LookUp[MAX_INSTRUMENTS][8][12];


private:
	CFile	File;

	int		ValueSize;
	bool	FillAdresses;
	char	*CodeBank;

	stLabel Labels[1024];
	int		LabelStack;
	int		Origin;

	int		GetWord(char *Word);
	int		GetValidValue(char *param);
	void	AbortError(char *Text);
	void	CheckStatement(char *Word);
	void	IncludeBinary(char *Filename);
	void	WriteBank(char c);

	int		SongSize;

	stNSFHeader Header;

};


