#ifndef _MMC5_H_
#define _MMC5_H_

#include "external.h"

class CMMCSquare : public CExChannel {
public:
	CMMCSquare(CMixer *pMixer, int ID) : CExChannel(pMixer, SNDCHIP_MMC5, ID) {};
	~CMMCSquare();

	void	Reset();
	void	Write(uint16 Address, uint8 Value);
	void	WriteControl(uint8 Value);
	uint8	ReadControl();
	void	Process(uint32 Time);

	void	LengthCounterUpdate();
	void	EnvelopeUpdate();

private:
	uint8	m_iDutyLength, m_iDutyCycle;

	uint8	m_iLooping, m_iEnvelopeFix, m_iEnvelopeSpeed;
	uint8	m_iEnvelopeVolume, m_iFixedVolume;
	int8	m_iEnvelopeCounter;
	bool	m_bInvert;

	uint8	m_iControlReg, m_iEnabled;
	uint16	m_iFrequency, m_iLengthCounter;
	uint32	m_iCounter;
};

class CMMC5 : public CExternal {
public:
	CMMC5(CMixer *pMixer);
	~CMMC5();
	void Reset();
	void Write(uint16 Address, uint8 Value);
	uint16 Read(uint16 Address);
	void EndFrame();
	void Process(uint32 Time);
	void LengthCounterUpdate();
	void EnvelopeUpdate();
private:	
	CMMCSquare *m_pSquare1;
	CMMCSquare *m_pSquare2;
	uint8 *m_pEXRAM;
	uint8 m_iMulLow;
	uint8 m_iMulHigh;
};

#endif /* _MMC5_H_ */