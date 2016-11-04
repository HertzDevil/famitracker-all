/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2006  Jonathan Liss
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

#pragma once


// CSampleWindow

class CSampleWindow : public CWnd
{
	DECLARE_DYNAMIC(CSampleWindow)

public:
	CSampleWindow();
	virtual ~CSampleWindow();

	static const int WIN_WIDTH = 145;
	static const int WIN_HEIGHT = 40;
 	
	bool		Active, Blur;
	int			*BlitBuffer;

	BITMAPINFO	bmi;

	void DrawSamples(int *Samples, int Count);
	virtual BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);

protected:
	void DrawFFT(int *Samples, int Count, CDC *pDC);
	void DrawGraph(int *Samples, int Count, CDC *pDC);

	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
};

class CSampleWinProc : public CWinThread
{
public:
	CSampleWindow *Wnd;
	virtual int Run();
	virtual BOOL InitInstance();
};
