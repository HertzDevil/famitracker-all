#include "stdafx.h"
#include "FamiTracker.h"
#include "SWLogo.h"

void CSWLogo::Activate()
{
}

void CSWLogo::Deactivate()
{
}

void CSWLogo::SetSampleData(int *pSamples, unsigned int iCount)
{
}

void CSWLogo::Draw(CDC *pDC, bool bMessage)
{
	if (!bMessage)
		return;

	CBitmap Bmp, *OldBmp;
	CDC		BitmapDC;

	Bmp.LoadBitmap(IDB_SAMPLEBG);
	BitmapDC.CreateCompatibleDC(pDC);
	OldBmp = BitmapDC.SelectObject(&Bmp);

	pDC->BitBlt(0, 0, WIN_WIDTH, WIN_HEIGHT, &BitmapDC, 0, 0, SRCCOPY);

	BitmapDC.SelectObject(OldBmp);
}
