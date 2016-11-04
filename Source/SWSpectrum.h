#pragma once

#include "SampleWindow.h"
#include "..\fft\fft.h"

const int FFT_POINTS = 256;

class CSWSpectrum : public CSampleWinState
{
public:
	void Activate();
	void Deactivate();
	void SetSampleData(int *iSamples, unsigned int iCount);
	void Draw(CDC *pDC, bool bMessage);

private:
	unsigned int	m_iCount;
	int				*m_pSamples;
	int				*m_pBlitBuffer;
	int				m_iWindowBufPtr, *m_pWindowBuf;
	BITMAPINFO		bmi;

	Fft				*m_pFftObject;
	int				m_iFftPoint[FFT_POINTS];
};