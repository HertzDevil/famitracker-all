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

#pragma once

enum {EDIT_NONE, EDIT_LINE, EDIT_POINT, EDIT_LOOP, EDIT_RELEASE};

// Graph editor base class
class CGraphEditor : public CWnd
{
public:
	CGraphEditor(CSequence *pSequence);
	virtual ~CGraphEditor();
	DECLARE_DYNAMIC(CGraphEditor)
protected:
	static const int GRAPH_LEFT = 28;			// Left side marigin
	static const int GRAPH_BOTTOM = 5;			// Bottom marigin
	static const int ITEM_MAX_WIDTH = 40;
	static const int COLOR_LINES = 0x404040;
protected:
	CWnd *m_pParentWnd;
	CSequence *m_pSequence;
	CFont *m_pSmallFont;
	CRect m_GraphRect;
	CRect m_BottomRect;
	CRect m_ClientRect;
	CBitmap *m_pBitmap;
	CDC *m_pBackDC;
	int m_iLastPlayPos;
	int m_iCurrentPlayPos;
protected:
	int GetItemWidth();
	virtual void Initialize();
	virtual int GetItemHeight();
	virtual void ModifyItem(CPoint point, bool Redraw);
	virtual void ModifyLoopPoint(CPoint point, bool Redraw);
	virtual void ModifyReleasePoint(CPoint point, bool Redraw);
	virtual void DrawRange(CDC *pDC, int Max, int Min);
	void DrawBackground(CDC *pDC, int Lines);
	void DrawLoopPoint(CDC *pDC, int StepWidth);
	void DrawReleasePoint(CDC *pDC, int StepWidth);
	void DrawLine(CDC *pDC);
	void DrawRect(CDC *pDC, int x, int y, int w, int h, bool Highlight);
	void PaintBuffer(CDC *pBackDC, CDC *pFrontDC);
	void CursorChanged(int x);
private:
	int m_iStartLineX, m_iEndLineX;
	int m_iStartLineY, m_iEndLineY;
	int m_iEditing;
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);

	BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, LPVOID lpParam = NULL);
};

// Bar graph editor
class CBarGraphEditor : public CGraphEditor
{
private:
	int m_iItems;
public:
	CBarGraphEditor(CSequence *pSequence, int Items) : CGraphEditor(pSequence), m_iItems(Items) { };
	afx_msg void OnPaint();
	void ModifyItem(CPoint point, bool Redraw);
	int GetItemHeight();
};

// Arpeggio graph editor
class CArpeggioGraphEditor : public CGraphEditor
{
public:
	DECLARE_DYNAMIC(CArpeggioGraphEditor)
	CArpeggioGraphEditor(CSequence *pSequence);
	virtual ~CArpeggioGraphEditor();
	CString GetNoteString(int Value);
private:
	static const int ITEMS = 20;
	int m_iScrollOffset;
	CScrollBar *m_pScrollBar;
protected:
	void Initialize();
	void DrawRange(CDC *pDC, int Max, int Min);
	afx_msg void OnPaint();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	void ModifyItem(CPoint point, bool Redraw);
	int GetItemHeight();
	
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};

// Pitch graph editor
class CPitchGraphEditor : public CGraphEditor
{
private:
	static const int ITEMS = 20;
public:
	CPitchGraphEditor(CSequence *pSequence) : CGraphEditor(pSequence) { };
	afx_msg void OnPaint();
	void ModifyItem(CPoint point, bool Redraw);
	int GetItemHeight();
};
