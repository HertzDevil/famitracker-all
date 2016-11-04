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

#include "famitrackerdoc.h"
#include "PatternWnd.h"
#include "InstrumentEditDlg.h"

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
public:

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	CDocument	*GetDocument() { return GetActiveDocument(); };
	CView		*GetView() { return GetActiveView(); };
	void		SetStatusText(LPCTSTR Text,...);
	void		DrawSamples(int *Samples, int Count);
	void		ChangeNoteState(int Note);

protected:  // control bar embedded members
	CStatusBar	m_wndStatusBar;
	CToolBar	m_wndToolBar;
	CPatternWnd	m_wndPatternWindow;
	CDialogBar	m_wndDialogBar;

	CListCtrl	*InstrumentList;
	CListCtrl	*InstSettingsList;
	CListCtrl	*ModifierList;

	bool		m_bInitialized;

	void		OpenInstrumentSettings();

	CInstrumentEditDlg	m_InstEdit;

// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	DECLARE_MESSAGE_MAP()
public:
	void ClearInstrumentList();
	void AddInstrument(int Index, const char *Name);
	void RemoveInstrument(int Index);
	void SetIndicatorTime(int Min, int Sec, int MSec);
	void RefreshPattern();
	void SetSongInfo(char *Name, char *Artist, char *Copyright);
	void SetupColors(void);
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle = WS_OVERLAPPEDWINDOW, const RECT& rect = rectDefault, CWnd* pParentWnd = NULL, LPCTSTR lpszMenuName = NULL, DWORD dwExStyle = 0, CCreateContext* pContext = NULL);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClickInstruments(NMHDR *pNotifyStruct, LRESULT *result);
	afx_msg void OnDblClkInstruments(NMHDR *pNotifyStruct, LRESULT *result);
	afx_msg void OnInstNameChange();
	afx_msg void OnTrackerTogglePlay();
	afx_msg void OnEnPatternsChange();
	afx_msg void OnDeltaposTempoSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnTempoChange();
	afx_msg void OnTrackerKillsound();
	afx_msg void OnDeltaposRowsSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposPatternsSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnRowsChange();
	afx_msg void OnCreateNSF();
	afx_msg void OnSoundSettings();
	afx_msg void OnNextFrame();
	afx_msg void OnPrevFrame();
	afx_msg void OnChangeAll();
	afx_msg void OnPaint();
	afx_msg void OnBnClickedIncFrame();
	afx_msg void OnBnClickedDecFrame();
	afx_msg void OnBnClickedAddInst();
	afx_msg void OnBnClickedRemoveInst();
	afx_msg void OnBnClickedEditInst();
	afx_msg void OnKeyRepeat();
	afx_msg void OnDeltaposKeyStepSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnKeyStepChange();
	afx_msg void OnHelpPerformance();
	afx_msg void OnUpdateSBTempo(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSBPosition(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSBInstrument(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSBOctave(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSBFrequency(CCmdUI *pCmdUI);
	afx_msg void OnUpdateKeyStepEdit(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSpeedEdit(CCmdUI *pCmdUI);
	afx_msg void OnUpdateRowsEdit(CCmdUI *pCmdUI);
	afx_msg void OnUpdateFramesEdit(CCmdUI *pCmdUI);
	afx_msg void OnUpdateKeyRepeat(CCmdUI *pCmdUI);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnFileGeneralsettings();
	afx_msg void OnEnSongNameChange();
	afx_msg void OnEnSongArtistChange();
	afx_msg void OnEnSongCopyrightChange();
	afx_msg void OnFileImportmidi();
	afx_msg void OnEnKillfocusSongName();
	afx_msg void OnEnKillfocusSongArtist();
	afx_msg void OnEnKillfocusSongCopyright();
	afx_msg void OnEnKillfocusTempo();
};


