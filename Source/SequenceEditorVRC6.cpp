/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2007  Jonathan Liss
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
#include <cmath>
#include "FamiTracker.h"
#include "FamiTrackerView.h"
#include "SequenceEditor.h"
#include "SequenceEditorVRC6.h"
#include "InstrumentSettings.h"
#include "InstrumentSettingsVRC6.h"

static const char *SEQUENCE_NAMES[] = {"Volume",
								"Arpeggio",
								"Pitch bend",
								"Hi-speed pitch bend",
								"Square pulse width"};

static const int EDITOR_LEFT	= 60;
static const int EDITOR_TOP		= 30;
static const int EDITOR_WIDTH	= 280;
static const int EDITOR_HEIGHT	= 150;

static const int LENGTH_HEIGHT	= 150;
static const int LENGTH_TOP		= 30;
static const int LENGTH_LEFT	= 10;
static const int LENGTH_WIDTH	= 18;
static const int LOOP_TOP		= 72;
static const int COORD_TOP		= 165;

enum EDIT {
	EDIT_NONE,
	EDIT_SEQUENCE,
	EDIT_LOOP,
	EDIT_SCROLL,
	EDIT_LINE
};

static const int NUM_MIN[] = {0, -10, -127, -127, 0};
static const int NUM_MAX[] = {15, 10, 126, 126, 3};

static const unsigned int COLOR_BARS = 0xC0A020;
static const unsigned int COLOR_BARS_HIGHLIGHT = 0xF0C060;

static bool IsInsideSequenceWin(int PosX, int PosY)
{
	if ((PosY > EDITOR_TOP) && (PosY < EDITOR_TOP + EDITOR_HEIGHT) && PosX > EDITOR_LEFT && PosX < EDITOR_LEFT + EDITOR_WIDTH)
		return true;

	return false;
}


// CSequenceEditorVRC6

IMPLEMENT_DYNAMIC(CSequenceEditorVRC6, CWnd)
CSequenceEditorVRC6::CSequenceEditorVRC6()
{
	m_pParent				= NULL;
	m_pBackDC				= NULL;
	m_iLastValueX			= 0;
	m_iLastValueY			= 0;
	m_iLoopPoint			= -1;
	m_iSeqOrigin			= 0;	
	m_iSequenceType			= 0;
	m_iSequenceItemCount	= 16;
	m_bScrollLock			= false;

	m_iStartLineX			= m_iStartLineY = 0;

}

CSequenceEditorVRC6::~CSequenceEditorVRC6()
{
}

BEGIN_MESSAGE_MAP(CSequenceEditorVRC6, CWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_TIMER()
END_MESSAGE_MAP()

void CSequenceEditorVRC6::SetParent(CInstrumentSettingsVRC6 *pParent)
{
	m_pParent = pParent;
}

CString CSequenceEditorVRC6::CompileList()
{
	// Create an MML string

	CString String;
	int i;

	for (i = 0; i < m_iSequenceItemCount; i++) {
		if (m_iLoopPoint == i)
			String.AppendFormat("| ");

		String.AppendFormat("%i ",  m_List->GetItem(i));
	}
	
	return String;
}

void CSequenceEditorVRC6::SetCurrentSequence(int Type, CSequence *List)
{
	m_List = List;

	m_iSequenceType = Type;

	m_iSequenceItemCount	= m_List->GetItemCount();
	m_iLoopPoint			= m_List->GetLoopPoint();

	m_iSeqOrigin	= 0;
	m_iMin			= NUM_MIN[Type];
	m_iMax			= NUM_MAX[Type];
	m_iLevels		= (m_iMax - m_iMin);
}

// CSequenceEditorVRC6 message handlers

void CSequenceEditorVRC6::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	if (!m_pBackDC)
		return;

	CString	Text;
	RECT	WinRect;

	GetWindowRect(&WinRect);

	int Width = WinRect.right - WinRect.left;
	int Height = WinRect.bottom - WinRect.top;

	m_iWidth	= Width;
	m_iHeight	= Height;

	int ColorBg		= theApp.m_pSettings->Appearance.iColBackground;
	int ColorHiBg	= theApp.m_pSettings->Appearance.iColBackgroundHilite;
	int ColorText	= theApp.m_pSettings->Appearance.iColPatternText;
	int ColorHiText	= theApp.m_pSettings->Appearance.iColPatternTextHilite;
	
	int ColorHiTextLo	= DIM(ColorHiText, 60);
	int ColorHiTextHi	= DIM(ColorHiText, 80);

	m_pBackDC->SelectObject(&m_Font);

	ClearBackground();

	// Title
	m_pBackDC->SetBkMode(TRANSPARENT);
	m_pBackDC->SetTextColor(0xFFFFFF);

	Text.Format("Sequence / %s", SEQUENCE_NAMES[m_iSequenceType]);
	m_pBackDC->TextOut(2, 2, Text);

	m_pBackDC->FillSolidRect(0, 21, m_iWidth, 1, 0xFFFFFF);

	DrawScrollBar(LENGTH_LEFT, LENGTH_TOP, LENGTH_WIDTH, LENGTH_HEIGHT);

	switch (m_iSequenceType) {
		case 0: DrawBarEditor(15);		break;
		case 1: DrawArpeggioEditor();	break;
		case 2: DrawPitchEditor();		break;
		case 3: DrawPitchEditor();		break;
		case 4: DrawBarEditor(7);	break;
	}

	// Prepare graphics
	m_pBackDC->SelectObject(&m_SmallFont);

	int Spacing = 15;

	if (m_iStartLineX != 0 && m_iStartLineY != 0) {
		CPen *OldPen, Pen;
		Pen.CreatePen(1, 3, 0xFFFFFF);
		OldPen = m_pBackDC->SelectObject(&Pen);
		m_pBackDC->MoveTo(m_iStartLineX, m_iStartLineY);
		m_pBackDC->LineTo(m_iEndLineX, m_iEndLineY);
		m_pBackDC->SelectObject(OldPen);
	}

	// Display window
	DisplayArea(&dc);
}

void CSequenceEditorVRC6::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_bSequenceSizing	= false;
	m_bLoopMoving		= false;
	m_iEditing			= EDIT_NONE;
	m_iStartLineX = m_iStartLineY = 0;

	SetFocus();

	CheckEditing(point.x, point.y);

	if ((point.x > 29 && point.x < 61)) {
		if ((point.y > (LENGTH_TOP + 16) && point.y < (LENGTH_TOP + 32))) {
			m_bSequenceSizing = true;
		}
	}

	CWnd::OnLButtonDown(nFlags, point);
}

void CSequenceEditorVRC6::OnMouseMove(UINT nFlags, CPoint point)
{
	static int LastPointY;
	static bool Twice;
	bool FixLoop = false;

	if ((nFlags & MK_LBUTTON) != 0) {
		CheckEditing(point.x, point.y);
	}
	else if ((nFlags & MK_RBUTTON) != 0)
		DrawLine(point.x, point.y);
	else {
		int WndWidth = (EDITOR_WIDTH - 2);
		int Width;

		if (m_iSequenceType == MOD_ARPEGGIO)
			WndWidth = EDITOR_WIDTH - 18;

		if (m_iSequenceItemCount > 0)
			Width = WndWidth / m_iSequenceItemCount;
		else
			Width = WndWidth;
		
		if (Width < 1)
			Width = 1;

		m_iLastValueX = (point.x - EDITOR_LEFT) / Width;
		m_iLastValueY = m_List->GetItem(m_iLastValueX);

		RedrawWindow();
	}

	CWnd::OnMouseMove(nFlags, point);
}

void CSequenceEditorVRC6::DrawLine(int PosX, int PosY) 
{
	int StartX, StartY, x;
	int EndX, EndY;
	float DeltaY, fY;

	if (IsInsideSequenceWin(PosX, PosY) && m_iEditing == EDIT_LINE) {
		m_iEndLineX = PosX;
		m_iEndLineY = PosY;

		StartX = (m_iStartLineX < m_iEndLineX ? m_iStartLineX : m_iEndLineX);
		StartY = (m_iStartLineX < m_iEndLineX ? m_iStartLineY : m_iEndLineY);
		EndX = (m_iStartLineX > m_iEndLineX ? m_iStartLineX : m_iEndLineX);
		EndY = (m_iStartLineX > m_iEndLineX ? m_iStartLineY : m_iEndLineY);

		DeltaY = float(EndY - StartY) / float((EndX - StartX) + 1);
		fY = float(StartY);

		for (x = StartX; x < EndX; x++) {
			ChangeItems(x, int(fY), true);
			fY += DeltaY;
		}

		ChangeItems(x, int(fY), false);
	}
}

void CSequenceEditorVRC6::CheckEditing(int PosX, int PosY)
{
	// Length box
	HandleScrollBar(LENGTH_LEFT, LENGTH_TOP, LENGTH_WIDTH, LENGTH_HEIGHT, PosX, PosY);

	if (!((PosX > EDITOR_LEFT) && (PosX < EDITOR_LEFT + EDITOR_WIDTH)))
		return;

	if (m_iEditing == EDIT_NONE) {
		// Decide what to do
		if ((PosY > EDITOR_TOP + EDITOR_HEIGHT) && (PosY < EDITOR_TOP + EDITOR_HEIGHT + 20)) {
			m_iEditing = EDIT_LOOP;
		}
		else if ((m_iSequenceType == MOD_ARPEGGIO) && (PosX - EDITOR_LEFT) > (EDITOR_WIDTH - 16) && PosY > (EDITOR_TOP + 20)) {
			m_iEditing = EDIT_SCROLL;
		}
		else {
			m_iEditing = EDIT_SEQUENCE;
		}
	}

	switch (m_iEditing) {
		case EDIT_SEQUENCE:
			if ((PosY > EDITOR_TOP) && (PosY < EDITOR_TOP + EDITOR_HEIGHT) && PosX > EDITOR_LEFT && PosX < EDITOR_LEFT + EDITOR_WIDTH)
				ChangeItems(PosX, PosY, false);
			break;
		case EDIT_LOOP:
			ChangeLoopPoint(PosX, PosY);
			break;
		case EDIT_SCROLL:
			if (m_iSequenceType == MOD_ARPEGGIO) {
				m_iSeqOrigin = (PosY - EDITOR_TOP) - EDITOR_HEIGHT / 2;

				if (m_iSeqOrigin > (EDITOR_HEIGHT / 2) - 6)
					m_iSeqOrigin = (EDITOR_HEIGHT / 2) - 6;
				if (m_iSeqOrigin < -(EDITOR_HEIGHT / 2) + 6)
					m_iSeqOrigin = -(EDITOR_HEIGHT / 2) + 6;
			}
			else if (m_iSequenceType == MOD_PITCH || m_iSequenceType == MOD_HIPITCH) {
				m_iSeqOrigin = (PosY - EDITOR_TOP) - EDITOR_HEIGHT / 2;

				if (m_iSeqOrigin > (EDITOR_HEIGHT / 2) - 6)
					m_iSeqOrigin = (EDITOR_HEIGHT / 2) - 6;
				if (m_iSeqOrigin < -(EDITOR_HEIGHT / 2) + 6)
					m_iSeqOrigin = -(EDITOR_HEIGHT / 2) + 6;

				m_iMax = 60 + m_iSeqOrigin * 2;
				m_iMin = -(60 + m_iSeqOrigin);

				m_iLevels = (m_iMax - m_iMin);
			}

			RedrawWindow();
			break;
	}
}

void CSequenceEditorVRC6::ChangeLoopPoint(int PosX, int PosY)
{
	if (m_iSequenceItemCount == 0)
		return;

	int Width = EDITOR_WIDTH / m_iSequenceItemCount;
	
	if (Width < 1)
		Width = 1;

	int Item = ((PosX + Width / 2) - EDITOR_LEFT) / Width;

	if (Item == m_iSequenceItemCount)
		Item = -1;

	m_iLoopPoint = Item;
	m_List->SetLoopPoint(m_iLoopPoint);

	m_pParent->CompileSequence();
	RedrawWindow();
}

void CSequenceEditorVRC6::ChangeItems(int PosX, int PosY, bool Quick)
{
	if (m_iSequenceItemCount == 0)
		return;

	int WndWidth = (EDITOR_WIDTH - 2);

	if (m_iSequenceType == MOD_ARPEGGIO)
		WndWidth = EDITOR_WIDTH - 18;

	int Width = WndWidth / m_iSequenceItemCount;
	
	if (Width < 1)
		Width = 1;

	int Item = (PosX - EDITOR_LEFT) / Width;
	int Value = 0;

	if (Item > (m_iSequenceItemCount - 1))
		Item = (m_iSequenceItemCount - 1);

	switch (m_iSequenceType) {
		case 0:
			PosY += EDITOR_HEIGHT / 30;
			Value = 15 - ((PosY - EDITOR_TOP) * 15) / EDITOR_HEIGHT;
			LIMIT(Value, 15, 0);
			break;
		case 1:
			PosY += EDITOR_HEIGHT / (m_iLevels + 1);
			Value = m_iMax + 1 - ((PosY - EDITOR_TOP) * (m_iLevels + 1)) / EDITOR_HEIGHT - m_iSeqOrigin;
			LIMIT(Value, 126, -127);
			break;
		case 2:
		case 3:
			PosY += EDITOR_HEIGHT / 256;
			Value = 128 - ((PosY - EDITOR_TOP) * 255) / EDITOR_HEIGHT;
			LIMIT(Value, 126, -127);
			break;
		case 4:
			PosY += EDITOR_HEIGHT / 14;
			Value = 7 - ((PosY - EDITOR_TOP) * 7) / EDITOR_HEIGHT;
			LIMIT(Value, 7, 0);
			break;
	}

	m_List->SetItem(Item, Value);
	m_iLastValueY = Value;
	m_iLastValueX = Item;

	if (!Quick) {
		m_pParent->CompileSequence();
		RedrawWindow();
	}
}

BOOL CSequenceEditorVRC6::PreCreateWindow(CREATESTRUCT& cs)
{
	m_bSequenceSizing	= false;
	m_bLoopMoving		= false;
	m_iEditing			= EDIT_NONE;

	return CWnd::PreCreateWindow(cs);
}

void CSequenceEditorVRC6::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bSequenceSizing	= false;
	m_bLoopMoving		= false;
	m_bScrollLock		= false;
	m_iEditing			= EDIT_NONE;
	m_iButtonPressed	= 0;

	RedrawWindow();

	CWnd::OnLButtonUp(nFlags, point);
}

void CSequenceEditorVRC6::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	OnLButtonDown(nFlags, point);
	CWnd::OnLButtonDblClk(nFlags, point);
}

BOOL CSequenceEditorVRC6::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message) {
		case WM_KEYDOWN:
			static_cast<CFamiTrackerView*>(theApp.GetView())->PreviewNote((unsigned char)pMsg->wParam);
			return TRUE;
		case WM_KEYUP:
			static_cast<CFamiTrackerView*>(theApp.GetView())->PreviewRelease((unsigned char)pMsg->wParam);
			return TRUE;
	}	

	return CWnd::PreTranslateMessage(pMsg);
}

void CSequenceEditorVRC6::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (IsInsideSequenceWin(point.x, point.y)) {
		m_iStartLineX = m_iEndLineX = point.x;
		m_iStartLineY = m_iEndLineY = point.y;
		m_iEditing = EDIT_LINE;
	}

	CWnd::OnRButtonDown(nFlags, point);
}

void CSequenceEditorVRC6::OnRButtonUp(UINT nFlags, CPoint point)
{
	m_iStartLineX = m_iStartLineY = 0;
	m_iEditing = EDIT_NONE;
	RedrawWindow();
	CWnd::OnRButtonUp(nFlags, point);
}

void CSequenceEditorVRC6::HandleScrollBar(int x, int y, int Width, int Height, int PosX, int PosY)
{
	if (PosX < x || PosX > (x + Width) || PosY < y || PosY > (y + Height))
		return;

	// Direct
	if (PosY >= (y + 16) && (PosY <= (y + Height - 16))) {
		m_iSequenceItemCount = (MAX_ITEMS - 1) - ((PosY - (y + 16)) * (MAX_ITEMS - 1)) / (Height - 32); // (127 * (y + Height - PosY - 32)) / (;
		m_iButtonPressed = BUTTON_LENGTH;
	}
	// Add
	else if (PosY >= y && PosY <= (y + 16)) {
		if (m_iSequenceItemCount < MAX_ITEMS)
			m_iSequenceItemCount++;
		m_iButtonPressed = BUTTON_ADD;
	}
	// Remove
	else if (PosY >= (y + Height - 16) && PosY <= (y + Height)) {
		if (m_iSequenceItemCount > 0)
			m_iSequenceItemCount--;
		m_iButtonPressed = BUTTON_REMOVE;
	}

	m_List->SetItemCount(m_iSequenceItemCount);

	RedrawWindow();
	m_pParent->CompileSequence();
}

// GUI

void CSequenceEditorVRC6::DisplayArea(CDC *pDC)
{
	pDC->BitBlt(0, 0, m_iWidth, m_iHeight, m_pBackDC, 0, 0, SRCCOPY);
}

void CSequenceEditorVRC6::ClearBackground()
{
	int i;

	m_pBackDC->FillSolidRect(0, 0, m_iWidth, m_iHeight, 0x102010);

	for (i = 0; i < 21; i++) {
		m_pBackDC->FillSolidRect(0, i, m_iWidth, 1, DIM_TO(0x804040, 0x102010, 100 - (i * 100) / 21));
	}

	for (i = 0; i < (m_iHeight - 21); i++) {
		m_pBackDC->FillSolidRect(0, i + 21, m_iWidth, 1, DIM_TO(0x604020, 0x102010, int(sinf((float(i) * 3.14f) / (m_iHeight - 21)) * 50.0f + 50.0f)));
	}

}

void CSequenceEditorVRC6::DrawLine(int x1, int y1, int x2, int y2)
{
	m_pBackDC->MoveTo(x1, y1);
	m_pBackDC->LineTo(x2, y2);
}

void CSequenceEditorVRC6::DrawScrollBar(int x, int y, int Width, int Height)
{
	int BarHeight = ((Height - 32) * m_iSequenceItemCount) / (MAX_ITEMS - 1);
	int BarStart = y + (Height - BarHeight) - 16;

	int ColorBg		= 0x303030;
	int ColorLight	= 0xC0C0C0;
	int ColorDark	= 0x808080;
	int ColCursor	= 0xA0A0A0;

	m_pBackDC->FillSolidRect(x, y, Width, Height, ColorBg);
	
	if (m_iButtonPressed == BUTTON_ADD)
		m_pBackDC->Draw3dRect(x, y, Width, 16, ColorDark, ColorLight);
	else
		m_pBackDC->Draw3dRect(x, y, Width, 16, ColorLight, ColorDark);

	if (m_iButtonPressed == BUTTON_REMOVE)
		m_pBackDC->Draw3dRect(x, y + Height - 16, Width, 16, ColorDark, ColorLight);
	else
		m_pBackDC->Draw3dRect(x, y + Height - 16, Width, 16, ColorLight, ColorDark);

	m_pBackDC->FillSolidRect(x, y + 16, 1, Height - 32, 0x808080);
	m_pBackDC->FillSolidRect(x + Width - 1, y + 16, 1, Height - 32, 0x808080);

	m_pBackDC->FillSolidRect(x + 1, BarStart, Width - 2, BarHeight, 0xA0A0A0);

	if (m_iButtonPressed == BUTTON_LENGTH)
		m_pBackDC->Draw3dRect(x + 1, BarStart, Width - 2, BarHeight, ColorDark, ColorLight);
	else
		m_pBackDC->Draw3dRect(x + 1, BarStart, Width - 2, BarHeight, ColorLight, ColorDark);

	CString Text;

	Text.Format("+");
	m_pBackDC->TextOut(x + 5, y + 1, Text);

	Text.Format("-");
	m_pBackDC->TextOut(x + 7, y + Height - 14, Text);

	// Not a part of the length bar

	if (m_iSequenceItemCount == 0)
		Text.Format("Length: # 0, 0 ms");
	else
		Text.Format("Length: # %i, %i ms {%i: %i}", m_iSequenceItemCount, m_iSequenceItemCount * (1000 / static_cast<CFamiTrackerDoc*>(theApp.GetDocument())->GetFrameRate()), m_iLastValueX, m_iLastValueY);
	
	m_pBackDC->TextOut(EDITOR_LEFT, EDITOR_TOP + EDITOR_HEIGHT + 6, Text);
}

void CSequenceEditorVRC6::CreateBackground(int Space)
{
	const int COL_BG = 0x808080;
	int i;

	m_pBackDC->FillSolidRect(EDITOR_LEFT, EDITOR_TOP, EDITOR_WIDTH, EDITOR_HEIGHT, COL_BG);
	m_pBackDC->Draw3dRect(EDITOR_LEFT, EDITOR_TOP, EDITOR_WIDTH, EDITOR_HEIGHT, 0x404040, 0xC0C0C0);

	if (Space > 0) {
		int MarkersHeight = EDITOR_HEIGHT - (EDITOR_HEIGHT / Space);

		for (i = 0; i < MarkersHeight; i += (MarkersHeight / (Space - 1))) {
			m_pBackDC->FillSolidRect(EDITOR_LEFT + 2, EDITOR_TOP + i + (EDITOR_HEIGHT / Space), EDITOR_WIDTH - 3, 1, 0xA0A0A0);
		}
	}
}

void CSequenceEditorVRC6::DrawArpeggioEditor()
{
	const int COL_BG = 0x808080;
	const int COL_TEXT = 0xFFFFFF;
	int i;

	CreateBackground(20);

	// Arpeggio scrollbar

	m_pBackDC->FillSolidRect(EDITOR_LEFT + EDITOR_WIDTH - 15, EDITOR_TOP + 1, 14, EDITOR_HEIGHT - 2, 0x808080);
	m_pBackDC->Draw3dRect(EDITOR_LEFT + EDITOR_WIDTH - 15, EDITOR_TOP + 1, 14, EDITOR_HEIGHT - 2, 0x606060, 0xC0C0C0);
	m_pBackDC->FillSolidRect(EDITOR_LEFT + EDITOR_WIDTH - 14, EDITOR_TOP + m_iSeqOrigin + 69, 12, 10, 0xA0A0A0);
	m_pBackDC->Draw3dRect(EDITOR_LEFT + EDITOR_WIDTH - 14, EDITOR_TOP + m_iSeqOrigin + 69, 12, 10, 0xC0C0C0, 0x606060);

	CString Text;

	Text.Format("%i", m_iMax - m_iSeqOrigin);
	m_pBackDC->TextOut(EDITOR_LEFT - 22, EDITOR_TOP - 4, Text);
	Text.Format("%i", m_iMin - m_iSeqOrigin);
	m_pBackDC->TextOut(EDITOR_LEFT - 22, EDITOR_TOP + EDITOR_HEIGHT - 6, Text);

	if (m_iSequenceItemCount > 0) {
		int ItemWidth = (EDITOR_WIDTH - 18) / m_iSequenceItemCount;
		int ItemHeight, ItemCenter;
		int ItemLeft = EDITOR_LEFT + 1;
		bool InRange = true;

		if (ItemWidth < 1)
			ItemWidth = 1;

		for (i = 0; i < m_iSequenceItemCount; i++) {
			if (i == m_iLoopPoint) {
				// Draw loop point
				Text.Format("^");
				m_pBackDC->TextOut(ItemLeft - 4, EDITOR_TOP + EDITOR_HEIGHT - 1, Text);
				m_pBackDC->FillSolidRect(ItemLeft - 1, EDITOR_TOP, 1, EDITOR_HEIGHT, COL_TEXT);
			}

			ItemHeight = (EDITOR_HEIGHT - 2) / 20;
			ItemCenter = EDITOR_TOP + EDITOR_HEIGHT / 2 - 4 - (/*m_iSequenceItems[i]*/ m_List->GetItem(i) + m_iSeqOrigin) * ItemHeight;
			InRange = ItemCenter > EDITOR_TOP && ItemCenter < (EDITOR_TOP + EDITOR_HEIGHT);

			if (InRange) {
				m_pBackDC->FillSolidRect(ItemLeft, ItemCenter-1, ItemWidth, ItemHeight + 2, DIM(COLOR_BARS, 80));
				m_pBackDC->Draw3dRect(ItemLeft, ItemCenter-1, ItemWidth, ItemHeight + 2, COLOR_BARS, DIM(COLOR_BARS, 50));
			}

			ItemLeft += ItemWidth;
		}
	}
}

void CSequenceEditorVRC6::DrawPitchEditor()
{
	const int COL_BG = 0x808080;
	const int COL_TEXT = 0xFFFFFF;
	int i;

	CreateBackground(2);

	m_pBackDC->SetBkMode(TRANSPARENT);
	m_pBackDC->SetTextColor(COL_TEXT);

	m_pBackDC->TextOut(EDITOR_LEFT - 15, EDITOR_TOP - 4, "15");
	m_pBackDC->TextOut(EDITOR_LEFT - 11, EDITOR_TOP + EDITOR_HEIGHT - 6, "0");

	if (m_iSequenceItemCount > 0) {
		int ItemWidth = (EDITOR_WIDTH - 2) / m_iSequenceItemCount;
		int ItemHeight, ItemCenter;
		int ItemLeft = EDITOR_LEFT + 1;
		bool InRange = true;

		if (ItemWidth < 1)
			ItemWidth = 1;

		for (i = 0; i < m_iSequenceItemCount; i++) {
			if (i == m_iLoopPoint) {
				// Draw loop point
				m_pBackDC->TextOut(ItemLeft - 4, EDITOR_TOP + EDITOR_HEIGHT - 1, "^");
				m_pBackDC->FillSolidRect(ItemLeft - 1, EDITOR_TOP, 1, EDITOR_HEIGHT, COL_TEXT);
			}

			ItemHeight = -m_List->GetItem(i) * EDITOR_HEIGHT / 255;
			ItemCenter = EDITOR_TOP + EDITOR_HEIGHT / 2;

			if (InRange) {
				if (ItemHeight < -1 || ItemHeight > 1) {
					m_pBackDC->FillSolidRect(ItemLeft, ItemCenter, ItemWidth, ItemHeight, DIM(COLOR_BARS, 80));
					m_pBackDC->Draw3dRect(ItemLeft, ItemCenter, ItemWidth, ItemHeight, COLOR_BARS, DIM(COLOR_BARS, 50));
				}
				else
					m_pBackDC->FillSolidRect(ItemLeft, ItemCenter, ItemWidth, ItemHeight - 2, DIM(COLOR_BARS, 80));
			}

			ItemLeft += ItemWidth;
		}
	}

}

void CSequenceEditorVRC6::DrawBarEditor(int Levels)
{
	const int COL_BG = 0x808080;
	const int COL_TEXT = 0xFFFFFF;
	int i;

	CreateBackground(Levels);

	m_pBackDC->SetBkMode(TRANSPARENT);
	m_pBackDC->SetTextColor(COL_TEXT);

	if (m_iSequenceItemCount > 0) {

		int ItemWidth = (EDITOR_WIDTH - 2) / m_iSequenceItemCount;
		int ItemHeight, ItemCenter;
		int ItemLeft = EDITOR_LEFT + 1;

		if (ItemWidth < 1)
			ItemWidth = 1;

		for (i = 0; i < m_iSequenceItemCount; i++) {
			if (i == m_iLoopPoint) {
				// Draw loop point
				m_pBackDC->TextOut(ItemLeft - 4, EDITOR_TOP + EDITOR_HEIGHT - 1, "^");
				m_pBackDC->FillSolidRect(ItemLeft - 1, EDITOR_TOP, 1, EDITOR_HEIGHT, COL_TEXT);
			}

			ItemHeight = -m_List->GetItem(i) * EDITOR_HEIGHT / Levels + 3;
			ItemCenter = EDITOR_TOP + EDITOR_HEIGHT - 2;

			if (ItemHeight < -1) {
				m_pBackDC->FillSolidRect(ItemLeft, ItemCenter, ItemWidth, ItemHeight, DIM(COLOR_BARS, 80));
				m_pBackDC->Draw3dRect(ItemLeft, ItemCenter, ItemWidth, ItemHeight, COLOR_BARS, DIM(COLOR_BARS, 50));
			}
			else
				m_pBackDC->FillSolidRect(ItemLeft, ItemCenter, ItemWidth, ItemHeight - 2, DIM(COLOR_BARS, 80));

			ItemLeft += ItemWidth;
		}
	}
}

////

BOOL CSequenceEditorVRC6::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, LPVOID lpParam)
{
	BOOL bResult = CWnd::CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, lpParam);

	CDC *pDC = GetDC();
	RECT WinRect;
	LOGFONT LogFont;

	const char *FONT_FACE = "Fixedsys";
	const char *SMALL_FONT_FACE = "Verdana";
	const char *BUTTON_FONT_FACE = "Verdana";

	m_pBackDC = new CDC;

	GetWindowRect(&WinRect);

	m_Bitmap.CreateCompatibleBitmap(pDC, WinRect.right, WinRect.bottom);
	m_pBackDC->CreateCompatibleDC(pDC);
	m_pOldBitmap = m_pBackDC->SelectObject(&m_Bitmap);

	memset(&LogFont, 0, sizeof LOGFONT);
	memcpy(LogFont.lfFaceName, FONT_FACE, strlen(FONT_FACE));
	LogFont.lfHeight = -12;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	m_Font.CreateFontIndirect(&LogFont);

	memset(&LogFont, 0, sizeof LOGFONT);
	memcpy(LogFont.lfFaceName, SMALL_FONT_FACE, strlen(SMALL_FONT_FACE));
	LogFont.lfHeight = -8;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	m_SmallFont.CreateFontIndirect(&LogFont);

	memset(&LogFont, 0, sizeof LOGFONT);
	memcpy(LogFont.lfFaceName, BUTTON_FONT_FACE, strlen(BUTTON_FONT_FACE));
	LogFont.lfHeight = -10;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	m_ButtonFont.CreateFontIndirect(&LogFont);

	m_pOldFont = m_pBackDC->SelectObject(&m_Font);

	m_iSwitchGroup1 = 0;
	m_iSwitchGroup2 = 0;
	m_iSwitchGroup3 = 0;

	return bResult;
}

BOOL CSequenceEditorVRC6::DestroyWindow()
{
	m_pBackDC->SelectObject(m_pOldBitmap);
	m_pBackDC->SelectObject(m_pOldFont);

	delete m_pBackDC;

	m_Bitmap.DeleteObject();
	m_Font.DeleteObject();
	m_SmallFont.DeleteObject();
	m_ButtonFont.DeleteObject();

	return CWnd::DestroyWindow();
}

void CSequenceEditorVRC6::OnTimer(UINT nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	RedrawWindow();
	CWnd::OnTimer(nIDEvent);
}
