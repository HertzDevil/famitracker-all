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

// This will be the new pattern editor

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerView.h"
#include "PatternView.h"
#include "FontDrawer.h"

const int SELECT_THRESHOLD = 5;

enum {COLUMN_NOTE, COLUMN_INSTRUMENT, COLUMN_VOLUME, COLUMN_EFF1, COLUMN_EFF2, COLUMN_EFF3, COLUMN_EFF4};

const unsigned int COLUMN_SPACE[] = {
	CHAR_WIDTH * 3 + COLUMN_SPACING,
	CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING, 
	CHAR_WIDTH + COLUMN_SPACING,  
	CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING,
	CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING,
	CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING,
	CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING
};

const unsigned int COLUMN_WIDTH[] = {
	CHAR_WIDTH * 3,
	CHAR_WIDTH, CHAR_WIDTH, 
	CHAR_WIDTH,
	CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH,
	CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH,
	CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH,
	CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH
};

const unsigned int SELECT_WIDTH[] = {
	CHAR_WIDTH * 3 + COLUMN_SPACING /*+ CHANNEL_SPACING*/,
	CHAR_WIDTH + COLUMN_SPACING, CHAR_WIDTH,
	CHAR_WIDTH + COLUMN_SPACING,  
	CHAR_WIDTH + COLUMN_SPACING, CHAR_WIDTH, CHAR_WIDTH /*+ COLUMN_SPACING*/,
	CHAR_WIDTH + COLUMN_SPACING, CHAR_WIDTH, CHAR_WIDTH /*+ COLUMN_SPACING*/,
	CHAR_WIDTH + COLUMN_SPACING, CHAR_WIDTH, CHAR_WIDTH /*+ COLUMN_SPACING*/,
	CHAR_WIDTH + COLUMN_SPACING, CHAR_WIDTH, CHAR_WIDTH /*+ COLUMN_SPACING*/
};

// Colors
const unsigned int COLOR_FG			= 0xE0E6E6;		// Foreground
const unsigned int COLOR_FG_LIGHT	= 0xFFFFFF;
const unsigned int COLOR_FG_DARK	= 0x808080;
const unsigned int COLOR_BG			= 0x404040;		// Background
const unsigned int COLOR_SEPARATOR	= 0x404040;		// Lines between channels

// Temporary, benchmarking

int m_iRedraws = 0, m_iFastRedraws = 0, m_iErases = 0, m_iBuffers = 0, m_iCharsDrawn;
CDC *m_pBackDC = NULL;
CBitmap *m_pBackBmp = NULL;

bool m_bSelectionInvalid;
bool m_bFullRowSelect;
bool m_bErasedBg;

int m_iCurrentHScrollPos;

int	m_iMouseHoverChan;
int m_iMouseHoverEffArrow;

void GradientRect(CDC *pDC, int x, int y, int w, int h, unsigned int c1, unsigned int c2, unsigned int c3)
{
	int range = /*y - y1*/ h;
	int p_range = range / 3;
	int i;

	int start = y, middle = y + p_range, end = y + h;

	for (i = start; i < middle; i++) {
		pDC->FillSolidRect(x, i, w, 1, DIM_TO(c2, c1, (i * 100) / p_range) );
	}

	for (i = middle; i < end; i++) {
		pDC->FillSolidRect(x, i, w, 1/*i*/, DIM_TO(c3, c2, ((i - p_range) * 100) / (range - p_range) ));
	}
}

int CPatternView::GetRowAtPoint(int PointY)
{
	return (PointY - HEADER_HEIGHT) / ROW_HEIGHT - (m_iVisibleRows / 2) + m_iMiddleRow;
}

int CPatternView::GetChannelAtPoint(int PointX)
{
	int i;
	int Offset = 0, StartOffset, Channels = m_pDocument->GetAvailableChannels();
	
	StartOffset = PointX - ROW_COL_WIDTH;
	
	for (i = m_iFirstChannel; i < Channels; i++) {
		Offset += m_iChannelWidths[i];
		if (StartOffset < Offset)
			return i;
	}

	return m_iFirstChannel + m_iChannelsVisible;
}

int CPatternView::GetColumnAtPoint(int PointX)
{
	const int EFFECT_WIDTH = (CHAR_WIDTH * 3 + COLUMN_SPACING);

	int i;
	int Offset;
	int ColumnOffset, Column = -1;
	int Channel = -1;
	int Channels = m_pDocument->GetAvailableChannels();

	Offset = COLUMN_SPACING - 3;

	int StartOffset = PointX - ROW_COL_WIDTH;

	if (StartOffset < 0)
		return -1;

	for (i = m_iFirstChannel; i < (signed)m_pDocument->GetAvailableChannels(); i++) {
		Offset += m_iChannelWidths[i];
		if (StartOffset < Offset) {
			ColumnOffset = m_iChannelWidths[i] - (Offset - StartOffset);
			Channel = i;
			break;
		}
	}

	if (Channel == -1)
		return -1;

	if (ColumnOffset > m_iChannelWidths[i])
		return -1;

	Offset = 0;

	for (i = 0; i < (signed)m_pDocument->GetEffColumns(Channel) * 3 + 7; i++) {
		Offset += COLUMN_SPACE[i];
		if (ColumnOffset < Offset) {
			Column = i;
			break;
		}
	}

	if (ColumnOffset > Offset)
		return -1;

	return Column;
}

bool CPatternView::IsColumnSelected(int Column, int Channel)
{
	int SelColStart = GetSelectColStart();
	int SelColEnd	= GetSelectColEnd();
	int SelStart, SelEnd;

	if (Channel > GetSelectChanStart() && Channel < GetSelectChanEnd())
		return true;

	// 0 = Note (0)
	// 1, 2 = Instrument (1)
	// 3 = Volume (2)
	// 4, 5, 6 = Effect 1 (3)
	// 7, 8, 9 = Effect 1 (4)
	// 10, 11, 12 = Effect 1 (5)
	// 13, 14, 15 = Effect 1 (6)

	switch (SelColStart) {
		case 0:						SelStart = 0; break;
		case 1: case 2:				SelStart = 1; break;
		case 3:						SelStart = 2; break;
		case 4: case 5: case 6:		SelStart = 3; break;
		case 7: case 8: case 9:		SelStart = 4; break;
		case 10: case 11: case 12:	SelStart = 5; break;
		case 13: case 14: case 15:	SelStart = 6; break;
	}

	switch (SelColEnd) {
		case 0:						SelEnd = 0; break;
		case 1: case 2:				SelEnd = 1; break;
		case 3:						SelEnd = 2; break;
		case 4: case 5: case 6:		SelEnd = 3; break;
		case 7: case 8: case 9:		SelEnd = 4; break;
		case 10: case 11: case 12:	SelEnd = 5; break;
		case 13: case 14: case 15:	SelEnd = 6; break;
	}

	
	if (Channel == GetSelectChanStart() && Channel == GetSelectChanEnd()) {
		if (Column >= SelStart && Column <= SelEnd)
			return true;
	}
	else if (Channel == GetSelectChanStart()) {
		if (Column >= SelStart)
			return true;
	}
	else if (Channel == GetSelectChanEnd()) {
		if (Column <= SelEnd)
			return true;		
	}

	return false;
}

int CPatternView::GetSelectColumn(int Column)
{
	// Translates cursor column to select column

	switch (Column) {
		case 0:	
			return COLUMN_NOTE;
		case 1: case 2:	
			return COLUMN_INSTRUMENT;
		case 3:	
			return COLUMN_VOLUME;
		case 4: case 5: case 6:	
			return COLUMN_EFF1;
		case 7: case 8: case 9:
			return COLUMN_EFF2;
		case 10: case 11: case 12:
			return COLUMN_EFF3;
		case 13: case 14: case 15:
			return COLUMN_EFF4;
	}

	return 0;
}

bool CPatternView::IsSingleChannelSelection()
{
	return m_iSelStartChannel == m_iSelEndChannel;
}

bool CPatternView::InitView(UINT ClipBoard)
{
	m_pBackDC = NULL;

	m_iClipBoard = ClipBoard;
	m_bShiftPressed = false;
	m_bControlPressed = false;
	m_bFollowMode = true;
	m_bForceFullRedraw = true;

	m_iHighlight = 4;
	m_iHighlightSecond = 16;

	for (int i = 0; i < UNDO_LEVELS; i++) {
		m_pUndoStack[i] = NULL;
	}

	Invalidate();

	return true;
}

void CPatternView::CreateFonts()
{
	LOGFONT LogFont;
	LPCSTR	FontName = theApp.m_pSettings->General.strFont;
	LPCSTR	HeaderFace = "Tahoma";

	// Create pattern font
	memset(&LogFont, 0, sizeof LOGFONT);
	memcpy(LogFont.lfFaceName, FontName, strlen(FontName));

	LogFont.lfHeight = -12;
	LogFont.lfQuality = DRAFT_QUALITY;
	LogFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

	PatternFont.DeleteObject();
	PatternFont.CreateFontIndirect(&LogFont);

	// Create header font
	memset(&LogFont, 0, sizeof LOGFONT);
	memcpy(LogFont.lfFaceName, HeaderFace, strlen(HeaderFace));

	LogFont.lfHeight = -11;
	//LogFont.lfWeight = 550;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;

	HeaderFont.DeleteObject();
	HeaderFont.CreateFontIndirect(&LogFont);
}

void CPatternView::SetDocument(CFamiTrackerDoc *pDoc, CFamiTrackerView *pView)
{
	m_pDocument = pDoc;
	m_pView = pView;

	// Reset variables

	m_iMiddleRow = 0;
	m_iCursorRow = 0;
	m_iCursorChannel = 0;
	m_iCursorColumn = 0;
	m_iCurrentFrame = 0;
	m_iFirstChannel = 0;

	m_iPlayFrame = 0;
	m_iPlayRow = 0;

	m_bSelecting = false;
	m_bDragging = false;
	m_bDragStart = false;
	m_iSelEndChannel = m_iSelStartChannel = 0;
	m_iSelEndColumn = m_iSelStartColumn = 0;
	m_iSelEndRow = m_iSelStartRow = 0;

	m_iUndoLevel = 0;
	m_iRedoLevel = 0;

	for (int i = 0; i < UNDO_LEVELS; i++) {
		if (m_pUndoStack[i]) {
			delete m_pUndoStack[i];
			m_pUndoStack[i] = NULL;
		}
	}

	Invalidate();

	// Quick fix
	//m_bIgnoreFirstClick = true;
}

void CPatternView::SetWindowSize(int width, int height)
{
	m_iWinWidth = width - GetSystemMetrics(SM_CXVSCROLL) - 4;
	m_iWinHeight = height - GetSystemMetrics(SM_CXHSCROLL) - 4;
	m_iVisibleRows = (m_iWinHeight - HEADER_HEIGHT /*- 4*/) / ROW_HEIGHT;
}

// Draw routines

void CPatternView::Invalidate()
{
	// Called when the pattern editor needs to be redrawn (and not just painted on screen)
	m_bUpdated = true;
}

void CPatternView::Modified()
{
	// Perform a full redraw
	m_bForceFullRedraw = true;
}

void CPatternView::AdjustCursor()
{
	// Adjust cursor and keep it inside the pattern area

	// Frame
	if ((unsigned)m_iCurrentFrame >= m_pDocument->GetFrameCount())
		m_iCurrentFrame = m_pDocument->GetFrameCount() - 1;

	// Follow play cursor
	if (m_bFollowMode && theApp.IsPlaying() /*&& !m_bSelecting*/) {
		m_iCursorRow = m_iPlayRow;
		m_iCurrentFrame = m_iPlayFrame;
	}

	m_iDrawFrame = m_iCurrentFrame;
	m_iPatternLength = GetCurrentPatternLength(m_iDrawFrame);

	if (m_iCursorRow > m_iPatternLength - 1)
		m_iCursorRow = m_iPatternLength - 1;

	if (theApp.m_pSettings->General.bFreeCursorEdit) {
		// Adjust if cursor is out of screen
		if (m_iMiddleRow < m_iVisibleRows / 2)
			m_iMiddleRow = m_iVisibleRows / 2;

		int CursorDifference = m_iCursorRow - m_iMiddleRow;

		// Bottom
		while (CursorDifference >= (m_iVisibleRows / 2) && CursorDifference > 0) {
			// Change these if you want one whole page to scroll instead of single lines
			m_iMiddleRow += 1;
			CursorDifference = (m_iCursorRow - m_iMiddleRow);
		}

		// Top
		while (-CursorDifference > (m_iVisibleRows / 2) && CursorDifference < 0) {
			m_iMiddleRow -= 1;
			CursorDifference = (m_iCursorRow - m_iMiddleRow);
		}
	}
	else {
		m_iMiddleRow = m_iCursorRow;
	}

	if (theApp.m_pSettings->General.bFramePreview) {
		m_iPrevPatternLength = GetCurrentPatternLength(m_iDrawFrame - 1);
		m_iNextPatternLength = GetCurrentPatternLength(m_iDrawFrame + 1);
	}
}

void CPatternView::AdjustCursorChannel()
{
	if (m_iFirstChannel > m_iCursorChannel)
		m_iFirstChannel = m_iCursorChannel;

	int ChannelCount = m_pDocument->GetAvailableChannels();
	int i, Offset;

	m_iChannelsVisible = ChannelCount - m_iFirstChannel;
	m_iVisibleWidth = 0;
	Offset = ROW_COL_WIDTH;
	for (i = m_iFirstChannel; i < ChannelCount; i++) {
		Offset += m_iChannelWidths[i];
		m_iVisibleWidth += m_iChannelWidths[i];
		if (Offset >= m_iWinWidth) {
			m_iChannelsVisible = i - m_iFirstChannel + 1;
			break;
		}
	}

	while ((m_iCursorChannel - m_iFirstChannel) > (m_iChannelsVisible - 1))
		m_iFirstChannel++;
}

void CPatternView::DrawScreen(CDC *pDC, CFamiTrackerView *pView)
{
	static int Redraws = 0;
	static bool WasSelecting;

#ifdef _DEBUG
	LARGE_INTEGER StartTime, EndTime;
	LARGE_INTEGER Freq;
#endif

	bool bFast = false;

	// Return if the back buffer isn't created
	if (!m_pBackDC)
		return;

//	m_TextRender.SetupFontPlane(pDC, &PatternFont);

	m_iCharsDrawn = 0;

	// Performance checking
#ifdef _DEBUG
	QueryPerformanceCounter(&StartTime);
#endif

	if (m_bUpdated) {
		AdjustCursor();

		// See if a fast redraw is possible
		if (m_iDrawCursorRow == (m_iCursorRow - 1) && !m_bSelecting && !WasSelecting && !theApp.m_pSettings->General.bFreeCursorEdit && !m_bErasedBg) {
			bFast = true;
		}

		if (m_bForceFullRedraw)
			bFast = false;

		// Fast scrolling down
		if (bFast) {

			// Todo: Fails when jumping and skipping

			m_iDrawCursorRow = m_iCursorRow;
			m_iDrawMiddleRow = m_iMiddleRow;
			FastScrollDown(m_pBackDC);

			m_pBackDC->SetWindowOrg(0, 0);

//			bFast = true;
			m_iFastRedraws++;
			//return;
		} 
		else {
			// Save these as those could change during drawing
			m_iDrawCursorRow = m_iCursorRow;
			m_iDrawMiddleRow = m_iMiddleRow;
			AdjustCursorChannel();
			PaintEditor();
			m_iRedraws++;
		}
	}

	m_bForceFullRedraw = false;
	m_bUpdated = false;

#ifdef WIP
	/*
	CString Wip;
	Wip.Format("WIP %i", VERSION_WIP);
	BackDC->SetBkColor(COLOR_SCHEME.CURSOR);
	BackDC->SetTextColor(COLOR_SCHEME.TEXT_HILITE);
	BackDC->TextOut(m_iWindowWidth - 47, m_iWindowHeight - 22, Wip);
	*/
#endif
#ifdef _DEBUG
	m_pBackDC->SetBkColor(COLOR_SCHEME.CURSOR);
	m_pBackDC->SetTextColor(COLOR_SCHEME.TEXT_HILITE);
	m_pBackDC->TextOut(m_iWinWidth - 47, 42, _T("DEBUG"));
	m_pBackDC->TextOut(m_iWinWidth - 47, 62, _T("DEBUG"));
	m_pBackDC->TextOut(m_iWinWidth - 47, 82, _T("DEBUG"));
#endif

	if (bFast)
		// Skip header
		pDC->BitBlt(0, HEADER_HEIGHT, /*m_iPatternWidth*/ m_iVisibleWidth + ROW_COL_WIDTH, m_iWinHeight - HEADER_HEIGHT, m_pBackDC, 0, HEADER_HEIGHT, SRCCOPY);
	else
		// Copy the back buffer to screen
		pDC->BitBlt(0, 0, /*m_iPatternWidth*/ m_iVisibleWidth + ROW_COL_WIDTH, m_iWinHeight, m_pBackDC, 0, 0, SRCCOPY);
//		UpdateScreen(pDC);

//	m_TextRender.DrawTextA(pDC, 60, 60, "TEST");

#ifdef _DEBUG

	QueryPerformanceCounter(&EndTime);
	QueryPerformanceFrequency(&Freq);

	CString Text;
	int PosY = 100;
	pDC->SetTextColor(0xFFFF);
	pDC->SetBkColor(0);
	Text.Format("%i ms", (__int64(EndTime.QuadPart) - __int64(StartTime.QuadPart)) / (__int64(Freq.QuadPart) / 1000));
	pDC->TextOutA(m_iWinWidth - 150, PosY, Text); PosY += 20;
	Text.Format("%i redraws", m_iRedraws);
	pDC->TextOutA(m_iWinWidth - 150, PosY, Text); PosY += 20;
	Text.Format("%i fast redraws", m_iFastRedraws);
	pDC->TextOutA(m_iWinWidth - 150, PosY, Text); PosY += 20;
	Text.Format("%i erases", m_iErases);
	pDC->TextOutA(m_iWinWidth - 150, PosY, Text); PosY += 20;
	Text.Format("%i new buffers", m_iBuffers);
	pDC->TextOutA(m_iWinWidth - 150, PosY, Text); PosY += 20;
	Text.Format("%i chars drawn", m_iCharsDrawn);
	pDC->TextOutA(m_iWinWidth - 150, PosY, Text); PosY += 20;
	Text.Format("%i rows visible", m_iVisibleRows);
	pDC->TextOutA(m_iWinWidth - 150, PosY, Text); PosY += 20;

	Text.Format("%i (%i) end sel", m_iSelEndChannel, m_pDocument->GetAvailableChannels());
	pDC->TextOutA(m_iWinWidth - 150, PosY, Text); PosY += 20;

#else

	/*
	CString Text;
	Text.Format("BETA");
	pDC->SetTextColor(0x00FFFF);
	pDC->SetBkMode(TRANSPARENT);
	pDC->TextOutA(m_iWinWidth - 50, m_iWinHeight - 30, Text);
	*/

#endif

	WasSelecting = m_bSelecting;
	m_bErasedBg = false;

	UpdateVerticalScroll();
	UpdateHorizontalScroll();
}

void CPatternView::UpdateScreen(CDC *pDC)
{
	// Copy the back buffer to screen
	pDC->BitBlt(0, 0, m_iPatternWidth + ROW_COL_WIDTH, m_iWinHeight, m_pBackDC, 0, 0, SRCCOPY);
}

CRect CPatternView::GetActiveRect()
{
	return CRect(0, 0, m_iPatternWidth + ROW_COL_WIDTH, m_iWinHeight);
}

// Called when the background is erased
void CPatternView::CreateBackground(CDC *pDC, bool bForce)
{
	static int LastWidth = 0;
	static int LastHeight = 0;
	static int LastPatternWidth = 0;

	int ColumnCount = 0, CurrentColumn = 0;

	m_iErases++;
	m_iPatternWidth = 0;

	// Calculate channel widths
	for (int i = 0; i < (signed)m_pDocument->GetAvailableChannels(); i++) {
		int Width = CHAR_WIDTH * 9 + COLUMN_SPACING * 4 + m_pDocument->GetEffColumns(i) * (3 * CHAR_WIDTH + COLUMN_SPACING);
		m_iChannelWidths[i] = Width + 1;
		m_iColumns[i] = 6 + m_pDocument->GetEffColumns(i) * 3;

		if (i == m_iCursorChannel) {
			CurrentColumn = ColumnCount + m_iCursorColumn;
			if (m_iCursorColumn > m_iColumns[i])
				m_iCursorColumn = m_iColumns[i];
		}

		ColumnCount += 7 + m_pDocument->GetEffColumns(i) * 3;

		m_iPatternWidth += Width + 1;
	}

	AdjustCursorChannel();

	m_bErasedBg = true;

	if (LastPatternWidth != m_iPatternWidth || LastWidth != m_iWinWidth || LastHeight != m_iWinHeight)
		bForce = true;

	// Allocate backbuffer area, only if window size or pattern width has changed
	if (bForce) {

		AdjustCursor();

		// Allocate backbuffer
		if (m_pBackBmp)
			delete m_pBackBmp;

		if (m_pBackDC)
			delete m_pBackDC;

		m_pBackBmp = new CBitmap;
		m_pBackDC = new CDC;

		// Setup dc
		m_pBackBmp->CreateCompatibleBitmap(pDC, m_iPatternWidth + ROW_COL_WIDTH, m_iWinHeight);
		m_pBackDC->CreateCompatibleDC(pDC);
		m_pBackDC->SelectObject(m_pBackBmp);

		EraseBackground(m_pBackDC);
		Invalidate();

		m_iBuffers++;

		LastWidth = m_iWinWidth;
		LastHeight = m_iWinHeight;
		LastPatternWidth = m_iPatternWidth;
	}

	// Draw directly on screen

#if 1

	// Channel header background
	pDC->FillSolidRect(m_iVisibleWidth + ROW_COL_WIDTH, 0, m_iWinWidth - m_iVisibleWidth - ROW_COL_WIDTH, HEADER_HEIGHT, COLOR_BG);
//	GradientRect(pDC, m_iVisibleWidth + ROW_COL_WIDTH, 0, m_iWinWidth - m_iVisibleWidth - ROW_COL_WIDTH, HEADER_HEIGHT, 0x908080, 0xF0E0E0, 0x806060);

	m_iColEmptyBg = DIM(theApp.m_pSettings->Appearance.iColBackground, 80);
	
	// The big empty area
	pDC->FillSolidRect(/*m_iPatternWidth*/ m_iVisibleWidth + ROW_COL_WIDTH, HEADER_HEIGHT, m_iWinWidth - /*m_iPatternWidth*/ m_iVisibleWidth - ROW_COL_WIDTH, m_iWinHeight - HEADER_HEIGHT, m_iColEmptyBg);

#else

	// Gray
	pDC->FillSolidRect(m_iPatternWidth + ROW_COL_WIDTH, 0, m_iWinWidth - m_iPatternWidth - ROW_COL_WIDTH, m_iWinHeight, COLOR_FG);

#endif
}

void CPatternView::PaintEditor()
{
	// Complete redraw of editor, slow

	// Pattern head
	DrawHeader(m_pBackDC);
	// Pattern area
	DrawRows(m_pBackDC);
	// Meters
	DrawMeters(m_pBackDC);
}

void CPatternView::EraseBackground(CDC *pDC)
{
	unsigned int ColBackground	 = theApp.m_pSettings->Appearance.iColBackground;
	unsigned int ColHiBackground = theApp.m_pSettings->Appearance.iColBackgroundHilite;
	unsigned int ColText		 = theApp.m_pSettings->Appearance.iColPatternText;
	unsigned int ColHiText		 = theApp.m_pSettings->Appearance.iColPatternTextHilite;

	// Corner
	//pDC->FillSolidRect(0, 0, ROW_COL_WIDTH, HEADER_HEIGHT, COLOR_FG);

//	GradientRect(pDC, 0, 0, ROW_COL_WIDTH, HEADER_HEIGHT, 0x908080, 0xF0E0E0, 0x806060);
//	pDC->Draw3dRect(0, 0, ROW_COL_WIDTH, HEADER_HEIGHT, COLOR_FG_LIGHT, COLOR_FG_DARK);

	// Head background
//	pDC->FillSolidRect(ROW_COL_WIDTH, 0, m_iWinWidth - ROW_COL_WIDTH, HEADER_HEIGHT, COLOR_FOREGROUND);

	// Side row display
//	pDC->FillSolidRect(0, HEADER_HEIGHT, ROW_COL_WIDTH, m_iWinHeight - HEADER_HEIGHT, COLOR_FG);
	pDC->Draw3dRect(0, HEADER_HEIGHT, ROW_COL_WIDTH, m_iWinHeight - HEADER_HEIGHT, COLOR_FG_LIGHT, COLOR_FG_DARK);


	// Pattern background
//	pDC->FillSolidRect(ROW_COL_WIDTH, HEADER_HEIGHT, m_iWinWidth - ROW_COL_WIDTH, m_iWinHeight - HEADER_HEIGHT, ColBackground);

//	pDC->FillSolidRect(ROW_COL_WIDTH, 0, m_iVisibleWidth, HEADER_HEIGHT, COLOR_FG);

}

void CPatternView::UpdateVerticalScroll()
{
	// Vertical scroll bar
	SCROLLINFO si;

	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
	si.nMin = 0;
	si.nMax = m_iPatternLength + 2;
	si.nPos = m_iDrawCursorRow;
	si.nPage = theApp.m_pSettings->General.iPageStepSize;

	m_pView->SetScrollInfo(SB_VERT, &si);
}

void CPatternView::UpdateHorizontalScroll()
{
	// Horizontal scroll bar
	SCROLLINFO si;

	int ColumnCount = 0, CurrentColumn = 0;

	// Calculate cursor pos
	for (int i = 0; i < (signed)m_pDocument->GetAvailableChannels(); i++) {
		if (i == m_iCursorChannel)
			CurrentColumn = ColumnCount + m_iCursorColumn;
		ColumnCount += 7 + m_pDocument->GetEffColumns(i) * 3;
	}

	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
	si.nMin = 0;
	si.nMax = ColumnCount + 3;
	si.nPos = CurrentColumn;
	si.nPage = m_pDocument->GetAvailableChannels();

	m_pView->SetScrollInfo(SB_HORZ, &si);
}

// Draw all rows
void CPatternView::DrawRows(CDC *pDC)
{
	int ColBg = theApp.m_pSettings->Appearance.iColBackground;
	int i, Row;
	CFont *OldFont;

	OldFont = pDC->SelectObject(&PatternFont);
	pDC->SetBkMode(TRANSPARENT);

	Row = m_iDrawMiddleRow - m_iVisibleRows / 2;

	int Channels = m_pDocument->GetAvailableChannels();

	// Todo: call a function recursively to preview more than just two frames

	for (i = 0; i < m_iVisibleRows; i++) {
		if (Row >= 0 && Row < m_iPatternLength) {
			DrawRow(pDC, Row, i, m_iDrawFrame, false);
		}
		else if (theApp.m_pSettings->General.bFramePreview) {
			// Next frame
			if (m_iDrawFrame < signed(m_pDocument->GetFrameCount() - 1) && Row >= m_iPatternLength) {
				int PatternRow = (Row - m_iPatternLength);
				if (PatternRow >= 0 && PatternRow < m_iNextPatternLength)
					DrawRow(pDC, PatternRow, i, m_iDrawFrame + 1, true);
				else
					ClearRow(pDC, i);
			}
			// Previous frame
			else if (m_iDrawFrame > 0 && Row < 0) {
				int PatternRow = (m_iPrevPatternLength + Row);
				if (PatternRow >= 0 && PatternRow < m_iPrevPatternLength)
					DrawRow(pDC, PatternRow, i, m_iDrawFrame - 1, true);
				else
					ClearRow(pDC, i);
			}
			else
				ClearRow(pDC, i);
		}
		Row++;
	}

	// Last unvisible row
	ClearRow(pDC, m_iVisibleRows);

	pDC->SetWindowOrg(-ROW_COL_WIDTH, -HEADER_HEIGHT);

	int iSeparatorCol;

	iSeparatorCol = DIM_TO(ColBg, (ColBg ^ 0xFFFFFF), 50);

	// Lines between channels
	int Offset = m_iChannelWidths[m_iFirstChannel];
	//for (i = 1; i < Channels + 1; i++) {
	for (i = m_iFirstChannel; i < Channels; i++) {
		pDC->FillSolidRect(Offset - 1, 0, 1, m_iWinHeight - ROW_COL_WIDTH, iSeparatorCol);
		Offset += m_iChannelWidths[i + 1];
	}

	// Restore
	pDC->SetWindowOrg(0, 0);
	pDC->SelectObject(OldFont);
}

void CPatternView::ClearRow(CDC *pDC, int Line)
{
	int ColBg = m_iColEmptyBg;//theApp.m_pSettings->Appearance.iColBackground;

	pDC->SetWindowOrg(0, -HEADER_HEIGHT);
	//pDC->FillSolidRect(ROW_COL_WIDTH, Line * ROW_HEIGHT, m_iPatternWidth, ROW_HEIGHT, ColBg);
	
	int Offset = ROW_COL_WIDTH;

	for (int i = m_iFirstChannel; i < m_iFirstChannel + m_iChannelsVisible /*int(m_pDocument->GetAvailableChannels())*/; i++) {
		pDC->FillSolidRect(Offset, Line * ROW_HEIGHT, m_iChannelWidths[i] - 1, ROW_HEIGHT, ColBg);
		Offset += m_iChannelWidths[i];
	}

	// Row number
//	pDC->FillSolidRect(2, Line * ROW_HEIGHT, ROW_COL_WIDTH - 4, ROW_HEIGHT, COLOR_FG);
	pDC->FillSolidRect(1, Line * ROW_HEIGHT, ROW_COL_WIDTH - 2, ROW_HEIGHT, ColBg);
}

// Draw a single row
void CPatternView::DrawRow(CDC *pDC, int Row, int Line, int Frame, bool bPreview)
{
	// Row is row from pattern to display
	// Line is (absolute) screen line

	CString Text;
	stChanNote NoteData;
	bool bHighlight, bSecondHighlight;

	unsigned int ColCursor = theApp.m_pSettings->Appearance.iColCursor;
	unsigned int ColBg = theApp.m_pSettings->Appearance.iColBackground;
	unsigned int ColHiBg = theApp.m_pSettings->Appearance.iColBackgroundHilite;
	unsigned int ColHiBg2 = theApp.m_pSettings->Appearance.iColBackgroundHilite2;
	unsigned int ColSelect = theApp.m_pSettings->Appearance.iColSelection;

	unsigned int BackColor;

	unsigned int PreviewBgCol = DIM(ColBg, 90);

	int i, j;
	int EffColumns;
	int SelStart;
	int Channels = /*m_pDocument->GetAvailableChannels()*/ m_iFirstChannel + m_iChannelsVisible;
	int Frames = m_pDocument->GetFrameCount();
	int PosX = 0;
	int PosY = Row * ROW_HEIGHT;
	int OffsetX = ROW_COL_WIDTH;

	// Start with row number column
	pDC->SetWindowOrg(0, -HEADER_HEIGHT);

	if (Frame != m_iCurrentFrame && !theApp.m_pSettings->General.bFramePreview) {
		ClearRow(pDC, Line);
		return;
	}

	// Highlight
	bHighlight = (m_iHighlight > 0) ? !(Row % m_iHighlight) : false;
	bSecondHighlight = (m_iHighlightSecond > 0) ? !(Row % m_iHighlightSecond) : false;

	// Clear
//	pDC->FillSolidRect(1, Line * ROW_HEIGHT /*+ 2*/, ROW_COL_WIDTH - 2, ROW_HEIGHT - 1, COLOR_FG);
	pDC->FillSolidRect(1, Line * ROW_HEIGHT /*+ 2*/, ROW_COL_WIDTH - 2, ROW_HEIGHT, ColBg);

	if (Frame >= Frames) {
		for (i = m_iFirstChannel; i < Channels; i++) {
			pDC->SetWindowOrg(-OffsetX, -HEADER_HEIGHT - (signed)Line * ROW_HEIGHT);
			pDC->FillSolidRect(0, 0, m_iChannelWidths[i] - 1, ROW_HEIGHT, ColBg);
			OffsetX += m_iChannelWidths[i];
		}
		return;
	}

	// Draw row number
	if (bPreview)
		pDC->SetTextColor(DIM(theApp.m_pSettings->Appearance.iColPatternText, 70));
//		pDC->SetTextColor(0x808080);
	else {
		if (bSecondHighlight)
			pDC->SetTextColor(theApp.m_pSettings->Appearance.iColPatternTextHilite2);
		else if (bHighlight)
			pDC->SetTextColor(theApp.m_pSettings->Appearance.iColPatternTextHilite);
		else
			pDC->SetTextColor(theApp.m_pSettings->Appearance.iColPatternText);
	}
//		pDC->SetTextColor(0x00);


//	CFont *OldFont = pDC->SelectObject(&HeaderFont);

	if (theApp.m_pSettings->General.bRowInHex) {
		Text.Format(_T("%02X"), Row);
		pDC->TextOut(7, Line * ROW_HEIGHT - 1, Text);
	}
	else {
		Text.Format(_T("%03i"), Row);
		pDC->TextOut(4, Line * ROW_HEIGHT - 1, Text);
	}

//	pDC->SelectObject(OldFont);

	/*
	pDC->FillSolidRect(2, (Line ) * ROW_HEIGHT + 1, ROW_COL_WIDTH - 4, 1, 0xFFFFFF);
	pDC->FillSolidRect(2, (Line ) * ROW_HEIGHT, ROW_COL_WIDTH - 4, 1, 0xB8B8B8);
	*/

	// Draw channels
	for (i = m_iFirstChannel; i < Channels; i++) {
		m_pDocument->GetNoteData(Frame, i, Row, &NoteData);

		pDC->SetWindowOrg(-OffsetX, -HEADER_HEIGHT - (signed)Line * ROW_HEIGHT);

		PosX = COLUMN_SPACING;
		SelStart = COLUMN_SPACING;

		EffColumns = m_pDocument->GetEffColumns(i) * 3 + 7;

		if (!bPreview) {
			if (bSecondHighlight) {
				// Highlighted row
				BackColor = ColHiBg2;
			}
			else if (bHighlight) {
				// Highlighted row
				BackColor = ColHiBg;
			}
			else {
				// Normal
				BackColor = ColBg;
			}

			if (Row == m_iDrawCursorRow && m_bHasFocus) {
				// Cursor row				
				if (m_pView->GetEditMode())
					BackColor = DIM_TO(0x000080, BackColor, 50);	// Red
				else
					BackColor = DIM_TO(0x800000, BackColor, 50);	// Blue
			}

			pDC->FillSolidRect(0, 0, m_iChannelWidths[i] - 1, ROW_HEIGHT, BackColor);
		}
		else {
			// Grayed out bg color
			pDC->FillSolidRect(0, 0, m_iChannelWidths[i] - 1, ROW_HEIGHT, PreviewBgCol);
			BackColor = PreviewBgCol;
		}

		if (Frame >= Frames)
			continue;

		if ((!m_bFollowMode /*|| m_bSelecting*/) && Row == m_iPlayRow && Frame == m_iPlayFrame && theApp.IsPlaying()) {
			// Play row
			if (bPreview) {
				pDC->FillSolidRect(0, 0, m_iChannelWidths[i] - 1, ROW_HEIGHT, 0x404040);
			}
			else {
				pDC->FillSolidRect(0, 0, m_iChannelWidths[i] - 1, ROW_HEIGHT, 0x600070);
				pDC->FillSolidRect(0, 0, m_iChannelWidths[i] - 1, 1, 0x400050);
				pDC->FillSolidRect(0, ROW_HEIGHT, m_iChannelWidths[i] - 1, 1, 0x100020);
			}
		}

		for (j = 0; j < EffColumns; j++) {
			if (m_bSelecting && !bPreview) {
				if (Row >= GetSelectRowStart() && Row <= GetSelectRowEnd()) {
					int Color = DIM_TO(ColSelect, BackColor, 80);
					if (i == GetSelectChanStart() && i == GetSelectChanEnd()) {
						if (j >= GetSelectColStart() && j <= GetSelectColEnd()) {
							pDC->FillSolidRect(SelStart - COLUMN_SPACING, 0, SELECT_WIDTH[j], ROW_HEIGHT, Color);
						}
					}
					else if (i == GetSelectChanStart() && i != GetSelectChanEnd()) {
						if (j >= GetSelectColStart()) {
							pDC->FillSolidRect(SelStart - COLUMN_SPACING, 0, SELECT_WIDTH[j], ROW_HEIGHT, Color);
						}
					}
					else if (i == GetSelectChanEnd() && i != GetSelectChanStart()) {
						if (j <= GetSelectColEnd()) {
							pDC->FillSolidRect(SelStart - COLUMN_SPACING, 0, SELECT_WIDTH[j], ROW_HEIGHT, Color);
						}
					}
					else if (i >= GetSelectChanStart() && i < GetSelectChanEnd()) {
						pDC->FillSolidRect(SelStart - COLUMN_SPACING, 0, SELECT_WIDTH[j], ROW_HEIGHT, Color);
					}
				}
			}
			if (m_bDragging && !bPreview) {
				const int SEL_DRAG_COL = 0xA0A0A0;
				if (Row >= m_iDragRowStart && Row <= m_iDragRowEnd) {
					if (i == m_iDragChanStart && i == m_iDragChanEnd) {
						if (j >= m_iDragColStart && j <= m_iDragColEnd) {
							pDC->FillSolidRect(SelStart - COLUMN_SPACING, 0, SELECT_WIDTH[j], ROW_HEIGHT, SEL_DRAG_COL);
						}
					}
					else if (i == m_iDragChanStart && i != m_iDragChanEnd) {
						if (j >= m_iDragColStart) {
							pDC->FillSolidRect(SelStart - COLUMN_SPACING, 0, SELECT_WIDTH[j], ROW_HEIGHT, SEL_DRAG_COL);
						}
					}
					else if (i == m_iDragChanEnd && i != m_iDragChanStart) {
						if (j <= m_iDragColEnd) {
							pDC->FillSolidRect(SelStart - COLUMN_SPACING, 0, SELECT_WIDTH[j], ROW_HEIGHT, SEL_DRAG_COL);
						}
					}
					else if (i >= m_iDragChanStart && i < m_iDragChanEnd) {
						pDC->FillSolidRect(SelStart - COLUMN_SPACING, 0, SELECT_WIDTH[j], ROW_HEIGHT, SEL_DRAG_COL);
					}
				}
			}

			bool bInvert = false;

			if (i == m_iCursorChannel && j == m_iCursorColumn && Row == m_iDrawCursorRow && !bPreview) {
				// Draw cursor box
				pDC->FillSolidRect(PosX - 2, 0, COLUMN_WIDTH[j], ROW_HEIGHT, DIM(ColCursor, 70));
				pDC->Draw3dRect(PosX - 2, 0, COLUMN_WIDTH[j], ROW_HEIGHT, ColCursor, DIM(ColCursor, 50));
				bInvert = true;
			}

			DrawCell(PosX, j, i, bHighlight, bSecondHighlight, bPreview, bInvert, BackColor, &NoteData, pDC);
			PosX += COLUMN_SPACE[j];
			SelStart += SELECT_WIDTH[j];
		}

		OffsetX += m_iChannelWidths[i];
	}

	if (m_iChannelsVisible < Channels && OffsetX < m_iWinWidth) {
		pDC->SetWindowOrg(-OffsetX, -HEADER_HEIGHT - (signed)Line * ROW_HEIGHT);
		pDC->FillSolidRect(0, 0, m_iPatternWidth - OffsetX + ROW_COL_WIDTH, ROW_HEIGHT, ColBg);	// Todo: adjust channel width
		OffsetX += m_iChannelWidths[i];
	}
}


void CPatternView::DrawCell(int PosX, int Column, int Channel, bool bHighlight, bool bHighlight2, bool bShaded, bool bInvert, unsigned int BackColor, stChanNote *NoteData, CDC *dc)
{
	const char NOTES_A[] = {'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'};
	const char NOTES_B[] = {'-', '#', '-', '#', '-', '-', '#', '-', '#', '-', '#', '-'};
	const char NOTES_C[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	const char HEX[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

	unsigned int BgCol = theApp.m_pSettings->Appearance.iColBackground;
	unsigned int HiBgCol = theApp.m_pSettings->Appearance.iColBackgroundHilite;
	unsigned int HiBgCol2 = theApp.m_pSettings->Appearance.iColBackgroundHilite2;
	unsigned int NoteCol = theApp.m_pSettings->Appearance.iColPatternText;
	unsigned int InstCol = theApp.m_pSettings->Appearance.iColPatternInstrument;
	unsigned int VolumeCol = theApp.m_pSettings->Appearance.iColPatternVolume;
	unsigned int EffCol = theApp.m_pSettings->Appearance.iColPatternEffect;
	unsigned int ColCursor = theApp.m_pSettings->Appearance.iColCursor;

	int Vol			= NoteData->Vol;
	int Note		= NoteData->Note;
	int Octave		= NoteData->Octave;
	int Instrument	= NoteData->Instrument;

	if (Note < 0 || Note > HALT)
		return;

	int EffNumber, EffParam;

	switch (Column) {
		case 4: case 5: case 6:
			EffNumber	= NoteData->EffNumber[0];
			EffParam	= NoteData->EffParam[0];
			break;
		case 7: case 8: case 9:
			EffNumber	= NoteData->EffNumber[1];
			EffParam	= NoteData->EffParam[1];
			break;
		case 10: case 11: case 12:
			EffNumber	= NoteData->EffNumber[2];
			EffParam	= NoteData->EffParam[2];
			break;
		case 13: case 14: case 15:
			EffNumber	= NoteData->EffNumber[3];
			EffParam	= NoteData->EffParam[3];
			break;
	}


	if (!theApp.m_pSettings->General.bPatternColor) {
		InstCol = VolumeCol = EffCol = NoteCol;
	}

	if (Instrument < 0x40 && !m_pDocument->IsInstrumentUsed(Instrument)) {
		InstCol = 0xFF;
	}

	if (bHighlight2) {
		NoteCol = theApp.m_pSettings->Appearance.iColPatternTextHilite2;
		BgCol = HiBgCol2;
	}
	else if (bHighlight) {
		NoteCol = theApp.m_pSettings->Appearance.iColPatternTextHilite;
		BgCol = HiBgCol;
	}
/*
	if (bInvert) {
		NoteCol = NoteCol ^ ColCursor;
		InstCol = InstCol ^ ColCursor;
		VolumeCol = VolumeCol ^ ColCursor;
		EffCol = EffCol ^ ColCursor;
	}
*/
//	unsigned int ShadedCol = DIM(NoteCol, 40);
	unsigned int ShadedCol = DIM_TO(NoteCol, BackColor, 40);

	if (bShaded) {
		ShadedCol	= DIM_TO(ShadedCol, BgCol, 50);
		NoteCol		= DIM_TO(NoteCol, BgCol, 50);
		VolumeCol	= DIM_TO(VolumeCol, BgCol, 50);
		EffCol		= DIM_TO(EffCol, BgCol, 50);
		InstCol		= DIM_TO(InstCol, BgCol, 50);
	}

	int PosY = -2;
	PosX -= 1;

#define BAR(x, y) dc->FillSolidRect((x) + 3, (y) + 7, CHAR_WIDTH - 7, 1, ShadedCol)

	switch (Column) {
		case 0:
			// Note and octave
			switch (Note) {
				case 0:
					/*
					dc->FillSolidRect(PosX + 2, PosY + 7, CHAR_WIDTH - 6, 1, ShadedCol);
					dc->FillSolidRect(PosX + CHAR_WIDTH + 2, PosY + 7, CHAR_WIDTH - 6, 1, ShadedCol);
					dc->FillSolidRect(PosX + CHAR_WIDTH * 2 + 2, PosY + 7, CHAR_WIDTH - 6, 1, ShadedCol);
					*/
					BAR(PosX, PosY);
					BAR(PosX + CHAR_WIDTH, PosY);
					BAR(PosX + CHAR_WIDTH * 2, PosY);
					break;
				case HALT:
					// Note stop
					dc->FillSolidRect(PosX + 4, 5, CHAR_WIDTH * 3 - 12, ROW_HEIGHT - 9, NoteCol);
					break;
				case RELEASE:
					// Note release
					dc->FillSolidRect(PosX + 2, PosY + 6, CHAR_WIDTH - 5, 4, NoteCol);
					dc->FillSolidRect(PosX + CHAR_WIDTH + 2, PosY + 6, CHAR_WIDTH - 5, 4, NoteCol);
					dc->FillSolidRect(PosX + CHAR_WIDTH * 2 + 2, PosY + 6, CHAR_WIDTH - 5, 4, NoteCol);
					break;
				default:
					if (Channel == 3) {
						char NoiseFreq = (Note - 1 + Octave * 12) & 0x0F;
						DrawChar(PosX, PosY, HEX[NoiseFreq], NoteCol, dc);
						DrawChar(PosX + CHAR_WIDTH, PosY, '-', NoteCol, dc);
						DrawChar(PosX + CHAR_WIDTH * 2, PosY, '#', NoteCol, dc);
					}
					else {
						DrawChar(PosX, PosY, NOTES_A[Note - 1], NoteCol, dc);
						DrawChar(PosX + CHAR_WIDTH, PosY, NOTES_B[Note - 1], NoteCol, dc);
						DrawChar(PosX + CHAR_WIDTH * 2, PosY, NOTES_C[Octave], NoteCol, dc);
					}
			}
			return;
		case 1:
			// Instrument x0
			if (Instrument == MAX_INSTRUMENTS || Note == HALT)
				//dc->FillSolidRect(PosX + 2, PosY + 7, CHAR_WIDTH - 6, 1, ShadedCol);
				BAR(PosX, PosY);
			else
				DrawChar(PosX, PosY, HEX[Instrument >> 4], InstCol, dc);
			return;
		case 2:
			// Instrument 0x
			if (Instrument == MAX_INSTRUMENTS || Note == HALT)
//				dc->FillSolidRect(PosX + 2, PosY + 7, CHAR_WIDTH - 6, 1, ShadedCol);
				BAR(PosX, PosY);
			else
				DrawChar(PosX, PosY, HEX[Instrument & 0x0F], InstCol, dc);
			return;
		case 3:
			// Volume (skip triangle)
			if (Vol == 0x10 || Channel == 2 || Channel == 4)
//				dc->FillSolidRect(PosX + 2, PosY + 7, CHAR_WIDTH - 6, 1, ShadedCol);
				BAR(PosX, PosY);
			else
				DrawChar(PosX, PosY, HEX[Vol & 0x0F], VolumeCol, dc);
			return;
		case 4: case 7: case 10: case 13:
			// Effect type
			if (EffNumber == 0)
//				dc->FillSolidRect(PosX + 2, PosY + 7, CHAR_WIDTH - 6, 1, ShadedCol);
				BAR(PosX, PosY);
			else
				DrawChar(PosX, PosY, EFF_CHAR[EffNumber - 1], EffCol, dc);
			return;
		case 5: case 8: case 11: case 14:
			// Effect param x
			if (EffNumber == 0)
//				dc->FillSolidRect(PosX + 2, PosY + 7, CHAR_WIDTH - 6, 1, ShadedCol);
				BAR(PosX, PosY);
			else
				DrawChar(PosX, PosY, HEX[(EffParam >> 4) & 0x0F], NoteCol, dc);
			return;
		case 6: case 9: case 12: case 15:
			// Effect param y
			if (EffNumber == 0)
//				dc->FillSolidRect(PosX + 2, PosY + 7, CHAR_WIDTH - 6, 1, ShadedCol);
				BAR(PosX, PosY);
			else
				DrawChar(PosX, PosY, HEX[EffParam & 0x0F], NoteCol, dc);
			return;
	}
}

void CPatternView::DrawHeader(CDC *pDC)
{
	DrawChannelNames(pDC);
}

void CPatternView::DrawChannelNames(CDC *pDC)
{
	const char *CHAN_NAMES[] = {"Square 1", "Square 2", "Triangle", "Noise", "DPCM",		// 2A03
								"Square 1", "Square 2", "Sawtooth",							// VRC6
								"Channel 1", "Channel 2", "Channel 3",						// VRC7
								"Channel 4", "Channel 5", "Channel 6",	
								"Square 1", "Square 2",	"(Voice)",							// MMC5
								"Square 1", "Square 2", "Square 3",							// 5B
								"Channel 1", "Channel 2", "Channel 3", "Channel 4",			// N106 
								"Channel 5", "Channel 6", "Channel 7", "Channel 8",
								"FDS"};														// FDS

	const char *pChanName;

	unsigned int Offset = ROW_COL_WIDTH;
	unsigned int AvailableChannels = m_pDocument->GetAvailableChannels();
	unsigned int i;

	CPoint ArrowPoints[3];
	CFont *OldFont;

	CBrush HoverBrush((COLORREF)0xFFFFFF);
	CPen HoverPen(PS_SOLID, 1, (COLORREF)0xA0A0A0);

	CBrush BlackBrush((COLORREF)0x505050);
	CPen BlackPen(PS_SOLID, 1, (COLORREF)0x808080);

	CObject *pOldBrush, *pOldPen;

//	pOldPen = pDC->SelectObject(&Pen);
	OldFont = pDC->SelectObject(&HeaderFont);
	pDC->SetBkMode(TRANSPARENT);

	// Head background
//	pDC->FillSolidRect(ROW_COL_WIDTH, 0, m_iWinWidth - ROW_COL_WIDTH, HEADER_HEIGHT, COLOR_FG);

	// Restore background
//	pDC->BitBlt(0, 0, m_iVisibleWidth, HEADER_HEIGHT, m_pBackDC, 0, 6, SRCCOPY);

//	pDC->FillSolidRect(ROW_COL_WIDTH, 0, m_iVisibleWidth, HEADER_HEIGHT, COLOR_FG);

//	unsigned int ChanNameBgCol = DIM_TO(0x807070, theApp.m_pSettings->Appearance.iColBackground, 75);
	unsigned int iHeadCol1, iHeadCol2, iHeadCol3;

	iHeadCol1 = GetSysColor(COLOR_3DFACE);
	iHeadCol2 = GetSysColor(COLOR_BTNHIGHLIGHT);// 0xFFFFFF;
	iHeadCol3 = GetSysColor(COLOR_APPWORKSPACE);

	if (m_pView->GetEditMode()) {
		//iHeadCol1 = DIM_TO(iHeadCol1, 0x0000FF, 85);
		iHeadCol3 = DIM_TO(iHeadCol3, 0x0000FF, 80);
	}

	GradientRect(pDC, 0, 0, m_iVisibleWidth + ROW_COL_WIDTH, HEADER_HEIGHT, iHeadCol1, iHeadCol2, iHeadCol3);

	// Corner box
	pDC->Draw3dRect(0, 0, ROW_COL_WIDTH, HEADER_HEIGHT, COLOR_FG_LIGHT, COLOR_FG_DARK);

	for (i = m_iFirstChannel; i < AvailableChannels; i++) {
		// Frame
		pDC->Draw3dRect(Offset, 0, m_iChannelWidths[i], HEADER_HEIGHT, COLOR_FG_LIGHT, iHeadCol3);
		// Text
		pChanName = CHAN_NAMES[m_pDocument->GetChannelType(i)];
		// Shadow
		pDC->SetTextColor(0xD0D0D0);
		pDC->TextOut(Offset + 11, 6, pChanName);
		// Foreground
		pDC->SetTextColor(m_pView->IsChannelMuted(i) ? 0x0000F0 : 0x202020);
		pDC->TextOut(Offset + 10, 5, pChanName);

		// Effect columns
		pDC->SetTextColor(0x404040);
		if (m_pDocument->GetEffColumns(i) > 0)
			pDC->TextOut(Offset + CHANNEL_WIDTH + COLUMN_SPACING + 2, HEADER_HEIGHT - 16, "fx2");
		if (m_pDocument->GetEffColumns(i) > 1)
			pDC->TextOut(Offset + CHANNEL_WIDTH + COLUMN_SPACING * 2 + CHAR_WIDTH * 3 + 2, HEADER_HEIGHT - 16, "fx3");
		if (m_pDocument->GetEffColumns(i) > 2)
			pDC->TextOut(Offset + CHANNEL_WIDTH + COLUMN_SPACING * 3 + (CHAR_WIDTH * 2) * 3 + 2, HEADER_HEIGHT - 16, "fx4");


		// Arrows for expanding/removing fx columns
		if (m_pDocument->GetEffColumns(i) > 0) {
			ArrowPoints[0].SetPoint(Offset + CHANNEL_WIDTH - 17,	 6);
			ArrowPoints[1].SetPoint(Offset + CHANNEL_WIDTH - 17,	 6 + 10);
			ArrowPoints[2].SetPoint(Offset + CHANNEL_WIDTH - 17 - 5, 6 + 5);

			if (m_iMouseHoverChan == i && m_iMouseHoverEffArrow == 1) {
				pOldBrush = pDC->SelectObject(&HoverBrush);
				pOldPen = pDC->SelectObject(&HoverPen);
			}
			else {
				pOldBrush = pDC->SelectObject(&BlackBrush);
				pOldPen = pDC->SelectObject(&BlackPen);
			}

			pDC->Polygon(ArrowPoints, 3);
			pDC->SelectObject(pOldBrush);
		}

		if (m_pDocument->GetEffColumns(i) < (MAX_EFFECT_COLUMNS - 1)) {
			ArrowPoints[0].SetPoint(Offset + CHANNEL_WIDTH - 11,	 6);
			ArrowPoints[1].SetPoint(Offset + CHANNEL_WIDTH - 11,	 6 + 10);
			ArrowPoints[2].SetPoint(Offset + CHANNEL_WIDTH - 11 + 5, 6 + 5);

			if (m_iMouseHoverChan == i && m_iMouseHoverEffArrow == 2) {
				pOldBrush = pDC->SelectObject(&HoverBrush);
				pOldPen = pDC->SelectObject(&HoverPen);
			}
			else {
				pOldBrush = pDC->SelectObject(&BlackBrush);
				pOldPen = pDC->SelectObject(&BlackPen);
			}

			pDC->Polygon(ArrowPoints, 3);
			pDC->SelectObject(pOldBrush);
		}

		Offset += m_iChannelWidths[i];
	}

	pDC->SelectObject(OldFont);
	pDC->SelectObject(pOldPen);
}

void CPatternView::DrawMeters(CDC *pDC)
{
	const unsigned int COL_DARK = 0x808080;//0x485848;
	const unsigned int COL_LIGHT = 0x20F040;
	
	const unsigned int COL_DARK_SHADOW = DIM(COL_DARK, 80);
	const unsigned int COL_LIGHT_SHADOW = DIM(COL_LIGHT, 60);

	const int BAR_TOP	 = 5+18;
	const int BAR_LEFT	 = ROW_COL_WIDTH + 7;
	const int BAR_SIZE	 = 6;
	const int BAR_SPACE	 = 1;
	const int BAR_HEIGHT = 5;

	static unsigned int colors[15];
	static unsigned int colors_dark[15];
	static unsigned int colors_shadow[15];
	static unsigned int colors_dark_shadow[15];

	static int LastSamplePos, LastDeltaPos;

	int AvailableChannels = (signed)m_pDocument->GetAvailableChannels();

	int Offset = BAR_LEFT;
	int i, c;

	CPoint points[5];
	CFont *pOldFont = pDC->SelectObject(&HeaderFont);

	if (colors[0] == 0) {
		for (i = 0; i < 15; i++) {
			colors[i] = DIM_TO(COL_LIGHT, 0x00F0F0, (100 - (i * i) / 3));
			colors_dark[i] = DIM_TO(COL_DARK, 0x00F0F0, (100 - (i * i) / 3));
			colors_shadow[i] = DIM(colors[i], 60);
			colors_dark_shadow[i] = DIM(colors_dark[i], 70);
		}
	}

	for (c = m_iFirstChannel; c < AvailableChannels; c++) {
		for (i = 0; i < 15; i++) {
			if (i < m_iVolLevels[c]) {
				pDC->FillSolidRect(Offset + (i * BAR_SIZE) + BAR_SIZE - 1, BAR_TOP + 1, 1, BAR_HEIGHT, colors_shadow[i]);
				pDC->FillSolidRect(Offset + (i * BAR_SIZE) + 1, BAR_TOP + BAR_HEIGHT, BAR_SIZE - 1, 1, colors_shadow[i]);
				pDC->FillSolidRect(CRect(Offset + (i * BAR_SIZE), BAR_TOP, Offset + (i * BAR_SIZE) + (BAR_SIZE - BAR_SPACE), BAR_TOP + BAR_HEIGHT), colors[i]);
				pDC->Draw3dRect(CRect(Offset + (i * BAR_SIZE), BAR_TOP, Offset + (i * BAR_SIZE) + (BAR_SIZE - BAR_SPACE), BAR_TOP + BAR_HEIGHT), colors[i], DIM(colors[i], 90));
			}
			else {
				pDC->FillSolidRect(Offset + (i * BAR_SIZE) + BAR_SIZE - 1, BAR_TOP + 1, BAR_SPACE, BAR_HEIGHT, COL_DARK_SHADOW);
				pDC->FillSolidRect(Offset + (i * BAR_SIZE) + 1, BAR_TOP + BAR_HEIGHT, BAR_SIZE - 1, 1, COL_DARK_SHADOW);
				pDC->FillSolidRect(CRect(Offset + (i * BAR_SIZE), BAR_TOP, Offset + (i * BAR_SIZE) + (BAR_SIZE - BAR_SPACE), BAR_TOP + BAR_HEIGHT), COL_DARK);
			}
		}

		Offset += m_iChannelWidths[c]; //CHANNEL_WIDTH + m_pDocument->GetEffColumns(c) * (CHAR_WIDTH * 3 + COLUMN_SPACING) + CHANNEL_SPACING * 2;
	}

	// DPCM
	if (m_DPCMState.SamplePos != LastSamplePos || m_DPCMState.DeltaCntr != LastDeltaPos) {
		if (theApp.GetMainWnd()->GetMenu()->GetMenuState(ID_TRACKER_DPCM, MF_BYCOMMAND) == MF_CHECKED) {
			CString Text;		   

			pDC->SetBkColor(COLOR_BG);
			pDC->SetTextColor(COLOR_FG);

			Text.Format("Sample position: %02X", m_DPCMState.SamplePos);
			pDC->TextOut(Offset + 20, 5, Text);

			Text.Format("Delta counter: %02X", m_DPCMState.DeltaCntr);
			pDC->TextOut(Offset + 20, 19, Text);

			LastSamplePos = m_DPCMState.SamplePos;
			LastDeltaPos = m_DPCMState.DeltaCntr;
		}
	}

	pDC->SelectObject(pOldFont);
}

void CPatternView::SetMeterValue(int Channel, int Value)
{
	m_iVolLevels[Channel] = Value;
}

void CPatternView::SetDPCMState(stDPCMState State)
{
	m_DPCMState = State;
}

// Draws a colored character
void CPatternView::DrawChar(int x, int y, char c, int Color, CDC *pDC)
{
	static CString Text;
	Text.Format(_T("%c"), c);
	pDC->SetTextColor(Color);
	pDC->TextOut(x, y, Text);
	m_iCharsDrawn++;
//	m_TextRender.DrawChar(pDC, x, y, c);
}

// Private //////////////////

// Todo: change this to universal scroll up and down any number of steps
void CPatternView::FastScrollDown(CDC *pDC)
{
	// Save cursor position
	m_iDrawCursorRow = m_iCursorRow;

	CFont *OldFont = pDC->SelectObject(&PatternFont);
	pDC->SetBkMode(TRANSPARENT);

	int Top = HEADER_HEIGHT + ((m_iVisibleRows / 2) + 2) * ROW_HEIGHT;
	int Width = ROW_COL_WIDTH + m_iPatternWidth - 1;
	int Height = (m_iVisibleRows / 2) * ROW_HEIGHT - 1;

	// Bottom section
	pDC->BitBlt(1, Top - ROW_HEIGHT, Width, Height - ROW_HEIGHT + 1, pDC, 1, Top, SRCCOPY); 

	// Top section
	Top = HEADER_HEIGHT + ROW_HEIGHT;
	pDC->BitBlt(1, Top - ROW_HEIGHT, Width, Height, pDC, 1, Top, SRCCOPY); 

	int Row;

	// Bottom row
	Row = m_iDrawCursorRow + (m_iVisibleRows / 2) - ((m_iVisibleRows & 1) ? 0 : 1);
	if (Row < m_iPatternLength) 
		DrawRow(pDC, Row, m_iVisibleRows - 1, m_iCurrentFrame, false);
	else if ((Row - m_iPatternLength) < m_iNextPatternLength && unsigned(m_iCurrentFrame) < (m_pDocument->GetFrameCount() - 1))
		DrawRow(pDC, (Row - m_iPatternLength), m_iVisibleRows - 1, m_iCurrentFrame + 1, true);
	else
		ClearRow(pDC, m_iVisibleRows - 1);

	// Middle row
	Row = m_iDrawCursorRow;
	if (Row < m_iPatternLength && Row >= 0) 
		DrawRow(pDC, Row, m_iVisibleRows / 2, m_iCurrentFrame, false);

	// Above middle row
	Row = m_iDrawCursorRow - 1;
	if (Row < m_iPatternLength && Row >= 0)
		DrawRow(pDC, Row, m_iVisibleRows / 2 - 1, m_iCurrentFrame, false);

	pDC->SelectObject(OldFont);

	UpdateVerticalScroll();
}

// Cursor movement

void CPatternView::UpdateSelection()
{
	if (m_bShiftPressed) {
		m_bSelecting = true;
		m_iSelEndChannel = m_iCursorChannel;
		m_iSelEndRow = m_iCursorRow;
		m_iSelEndColumn = m_iCursorColumn;
	}
	else {
		m_bSelecting = false;
		m_iSelStartChannel = m_iCursorChannel;
		m_iSelStartRow = m_iCursorRow;
		m_iSelStartColumn = m_iCursorColumn;
	}
}

void CPatternView::MoveDown(int Step)
{
	Step = Step == 0 ? 1 : Step;
	MoveToRow(m_iCursorRow + Step);
	UpdateSelection();
}

void CPatternView::MoveUp(int Step)
{
	Step = Step == 0 ? 1 : Step;
	MoveToRow(m_iCursorRow - Step);
	UpdateSelection();
}

void CPatternView::MoveLeft()
{
	if (m_iCursorColumn > 0)
		m_iCursorColumn--;
	else {
		if (m_iCursorChannel > 0) {
			m_iCursorChannel--;
			m_iCursorColumn = m_iColumns[m_iCursorChannel];
		}
		else {
			if (theApp.m_pSettings->General.bWrapCursor) {
				m_iCursorChannel = m_pDocument->GetAvailableChannels() - 1;
				m_iCursorColumn = m_iColumns[m_iCursorChannel];
			}
		}
	}
	UpdateSelection();
}

void CPatternView::MoveRight()
{
	if (m_iCursorColumn < m_iColumns[m_iCursorChannel])
		m_iCursorColumn++;
	else {
		if (m_iCursorChannel < (signed)m_pDocument->GetAvailableChannels() - 1) {
			m_iCursorChannel++;
			m_iCursorColumn = 0;
		}
		else {
			if (theApp.m_pSettings->General.bWrapCursor) {
				m_iCursorChannel = 0;
				m_iCursorColumn = 0;
			}
		}
	}
	UpdateSelection();
}

void CPatternView::MoveToTop()
{
	MoveToRow(0);
	UpdateSelection();
}

void CPatternView::MoveToBottom()
{
	MoveToRow(m_iPatternLength - 1);
	UpdateSelection();
}

void CPatternView::MovePageUp()
{
	MoveToRow(m_iCursorRow - theApp.m_pSettings->General.iPageStepSize);
	UpdateSelection();
}

void CPatternView::MovePageDown()
{
	MoveToRow(m_iCursorRow + theApp.m_pSettings->General.iPageStepSize);
	UpdateSelection();
}

void CPatternView::MoveToRow(int Row)
{
	if (Row < 0) {
		if (theApp.m_pSettings->General.bWrapFrames) {
			MoveToFrame(m_iCurrentFrame - 1);
			Row = m_pDocument->GetPatternLength() + Row;
		}
		else {
			if (theApp.m_pSettings->General.bWrapCursor)
				Row = m_iPatternLength - 1;
			else
				Row = 0;
		}
	}
	else if (Row > m_iPatternLength - 1) {
		if (theApp.m_pSettings->General.bWrapFrames) {
			MoveToFrame(m_iCurrentFrame + 1);
			Row = Row - m_iPatternLength;
		}
		else {
			if (theApp.m_pSettings->General.bWrapCursor)
				Row = 0;
			else
				Row = m_iPatternLength - 1;
		}
	}

	if (m_bSelecting)
		m_bForceFullRedraw = true;

	m_iCursorRow = Row;
	m_bSelecting = false;
}

void CPatternView::MoveToFrame(int Frame)
{
	if (Frame < 0) {
		if (theApp.m_pSettings->General.bWrapFrames) {
			Frame = m_pDocument->GetFrameCount() - 1;
		}
		else
			Frame = 0;
	}
	if (Frame > (signed)m_pDocument->GetFrameCount() - 1) {
		if (theApp.m_pSettings->General.bWrapFrames) {
			Frame = 0;
		}
		else
			Frame = (signed)m_pDocument->GetFrameCount() - 1;
	}
	
	if (theApp.IsPlaying() && m_bFollowMode) {
		if (m_iPlayFrame != Frame) {
			m_iPlayFrame = Frame;
			m_iPlayRow = 0;
		}
	}

	m_iCurrentFrame = Frame;
	m_bSelecting = false;
	m_bForceFullRedraw = true;
}

void CPatternView::MoveToChannel(int Channel)
{
	if (Channel < 0) {
		if (theApp.m_pSettings->General.bWrapCursor)
			Channel = m_pDocument->GetAvailableChannels() - 1;
		else
			Channel = 0;
	}
	else if (Channel > (signed)m_pDocument->GetAvailableChannels() - 1) {
		if (theApp.m_pSettings->General.bWrapCursor)
			Channel = 0;
		else
			Channel = m_pDocument->GetAvailableChannels() - 1;
	}
	m_iCursorChannel = Channel;
	m_iCursorColumn = 0;
	m_bSelecting = false;
}

void CPatternView::MoveToColumn(int Column)
{
	m_iCursorColumn = Column;
	m_bSelecting = false;
}

void CPatternView::NextFrame()
{
	MoveToFrame(m_iCurrentFrame + 1);
	m_bSelecting = false;
}

void CPatternView::PreviousFrame()
{
	MoveToFrame(m_iCurrentFrame - 1);
	m_bSelecting = false;
}

void CPatternView::NextChannel()
{
	MoveToChannel(m_iCursorChannel + 1);
	m_iCursorColumn = 0;
}

void CPatternView::PreviousChannel()
{
	MoveToChannel(m_iCursorChannel - 1);
	m_iCursorColumn = 0;
}

void CPatternView::FirstChannel()
{
	MoveToChannel(0);
	m_iCursorColumn	= 0;
	UpdateSelection();
}

void CPatternView::LastChannel()
{
	MoveToChannel(m_pDocument->GetAvailableChannels() - 1);
	m_iCursorColumn	= 0;
	UpdateSelection();
}

// Used by the player

bool CPatternView::StepRow()
{
	m_iPlayRow++;
	if (m_iPlayRow >= m_iPlayPatternLength) {
		m_iPlayRow = 0;
		return true;
	}
	return false;
}

bool CPatternView::StepFrame()
{
	m_bForceFullRedraw = true;
	m_iPlayFrame++;
	m_iPlayPatternLength = GetCurrentPatternLength(m_iPlayFrame);
	if (m_iPlayFrame >= (signed)m_pDocument->GetFrameCount()) {
		m_iPlayFrame = 0;
		m_iPlayPatternLength = GetCurrentPatternLength(m_iPlayFrame);
		return true;
	}
	return false;
}

void CPatternView::JumpToRow(int Row)
{
	if (Row >= m_iPatternLength)
		Row = m_iPatternLength - 1;

	m_iPlayRow = Row;
}

void CPatternView::JumpToFrame(int Frame)
{
	if ((unsigned int)Frame >= m_pDocument->GetFrameCount())
		Frame = m_pDocument->GetFrameCount() - 1;

	m_iPlayFrame = Frame;
	m_iPlayPatternLength = GetCurrentPatternLength(Frame);
}

// Mouse routines

void CPatternView::OnMouseDown(CPoint point)
{
	m_bSelectionInvalid = true;

	if (point.y < HEADER_HEIGHT) {
		// Channel headers
		int Channel = GetChannelAtPoint(point.x);
		int Column = GetColumnAtPoint(point.x);

		if (Channel < 0 || Channel > (signed)m_pDocument->GetAvailableChannels() - 1)
			return;

		// Mute/unmute
		if (Column < 5) {
			m_pView->ToggleChannel(Channel);
		}
		// Remove one track effect column
		else if (Column == 5) {
			if (m_pDocument->GetEffColumns(Channel) > 0)
				m_pDocument->SetEffColumns(Channel, m_pDocument->GetEffColumns(Channel) - 1);
		}
		// Add one track effect column
		else if (Column == 6) {
			if (m_pDocument->GetEffColumns(Channel) < (MAX_EFFECT_COLUMNS - 1))
				m_pDocument->SetEffColumns(Channel, m_pDocument->GetEffColumns(Channel) + 1);
		}
	}
	else if (point.y > HEADER_HEIGHT) {

		int Row = GetRowAtPoint(point.y);
		int Channel = GetChannelAtPoint(point.x);
		int Column = GetColumnAtPoint(point.x);

		if (m_bShiftPressed && !IsWithinSelection(Column, Channel, Row)) {
			// Expand selection
			if (!m_bSelecting) {
				m_iSelStartChannel = m_iCursorChannel;
				m_iSelStartColumn = m_iCursorColumn;
				m_iSelStartRow = m_iCursorRow;
			}
			m_iSelEndChannel = Channel;
			m_iSelEndColumn = Column;
			m_iSelEndRow = Row;
			m_bSelecting = true;
			m_bSelectionInvalid = true;
			m_bFullRowSelect = false;
			SelStartPoint = point;
		}
		else {
			if (point.x < (m_iPatternWidth + ROW_COL_WIDTH)) {
				// Pattern area
				if (point.x < ROW_COL_WIDTH) {
					m_iSelStartRow = m_iSelEndRow = Row;
					m_iSelStartChannel = 0;
					m_iSelEndChannel = m_pDocument->GetAvailableChannels();
					m_iSelStartColumn = 0;
					m_iSelEndColumn = m_pDocument->GetEffColumns(m_pDocument->GetAvailableChannels()) * 3 + 7;
					m_bSelecting = false;
					m_bSelectionInvalid = false;
					m_bFullRowSelect = true;
					SelStartPoint = point;
				}
				else
					m_bFullRowSelect = false;

				if (Channel < 0 || Channel > (signed)m_pDocument->GetAvailableChannels() - 1)
					return;
				if (Row < 0)
					return;
				if (Column < 0)
					return;

				if (m_bSelecting) {
					if (Row >= GetSelectRowStart() && Row <= GetSelectRowEnd() && Channel >= GetSelectChanStart() && Channel <= GetSelectChanEnd()) {
						if (GetSelectChanStart() == GetSelectChanEnd()) {
							if (Channel == GetSelectChanStart() && (Column >= GetSelectColStart() && Column <= GetSelectColEnd()))
								m_bDragStart = true;
							else {
								m_bDragStart = false;
								m_bDragging = false;
								m_bSelecting = false;
							}
						}
						else {
							if (GetSelectChanStart() == Channel && Column >= GetSelectColStart())
								m_bDragStart = true;
							else if (GetSelectChanEnd() == Channel && Column <= GetSelectColEnd())
								m_bDragStart = true;
							else if (GetSelectChanStart() < Channel && GetSelectChanEnd() > Channel)
								m_bDragStart = true;
							else {
								m_bDragStart = false;
								m_bDragging = false;
								m_bSelecting = false;
							}
						}
					}
					else {
						m_bDragStart = false;
						m_bDragging = false;
						m_bSelecting = false;
					}
				}

				if (!m_bDragging && !m_bDragStart) {
					m_iSelStartRow = m_iSelEndRow = Row;
					m_iSelStartChannel = m_iSelEndChannel = Channel;
					m_iSelStartColumn = m_iSelEndColumn = Column;
				}
				else {
					m_iDragPointRow = Row;
					m_iDragPointChan = Channel;
					m_iDragPointCol = GetSelectColumn(Column);
				}

				SelStartPoint = point;
				m_bSelectionInvalid = false;
			}
			else {			
				// Clicked outside the patterns
				m_bDragStart = false;
				m_bDragging = false;
				m_bSelecting = false;
			}
		}
	}
}

void CPatternView::OnMouseUp(CPoint point)
{
	if (point.y < HEADER_HEIGHT) {
		// Channel headers
		if (m_bDragging) {
			m_bDragging = false;
			m_bDragStart = false;
		}
	}
	else if (point.y > HEADER_HEIGHT) {
		// Pattern area
		int Row = GetRowAtPoint(point.y);
		int Channel = GetChannelAtPoint(point.x);
		int Column = GetColumnAtPoint(point.x);

		if (point.x < ROW_COL_WIDTH) {
			if (m_bDragging) {
				m_bDragging = false;
				m_bDragStart = false;
			}
			// Row column, move to clicked row
			if (Row < 0 || m_bSelecting)
				return;
			m_iCursorRow = Row;
			return;
		}

		if (m_bDragging) {
			m_bDragging = false;
			m_bDragStart = false;
			if (m_iDragChanStart != m_iSelStartChannel || m_iDragRowStart != m_iSelStartRow || m_iDragColStart != m_iSelStartColumn) {
				if (m_iDragChanStart < 0) {
					m_iDragChanStart = 0;
					//m_iSelStartColumn = 0;
					m_iDragColStart = 0;
				}
				if (m_iDragChanEnd > (signed)m_pDocument->GetAvailableChannels() - 1) {
					m_iDragChanEnd = (signed)m_pDocument->GetAvailableChannels() - 1;
					//m_iSelEndColumn = m_pDocument->GetEffColumns(m_pDocument->GetAvailableChannels()) * 3 + 7;
					m_iDragColEnd = m_pDocument->GetEffColumns(m_pDocument->GetAvailableChannels()) * 3 + 7;
				}
				if (m_iDragRowStart < 0)
					m_iDragRowStart = 0;
				if (m_iDragRowEnd > m_iPatternLength - 1)
					m_iDragRowEnd = m_iPatternLength - 1;

				if (m_iDragColEnd > 15)
					m_iDragColEnd = 15;

				// Move or copy selected pattern data
				stClipData2 ClipData;
				AddUndo();
				Copy(&ClipData);
				if (!m_bControlPressed && !m_bShiftPressed)
					Delete();
				m_iCursorChannel = m_iDragChanStart;
				m_iCursorRow = m_iDragRowStart;
				m_iCursorColumn = m_iDragColStart;//  GetSelectColStart();
				if (m_bShiftPressed)
					PasteMix(&ClipData);
				else
					Paste(&ClipData);

				// Move selection to new point
//				if (m_iSelStartChannel > m_iSelEndChannel) {
					// Swap column order if selection was made backward
					/*
					int Col = m_iSelStartColumn;
					m_iSelStartColumn = m_iSelEndColumn;
					m_iSelEndColumn = Col;
					*/
//				}

				m_iSelStartChannel = m_iDragChanStart;
				m_iSelEndChannel = m_iDragChanEnd;
				m_iSelStartColumn = m_iDragColStart;
				m_iSelEndColumn = m_iDragColEnd;
				m_iSelStartRow = m_iDragRowStart;
				m_iSelEndRow = m_iDragRowEnd;
			}
		}
		else if (m_bDragStart && !m_bDragging) {
			m_bDragStart = false;
			m_bSelecting = false;
		}

		if (m_bSelecting)
			return;

		if (Channel < 0 || Channel > (signed)m_pDocument->GetAvailableChannels() - 1)
			return;
		if (Row < 0)
			return;
		if (Column < 0)
			return;

		m_iCursorRow = Row;
		m_iCursorChannel = Channel;
		m_iCursorColumn = Column;
	}
}

bool CPatternView::OnMouseHover(UINT nFlags, CPoint point)
{
	bool bRedraw = false;

	if (point.y < HEADER_HEIGHT) {
		int Channel = GetChannelAtPoint(point.x);
		int Column = GetColumnAtPoint(point.x);

		if (Channel < 0 || Channel > (signed)m_pDocument->GetAvailableChannels() - 1) {
			bRedraw = m_iMouseHoverEffArrow != 0;
			m_iMouseHoverEffArrow = 0;
			return bRedraw;
		}

		m_iMouseHoverChan = Channel;

		if (Column == 5) {
			if (m_pDocument->GetEffColumns(Channel) > 0) {
				bRedraw = m_iMouseHoverEffArrow != 1;
				m_iMouseHoverEffArrow = 1;
			}
		}
		else if (Column == 6) {
			if (m_pDocument->GetEffColumns(Channel) < (MAX_EFFECT_COLUMNS - 1)) {
				bRedraw = m_iMouseHoverEffArrow != 2;
				m_iMouseHoverEffArrow = 2;
			}
		}
		else {
			bRedraw = m_iMouseHoverEffArrow != 0;
			m_iMouseHoverEffArrow = 0;
		}
	}
	else {
		bRedraw = m_iMouseHoverEffArrow != 0;
		m_iMouseHoverEffArrow = 0;
	}

	return bRedraw;
}

bool CPatternView::OnMouseNcMove()
{
	bool bRedraw = m_iMouseHoverEffArrow != 0;
	m_iMouseHoverEffArrow = 0;
	return bRedraw;	
}

void CPatternView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (point.y < HEADER_HEIGHT) {
		// Channel headers
	}
	else if (point.y > HEADER_HEIGHT) {
		// Pattern area
		int Column = GetColumnAtPoint(point.x);
		int Channel = GetChannelAtPoint(point.x);
		int Row = GetRowAtPoint(point.y);

		if (!m_bSelecting) {
			// Enable selection only if in the pattern field
			if (point.x < (m_iPatternWidth + ROW_COL_WIDTH) && !m_bSelectionInvalid) {
				// Selection threshold
				if (abs(SelStartPoint.x - point.x) > SELECT_THRESHOLD || abs(SelStartPoint.y - point.y) > SELECT_THRESHOLD)
					m_bSelecting = true;
			}
		}
		else {
			if (Row < 0)
				Row = 0;
			if (Row >= m_iPatternLength)
				Row = m_iPatternLength - 1;

			if (Channel < 0)
				Channel = 0;
			if (Channel > (signed)m_pDocument->GetAvailableChannels() - 1) {
				Channel = (signed)m_pDocument->GetAvailableChannels() - 1;
				Column = m_pDocument->GetEffColumns(Channel) * 3 + 4;
			}

			if (m_bDragStart) {
				if (abs(SelStartPoint.x - point.x) > SELECT_THRESHOLD || abs(SelStartPoint.y - point.y) > SELECT_THRESHOLD)
					m_bDragging = true;

				if (m_bDragging) {
					int ChanOffset = Channel - m_iDragPointChan;
					int RowOffset = Row - m_iDragPointRow;

					m_iDragColStart = GetSelectColStart();
					m_iDragColEnd = GetSelectColEnd();

					if (IsSingleChannelSelection() && GetSelectColumn(GetSelectColStart()) >= COLUMN_EFF1) {
						// Allow draggin between effect columns in the same channel
						if (GetSelectColumn(Column) >= COLUMN_EFF1) {
							m_iDragColStart = Column - (((Column - 1) % 3));
						}
						else {
							m_iDragColStart = 4;
						}
						m_iDragColEnd = m_iDragColStart + (GetSelectColEnd() - GetSelectColStart());
					} 

					m_iDragRowStart = GetSelectRowStart() + RowOffset;
					m_iDragRowEnd = GetSelectRowEnd() + RowOffset;
					m_iDragChanStart = GetSelectChanStart() + ChanOffset;
					m_iDragChanEnd = GetSelectChanEnd() + ChanOffset;
				}
			}
			else {
				if (/*point.x < ROW_COL_WIDTH*/ m_bFullRowSelect) {
					m_iSelEndRow = Row;
				}
				else {
					m_iSelEndChannel = Channel;
					m_iSelEndColumn = Column;
					m_iSelEndRow = Row;
				}
			}

			// Scroll
			if ((Row - m_iMiddleRow) == (m_iVisibleRows / 2) - 1) {
				if (m_iCursorRow < m_iPatternLength && m_iMiddleRow < (m_iPatternLength - (m_iVisibleRows / 2))) {
					m_iCursorRow++;
					m_iMiddleRow++;
				}
			}
			else if ((Row - m_iMiddleRow) == -(m_iVisibleRows / 2)) {
				if (m_iCursorRow > 0 && m_iMiddleRow > (m_iVisibleRows / 2)) {
					m_iCursorRow--;
					m_iMiddleRow--;
				}
			}
		}
	}
}

void CPatternView::OnMouseDblClk(CPoint point)
{
	if (point.y < HEADER_HEIGHT) {
		// Channel headers
		int Channel = GetChannelAtPoint(point.x);
		int Column = GetColumnAtPoint(point.x);

		if (Channel < 0 || Channel > (signed)m_pDocument->GetAvailableChannels() - 1)
			return;

		// Solo
		if (Column < 5) {
			m_pView->SoloChannel(Channel);
		}		
		// Remove one track effect column
		else if (Column == 5) {
			if (m_pDocument->GetEffColumns(Channel) > 0)
				m_pDocument->SetEffColumns(Channel, m_pDocument->GetEffColumns(Channel) - 1);
		}
		// Add one track effect column
		else if (Column == 6) {
			if (m_pDocument->GetEffColumns(Channel) < (MAX_EFFECT_COLUMNS - 1))
				m_pDocument->SetEffColumns(Channel, m_pDocument->GetEffColumns(Channel) + 1);
		}
	}
	else if (point.y > HEADER_HEIGHT) {
		// Select whole channel
		SelectAllChannel();
		if (point.x < ROW_COL_WIDTH)
			// Select whole frame
			SelectAll();
	}
}

void CPatternView::OnMouseScroll(int Delta)
{
	int ScrollLength;

	if (Delta != 0) {
		if (Delta < 0)
			ScrollLength = theApp.m_pSettings->General.iPageStepSize;
		else
			ScrollLength = -theApp.m_pSettings->General.iPageStepSize;

		m_iCursorRow += ScrollLength;

		if (m_iCursorRow > (m_iPatternLength - 1)) {
			if (theApp.m_pSettings->General.bWrapFrames) {
				m_iCursorRow -= (m_iPatternLength - 1);
				MoveToFrame(m_iCurrentFrame + 1);
			}
			else
				m_iCursorRow = (m_iPatternLength - 1);
		}
		else if (m_iCursorRow < 0) {
			if (theApp.m_pSettings->General.bWrapFrames) {
				m_iCursorRow += (m_iPrevPatternLength - 1);
				MoveToFrame(m_iCurrentFrame - 1);
			}
			else
				m_iCursorRow = 0;
		}

		m_iMiddleRow = m_iCursorRow;
	}
}

void CPatternView::OnMouseRDown(CPoint point)
{
	if (point.y < HEADER_HEIGHT) {
		// Channel headers
	}
	else if (point.y > HEADER_HEIGHT) {
		// Pattern area
		int Row = GetRowAtPoint(point.y);
		int Channel = GetChannelAtPoint(point.x);
		int Column = GetColumnAtPoint(point.x);

		if (Channel < 0 || Channel > (signed)m_pDocument->GetAvailableChannels() - 1)
			return;
		if (Row < 0)
			return;
		if (Column < 0)
			return;

		m_iCursorRow = Row;
		m_iCursorChannel = Channel;
		m_iCursorColumn = Column;
	}
}

bool CPatternView::CancelDragging()
{
	bool WasDragging = m_bDragging || m_bDragStart;
	m_bDragging = false;
	m_bDragStart = false;
	if (WasDragging)
		m_bSelecting = false;
	return WasDragging;
}

int CPatternView::GetFrame() const
{
	return m_iCurrentFrame;
}

int CPatternView::GetChannel() const
{
	return m_iCursorChannel;
}

int CPatternView::GetRow() const
{
	return m_iCursorRow;
}

int CPatternView::GetColumn() const
{
	return m_iCursorColumn;
}

int CPatternView::GetPlayFrame() const
{
	return m_iPlayFrame;
}

int CPatternView::GetPlayRow() const
{
	return m_iPlayRow;
}


// Selection routines ///////////////////////////////////////////////////////////////////////////////////////

bool CPatternView::IsWithinSelection(int Col, int Channel, int Row) const
{
	if (Row >= GetSelectRowStart() && Row <= GetSelectRowEnd()) {
		if (Channel == GetSelectChanStart() && Channel == GetSelectChanEnd()) {
			if (Col >= GetSelectColStart() && Col <= GetSelectColEnd()) {
				return true;
			}
		}
		else if (Channel == GetSelectChanStart() && Channel != GetSelectChanEnd()) {
			if (Col >= GetSelectColStart()) {
				return true;
			}
		}
		else if (Channel == GetSelectChanEnd() && Channel != GetSelectChanStart()) {
			if (Col <= GetSelectColEnd()) {
				return true;
			}
		}
		else if (Channel >= GetSelectChanStart() && Channel < GetSelectChanEnd()) {
			return true;
		}
	}
	
	return false;
}

int CPatternView::GetSelectRowStart() const 
{
	return (m_iSelEndRow > m_iSelStartRow ? m_iSelStartRow : m_iSelEndRow); 
}

int CPatternView::GetSelectRowEnd() const 
{ 
	return (m_iSelEndRow > m_iSelStartRow ? m_iSelEndRow : m_iSelStartRow); 
}

int CPatternView::GetSelectColStart() const 
{
	int Col;
	if (m_iSelStartChannel == m_iSelEndChannel)
		Col = (m_iSelEndColumn > m_iSelStartColumn ? m_iSelStartColumn : m_iSelEndColumn); 
	else if (m_iSelEndChannel > m_iSelStartChannel)
		Col = m_iSelStartColumn;
	else 
		Col = m_iSelEndColumn;

	switch (Col) {
		case 2: Col = 1; break;
		case 5: case 6: Col = 4; break;
		case 8: case 9: Col = 7; break;
		case 11: case 12: Col = 10; break;
		case 14: case 15: Col = 13; break;
	}
	return Col;
}

int CPatternView::GetSelectColEnd() const 
{ 
	int Col;
	if (m_iSelStartChannel == m_iSelEndChannel)
		Col = (m_iSelEndColumn > m_iSelStartColumn ? m_iSelEndColumn : m_iSelStartColumn); 
	else if (m_iSelEndChannel > m_iSelStartChannel)
		Col = m_iSelEndColumn;
	else
		Col = m_iSelStartColumn;

	switch (Col) {
		case 1: Col = 2; break;
		case 4: case 5: Col = 6; break;
		case 7: case 8: Col = 9; break;
		case 10: case 11: Col = 12; break;
		case 13: case 14: Col = 15; break;
	}

	return Col;
}

int CPatternView::GetSelectChanStart() const 
{ 
	return (m_iSelEndChannel > m_iSelStartChannel ? m_iSelStartChannel : m_iSelEndChannel); 
}

int CPatternView::GetSelectChanEnd() const 
{ 
	return (m_iSelEndChannel > m_iSelStartChannel ? m_iSelEndChannel : m_iSelStartChannel); 
}

// Copy and paste ///////////////////////////////////////////////////////////////////////////////////////////

void CPatternView::Copy(stClipData2 *pClipData)
{
	// Copy selection

	int i, j;
	int Channel = 0;
	int Row;

	stChanNote NoteData;

	pClipData->Channels = GetSelectChanEnd() - GetSelectChanStart() + 1;
	pClipData->StartColumn = GetSelectColumn(GetSelectColStart());
	pClipData->EndColumn = GetSelectColumn(GetSelectColEnd());
	pClipData->Rows = GetSelectRowEnd() - GetSelectRowStart() + 1;

	for (i = GetSelectChanStart(); i <= GetSelectChanEnd(); i++) {
		if (i < 0 || i > (signed)m_pDocument->GetAvailableChannels())
			continue;
		Row = 0;
		for (j = GetSelectRowStart(); j <= GetSelectRowEnd(); j++) {
			if (j < 0 || j > m_iPatternLength)
				continue;

			m_pDocument->GetNoteData(m_iCurrentFrame, i, j, &NoteData);

			if (IsColumnSelected(COLUMN_NOTE, i)) {
				pClipData->Pattern[Channel][Row].Note = NoteData.Note;
				pClipData->Pattern[Channel][Row].Octave = NoteData.Octave;
			}
			if (IsColumnSelected(COLUMN_INSTRUMENT, i)) {
				pClipData->Pattern[Channel][Row].Instrument = NoteData.Instrument;
			}
			if (IsColumnSelected(COLUMN_VOLUME, i)) {
				pClipData->Pattern[Channel][Row].Vol = NoteData.Vol;
			}
			if (IsColumnSelected(COLUMN_EFF1, i)) {
				pClipData->Pattern[Channel][Row].EffNumber[0] = NoteData.EffNumber[0];
				pClipData->Pattern[Channel][Row].EffParam[0] = NoteData.EffParam[0];
			}
			if (IsColumnSelected(COLUMN_EFF2, i)) {
				pClipData->Pattern[Channel][Row].EffNumber[1] = NoteData.EffNumber[1];
				pClipData->Pattern[Channel][Row].EffParam[1] = NoteData.EffParam[1];
			}
			if (IsColumnSelected(COLUMN_EFF3, i)) {
				pClipData->Pattern[Channel][Row].EffNumber[2] = NoteData.EffNumber[2];
				pClipData->Pattern[Channel][Row].EffParam[2] = NoteData.EffParam[2];
			}
			if (IsColumnSelected(COLUMN_EFF4, i)) {
				pClipData->Pattern[Channel][Row].EffNumber[3] = NoteData.EffNumber[3];
				pClipData->Pattern[Channel][Row].EffParam[3] = NoteData.EffParam[3];
			}

			Row++;
		}
		Channel++;
	}
}

void CPatternView::Cut()
{
	// Cut selection
}

void CPatternView::Paste(stClipData2 *pClipData)
{
	// Paste

	int i, j;

	stChanNote NoteData;

	// Special, single channel and effect columns only
	if (pClipData->Channels == 1 && pClipData->StartColumn >= COLUMN_EFF1) {
		int EffOffset, Col;

		for (j = 0; j < pClipData->Rows; j++) {
			if ((j + m_iCursorRow) < 0 || (j + m_iCursorRow) >= m_iPatternLength)
				continue;

			m_pDocument->GetNoteData(m_iCurrentFrame, m_iCursorChannel, j + m_iCursorRow, &NoteData);

			for (i = pClipData->StartColumn - COLUMN_EFF1; i < (pClipData->EndColumn - COLUMN_EFF1 + 1); i++) {
				EffOffset = (GetSelectColumn(m_iCursorColumn) - COLUMN_EFF1) + (i - (pClipData->StartColumn - COLUMN_EFF1));
				Col = i;

				if (EffOffset < 4 && EffOffset >= 0) {
					NoteData.EffNumber[EffOffset] = pClipData->Pattern[0][j].EffNumber[Col];
					NoteData.EffParam[EffOffset] = pClipData->Pattern[0][j].EffParam[Col];
				}
			}
			m_pDocument->SetNoteData(m_iCurrentFrame, m_iCursorChannel, j + m_iCursorRow, &NoteData);
		}
		return;
	}	

	for (i = 0; i < pClipData->Channels; i++) {
		if ((i + m_iCursorChannel) < 0 || (i + m_iCursorChannel) >= (signed)m_pDocument->GetAvailableChannels())
			continue;

		for (j = 0; j < pClipData->Rows; j++) {
			if ((j + m_iCursorRow) < 0 || (j + m_iCursorRow) >= m_iPatternLength)
				continue;

			m_pDocument->GetNoteData(m_iCurrentFrame, i + m_iCursorChannel, j + m_iCursorRow, &NoteData);

			if ((i != 0 || pClipData->StartColumn <= COLUMN_NOTE) && (i != (pClipData->Channels - 1) || pClipData->EndColumn >= COLUMN_NOTE)) {
				NoteData.Note = pClipData->Pattern[i][j].Note;
				NoteData.Octave = pClipData->Pattern[i][j].Octave;
			}
			if ((i != 0 || pClipData->StartColumn <= COLUMN_INSTRUMENT) && (i != (pClipData->Channels - 1) || pClipData->EndColumn >= COLUMN_INSTRUMENT)) {
				NoteData.Instrument = pClipData->Pattern[i][j].Instrument;
			}
			if ((i != 0 || pClipData->StartColumn <= COLUMN_VOLUME) && (i != (pClipData->Channels - 1) || pClipData->EndColumn >= COLUMN_VOLUME)) {
				NoteData.Vol = pClipData->Pattern[i][j].Vol;
			}
			if ((i != 0 || pClipData->StartColumn <= COLUMN_EFF1) && (i != (pClipData->Channels - 1) || pClipData->EndColumn >= COLUMN_EFF1)) {
				NoteData.EffNumber[0] = pClipData->Pattern[i][j].EffNumber[0];
				NoteData.EffParam[0] = pClipData->Pattern[i][j].EffParam[0];
			}
			if ((i != 0 || pClipData->StartColumn <= COLUMN_EFF2) && (i != (pClipData->Channels - 1) || pClipData->EndColumn >= COLUMN_EFF2)) {
				NoteData.EffNumber[1] = pClipData->Pattern[i][j].EffNumber[1];
				NoteData.EffParam[1] = pClipData->Pattern[i][j].EffParam[1];
			}
			if ((i != 0 || pClipData->StartColumn <= COLUMN_EFF3) && (i != (pClipData->Channels - 1) || pClipData->EndColumn >= COLUMN_EFF3)) {
				NoteData.EffNumber[2] = pClipData->Pattern[i][j].EffNumber[2];
				NoteData.EffParam[2] = pClipData->Pattern[i][j].EffParam[2];
			}
			if ((i != 0 || pClipData->StartColumn <= COLUMN_EFF4) && (i != (pClipData->Channels - 1) || pClipData->EndColumn >= COLUMN_EFF4)) {
				NoteData.EffNumber[3] = pClipData->Pattern[i][j].EffNumber[3];
				NoteData.EffParam[3] = pClipData->Pattern[i][j].EffParam[3];
			}

			m_pDocument->SetNoteData(m_iCurrentFrame, i + m_iCursorChannel, j + m_iCursorRow, &NoteData);
		}
	}
}

void CPatternView::PasteMix(stClipData2 *pClipData)
{
	// Paste and mix

	int i, j;

	stChanNote NoteData;

	// Special, single channel and effect columns only
	if (pClipData->Channels == 1 && pClipData->StartColumn >= COLUMN_EFF1) {
		int EffOffset, Col;

		for (j = 0; j < pClipData->Rows; j++) {
			if ((j + m_iCursorRow) < 0 || (j + m_iCursorRow) >= m_iPatternLength)
				continue;

			m_pDocument->GetNoteData(m_iCurrentFrame, m_iCursorChannel, j + m_iCursorRow, &NoteData);

			for (i = pClipData->StartColumn - COLUMN_EFF1; i < (pClipData->EndColumn - COLUMN_EFF1 + 1); i++) {
				EffOffset = (GetSelectColumn(m_iCursorColumn) - COLUMN_EFF1) + (i - (pClipData->StartColumn - COLUMN_EFF1));
				Col = i;

				if (EffOffset < 4 && EffOffset >= 0 && pClipData->Pattern[0][j].EffNumber[Col] > EF_NONE) {
					NoteData.EffNumber[EffOffset] = pClipData->Pattern[0][j].EffNumber[Col];
					NoteData.EffParam[EffOffset] = pClipData->Pattern[0][j].EffParam[Col];
				}
			}
			m_pDocument->SetNoteData(m_iCurrentFrame, m_iCursorChannel, j + m_iCursorRow, &NoteData);
		}
		return;
	}	

	for (i = 0; i < pClipData->Channels; i++) {
		if ((i + m_iCursorChannel) < 0 || (i + m_iCursorChannel) >= (signed)m_pDocument->GetAvailableChannels())
			continue;

		for (j = 0; j < pClipData->Rows; j++) {
			if ((j + m_iCursorRow) < 0 || (j + m_iCursorRow) >= m_iPatternLength)
				continue;

			m_pDocument->GetNoteData(m_iCurrentFrame, i + m_iCursorChannel, j + m_iCursorRow, &NoteData);

			if ((i != 0 || pClipData->StartColumn <= COLUMN_NOTE) && (i != (pClipData->Channels - 1) || pClipData->EndColumn >= COLUMN_NOTE) && pClipData->Pattern[i][j].Note > 0) {
				NoteData.Note = pClipData->Pattern[i][j].Note;
				NoteData.Octave = pClipData->Pattern[i][j].Octave;
			}
			if ((i != 0 || pClipData->StartColumn <= COLUMN_INSTRUMENT) && (i != (pClipData->Channels - 1) || pClipData->EndColumn >= COLUMN_INSTRUMENT) && pClipData->Pattern[i][j].Instrument < MAX_INSTRUMENTS) {
				NoteData.Instrument = pClipData->Pattern[i][j].Instrument;
			}
			if ((i != 0 || pClipData->StartColumn <= COLUMN_VOLUME) && (i != (pClipData->Channels - 1) || pClipData->EndColumn >= COLUMN_VOLUME) && pClipData->Pattern[i][j].Vol < 0x10) {
				NoteData.Vol = pClipData->Pattern[i][j].Vol;
			}
			if ((i != 0 || pClipData->StartColumn <= COLUMN_EFF1) && (i != (pClipData->Channels - 1) || pClipData->EndColumn >= COLUMN_EFF1) && pClipData->Pattern[i][j].EffNumber[0] != EF_NONE) {
				NoteData.EffNumber[0] = pClipData->Pattern[i][j].EffNumber[0];
				NoteData.EffParam[0] = pClipData->Pattern[i][j].EffParam[0];
			}
			if ((i != 0 || pClipData->StartColumn <= COLUMN_EFF2) && (i != (pClipData->Channels - 1) || pClipData->EndColumn >= COLUMN_EFF2) && pClipData->Pattern[i][j].EffNumber[1] != EF_NONE) {
				NoteData.EffNumber[1] = pClipData->Pattern[i][j].EffNumber[1];
				NoteData.EffParam[1] = pClipData->Pattern[i][j].EffParam[1];
			}
			if ((i != 0 || pClipData->StartColumn <= COLUMN_EFF3) && (i != (pClipData->Channels - 1) || pClipData->EndColumn >= COLUMN_EFF3) && pClipData->Pattern[i][j].EffNumber[2] != EF_NONE) {
				NoteData.EffNumber[2] = pClipData->Pattern[i][j].EffNumber[2];
				NoteData.EffParam[2] = pClipData->Pattern[i][j].EffParam[2];
			}
			if ((i != 0 || pClipData->StartColumn <= COLUMN_EFF4) && (i != (pClipData->Channels - 1) || pClipData->EndColumn >= COLUMN_EFF4) && pClipData->Pattern[i][j].EffNumber[3] != EF_NONE) {
				NoteData.EffNumber[3] = pClipData->Pattern[i][j].EffNumber[3];
				NoteData.EffParam[3] = pClipData->Pattern[i][j].EffParam[3];
			}

			m_pDocument->SetNoteData(m_iCurrentFrame, i + m_iCursorChannel, j + m_iCursorRow, &NoteData);
		}
	}
}

void CPatternView::Delete()
{
	// Delete selection

	stChanNote NoteData;

	int i, j;

	for (i = GetSelectChanStart(); i <= GetSelectChanEnd(); i++) {
		if (i < 0 || i > (signed)m_pDocument->GetAvailableChannels())
			continue;

		for (j = GetSelectRowStart(); j <= GetSelectRowEnd(); j++) {
			if (j < 0 || j > m_iPatternLength)
				continue;

			m_pDocument->GetNoteData(m_iCurrentFrame, i, j, &NoteData);

			if (IsColumnSelected(COLUMN_NOTE, i)) {
				NoteData.Note = 0;
				NoteData.Octave = 0;
			}
			if (IsColumnSelected(COLUMN_INSTRUMENT, i)) {
				NoteData.Instrument = 0x40;
			}
			if (IsColumnSelected(COLUMN_VOLUME, i)) {
				NoteData.Vol = 0x10;
			}
			if (IsColumnSelected(COLUMN_EFF1, i)) {
				NoteData.EffNumber[0] = 0x0;
				NoteData.EffParam[0] = 0x0;
			}
			if (IsColumnSelected(COLUMN_EFF2, i)) {
				NoteData.EffNumber[1] = 0x0;
				NoteData.EffParam[1] = 0x0;
			}
			if (IsColumnSelected(COLUMN_EFF3, i)) {
				NoteData.EffNumber[2] = 0x0;
				NoteData.EffParam[2] = 0x0;
			}
			if (IsColumnSelected(COLUMN_EFF4, i)) {
				NoteData.EffNumber[3] = 0x0;
				NoteData.EffParam[3] = 0x0;
			}
		
			m_pDocument->SetNoteData(m_iCurrentFrame, i, j, &NoteData);
		}
	}
}

void CPatternView::RemoveSelectedNotes()
{
	stChanNote NoteData;

	int i, j;

	for (i = GetSelectChanStart(); i <= GetSelectChanEnd(); i++) {
		if (i < 0 || i > (signed)m_pDocument->GetAvailableChannels())
			continue;

		for (j = GetSelectRowStart(); j <= GetSelectRowEnd(); j++) {
			if (j < 0 || j > m_iPatternLength)
				continue;

			m_pDocument->GetNoteData(m_iCurrentFrame, i, j, &NoteData);

			if (IsColumnSelected(COLUMN_NOTE, i)) {
				NoteData.Note = 0;
				NoteData.Octave = 0;
			}
			if (IsColumnSelected(COLUMN_INSTRUMENT, i)) {
				NoteData.Instrument = 0x40;
			}
			if (IsColumnSelected(COLUMN_VOLUME, i)) {
				NoteData.Vol = 0x10;
			}
			if (IsColumnSelected(COLUMN_EFF1, i)) {
				NoteData.EffNumber[0] = 0x0;
				NoteData.EffParam[0] = 0x0;
			}
			if (IsColumnSelected(COLUMN_EFF2, i)) {
				NoteData.EffNumber[1] = 0x0;
				NoteData.EffParam[1] = 0x0;
			}
			if (IsColumnSelected(COLUMN_EFF3, i)) {
				NoteData.EffNumber[2] = 0x0;
				NoteData.EffParam[2] = 0x0;
			}
			if (IsColumnSelected(COLUMN_EFF4, i)) {
				NoteData.EffNumber[3] = 0x0;
				NoteData.EffParam[3] = 0x0;
			}
		
			m_pDocument->SetNoteData(m_iCurrentFrame, i, j, &NoteData);
		}
	}
}

bool CPatternView::IsSelecting() const
{
	return m_bSelecting;
}

void CPatternView::SelectAllChannel()
{
	m_bSelecting = true;
	m_iSelStartRow = 0;
	m_iSelEndRow = m_iPatternLength;
	m_iSelStartChannel = m_iCursorChannel;
	m_iSelEndChannel = m_iCursorChannel;
	m_iSelStartColumn = 0;
	m_iSelEndColumn = m_pDocument->GetEffColumns(m_iCursorChannel) * 3 + 7;
}

void CPatternView::SelectAll()
{
	if (!m_bSelecting)
		SelectAllChannel();
	else {
		m_bSelecting = true;
		m_iSelStartRow = 0;
		m_iSelEndRow = m_iPatternLength;
		m_iSelStartChannel = 0;
		m_iSelEndChannel = m_pDocument->GetAvailableChannels();
		m_iSelStartColumn = 0;
		m_iSelEndColumn = m_pDocument->GetEffColumns(m_pDocument->GetAvailableChannels()) * 3 + 7;
	}
}

void CPatternView::Transpose(int Type)
{
	stChanNote Note;

	int RowStart = GetSelectRowStart();
	int RowEnd = GetSelectRowEnd();
	int ChanStart = GetSelectChanStart();
	int ChanEnd = GetSelectChanEnd();
	int i, j;

	if (!m_bSelecting) {
		RowStart = m_iCursorRow;
		RowEnd = m_iCursorRow;
		ChanStart = m_iCursorChannel;
		ChanEnd = m_iCursorChannel;
	}

	AddUndo();
	
	for (i = ChanStart; i <= ChanEnd; i++) {
		if (!IsColumnSelected(COLUMN_NOTE, i))
			continue;
		for (j = RowStart; j <= RowEnd; j++) {
			m_pDocument->GetNoteData(m_iCurrentFrame, i, j, &Note);
			switch (Type) {
				case TRANSPOSE_DEC_NOTES:
					if (Note.Note > 0 && Note.Note != HALT && Note.Note != RELEASE) {
						if (Note.Note > 1) 
							Note.Note--;
						else {
							if (Note.Octave > 0) {
								Note.Note = B;
								Note.Octave--;
							}
						}
					}
					break;
				case TRANSPOSE_INC_NOTES:
					if (Note.Note > 0 && Note.Note != HALT && Note.Note != RELEASE) {
						if (Note.Note < B)
							Note.Note++;
						else {
							if (Note.Octave < 8) {
								Note.Note = C;
								Note.Octave++;
							}
						}
					}
					break;
				case TRANSPOSE_DEC_OCTAVES:
					if (Note.Octave > 0) 
						Note.Octave--;
					break;
				case TRANSPOSE_INC_OCTAVES:
					if (Note.Octave < 8) 
						Note.Octave++;
					break;
			}
			m_pDocument->SetNoteData(m_iCurrentFrame, i, j, &Note);
		}
	}
}

void CPatternView::ScrollValues(int Type)
{
	stChanNote Note;

	int RowStart = GetSelectRowStart();
	int RowEnd = GetSelectRowEnd();
	int ChanStart = GetSelectChanStart();
	int ChanEnd = GetSelectChanEnd();
	int i, j, k;

	if (!m_bSelecting) {
		RowStart = m_iCursorRow;
		RowEnd = m_iCursorRow;
		ChanStart = m_iCursorChannel;
		ChanEnd = m_iCursorChannel;
	}

	AddUndo();
	
	for (i = ChanStart; i <= ChanEnd; i++) {
		for (k = 1; k < 7; k++) {
			if (!IsColumnSelected(k, i))
				continue;
			for (j = RowStart; j <= RowEnd; j++) {
				m_pDocument->GetNoteData(m_iCurrentFrame, i, j, &Note);
				switch (k) {
					case COLUMN_INSTRUMENT:
						if (Note.Instrument != MAX_INSTRUMENTS) {
							if ((Type < 0 && Note.Instrument > 0) || (Type > 0 && Note.Instrument < MAX_INSTRUMENTS - 1))
								Note.Instrument += Type;
						}
						break;
					case COLUMN_VOLUME:
						if (Note.Vol != 0x10) {
							if ((Type < 0 && Note.Vol > 0) || (Type > 0 && Note.Vol < 0x0F))
								Note.Vol += Type;
						}
						break;
					case COLUMN_EFF1:
					case COLUMN_EFF2:
					case COLUMN_EFF3:
					case COLUMN_EFF4:
						if (Note.EffNumber[k - COLUMN_EFF1] != EF_NONE) {
							if ((Type < 0 && Note.EffParam[k - COLUMN_EFF1] > 0) || (Type > 0 && Note.EffParam[k - COLUMN_EFF1] < 255))
								Note.EffParam[k - COLUMN_EFF1] += Type;
						}
						break;
				}
				m_pDocument->SetNoteData(m_iCurrentFrame, i, j, &Note);
			}
		}
	}
}

void CPatternView::ShiftPressed(bool Pressed)
{
	m_bShiftPressed = Pressed;
}

void CPatternView::ControlPressed(bool Pressed)
{
	m_bControlPressed = Pressed;
}

// Undo / redo //////////////////////////////////////////////////////////////////////////////////////////////

stUndoBlock *CPatternView::SaveUndoState()
{
	stUndoBlock *pUndoBlock;
	int i, j;

	pUndoBlock = new stUndoBlock;

	// Save cursor
	pUndoBlock->Channel	= m_iCursorChannel;
	pUndoBlock->Row		= m_iCursorRow;
	pUndoBlock->Column	= m_iCursorColumn;
	pUndoBlock->Frame	= m_iCurrentFrame;
	pUndoBlock->PatternLen = m_iPatternLength;
	pUndoBlock->Track = m_pDocument->GetSelectedTrack();

	for (i = 0; i < (signed)m_pDocument->GetAvailableChannels(); i++) {
		for (j = 0; j < m_iPatternLength; j++) {
			m_pDocument->GetNoteData(m_iCurrentFrame, i, j, &pUndoBlock->ChannelData[i][j]);
		}
		pUndoBlock->Patterns[i] = m_pDocument->GetPatternAtFrame(m_iCurrentFrame, i);
	}

	return pUndoBlock;
}

void CPatternView::AddUndo()
{
	stUndoBlock *pUndoBlock;
	int i;

	pUndoBlock = SaveUndoState();

	if (m_iUndoLevel < UNDO_LEVELS - 1) {
		m_iUndoLevel++;
		if (m_pUndoStack[m_iUndoLevel - 1])
			delete m_pUndoStack[m_iUndoLevel - 1];
	}
	else {
		if (m_pUndoStack[0])
			delete m_pUndoStack[0];
		for (i = 1; i < UNDO_LEVELS; i++) {
			m_pUndoStack[i - 1] = m_pUndoStack[i];
		}
	}

	m_pUndoStack[m_iUndoLevel - 1] = pUndoBlock;

	m_iRedoLevel = 0;
}

void CPatternView::Undo()
{
	int i, j;

	if (!m_iUndoLevel)
		return;

	if (!m_iRedoLevel) {
		// Save current state
		if (m_pUndoStack[m_iUndoLevel])
			delete m_pUndoStack[m_iUndoLevel];
		m_pUndoStack[m_iUndoLevel] = SaveUndoState();
	}

	m_iRedoLevel++;
	m_iUndoLevel--;

	if (m_pDocument->GetSelectedTrack() != m_pUndoStack[m_iUndoLevel]->Track)
		m_pDocument->SelectTrack(m_pUndoStack[m_iUndoLevel]->Track);

	m_iCursorChannel	= m_pUndoStack[m_iUndoLevel]->Channel;
	m_iCursorRow		= m_pUndoStack[m_iUndoLevel]->Row;
	m_iCursorColumn		= m_pUndoStack[m_iUndoLevel]->Column;
	m_iCurrentFrame		= m_pUndoStack[m_iUndoLevel]->Frame;

	for (i = 0; i < (signed)m_pDocument->GetAvailableChannels(); i++) {
		m_pDocument->SetPatternAtFrame(m_iCurrentFrame, i, m_pUndoStack[m_iUndoLevel]->Patterns[i]);
		for (j = 0; j < m_iPatternLength; j++) {
			m_pDocument->SetNoteData(m_iCurrentFrame, i, j, &m_pUndoStack[m_iUndoLevel]->ChannelData[i][j]);
		}
	}

	CString Text;
	Text.Format(_T("Undo (%i / %i)"), m_iUndoLevel, m_iUndoLevel + m_iRedoLevel);
	m_pView->SetMessageText(Text);
}

void CPatternView::Redo()
{
	int Track = m_pDocument->GetSelectedTrack();

	if (!m_iRedoLevel)
		return;

	m_iUndoLevel++;
	m_iRedoLevel--;

	int i, j;

	if (m_pDocument->GetSelectedTrack() != m_pUndoStack[m_iUndoLevel]->Track)
		m_pDocument->SelectTrack(m_pUndoStack[m_iUndoLevel]->Track);

	m_iCursorChannel	= m_pUndoStack[m_iUndoLevel]->Channel;
	m_iCursorRow		= m_pUndoStack[m_iUndoLevel]->Row;
	m_iCursorColumn		= m_pUndoStack[m_iUndoLevel]->Column;
	m_iCurrentFrame		= m_pUndoStack[m_iUndoLevel]->Frame;

	for (i = 0; i < (signed)m_pDocument->GetAvailableChannels(); i++) {
		m_pDocument->SetPatternAtFrame(m_iCurrentFrame, i, m_pUndoStack[m_iUndoLevel]->Patterns[i]);
		for (j = 0; j < m_iPatternLength; j++) {
			m_pDocument->SetNoteData(m_iCurrentFrame, i, j, &m_pUndoStack[m_iUndoLevel]->ChannelData[i][j]);
		}
	}

	CString Text;
	Text.Format(_T("Redo (%i / %i)"), m_iUndoLevel, m_iUndoLevel + m_iRedoLevel);
	m_pView->SetMessageText(Text);
}

bool CPatternView::CanUndo()
{
	return m_iUndoLevel > 0;
}

bool CPatternView::CanRedo()
{
	return m_iRedoLevel > 0;
}

void CPatternView::ClearSelection()
{
	m_bSelecting = false;
}

// Other ////////////////////////////////////////////////////////////////////////////////////////////////////

int CPatternView::GetCurrentPatternLength(int Frame)
{
	stChanNote Note;
	int i, j, k;
	int Channels = m_pDocument->GetAvailableChannels();
	int PatternLength = m_pDocument->GetPatternLength();	// default length

	if (!theApp.m_pSettings->General.bFramePreview)
		return PatternLength;

	if (Frame < 0 || Frame >= (signed)m_pDocument->GetFrameCount())
		return PatternLength;

	for (j = 0; j < PatternLength; j++) {
		for (i = 0; i < Channels; i++) {
			m_pDocument->GetNoteData(Frame, i, j, &Note);
			for (k = 0; k < (signed)m_pDocument->GetEffColumns(i) + 1; k++) {
				switch (Note.EffNumber[k]) {
					case EF_SKIP:
					case EF_JUMP:
					case EF_HALT:
						return j + 1;
				}
			}
		}
	}

	return PatternLength;
}

void CPatternView::SetHighlight(int Rows, int SecondRows)
{
	m_iHighlight = Rows;
	m_iHighlightSecond = SecondRows;
}

void CPatternView::SetFollowMove(bool bEnable)
{
	m_bFollowMode = bEnable;
}

void CPatternView::SetFocus(bool bFocus)
{
	m_bHasFocus = bFocus;
}

void CPatternView::Interpolate()
{
	stChanNote NoteData;
	int StartRow = GetSelectRowStart();
	int EndRow = GetSelectRowEnd();
	float StartVal, EndVal, Delta;
	int i, j, k;
	int Effect;

	if (!m_bSelecting)
		return;

	AddUndo();

	for (i = GetSelectChanStart(); i <= GetSelectChanEnd(); i++) {
		for (k = 0; k < (signed)m_pDocument->GetEffColumns(i) + 2; k++) {
			switch (k) {
				case 0:	// Volume
					if (!IsColumnSelected(COLUMN_VOLUME, i))
						continue;
					m_pDocument->GetNoteData(m_iCurrentFrame, i, StartRow, &NoteData);
					if (NoteData.Vol == 0x10)
						continue;
					StartVal = (float)NoteData.Vol;
					m_pDocument->GetNoteData(m_iCurrentFrame, i, EndRow, &NoteData);
					if (NoteData.Vol == 0x10)
						continue;
					EndVal = (float)NoteData.Vol;
					break;
				case 1:	// Effect 1
				case 2:
				case 3:
				case 4:
					if (!IsColumnSelected(COLUMN_EFF1 + k - 1, i))
						continue;
					m_pDocument->GetNoteData(m_iCurrentFrame, i, StartRow, &NoteData);
					if (NoteData.EffNumber[k - 1] == EF_NONE)
						continue;
					StartVal = (float)NoteData.EffParam[k - 1];
					Effect = NoteData.EffNumber[k - 1];
					m_pDocument->GetNoteData(m_iCurrentFrame, i, EndRow, &NoteData);
					if (NoteData.EffNumber[k - 1] == EF_NONE)
						continue;
					EndVal = (float)NoteData.EffParam[k - 1];
					break;
			}

			Delta = (EndVal - StartVal) / float(EndRow - StartRow);

			for (j = StartRow; j < EndRow; j++) {
				m_pDocument->GetNoteData(m_iCurrentFrame, i, j, &NoteData);
				switch (k) {
					case 0: 
						NoteData.Vol = (int)StartVal; 
						break;
					case 1: 
					case 2: 
					case 3: 
					case 4:
						NoteData.EffNumber[k - 1] = Effect;
						NoteData.EffParam[k - 1] = (int)StartVal; 
						break;
				}
				StartVal += Delta;
				m_pDocument->SetNoteData(m_iCurrentFrame, i, j, &NoteData);
			}
		}
	}
}


void CPatternView::Reverse()
{
	stChanNote ReverseBuffer[MAX_PATTERN_LENGTH];
	stChanNote NoteData;
	int StartRow = GetSelectRowStart();
	int EndRow = GetSelectRowEnd();
	int i, j, k, m;

	if (!m_bSelecting)
		return;

	AddUndo();

	for (i = GetSelectChanStart(); i <= GetSelectChanEnd(); i++) {
		// Copy the selected rows
		for (j = StartRow, m = 0; j < EndRow + 1; j++, m++) {
			m_pDocument->GetNoteData(m_iCurrentFrame, i, j, ReverseBuffer + m);
		}
		// Paste reversed
		for (j = EndRow, m = 0; j > StartRow - 1; j--, m++) {
			m_pDocument->GetNoteData(m_iCurrentFrame, i, j, &NoteData);
			if (IsColumnSelected(COLUMN_NOTE, i)) {
				NoteData.Note = ReverseBuffer[m].Note;
				NoteData.Octave = ReverseBuffer[m].Octave;
			}
			if (IsColumnSelected(COLUMN_INSTRUMENT, i))
				NoteData.Instrument = ReverseBuffer[m].Instrument;
			if (IsColumnSelected(COLUMN_VOLUME, i))
				NoteData.Vol = ReverseBuffer[m].Vol;
			for (k = 0; k < 4; k++) {
				if (IsColumnSelected(k + COLUMN_EFF1, i)) {
					NoteData.EffNumber[k] = ReverseBuffer[m].EffNumber[k];
					NoteData.EffParam[k] = ReverseBuffer[m].EffParam[k];
				}
			}
			m_pDocument->SetNoteData(m_iCurrentFrame, i, j, &NoteData);
		}
	}
}
