#include "stdafx.h"
#include "FamiTracker.h"
#include "FontDrawer.h"

const int FONT_CHAR_WIDTH = 10;
const int FONT_CHAR_HEIGHT = 18;

const int PLANE_WIDTH = 200;
const int PLANE_HEIGHT = FONT_CHAR_HEIGHT;

const int FIRST_CHAR	= 0x20;
const int LAST_CHAR		= 0x80;

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

		m_hFontDC.SetTextColor(0xFF00FF);

		for (int i = FIRST_CHAR; i < LAST_CHAR; i++) {
			char c[2];
			c[0] = i;
			m_hFontDC.TextOut((i - FIRST_CHAR) * FONT_CHAR_WIDTH, 0, c, 1);
		}

		m_hFontDC.SelectObject(pOldFont);
		//m_hFontDC.SelectObject(pOldBmp);
	}
}

void CFontDrawer::DrawText(CDC *pDC, int x, int y, char *pStr)
{
	pDC->BitBlt(x, y, FONT_CHAR_WIDTH * (LAST_CHAR - FIRST_CHAR + 20), FONT_CHAR_HEIGHT, &m_hFontDC, 0, 0, SRCCOPY);
}
