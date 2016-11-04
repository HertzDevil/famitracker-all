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

#ifndef _APU_H_
#define _APU_H_

#include "../common.h"

const uint8 SNDCHIP_NONE  = 0;
const uint8 SNDCHIP_VRC6  = 1;			// Konami VRCVI
const uint8 SNDCHIP_VRC7  = 2;			// Konami VRCVII
const uint8 SNDCHIP_FDS	  = 4;			// Famicom Disk Sound
const uint8 SNDCHIP_MMC5  = 8;			// Nintendo MMC5
const uint8 SNDCHIP_N106  = 16;			// Namco N-106
const uint8 SNDCHIP_5B	  = 32;			// Sunsoft 5B

enum {MACHINE_NTSC, MACHINE_PAL};

#include "../SoundInterface.h"

// APU stuff
#include "mixer.h"
#include "vrc6.h"
#include "mmc5.h"
#include "fds.h"
#include "n106.h"
#include "vrc7.h"

class CSquare;
class CTriangle;
class CNoise;
class CDPCM;

//class CEmulator;
class CCPU;

class CAPU {
public:
	CAPU(ICallback *pCallback, CSampleMem *pSampleMem);
	~CAPU();

	void	Shutdown();
	void	Halt();
	void	Reset();
	void	Process();
	void	EndFrame();
	void	SetNextTime(uint32 Cycles);

	uint8	Read4015();
	void	Write4017(uint8 Value);
	void	Write4015(uint8 Value);
	void	Write(uint16 Address, uint8 Value);

	void	SetExternalSound(uint8 Chip);
	void	ExternalWrite(uint16 Address, uint8 Value);
	uint8	ExternalRead(uint16 Address);
	
	void	ChangeMachine(int Machine);
	void	SetSoundInterface(ISoundInterface *pSoundInterface);
	bool	SetupSound(int SampleRate, int NrChannels, int Speed);
	void	SetupMixer(int LowCut, int HighCut, int HighDamp, int Volume) const;

	int32	GetVol(uint8 Chan);
	uint8	GetSamplePos();
	uint8	GetDeltaCounter();
	bool	DPCMPlaying();

public:
	static const uint8	LENGTH_TABLE[];
	static const uint32	BASE_FREQ_NTSC;
	static const uint32	BASE_FREQ_PAL;
	static const uint8	FRAME_RATE_NTSC;
	static const uint8	FRAME_RATE_PAL;

private:
	static const int SEQUENCER_PERIOD;
	
	inline void Clock_240Hz();
	inline void	Clock_120Hz();
	inline void	Clock_60Hz();
	inline void	ClockSequence();
		
private:
	ISoundInterface	*m_pSoundInterface;
	CMixer			*m_pMixer;
	ICallback		*m_pParent;

	CSquare			*SquareCh1;
	CSquare			*SquareCh2;
	CTriangle		*TriangleCh;
	CNoise			*NoiseCh;
	CDPCM			*DPCMCh;

	CVRC6			*m_pVRC6;
	CMMC5			*m_pMMC5;
	CFDS			*m_pFDS;
	CN106			*m_pN106;
	CVRC7			*m_pVRC7;

	uint8			m_iExternalSoundChip;				// External sound chip, if used
	uint32			m_iCurrentTime;
	uint32			m_iNextStopTime;

	uint32			FramePeriod;						// Cycles per frame
	uint32			FrameCycles;						// Cycles emulated from start of frame
	uint32			FrameClock;							// Clock for frame sequencer
	uint8			FrameSequence;						// Frame sequence
	uint8			FrameMode;							// 4 or 5-steps frame sequence
	int32			FrameCyclesLeft;

	uint32			SoundBufferSamples;					// Size of buffer, in samples
	bool			m_bStereoEnabled;					// If stereo is enabled

	uint32			SampleSizeShift;					// To convert samples to bytes
	uint32			SoundBufferSize;					// Size of buffer, counting int32s
//	uint32			SoundBufferSamples;					// Size of buffer, in samples
	uint32			BufferPointer;						// Fill pos in buffer
	int16			*SoundBuffer;						// Sound transfer buffer

};

#endif /* _APU_H_ */