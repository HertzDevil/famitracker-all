/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005  Jonathan Liss
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

const struct {
	const static int BACKGROUND			= 0xFF000000;		// Background color
	const static int TEXT_NORMAL		= 0x0000FF00;		// Normal text color
	const static int TEXT_HILITE		= 0x0000F0F0;		// Highlighted text color
	const static int TEXT_MUTED			= 0x000000FF;		// Channel muted color
	const static int BORDER_NORMAL		= 0x00800000;		// Normal border, inactive
	const static int BORDER_EDIT		= 0x00000080;		// Edit border, inactive
	const static int BORDER_NORMAL_A	= 0x00FF0000;		// Normal border, active
	const static int BORDER_EDIT_A		= 0x000000FF;		// Edit border, active
} COLOR_SCHEME;

const static char *FONT_FACE = "Fixedsys";

const int UNDO_LEVELS = 30;

struct stUndoBlock {
	stChanNote ChannelData[MAX_PATTERN_LENGTH];
	int Pattern;
	int Channel;
	int Row, Column;
	int Frame;
};

class CFamiTrackerView : public CView
{
protected: // create from serialization only
	CFamiTrackerView();
	DECLARE_DYNCREATE(CFamiTrackerView)

// Attributes
public:
	CFamiTrackerDoc* GetDocument() const;

// Operations
public:

// Overrides
	public:
//	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	int WindowWidth;
	int WindowHeight;
	int VisibleRows;

// Implementation
public:
	virtual ~CFamiTrackerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Drawing
	void	DrawChar(int x, int y, char c);
	void	DrawLine(stChanNote *NoteData, int Line, int Channel);

	void	ResetView();

	void	SelectPattern(int Index);
	void	SelectNextPattern();
	void	SelectPrevPattern();
	void	SelectFirstPattern();
	void	SelectLastPattern();

	int		GetCurrentPattern() { return m_iCurrentFrame; };
	int		GetCurrentChannel() { return m_iCursorChannel; };

	void	IncreaseCurrentFrame();
	void	DecreaseCurrentFrame();

	void	MoveCursorLeft();
	void	MoveCursorRight();
	void	MoveCursorNextChannel();
	void	MoveCursorPrevChannel();
	void	MoveCursorFirstChannel();
	void	MoveCursorLastChannel();
	void	SelectChannel(int Index);

	void	SetSongSpeed(int Speed);
	void	SetInstrument(int Instrument);

	void	RegisterKeyState(int Channel, int Note);

	int		m_iKeyStepping;
	bool	m_bChangeAllPattern;
	bool	m_bEditEnable;
	bool	m_bPasteOverwrite;

	bool	m_bMuteChannels[5];

	int		m_iInstrument;			// Selected instrument
	int		m_iOctave;
	int		m_iCurrentTempo;

	bool	m_bInitialized;

protected:
	int		m_iCurrentFrame;		// Middle row in frame list
	int		m_iCurrentRow;			// Middle row in pattern field
	int		m_iCursorRow;			// Selected row
	int		m_iCursorChannel;		// Selected channel
	int		m_iCursorColumn;		// Selected channel-part (note, instrument or effect)

	bool	m_bPlayLooped;
	bool	m_bPlaying;
	int		m_iPlayTime;

	int		m_iLastRowState;
	int		m_iLastFrameState;

	int		m_iUndoLevel;
	int		m_iRedoLevel;
	bool	m_bHasFocus;

	unsigned int	m_iVolLevels[5];

	stUndoBlock		UndoStack[UNDO_LEVELS + 1];

	bool	m_bRedrawed;

	int		m_iSelectStart;
	int		m_iSelectEnd;
	int		m_iSelectChannel;
	int		m_iClipBoard;

	void	WrapSelectedLine();
	void	WrapSelectedPattern();

	void	ScrollRowUp();
	void	ScrollRowDown();
	void	ScrollPageUp();
	void	ScrollPageDown();
	void	ScrollToTop();
	void	ScrollToBottom();

	void	StepDown();

	void	InterpretKey(int Key);
	void	KeyReleased(int Key);
	bool	TranslateKey(int Key, stChanNote *Note);
	bool	PreventRepeat(int Key);

	char	m_cKeyList[256];

	void	PlayRow(int RowPos, int PatternPos);

	void	DrawMeters(CDC *pDC);
	void	ForceRedraw();

	void	RemoveWithoutDelete();
	void	SelectWholePattern(int Channel);

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
	virtual void OnDraw(CDC* /*pDC*/);
public:
	void TogglePlayback(void);
	void PlayPattern(void);
	bool IsPlaying(void);
	void SetSpeed(int Speed);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	void AddUndo(int Channel);

	int GetSelectedChannel() {
		return m_iCursorChannel;
	}

protected:
	virtual void CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType = adjustBorder);
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
//	afx_msg void OnTimer(UINT nIDEvent);
private:
	void PlaybackTick(void);
	void UpdateTrackerView();
	void SetMessageText(LPCSTR lpszText);
	void InsertNote(int Note, int Octave, int Channel, int Velocity);
	void StopNote(int Channel);

	bool m_bShiftPressed;
	bool m_bForceRedraw;
	bool FirstTick;
	int	TickPeriod;
	int PlayerSyncTick;
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnEditCopy();
	afx_msg void OnEditCut();
	afx_msg void OnEditPaste();
	afx_msg void OnUpdateEditCopy(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditCut(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditPaste(CCmdUI *pCmdUI);
	afx_msg void OnEditDelete();
	afx_msg void OnUpdateEditDelete(CCmdUI *pCmdUI);
	afx_msg void OnUpdateTrackerStop(CCmdUI *pCmdUI);
	afx_msg void OnUpdateTrackerPlay(CCmdUI *pCmdUI);
	afx_msg void OnUpdateTrackerPlaypattern(CCmdUI *pCmdUI);
	afx_msg void OnTrackerPlay();
	afx_msg void OnTrackerPlaypattern();
	afx_msg void OnTrackerStop();
	afx_msg void OnTrackerEdit();
	afx_msg void OnUpdateTrackerEdit(CCmdUI *pCmdUI);
protected:
	virtual void OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/);
public:
	afx_msg void OnTrackerPal();
	afx_msg void OnTrackerNtsc();
	afx_msg void OnUpdateTrackerPal(CCmdUI *pCmdUI);
	afx_msg void OnUpdateTrackerNtsc(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSpeedDefalut(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSpeedCustom(CCmdUI *pCmdUI);
	afx_msg void OnSpeedCustom();
	afx_msg void OnSpeedDefalut();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnEditPasteoverwrite();
	afx_msg void OnUpdateEditPasteoverwrite(CCmdUI *pCmdUI);
	afx_msg void OnTransposeDecreasenote();
	afx_msg void OnTransposeDecreaseoctave();
	afx_msg void OnTransposeIncreasenote();
	afx_msg void OnTransposeIncreaseoctave();
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateEditUndo(CCmdUI *pCmdUI);
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateEditRedo(CCmdUI *pCmdUI);
	virtual void OnInitialUpdate();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnEditSelectall();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	void TranslateMidiMessage();
	void SetupMidi();
	afx_msg void OnFileMidisettings();
protected:
	virtual void PostNcDestroy();
public:
//	virtual BOOL DestroyWindow();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnEditEnablemidi();
	afx_msg void OnUpdateEditEnablemidi(CCmdUI *pCmdUI);
	afx_msg void OnFrameInsert();
	afx_msg void OnFrameRemove();
	afx_msg void OnTrackerPlayrow();
};

#ifndef _DEBUG  // debug version in FamiTrackerView.cpp
inline CFamiTrackerDoc* CFamiTrackerView::GetDocument() const
   { return reinterpret_cast<CFamiTrackerDoc*>(m_pDocument); }
#endif

