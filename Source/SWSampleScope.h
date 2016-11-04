#pragma once

#include "SampleWindow.h"

class CSWSampleScope : public CSampleWinState
{
public:
	CSWSampleScope(bool bBlur);
	void			Activate();
	void			Deactivate();
	void			Draw(CDC *pDC, bool bMessage);
	void			SetSampleData(int *pSamples, unsigned int iCount);

private:
	bool			m_bBlur;
	unsigned int	m_iCount;
	int				*m_pSamples;
	int				*m_pBlitBuffer;
	int				m_iWindowBufPtr, *m_pWindowBuf;
	BITMAPINFO		bmi;
};
