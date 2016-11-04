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

// CSampleView control

class CSampleView : public CStatic {
	DECLARE_DYNAMIC(CSampleView)
protected:
	DECLARE_MESSAGE_MAP()
public:
	CSampleView();
	void DrawPlayCursor(int Pos);
	void DrawStartCursor();
	int GetStartOffset() const { return m_iStartCursor; };
	int GetSelStart() const { return m_iSelStart; };
	int GetSelEnd() const { return m_iSelEnd; };
	void CalculateSample(CDSample *pSample, int Start);
	int GetBlock(int Pixel) const;
	int GetPixel(int Block) const;
	void UpdateInfo();
private:
	int *m_pSamples;
	int m_iSize;
	int m_iSelStart;
	int m_iSelEnd;

	int m_iStartCursor;

	double m_dSampleStep;
	int m_iBlockSize;

	bool m_bClicked;

	CRect clientRect;
	CDC copy;
	CBitmap copyBmp;

public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};

// CSampleEditorDlg dialog

class CSampleEditorDlg : public CDialog
{
	DECLARE_DYNAMIC(CSampleEditorDlg)

public:
	CSampleEditorDlg(CWnd* pParent = NULL, CDSample *pSample = NULL);   // standard constructor
	virtual ~CSampleEditorDlg();

// Dialog Data
	enum { IDD = IDD_SAMPLE_EDITOR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void MoveControls();
	void UpdateSampleView();

	CDSample	*m_pSample;
	CDSample	*m_pOriginalSample;
	CSampleView *m_pSampleView;
	CSoundGen	*m_pSoundGen;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnMove(int x, int y);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedPlay();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedDelete();
	afx_msg void OnNMCustomdrawSlider1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedTest();
	afx_msg void OnBnClickedDeltastart();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnBnClickedTilt();
};
