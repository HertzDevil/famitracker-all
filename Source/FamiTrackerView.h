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

#include <afxmt.h>

// Paste modes
enum {PASTE_MODE_NORMAL, PASTE_MODE_OVERWRITE, PASTE_MODE_MIX};

// Tracker playing interface commands
enum {
	CMD_STEP_DOWN = 1,
	CMD_HALT,
	CMD_JUMP_TO,
	CMD_SKIP_TO,
	CMD_MOVE_TO_TOP,
	CMD_IS_PLAYING,
	CMD_READ_ROW,
	CMD_GET_TEMPO,
	CMD_GET_SPEED,
	CMD_TIME,
	CMD_TICK,
	CMD_BEGIN,
	CMD_GET_FRAME,
	CMD_MOVE_TO_START,
	CMD_MOVE_TO_CURSOR
};

// Custom window messages
enum {MSG_UPDATE = WM_USER, MSG_MIDI_EVENT = WM_USER + 1};

// External classes
class CFamiTrackerDoc;
class CPatternView;
class CFrameBoxWnd;

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

	// Instruments
	void		 SetInstrument(int Instrument);
	unsigned int GetInstrument() const;
	bool		 SwitchToInstrument() const { return m_bSwitchToInstrument; };
	void		 SwitchToInstrument(bool Switch) { m_bSwitchToInstrument = Switch; };

	// Scrolling/viewing no-editing functions
	void		 MoveCursorNextChannel();
	void		 MoveCursorPrevChannel();

	void		 SelectNextFrame();
	void		 SelectPrevFrame();
	void		 SelectFirstFrame();
	void		 SelectLastFrame();
	void		 SelectFrame(unsigned int Frame);
	void		 SelectChannel(unsigned int Channel);
	 
	unsigned int GetSelectedFrame() const;
	unsigned int GetSelectedChannel() const;
	unsigned int GetPlayFrame() const;
	bool		 GetFollowMode() const;
	int			 GetSelectedChipType() const;
	unsigned int GetOctave() const { return m_iOctave; };
	bool		 GetEditMode() const { return m_bEditEnable; };

	void		 SetFrameQueue(int Frame);
	int			 GetFrameQueue() const;

	// Settings
	unsigned int GetStepping() const { return m_iRealKeyStepping; };	
	void		 SetStepping(int Step);
	void		 SetChangeAllPattern(bool ChangeAll) { m_bChangeAllPattern = ChangeAll; };
	bool		 ChangeAllPatterns() { return m_bChangeAllPattern; };
	void		 SetFollowMode(bool Mode);
	void		 SetOctave(unsigned int iOctave);

	// Document editing functions
	void		 IncreaseCurrentPattern();
	void		 DecreaseCurrentPattern();

	// Player
	int			 PlayerCommand(char Command, int Value);
	void 		 GetRow(CFamiTrackerDoc *pDoc);
	void		 MakeSilent();
	void		 RegisterKeyState(int Channel, int Note);
	void		 FeedNote(int Channel, stChanNote *NoteData);

	// Note preview
	void		 PreviewNote(unsigned char Key);
	void		 PreviewRelease(unsigned char Key);

	// General
	void		 SoloChannel(unsigned int Channel);
	void		 ToggleChannel(unsigned int Channel);
	bool		 IsChannelSolo(unsigned int Channel) const;
	bool		 IsChannelMuted(unsigned int Channel) const;

	// Drawing
	void		 UpdateEditor(LPARAM lHint);

	// Keys
	bool		 IsControlPressed() const;

	// For UI updates
	bool		 IsSelecting() const;
	bool		 IsClipboardAvailable() const;

	void		 SetupColors();

	void		 DrawFrameWindow();

	bool	DoRelease() const;

protected:
	// General
	void	StepDown();

	// Input handling
	void	HandleKeyboardInput(char Key);
	void	TranslateMidiMessage();
	void	RemoveWithoutDelete();
	void	DeleteKey();
	void	InsertKey();
	void	BackKey();
	void	OnKeyHome();

	// MIDI note functions
	void	TriggerMIDINote(unsigned int Channel, unsigned int MidiNote, unsigned int Velocity, bool Insert);
	void	ReleaseMIDINote(unsigned int Channel, unsigned int MidiNote, bool InsertCut);
	void	CutMIDINote(unsigned int Channel, unsigned int MidiNote, bool InsertCut);

//
// Private functions
//
private:

	// Input handling
	void	KeyIncreaseAction();
	void	KeyDecreaseAction();
	int		TranslateKey(unsigned char Key);
	int		TranslateKey2(unsigned char Key);
	int		TranslateKeyAzerty(unsigned char Key);
	bool	CheckClearKey(unsigned char Key);
	bool	CheckHaltKey(unsigned char Key);
	bool	CheckReleaseKey(unsigned char Key);
	bool	CheckRepeatKey(unsigned char Key);
	bool	PreventRepeat(unsigned char Key, bool Insert);
	void	RepeatRelease(unsigned char Key);
	bool	EditInstrumentColumn(stChanNote &Note, int Value);
	bool	EditVolumeColumn(stChanNote &Note, int Value);
	bool	EditEffNumberColumn(stChanNote &Note, unsigned char nChar, int EffectIndex);
	bool	EditEffParamColumn(stChanNote &Note, int Value, int EffectIndex);
	void	InsertEffect(char Effect);
	void	InsertNote(int Note, int Octave, int Channel, int Velocity);

	// MIDI keyboard emulation
	void	HandleKeyboardNote(char nChar, bool Pressed);

	// Note handling
	void	PlayNote(unsigned int Channel, unsigned int Note, unsigned int Octave, unsigned int Velocity);
	void	ReleaseNote(unsigned int Channel);
	void	HaltNote(unsigned int Channel);
	
	void	UpdateArpDisplay();
	
//
// Constants
//
public:
	static const TCHAR CLIPBOARD_ID[];

//
// View variables
//
protected:
	// General
	bool				m_bHasFocus;
	UINT				m_iClipBoard;
	int					m_iMenuChannel;

	// Cursor & editing
	unsigned int		m_iKeyStepping;							// Numbers of rows to jump when moving
	unsigned int		m_iRealKeyStepping;
	unsigned int		m_iInstrument;							// Selected instrument
	unsigned int		m_iOctave;								// Selected octave	
	bool				m_bEditEnable;							// Edit is enabled
	bool				m_bSwitchToInstrument;
	bool				m_bFollowMode;							// Follow mode, default true
	bool				m_bShiftPressed, m_bControlPressed;		// Shift and control keys
	bool				m_bChangeAllPattern;					// All pattern will change
	bool				m_bMaskInstrument;
	int					m_iPasteMode;

	// Playing
	bool				m_bMuteChannels[MAX_CHANNELS];
	unsigned int		m_iPlayTime;
	int					m_iFrameQueue;
	int					m_iSwitchToInstrument;

	// Auto arpeggio
	char				m_iAutoArpNotes[128];
	int					m_iAutoArpPtr, m_iLastAutoArpPtr;
	int					m_iAutoArpKeyCount;

	// Window size
	unsigned int		m_iWindowWidth, m_iWindowHeight;

	// Drawing
	bool				m_bForceRedraw;
	bool				m_bUpdateBackground;

	// Input
	char				m_cKeyList[256];
	unsigned int		m_iKeyboardNote;
	int					m_iLastNote;
	int					m_iLastInstrument, m_iLastVolume;
	int					m_iLastEffect, m_iLastEffectParam;

	// MIDI
	unsigned int		m_iLastMIDINote;
	unsigned int		m_iActiveNotes[MAX_CHANNELS];

	// Drawing
	CPatternView		*m_pPatternView;
	CFrameBoxWnd		*m_pFrameBoxWnd;

	// Synchronization
	CCriticalSection	m_csDrawLock;

// ---------------------------
// TODO: Remove these below
// ---------------------------

// Operations
public:

	// change this
	unsigned int Arpeggiate[MAX_CHANNELS];

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

protected:
	virtual void CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType = adjustBorder);
	virtual void OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/);

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);

	afx_msg void OnEditUndo();
	afx_msg void OnEditCopy();
	afx_msg void OnEditCut();
	afx_msg void OnEditPaste();
	afx_msg void OnEditDelete();
	afx_msg void OnEditInstrumentMask();
	afx_msg void OnEditPasteoverwrite();

	afx_msg void OnTrackerEdit();
	afx_msg void OnTrackerPal();
	afx_msg void OnTrackerNtsc();
	afx_msg void OnSpeedCustom();
	afx_msg void OnSpeedDefault();
	afx_msg void OnTransposeDecreasenote();
	afx_msg void OnTransposeDecreaseoctave();
	afx_msg void OnTransposeIncreasenote();
	afx_msg void OnTransposeIncreaseoctave();

	afx_msg void OnUpdateEditCopy(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditCut(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditPaste(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditDelete(CCmdUI *pCmdUI);
	afx_msg void OnUpdateTrackerEdit(CCmdUI *pCmdUI);
	afx_msg void OnUpdateTrackerPal(CCmdUI *pCmdUI);
	afx_msg void OnUpdateTrackerNtsc(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSpeedDefault(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSpeedCustom(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditPasteoverwrite(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditInstrumentMask(CCmdUI *pCmdUI);

	afx_msg void OnIncreaseStepSize();
	afx_msg void OnDecreaseStepSize();

	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
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
	afx_msg void OnFrameInsert();
	afx_msg void OnFrameRemove();
	afx_msg void OnTrackerPlayrow();
	afx_msg void OnUpdateFrameRemove(CCmdUI *pCmdUI);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnEditPastemix();
	afx_msg void OnUpdateEditPastemix(CCmdUI *pCmdUI);
	afx_msg void OnModuleMoveframedown();
	afx_msg void OnModuleMoveframeup();
	afx_msg void OnUpdateModuleMoveframedown(CCmdUI *pCmdUI);
	afx_msg void OnUpdateModuleMoveframeup(CCmdUI *pCmdUI);
	afx_msg void OnTrackerToggleChannel();
	afx_msg void OnTrackerSoloChannel();
	afx_msg void OnNextOctave();
	afx_msg void OnPreviousOctave();
	afx_msg void OnPasteOverwrite();
	afx_msg void OnPasteMixed();
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnEditInterpolate();
	afx_msg void OnEditReplaceInstrument();
	afx_msg void OnEditReverse();
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg void OnOneStepUp();
	afx_msg void OnOneStepDown();
	afx_msg void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnBlockStart();
	afx_msg void OnBlockEnd();
	afx_msg void OnPickupRow();
};

#ifndef _DEBUG  // debug version in FamiTrackerView.cpp
inline CFamiTrackerDoc* CFamiTrackerView::GetDocument() const
   { return reinterpret_cast<CFamiTrackerDoc*>(m_pDocument); }
#endif

