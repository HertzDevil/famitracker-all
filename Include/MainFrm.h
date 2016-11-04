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

#include "Famitrackerdoc.h"
#include "FrameBoxWnd.h"
#include "InstrumentEditDlg.h"
#include "PerformanceDlg.h"
#include "SampleWindow.h"
#include "DialogReBar.h"
#include "ControlPanelDlg.h"

class CCustomEdit : public CEdit {
	DECLARE_DYNAMIC(CCustomEdit)
protected:
	DECLARE_MESSAGE_MAP()
public:
	CCustomEdit() {
		m_bUpdate = false;
	}
	bool IsEditable();
	bool Update();
	int GetValue();
private:
	void OnLButtonDblClk(UINT nFlags, CPoint point);
	void OnSetFocus(CWnd* pOldWnd);
	void OnKillFocus(CWnd* pNewWnd);
private:
	bool m_bUpdate;
	int m_iValue;
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnFileAddsong();
};

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

private:
	static const char NEW_INST_NAME[];

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

	CDocument		*GetDocument() { return GetActiveDocument(); };
	CView			*GetView() { return GetActiveView(); };
	void			SetStatusText(LPCTSTR Text,...);
	void			DrawSamples(int *Samples, int Count);
	void			ChangeNoteState(int Note);
	void			CloseInstrumentSettings();
	
	void			UpdateTrackBox();
	void			ResizeFrameWindow();

	void			UpdateControls();

protected:
	bool			CreateDialogPanels();
	bool			CreateToolbars();
	bool			CreateInstrumentToolbar();
	bool			CreateSampleWindow();
	void			OpenInstrumentSettings();
	void			SetSpeed(int Speed);
	void			SetTempo(int Tempo);
	void			SetRowCount(int Count);
	void			SetFrameCount(int Count);

protected:  // control bar embedded members
	CStatusBar			m_wndStatusBar;
	CToolBar			m_wndToolBar;
	CFrameBoxWnd		m_wndFrameWindow;
	CReBar				m_wndToolBarReBar;
	CDialogReBar		m_wndOctaveBar;
	CDialogBar			m_wndControlBar;	// Parent to frame editor and settings/instrument editor
	CControlPanelDlg	m_wndDialogBar;

	CToolBarCtrl		m_wndInstToolBar;
	CWnd				m_wndInstToolBarWnd;
	CReBarCtrl			m_wndInstToolReBar;

	CListCtrl			*InstrumentList;
	CListCtrl			*InstSettingsList;
	CListCtrl			*ModifierList;
	CImageList			*m_pImageList;

	CInstrumentEditDlg	m_InstEdit;
	CPerformanceDlg		m_PerformanceDlg;
	CSampleWindow		m_SampleWindow;
	CSampleWinProc		m_SampleProc;

	CCustomEdit			*m_pCustomEditSpeed;
	CCustomEdit			*m_pCustomEditTempo;
	CCustomEdit			*m_pCustomEditLength;
	CCustomEdit			*m_pCustomEditFrames;
	CCustomEdit			*m_pCustomEditStep;

	bool m_bInitialized;

// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	DECLARE_MESSAGE_MAP()
public:
	void ClearInstrumentList();
	void AddInstrument(int Index, const char *Name, int Type);
	void RemoveInstrument(int Index);
	void SetIndicatorTime(int Min, int Sec, int MSec);
	void DrawFrameWindow();
	void SetSongInfo(char *Name, char *Artist, char *Copyright);
	void SetupColors(void);
	void DisplayOctave();
	void UpdateInstrumentIndex();
	int GetHighlightRow();
	int GetSecondHighlightRow();
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle = WS_OVERLAPPEDWINDOW, const RECT& rect = rectDefault, CWnd* pParentWnd = NULL, LPCTSTR lpszMenuName = NULL, DWORD dwExStyle = 0, CCreateContext* pContext = NULL);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClickInstruments(NMHDR *pNotifyStruct, LRESULT *result);
	afx_msg void OnDblClkInstruments(NMHDR *pNotifyStruct, LRESULT *result);
	afx_msg void OnInstNameChange();
	afx_msg void OnDeltaposTempoSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposSpeedSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTrackerKillsound();
	afx_msg void OnDeltaposRowsSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposFrameSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCreateNSF();
	afx_msg void OnCreateWAV();
	afx_msg void OnNextFrame();
	afx_msg void OnPrevFrame();
	afx_msg void OnChangeAll();
	afx_msg void OnPaint();
	afx_msg void OnBnClickedIncFrame();
	afx_msg void OnBnClickedDecFrame();
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
	afx_msg void OnUpdateTempoEdit(CCmdUI *pCmdUI);
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
	virtual BOOL DestroyWindow();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnModuleModuleproperties();
	afx_msg void OnLoadInstrument();
	afx_msg void OnSaveInstrument();
	afx_msg void OnEditInstrument();
	afx_msg void OnAddInstrument();
	afx_msg void OnRemoveInstrument();
	afx_msg void OnBnClickedEditInst();
	afx_msg void OnCbnSelchangeSong();
	afx_msg void OnCbnSelchangeOctave();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnRemoveFocus();
	afx_msg void OnNextSong();
	afx_msg void OnPrevSong();
	afx_msg void OnUpdateNextSong(CCmdUI *pCmdUI);
	afx_msg void OnUpdatePrevSong(CCmdUI *pCmdUI);
	afx_msg void OnTrackerSwitchToInstrument();
	afx_msg void OnUpdateTrackerSwitchToInstrument(CCmdUI *pCmdUI);
	afx_msg LRESULT OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu);
	afx_msg void OnClickedFollow();
	afx_msg void OnToggleFollow();
	afx_msg void OnViewControlpanel();
	afx_msg void OnUpdateViewControlpanel(CCmdUI *pCmdUI);
	afx_msg void OnClearPatterns();
	afx_msg void OnDuplicateFrame();
	afx_msg void OnTrackerDPCM();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
};


