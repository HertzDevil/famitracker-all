#ifndef _DSOUNDINTERFACE_H_
#define _DSOUNDINTERFACE_H_

#include "SoundInterface.h"
#include "DirectSound.h"

class ISampleGraph;
class CVUMeter;

class CSoundInterface : public ISoundInterface {
public:
	CSoundInterface();
	~CSoundInterface();
	void FlushAudio(int32 *Samples, uint32 SampleCount);
	void StopSound();
	void BeginSound();
	void Quit();
	void Reset();

public:
	void SetDSoundChannel(CDSoundChannel *pDSoundChannel);
private:
	CDSoundChannel	*m_pDSoundChannel;
	void			*m_pDSoundFormat;
	uint32			m_iBufferPtr;
	uint32			m_iBufferSize;
};

#endif /* _DSOUNDINTERFACE_H_ */