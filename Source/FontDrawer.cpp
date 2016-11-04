#include "stdafx.h"
#include "FamiTracker.h"
#include "FontDrawer.h"

const int FONT_CHAR_WIDTH = 8;
const int FONT_CHAR_HEIGHT = 14;

const int PLANE_HEIGHT = FONT_CHAR_HEIGHT;

const int FIRST_CHAR	= 0x20;
const int LAST_CHAR		= 0x80;

const int PLANE_WIDTH	= (LAST_CHAR - FIRST_CHAR) * FONT_CHAR_WIDTH;

//CFont Font;

// This was never used

CFontDrawer::CFontDrawer()
{
	m_hFontDC.m_hDC = NULL;
}

void CFontDrawer::SetupFontPlane(CDC *pDC, CFont *pFont)
{
	if (!m_hFontDC.m_hDC) {
		CBitmap *pOldBmp;
		m_hFontDC.CreateCompatibleDC(pDC);
		m_hFontBmp.CreateCompatibleBitmap(pDC, PLANE_WIDTH, PLANE_HEIGHT);
		CFont *pOldFont = m_hFontDC.SelectObject(pFont);
		pOldBmp = m_hFontDC.SelectObject(&m_hFontBmp);

		m_hFontDC.SetTextColor(0xFFFFFF);
		m_hFontDC.SetBkColor(0);

		for (int i = FIRST_CHAR; i < LAST_CHAR; i++) {
			char c[2] = {0, 0};
			c[0] = i;
			m_hFontDC.TextOut((i - FIRST_CHAR) * FONT_CHAR_WIDTH, 0, c, 1);
		}

		m_hFontDC.SelectObject(pOldFont);
		//m_hFontDC.SelectObject(pOldBmp);
	}
}

void CFontDrawer::DrawText(CDC *pDC, int x, int y, char *pStr)
{
	int StringLen = strlen(pStr), i, c;

	for (i = 0; i < StringLen; i++) {
		c = int(pStr[i]) - FIRST_CHAR;
		if (c > 0)
			pDC->BitBlt(x + i * FONT_CHAR_WIDTH, y, FONT_CHAR_WIDTH, FONT_CHAR_HEIGHT, &m_hFontDC, c * FONT_CHAR_WIDTH, 0, SRCCOPY);
	}

//	pDC->BitBlt(x, y, FONT_CHAR_WIDTH * (LAST_CHAR - FIRST_CHAR + 20), FONT_CHAR_HEIGHT, &m_hFontDC, 0, 0, SRCCOPY);
}

void CFontDrawer::DrawChar(CDC *pDC, int x, int y, int c)
{
	c = (c - FIRST_CHAR) * FONT_CHAR_WIDTH;

//	pDC->SelectObject

/*
	for (int ix = 0; ix < FONT_CHAR_WIDTH; ix++) {
		for (int iy = 0; iy < FONT_CHAR_HEIGHT; iy++) {

			if (m_hFontDC.GetPixel(c + ix, iy))
				pDC->SetPixel(x + ix, y + iy, 0xFFFF);

		}
	}
*/
	/*
	CPen Pen(PS_SOLID, 1, 0xFFFF00), *OldPen, *OldPen2;
	OldPen = m_hFontDC.SelectObject(&Pen);
	OldPen2 = pDC->SelectObject(&Pen);
	*/

	CBrush Brush(0xFF00FF), *OldBrush;

	OldBrush = pDC->SelectObject(&Brush);

	//pDC->SetROP2(R2_COPYPEN);

	pDC->BitBlt(x, y, FONT_CHAR_WIDTH, FONT_CHAR_HEIGHT, &m_hFontDC, c, 0, MERGECOPY );

	pDC->SelectObject(OldBrush);

/*
	m_hFontDC.SelectObject(OldPen);
	pDC->SelectObject(OldPen2);
	*/
}