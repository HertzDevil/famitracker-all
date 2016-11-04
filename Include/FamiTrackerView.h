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

const struct {
	const static int BACKGROUND			= 0x00000000;		// Background color
	const static int BACKGROUND_HILITE	= 0x00000000;		// Background color
	const static int TEXT_NORMAL		= 0x0000FF00;		// Normal text color
	const static int TEXT_HILITE		= 0x0000F0F0;		// Highlighted text color
	const static int TEXT_MUTED			= 0x000000FF;		// Channel muted color
	const static int TEXT_INSTRUMENT	= 0x0080FF80;
	const static int TEXT_VOLUME		= 0x00FF8080;
	const static int TEXT_EFFECT		= 0x008080FF;
	const static int BORDER_NORMAL		= 0x006671F4;		// Normal border, inactive
	const static int BORDER_EDIT		= 0x00000080;		// Edit border, inactive
	const static int BORDER_NORMAL_A	= 0x00C02020;		// Normal border, active
	const static int BORDER_EDIT_A		= 0x000000FF;		// Edit border, active
	const static int SELECTION			= 0x00FF0000;
	const static int CURSOR				= 0x000000FF;
} COLOR_SCHEME;

const unsigned int DEFAULT_TEMPO_NTSC	= 150;
const unsigned int DEFAULT_TEMPO_PAL	= 125;
const unsigned int DEFAULT_SPEED		= 6;

const static char *FONT_FACE = "Fixedsys";

const int UNDO_LEVELS = 30;

enum {PASTE_MODE_NORMAL, PASTE_MODE_OVERWRITE, PASTE_MODE_MIX};

enum eCOLUMNS {C_NOTE, 
			   C_INSTRUMENT1, C_INSTRUMENT2, 
			   C_VOLUME, 
			   C_EFF_NUM, C_EFF_PARAM1, C_EFF_PARAM2,
			   C_EFF2_NUM, C_EFF2_PARAM1, C_EFF2_PARAM2, 
			   C_EFF3_NUM, C_EFF3_PARAM1, C_EFF3_PARAM2, 
			   C_EFF4_NUM, C_EFF4_PARAM1, C_EFF4_PARAM2};

enum {
	CMD_STEP_DOWN = 1,
	CMD_HALT,
	CMD_JUMP_TO,
	CMD_SKIP_TO,
	CMD_MOVE_TO_TOP,
	CMD_IS_PLAYING,
	CMD_UPDATE_ROW,
	CMD_GET_TEMPO,
	CMD_GET_SPEED,
	CMD_TIME,
	CMD_TICK,
	CMD_BEGIN,
	CMD_GET_FRAME,
	CMD_MOVE_TO_START,
};

struct stUndoBlock {
	stChanNote ChannelData[MAX_PATTERN_LENGTH];
	int Pattern;
	int Channel;
	int Row, Column;
	int Frame;
};

class CSelectPoint {
public:
	unsigned int m_iChanStart, m_iChanEnd;
	unsigned int m_iColStart, m_iColEnd;
	unsigned int m_iRowStart, m_iRowEnd;
private:
};

const unsigned int COLUMNS = 7;

class CSoundGen;

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
	void				SetSoundGen(CSoundGen* pSoundGen) { m_pSoundGenerator = pSoundGen; };
	void				SetInstrument(int Instrument);
	unsigned int		GetInstrument() const { return m_iInstrument; };
	unsigned int		GetOctave() const { return m_iOctave; };
	void				SetOctave(unsigned int iOctave);
	void				SetFollowMode(bool Mode);
	void				CreateFont();

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

	int					GetCurrentChannelType();

	void				MoveCursor(int Step);

	// Settings
	unsigned int		GetStepping() const { return m_iRealKeyStepping; };	
	void				SetStepping(int Step) { m_iRealKeyStepping = Step; if (Step > 0) m_iKeyStepping = Step; else m_iKeyStepping = 1; };

	void				SetChangeAllPattern(bool ChangeAll) { m_bChangeAllPattern = ChangeAll; };

	// Document editing functions
	void				IncreaseCurrentPattern();
	void				DecreaseCurrentPattern();

	// Player
	int					PlayerCommand(char Command, int Value);
	void 				GetRow(CFamiTrackerDoc *pDoc);

	stChanNote			GetNoteSynced(int Channel);

	// Note preview
	void				PreviewNote(unsigned char Key);
	void				PreviewRelease(unsigned char Key);

	bool				SwitchToInstrument() { return m_bSwitchToInstrument; };
	void				SwitchToInstrument(bool Switch) { m_bSwitchToInstrument = Switch; };

	int					GetPlayFrame() { return m_iPlayFrame; };
	bool				GetFollowMode() { return m_bFollowMode; };

protected:
	// Drawing functions
	void				DrawChar(int x, int y, char c, int Color, CDC *dc);
	void				DrawLine(int Offset, stChanNote *NoteData, int Line, int Column, int Channel, int Color, bool bHighlight, bool bShaded, CDC *dc);
	void				DrawMeters(CDC *pDC);
	void				PrepareBackground(CDC *dc);
	void				DrawPatternField(CDC *dc);
	void				DrawChannelHeaders(CDC *pDC);
	void				DrawPatternRow(CDC *pDC, unsigned int Row, unsigned int Line, unsigned int Frame, bool Preview);
	void				CacheBackground(CDC *pDC);
	void				RestoreBackground(CDC *pDC);
	void				CreateBackground();

	void				FastRedraw();

	unsigned int		GetChannelAtPoint(unsigned int PointX);
	unsigned int		GetColumnAtPoint(unsigned int PointX, unsigned int MaxColumns);
	int					GetRowAtPoint(unsigned int PointY);

	void				ClickChannelBar(unsigned int Channel, unsigned int Column);

	// General
	void				StepDown();
	void				AddUndo(unsigned int Channel);
	void				SelectWholePattern(unsigned int Channel);

	unsigned int		GetCurrentColumnCount() const { return COLUMNS + GetDocument()->GetEffColumns(m_iCursorChannel) * 3; };
	void				ScanActualLengths();
	unsigned int		GetCurrentPatternLength();

	// Player
	void				GetStartSpeed();

	// Input handling
	void				HandleKeyboardInput(char Key);
	void				TranslateMidiMessage();
	void				CheckSelectionBeforeMove();
	void				CheckSelectionAfterMove();
	void				RemoveWithoutDelete();

	// MIDI note functions
	void				TriggerMIDINote(unsigned int Channel, unsigned int MidiNote, unsigned int Velocity, bool Insert);
	void				ReleaseMIDINote(unsigned int Channel, unsigned int MidiNote, bool InsertCut);
	void				CutMIDINote(unsigned int Channel, unsigned int MidiNote, bool InsertCut);

	// Selection
	unsigned int		GetSelectRowStart() const;
	unsigned int		GetSelectRowEnd() const;
	unsigned int		GetSelectColStart() const;
	unsigned int		GetSelectColEnd() const;

	void				CheckSelectionStart(CPoint point);
	void				CheckSelectionChange(CPoint point);

//
// Private functions
//
private:
	// General
	void				SoloChannel(unsigned int Channel);
	void				ToggleChannel(unsigned int Channel);
	bool				IsChannelSolo(unsigned int Channel);

	// Input handling
	void				KeyIncreaseAction();
	void				KeyDecreaseAction();
	int					TranslateKey(unsigned char Key);
	int					TranslateKey2(unsigned char Key);
	int					TranslateKeyAzerty(unsigned char Key);
	bool				CheckHaltKey(unsigned char Key);
	bool				CheckReleaseKey(unsigned char Key);
	bool				PreventRepeat(unsigned char Key, bool Insert);
	void				RepeatRelease(unsigned char Key);
	bool				EditInstrumentColumn(stChanNote &Note, int Value);
	bool				EditVolumeColumn(stChanNote &Note, int Value);
	bool				EditEffNumberColumn(stChanNote &Note, unsigned char nChar, int EffectIndex);
	bool				EditEffParamColumn(stChanNote &Note, int Value, int EffectIndex);
	void				InsertEffect(char Effect);

	// Keys
	void				OnKeyHome();

	// MIDI keyboard emulation
	void				HandleKeyboardNote(char nChar, bool Pressed);

	// Note handling
	void				PlayNote(unsigned int Channel, unsigned int Note, unsigned int Octave, unsigned int Velocity);
	void				HaltNote(unsigned int Channel, bool HardHalt);
	
	void				UpdateArpDisplay();

//
// View variables
//
protected:
	// General
	bool				m_bInitialized;
	bool				m_bHasFocus;
	UINT				m_iClipBoard;
	unsigned int		m_iActualLengths[MAX_FRAMES];
	unsigned int		m_iNextPreviewFrame[MAX_FRAMES];

	// Cursor & editing
	unsigned int		m_iCurrentFrame;											// Frame displayed on screen
	unsigned int		m_iCurrentRow;												// Middle row in pattern field
	unsigned int		m_iPlayRow;													// Current row for playback
	unsigned int		m_iPlayFrame;												// Current frame for playback
	unsigned int		m_iCursorChannel, m_iCursorColumn;							// Selected channel-part (note, instrument or effect)
	unsigned int		m_iLastRowState, m_iLastFrameState, m_iLastCursorColumn;	// tracks screen refreshing
	unsigned int		m_iKeyStepping;												// Numbers of rows to jump when moving
	unsigned int		m_iRealKeyStepping;
	int					m_iCursorRow;												// Should be signed
	bool				m_bChangeAllPattern;										// All pattern will change
	int					m_iPasteMode;
	bool				m_bEditEnable;												// Edit is enabled
	bool				m_bShiftPressed, m_bControlPressed;							// Shift and control keys
	unsigned int		m_iInstrument;												// Selected instrument
	unsigned int		m_iOctave;													// Selected octave
	bool				m_bSwitchToInstrument;
	bool				m_bFollowMode;												// Follow mode, default true
	
	// Playing
	bool				m_bMuteChannels[MAX_CHANNELS];
	unsigned int		m_iPlayTime;
	unsigned int		m_iTickPeriod;
	unsigned int		m_iTempo, m_iSpeed;					// Tempo and speed

	char				m_iAutoArpNotes[128];
	int					m_iAutoArpPtr, m_iLastAutoArpPtr;
	int					m_iAutoArpKeyCount;

	// Window size
	unsigned int		m_iWindowWidth, m_iWindowHeight;
	unsigned int		m_iVisibleRows;

	// Selection
	unsigned int		m_iSelectChanStart, m_iSelectChanEnd;
	unsigned int		m_iSelectRowStart, m_iSelectRowEnd;
	unsigned int		m_iSelectColStart, m_iSelectColEnd;
	bool				m_bSelectionActive;

	// Drawing
	bool				m_bForceRedraw;						// Draw the thing
	unsigned int		m_iStartChan;						// The first drawn channel
	unsigned int		m_iChannelsVisible;
	unsigned int		m_iChannelWidths[MAX_CHANNELS];
	unsigned int		m_iChannelColumns[MAX_CHANNELS];
	unsigned int		m_iVolLevels[MAX_CHANNELS];
	CDC					*m_pBackDC;
	CBitmap				m_bmpCache, *m_pOldCacheBmp;
	bool				m_bUpdateBackground;
	CFont				m_PatternFont;
	CFont				m_HeadFont;
	int					m_iChanHeadFadeCol[13];
	int					m_iRowHighlight;

//	CFontDrawer			m_FontDrawer;

	// Undo
	unsigned int		m_iUndoLevel, m_iRedoLevel;
	stUndoBlock			m_UndoStack[UNDO_LEVELS + 1];

	// Input
	char				m_cKeyList[256];
	bool				m_bMaskInstrument;

	// Unsorted
	bool				IgnoreFirst;

	unsigned int		m_iKeyboardNote;
	unsigned int		m_iSoloChannel;

	CSoundGen			*m_pSoundGenerator;

	// MIDI
	unsigned int		m_iLastMIDINote;
	unsigned int		m_iActiveNotes[MAX_CHANNELS];

// ---------------------------
// These below will be removed
// ---------------------------

// Operations
public:
	void	RegisterKeyState(int Channel, int Note);

	// change this
	stChanNote	CurrentNotes[MAX_CHANNELS];
	bool		NewNoteData[MAX_CHANNELS];
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
public:
	void ForceRedraw();

	void InsertNote(int Note, int Octave, int Channel, int Velocity);
	void StopNote(int Channel);
	void RefreshStatusMessage();

	void FeedNote(int Channel, stChanNote *NoteData);

protected:
	virtual void CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType = adjustBorder);
	virtual void OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/);
	virtual void PostNcDestroy();

public:
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
	afx_msg void OnEditGradient();
	afx_msg void OnNextInstrument();
	afx_msg void OnPrevInstrument();
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};

#ifndef _DEBUG  // debug version in FamiTrackerView.cpp
inline CFamiTrackerDoc* CFamiTrackerView::GetDocument() const
   { return reinterpret_cast<CFamiTrackerDoc*>(m_pDocument); }
#endif

