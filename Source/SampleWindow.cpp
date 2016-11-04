/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2009  Jonathan Liss
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
#include "..\include\samplewindow.h"

#include "SWSampleScope.h"
#include "SWSpectrum.h"
#include "SWLogo.h"

// CSampleWindow

IMPLEMENT_DYNAMIC(CSampleWindow, CWnd)


CSampleWindow::CSampleWindow()
{
//	m_iMin = m_iSec = m_iMSec = 0;
	m_iCurrentState = 0;
}

CSampleWindow::~CSampleWindow()
{
}

BEGIN_MESSAGE_MAP(CSampleWindow, CWnd)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_PAINT()
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

// CSampleWinProc message handlers

BOOL CSampleWinProc::InitInstance()
{
	return TRUE;
}

int CSampleWinProc::Run()
{
	MSG msg;
	bool Running = true;

	while (Running) {
		while (GetMessage(&msg, NULL, 0, 0)) {
			if (msg.message == WM_USER) {
				Wnd->DrawSamples((int*)msg.wParam, (int)msg.lParam);
			}
			else if (msg.message == WM_QUIT) {
				Running = false;
			}
		}
	}

	return CWinThread::Run();
}


// State methods

void CSampleWindow::NextState()
{
	m_iCurrentState = (m_iCurrentState + 1) % STATE_COUNT;
	Invalidate();
}

// CSampleWindow message handlers

void CSampleWindow::DrawSamples(int *Samples, int Count)
{
	if (m_hWnd) {
		CDC *pDC = GetDC();
		m_pStates[m_iCurrentState]->SetSampleData(Samples, Count);
		m_pStates[m_iCurrentState]->Draw(pDC, false);
		ReleaseDC(pDC);
		delete [] Samples;
	}
}

BOOL CSampleWindow::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
	m_pStates[0] = new CSWSampleScope(false);
	m_pStates[1] = new CSWSampleScope(true);
	m_pStates[2] = new CSWSpectrum();
	m_pStates[3] = new CSWLogo();
	
	m_iCurrentState = 0;

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


BOOL CSampleWindow::DestroyWindow()
{
	for (int i = 0; i < STATE_COUNT; i++) {
		m_pStates[i]->Deactivate();
	}

	return CWnd::DestroyWindow();
}
