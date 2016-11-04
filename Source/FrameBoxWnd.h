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

class CFamiTrackerDoc;
class CFamiTrackerView;

// CFrameBoxWnd

class CFrameBoxWnd : public CWnd
{
	DECLARE_DYNAMIC(CFrameBoxWnd)
public:
	CFrameBoxWnd(CMainFrame *pMainFrm);
	virtual ~CFrameBoxWnd();

	void AssignDocument(CFamiTrackerDoc *pDoc, CFamiTrackerView *pView);
	void EnableInput();
	bool InputEnabled() const;
	void CreateGdiObjects();

public:
	static const int FRAME_ITEM_WIDTH = 20;

	static const int ROW_HEIGHT = 15;
	static const int TOP_OFFSET = 3;

	static const TCHAR DEFAULT_FONT[];
	
private:
	// GDI objects
	CFont	m_Font;
	CBitmap m_bmpBack;
	CDC		m_dcBack;

	HACCEL m_hAccel;
	CMainFrame *m_pMainFrame;
	int m_iHiglightLine;
	int m_iFirstChannel;
	int m_iCursorPos;
	int m_iNewPattern;
	bool m_bInputEnable;
	bool m_bCursor;
	bool m_bControl;
	int m_iCopiedValues[MAX_CHANNELS];
	CFamiTrackerDoc *m_pDocument;
	CFamiTrackerView *m_pView;

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};


