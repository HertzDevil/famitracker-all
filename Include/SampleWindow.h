/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2007  Jonathan Liss
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

class CSampleWinState
{
public:
	virtual void Activate() = 0;
	virtual void Deactivate() = 0;
	virtual void SetSampleData(int *iSamples, unsigned int iCount) = 0;
	virtual void Draw(CDC *pDC, bool bMessage) = 0;

protected:
	static const int WIN_WIDTH = 145;
	static const int WIN_HEIGHT = 40;
};

// CSampleWindow

class CSampleWindow : public CWnd
{
	DECLARE_DYNAMIC(CSampleWindow)

public:
	CSampleWindow();
	virtual ~CSampleWindow();

	void DrawSamples(int *Samples, int Count);

	static const int WIN_WIDTH = 145;
	static const int WIN_HEIGHT = 40;
 	
private:
	static const int STATE_COUNT = 4;

	void NextState();

	CSampleWinState *m_pStates[STATE_COUNT];
	unsigned int	m_iCurrentState;

public:
	virtual BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual BOOL DestroyWindow();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
public:
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
};

class CSampleWinProc : public CWinThread
{
public:
	CSampleWindow *Wnd;
	virtual int Run();
	virtual BOOL InitInstance();
};
