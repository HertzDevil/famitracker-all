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
#include "SampleWindow.h"
#include "..\fft\fft.h"
#include "resource.h"
#include "Settings.h"

#include "SWSampleScope.h"
#include "SWSpectrum.h"
#include "SWLogo.h"

// CSampleWindow

IMPLEMENT_DYNAMIC(CSampleWindow, CWnd)


CSampleWindow::CSampleWindow() :
	m_iCurrentState(0)
{
}

CSampleWindow::~CSampleWindow()
{
	for (int i = 0; i < 4; ++i) {
		SAFE_RELEASE(m_pStates[i]);
	}
}

BEGIN_MESSAGE_MAP(CSampleWindow, CWnd)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_PAINT()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

// State methods

void CSampleWindow::NextState()
{
	m_iCurrentState = (m_iCurrentState + 1) % STATE_COUNT;
	Invalidate();

	theApp.GetSettings()->SampleWinState = m_iCurrentState;
}

// CSampleWindow message handlers

void CSampleWindow::DrawSamples(int *Samples, int Count)
{
	if (m_hWnd) {
		CDC *pDC = GetDC();
		m_pStates[m_iCurrentState]->SetSampleData(Samples, Count);
		m_pStates[m_iCurrentState]->Draw(pDC, false);
		ReleaseDC(pDC);
		//delete [] Samples;
	}
}

BOOL CSampleWindow::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
	m_pStates[0] = new CSWSampleScope(false);
	m_pStates[1] = new CSWSampleScope(true);
	m_pStates[2] = new CSWSpectrum();
	m_pStates[3] = new CSWLogo();
	
	// This is saved
	m_iCurrentState = theApp.GetSettings()->SampleWinState;

	for (int i = 0; i < STATE_COUNT; i++) {
		m_pStates[i]->Activate();
	}
	
	return CWnd::CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}

void CSampleWindow::OnLButtonDown(UINT nFlags, CPoint point)
{
	NextState();
	CWnd::OnLButtonDown(nFlags, point);
}

void CSampleWindow::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	NextState();
	CWnd::OnLButtonDblClk(nFlags, point);
}

void CSampleWindow::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	m_pStates[m_iCurrentState]->Draw(&dc, true);
}

void CSampleWindow::OnRButtonUp(UINT nFlags, CPoint point)
{
	CMenu *PopupMenu, PopupMenuBar;
	PopupMenuBar.LoadMenu(IDR_SAMPLE_WND_POPUP);
	PopupMenu = PopupMenuBar.GetSubMenu(0);

	CPoint menuPoint;
	CRect rect;

	GetWindowRect(rect);

	menuPoint.x = rect.left + point.x;
	menuPoint.y = rect.top + point.y;

	UINT Result = PopupMenu->TrackPopupMenu(TPM_RETURNCMD, menuPoint.x, menuPoint.y, this);

	switch (Result) {
		case ID_POPUP_SAMPLEGRAPH1:
			m_iCurrentState = 0;
			break;
		case ID_POPUP_SAMPLEGRAPH2:
			m_iCurrentState = 1;
			break;
		case ID_POPUP_SPECTRUMANALYZER:
			m_iCurrentState = 2;
			break;
		case ID_POPUP_NOTHING:
			m_iCurrentState = 3;
			break;
	}

	Invalidate();
	theApp.GetSettings()->SampleWinState = m_iCurrentState;

	CWnd::OnRButtonUp(nFlags, point);
}
