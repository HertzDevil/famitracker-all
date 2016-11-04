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

#include "stdafx.h"
#include "FamiTracker.h"
#include "SequenceEditor.h"
#include "InstrumentSettings.h"
#include "..\include\sequenceeditor.h"

const char *SEQUENCE_NAMES[] = {"Volume", 
								"Arpeggio", 
								"Pitch", 
								"Hi-pitch", 
								"Duty cycle & noise mode"};

const int EDITOR_LEFT	= 90;
const int EDITOR_TOP	= 40;
const int EDITOR_WIDTH	= 250;
const int EDITOR_HEIGHT	= 140;

const int LENGTH_TOP	= 30;
const int LOOP_TOP		= 72;
const int COORD_TOP		= 165;

enum EDIT {
	EDIT_NONE,
	EDIT_SEQUENCE,
	EDIT_LOOP,
	EDIT_SCROLL,
	EDIT_LINE
};

bool IsInsideSequenceWin(int PosX, int PosY)
{
	if ((PosY > EDITOR_TOP) && (PosY < EDITOR_TOP + EDITOR_HEIGHT) && PosX > EDITOR_LEFT && PosX < EDITOR_LEFT + EDITOR_WIDTH)
		return true;

	return false;
}


// CSequenceEditor

IMPLEMENT_DYNAMIC(CSequenceEditor, CWnd)
CSequenceEditor::CSequenceEditor()
{
	m_iLastValueX = 0;
	m_iLastValueY = 0;
	m_iLoopPoint = -1;
	m_iSeqOrigin = 0;
	
	m_pParent = NULL;

	m_iSequenceType = 0;
	m_iSequenceItemCount = 16;

	m_iStartLineX = m_iStartLineY = 0;

	m_bScrollLock = false;
}

CSequenceEditor::~CSequenceEditor()
{
}


BEGIN_MESSAGE_MAP(CSequenceEditor, CWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

void CSequenceEditor::SetParent(CInstrumentSettings *pParent)
{
	m_pParent = pParent;
}

CString CSequenceEditor::CompileList()
{
	// Create an MML

	CString String;
	int i;

	for (i = 0; i < m_iSequenceItemCount; i++) {
		if (m_iLoopPoint == i)
			String.AppendFormat("| ");

		String.AppendFormat("%i ", m_iSequenceItems[i]);
	}
	
	return String;
}

const int NUM_MIN[] = {0, -15, -127, -127, 0};
const int NUM_MAX[] = {15, 15, 126, 126, 3};

void CSequenceEditor::SetCurrentSequence(int Type, stSequence *List)
{
	unsigned int i;
	int j, x = 0, val;
	bool LoopEnabled = false;

	m_iLoopPoint = -1;

	memset(m_iSequenceItems, 0, sizeof(int) * MAX_ITEMS);
	m_List = List;

	m_iSequenceType = Type;

	for (i = 0; i < List->Count; i++) {
		if (List->Length[i] < 0) {
			LoopEnabled = true;
			m_iLoopPoint = x;
			for (j = signed(List->Count + List->Length[i] - 1); j < signed(List->Count - 1); j++)
				m_iLoopPoint -= (List->Length[j] + 1);
		}
		else {
			for (j = 0; j < (List->Length[i] + 1); j++) {
				val = List->Value[i];
				switch (m_iSequenceType) {
					case MOD_VOLUME: LIMIT(val, 15, 0); break;
					case MOD_DUTYCYCLE: LIMIT(val, 3, 0); break;
				}
				m_iSequenceItems[x++] = val;
			}
		}
	}

	m_iSequenceItemCount = x;

	if (LoopEnabled) {
		if (m_iLoopPoint < 0)
			m_iLoopPoint = 0;
	}
	else {
		m_iLoopPoint = x;
	}

	m_iSeqOrigin	= 0;
	m_iMin			= NUM_MIN[Type];
	m_iMax			= NUM_MAX[Type];
	m_iLevels		= (m_iMax - m_iMin);
}

// CSequenceEditor message handlers

void CSequenceEditor::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	const char *FONT_FACE = "Fixedsys";
	const char *SMALL_FONT_FACE = "Verdana";

	CString	Text;

	CDC		dcBack;
	CBitmap	Bmp, *OldBmp;

	RECT	WinRect;
	LOGFONT LogFont;
	CFont	Font, SmallFont, *OldFont;

	GetWindowRect(&WinRect);

	int Width = WinRect.right;
	int Height = WinRect.bottom;

	
	int ColorBg		= theApp.m_pSettings->Appearance.iColBackground;
	int ColorHiBg	= theApp.m_pSettings->Appearance.iColBackgroundHilite;
	int ColorText	= theApp.m_pSettings->Appearance.iColPatternText;
	int ColorHiText	= theApp.m_pSettings->Appearance.iColPatternTextHilite;
	
	int ColorHiTextLo	= DIM(ColorHiText, 60);
	int ColorHiTextHi	= DIM(ColorHiText, 80);

	int i;

	// Create a temporary back buffer
	Bmp.CreateCompatibleBitmap(&dc, Width, Height);
	dcBack.CreateCompatibleDC(&dc);
	OldBmp = dcBack.SelectObject(&Bmp);

	memset(&LogFont, 0, sizeof LOGFONT);
	memcpy(LogFont.lfFaceName, FONT_FACE, strlen(FONT_FACE));
	LogFont.lfHeight = -12;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	Font.CreateFontIndirect(&LogFont);

	memset(&LogFont, 0, sizeof LOGFONT);
	memcpy(LogFont.lfFaceName, SMALL_FONT_FACE, strlen(SMALL_FONT_FACE));
	LogFont.lfHeight = -8;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	SmallFont.CreateFontIndirect(&LogFont);

	OldFont = dcBack.SelectObject(&Font);

	// Draw graphics
	dcBack.FillSolidRect(0, 0, Width, Height, ColorBg);

	dcBack.SetBkColor(ColorBg);
	dcBack.SetTextColor(ColorText);

	// Sequence
	Text.Format("Sequence / %s", SEQUENCE_NAMES[m_iSequenceType]);
	dcBack.TextOut(5, 5, Text);

	// Length box
	dcBack.FillSolidRect(10, LENGTH_TOP + 20, 16, EDITOR_HEIGHT - 30, ColorText);
	dcBack.FillSolidRect(11, LENGTH_TOP + 21, 14, 15, ColorBg);
	dcBack.FillSolidRect(11, LENGTH_TOP + 37, 14, EDITOR_HEIGHT - 64, ColorBg);
	dcBack.FillSolidRect(11, LENGTH_TOP + EDITOR_HEIGHT - 26, 14, 15, ColorBg);

	int BarHeight = (76 * m_iSequenceItemCount) / 127;

	dcBack.FillSolidRect(11, LENGTH_TOP + (EDITOR_HEIGHT - BarHeight) - 27, 14, BarHeight, ColorHiText);

	dcBack.SetBkColor(ColorBg);

	Text.Format("Items");
	dcBack.TextOut(5, LENGTH_TOP, Text);
	Text.Format("# %i", m_iSequenceItemCount);
	dcBack.TextOut(30, LENGTH_TOP + 20, Text);
	if (m_iSequenceItemCount == 0)
		Text.Format("0 ms");
	else
		Text.Format("%i ms", m_iSequenceItemCount * (1000 / static_cast<CFamiTrackerDoc*>(theApp.pDocument)->GetFrameRate()));
	dcBack.TextOut(30, LENGTH_TOP + 36, Text);
	Text.Format("+");
	dcBack.TextOut(14, LENGTH_TOP + 21, Text);
	Text.Format("-");
	dcBack.TextOut(14, LENGTH_TOP + EDITOR_HEIGHT - 26, Text);

	/*
	Text.Format("Length");
	dcBack.TextOut(5, LENGTH_TOP, Text);
	Text.Format("%02i [+-]", m_iSequenceItemCount);
	dcBack.TextOut(5, LENGTH_TOP + 16, Text);

	Text.Format("Loop");
	dcBack.TextOut(5, LOOP_TOP, Text);
	Text.Format("%02i [+-]", m_iLoopPoint);
	dcBack.TextOut(5, LOOP_TOP + 16, Text);
	*/

	Text.Format("y = %i", m_iLastValueY);
	dcBack.TextOut(5, COORD_TOP, Text);
	Text.Format("x = %i", m_iLastValueX);
	dcBack.TextOut(5, COORD_TOP + 16, Text);

	// Prepare graphics
	dcBack.SelectObject(&SmallFont);

	// The 'stuff'
	dcBack.FillSolidRect(EDITOR_LEFT, EDITOR_TOP, EDITOR_WIDTH, EDITOR_HEIGHT, ColorBg);
	dcBack.FillSolidRect(EDITOR_LEFT, EDITOR_TOP, 1, EDITOR_HEIGHT, ColorText);

	dcBack.FillSolidRect(EDITOR_LEFT, EDITOR_TOP + EDITOR_HEIGHT, EDITOR_WIDTH, 1, ColorText);

	int Spacing;

	switch (m_iSequenceType) {
		case 0:
			dcBack.FillSolidRect(EDITOR_LEFT, EDITOR_TOP + EDITOR_HEIGHT, EDITOR_WIDTH, 1, ColorText);
			Spacing = 15;
			break;
		case 1:
			dcBack.FillSolidRect(EDITOR_LEFT, EDITOR_TOP + EDITOR_HEIGHT / 2, EDITOR_WIDTH, 1, ColorText);
			Spacing = 31;
			break;
		case 2:
		case 3:
			Spacing = 0;
			break;
		case 4:
			// 4
			dcBack.FillSolidRect(EDITOR_LEFT - 21, EDITOR_TOP - 4, 1, 8, ColorText);
			dcBack.FillSolidRect(EDITOR_LEFT - 6, EDITOR_TOP - 4, 1, 8, ColorText);
			dcBack.FillSolidRect(EDITOR_LEFT - 21, EDITOR_TOP - 4, 12, 1, ColorText);
			dcBack.FillSolidRect(EDITOR_LEFT - 10, EDITOR_TOP - 4, 1, 8, ColorText);
			dcBack.FillSolidRect(EDITOR_LEFT - 10, EDITOR_TOP + 3, 4, 1, ColorText);
			// 3
			dcBack.FillSolidRect(EDITOR_LEFT - 21, EDITOR_TOP + EDITOR_HEIGHT / 3 - 4, 1, 8, ColorText);
			dcBack.FillSolidRect(EDITOR_LEFT - 6, EDITOR_TOP + EDITOR_HEIGHT / 3 - 4, 1, 8, ColorText);
			dcBack.FillSolidRect(EDITOR_LEFT - 21, EDITOR_TOP + EDITOR_HEIGHT / 3 - 4, 8, 1, ColorText);
			dcBack.FillSolidRect(EDITOR_LEFT - 14, EDITOR_TOP + EDITOR_HEIGHT / 3 - 4, 1, 8, ColorText);
			dcBack.FillSolidRect(EDITOR_LEFT - 14, EDITOR_TOP + EDITOR_HEIGHT / 3 + 3, 9, 1, ColorText);
			// 2
			dcBack.FillSolidRect(EDITOR_LEFT - 21, EDITOR_TOP + (EDITOR_HEIGHT * 2) / 3 - 4, 1, 8, ColorText);
			dcBack.FillSolidRect(EDITOR_LEFT - 6, EDITOR_TOP + (EDITOR_HEIGHT * 2) / 3 - 4, 1, 8, ColorText);
			dcBack.FillSolidRect(EDITOR_LEFT - 21, EDITOR_TOP + (EDITOR_HEIGHT * 2) / 3 - 4, 4, 1, ColorText);
			dcBack.FillSolidRect(EDITOR_LEFT - 17, EDITOR_TOP + (EDITOR_HEIGHT * 2) / 3 - 4, 1, 8, ColorText);
			dcBack.FillSolidRect(EDITOR_LEFT - 17, EDITOR_TOP + (EDITOR_HEIGHT * 2) / 3 + 3, 12, 1, ColorText);
			// 1
			dcBack.FillSolidRect(EDITOR_LEFT - 21, EDITOR_TOP + EDITOR_HEIGHT - 4, 1, 8, ColorText);
			dcBack.FillSolidRect(EDITOR_LEFT - 6, EDITOR_TOP + EDITOR_HEIGHT - 4, 1, 8, ColorText);
			dcBack.FillSolidRect(EDITOR_LEFT - 21, EDITOR_TOP + EDITOR_HEIGHT - 4, 2, 1, ColorText);
			dcBack.FillSolidRect(EDITOR_LEFT - 19, EDITOR_TOP + EDITOR_HEIGHT - 4, 1, 8, ColorText);
			dcBack.FillSolidRect(EDITOR_LEFT - 19, EDITOR_TOP + EDITOR_HEIGHT + 3, 14, 1, ColorText);

			dcBack.FillSolidRect(EDITOR_LEFT, EDITOR_TOP + EDITOR_HEIGHT, EDITOR_WIDTH, 1, ColorText);
			Spacing = 3;
			break;
	}
	
	if (m_iSequenceType != 4) {
		dcBack.SetBkColor(ColorBg);
		Text.Format("%2i", m_iMax - m_iSeqOrigin);
		dcBack.TextOut(EDITOR_LEFT - 15, EDITOR_TOP - 5, Text);
		Text.Format("%3i", m_iMin - m_iSeqOrigin);
		dcBack.TextOut(EDITOR_LEFT - 19, EDITOR_TOP + EDITOR_HEIGHT - 5, Text);
	}

	if (Spacing > 0) {
		for (i = 0; i < EDITOR_HEIGHT; i += (EDITOR_HEIGHT / Spacing)) {
			dcBack.FillSolidRect(EDITOR_LEFT, EDITOR_TOP + i, -2, 1, ColorText);
			dcBack.FillSolidRect(EDITOR_LEFT + 1, EDITOR_TOP + i, EDITOR_WIDTH, 1, ColorHiBg);
		}
	}

	int WndWidth = EDITOR_WIDTH;

	if (m_iSequenceType == MOD_ARPEGGIO)
		WndWidth = EDITOR_WIDTH - 10;

	if (m_iSequenceItemCount > 0) {

		int ItemWidth = WndWidth / m_iSequenceItemCount;
		int ItemHeight, ItemCenter;
		int ItemLeft = EDITOR_LEFT + 1;
		bool InRange = true;

		if (ItemWidth < 1)
			ItemWidth = 1;

		for (i = 0; i < m_iSequenceItemCount; i++) {
			if (((i - 1) % 5) == 0) {
				dcBack.SetBkColor(ColorBg);
				dcBack.SetTextColor(ColorText);
				Text.Format("%i", i);
				dcBack.TextOut(ItemLeft, EDITOR_TOP + EDITOR_HEIGHT + 8, Text);
			}

			if (i == m_iLoopPoint) {
				// Draw loop point
				dcBack.SetBkColor(ColorBg);
				Text.Format("^");
				dcBack.TextOut(ItemLeft - 4, EDITOR_TOP + EDITOR_HEIGHT + 1, Text);
				dcBack.FillSolidRect(ItemLeft - 1, EDITOR_TOP, 1, EDITOR_HEIGHT, ColorText);
			}

			switch (m_iSequenceType) {
				case 0:
					ItemHeight = -m_iSequenceItems[i] * EDITOR_HEIGHT / 15;
					ItemCenter = EDITOR_TOP + EDITOR_HEIGHT - 1;
					break;
				case 1:
					ItemCenter = EDITOR_TOP + (EDITOR_HEIGHT / 2) - ((m_iSequenceItems[i] + m_iSeqOrigin) * EDITOR_HEIGHT) / m_iLevels - 3;
					ItemHeight = 6;
					InRange = ItemCenter > EDITOR_TOP && ItemCenter < (EDITOR_TOP + EDITOR_HEIGHT);
					break;
				case 2:
				case 3:
					ItemHeight = -m_iSequenceItems[i] * EDITOR_HEIGHT / 255;
					ItemCenter = EDITOR_TOP + EDITOR_HEIGHT / 2;
					break;
				case 4:
					ItemHeight = -m_iSequenceItems[i] * EDITOR_HEIGHT / 3;
					ItemCenter = EDITOR_TOP + EDITOR_HEIGHT - 1;
					break;
			}

			if (ItemWidth == 1) {
				if (InRange)
					dcBack.FillSolidRect(ItemLeft, ItemCenter, ItemWidth + 1, ItemHeight, ColorHiText);
				ItemLeft += 1;
			}
			else {
				if (InRange) {
					dcBack.FillSolidRect(ItemLeft, ItemCenter, ItemWidth - 1, ItemHeight, ColorHiText);
					if (ItemWidth > 3) {
						dcBack.FillSolidRect(ItemLeft, ItemCenter, 1, ItemHeight, ColorHiTextLo);
						dcBack.FillSolidRect(ItemLeft, ItemCenter, ItemWidth - 1, 1, ColorHiTextLo);
						dcBack.FillSolidRect(ItemLeft + ItemWidth - 2, ItemCenter, 1, ItemHeight, ColorHiTextHi);
						dcBack.FillSolidRect(ItemLeft, ItemCenter + ItemHeight, ItemWidth - 2, 1, ColorHiTextHi);
					}
				}
				ItemLeft += ItemWidth;
			}
		}
	}

	if (m_iSequenceType == MOD_ARPEGGIO /*|| m_iSequenceType == MOD_PITCH || m_iSequenceType == MOD_HIPITCH*/) {
		dcBack.FillSolidRect(EDITOR_LEFT + EDITOR_WIDTH - 10, EDITOR_TOP, 10, EDITOR_HEIGHT, ColorText);
		dcBack.FillSolidRect(EDITOR_LEFT + EDITOR_WIDTH - 9, EDITOR_TOP + 1, 8, EDITOR_HEIGHT - 2, ColorBg);

		// Cursor
		dcBack.FillSolidRect(EDITOR_LEFT + EDITOR_WIDTH - 8, EDITOR_TOP + EDITOR_HEIGHT / 2 + m_iSeqOrigin - 5, 6, 10, ColorText);
	}

	if (m_iStartLineX != 0 && m_iStartLineY != 0) {
		CPen *OldPen, Pen;
		Pen.CreatePen(1, 3, 0xFFFFFF);
		OldPen = dcBack.SelectObject(&Pen);
		//dcBack.SetBkColor(0xFFFFFFFF);
		dcBack.MoveTo(m_iStartLineX, m_iStartLineY);
		dcBack.LineTo(m_iEndLineX, m_iEndLineY);
		dcBack.SelectObject(OldPen);
	}

	// Display window
	dc.BitBlt(0, 0, Width, Height, &dcBack, 0, 0, SRCCOPY);

	// Clean up
	dcBack.SelectObject(OldFont);
	dcBack.SelectObject(OldBmp);
}

void CSequenceEditor::OnLButtonDown(UINT nFlags, CPoint point)
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

void CSequenceEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	static int LastPointY;
	static bool Twice;
	bool FixLoop = false;

	if ((nFlags & MK_LBUTTON) != 0)
		CheckEditing(point.x, point.y);
	else if ((nFlags & MK_RBUTTON) != 0)
		DrawLine(point.x, point.y);

	CWnd::OnMouseMove(nFlags, point);
}

void CSequenceEditor::DrawLine(int PosX, int PosY) 
{
	int StartX, StartY;
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

		for (int x = StartX; x < EndX; x++) {
			ChangeItems(x, int(fY), true);
			fY += DeltaY;
		}

		ChangeItems(x, int(fY), false);
	}
}

void CSequenceEditor::CheckEditing(int PosX, int PosY)
{
	// Length box
	if (PosX > 10 && PosX < 24) {
		bool FixLoop = false;

		if (m_iLoopPoint == m_iSequenceItemCount)
			FixLoop = true;

		if ((PosY > LENGTH_TOP + 36) && (PosY < LENGTH_TOP + EDITOR_HEIGHT - 26)) {
			// Directly
			m_iSequenceItemCount = (127 * (LENGTH_TOP + EDITOR_HEIGHT - PosY - 26)) / 76;			
		}
		else if (PosY > LENGTH_TOP + 20 && PosY < LENGTH_TOP + 34) {
			// Add
			if (m_iSequenceItemCount < 127)
				m_iSequenceItemCount++;
		}
		else if (PosY > LENGTH_TOP + EDITOR_HEIGHT - 26 && PosY < LENGTH_TOP + EDITOR_HEIGHT - 12) {
			// Remove
			if (m_iSequenceItemCount > 0)
				m_iSequenceItemCount--;
		}

		if (FixLoop)
			m_iLoopPoint = m_iSequenceItemCount;

		RedrawWindow();
		m_pParent->CompileSequence();
	}

	if (!((PosX > EDITOR_LEFT) && (PosX < EDITOR_LEFT + EDITOR_WIDTH)))
		return;

	if (m_iEditing == EDIT_NONE) {
		// Decide what to do
		if ((PosY > EDITOR_TOP + EDITOR_HEIGHT) && (PosY < EDITOR_TOP + EDITOR_HEIGHT + 20)) {
			m_iEditing = EDIT_LOOP;
		}
		else if ((m_iSequenceType == MOD_ARPEGGIO) && (PosX - EDITOR_LEFT) > (EDITOR_WIDTH - 10)) {
			m_iEditing = EDIT_SCROLL;
		}
		else {
			m_iEditing = EDIT_SEQUENCE;
		}
	}

	switch (m_iEditing) {
		case EDIT_SEQUENCE:
			if ((PosY > EDITOR_TOP) && (PosY < EDITOR_TOP + EDITOR_HEIGHT) && PosX > 0 && PosX < EDITOR_LEFT + EDITOR_WIDTH)
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

void CSequenceEditor::ChangeLoopPoint(int PosX, int PosY)
{
	if (m_iSequenceItemCount == 0)
		return;

	int Width = EDITOR_WIDTH / m_iSequenceItemCount;
	
	if (Width < 1)
		Width = 1;

	int Item = ((PosX + Width / 2) - EDITOR_LEFT) / Width;

	m_iLoopPoint = Item;

	m_pParent->CompileSequence();
	RedrawWindow();
}

void CSequenceEditor::ChangeItems(int PosX, int PosY, bool Quick)
{
	if (m_iSequenceItemCount == 0)
		return;

	int WndWidth = EDITOR_WIDTH;

	if (m_iSequenceType == MOD_ARPEGGIO)
		WndWidth = EDITOR_WIDTH - 10;

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
			PosY += EDITOR_HEIGHT / 6;
			Value = 3 - ((PosY - EDITOR_TOP) * 3) / EDITOR_HEIGHT;
			LIMIT(Value, 4, 0);
			break;
	}

	m_iSequenceItems[Item] = Value;
	m_iLastValueY = Value;
	m_iLastValueX = Item;

	if (!Quick) {
		m_pParent->CompileSequence();
		RedrawWindow();
	}
}

BOOL CSequenceEditor::PreCreateWindow(CREATESTRUCT& cs)
{
	m_bSequenceSizing	= false;
	m_bLoopMoving		= false;
	m_iEditing			= EDIT_NONE;

	return CWnd::PreCreateWindow(cs);
}

void CSequenceEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bSequenceSizing	= false;
	m_bLoopMoving		= false;
	m_bScrollLock		= false;
	m_iEditing			= EDIT_NONE;

	CWnd::OnLButtonUp(nFlags, point);
}

void CSequenceEditor::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	OnLButtonDown(nFlags, point);
	CWnd::OnLButtonDblClk(nFlags, point);
}

BOOL CSequenceEditor::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message) {
		case WM_KEYDOWN:
			static_cast<CFamiTrackerView*>(theApp.pView)->PlayNote(int(pMsg->wParam));
			return TRUE;
		case WM_KEYUP:
			static_cast<CFamiTrackerView*>(theApp.pView)->KeyReleased(int(pMsg->wParam));
			return TRUE;
	}

	return CWnd::PreTranslateMessage(pMsg);
}

void CSequenceEditor::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (IsInsideSequenceWin(point.x, point.y)) {
		m_iStartLineX = m_iEndLineX = point.x;
		m_iStartLineY = m_iEndLineY = point.y;
		m_iEditing = EDIT_LINE;
	}

	CWnd::OnRButtonDown(nFlags, point);
}


void CSequenceEditor::OnRButtonUp(UINT nFlags, CPoint point)
{
	m_iStartLineX = m_iStartLineY = 0;
	m_iEditing = EDIT_NONE;
	RedrawWindow();
	CWnd::OnRButtonUp(nFlags, point);
}
