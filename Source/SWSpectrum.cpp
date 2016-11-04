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

#include "stdafx.h"
#include "FamiTracker.h"
#include "SWSpectrum.h"

CSWSpectrum::CSWSpectrum() :
	m_pBlitBuffer(NULL),
	m_pFftObject(NULL)
{
}

CSWSpectrum::~CSWSpectrum()
{
	SAFE_RELEASE_ARRAY(m_pBlitBuffer);
	SAFE_RELEASE(m_pFftObject);
}

void CSWSpectrum::Activate()
{
	memset(&bmi, 0, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize	 = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biHeight	 = -WIN_HEIGHT;
	bmi.bmiHeader.biWidth	 = WIN_WIDTH;
	bmi.bmiHeader.biPlanes	 = 1;

	SAFE_RELEASE_ARRAY(m_pBlitBuffer);

	m_pBlitBuffer = new int[WIN_WIDTH * WIN_HEIGHT * 2];
	memset(m_pBlitBuffer, 0, WIN_WIDTH * WIN_HEIGHT * sizeof(int));

	SAFE_RELEASE(m_pFftObject);

	m_pFftObject = new Fft(FFT_POINTS, 44100);

	for (int i = 0; i < FFT_POINTS; i++)
		m_iFftPoint[i] = 0;

	m_iCount = 0;
}

void CSWSpectrum::Deactivate()
{
	SAFE_RELEASE_ARRAY(m_pBlitBuffer);
	SAFE_RELEASE(m_pFftObject);
}

void CSWSpectrum::SetSampleData(int *pSamples, unsigned int iCount)
{
	m_iCount = iCount;
	m_pSamples = pSamples;

	int i = 0;

	while (iCount >= FFT_POINTS) {
		m_pFftObject->CopyIn(FFT_POINTS, m_pSamples + i);
		m_pFftObject->Transform();
		i += FFT_POINTS;
		iCount -= FFT_POINTS;
	}
}

void CSWSpectrum::Draw(CDC *pDC, bool bMessage)
{
	unsigned int i = 0; int y, bar;

	if (bMessage)
		return;

	float Stepping = (float)(FFT_POINTS - (float(FFT_POINTS) / 1.5f)) / (float)WIN_WIDTH; //(float)(FFT_POINTS - (FFT_POINTS / 2) - 40) / (float)WIN_WIDTH;
	float Step = 0;

	for (i = 0; i < WIN_WIDTH; i++) {
		// skip the first 20 Hzs
		bar = (int)m_pFftObject->GetIntensity(int(Step)/* + 20*/) / 80;

		if (bar > WIN_HEIGHT)
			bar = WIN_HEIGHT;

		if (bar > m_iFftPoint[(int)Step]) {
			m_iFftPoint[(int)Step] = bar;
		}
		else {
			m_iFftPoint[(int)Step] -= 2;
		}

		bar = m_iFftPoint[(int)Step];

		for (y = 0; y < WIN_HEIGHT; y++) {
			if (y < bar)
				m_pBlitBuffer[(WIN_HEIGHT - y) * WIN_WIDTH + i] = 0xFFFFFF - (DIM(0xFFFF80, (y * 100) / bar) + 0x80) + 0x000080;
			else
				m_pBlitBuffer[(WIN_HEIGHT - y) * WIN_WIDTH + i] = 0x000000;
		}

		Step += Stepping;
	}

	StretchDIBits(*pDC, 0, 0, WIN_WIDTH, WIN_HEIGHT, 0, 0, WIN_WIDTH, WIN_HEIGHT, m_pBlitBuffer, &bmi, DIB_RGB_COLORS, SRCCOPY);
}
