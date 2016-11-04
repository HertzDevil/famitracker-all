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

class CWaveEditor : public CWnd
{
public:
	CWaveEditor(int sx, int sy, int lx, int ly);
	virtual ~CWaveEditor();
	DECLARE_DYNAMIC(CWaveEditor)
private:
	void EditWave(CPoint pt1, CPoint pt2);
	void EditWave(CPoint point);
	void DrawLine(CDC *pDC);

protected:
	virtual int GetSample(int i) const = 0;
	virtual void SetSample(int i, int s) = 0;
	virtual int GetMaxSamples() const = 0;
	virtual void DrawRect(CDC *pDC, int x, int y, int sx, int sy) const = 0;

protected:
	int m_iSX, m_iSY;
	int m_iLX, m_iLY;

	CPoint m_ptLineStart, m_ptLineEnd;

	bool m_bDrawLine;

	static bool m_bLineMode;

public:
	void WaveChanged();
	virtual afx_msg void OnPaint();

protected:
	DECLARE_MESSAGE_MAP()

public:
	BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
};

// Templates would be better but doesn't work well with MFC unfortunately

// FDS wave
class CWaveEditorFDS : public CWaveEditor
{
public:
	CWaveEditorFDS(int sx, int sy, int lx, int ly) : CWaveEditor(sx, sy, lx, ly), m_pInstrument(NULL) {};
	void SetInstrument(CInstrumentFDS *pInst);
protected:
	virtual int GetSample(int i) const;
	virtual void SetSample(int i, int s);
	virtual int GetMaxSamples() const;
	virtual void DrawRect(CDC *pDC, int x, int y, int sx, int sy) const;
protected:
	CInstrumentFDS *m_pInstrument;
};

// N106 wave
class CWaveEditorN106 : public CWaveEditor
{
public:
	CWaveEditorN106(int sx, int sy, int lx, int ly) : CWaveEditor(sx, sy, lx, ly), m_pInstrument(NULL) {};
	void SetInstrument(CInstrumentN106 *pInst);
protected:
	virtual int GetSample(int i) const;
	virtual void SetSample(int i, int s);
	virtual int GetMaxSamples() const;
	virtual void DrawRect(CDC *pDC, int x, int y, int sx, int sy) const;
protected:
	CInstrumentN106 *m_pInstrument;
};
