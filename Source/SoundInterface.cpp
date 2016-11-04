#include "stdafx.h"
#include "FamiTracker.h"
#include "common.h"
#include "DSoundInterface.h"
#include "DSound.h"
//#include "SampleGraph.h"
//#include "MixerDialog.h"

int SampleShift = 0;
int32 SampleMax = 32767;
int32 SampleMin = -32768;

int32 *m_iGraphBuffer;

CSoundInterface::CSoundInterface()
{
	m_pDSoundFormat = NULL;
}

CSoundInterface::~CSoundInterface() 
{
	if (m_pDSoundFormat)
		delete [] m_pDSoundFormat;
}

void CSoundInterface::FlushAudio(int32 *Samples, uint32 SampleCount)
{
	// Transfer the buffer to DirectSound
	//
	// Buffer	- Pointer to a buffer containing 32 bit samples
	// Samples	- Amount of samples in the buffer
	//

	int32	Sample;
	uint32	SamplesInBytes;
	uint32	BlockSamples;

	BlockSamples = m_pDSoundChannel->GetBlockSamples();

	// Convert samples
	for (uint32 i = 0; i < SampleCount; i++) {
		Sample = Samples[i] >> SampleShift;

		if (Sample > SampleMax)
			Sample = SampleMax;
		if (Sample < SampleMin)
			Sample = SampleMin;

		m_iGraphBuffer[m_iBufferPtr] = Sample;

		((int16*)(m_pDSoundFormat))[m_iBufferPtr++] = (int16)Sample;

		if (m_iBufferPtr == BlockSamples) {
			// Wait until it's time
			while (m_pDSoundChannel->WaitForDirectSoundEvent() == 1) {
				m_pDSoundChannel->ResetNotification();
			}

			SamplesInBytes = (BlockSamples * 2);
			m_pDSoundChannel->WriteSoundBuffer(m_pDSoundFormat, SamplesInBytes);

			theApp.DrawSamples((int*)m_iGraphBuffer, m_iBufferSize);
			m_iGraphBuffer = new int32[m_iBufferSize + 16];

			m_iBufferPtr = 0; //-= BlockSamples;
		}
	}
}

void CSoundInterface::SetDSoundChannel(CDSoundChannel *pDSoundChannel)
{
	uint32 BlockSize;

	if (m_pDSoundFormat)
		delete [] m_pDSoundFormat;

	BlockSize = pDSoundChannel->GetBlockSize() / (16 / 8);
	m_pDSoundFormat = new int16[BlockSize];
	m_pDSoundChannel = pDSoundChannel;
	m_iBufferSize = BlockSize;
	m_iBufferPtr = 0;

	m_iGraphBuffer = new int32[m_iBufferSize];
}

void CSoundInterface::Reset()
{
//	PrintDebug(" * SoundInterface reset\n");
	m_pDSoundChannel->Stop();
//	m_pDSoundChannel->Prepare();
	m_iBufferPtr = 0;

//	m_pVUMeter->Reset();
}

void CSoundInterface::StopSound()
{
//	PrintDebug(" * SoundInterface stopping audio\n");
	m_pDSoundChannel->Stop();
	m_iBufferPtr = 0;
}

void CSoundInterface::Quit()
{
	m_pDSoundChannel->SetNotification();
}
