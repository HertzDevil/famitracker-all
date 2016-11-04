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

// The idea is to move big parts of CFamiTrackerView to this class

#pragma once

#include "FamiTrackerDoc.h"
#include "SoundGen.h"

enum {TRANSPOSE_DEC_NOTES, TRANSPOSE_INC_NOTES, TRANSPOSE_DEC_OCTAVES, TRANSPOSE_INC_OCTAVES};

// Levels of undo in the editor. Higher value requires more memory
const int UNDO_LEVELS = 30;

// Graphical layout of pattern editor

// Top header (channel names etc)
const int HEADER_HEIGHT = 36 /*+ 16*/;
const int HEADER_CHAN_START = /*16*/ 0;
const int HEADER_CHAN_HEIGHT = 36;
// Row column
const int ROW_COL_WIDTH = 32;
const int ROW_HEIGHT = 12;
// Patterns
const int CHANNEL_WIDTH = 107;
const int COLUMN_SPACING = 4;
const int CHAR_WIDTH = 10;

struct stClipData {
	int	Channels;		// Number of channels copied
	int Rows;			// Number of rows copied
	int StartColumn;	// Start column in first channel
	int EndColumn;		// End column in last channel
	stChanNote Pattern[MAX_CHANNELS][MAX_PATTERN_LENGTH];
};

struct stUndoBlock {
	stChanNote ChannelData[MAX_CHANNELS][MAX_PATTERN_LENGTH];
	int Patterns[MAX_CHANNELS];
	int Track;
	int Channel;
	int Row, Column;
	int Frame;
	int PatternLen;
};

class CPatternView {
public:
	bool InitView(UINT ClipBoard);
	void CreateFonts();
	void SetDocument(CFamiTrackerDoc *pDoc, CFamiTrackerView *pView);
	void SetWindowSize(int width, int height);

	void Invalidate(bool bEntire);
	void Modified();
	void AdjustCursor();
	void AdjustCursorChannel();
	bool FullErase() const;

	void DrawScreen(CDC *pDC, CFamiTrackerView *pView);
	void CreateBackground(CDC *pDC, bool bForce);
	void UpdateScreen(CDC *pDC);
	void DrawHeader(CDC *pDC);
	void DrawMeters(CDC *pDC);
	void FastScrollDown(CDC *pDC);

	void SetDPCMState(stDPCMState State);

	void SetFocus(bool bFocus);

	void PaintEditor();
	void EraseBackground(CDC *pDC);
	CRect GetActiveRect();

	// Cursor
	void MoveDown(int Step);
	void MoveUp(int Step);
	void MoveLeft();
	void MoveRight();
	void MoveToTop();
	void MoveToBottom();
	void MovePageUp();
	void MovePageDown();
	void MoveToRow(int Row);
	void MoveToFrame(int Frame);
	void MoveToChannel(int Channel);
	void MoveToColumn(int Column);
	void NextFrame();
	void PreviousFrame();
	void NextChannel();
	void PreviousChannel();
	void FirstChannel();
	void LastChannel();

	bool StepRow();
	bool StepFrame();
	void JumpToRow(int Row);
	void JumpToFrame(int Frame);

	int GetFrame() const;
	int GetChannel() const;
	int GetRow() const;
	int GetColumn() const;
	int GetPlayFrame() const;
	int GetPlayRow() const;

	// Mouse
	void OnMouseDown(CPoint point);
	void OnMouseUp(CPoint point);
	bool OnMouseHover(UINT nFlags, CPoint point);
	bool OnMouseNcMove();
	void OnMouseMove(UINT nFlags, CPoint point);
	void OnMouseDblClk(CPoint point);
	void OnMouseScroll(int Delta);
	void OnMouseRDown(CPoint point);

	bool CancelDragging();

	// Edit: Copy & paste, selection
	void Copy(stClipData *pClipData);
	void Cut();
	void Paste(stClipData *pClipData);
	void PasteMix(stClipData *pClipData);
	void Delete();
	void RemoveSelectedNotes();

	bool IsSelecting() const;
	void SelectAllChannel();
	void SelectAll();

	void Interpolate();
	void Reverse();
	void ReplaceInstrument(int Instrument);

	void AddUndo();
	void Undo();
	void Redo();
	bool CanUndo();
	bool CanRedo();

	void ClearSelection();

	// Various
	void Transpose(int Type);
	void ScrollValues(int Type);

	// Keys
	void ShiftPressed(bool Pressed);
	void ControlPressed(bool Pressed);

	void SetHighlight(int Rows, int SecondRows);
	void SetFollowMove(bool bEnable);

	bool IsPlayCursorVisible();

	CString GetMMLString();

	int GetChannelAtPoint(int PointX);

	bool ScrollTimer();

private:
	int GetRowAtPoint(int PointY);
	int GetColumnAtPoint(int PointX);
	bool IsColumnSelected(int Column, int Channel);
	int GetSelectColumn(int Column);
	bool IsSingleChannelSelection();

	void ClearRow(CDC *pDC, int Line);
	void DrawRows(CDC *pDC);
	void DrawRow(CDC *pDC, int Row, int Line, int Frame, bool bPreview);

	void DrawCell(int PosX, int Column, int Channel, bool bHighlight, bool bHighlight2, bool bShaded, bool bInvert, unsigned int BackColor, stChanNote *NoteData, CDC *dc);

	void DrawChar(int x, int y, char c, int Color, CDC *pDC);

	void DrawNoteColumn(unsigned int PosX, unsigned int PosY, CDC *pDC);
	void DrawInstrumentColumn(unsigned int PosX, unsigned int PosY, CDC *pDC);

	void UpdateVerticalScroll();
	void UpdateHorizontalScroll();

	// Head
	void DrawChannelNames(CDC *pDC);

	void DrawUnbufferedArea(CDC *pDC);

	// Selection routines
	bool IsWithinSelection(int Col, int Channel, int Row) const;
	int GetSelectRowStart() const; 
	int GetSelectRowEnd() const;
	int GetSelectColStart() const; 
	int GetSelectColEnd() const;
	int GetSelectChanStart() const;
	int GetSelectChanEnd() const;

	void UpdateSelection();

	// Other
	int GetCurrentPatternLength(int Frame);

	stUndoBlock *SaveUndoState();
	void RestoreState(stUndoBlock *pUndoBlock);

private:
	CFamiTrackerDoc *m_pDocument;
	CFamiTrackerView *m_pView;

	UINT m_iClipBoard;

	// Window
	int m_iWinWidth, m_iWinHeight;
	int	m_iVisibleRows;

	// Cursor
	int	m_iCursorRow;			// The row the cursor is on
	int m_iMiddleRow;			// The row in the middle of the editor
	int m_iCursorChannel;
	int m_iCursorColumn;

	int	m_iCurrentFrame;
	int m_iPatternLength;
	int	m_iPrevPatternLength;
	int	m_iNextPatternLength;
	int m_iPlayPatternLength;

	int m_iChannelWidths[MAX_CHANNELS];
	int m_iColumns[MAX_CHANNELS];

	int m_iPlayRow;
	int m_iPlayFrame;

	bool m_bFollowMode;

	// Drawing
	int m_iDrawCursorRow;
	int m_iDrawMiddleRow;
	int m_iDrawFrame;
	int m_iFirstChannel;
	int m_iChannelsVisible;
	int m_iHighlight;
	int m_iHighlightSecond;
	int m_iPatternWidth;
	int m_iVisibleWidth;

	bool m_bForceFullRedraw;
	bool m_bDrawEntire;

	int	m_iActualLengths[MAX_FRAMES];
	int	m_iNextPreviewFrame[MAX_FRAMES];

	bool m_bHasFocus;

	bool m_bUpdated;

	// Colors
	unsigned int m_iColEmptyBg;

	// Meters and DPCM
	stDPCMState m_DPCMState;

	// Selection
	bool m_bSelecting;
	bool m_bDragStart;
	bool m_bDragging;
	int	m_iSelStartRow, m_iSelEndRow;
	int	m_iSelStartChannel, m_iSelEndChannel;
	int	m_iSelStartColumn, m_iSelEndColumn;

	int m_iDragRowStart, m_iDragRowEnd;
	int m_iDragChanStart, m_iDragChanEnd;
	int m_iDragColStart, m_iDragColEnd;
	int m_iDragPointRow, m_iDragPointChan, m_iDragPointCol;

	CPoint SelStartPoint;

	// Keys
	bool m_bShiftPressed;
	bool m_bControlPressed;

	// Undo
	int m_iUndoLevel, m_iRedoLevel;
	stUndoBlock *m_pUndoStack[UNDO_LEVELS + 1];

	// GDI objects
	CDC		*m_pBackDC;
	CBitmap	m_bmpCache, *m_pOldCacheBmp;
	CFont	HeaderFont;
	CFont	PatternFont;

	// Scrolling
	CPoint m_ScrollMousePos;
	UINT m_nScrollFlags;
	int m_iScrolling;

};
