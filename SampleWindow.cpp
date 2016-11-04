/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005  Jonathan Liss
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


// CSampleWindow

IMPLEMENT_DYNAMIC(CSampleWindow, CWnd)
CSampleWindow::CSampleWindow()
{
}

CSampleWindow::~CSampleWindow()
{
}


BEGIN_MESSAGE_MAP(CSampleWindow, CWnd)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_PAINT()
END_MESSAGE_MAP()

int *WndBuf;
int WndBufPtr;

// CSampleWinProc message handlers

BOOL CSampleWinProc::InitInstance()
{
	// TODO: Add your specialized code here and/or call the base class

	//return CWinThread::InitInstance();
	return TRUE;
}

int CSampleWinProc::Run()
{
	bool Running = true;

	MSG msg;

	while (Running) {
		while (GetMessage(&msg, NULL, 0, 0)) {
			if (msg.message == WM_USER) {
				Wnd->DrawSamples((int*)msg.wParam, (int)msg.lParam);
			}
		}
	}

	return CWinThread::Run();
}


// CSampleWindow message handlers

void CSampleWindow::DrawSamples(int *Samples, int Count)
{
	//this was made quickly and dirty

	if (!Active)
		return;

	if (m_hWnd == NULL)
		return;

	CDC *pDC = GetDC();
	
	int i = 0;
	
	while (i < Count) {
		WndBuf[WndBufPtr / 7] = Samples[i++];
		WndBufPtr++;
		
		if ((WndBufPtr / 7) > WIN_WIDTH) {
			
			WndBufPtr = 0;
			int x = 0;
			int y = 0;
			int s;
			int l;

			l = (WndBuf[0] / 1000) + WIN_HEIGHT / 2;

			for (x = 0; x < WIN_WIDTH; x++) {
			
				s = (WndBuf[x] / 1000) + WIN_HEIGHT / 2;

				for (y = 0; y < WIN_HEIGHT; y++) {
					
					if ((y == s) || ((y > s && y <= l) || (y >= l && y < s))) {
						if (y > 2)
							BlitBuffer[(y-2) * WIN_WIDTH + x] = 0x508040;
						if (y > 1)
							BlitBuffer[(y-1) * WIN_WIDTH + x] = 0x80F070;
						BlitBuffer[(y+0) * WIN_WIDTH + x] = 0x508040;
					}
					else
						BlitBuffer[y * WIN_WIDTH + x] = 0xFF000000;
				}
				l = s;
			}

			StretchDIBits(*pDC, 0, 0, WIN_WIDTH, WIN_HEIGHT, 0, 0, WIN_WIDTH, WIN_HEIGHT, BlitBuffer, &bmi, DIB_RGB_COLORS, SRCCOPY);
		}
	}

	delete [] Samples;

	ReleaseDC(pDC);
}

BOOL CSampleWindow::OnEraseBkgnd(CDC* pDC)
{
	if (!Active) {
		CBitmap Bmp, *OldBmp;
		CDC		BitmapDC;

		Bmp.LoadBitmap(IDB_SAMPLEBG);
		BitmapDC.CreateCompatibleDC(pDC);
		OldBmp = BitmapDC.SelectObject(&Bmp);

		pDC->BitBlt(0, 0, WIN_WIDTH, WIN_HEIGHT, &BitmapDC, 0, 0, SRCCOPY);

		BitmapDC.SelectObject(OldBmp);
	}

	return NULL;
}

BOOL CSampleWindow::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
	// TODO: Add your specialized code here and/or call the base class

	memset(&bmi, 0, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biBitCount	= 32;
	bmi.bmiHeader.biHeight		= -WIN_HEIGHT;
	bmi.bmiHeader.biWidth		= WIN_WIDTH;
	bmi.bmiHeader.biPlanes		= 1;

	BlitBuffer = new int[WIN_WIDTH * WIN_HEIGHT * 2];

	memset(BlitBuffer, 0, WIN_WIDTH * WIN_HEIGHT * sizeof(int));

	Active = true;

	WndBufPtr = 0;
	WndBuf = new int[WIN_WIDTH];

	return CWnd::CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}

void CSampleWindow::OnLButtonDown(UINT nFlags, CPoint point)
{
	Active = !Active;

	if (!Active)
		RedrawWindow();

	CWnd::OnLButtonDown(nFlags, point);
}

void CSampleWindow::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	
	if (!Active) {
		CBitmap Bmp, *OldBmp;
		CDC		BitmapDC;

		Bmp.LoadBitmap(IDB_SAMPLEBG);
		BitmapDC.CreateCompatibleDC(&dc);
		OldBmp = BitmapDC.SelectObject(&Bmp);

		dc.BitBlt(0, 0, WIN_WIDTH, WIN_HEIGHT, &BitmapDC, 0, 0, SRCCOPY);

		BitmapDC.SelectObject(OldBmp);
	}
}

