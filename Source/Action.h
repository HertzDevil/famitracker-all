/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2012  Jonathan Liss
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

//
// Change MAX_LEVELS in the class CActionHandler if you want more undo levels
//

// Base class for action commands
class CAction
{
public:
	CAction(int iAction);
	virtual ~CAction();

	virtual bool SaveState(CMainFrame *pMainFrm) = 0;
	virtual void Undo(CMainFrame *pMainFrm) = 0;
	virtual void Redo(CMainFrame *pMainFrm) = 0;

	int GetAction() const;

protected:
	int m_iAction;
};

class CPatternView;
struct stClipData;

// Pattern commands
class CPatternAction : public CAction
{
public:
	enum ACTIONS
	{
		ACT_EDIT_NOTE,
		ACT_INSERT_ROW,
		ACT_DELETE_ROW,
		ACT_INCREASE,
		ACT_DECREASE,
		ACT_EDIT_PASTE,
		ACT_EDIT_DELETE,
		ACT_EDIT_DELETE_ROWS,
		ACT_TRANSPOSE,
		ACT_SCROLL_VALUES,
		ACT_INTERPOLATE,
		ACT_REVERSE,
		ACT_REPLACE_INSTRUMENT,
		ACT_DRAG_AND_DROP,
		ACT_PATTERN_LENGTH,
		ACT_EXPAND_PATTERN,
		ACT_SHRINK_PATTERN
	};

public:
	CPatternAction(int iAction);
	~CPatternAction();

	bool SaveState(CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm);
	void Redo(CMainFrame *pMainFrm);

public:
	void SetNote(stChanNote &Note);
	void SetDelete(bool PullUp, bool Back);
	void SetPaste(stClipData *pClipData, int Mode);
	void SetSelection(CSelection *pSelection);
	void SetTranspose(int Mode);
	void SetScroll(int Scroll);
	void SetInstrument(int Instrument);
	void SetDragAndDrop(stClipData *pClipData, bool bDelete, bool bMix, CSelection *pDragTarget);
	void SetPatternLength(int Length);
	void Update(CMainFrame *pMainFrm);

private:
	void SaveEntire(CPatternView *pPatternView);
	void RestoreEntire(CPatternView *pPatternView);
	void IncreaseRowAction(CFamiTrackerDoc *pDoc);
	void DecreaseRowAction(CFamiTrackerDoc *pDoc);

	void SaveSelection(CPatternView *pPatternView);
	void RestoreSelection(CPatternView *pPatternView);

private:
	stChanNote m_NewNote;
	stChanNote m_OldNote;

	int m_iUndoTrack;
	int m_iUndoFrame;
	int m_iUndoChannel;
	int m_iUndoRow;
	int m_iUndoColumn;
	int m_iUndoColumnCount;

	int m_iRedoTrack;
	int m_iRedoFrame;
	int m_iRedoChannel;
	int m_iRedoRow;
	int m_iRedoColumn;
	int m_iRedoColumnCount;

	int m_iPatternLen;
	int m_iActualPatternLen;

	int m_iNewPatternLen;
	int m_iOldPatternLen;

	bool m_bPullUp;
	bool m_bBack;

	int m_iPasteMode;
	stClipData *m_pClipData;
	stClipData *m_pUndoClipData;
	CSelection m_selection;

	int m_iTransposeMode;
	int m_iScrollValue;
	int m_iInstrument;

	bool m_bDragDelete;
	bool m_bDragMix;
	CSelection m_dragTarget;
};

// Frame commands
class CFrameAction : public CAction
{
public:
	enum ACTIONS
	{
		ACT_ADD,
		ACT_REMOVE,
		ACT_DUPLICATE,
		ACT_DUPLICATE_PATTERNS,
		ACT_CHANGE_COUNT,
		ACT_SET_PATTERN,
		ACT_SET_PATTERN_ALL,
		ACT_CHANGE_PATTERN,
		ACT_CHANGE_PATTERN_ALL,
		ACT_MOVE_DOWN,
		ACT_MOVE_UP,
		ACT_PASTE
	};

public:
	CFrameAction(int iAction);
	~CFrameAction();

	bool SaveState(CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm);
	void Redo(CMainFrame *pMainFrm);

public:
	void SetFrameCount(unsigned int FrameCount);
	void SetPattern(unsigned int Pattern);
	void SetPatternDelta(int Delta, bool ChangeAll);
	void Update(CMainFrame *pMainFrm);
	void SetPasteData(int *pData);

private:
	void SaveFrame(CFamiTrackerDoc *pDoc);
	void RestoreFrame(CFamiTrackerDoc *pDoc);

private:
	unsigned int m_iUndoFramePos;
	unsigned int m_iUndoChannelPos;
	unsigned int m_iRedoFramePos;
	unsigned int m_iRedoChannelPos;

	unsigned int m_iNewFrameCount;
	unsigned int m_iUndoFrameCount;
	unsigned int m_iNewPattern;
	unsigned int m_iOldPattern;
	
	int m_iPatternDelta;
	bool m_bChangeAll;

	int m_iPasteData[MAX_CHANNELS];

	unsigned int m_iPatterns[MAX_CHANNELS];

};

// Stores action objects (a dual-stack, kind of)
class CActionHandler
{
public:
	CActionHandler();
	~CActionHandler();

	void Clear();
	void Push(CAction *pAction);
	CAction *PopUndo();
	CAction *PopRedo();
	CAction *GetLastAction() const;
	int GetUndoLevel() const;
	int GetRedoLevel() const;
	bool CanUndo() const;
	bool CanRedo() const;

public:
	// Levels of undo in the editor. Higher value requires more memory
	static const int MAX_LEVELS = 64;

private:
	CAction *m_pActionStack[MAX_LEVELS + 1];
	int		m_iUndoLevel;
	int		m_iRedoLevel;

};

