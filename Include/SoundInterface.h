#ifndef _SOUNDINTERFACE_H_
#define _SOUNDINTERFACE_H_

#include "common.h"

class ISoundInterface
{
public:
	virtual void FlushAudio(int32 *Samples, uint32 SampleCount) = 0;
	virtual void StopSound() = 0;
	virtual void Quit() = 0;
	virtual void Reset() = 0;
private:
};

#endif /* _SOUNDINTERFACE_H_ */