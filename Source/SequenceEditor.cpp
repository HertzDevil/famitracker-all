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
#include <cmath>
#include "FamiTracker.h"
#include "SequenceEditor.h"
#include "InstrumentSettings.h"

const char *SEQUENCE_NAMES[] = {"Volume",
								"Arpeggio",
								"Pitch bend",
								"Hi-speed pitch bend",
								"Duty cycle & noise mode"};

const int EDITOR_LEFT	= 60;
const int EDITOR_TOP	= 30;
const int EDITOR_WIDTH	= 280;
const int EDITOR_HEIGHT	= 150;

const int LENGTH_HEIGHT = 150;
const int LENGTH_TOP	= 30;
const int LENGTH_LEFT	= 10;
const int LENGTH_WIDTH	= 18;
const int LOOP_TOP		= 72;
const int COORD_TOP		= 165;

enum EDIT {
	EDIT_NONE,
	EDIT_SEQUENCE,
	EDIT_LOOP,
	EDIT_SCROLL,
	EDIT_LINE
};

const int PRESET_FIRST_GROUP_LEFT		= 20;
const int PRESET_FIRST_GROUP_TOP		= 40;
const int PRESET_FIRST_GROUP_SPACING	= 25;
const int PRESET_SPEED_GROUP_LEFT		= 180;
const int PRESET_DIRECTION_GROUP_LEFT	= 270;

const char *PRESET_VOL_TYPES[] = {"Linear fade out", "Linear fade in", "Exponential fade out", "Exponential fade in"};
const char *PRESET_ARP_TYPES[] = {"Chord, major", "Chord, minor", "Chord, sus 2", "Chord, sus 4"};
const char *PRESET_PITCH_TYPES[] = {"Slide down", "Slide up", "Vibrato", "Vibrato 2"};
const char *PRESET_HIPITCH_TYPES[] = {"Drum (triangle)"};
const char *PRESET_DUTY_TYPES[] = {"Duty cycle cycling 1", "Duty cycle cycling 2"};
const char *PRESET_SPEED_TYPES[] = {"Fast", "Medium", "Slow"};
const char *PRESET_DIRECTION_TYPES[] = {"Up", "Down"};

const unsigned int COLOR_BARS = 0xC0A020;

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

const int NUM_MIN[] = {0, -10, -127, -127, 0};
const int NUM_MAX[] = {15, 10, 126, 126, 3};

void CSequenceEditor::SetCurrentSequence(int Type, stSequence *List)
{
	unsigned int i;
	int j, x = 0, val;
	bool LoopEnabled = false;

	m_bShowPresets = false;
	m_iLoopPoint = -1;

	memset(m_iSequenceItems, 0, sizeof(int) * MAX_ITEMS);
	m_List = List;

	m_iSequenceType = Type;

	for (i = 0; i < List->Count && x < MAX_ITEMS; i++) {
		if (List->Length[i] < 0) {
			LoopEnabled = true;
			m_iLoopPoint = x;
			for (j = signed(List->Count + List->Length[i] - 1); j < signed(List->Count - 1); j++)
				m_iLoopPoint -= (List->Length[j] + 1);
		}
		else {
			for (j = 0; j < (List->Length[i] + 1) && x < MAX_ITEMS; j++) {
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

	int i;

	m_pBackDC->SelectObject(&m_Font);

	ClearBackground();

	// Title
	m_pBackDC->SetBkMode(TRANSPARENT);
	m_pBackDC->SetTextColor(0xFFFFFF);

	Text.Format("Sequence / %s", SEQUENCE_NAMES[m_iSequenceType]);
	m_pBackDC->TextOut(2, 2, Text);

	DrawButton(m_iWidth - 56, 2, 50, "Presets", BUTTON_PRESET);

	m_pBackDC->FillSolidRect(0, 21, m_iWidth, 1, 0xFFFFFF);

	if (m_bShowPresets) {

		m_pBackDC->FillSolidRect(10, m_iHeight - 35, m_iWidth - 20, 1, 0xFFFFFF);
		DrawButton(20, m_iHeight - 25, 40, "Done", BUTTON_DONE);
		DrawButton(70, m_iHeight - 25, 50, "Cancel", BUTTON_CANCEL);

		switch (m_iSequenceType) {
			case 0:
			case 1:
			case 4:
				for (i = 0; i < 3; i++)
					DrawSwitch(PRESET_SPEED_GROUP_LEFT, PRESET_FIRST_GROUP_TOP + i * PRESET_FIRST_GROUP_SPACING, 60, PRESET_SPEED_TYPES[i], m_iSwitchGroup2 == i);
				break;
		}

		switch (m_iSequenceType) {
			case 0:
				for (i = 0; i < 4; i++)
					DrawSwitch(PRESET_FIRST_GROUP_LEFT, PRESET_FIRST_GROUP_TOP + i * PRESET_FIRST_GROUP_SPACING, 130, PRESET_VOL_TYPES[i], m_iSwitchGroup1 == i);
				break;
			case 1:
				for (i = 0; i < 4; i++)
					DrawSwitch(PRESET_FIRST_GROUP_LEFT, PRESET_FIRST_GROUP_TOP + i * PRESET_FIRST_GROUP_SPACING, 130, PRESET_ARP_TYPES[i], m_iSwitchGroup1 == i);
				for (i = 0; i < 2; i++)
					DrawSwitch(PRESET_DIRECTION_GROUP_LEFT, PRESET_FIRST_GROUP_TOP + i * PRESET_FIRST_GROUP_SPACING, 60, PRESET_DIRECTION_TYPES[i], m_iSwitchGroup3 == i);
				break;
			case 2:
				for (i = 0; i < 4; i++)
					DrawSwitch(PRESET_FIRST_GROUP_LEFT, PRESET_FIRST_GROUP_TOP + i * PRESET_FIRST_GROUP_SPACING, 130, PRESET_PITCH_TYPES[i], m_iSwitchGroup1 == i);
				break;
			case 3:
				for (i = 0; i < 1; i++)
					DrawSwitch(PRESET_FIRST_GROUP_LEFT, PRESET_FIRST_GROUP_TOP + i * PRESET_FIRST_GROUP_SPACING, 130, PRESET_HIPITCH_TYPES[i], m_iSwitchGroup1 == i);
				break;
			case 4:
				for (i = 0; i < 2; i++)
					DrawSwitch(PRESET_FIRST_GROUP_LEFT, PRESET_FIRST_GROUP_TOP + i * PRESET_FIRST_GROUP_SPACING, 130, PRESET_DUTY_TYPES[i], m_iSwitchGroup1 == i);
				break;
		}

	}
	else {

		DrawScrollBar(LENGTH_LEFT, LENGTH_TOP, LENGTH_WIDTH, LENGTH_HEIGHT);

		switch (m_iSequenceType) {
			case 0: DrawVolumeEditor();		break;
			case 1: DrawArpeggioEditor();	break;
			case 2: DrawPitchEditor();		break;
			case 3: DrawPitchEditor();		break;
			case 4: DrawDutyEditor();		break;

		}

		// Prepare graphics
		m_pBackDC->SelectObject(&m_SmallFont);

		int Spacing = 15;

		if (m_iStartLineX != 0 && m_iStartLineY != 0) {
			CPen *OldPen, Pen;
			Pen.CreatePen(1, 3, 0xFFFFFF);
			OldPen = m_pBackDC->SelectObject(&Pen);
			//dcBack.SetBkColor(0xFFFFFFFF);
			m_pBackDC->MoveTo(m_iStartLineX, m_iStartLineY);
			m_pBackDC->LineTo(m_iEndLineX, m_iEndLineY);
			m_pBackDC->SelectObject(OldPen);
		}
	}

	// Display window
	DisplayArea(&dc);
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
	int i;

	// Presets
	HandleButton(m_iWidth - 56, 2, 50, BUTTON_PRESET, PosX, PosY);

	if (m_bShowPresets) {

		HandleButton(20, m_iHeight - 25, 40, BUTTON_DONE, PosX, PosY);
		HandleButton(70, m_iHeight - 25, 50, BUTTON_CANCEL, PosX, PosY);

		switch (m_iSequenceType) {
			case 0:
				for (i = 0; i < 4; i++)
					HandleSwitchButton(PRESET_FIRST_GROUP_LEFT, PRESET_FIRST_GROUP_TOP + i * PRESET_FIRST_GROUP_SPACING, 130, PosX, PosY, m_iSwitchGroup1, i);
				for (i = 0; i < 3; i++)
					HandleSwitchButton(PRESET_SPEED_GROUP_LEFT, PRESET_FIRST_GROUP_TOP + i * PRESET_FIRST_GROUP_SPACING, 60, PosX, PosY, m_iSwitchGroup2, i);
				break;
			case 1:
				for (i = 0; i < 4; i++)
					HandleSwitchButton(PRESET_FIRST_GROUP_LEFT, PRESET_FIRST_GROUP_TOP + i * PRESET_FIRST_GROUP_SPACING, 130, PosX, PosY, m_iSwitchGroup1, i);
				for (i = 0; i < 3; i++)
					HandleSwitchButton(PRESET_SPEED_GROUP_LEFT, PRESET_FIRST_GROUP_TOP + i * PRESET_FIRST_GROUP_SPACING, 60, PosX, PosY, m_iSwitchGroup2, i);
				for (i = 0; i < 2; i++)
					HandleSwitchButton(PRESET_DIRECTION_GROUP_LEFT, PRESET_FIRST_GROUP_TOP + i * PRESET_FIRST_GROUP_SPACING, 60, PosX, PosY, m_iSwitchGroup3, i);
				break;
			case 2:
				for (i = 0; i < 4; i++)
					HandleSwitchButton(PRESET_FIRST_GROUP_LEFT, PRESET_FIRST_GROUP_TOP + i * PRESET_FIRST_GROUP_SPACING, 130, PosX, PosY, m_iSwitchGroup1, i);
				break;
			case 3:
				for (i = 0; i < 1; i++)
					HandleSwitchButton(PRESET_FIRST_GROUP_LEFT, PRESET_FIRST_GROUP_TOP + i * PRESET_FIRST_GROUP_SPACING, 130, PosX, PosY, m_iSwitchGroup1, i);
				break;
			case 4:
				for (i = 0; i < 2; i++)
					HandleSwitchButton(PRESET_FIRST_GROUP_LEFT, PRESET_FIRST_GROUP_TOP + i * PRESET_FIRST_GROUP_SPACING, 130, PosX, PosY, m_iSwitchGroup1, i);
				for (i = 0; i < 3; i++)
					HandleSwitchButton(PRESET_SPEED_GROUP_LEFT, PRESET_FIRST_GROUP_TOP + i * PRESET_FIRST_GROUP_SPACING, 60, PosX, PosY, m_iSwitchGroup2, i);
				break;
		}
	}
	else {
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

	switch (m_iButtonPressed) {
		case BUTTON_PRESET:
			m_bShowPresets = true;
			break;
		case BUTTON_DONE:
			LoadPreset();
		case BUTTON_CANCEL:
			m_bShowPresets = false;
			break;
	}

	m_iButtonPressed	= 0;

	RedrawWindow();

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

// 

void CSequenceEditor::HandleScrollBar(int x, int y, int Width, int Height, int PosX, int PosY)
{
	if (PosX < x || PosX > (x + Width) || PosY < y || PosY > (y + Height))
		return;

	bool FixLoop = false;

	if (m_iLoopPoint == m_iSequenceItemCount)
		FixLoop = true;

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

	/*
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

*/

	if (FixLoop)
		m_iLoopPoint = m_iSequenceItemCount;

	RedrawWindow();
	m_pParent->CompileSequence();
}

bool CSequenceEditor::HandleButton(int x, int y, int Width, int Name, int PosX, int PosY)
{
	if (PosX >= x && PosX <= (x + Width) && PosY >= y && PosY <= (y + 16)) {
		m_iButtonPressed = Name;
		RedrawWindow();
		return true;
	}

	return false;
}

void CSequenceEditor::HandleSwitchButton(int x, int y, int Width, int PosX, int PosY, int &Switch, int Nr)
{
	if (PosX >= x && PosX <= (x + Width) && PosY >= y && PosY <= (y + 16)) {
		Switch = Nr;
	}
}

// GUI

void CSequenceEditor::DisplayArea(CDC *pDC)
{
	pDC->BitBlt(0, 0, m_iWidth, m_iHeight, m_pBackDC, 0, 0, SRCCOPY);
}

void CSequenceEditor::ClearBackground()
{
	m_pBackDC->FillSolidRect(0, 0, m_iWidth, m_iHeight, 0x102010);

	for (int i = 0; i < 21; i++) {
		m_pBackDC->FillSolidRect(0, i, m_iWidth, 1, DIM_TO(0x804040, 0x102010, 100 - (i * 100) / 21));
	}

	for (i = 0; i < (m_iHeight - 21); i++) {
		m_pBackDC->FillSolidRect(0, i + 21, m_iWidth, 1, DIM_TO(0x604020, 0x102010, int(sinf((float(i) * 3.14f) / (m_iHeight - 21)) * 50.0f + 50.0f)));
	}

}

void CSequenceEditor::DrawLine(int x1, int y1, int x2, int y2)
{
	m_pBackDC->MoveTo(x1, y1);
	m_pBackDC->LineTo(x2, y2);
}

void CSequenceEditor::DrawScrollBar(int x, int y, int Width, int Height)
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
		Text.Format("Length: # %i, %i ms {%i: %i}", m_iSequenceItemCount, m_iSequenceItemCount * (1000 / static_cast<CFamiTrackerDoc*>(theApp.pDocument)->GetFrameRate()), m_iLastValueX, m_iLastValueY);
	
	m_pBackDC->TextOut(EDITOR_LEFT, EDITOR_TOP + EDITOR_HEIGHT + 6, Text);
}

void CSequenceEditor::DrawButton(int x, int y, int Width, CString Text, int Name)
{
	m_pBackDC->SelectObject(m_ButtonFont);
	m_pBackDC->SetTextColor(0xFFFFFF);
	m_pBackDC->SetBkMode(TRANSPARENT);

	if (m_iButtonPressed == Name) {
		m_pBackDC->FillSolidRect(x, y, Width, 16, 0x404040);
		m_pBackDC->Draw3dRect(x, y, Width, 16, 0x808080, 0xFFFFFF);
	}
	else
		m_pBackDC->Draw3dRect(x, y, Width, 16, 0xFFFFFF, 0x808080);

	m_pBackDC->TextOut(x + 4, y + 2, Text);
}

void CSequenceEditor::DrawSwitch(int x, int y, int Width, CString Text, bool Pressed)
{
	m_pBackDC->SelectObject(m_ButtonFont);
	m_pBackDC->SetTextColor(0xFFFFFF);
	m_pBackDC->SetBkMode(TRANSPARENT);

	if (Pressed) {
		m_pBackDC->FillSolidRect(x, y, Width, 16, 0x404040);
		m_pBackDC->Draw3dRect(x, y, Width, 16, 0x808080, 0xFFFFFF);
	}
	else
		m_pBackDC->Draw3dRect(x, y, Width, 16, 0xFFFFFF, 0x808080);

	m_pBackDC->TextOut(x + 4, y + 2, Text);
}

void CSequenceEditor::CreateBackground(int Space)
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

void CSequenceEditor::DrawVolumeEditor()
{
	const int COL_BG = 0x808080;
	const int COL_TEXT = 0xFFFFFF;

	int i;

	CreateBackground(15);

	m_pBackDC->SetBkMode(TRANSPARENT);
	m_pBackDC->SetTextColor(COL_TEXT);

	CString Text;

	Text.Format("15");
	m_pBackDC->TextOut(EDITOR_LEFT - 15, EDITOR_TOP - 4, Text);
	Text.Format("0");
	m_pBackDC->TextOut(EDITOR_LEFT - 11, EDITOR_TOP + EDITOR_HEIGHT - 6, Text);

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
				Text.Format("^");
				m_pBackDC->TextOut(ItemLeft - 4, EDITOR_TOP + EDITOR_HEIGHT - 1, Text);
				m_pBackDC->FillSolidRect(ItemLeft - 1, EDITOR_TOP, 1, EDITOR_HEIGHT, COL_TEXT);
			}

			ItemHeight = -m_iSequenceItems[i] * EDITOR_HEIGHT / 15 + 3;
			ItemCenter = EDITOR_TOP + EDITOR_HEIGHT - 2;

			if (InRange) {
				if (ItemHeight < -1) {
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

void CSequenceEditor::DrawArpeggioEditor()
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

			//ItemCenter = EDITOR_TOP + (EDITOR_HEIGHT / 2) - ((m_iSequenceItems[i] + m_iSeqOrigin) * EDITOR_HEIGHT) / /*m_iLevels*/ 20 - 4;
			ItemHeight = (EDITOR_HEIGHT - 2) / 20;
			ItemCenter = EDITOR_TOP + EDITOR_HEIGHT / 2 - 4 - (m_iSequenceItems[i] + m_iSeqOrigin) * ItemHeight;
			InRange = ItemCenter > EDITOR_TOP && ItemCenter < (EDITOR_TOP + EDITOR_HEIGHT);

			if (InRange) {
				m_pBackDC->FillSolidRect(ItemLeft, ItemCenter-1, ItemWidth, ItemHeight + 2, DIM(COLOR_BARS, 80));
				m_pBackDC->Draw3dRect(ItemLeft, ItemCenter-1, ItemWidth, ItemHeight + 2, COLOR_BARS, DIM(COLOR_BARS, 50));
			}

			ItemLeft += ItemWidth;
		}
	}
}

void CSequenceEditor::DrawPitchEditor()
{
	const int COL_BG = 0x808080;
	const int COL_TEXT = 0xFFFFFF;

	int i;

	CreateBackground(2);

	m_pBackDC->SetBkMode(TRANSPARENT);
	m_pBackDC->SetTextColor(COL_TEXT);

	CString Text;

	Text.Format("15");
	m_pBackDC->TextOut(EDITOR_LEFT - 15, EDITOR_TOP - 4, Text);
	Text.Format("0");
	m_pBackDC->TextOut(EDITOR_LEFT - 11, EDITOR_TOP + EDITOR_HEIGHT - 6, Text);

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
				Text.Format("^");
				m_pBackDC->TextOut(ItemLeft - 4, EDITOR_TOP + EDITOR_HEIGHT - 1, Text);
				m_pBackDC->FillSolidRect(ItemLeft - 1, EDITOR_TOP, 1, EDITOR_HEIGHT, COL_TEXT);
			}

			ItemHeight = -m_iSequenceItems[i] * EDITOR_HEIGHT / 255;
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

void CSequenceEditor::DrawDutyEditor()
{
	const int COL_BG = 0x808080;
	const int COL_TEXT = 0xFFFFFF;

	int i;

	CreateBackground(3);

	m_pBackDC->SetBkMode(TRANSPARENT);
	m_pBackDC->SetTextColor(COL_TEXT);

	CString Text;

	Text.Format("75%%");
	m_pBackDC->TextOut(EDITOR_LEFT - 27, EDITOR_TOP - 4, Text);
	Text.Format("50%%");
	m_pBackDC->TextOut(EDITOR_LEFT - 27, EDITOR_TOP + 44, Text);
	Text.Format("25%%");
	m_pBackDC->TextOut(EDITOR_LEFT - 27, EDITOR_TOP + 94, Text);
	Text.Format("12%%");
	m_pBackDC->TextOut(EDITOR_LEFT - 27, EDITOR_TOP + 144, Text);

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
				Text.Format("^");
				m_pBackDC->TextOut(ItemLeft - 4, EDITOR_TOP + EDITOR_HEIGHT - 1, Text);
				m_pBackDC->FillSolidRect(ItemLeft - 1, EDITOR_TOP, 1, EDITOR_HEIGHT, COL_TEXT);
			}

			ItemHeight = -m_iSequenceItems[i] * EDITOR_HEIGHT / 3 + 3;
			ItemCenter = EDITOR_TOP + EDITOR_HEIGHT - 2;

			if (InRange) {
				if (ItemHeight < -1) {
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

////

BOOL CSequenceEditor::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, LPVOID lpParam)
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

	m_bShowPresets = false;
	m_iSwitchGroup1 = 0;
	m_iSwitchGroup2 = 0;
	m_iSwitchGroup3 = 0;

	return bResult;
}

BOOL CSequenceEditor::DestroyWindow()
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

void CSequenceEditor::LoadPreset()
{
	int i;

	const int ARP_MAJOR[] = {0, 4, 7};
	const int ARP_MINOR[] = {0, 3, 7};
	const int ARP_SUS2[] = {0, 2, 7};
	const int ARP_SUS4[] = {0, 5, 7};
	const int PITCH_VIB[] = {0, 0, 2, 2, 0, 0, -2, -2};
	const int PITCH_VIB2[] = {0, 0, 1, 2, 2, 1, 0, 0, -1, -2, -2, -1};
	const int DUTY_1[] = {2, 1, 0, 1};
	const int DUTY_2[] = {0, 1, 2, 3};

	switch (m_iSequenceType) {
		// Volume
		case 0:
			switch (m_iSwitchGroup2) {
				case 0: m_iSequenceItemCount = 10; break;
				case 1: m_iSequenceItemCount = 25; break;
				case 2: m_iSequenceItemCount = 50; break;
			}
			m_iLoopPoint = -1;
			switch (m_iSwitchGroup1) {
				case 0:
					for (i = 0; i < m_iSequenceItemCount; i++) {
						m_iSequenceItems[i] = 15 - (i * 15) / (m_iSequenceItemCount - 1);
					}
					break;
				case 1:
					for (i = 0; i < m_iSequenceItemCount; i++) {
						m_iSequenceItems[i] = (i * 15) / (m_iSequenceItemCount - 1);
					}
					break;
				case 2:
					for (i = 0; i < m_iSequenceItemCount; i++) {
						m_iSequenceItems[i] = (int)exp((float(m_iSequenceItemCount - i) * 2.71f) / float(m_iSequenceItemCount));
					}
					m_iSequenceItems[m_iSequenceItemCount - 1] = 0;
					break;
				case 3:
					for (i = 0; i < m_iSequenceItemCount; i++) {
						m_iSequenceItems[i] = int(exp((float(i) * 2.71f) / float(m_iSequenceItemCount)));
					}
					break;
			}
			break;
		// Arpeggio
		case 1:
			int Invert, Index;

			switch (m_iSwitchGroup2) {
				case 0: m_iSequenceItemCount = 3; break;
				case 1: m_iSequenceItemCount = 6; break;
				case 2: m_iSequenceItemCount = 12; break;
			}

			m_iLoopPoint = 0;
			
			if (m_iSwitchGroup3 == 1)
				Invert = 2;
			else
				Invert = 0;

			for (i = 0; i < m_iSequenceItemCount; i++) {
				Index = abs(Invert - ((i * 3) / m_iSequenceItemCount));
				switch (m_iSwitchGroup1) {
					case 0: m_iSequenceItems[i] = ARP_MAJOR[Index]; break;
					case 1: m_iSequenceItems[i] = ARP_MINOR[Index]; break;
					case 2: m_iSequenceItems[i] = ARP_SUS2[Index]; break;
					case 3: m_iSequenceItems[i] = ARP_SUS4[Index]; break;
				}
			}
			break;
		case 2:
			switch (m_iSwitchGroup1) {
				case 0:
					m_iSequenceItemCount = 8;
					m_iLoopPoint = 7;
					for (i = 0; i < m_iSequenceItemCount; i++)
						m_iSequenceItems[i] = i * 3;
					break;
				case 1:
					m_iSequenceItemCount = 8;
					m_iLoopPoint = 7;
					for (i = 0; i < m_iSequenceItemCount; i++)
						m_iSequenceItems[i] = -(m_iSequenceItemCount - i + 3) * 1;
					break;
				case 2:
					m_iSequenceItemCount = 8;
					m_iLoopPoint = 0;
					for (i = 0; i < m_iSequenceItemCount; i++)
						m_iSequenceItems[i] = PITCH_VIB[i];
					break;
				case 3:
					m_iSequenceItemCount = 12;
					m_iLoopPoint = 0;
					for (i = 0; i < m_iSequenceItemCount; i++)
						m_iSequenceItems[i] = PITCH_VIB2[i];
					break;
					
			}
			break;
		case 3:
			m_iSequenceItemCount = 3;
			m_iLoopPoint = 2;
			m_iSequenceItems[0] = 0;
			m_iSequenceItems[1] = 8;
			m_iSequenceItems[2] = 11;
			break;
		case 4:
			switch (m_iSwitchGroup2) {
				case 0: m_iSequenceItemCount = 12; break;
				case 1: m_iSequenceItemCount = 20; break;
				case 2: m_iSequenceItemCount = 28; break;
			}
			switch (m_iSwitchGroup1) {
				case 0:
					m_iLoopPoint = 0;
					for (i = 0; i < m_iSequenceItemCount; i++)
						m_iSequenceItems[i] = DUTY_1[(i * 4) / m_iSequenceItemCount];
					break;
				case 1:
					m_iLoopPoint = 0;
					for (i = 0; i < m_iSequenceItemCount; i++)
						m_iSequenceItems[i] = DUTY_2[(i * 4) / m_iSequenceItemCount];
					break;
			}

			break;
	}

	m_pParent->CompileSequence();
}
