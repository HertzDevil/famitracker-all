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

#pragma once

const int MAX_ITEMS = MAX_SEQ_ITEMS * 2 - 2;

const int SEQUENCE_EDIT_WIDTH = 540;
const int SEQUENCE_EDIT_HEIGHT = 230;

class CInstrumentSettings;

// CSequenceEditor

enum {BUTTON_PRESET = 1, BUTTON_ADD, BUTTON_REMOVE, BUTTON_LENGTH, BUTTON_DONE, BUTTON_CANCEL};

class CSequenceEditor : public CWnd
{
	DECLARE_DYNAMIC(CSequenceEditor)

public:
	CSequenceEditor();
	virtual ~CSequenceEditor();

	void SetCurrentSequence(int Type, CSequence *List);
	void DrawLine(int PosX, int PosY);
	void CheckEditing(int PosX, int PosY);
	void ChangeItems(int PosX, int PosY, bool Quick);
	void ChangeLoopPoint(int PosX, int PosY);

	void SetParent(CInstrumentSettings *pParent);

	CString CompileList();

private:
	CSequence *m_List;

	int		m_iMax, m_iMin;
	int		m_iSequenceType;
	int		m_iSequenceItemCount;
//	int		m_iSequenceItems[MAX_ITEMS];
	int		m_iLastValueX, m_iLastValueY;
	int		m_iLoopPoint;
	int		m_iEditing;
	int		m_iSeqOrigin;
	int		m_iLevels;
	bool	m_bSequenceSizing, m_bLoopMoving, m_bScrollLock;

	int		m_iStartLineX, m_iStartLineY;
	int		m_iEndLineX, m_iEndLineY;

	int		m_iWidth, m_iHeight;

	CDC		*m_pBackDC;
	CFont	m_Font, m_ButtonFont, m_SmallFont, *m_pOldFont;
	CBitmap	m_Bitmap, *m_pOldBitmap;

	int		m_iButtonPressed;
	bool	m_bShowPresets;
	int		m_iSwitchGroup1;
	int		m_iSwitchGroup2;
	int		m_iSwitchGroup3;

private:
	void	HandleScrollBar(int x, int y, int Width, int Height, int PosX, int PosY);
	bool	HandleButton(int x, int y, int Width, int Name, int PosX, int PosY);
	void	HandleSwitchButton(int x, int y, int Width, int PosX, int PosY, int &Switch, int Nr);

	void	DisplayArea(CDC *pDC);
	void	ClearBackground();
	void	DrawLine(int x1, int y1, int x2, int y2);
	void	DrawScrollBar(int x, int y, int Width, int Height);
	void	DrawButton(int x, int y, int Width, CString Text, int Name);
	void	DrawSwitch(int x, int y, int Width, CString Text, bool Pressed);
	void	CreateBackground(int Space);
	void	DrawVolumeEditor();
	void	DrawArpeggioEditor();
	void	DrawPitchEditor();
	void	DrawDutyEditor();

	void	LoadPreset();

protected:
	CInstrumentSettings	*m_pParent;
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
public:
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	virtual BOOL DestroyWindow();
	virtual BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, LPVOID lpParam = NULL);
	afx_msg void OnTimer(UINT nIDEvent);
};


