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

#include "CustomExporterInterfaces.h"

// Instrument types
enum {
	INST_NONE = 0,
	INST_2A03 = 1,
	INST_VRC6,
	INST_VRC7,
	INST_FDS,
	INST_N106,
	INST_S5B
};

// External classes
class CCompiler;
class CDocumentFile;
class CSequence;
class CFamiTrackerDoc;

// Instrument base class
class CInstrument {
public:
	CInstrument();
	virtual ~CInstrument();
	void SetName(const char *Name);
	void GetName(char *Name) const;
	char* GetName();
public:
	virtual int GetType() const = 0;												// Returns instrument type
	virtual CInstrument* CreateNew() = 0;											// Creates a new object
	virtual CInstrument* Clone() = 0;												// Creates a copy
	virtual void Store(CDocumentFile *pDocFile) = 0;								// Saves the instrument to the module
	virtual bool Load(CDocumentFile *pDocFile) = 0;									// Loads the instrument from a module
	virtual void SaveFile(CFile *pFile, CFamiTrackerDoc *pDoc) = 0;					// Saves to an FTI file
	virtual bool LoadFile(CFile *pFile, int iVersion, CFamiTrackerDoc *pDoc) = 0;	// Loads from an FTI file
	virtual int CompileSize(CCompiler *pCompiler) = 0;								// Gets the compiled size
	virtual int Compile(CCompiler *pCompiler, int Index) = 0;						// Compiles the instrument for NSF generation
	virtual bool CanRelease() const = 0;
private:
	char m_cName[128];
	int	 m_iType;
};

class CInstrument2A03 : public CInstrument, public CInstrument2A03Interface {
public:
	CInstrument2A03();
	virtual int	GetType() const { return INST_2A03; };
	virtual CInstrument* CreateNew() { return new CInstrument2A03(); };
	virtual CInstrument* Clone();
	virtual void Store(CDocumentFile *pFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CFile *pFile, CFamiTrackerDoc *pDoc);
	virtual bool LoadFile(CFile *pFile, int iVersion, CFamiTrackerDoc *pDoc);
	virtual int CompileSize(CCompiler *pCompiler);
	virtual int Compile(CCompiler *pCompiler, int Index);
	virtual bool CanRelease() const;

public:
	int		GetSeqEnable(int Index) const;
	int		GetSeqIndex(int Index) const;
	void	SetSeqIndex(int Index, int Value);
	void	SetSeqEnable(int Index, int Value);

	char	GetSample(int Octave, int Note) const;
	char	GetSamplePitch(int Octave, int Note) const;
	bool	GetSampleLoop(int Octave, int Note) const;
	char	GetSampleLoopOffset(int Octave, int Note) const;
	void	SetSample(int Octave, int Note, char Sample);
	void	SetSamplePitch(int Octave, int Note, char Pitch);
	void	SetSampleLoop(int Octave, int Note, bool Loop);
	void	SetSampleLoopOffset(int Octave, int Note, char Offset);

	void	SetPitchOption(int Option);
	int		GetPitchOption() const;

	bool	AssignedSamples() const;

public:
	static const int SEQUENCE_COUNT = 5;
	static const int SEQUENCE_TYPES[];

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
	virtual int	GetType() const { return INST_VRC6; };
	virtual CInstrument* CreateNew() { return new CInstrumentVRC6(); };
	virtual CInstrument* Clone();
	virtual void Store(CDocumentFile *pDocFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CFile *pFile, CFamiTrackerDoc *pDoc);
	virtual bool LoadFile(CFile *pFile, int iVersion, CFamiTrackerDoc *pDoc);
	virtual int CompileSize(CCompiler *pCompiler);
	virtual int Compile(CCompiler *pCompiler, int Index);
	virtual bool CanRelease() const;
public:
	int		GetSeqEnable(int Index) const;
	int		GetSeqIndex(int Index) const;
	void	SetSeqEnable(int Index, int Value);
	void	SetSeqIndex(int Index, int Value);
public:
	static const int SEQUENCE_COUNT = 5;
	static const int SEQUENCE_TYPES[];
private:
	int		m_iSeqEnable[SEQ_COUNT];
	int		m_iSeqIndex[SEQ_COUNT];
};

class CInstrumentVRC7 : public CInstrument {
public:
	CInstrumentVRC7();
	virtual int	GetType() const { return INST_VRC7; };
	virtual CInstrument* CreateNew() { return new CInstrumentVRC7(); };
	virtual CInstrument* Clone();
	virtual void Store(CDocumentFile *pDocFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CFile *pFile, CFamiTrackerDoc *pDoc);
	virtual bool LoadFile(CFile *pFile, int iVersion, CFamiTrackerDoc *pDoc);
	virtual int CompileSize(CCompiler *pCompiler);
	virtual int Compile(CCompiler *pCompiler, int Index);
	virtual bool CanRelease() const;
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
	virtual ~CInstrumentFDS();
	virtual int GetType() const { return INST_FDS; };
	virtual CInstrument* CreateNew() { return new CInstrumentFDS(); };
	virtual CInstrument* Clone();
	virtual void Store(CDocumentFile *pDocFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CFile *pFile, CFamiTrackerDoc *pDoc);
	virtual bool LoadFile(CFile *pFile, int iVersion, CFamiTrackerDoc *pDoc);
	virtual int CompileSize(CCompiler *pCompiler);
	virtual int Compile(CCompiler *pCompiler, int Index);
	virtual bool CanRelease() const;
public:
	unsigned char GetSample(int Index) const;
	void	SetSample(int Index, int Sample);
	int		GetModulationSpeed() const;
	void	SetModulationSpeed(int Speed);
	int		GetModulation(int Index) const;
	void	SetModulation(int Index, int Value);
	int		GetModulationDepth() const;
	void	SetModulationDepth(int Depth);
	int		GetModulationDelay() const;
	void	SetModulationDelay(int Delay);
	bool	GetModulationEnable() const;
	void	SetModulationEnable(bool Enable);
	bool	HasChanged();
	CSequence* GetVolumeSeq() const;
	CSequence* GetArpSeq() const;
	CSequence* GetPitchSeq() const;
private:
	void StoreSequence(CDocumentFile *pDocFile, CSequence *pSeq);
	bool LoadSequence(CDocumentFile *pDocFile, CSequence *pSeq);
	void StoreInstSequence(CFile *pDocFile, CSequence *pSeq);
	bool LoadInstSequence(CFile *pFile, CSequence *pSeq);
public:
	static const int WAVE_SIZE = 64;
	static const int MOD_SIZE = 32;
private:
	// Used to update instrument when changed in instrument editor
	bool m_bChanged;

	// Instrument data
	unsigned char m_iSamples[64];
	unsigned char m_iModulation[32];
	int			  m_iModulationSpeed;
	int			  m_iModulationDepth;
	int			  m_iModulationDelay;
	bool		  m_bModulationEnable;

	CSequence*	  m_pVolume;
	CSequence*	  m_pArpeggio;
	CSequence*	  m_pPitch;
};

class CInstrumentN106 : public CInstrument {
public:
	CInstrumentN106();
	virtual int GetType() const { return INST_N106; };
	virtual CInstrument* CreateNew() { return new CInstrumentN106(); };
	virtual CInstrument* Clone();
	virtual void Store(CDocumentFile *pDocFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CFile *pFile, CFamiTrackerDoc *pDoc);
	virtual bool LoadFile(CFile *pFile, int iVersion, CFamiTrackerDoc *pDoc);
	virtual int CompileSize(CCompiler *pCompiler);
	virtual int Compile(CCompiler *pCompiler, int Index);
	virtual bool CanRelease() const;
public:	
	int		GetSeqEnable(int Index) const;
	int		GetSeqIndex(int Index) const;
	void	SetSeqEnable(int Index, int Value);
	void	SetSeqIndex(int Index, int Value);
	int		GetWaveSize() const;
	void	SetWaveSize(int size);
	int		GetWavePos() const;
	void	SetWavePos(int pos);
	int		GetSample(int index) const;
	void	SetSample(int index, int sample);
public:
	static const int SEQUENCE_COUNT = 5;
	static const int SEQUENCE_TYPES[];
	static const int MAX_N106_WAVE = 32;
private:
	int		m_iSeqEnable[SEQ_COUNT];
	int		m_iSeqIndex[SEQ_COUNT];
	int		m_iSamples[MAX_N106_WAVE];
	int		m_iSampleLen;
	int		m_iSamplePos;
	// TODO
};

class CInstrumentS5B : public CInstrument {
public:
	CInstrumentS5B();
	virtual int GetType() const { return INST_S5B; };
	virtual CInstrument* CreateNew() { return new CInstrumentS5B(); };
	virtual CInstrument* Clone();
	virtual void Store(CDocumentFile *pDocFile);
	virtual bool Load(CDocumentFile *pDocFile);
	virtual void SaveFile(CFile *pFile, CFamiTrackerDoc *pDoc);
	virtual bool LoadFile(CFile *pFile, int iVersion, CFamiTrackerDoc *pDoc);
	virtual int CompileSize(CCompiler *pCompiler);
	virtual int Compile(CCompiler *pCompiler, int Index);
	virtual bool CanRelease() const;
public:
	int		GetSeqEnable(int Index) const;
	int		GetSeqIndex(int Index) const;
	void	SetSeqEnable(int Index, int Value);
	void	SetSeqIndex(int Index, int Value);
public:
	static const int SEQUENCE_COUNT = 5;
	static const int SEQUENCE_TYPES[];
private:
	int		m_iSeqEnable[SEQ_COUNT];
	int		m_iSeqIndex[SEQ_COUNT];
};

