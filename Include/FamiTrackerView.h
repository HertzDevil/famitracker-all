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

const struct {
	const static int BACKGROUND			= 0x00000000;		// Background color
	const static int BACKGROUND_HILITE	= 0x00000000;		// Background color
	const static int TEXT_NORMAL		= 0x0000FF00;		// Normal text color
	const static int TEXT_HILITE		= 0x0000F0F0;		// Highlighted text color
	const static int TEXT_MUTED			= 0x000000FF;		// Channel muted color
	const static int BORDER_NORMAL		= 0x00800000;		// Normal border, inactive
	const static int BORDER_EDIT		= 0x00000080;		// Edit border, inactive
	const static int BORDER_NORMAL_A	= 0x00FF0000;		// Normal border, active
	const static int BORDER_EDIT_A		= 0x000000FF;		// Edit border, active
	const static int SELECTION			= 0x00FF0000;
	const static int CURSOR				= 0x000000FF;
} COLOR_SCHEME;

const static char *FONT_FACE = "Fixedsys";

const int UNDO_LEVELS = 30;

enum eCOLUMNS {C_NOTE, 
			   C_INSTRUMENT1, C_INSTRUMENT2, 
			   C_VOLUME, 
			   C_EFF_NUM, C_EFF_PARAM1, C_EFF_PARAM2,
			   C_EFF2_NUM, C_EFF2_PARAM1, C_EFF2_PARAM2, 
			   C_EFF3_NUM, C_EFF3_PARAM1, C_EFF3_PARAM2, 
			   C_EFF4_NUM, C_EFF4_PARAM1, C_EFF4_PARAM2};

struct stUndoBlock {
	stChanNote ChannelData[MAX_PATTERN_LENGTH];
	int Pattern;
	int Channel;
	int Row, Column;
	int Frame;
};

const unsigned int COLUMNS = 7;

class CFamiTrackerView : public CView
{
protected: // create from serialization only
	CFamiTrackerView();
	DECLARE_DYNCREATE(CFamiTrackerView)

// Attributes
public:
	CFamiTrackerDoc* GetDocument() const;



//
// View access functions
//
public:
	// General
	void				SetInstrument(int Instrument);
	unsigned int		GetInstrument() const { return m_iInstrument; };

	unsigned int		GetOctave() const { return m_iOctave; };
	
	// Scrolling/viewing no-editing functions
	void				MoveCursorUp();
	void				MoveCursorDown();
	void				MoveCursorPageUp();
	void				MoveCursorPageDown();
	void				MoveCursorToTop();
	void				MoveCursorToBottom();

	void				MoveCursorLeft();
	void				MoveCursorRight();
	void				MoveCursorNextChannel();
	void				MoveCursorPrevChannel();
	void				MoveCursorFirstChannel();
	void				MoveCursorLastChannel();

	void				SelectNextFrame();
	void				SelectPrevFrame();
	void				SelectFirstFrame();
	void				SelectLastFrame();
	void				SelectFrame(unsigned int Frame);
	unsigned int		GetSelectedFrame() const { return m_iCurrentFrame; };

	unsigned int		GetCurrentTempo() const { return (m_iTempo * 6) / m_iSpeed; };
	void				SetSongSpeed(unsigned int Speed);

	void				SelectChannel(unsigned int Channel);
	unsigned int		GetSelectedChannel() const { return m_iCursorChannel; };

	// Settings
	unsigned int		GetStepping() const { return m_iKeyStepping; };
	void				SetStepping(int Step) { m_iKeyStepping = Step; };
	void				SetChangeAllPattern(bool ChangeAll) { m_bChangeAllPattern = ChangeAll; };

	// Document editing functions
	void				IncreaseCurrentPattern();
	void				DecreaseCurrentPattern();

	// Player
	void				PlaybackTick(CFamiTrackerDoc *pDoc);

protected:
	// Drawing functions
	void				DrawChar(int x, int y, char c, int Color, CDC *dc);
	void				DrawLine(int Offset, stChanNote *NoteData, int Line, int Column, int Channel, int Color, CDC *dc);
	void				DrawMeters(CDC *pDC);
	void				PrepareBackground(CDC *dc);
	void				DrawPatternField(CDC *dc);

	unsigned int		GetChannelAtPoint(unsigned int PointX);
	unsigned int		GetColumnAtPoint(unsigned int PointX, unsigned int MaxColumns);

	// General
	void				StepDown();
	void				AddUndo(unsigned int Channel);
	void				SelectWholePattern(unsigned int Channel);

	unsigned int		GetCurrentColumnCount() const { return COLUMNS + GetDocument()->GetEffColumns(m_iCursorChannel) * 3; };

	unsigned int		GetSelectStart() const { return (m_iSelectEnd > m_iSelectStart ? m_iSelectStart : m_iSelectEnd); };
	unsigned int		GetSelectEnd()	 const { return (m_iSelectEnd > m_iSelectStart ? m_iSelectEnd : m_iSelectStart); };
	unsigned int		GetSelectColStart() const { return (m_iSelectColEnd > m_iSelectColStart ? m_iSelectColStart : m_iSelectColEnd); };
	unsigned int		GetSelectColEnd()	const { return (m_iSelectColEnd > m_iSelectColStart ? m_iSelectColEnd : m_iSelectColStart); };

	// Player
	void				GetStartSpeed();

	// Input handling
	void				HandleKeyboardInput(int Key);
	void				TranslateMidiMessage();
	void				InterpretKey(int Key);
	bool				PreventRepeat(int Key);
	stChanNote			TranslateKey(int Key);

	void				RemoveWithoutDelete();

//
// Important View variables
//
protected:
	unsigned int		m_iInstrument;			// Selected instrument
	unsigned int		m_iOctave;

	// Cursor & editing
	unsigned int		m_iCurrentFrame;										// Middle row in frame list (current frame on screen)
	unsigned int		m_iCurrentRow;											// Middle row in pattern field (mostly locked at m_iCursorRow)
	unsigned int		m_iCursorRow, m_iCursorChannel, m_iCursorColumn;		// Selected channel-part (note, instrument or effect)

	unsigned int		m_iLastRowState, m_iLastFrameState, m_iLastCursorColumn;
	unsigned int		m_iKeyStepping;

	bool				m_bChangeAllPattern;
	bool				m_bPasteOverwrite;
	bool				m_bEditEnable;

	// General
	bool				m_bInitialized;
	bool				m_bShiftPressed;
	bool				m_bHasFocus;
	UINT				m_iClipBoard;

	// Playing
	bool				m_bPlaying, m_bPlayLooped;
	bool				m_bFirstTick;
	bool				m_bMuteChannels[5];
	unsigned int		m_iPlayTime;
	unsigned int		m_iTickPeriod;
	unsigned int		m_iPlayerSyncTick;

	unsigned int		m_iTempo, m_iSpeed;		// Tempo and speed
	int					m_iTempoAccum;			// Used for speed calculation

	// Window size
	unsigned int		m_iWindowWidth, m_iWindowHeight;
	unsigned int		m_iVisibleRows;

	// Selection
	unsigned int		m_iSelectStart, m_iSelectEnd;
	unsigned int		m_iSelectColStart, m_iSelectColEnd;
	unsigned int		m_iSelectChannel;

	// Drawing
	bool				m_bForceRedraw;						// Draw the thing
	unsigned int		m_iStartChan;						// The first drawn channel
	unsigned int		m_iChannelsVisible;
	unsigned int		m_iChannelWidths[MAX_CHANNELS];
	unsigned int		m_iChannelColumns[MAX_CHANNELS];
	unsigned int		m_iVolLevels[5];

	// Undo
	unsigned int		m_iUndoLevel, m_iRedoLevel;
	stUndoBlock			m_UndoStack[UNDO_LEVELS + 1];

	// Input
	char				m_cKeyList[256];

	// Unsorted
	bool				IgnoreFirst;

	unsigned int		m_iKeyboardNote;
	unsigned int		m_iSoloChannel;



// ---------------------------
// These below will be removed
// ---------------------------

// Operations
public:
	void	PlayNote(int Key);
	void	KeyReleased(int Key);
	void	RegisterKeyState(int Channel, int Note);

/////////7
	stChanNote	CurrentNotes[MAX_CHANNELS];
	bool		NewNoteData[MAX_CHANNELS];
////////7777777


// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:
	virtual ~CFamiTrackerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
	virtual void OnDraw(CDC* /*pDC*/);
public:
	void TogglePlayback(void);
	void PlayPattern(void);
	bool IsPlaying(void);
	void SetSpeed(int Speed);
	void ForceRedraw();

	void InsertNote(int Note, int Octave, int Channel, int Velocity);
	void StopNote(int Channel);
	void RefreshStatusMessage();

	void FeedNote(int Channel, stChanNote *NoteData);
	void MuteNote(int Channel);

protected:
	virtual void CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType = adjustBorder);
	virtual void OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/);
	virtual void PostNcDestroy();

private:
	void SetMessageText(LPCSTR lpszText);

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
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
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnEditEnablemidi();
	afx_msg void OnUpdateEditEnablemidi(CCmdUI *pCmdUI);
	afx_msg void OnFrameInsert();
	afx_msg void OnFrameRemove();
	afx_msg void OnTrackerPlayrow();
	afx_msg void OnUpdateFrameRemove(CCmdUI *pCmdUI);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
};

#ifndef _DEBUG  // debug version in FamiTrackerView.cpp
inline CFamiTrackerDoc* CFamiTrackerView::GetDocument() const
   { return reinterpret_cast<CFamiTrackerDoc*>(m_pDocument); }
#endif

