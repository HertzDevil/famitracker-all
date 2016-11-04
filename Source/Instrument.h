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

#include "DocumentFile.h"
#include "Compiler.h"

class CInstrument {
public:
	CInstrument();
	void SetName(char *Name);
	void GetName(char *Name) const;
	char* GetName();
public:
	virtual int GetType() = 0;									// Returns instrument type
	virtual CInstrument* CreateNew() = 0;						// Creates a new object
	virtual CInstrument* Clone() = 0;							// Creates a copy
	virtual void Store(CDocumentFile *pDocFile) = 0;			// Saves the instrument to the module
	virtual bool Load(CDocumentFile *pDocFile) = 0;				// Loads the instrument from a module
	virtual void SaveFile(CFile *pFile) = 0;					// Saves to an FTI file
	virtual bool LoadFile(CFile *pFile, int iVersion) = 0;		// Loads from an FTI file
	virtual int CompileSize(CCompiler *pCompiler) = 0;			// Gets the compiled size
	virtual int Compile(CCompiler *pCompiler, int Index) = 0;	// Compiles the instrument for NSF generation
private:
	char m_cName[128];
	int	 m_iType;
};

class CInstrument2A03Interface
{
public:
	virtual int GetSeqEnable(int Index) const = 0;
	virtual int GetSeqIndex(int Index) const = 0;
};

class CInstrument2A03 : public CInstrument, public CInstrument2A03Interface {
public:
	CInstrument2A03();
	virtual int	GetType() { return INST_2A03; };
	virtual CInstrument* CreateNew() { return new CInstrument2A03(); };
	virtual CInstrument* Clone();
	virtual void Store(CDocumentFile *pFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CFile *pFile);
	virtual bool LoadFile(CFile *pFile, int iVersion);
	virtual int CompileSize(CCompiler *pCompiler);
	virtual int Compile(CCompiler *pCompiler, int Index);

public:
	int		GetSeqEnable(int Index) const;
	int		GetSeqIndex(int Index) const;
	void	SetSeqIndex(int Index, int Value);
	void	SetSeqEnable(int Index, int Value);

	char	GetSample(int Octave, int Note) const;
	char	GetSamplePitch(int Octave, int Note) const;
	char	GetSampleLoopOffset(int Octave, int Note) const;
	void	SetSample(int Octave, int Note, char Sample);
	void	SetSamplePitch(int Octave, int Note, char Pitch);
	void	SetSampleLoopOffset(int Octave, int Note, char Offset);

	void	SetPitchOption(int Option);
	int		GetPitchOption() const;

	bool	AssignedSamples() const;

private:
	int		m_iSeqEnable[SEQ_COUNT];
	int		m_iSeqIndex[SEQ_COUNT];
	char	m_cSamples[OCTAVE_RANGE][12];				// Samples
	char	m_cSamplePitch[OCTAVE_RANGE][12];			// Play pitch/loop
	char	m_cSampleLoopOffset[OCTAVE_RANGE][12];		// Loop offset

	int		m_iPitchOption;
};

class CInstrumentVRC6 : public CInstrument {
public:
	CInstrumentVRC6();
	virtual int	GetType() { return INST_VRC6; };
	virtual CInstrument* CreateNew() { return new CInstrumentVRC6(); };
	virtual CInstrument* Clone();
	virtual void Store(CDocumentFile *pDocFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CFile *pFile);
	virtual bool LoadFile(CFile *pFile, int iVersion);
	virtual int CompileSize(CCompiler *pCompiler);
	virtual int Compile(CCompiler *pCompiler, int Index);
public:
	int		GetSeqEnable(int Index);
	int		GetSeqIndex(int Index);
	void	SetSeqEnable(int Index, int Value);
	void	SetSeqIndex(int Index, int Value);
private:
	int		m_iSeqEnable[SEQ_COUNT];
	int		m_iSeqIndex[SEQ_COUNT];
};

class CInstrumentVRC7 : public CInstrument {
public:
	CInstrumentVRC7();
	virtual int	GetType() { return INST_VRC7; };
	virtual CInstrument* CreateNew() { return new CInstrumentVRC7(); };
	virtual CInstrument* Clone();
	virtual void Store(CDocumentFile *pDocFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CFile *pFile);
	virtual bool LoadFile(CFile *pFile, int iVersion);
	virtual int CompileSize(CCompiler *pCompiler);
	virtual int Compile(CCompiler *pCompiler, int Index);
public:
	void		 SetPatch(unsigned int Patch);
	unsigned int GetPatch() const;
	void		 SetCustomReg(int Reg, unsigned int Value);
	unsigned int GetCustomReg(int Reg) const;
private:
	unsigned int m_iPatch;
	unsigned int m_iRegs[8];		// Custom patch settings
};

class CInstrumentFDS : public CInstrument {
public:
	CInstrumentFDS();
	~CInstrumentFDS();
	virtual int GetType() { return INST_FDS; };
	virtual CInstrument* CreateNew() { return new CInstrumentFDS(); };
	virtual CInstrument* Clone();
	virtual void Store(CDocumentFile *pDocFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CFile *pFile);
	virtual bool LoadFile(CFile *pFile, int iVersion);
	virtual int CompileSize(CCompiler *pCompiler);
	virtual int Compile(CCompiler *pCompiler, int Index);
public:
	unsigned char GetSample(int Index) const;
	void	SetSample(int Index, int Sample);
	int		GetModulationFreq() const;
	void	SetModulationFreq(int Freq);
	int		GetModulation(int Index) const;
	void	SetModulation(int Index, int Value);
	int		GetModulationDepth() const;
	void	SetModulationDepth(int Depth);
	int		GetModulationDelay() const;
	void	SetModulationDelay(int Delay);
	bool	HasChanged();
	CSequence* GetVolumeSeq() const;
	CSequence* GetArpSeq() const;
	CSequence* GetPitchSeq() const;
private:
	void StoreSequence(CDocumentFile *pDocFile, CSequence *pSeq);
	bool LoadSequence(CDocumentFile *pDocFile, CSequence *pSeq);
	void StoreInstSequence(CFile *pDocFile, CSequence *pSeq);
	bool LoadInstSequence(CFile *pFile, CSequence *pSeq);
private:
	bool m_bChanged;

	// Instrument data
	unsigned char	m_iSamples[64];
	unsigned char	m_iModulation[32];
	int				m_iModulationFreq;
	int				m_iModulationDepth;
	int				m_iModulationDelay;

	CSequence *m_pVolume;
	CSequence *m_pArpeggio;
	CSequence *m_pPitch;
};

class CInstrumentN106 : public CInstrument {
public:
	CInstrumentN106();
	virtual int GetType() { return INST_N106; };
	virtual CInstrument* CreateNew() { return new CInstrumentN106(); };
	virtual CInstrument* Clone();
	virtual void Store(CDocumentFile *pDocFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CFile *pFile);
	virtual bool LoadFile(CFile *pFile, int iVersion);
	virtual int CompileSize(CCompiler *pCompiler);
	virtual int Compile(CCompiler *pCompiler, int Index);
public:
private:
	// Todo
};

class CInstrument5B : public CInstrument {
public:
	CInstrument5B();
	virtual int GetType() { return INST_5B; };
	virtual CInstrument* CreateNew() { return new CInstrument5B(); };
	virtual CInstrument* Clone();
	virtual void Store(CDocumentFile *pDocFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CFile *pFile);
	virtual bool LoadFile(CFile *pFile, int iVersion);
	virtual int CompileSize(CCompiler *pCompiler);
	virtual int Compile(CCompiler *pCompiler, int Index);
public:
private:
	// Todo
};
