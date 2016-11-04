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

class CFamiTrackerView;

// CInstrumentDPCM dialog

class CInstrumentDPCM : public CDialog
{
	DECLARE_DYNAMIC(CInstrumentDPCM)

public:
	CInstrumentDPCM(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInstrumentDPCM();

// Dialog Data
	enum { IDD = IDD_INSTRUMENT_DPCM };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void		BuildKeyList();
	void		BuildSampleList();
	void		UpdateKey(int Index);
	void		LoadSample(char *FilePath, char *FileName);
	void		InsertSample(CDSample *pNewSample);

	CListCtrl	*m_pTableListCtrl;
	CListCtrl	*m_pSampleListCtrl;

	int			m_iSelectedSample;
	int			m_iOctave, m_iSelectedKey;

	CFamiTrackerDoc		*pDoc;
	CFamiTrackerView	*pView;

	DECLARE_MESSAGE_MAP()
public:
	void SetCurrentInstrument(int Index);
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedLoad();
	afx_msg void OnBnClickedUnload();
	afx_msg void OnNMClickSampleList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedImport();
	afx_msg void OnCbnSelchangeOctave();
	afx_msg void OnCbnSelchangePitch();
	afx_msg void OnLvnItemchangedTable(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickTable(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCbnSelchangeSamples();
	afx_msg void OnBnClickedSave();
	afx_msg void OnBnClickedLoop();
//	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedSawhack();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedAdd();
	afx_msg void OnBnClickedRemove();
};
