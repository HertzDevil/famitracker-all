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

// This is the wave editor for FDS and N106

#include "stdafx.h"
#include "FamiTrackerDoc.h"
#include "instrument.h"
#include "WaveEditor.h"
#include "Resource.h"


IMPLEMENT_DYNAMIC(CWaveEditor, CWnd)

BEGIN_MESSAGE_MAP(CWaveEditor, CWnd)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()


CWaveEditor::CWaveEditor(int sx, int sy, int lx, int ly) 
 : m_iSX(sx), m_iSY(sy), m_iLX(lx), m_iLY(ly),
   m_pInstrument(NULL)
{
}

CWaveEditor::~CWaveEditor()
{
}

BOOL CWaveEditor::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd)
{
	CRect newRect;

	newRect.top = rect.top;
	newRect.left = rect.left;
	newRect.bottom = rect.top + m_iLY * m_iSY + 4;
	newRect.right = rect.left + m_iLX * m_iSX + 4;

	if (CWnd::CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle, newRect, pParentWnd, 0) == -1)
		return -1;

	return 0;
}

void CWaveEditor::OnPaint()
{
	CPaintDC dc(this);

	// Draw the sample
	dc.FillSolidRect(0, 0, m_iLX * m_iSX, m_iLY * m_iSY, 0xA0A0A0);

	for (int i = 0; i < m_iLX; i += 2) {
		dc.FillSolidRect(0, i * m_iSY, m_iLX * m_iSX, m_iSY, 0xB0B0B0);
	}

	for (int i = 0; i < m_iLX; i++) {
		int Sample = 63 - m_pInstrument->GetSample(i);
		dc.Rectangle(i * m_iSX, Sample * m_iSY, i * m_iSX + m_iSX, Sample * m_iSY + m_iSY);
	}

//	dc.FillSolidRect(0, 0, 100, 100, 0x0);
}

void CWaveEditor::SetInstrument(CInstrumentFDS *pInst)
{
	m_pInstrument = pInst;
	Invalidate();
	RedrawWindow();
}

void CWaveEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	if (nFlags & MK_LBUTTON)
		EditWave(point);
	CWnd::OnMouseMove(nFlags, point);
}

void CWaveEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	EditWave(point);
	CWnd::OnLButtonDown(nFlags, point);
}

void CWaveEditor::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Draw a line
	CWnd::OnRButtonDown(nFlags, point);
}

void CWaveEditor::EditWave(CPoint point)
{
	int index = (point.x - 2) / m_iSX;
	int sample = 64 - ((point.y + 1) / m_iSY);
	int s;

	if (sample < 0)
		sample = 0;
	if (sample > 63)
		sample = 63;

	if (index < 0)
		index = 0;
	if (index > 63)
		index = 63;

	CDC *pDC = GetDC();

	// Erase old sample
	s = 63 - m_pInstrument->GetSample(index);
	pDC->FillSolidRect(index * m_iSX, s * m_iSY, m_iSX, m_iSY, s & 1 ? 0xA0A0A0 : 0xB0B0B0);

	m_pInstrument->SetSample(index, sample);

	// New sample
	s = 63 - m_pInstrument->GetSample(index);
	pDC->FillSolidRect(index * m_iSX, s * m_iSY, m_iSX, m_iSY, 0x000000);

	ReleaseDC(pDC);

	// Indicates wave change
	GetParent()->PostMessageA(WM_USER);
}
