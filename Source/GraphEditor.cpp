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

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "GraphEditor.h"
#include "SequenceEditor.h"

#define ENABLE_RELEASE

// CGraphEditor

// The graphical sequence editor

IMPLEMENT_DYNAMIC(CGraphEditor, CWnd)

BEGIN_MESSAGE_MAP(CGraphEditor, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_TIMER()
END_MESSAGE_MAP()


CGraphEditor::CGraphEditor(CSequence *pSequence) : m_pBackDC(NULL), m_pBitmap(NULL), m_pSmallFont(NULL)
{
	m_pSequence = pSequence;
	m_iLastPlayPos = 0;

	m_iStartLineX = 0;
	m_iStartLineY = 0;
	m_iEndLineX = 0;
	m_iEndLineY = 0;
}

CGraphEditor::~CGraphEditor()
{
	if (m_pSmallFont)
		delete m_pSmallFont;

	if (m_pBackDC)
		delete m_pBackDC;

	if (m_pBitmap)
		delete m_pBitmap;
}

BOOL CGraphEditor::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

BOOL CGraphEditor::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, LPVOID lpParam)
{
	if (CWnd::CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, lpParam) == -1)
		return -1;

	LOGFONT LogFont;

	const char *SMALL_FONT_FACE = "Verdana";

	m_pSmallFont = new CFont();

	memset(&LogFont, 0, sizeof LOGFONT);
	memcpy(LogFont.lfFaceName, SMALL_FONT_FACE, strlen(SMALL_FONT_FACE));
	LogFont.lfHeight = -10;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	m_pSmallFont->CreateFontIndirect(&LogFont);

	// Calculate the draw areas
	GetClientRect(m_GraphRect);
	GetClientRect(m_BottomRect);
	GetClientRect(m_ClientRect);

	m_GraphRect.top += GRAPH_BOTTOM * 2 + 3;
	m_GraphRect.bottom -= GRAPH_BOTTOM;
	m_GraphRect.left += GRAPH_LEFT;
	m_GraphRect.bottom = (m_GraphRect.bottom / 15) * 15;
	m_BottomRect.left += GRAPH_LEFT;
	m_BottomRect.top = m_GraphRect.bottom;

	m_pParentWnd = pParentWnd;

	RedrawWindow();

	CDC *pDC = GetDC();

	m_pBackDC = new CDC();
	m_pBitmap = new CBitmap();
	m_pBackDC->CreateCompatibleDC(pDC);
	m_pBitmap->CreateCompatibleBitmap(pDC, m_ClientRect.Width(), m_ClientRect.Height());
	m_pBackDC->SelectObject(m_pBitmap);

	ReleaseDC(pDC);

	Initialize();

	CWnd::SetTimer(0, 30, NULL);

	return 0;
}

void CGraphEditor::Initialize()
{
	// Allow extra initialization
}

void CGraphEditor::OnTimer(UINT nIDEvent)
{
	if (m_pSequence) {
		int Pos = m_pSequence->GetPlayPos();
		if (Pos != m_iLastPlayPos) {
			m_iCurrentPlayPos = Pos;
			RedrawWindow();
		}
		m_iLastPlayPos = Pos;
	}
}

void CGraphEditor::OnPaint()
{
	CPaintDC dc(this);
}

void CGraphEditor::PaintBuffer(CDC *pBackDC, CDC *pFrontDC)
{
	pFrontDC->BitBlt(0, 0, m_ClientRect.Width(), m_ClientRect.Height(), pBackDC, 0, 0, SRCCOPY);
}

void CGraphEditor::DrawBackground(CDC *pDC, int Lines)
{
	// Fill background
	pDC->FillSolidRect(m_ClientRect, 0);
	pDC->Draw3dRect(m_GraphRect, 0x808080, 0xC0C0C0);

	// Fill bottom area
	pDC->FillSolidRect(m_BottomRect, 0x404040);

	if (m_pSequence) {
		int ItemWidth = GetItemWidth();

		// Draw horizontal bars
		for (unsigned int i = 1; i < m_pSequence->GetItemCount(); i += 2) {
			int x = m_GraphRect.left + i * ItemWidth + 1;
			int y = m_GraphRect.top + 1;
			int w = ItemWidth;
			int h = m_GraphRect.Height() - 2;
			pDC->FillSolidRect(x, y, w, h, 0x202020);
		}
	}

	if (Lines > 0) {
		int StepHeight = m_GraphRect.Height() / Lines;

		// Draw vertical bars
		for (int i = 0; i < Lines; i++) {
			int x = m_GraphRect.left + 1;
			int y = m_GraphRect.top + StepHeight * i;
			int w = m_GraphRect.Width() - 2;
			int h = 1;
			pDC->FillSolidRect(x, y, w, h, COLOR_LINES);
		}
	}
}

void CGraphEditor::DrawRange(CDC *pDC, int Max, int Min)
{
	CFont *pOldFont = pDC->SelectObject(m_pSmallFont);
	CString line;

	pDC->FillSolidRect(m_ClientRect.left, m_GraphRect.top, m_GraphRect.left, m_GraphRect.bottom, 0x00);

	pDC->SetBkMode(OPAQUE);
	pDC->SetTextColor(0xFFFFFF);
	pDC->SetBkColor(pDC->GetPixel(0, 0));	// Ugly but works

	line.Format("%02i", Max);
	pDC->TextOut(2, m_GraphRect.top - 3, line);

	line.Format("%02i", Min);
	pDC->TextOut(2, m_GraphRect.bottom - 13, line);

	pDC->SelectObject(pOldFont);
}

void CGraphEditor::DrawLoopPoint(CDC *pDC, int StepWidth)
{
	// Draw loop point
	int LoopPoint = m_pSequence->GetLoopPoint();

	CFont *pOldFont = pDC->SelectObject(m_pSmallFont);

	if (LoopPoint > -1) {
		int x = StepWidth * LoopPoint + GRAPH_LEFT + 1;

		pDC->FillSolidRect(x + 1, m_BottomRect.top, m_BottomRect.right - x, m_BottomRect.bottom, 0x606000);
		pDC->FillSolidRect(x, m_BottomRect.top, 1, m_BottomRect.bottom, 0xF0F000);

		pDC->SetTextColor(0xFFFFFF);
		pDC->SetBkMode(TRANSPARENT);
		pDC->TextOut(x + 4, m_BottomRect.top, "Loop");
	}

	pDC->SelectObject(pOldFont);
}

void CGraphEditor::DrawReleasePoint(CDC *pDC, int StepWidth)
{
	// Draw loop point
	int ReleasePoint = m_pSequence->GetReleasePoint();

	CFont *pOldFont = pDC->SelectObject(m_pSmallFont);

	if (ReleasePoint > -1) {
		int x = StepWidth * ReleasePoint + GRAPH_LEFT + 1;

		pDC->FillSolidRect(x + 1, m_BottomRect.top, m_BottomRect.right - x, m_BottomRect.bottom, 0x600060);
		pDC->FillSolidRect(x, m_BottomRect.top, 1, m_BottomRect.bottom, 0xF000F0);

		pDC->SetTextColor(0xFFFFFF);
		pDC->SetBkMode(TRANSPARENT);
		pDC->TextOut(x + 4, m_BottomRect.top, "Release");
	}

	pDC->SelectObject(pOldFont);
}

void CGraphEditor::DrawLine(CDC *pDC)
{
	if (m_iStartLineX != 0 && m_iStartLineY != 0) {
		CPen *OldPen, Pen;
		Pen.CreatePen(1, 3, 0xFFFFFF);
		OldPen = pDC->SelectObject(&Pen);
		pDC->MoveTo(m_iStartLineX, m_iStartLineY);
		pDC->LineTo(m_iEndLineX, m_iEndLineY);
		pDC->SelectObject(OldPen);
	}
}

void CGraphEditor::DrawRect(CDC *pDC, int x, int y, int w, int h, bool Highlight)
{
	if (Highlight) {
		pDC->FillSolidRect(x, y, w, h, 0x50C050);
		pDC->Draw3dRect(x, y, w, h, 0x80FF80, 0x408040);
	}
	else {
		pDC->FillSolidRect(x, y, w, h, 0xC0C0C0);
		pDC->Draw3dRect(x, y, w, h, 0xFFFFFF, 0x808080);
	}
}

int CGraphEditor::GetItemWidth()
{
	if (!m_pSequence || m_pSequence->GetItemCount() == 0)
		return 0;

	int Width = m_GraphRect.Width();
	Width = (Width / m_pSequence->GetItemCount()) * m_pSequence->GetItemCount();

	Width = Width / m_pSequence->GetItemCount();

	if (Width > ITEM_MAX_WIDTH)
		Width = ITEM_MAX_WIDTH;

	return Width;
}

int CGraphEditor::GetItemHeight()
{
	// Virtual function
	return 0;
}

void CGraphEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();

	if (point.y < m_GraphRect.bottom) {
		m_iEditing = EDIT_POINT;
		ModifyItem(point, true);
		if (point.x > GRAPH_LEFT)
			CursorChanged(point.x - GRAPH_LEFT);
	}
	else if (point.y > m_GraphRect.bottom) {
		m_iEditing = EDIT_LOOP;
		ModifyLoopPoint(point, true);
	}
}

void CGraphEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
}

void CGraphEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	if (nFlags & MK_LBUTTON) {
		switch (m_iEditing) {
			case EDIT_POINT:
				ModifyItem(point, true);
				break;
			case EDIT_LOOP:
				ModifyLoopPoint(point, true);
				break;
		}
	}
	else if (nFlags & MK_RBUTTON) {
		switch (m_iEditing) {
			case EDIT_LINE:	{
				int StartX, StartY, x;
				int EndX, EndY;
				float DeltaY, fY;

				int PosX = point.x;
				int PosY = point.y;

				if (m_iEditing == EDIT_LINE) {
					m_iEndLineX = PosX;
					m_iEndLineY = PosY;

					StartX = (m_iStartLineX < m_iEndLineX ? m_iStartLineX : m_iEndLineX);
					StartY = (m_iStartLineX < m_iEndLineX ? m_iStartLineY : m_iEndLineY);
					EndX = (m_iStartLineX > m_iEndLineX ? m_iStartLineX : m_iEndLineX);
					EndY = (m_iStartLineX > m_iEndLineX ? m_iStartLineY : m_iEndLineY);

					DeltaY = float(EndY - StartY) / float((EndX - StartX) + 1);
					fY = float(StartY);

					for (x = StartX; x < EndX; x++) {
						ModifyItem(CPoint(x, (int)fY), false);
						fY += DeltaY;
					}

					RedrawWindow();
					m_pParentWnd->PostMessage(WM_SEQUENCE_CHANGED, 1);

					//ModifyItem(CPoint(x, (int)fY), true);
				}
				}
				break;
			case EDIT_RELEASE:
				ModifyReleasePoint(point, true);
				break;
		}
	}

	// Notify parent
	if (m_pSequence->GetItemCount() > 0 && point.x > GRAPH_LEFT) {
		CursorChanged(point.x - GRAPH_LEFT);
	}
	else
		m_pParentWnd->PostMessage(WM_CURSOR_CHANGE, 0, 0);
}

void CGraphEditor::OnRButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();

	if (point.y < m_GraphRect.bottom) {
		m_iStartLineX = m_iEndLineX = point.x;
		m_iStartLineY = m_iEndLineY = point.y;
		m_iEditing = EDIT_LINE;
	}
	else if (point.y > m_GraphRect.bottom) {
		m_iEditing = EDIT_RELEASE;
		ModifyReleasePoint(point, true);
	}

	CWnd::OnRButtonDown(nFlags, point);
}

void CGraphEditor::OnRButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();

	m_iStartLineX = m_iStartLineY = 0;
	m_iEditing = EDIT_NONE;
	RedrawWindow();
	CWnd::OnRButtonUp(nFlags, point);
}

void CGraphEditor::ModifyItem(CPoint point, bool Redraw)
{
	if (Redraw) {
		RedrawWindow(NULL);
		m_pParentWnd->PostMessage(WM_SEQUENCE_CHANGED, 1);
	}
}

void CGraphEditor::ModifyLoopPoint(CPoint point, bool Redraw)
{
	if (!m_pSequence || !m_pSequence->GetItemCount())
		return;

	int ItemWidth = GetItemWidth();
	int LoopPoint = (point.x - GRAPH_LEFT + (ItemWidth / 2)) / ItemWidth;

	// Disable loop point by dragging it to far right
	if (LoopPoint >= (int)m_pSequence->GetItemCount())
		LoopPoint = -1;

	m_pSequence->SetLoopPoint(LoopPoint);

	if (Redraw) {
		RedrawWindow(NULL);
		m_pParentWnd->PostMessage(WM_SEQUENCE_CHANGED, 1);
	}
}

void CGraphEditor::ModifyReleasePoint(CPoint point, bool Redraw)
{
	if (!m_pSequence || !m_pSequence->GetItemCount())
		return;

	// Temporarily disabled
//	return;
#ifndef ENABLE_RELEASE
	return;
#endif

	int ItemWidth = GetItemWidth();
	int ReleasePoint = (point.x - GRAPH_LEFT + (ItemWidth / 2)) / ItemWidth;

	// Disable loop point by dragging it to far right
	if (ReleasePoint >= (int)m_pSequence->GetItemCount())
		ReleasePoint = -1;

	m_pSequence->SetReleasePoint(ReleasePoint);

	if (Redraw) {
		RedrawWindow(NULL);
		m_pParentWnd->PostMessage(WM_SEQUENCE_CHANGED, 1);
	}
}

void CGraphEditor::CursorChanged(int x)
{
	if (m_pSequence->GetItemCount() == 0)
		return;

	int Pos = x / ((m_GraphRect.Width() - 2) / m_pSequence->GetItemCount());
	int Value = m_pSequence->GetItem(Pos);
	m_pParentWnd->PostMessage(WM_CURSOR_CHANGE, Pos, Value);
}

// Bar graph editor (volume and duty setting)

void CBarGraphEditor::OnPaint()
{
	CPaintDC dc(this);
	
	CDC *pDC = m_pBackDC;

	if (!pDC)
		return;

	DrawBackground(pDC, m_iItems);
	DrawRange(pDC, m_iItems, 0);

	// Return now if no sequence is selected
	if (!m_pSequence) {
		PaintBuffer(m_pBackDC, &dc);
		return;
	}

	// Draw items
	int Count = m_pSequence->GetItemCount();

	if (!Count) {
		PaintBuffer(m_pBackDC, &dc);
		return;
	}

	int StepWidth = GetItemWidth();
	int StepHeight = m_GraphRect.Height() / m_iItems;

	// Draw items
	for (int i = 0; i < Count; i++) {
		int x = m_GraphRect.left + i * StepWidth + 1;
		int y = m_GraphRect.top + StepHeight * (m_iItems - m_pSequence->GetItem(i));
		int w = StepWidth;
		int h = StepHeight * m_pSequence->GetItem(i);
		DrawRect(pDC, x, y, w, h, m_iCurrentPlayPos == i);
	}
	
	DrawLoopPoint(pDC, StepWidth);
	DrawReleasePoint(pDC, StepWidth);
	DrawLine(pDC);

	PaintBuffer(m_pBackDC, &dc);
}

void CBarGraphEditor::ModifyItem(CPoint point, bool Redraw)
{
	int ItemIndex;
	int ItemValue;

	if (!m_pSequence || !m_pSequence->GetItemCount())
		return;

	int ItemWidth = GetItemWidth();
	int ItemHeight = GetItemHeight();

	ItemIndex = (point.x - GRAPH_LEFT) / ItemWidth;
	ItemValue = m_iItems - (((point.y - m_GraphRect.top) + (ItemHeight / 2)) / ItemHeight);

	if (ItemValue < 0)
		ItemValue = 0;
	if (ItemValue > m_iItems)
		ItemValue = m_iItems;

	if (ItemIndex < 0 || ItemIndex >= (int)m_pSequence->GetItemCount())
		return;

	m_pSequence->SetItem(ItemIndex, ItemValue);

	CGraphEditor::ModifyItem(point, Redraw);
}

int CBarGraphEditor::GetItemHeight()
{
	return m_GraphRect.Height() / m_iItems;
}

// Arpeggio graph editor

IMPLEMENT_DYNAMIC(CArpeggioGraphEditor, CGraphEditor)

BEGIN_MESSAGE_MAP(CArpeggioGraphEditor, CGraphEditor)
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

CArpeggioGraphEditor::CArpeggioGraphEditor(CSequence *pSequence) : CGraphEditor(pSequence), m_iScrollOffset(0), m_pScrollBar(NULL)
{
}

CArpeggioGraphEditor::~CArpeggioGraphEditor()
{
	if (m_pScrollBar)
		delete m_pScrollBar;
}

void CArpeggioGraphEditor::Initialize()
{
	// Setup scrollbar
	const int SCROLLBAR_WIDTH = 18;
	SCROLLINFO info;

	m_pScrollBar = new CScrollBar();

	m_GraphRect.right -= SCROLLBAR_WIDTH;
	m_pScrollBar->Create(SBS_VERT | SBS_LEFTALIGN | WS_CHILD | WS_VISIBLE, CRect(m_GraphRect.right, m_GraphRect.top, m_GraphRect.right + SCROLLBAR_WIDTH, m_GraphRect.bottom), this, 0);

	info.cbSize = sizeof(SCROLLINFO);
	info.fMask = SIF_ALL;

	if (m_pSequence->GetSetting() == 0) {
		info.nMax = 192;
		info.nMin = 0;
		info.nPage = 10;
		info.nPos = 96;
	}
	else {
		info.nMax = 96;
		info.nMin = 0;
		info.nPage = 1;
		info.nPos = 96;
	}

	m_iScrollOffset = 0;

	m_pScrollBar->SetScrollInfo(&info);
	m_pScrollBar->ShowScrollBar(TRUE);
	m_pScrollBar->EnableScrollBar(ESB_ENABLE_BOTH);

	m_ClientRect.right -= SCROLLBAR_WIDTH;

	CGraphEditor::Initialize();
}

CString CArpeggioGraphEditor::GetNoteString(int Value)
{
	const char NOTES_A[] = {'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'};
	const char NOTES_B[] = {'-', '#', '-', '#', '-', '-', '#', '-', '#', '-', '#', '-'};
	const char NOTES_C[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

	CString line;
	int Octave, Note;

	Octave = Value / 12;
	Note = Value % 12;

	line.Format("%c%c%c", NOTES_A[Note], NOTES_B[Note], NOTES_C[Octave]);

	return line;
}

void CArpeggioGraphEditor::DrawRange(CDC *pDC, int Max, int Min)
{
	const char NOTES_A[] = {'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'};
	const char NOTES_B[] = {'-', '#', '-', '#', '-', '-', '#', '-', '#', '-', '#', '-'};
	const char NOTES_C[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

	CString line;

	if (m_pSequence->GetSetting() == 0)
		CGraphEditor::DrawRange(pDC, Max, Min);
	else {
		pDC->FillSolidRect(m_ClientRect.left, m_GraphRect.top, m_GraphRect.left, m_GraphRect.bottom, 0x00);

		CFont *pOldFont = pDC->SelectObject(m_pSmallFont);

//		pDC->SetBkMode(OPAQUE);
		pDC->SetTextColor(0xFFFFFF);
		pDC->SetBkColor(0);

		int NoteValue, Octave, Note;

		// Top
		NoteValue = m_iScrollOffset + 20;
		Octave = NoteValue / 12;
		Note = NoteValue % 12;

		//line.Format("%c%c%c", NOTES_A[Note], NOTES_B[Note], NOTES_C[Octave]);
		line = GetNoteString(NoteValue);
		pDC->TextOut(2, m_GraphRect.top - 3, line);

		// Bottom
		NoteValue = m_iScrollOffset;
		Octave = NoteValue / 12;
		Note = NoteValue % 12;

		line.Format("%c%c%c", NOTES_A[Note], NOTES_B[Note], NOTES_C[Octave]);
		pDC->TextOut(2, m_GraphRect.bottom - 13, line);

		pDC->SelectObject(pOldFont);
	}
}

void CArpeggioGraphEditor::OnPaint()
{
	CPaintDC dc(this);

	CDC *pDC = m_pBackDC;
	if (!pDC)
		return;

	DrawBackground(pDC, ITEMS + 1);
	DrawRange(pDC, m_iScrollOffset + 10, m_iScrollOffset - 10);

	// Return now if no sequence is selected
	if (!m_pSequence) {
		PaintBuffer(m_pBackDC, &dc);
		return;
	}

	// Draw items
	int Count = m_pSequence->GetItemCount();

	if (!Count) {
		PaintBuffer(m_pBackDC, &dc);
		return;
	}

	int StepWidth = GetItemWidth();
	int StepHeight = m_GraphRect.Height() / ITEMS;

	// One last line
	pDC->FillSolidRect(m_GraphRect.left + 1, m_GraphRect.top + 20 * StepHeight, m_GraphRect.Width() - 2, 1, COLOR_LINES);

	// Draw items
	for (int i = 0; i < Count; i++) {
		int item = (ITEMS / 2) - m_pSequence->GetItem(i) + m_iScrollOffset;

		if (m_pSequence->GetSetting() == 1)
			item += (ITEMS / 2);

		if (item >= 0 && item <= 20) {
			int x = m_GraphRect.left + i * StepWidth + 1;
			int y = m_GraphRect.top + StepHeight * item + 1;
			int w = StepWidth - 1;
			int h = StepHeight - 1;
			DrawRect(pDC, x, y, w, h, m_iCurrentPlayPos == i);
		}
	}

	DrawLoopPoint(pDC, StepWidth);
	DrawReleasePoint(pDC, StepWidth);
	DrawLine(pDC);

	PaintBuffer(m_pBackDC, &dc);
}

void CArpeggioGraphEditor::ModifyItem(CPoint point, bool Redraw)
{
	int ItemIndex;
	int ItemValue;

	if (!m_pSequence || !m_pSequence->GetItemCount())
		return;

	int ItemWidth = GetItemWidth();
	int ItemHeight = GetItemHeight();

	ItemIndex = (point.x - GRAPH_LEFT) / ItemWidth;

	if (m_pSequence->GetSetting() == 0) {
		// Relative
		ItemValue = (ITEMS / 2) - ((point.y - m_GraphRect.top) / ItemHeight) + m_iScrollOffset;
		if (ItemValue < -96)
			ItemValue = -96;
		if (ItemValue > 96)
			ItemValue = 96;
	}
	else {
		// Absolute
		ItemValue = ITEMS - ((point.y - m_GraphRect.top) / ItemHeight) + m_iScrollOffset;
		if (ItemValue < 0)
			ItemValue = 0;
		if (ItemValue > 96)
			ItemValue = 96;
	}

	if (ItemIndex < 0 || ItemIndex >= (int)m_pSequence->GetItemCount())
		return;

	m_pSequence->SetItem(ItemIndex, ItemValue);

	CGraphEditor::ModifyItem(point, Redraw);
}

int CArpeggioGraphEditor::GetItemHeight()
{
	return m_GraphRect.Height() / ITEMS;
}

void CArpeggioGraphEditor::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	switch (nSBCode) {
		case SB_LINEDOWN: 
			m_iScrollOffset--;
			break;
		case SB_LINEUP:
			m_iScrollOffset++;
			break;
		case SB_PAGEDOWN:
			m_iScrollOffset -= 10;
			break;
		case SB_PAGEUP:
			m_iScrollOffset += 10;
			break;
		case SB_TOP:
			m_iScrollOffset = 96;
			break;
		case SB_BOTTOM:	
			m_iScrollOffset = (m_pSequence->GetSetting() == 0) ? -96 : 0;
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			m_iScrollOffset = 96 - (int)nPos;
			break;
		case SB_ENDSCROLL:
		default:
			return;
	}

	if (m_iScrollOffset > 96)
		m_iScrollOffset = 96;

	if (m_pSequence->GetSetting() == 0) {
		if (m_iScrollOffset < -96)
			m_iScrollOffset = -96;
	}
	else {
		if (m_iScrollOffset < 0)
			m_iScrollOffset = 0;
	}

	pScrollBar->SetScrollPos(96 - m_iScrollOffset);

	RedrawWindow();

	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

BOOL CArpeggioGraphEditor::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	m_pScrollBar->SetScrollPos(m_iScrollOffset + zDelta);
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

// Pitch graph editor

void CPitchGraphEditor::OnPaint()
{
	CPaintDC dc(this);

	CDC *pDC = m_pBackDC;

	if (!pDC)
		return;

	DrawBackground(pDC, 0);
	DrawRange(pDC, 127, -128);

	pDC->FillSolidRect(m_GraphRect.left, m_GraphRect.top + m_GraphRect.Height() / 2, m_GraphRect.Width(), 1, COLOR_LINES);

	// Return now if no sequence is selected
	if (!m_pSequence) {
		PaintBuffer(m_pBackDC, &dc);
		return;
	}

	// Draw items
	int Count = m_pSequence->GetItemCount();

	if (!Count) {
		PaintBuffer(m_pBackDC, &dc);
		return;
	}

	int StepWidth = GetItemWidth();
	int StepHeight = m_GraphRect.Height() / ITEMS;

	// One last line
	pDC->FillSolidRect(m_GraphRect.left + 1, m_GraphRect.top + 20 * StepHeight, m_GraphRect.Width() - 2, 1, COLOR_LINES);

	// Draw items
	for (int i = 0; i < Count; i++) {
		int item = m_pSequence->GetItem(i);
		int x = m_GraphRect.left + i * StepWidth;
		int y = m_GraphRect.top + m_GraphRect.Height() / 2;
		int w = StepWidth - 1;
		int h = -(item * m_GraphRect.Height()) / 255 ;
		DrawRect(pDC, x, y, w, h, m_iCurrentPlayPos == i);
	}

	DrawLoopPoint(pDC, StepWidth);
	DrawReleasePoint(pDC, StepWidth);
	DrawLine(pDC);

	PaintBuffer(m_pBackDC, &dc);
}

int CPitchGraphEditor::GetItemHeight()
{
	return m_GraphRect.Height() / ITEMS;
}

void CPitchGraphEditor::ModifyItem(CPoint point, bool Redraw)
{
	int ItemIndex;
	int ItemValue;

	if (!m_pSequence || !m_pSequence->GetItemCount())
		return;

	int ItemWidth = GetItemWidth();
	int ItemHeight = GetItemHeight();

	int MouseY = (point.y - m_GraphRect.top) - (m_GraphRect.Height() / 2);

	ItemIndex = (point.x - GRAPH_LEFT) / ItemWidth;
	ItemValue = -(MouseY * 255) / m_GraphRect.Height();

	if (ItemValue < -128)
		ItemValue = -128;
	if (ItemValue > 127)
		ItemValue = 127;

	if (ItemIndex < 0 || ItemIndex >= (int)m_pSequence->GetItemCount())
		return;

	m_pSequence->SetItem(ItemIndex, ItemValue);

	CGraphEditor::ModifyItem(point, Redraw);
}
