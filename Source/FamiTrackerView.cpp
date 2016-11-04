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
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "MIDI.h"
#include "InstrumentEditDlg.h"
#include "SpeedDlg.h"
#include "SoundGen.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define GET_CHANNEL(x)	((x - LEFT_OFFSET - 40) / CHANNEL_WIDTH)
#define GET_COLUMN(x)	((x - LEFT_OFFSET - 40) - (GET_CHANNEL(x) * CHANNEL_WIDTH))

const char CLIPBOARD_ID[] = "FamiTracker";

enum { COL_NOTE, COL_INSTRUMENT, COL_VOLUME, COL_EFFECT };

const unsigned int PAGE_SIZE = 4;

const unsigned int TEXT_POSITION		= 10;
const unsigned int TOP_OFFSET			= 46;
const unsigned int LEFT_OFFSET			= 10;
const unsigned int ROW_HEIGHT			= 18;

const unsigned int COLUMN_SPACING		= 4;
const unsigned int CHAR_WIDTH			= 12;

const unsigned int ROW_NUMBER_WIDTH		= 20;
const unsigned int CHANNEL_SPACING		= 10;

const unsigned int CHANNEL_START		= LEFT_OFFSET * 2 + ROW_NUMBER_WIDTH - 3;

const unsigned int CHANNEL_WIDTH		= CHAR_WIDTH * 9 + COLUMN_SPACING * 3;
const unsigned int CHAN_AREA_WIDTH		= CHANNEL_WIDTH + CHANNEL_SPACING * 2;

const unsigned int COLUMN_SPACE[]		= {CHAR_WIDTH * 3 + COLUMN_SPACING,
										   CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING, 
										   CHAR_WIDTH + COLUMN_SPACING,  
										   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING,
										   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING,
										   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING,
										   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING};

const unsigned int CURSOR_SPACE[]		= {CHAR_WIDTH * 3,
										   CHAR_WIDTH, CHAR_WIDTH,
										   CHAR_WIDTH,
										   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH,
										   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH,
										   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH,
										   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH};

const unsigned int COLUMN_SEL[]			= {0, CHAR_WIDTH * 3 + COLUMN_SPACING,
											CHAR_WIDTH * 4 + COLUMN_SPACING + 2,
											CHAR_WIDTH * 5 + COLUMN_SPACING * 2, 
											CHAR_WIDTH * 6 + COLUMN_SPACING * 3,
											CHAR_WIDTH * 7 + COLUMN_SPACING * 4 - 2,
											CHAR_WIDTH * 8 + COLUMN_SPACING * 4 - 2, 
											CHAR_WIDTH * 9 + COLUMN_SPACING * 4,
											CHAR_WIDTH * 10 + COLUMN_SPACING * 5 - 2,
											CHAR_WIDTH * 11 + COLUMN_SPACING * 5 - 2, 
											CHAR_WIDTH * 12 + COLUMN_SPACING * 5,
											CHAR_WIDTH * 13 + COLUMN_SPACING * 6 - 2,
											CHAR_WIDTH * 14 + COLUMN_SPACING * 6 - 2, 
											CHAR_WIDTH * 15 + COLUMN_SPACING * 6,
											CHAR_WIDTH * 16 + COLUMN_SPACING * 7 - 2,
											CHAR_WIDTH * 17 + COLUMN_SPACING * 7 - 2, 
											CHAR_WIDTH * 18 + COLUMN_SPACING * 7};

const unsigned int GET_APPR_START_SEL[]	= {0, 1, 1, 3, 4, 4, 4, 7, 7, 7, 10, 10, 10, 13, 13, 13};
const unsigned int GET_APPR_END_SEL[]	= {0, 2, 2, 3, 6, 6, 6, 9, 9, 9, 12, 12, 12, 15, 15, 15};

int FadeCol[5];

// maybe...
int ArpeggioNotes[128];
int ArpeggioPtr = 0, LastArpPtr = 0;
bool DoArpeggiate = false;
int KeyCount = 0;

// CFamiTrackerView

IMPLEMENT_DYNCREATE(CFamiTrackerView, CView)

BEGIN_MESSAGE_MAP(CFamiTrackerView, CView)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_TIMER()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
	ON_COMMAND(ID_EDIT_SELECTALL, OnEditSelectall)
	ON_COMMAND(ID_EDIT_PASTEOVERWRITE, OnEditPasteoverwrite)
	ON_COMMAND(ID_EDIT_ENABLEMIDI, OnEditEnablemidi)
	ON_COMMAND(ID_FRAME_INSERT, OnFrameInsert)
	ON_COMMAND(ID_FRAME_REMOVE, OnFrameRemove)
	ON_COMMAND(ID_TRANSPOSE_DECREASENOTE, OnTransposeDecreasenote)
	ON_COMMAND(ID_TRANSPOSE_DECREASEOCTAVE, OnTransposeDecreaseoctave)
	ON_COMMAND(ID_TRANSPOSE_INCREASENOTE, OnTransposeIncreasenote)
	ON_COMMAND(ID_TRANSPOSE_INCREASEOCTAVE, OnTransposeIncreaseoctave)
	ON_COMMAND(ID_TRACKER_PLAYROW, OnTrackerPlayrow)
	ON_COMMAND(ID_TRACKER_EDIT, OnTrackerEdit)
	ON_COMMAND(ID_TRACKER_PAL, OnTrackerPal)
	ON_COMMAND(ID_TRACKER_NTSC, OnTrackerNtsc)
	ON_COMMAND(ID_SPEED_CUSTOM, OnSpeedCustom)
	ON_COMMAND(ID_SPEED_DEFAULT, OnSpeedDefault)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditDelete)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_EDIT, OnUpdateTrackerEdit)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_PAL, OnUpdateTrackerPal)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_NTSC, OnUpdateTrackerNtsc)
	ON_UPDATE_COMMAND_UI(ID_SPEED_DEFAULT, OnUpdateSpeedDefault)
	ON_UPDATE_COMMAND_UI(ID_SPEED_CUSTOM, OnUpdateSpeedCustom)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTEOVERWRITE, OnUpdateEditPasteoverwrite)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_ENABLEMIDI, OnUpdateEditEnablemidi)
	ON_UPDATE_COMMAND_UI(ID_FRAME_REMOVE, OnUpdateFrameRemove)
	ON_COMMAND(ID_EDIT_PASTEMIX, OnEditPastemix)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTEMIX, OnUpdateEditPastemix)
	ON_COMMAND(ID_MODULE_MOVEFRAMEDOWN, OnModuleMoveframedown)
	ON_COMMAND(ID_MODULE_MOVEFRAMEUP, OnModuleMoveframeup)
	ON_UPDATE_COMMAND_UI(ID_MODULE_MOVEFRAMEDOWN, OnUpdateModuleMoveframedown)
	ON_UPDATE_COMMAND_UI(ID_MODULE_MOVEFRAMEUP, OnUpdateModuleMoveframeup)
END_MESSAGE_MAP()

struct stClipData {
	unsigned int		Size;
	bool				FirstColumn;
	unsigned int		NumCols;
	char				Notes[MAX_PATTERN_LENGTH];
	char				Octaves[MAX_PATTERN_LENGTH];
	int					ColumnData[COLUMNS + 3 * 4][MAX_PATTERN_LENGTH];
};

volatile static int iStillAlive = 20;

bool IsColumnSelected(int Start, int End, int Column)
{
	int SelColStart = Start;
	int SelColEnd	= End;

	if (SelColStart > SelColEnd) {
		SelColStart = End; 
		SelColEnd	= Start;
	}

	if (SelColStart == 2)
		SelColStart = 1;
	if (SelColStart == 5 || SelColStart == 6)
		SelColStart = 4;
	if (SelColStart == 8 || SelColStart == 9)
		SelColStart = 7;
	if (SelColStart == 11 || SelColStart == 12)
		SelColStart = 10;
	if (SelColStart == 14 || SelColStart == 15)
		SelColStart = 13;

	if (SelColEnd == 1)
		SelColEnd = 2;
	if (SelColEnd == 4 || SelColEnd == 5)
		SelColEnd = 6;
	if (SelColEnd == 7 || SelColEnd == 8)
		SelColEnd = 9;
	if (SelColEnd == 10 || SelColEnd == 11)
		SelColEnd = 12;
	if (SelColEnd == 13 || SelColEnd == 14)
		SelColEnd = 15;

	switch (Column) {
		case 0:
			return (SelColStart == 0);
		case 1:
			return (SelColStart <= 1 && SelColEnd >= 2);
		case 2:
			return (SelColStart <= 3 && SelColEnd >= 3);
		case 3:
			return (SelColStart <= 4 && SelColEnd >= 6);
		case 4:
			return (SelColStart <= 7 && SelColEnd >= 9);
		case 5:
			return (SelColStart <= 10 && SelColEnd >= 12);
		case 6:
			return (SelColEnd == 15);
	}

	return false;
}

int GetMax(int x, int y)
{
	return (x > y ? x : y);
}

int GetMin(int x, int y)
{
	return (x < y ? x : y);
}

int GetColumnAtPoint(int PointX, int MaxColumns)
{
	int ColumnOffset, Offset = 0;
	int i;
	int Column = 0;

	ColumnOffset = PointX;

	for (i = 0; i < MaxColumns; i++) {
		Offset += COLUMN_SPACE[i];
		if (ColumnOffset < Offset) {
			Column = i;
			break;
		}
	}

	return Column;
}

// Convert keys 0-F to numbers
int ConvertKeyToHex(int Key) {

	switch (Key) {
		case 48: return 0x00;
		case 49: return 0x01;
		case 50: return 0x02;
		case 51: return 0x03;
		case 52: return 0x04;
		case 53: return 0x05;
		case 54: return 0x06;
		case 55: return 0x07;
		case 56: return 0x08;
		case 57: return 0x09;
		case 65: return 0x0A;
		case 66: return 0x0B;
		case 67: return 0x0C;
		case 68: return 0x0D;
		case 69: return 0x0E;
		case 70: return 0x0F;
	}

	return -1;
}

// Convert keys 0-9 to numbers
int ConvertKeyToDec(int Key)
{
	if (Key > 47 && Key < 58)
		return Key - 48;

	return -1;
}


// CFamiTrackerView construction/destruction

CFamiTrackerView::CFamiTrackerView()
{
	int i;

	m_bInitialized		= false;

	m_iCurrentRow		= 0;
	m_iCurrentFrame		= 0;
	m_iCursorRow		= 0;
	m_iKeyStepping		= 1;
	m_bEditEnable		= false;

	m_bPlayLooped		= false;
	m_bPlaying			= false;
	m_bShiftPressed		= false;
	m_bChangeAllPattern	= false;
	m_bPasteOverwrite	= false;
	m_bPasteMix			= false;

	m_iCursorChannel	= 0;
	m_iCursorRow		= 0;
	m_iCursorColumn		= 0;
	m_iStartChan		= 0;

	m_iSelectStart		= -1;
	m_iSelectEnd		= 0;
	m_iSelectChannel	= 0;
	m_iSelectColStart	= 0;
	m_iSelectColEnd		= 0;

	m_iOctave			= 3;
	m_iInstrument		= 0;

	m_iUndoLevel		= 0;
	m_iRedoLevel		= 0;

	m_iKeyboardNote		= -1;
	m_iSoloChannel		= -1;

	m_iTempo			= 150;
	m_iSpeed			= 6;
	m_iTickPeriod		= 6;

	m_iPlayTime			= 0;

	for (i = 0; i < MAX_CHANNELS; i++) {
		m_bMuteChannels[i] = false;

		CurrentNotes[i].EffNumber[0] = 0;
		CurrentNotes[i].EffParam[0] = 0;
		CurrentNotes[i].Instrument	= 0;
		CurrentNotes[i].Octave		= 0;
		CurrentNotes[i].Note		= 0;
		CurrentNotes[i].Vol			= 0;
	}

	for (i = 0; i < 256; i++)
		m_cKeyList[i] = 0;

	memset(Arpeggiate, 0, sizeof(int) * MAX_CHANNELS);
}

CFamiTrackerView::~CFamiTrackerView()
{
}

BOOL CFamiTrackerView::PreCreateWindow(CREATESTRUCT& cs)
{
	m_iClipBoard = RegisterClipboardFormat(CLIPBOARD_ID);

	if (m_iClipBoard == 0)
		theApp.DisplayError(IDS_CLIPBOARD_ERROR);

	return CView::PreCreateWindow(cs);
}


// CFamiTrackerView diagnostics

#ifdef _DEBUG
void CFamiTrackerView::AssertValid() const
{
	CView::AssertValid();
}

void CFamiTrackerView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CFamiTrackerDoc* CFamiTrackerView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFamiTrackerDoc)));
	return (CFamiTrackerDoc*)m_pDocument;
}
#endif //_DEBUG

// CFamiTrackerView message handlers

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tracker drawing routines
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::PrepareBackground(CDC *dc)
{
	const unsigned int EFF_COL_WIDTH = ((CHAR_WIDTH * 3) + COLUMN_SPACING);
	const char *CHANNEL_NAMES[] = {"Square 1", "Square 2", "Triangle", "Noise", "DPCM"};
	const char *CHANNEL_NAMES_VRC6[] = {"Square 1", "Square 2", "Sawtooth"};
	
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int ColBackground		= theApp.m_pSettings->Appearance.iColBackground;
	unsigned int ColHiBackground	= theApp.m_pSettings->Appearance.iColBackgroundHilite;
	unsigned int ColText			= theApp.m_pSettings->Appearance.iColPatternText;
	unsigned int ColHiText			= theApp.m_pSettings->Appearance.iColPatternTextHilite;
	unsigned int ColSelection		= theApp.m_pSettings->Appearance.iColSelection;
	unsigned int ColCursor			= theApp.m_pSettings->Appearance.iColCursor;

	CBrush Brush, ArrowBrush(0x80B0B0), *OldBrush;
	CPen Pen, ArrowPen(PS_SOLID, 1, 0x606060), *OldPen;

	CPoint ArrowPoints[3];

	unsigned int AvailableChannels = pDoc->GetAvailableChannels();
	unsigned int CurrentColumn = 0, ColumnCount = 0;
	unsigned int i, Offset, TotalWidth = 0, HiddenChannels = 0, VisibleWidth = 0;

	// Determine weither the view needs to be scrolled horizontally
	Offset = CHANNEL_START + CHANNEL_SPACING;

	m_iChannelsVisible = AvailableChannels;

	// Don't begin after the cursor
	if (m_iStartChan > m_iCursorChannel)
		m_iStartChan = m_iCursorChannel;

	// Calculate channel widths
	for (i = 0; i < AvailableChannels; i++) {		
		m_iChannelWidths[i] = CHANNEL_WIDTH + pDoc->GetEffColumns(i) * EFF_COL_WIDTH + CHANNEL_SPACING * 2;
		m_iChannelColumns[i] = COLUMNS + pDoc->GetEffColumns(i) * 3;

		if (m_iCursorChannel == i)
			CurrentColumn = ColumnCount + m_iCursorColumn;

		ColumnCount += COLUMNS + pDoc->GetEffColumns(i) * 3;

		TotalWidth += m_iChannelWidths[i];
	}

	Offset = CHANNEL_START + CHANNEL_SPACING;

	// Find out the first visible channel on screen
	for (i = m_iStartChan; i < AvailableChannels; i++) {
		if (m_iCursorChannel == i && (Offset + m_iChannelWidths[i]) > m_iWindowWidth) {
			m_iStartChan++;
			i = m_iStartChan;
			Offset = CHANNEL_START + CHANNEL_SPACING;
			continue;
		}

		Offset += m_iChannelWidths[i];
	}

	Offset = CHANNEL_START + CHANNEL_SPACING;

	for (i = m_iStartChan; i < AvailableChannels; i++) {
//		if (i < m_iStartChan)
//			HiddenChannels += m_iChannelWidths[i];
		if ((Offset + m_iChannelWidths[i]) <= m_iWindowWidth)
			VisibleWidth += m_iChannelWidths[i];

		Offset += m_iChannelWidths[i];
	}

	Offset = CHANNEL_START + CHANNEL_SPACING;

	// Create colors
	Brush.CreateSolidBrush(ColBackground);

	// Border color
	int BorderColor;

	if (m_bEditEnable)
		BorderColor = COLOR_SCHEME.BORDER_EDIT_A;
	else
		BorderColor = COLOR_SCHEME.BORDER_NORMAL_A;

	if (!m_bHasFocus)
		BorderColor	= DIM(BorderColor, 80);

	OldPen = dc->SelectObject(&Pen);
	OldBrush = dc->SelectObject(&Brush);

	// Clear background
	dc->FillSolidRect(0, 0, m_iWindowWidth - 4, m_iWindowHeight - 4, ColBackground);

	dc->SetBkMode(TRANSPARENT);

	const unsigned int GRAY = 0x606060;

	for (i = 0; i < 3; i++) {
		unsigned int iCol = DIM(BorderColor, (100 - i * 20));
		dc->Draw3dRect(i, i, m_iWindowWidth - 4 - i * 2, m_iWindowHeight - 4 - i * 2, iCol, iCol);
	}

	// Draw the channel header background
	for (i = 0; i < TOP_OFFSET && VisibleWidth > 0; i++) {
		dc->FillSolidRect(4, 4 + i, m_iWindowWidth - 12/*VisibleWidth + CHANNEL_START - 6*/, 1, DIM_TO(0x404040, ColBackground, ((TOP_OFFSET - i) * 100) / TOP_OFFSET));
	}

	int CurTopColor;

	dc->SelectObject(ArrowPen);
	dc->SelectObject(ArrowBrush);

	// Print channel headers
	for (i = m_iStartChan; i < AvailableChannels; i++) {
		// Draw vertical line
		dc->FillSolidRect(Offset - CHANNEL_SPACING - 3, 3, 1, m_iWindowHeight - 10, 0x808080);

		// If channel will go outside the window, break
		if (Offset + m_iChannelWidths[i] > m_iWindowWidth) {
			m_iChannelsVisible = i;
			break;
		}

		CurTopColor = DIM_TO(COLOR_SCHEME.TEXT_MUTED, COLOR_SCHEME.TEXT_HILITE, FadeCol[i]);

		// Print the text
		dc->SetTextColor(m_bMuteChannels[i] ? COLOR_SCHEME.TEXT_MUTED : COLOR_SCHEME.TEXT_HILITE);
		dc->SetBkColor(ColHiBackground);
		
		if (i > 4) {	// expansion names
			switch (pDoc->GetExpansionChip()) {
				case CHIP_VRC6: 
					dc->TextOut(Offset, TEXT_POSITION, CHANNEL_NAMES_VRC6[i - 5]);
					break;
			}
		}
		else { // internal names
/*			dc->SetTextColor(DIM(CurTopColor, 50));
			dc->TextOut(Offset + 1, TEXT_POSITION + 1, CHANNEL_NAMES[i]);

			dc->SetTextColor(CurTopColor);
			dc->TextOut(Offset, TEXT_POSITION, CHANNEL_NAMES[i]);*/
		}

		// Display the fx columns with some flashy text
		dc->SetTextColor(DIM(ColHiText, 40));

		if (pDoc->GetEffColumns(i) > 0)
			dc->TextOut(Offset + CHANNEL_WIDTH + 8, TEXT_POSITION + 12, "fx2");
		if (pDoc->GetEffColumns(i) > 1)
			dc->TextOut(Offset + CHANNEL_WIDTH + 48, TEXT_POSITION + 12, "fx3");
		if (pDoc->GetEffColumns(i) > 2)
			dc->TextOut(Offset + CHANNEL_WIDTH + 88, TEXT_POSITION + 12, "fx4");

		// Arrows for expanding/removing fx columns
		if (pDoc->GetEffColumns(i) > 0) {			
			ArrowPoints[0].SetPoint(Offset + CHANNEL_WIDTH - 17, 12);
			ArrowPoints[1].SetPoint(Offset + CHANNEL_WIDTH - 17, 12 + 10);
			ArrowPoints[2].SetPoint(Offset + CHANNEL_WIDTH - 17 - 5, 12 + 5);
			dc->Polygon(ArrowPoints, 3);
		}

		if (pDoc->GetEffColumns(i) < (MAX_EFFECT_COLUMNS - 1)) {
			ArrowPoints[0].SetPoint(Offset + CHANNEL_WIDTH - 11, 12);
			ArrowPoints[1].SetPoint(Offset + CHANNEL_WIDTH - 11, 12 + 10);
			ArrowPoints[2].SetPoint(Offset + CHANNEL_WIDTH - 11 + 5, 12 + 5);
			dc->Polygon(ArrowPoints, 3);
		}

		Offset += m_iChannelWidths[i];
	}

	// Draw the last vertical channel line
	dc->FillSolidRect(Offset - CHANNEL_SPACING - 3, 3, 1, m_iWindowHeight - 10, 0x808080);

	// Display if there's channels outside the screen
	dc->SetTextColor(COLOR_SCHEME.TEXT_HILITE);

	// Before
	if (m_iStartChan != 0)
		dc->TextOut(10, TEXT_POSITION, "<<", 2);

	// After
	if (m_iChannelsVisible != AvailableChannels && (m_iWindowWidth - Offset) > 24)
		dc->TextOut(Offset, TEXT_POSITION, ">>", 2);

	Offset -= 15;

	if (Offset > (m_iWindowWidth - 2))
		Offset = (m_iWindowWidth - 2);

	// Draw the horizontal line between the channel names and tracker field
	dc->FillSolidRect(3, TOP_OFFSET - 2, Offset, 1, 0x808080);

	// Delete objects
	dc->SelectObject(OldPen);
	dc->SelectObject(OldBrush);

	// Update horizontal scrollbars
	SetScrollRange(SB_HORZ, 0, ColumnCount - 1);
	SetScrollPos(SB_HORZ, CurrentColumn);
}

void CFamiTrackerView::DrawPatternField(CDC* dc)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	const int PREVIEW_FADE_LEVEL	= 40;

	unsigned int ColBackground		= theApp.m_pSettings->Appearance.iColBackground;
	unsigned int ColHiBackground	= theApp.m_pSettings->Appearance.iColBackgroundHilite;
	unsigned int ColText			= theApp.m_pSettings->Appearance.iColPatternText;
	unsigned int ColHiText			= theApp.m_pSettings->Appearance.iColPatternTextHilite;
	unsigned int ColSelection		= theApp.m_pSettings->Appearance.iColSelection;
	unsigned int ColCursor			= theApp.m_pSettings->Appearance.iColCursor;

	unsigned int SelectStart		= GetSelectStart();
	unsigned int SelectEnd			= GetSelectEnd();
	unsigned int SelectColStart		= GetSelectColStart();
	unsigned int SelectColEnd		= GetSelectColEnd();

	unsigned int CurrentBgCol, CurrentTextCol;

	unsigned int RowsInField;
	unsigned int i, x, c;

	unsigned int Offset, ColOffset;
	unsigned int Row, TransparentRow;
	unsigned int Width;
	unsigned int ThisFrame = m_iCurrentFrame;

	ScanActualLengths();

	RowsInField = m_iActualLengths[ThisFrame];

	stChanNote Note;
	CString RowText;

	int CursorDifference = signed(m_iCursorRow - m_iCurrentRow);

	// Adjust if cursor is locked or the tracker is playing
	if (m_bPlaying || !theApp.m_pSettings->General.bFreeCursorEdit) {
		m_iCurrentRow = m_iCursorRow;
	}
	else {
		// Adjust if cursor is out of screen (if free cursor)
		while (CursorDifference >= signed(m_iVisibleRows / 2) && CursorDifference > 0) {
			m_iCurrentRow++;
			CursorDifference = signed(m_iCursorRow - m_iCurrentRow);
		}

		while (-CursorDifference > signed(m_iVisibleRows / 2) && CursorDifference < 0) {
			m_iCurrentRow--;
			CursorDifference = signed(m_iCursorRow - m_iCurrentRow);
		}
	}

	if (m_iCursorRow > (RowsInField - 1))
		m_iCursorRow = RowsInField - 1;

	unsigned int RowCursor = m_iCursorRow;
	unsigned int MiddleRow = m_iCurrentRow;

	// Choose start row
	if (MiddleRow > (m_iVisibleRows / 2))
		Row = MiddleRow - (m_iVisibleRows / 2);
	else 
		Row = 0;

	int Line = 0;

	// Draw all rows
	for (i = 0; i < m_iVisibleRows; i++, Line++) {
		if ((MiddleRow - (m_iVisibleRows / 2) + Line) >= 0 && (MiddleRow - (m_iVisibleRows / 2) + Line) < RowsInField) {

			if ((Row & 3) == 0) {
				// Highlight
				CurrentTextCol = ColHiText;
				CurrentBgCol = ColHiBackground;

				dc->FillSolidRect(3, TOP_OFFSET + (ROW_HEIGHT * Line), CHANNEL_START - 6, ROW_HEIGHT - 2, CurrentBgCol);
			}
			else {
				CurrentTextCol = ColText;
				CurrentBgCol = ColBackground;
			}

			dc->SetTextColor(CurrentTextCol);

			if (theApp.m_pSettings->General.bRowInHex) {
				RowText.Format("%02X", Row);
				dc->TextOut(LEFT_OFFSET, TOP_OFFSET + Line * ROW_HEIGHT, RowText);
			}
			else {
				RowText.Format("%03i", Row);
				dc->TextOut(LEFT_OFFSET - 3, TOP_OFFSET + Line * ROW_HEIGHT, RowText);
			}

			Offset = CHANNEL_START + CHANNEL_SPACING;

			for (x = m_iStartChan; x < m_iChannelsVisible; x++) {
				ColOffset = 0;

				// Highlighted row, draw bg
				if (!(Row & 3) &&  x < 5 && Row != RowCursor) {
					Width = CHAN_AREA_WIDTH + pDoc->GetEffColumns(x) * (CHAR_WIDTH * 3 + COLUMN_SPACING) - 1;
					dc->FillSolidRect(Offset - 2 - CHANNEL_SPACING, TOP_OFFSET + (ROW_HEIGHT * Line), Width, ROW_HEIGHT - 2, /*ColHiBackground*/ CurrentBgCol);
				}
				else if (Row == RowCursor) {
					Width = CHAN_AREA_WIDTH + pDoc->GetEffColumns(x) * (CHAR_WIDTH * 3 + COLUMN_SPACING) - 1;
					dc->FillSolidRect(Offset - 2 - CHANNEL_SPACING, TOP_OFFSET + (ROW_HEIGHT * Line), Width, ROW_HEIGHT - 2, /*ColHiBackground*/ DIM_TO(ColCursor, CurrentBgCol, 20));
				}

				// Selection
				if (m_iSelectStart != -1 && x == m_iSelectChannel && Row >= SelectStart && Row <= SelectEnd) {
					int SelStart	= TOP_OFFSET + ROW_HEIGHT * Line /*(Row + ((m_iVisibleRows / 2) - RowCursor))*/;
					int SelChan		= Offset + COLUMN_SEL[SelectColStart] - 4;
					int SelChanEnd	= Offset + COLUMN_SEL[SelectColEnd + 1] - SelChan - 4;
					dc->FillSolidRect(SelChan, SelStart - 1, SelChanEnd, ROW_HEIGHT, ColSelection);
				}

				// Draw all columns
				for (c = 0; c < m_iChannelColumns[x]; c++) {

					// Cursor box
					if (Row == RowCursor && x == m_iCursorChannel && c == m_iCursorColumn) {
						unsigned int BoxTop	= TOP_OFFSET + ROW_HEIGHT * Line;
						unsigned int SelChan = Offset + ColOffset - 2;

						dc->FillSolidRect(SelChan, BoxTop - 1, CURSOR_SPACE[m_iCursorColumn], ROW_HEIGHT, ColCursor);
					}

					pDoc->GetNoteData(ThisFrame, x, Row, &Note);
					DrawLine(Offset + ColOffset, &Note, Line, c, x, CurrentTextCol, dc);

					ColOffset += COLUMN_SPACE[c];
				}

				Offset += m_iChannelWidths[x];
			}

			Row++;
		}

		// Previous
		else if ((ThisFrame > 0) && int(MiddleRow - (m_iVisibleRows / 2) + Line) < 0 && int(MiddleRow - (m_iVisibleRows / 2) + Line) >= -int(m_iActualLengths[ThisFrame - 1]) && theApp.m_pSettings->General.bFramePreview) {
			
			TransparentRow = int(m_iActualLengths[ThisFrame - 1]) + int(MiddleRow - (m_iVisibleRows / 2) + Line);

			if ((TransparentRow & 3) == 0) {
				// Highlight
				CurrentTextCol = DIM_TO(ColHiText, ColHiBackground, PREVIEW_FADE_LEVEL);
				CurrentBgCol = ColHiBackground;

				dc->FillSolidRect(3, TOP_OFFSET + (ROW_HEIGHT * Line), CHANNEL_START - 6, ROW_HEIGHT - 2, CurrentBgCol);
			}
			else {
				CurrentTextCol = DIM_TO(ColText, ColBackground, PREVIEW_FADE_LEVEL);
				CurrentBgCol = ColBackground;
			}

			dc->SetTextColor(CurrentTextCol);

			if (theApp.m_pSettings->General.bRowInHex) {
				RowText.Format("%02X", TransparentRow);
				dc->TextOut(LEFT_OFFSET, TOP_OFFSET + Line * ROW_HEIGHT, RowText);
			}
			else {
				RowText.Format("%03i", TransparentRow);
				dc->TextOut(LEFT_OFFSET - 3, TOP_OFFSET + Line * ROW_HEIGHT, RowText);
			}

			Offset = CHANNEL_START + CHANNEL_SPACING;

			for (x = m_iStartChan; x < m_iChannelsVisible; x++) {
				ColOffset = 0;

				// Highlighted row, draw bg
				if (!(TransparentRow & 3) &&  x < 5) {
					Width = CHAN_AREA_WIDTH + pDoc->GetEffColumns(x) * (CHAR_WIDTH * 3 + COLUMN_SPACING) - 1;
					dc->FillSolidRect(Offset - 2 - CHANNEL_SPACING, TOP_OFFSET + (ROW_HEIGHT * Line), Width, ROW_HEIGHT - 2, /*ColHiBackground*/ CurrentBgCol);
				}

				// Draw all columns
				for (c = 0; c < m_iChannelColumns[x]; c++) {
					dc->SetBkColor(CurrentBgCol);
					pDoc->GetNoteData(ThisFrame - 1, x, TransparentRow, &Note);
					DrawLine(Offset + ColOffset, &Note, Line, c, x, CurrentTextCol, dc);
					ColOffset += COLUMN_SPACE[c];
				}

				Offset += m_iChannelWidths[x];
			}
		}
		// Next
		else if ((ThisFrame < (pDoc->GetFrameCount() - 1)) && int(MiddleRow - (m_iVisibleRows / 2) + Line) >= int(RowsInField) && int(MiddleRow - (m_iVisibleRows / 2) + Line) < int(RowsInField + m_iActualLengths[ThisFrame + 1]) && theApp.m_pSettings->General.bFramePreview) {

			TransparentRow = int(MiddleRow - (m_iVisibleRows / 2) + Line) - RowsInField;

			if ((TransparentRow & 3) == 0) {
				// Highlight
				CurrentTextCol = DIM_TO(ColHiText, ColHiBackground, PREVIEW_FADE_LEVEL);
				CurrentBgCol = ColHiBackground;

				dc->FillSolidRect(3, TOP_OFFSET + (ROW_HEIGHT * Line), CHANNEL_START - 6, ROW_HEIGHT - 2, CurrentBgCol);
			}
			else {
				CurrentTextCol = DIM_TO(ColText, ColBackground, PREVIEW_FADE_LEVEL);
				CurrentBgCol = ColBackground;
			}

			dc->SetTextColor(CurrentTextCol);
			dc->SetBkColor(CurrentBgCol);

			if (theApp.m_pSettings->General.bRowInHex) {
				RowText.Format("%02X", TransparentRow);
				dc->TextOut(LEFT_OFFSET, TOP_OFFSET + Line * ROW_HEIGHT, RowText);
			}
			else {
				RowText.Format("%03i", TransparentRow);
				dc->TextOut(LEFT_OFFSET - 3, TOP_OFFSET + Line * ROW_HEIGHT, RowText);
			}

			Offset = CHANNEL_START + CHANNEL_SPACING;

			for (x = m_iStartChan; x < m_iChannelsVisible; x++) {
				ColOffset = 0;

				// Highlighted row, draw bg
				if (!(TransparentRow & 3) &&  x < 5) {
					Width = CHAN_AREA_WIDTH + pDoc->GetEffColumns(x) * (CHAR_WIDTH * 3 + COLUMN_SPACING) - 1;
					dc->FillSolidRect(Offset - 2 - CHANNEL_SPACING, TOP_OFFSET + (ROW_HEIGHT * Line), Width, ROW_HEIGHT - 2, CurrentBgCol);
				}

				// Draw all columns
				for (c = 0; c < m_iChannelColumns[x]; c++) {
					dc->SetBkColor(CurrentBgCol);
					pDoc->GetNoteData(m_iNextPreviewFrame[ThisFrame]/* + 1*/, x, TransparentRow, &Note);
					DrawLine(Offset + ColOffset, &Note, Line, c, x, CurrentTextCol, dc);
					ColOffset += COLUMN_SPACE[c];
				}

				Offset += m_iChannelWidths[x];
			}
		}
		//Line++;
	}

	unsigned int CursorCol1 = DIM(ColCursor, 100);
	unsigned int CursorCol2 = DIM(ColCursor, 80);
	unsigned int CursorCol3 = DIM(ColCursor, 60);

	// Selected row
	dc->Draw3dRect(3, TOP_OFFSET + (m_iVisibleRows / 2) * ROW_HEIGHT - 2, m_iWindowWidth - 10, ROW_HEIGHT + 2, CursorCol3, CursorCol3);
	dc->Draw3dRect(4, TOP_OFFSET + (m_iVisibleRows / 2) * ROW_HEIGHT - 1, m_iWindowWidth - 12, ROW_HEIGHT, CursorCol2, CursorCol2);
	dc->Draw3dRect(5, TOP_OFFSET + (m_iVisibleRows / 2) * ROW_HEIGHT, m_iWindowWidth - 14, ROW_HEIGHT - 2, CursorCol1, CursorCol1);

	DrawMeters(dc);
	DrawChannelHeaders(dc);

	// Update vertical scrollbar
	if (RowsInField > 1)
		SetScrollRange(SB_VERT, 0, RowsInField - 1);
	else
		SetScrollRange(SB_VERT, 0, 1);

	SetScrollPos(SB_VERT, m_iCurrentRow);

#ifdef WIP
	CString Wip;
	Wip.Format("WIP %i", VERSION_WIP);
	dc->SetBkColor(COLOR_SCHEME.CURSOR);
	dc->SetTextColor(COLOR_SCHEME.TEXT_HILITE);
	dc->TextOut(m_iWindowWidth - 47, m_iWindowHeight - 22, Wip);
#endif

	/*
	CString Beta;
	Beta.Format("BETA");
	dc->SetBkColor(COLOR_SCHEME.CURSOR);
	dc->SetTextColor(COLOR_SCHEME.TEXT_HILITE);
	dc->TextOut(m_iWindowWidth - 47, m_iWindowHeight - 22, Beta);
	*/
}

void CFamiTrackerView::OnDraw(CDC* pDC)
{	
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	static DWORD LastTime;

	LOGFONT LogFont;
	CBitmap Bmp, *OldBmp;
	CFont	Font, *OldFont;
	CDC		*BackDC;

	LPCSTR	FontName = theApp.m_pSettings->General.strFont;

	// Make sure this isn't flooded
	if ((GetTickCount() - LastTime) < 30 && m_bPlaying)
		return;

	LastTime = GetTickCount();

	// Check document
	if (!pDoc->IsFileLoaded()) {
		pDC->FillSolidRect(0, 0, m_iWindowWidth, m_iWindowHeight, 0x000000);
		pDC->SetTextColor(0xFFFFFF);
		pDC->TextOut(m_iWindowWidth / 2 - 84, m_iWindowHeight / 2 - 4, "No document is loaded");
		return;
	}

	// Set up the drawing area
	BackDC = new CDC;

	// Create the font
	memset(&LogFont, 0, sizeof LOGFONT);
	memcpy(LogFont.lfFaceName, FontName, strlen(FontName));

	LogFont.lfHeight = -12;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;

	Font.CreateFontIndirect(&LogFont);

	// Setup dc
	Bmp.CreateCompatibleBitmap(pDC, m_iWindowWidth, m_iWindowHeight);
	BackDC->CreateCompatibleDC(pDC);
	OldBmp = BackDC->SelectObject(&Bmp);
	OldFont = BackDC->SelectObject(&Font);

	// Create the background
	PrepareBackground(BackDC);

	// Draw the field
	DrawPatternField(BackDC);

	// Display area and clean up
	pDC->BitBlt(0, 0, m_iWindowWidth, m_iWindowHeight, BackDC, 0, 0, SRCCOPY);

	BackDC->SelectObject(OldBmp);
	BackDC->SelectObject(OldFont);

	delete BackDC;
}

void CFamiTrackerView::DrawLine(int Offset, stChanNote *NoteData, int Line, int Column, int Channel, int Color, CDC *dc)
{
	const char NOTES_A[] = {'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'};
	const char NOTES_B[] = {'-', '#', '-', '#', '-', '-', '#', '-', '#', '-', '#', '-'};
	const char NOTES_C[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	const char HEX[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

	int LineY		= TOP_OFFSET + Line * ROW_HEIGHT;
	int OffsetX		= Offset;

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

	int ShadedCol = DIM(Color, 50);

	switch (Column) {
		case 0:
			// Note and octave
			switch (Note) {
				case 0:
				case HALT:
					if (Note == 0) {
						DrawChar(OffsetX, LineY, '-', ShadedCol, dc);
						DrawChar(OffsetX + CHAR_WIDTH, LineY, '-', ShadedCol, dc);
						DrawChar(OffsetX + CHAR_WIDTH * 2, LineY, '-', ShadedCol, dc);
					}
					else 
					{
						DrawChar(OffsetX, LineY, '*', Color, dc);
						DrawChar(OffsetX + CHAR_WIDTH, LineY, '*', Color, dc);
						DrawChar(OffsetX + CHAR_WIDTH * 2, LineY, '*', Color, dc);
					}
					break;
				default:
					DrawChar(OffsetX, LineY, NOTES_A[Note - 1], Color, dc);
					DrawChar(OffsetX + CHAR_WIDTH, LineY, NOTES_B[Note - 1], Color, dc);
					DrawChar(OffsetX + CHAR_WIDTH * 2, LineY, NOTES_C[Octave], Color, dc);
			}
			break;
		case 1:
			// Instrument x0
			if (Instrument == MAX_INSTRUMENTS || Note == HALT)
				DrawChar(OffsetX, LineY, '-', ShadedCol, dc);
			else
				DrawChar(OffsetX, LineY, HEX[Instrument >> 4], Color, dc);
			break;
		case 2:
			// Instrument 0x
			if (Instrument == MAX_INSTRUMENTS || Note == HALT)
				DrawChar(OffsetX, LineY, '-', ShadedCol, dc);
			else
				DrawChar(OffsetX, LineY, HEX[Instrument & 0x0F], Color, dc);
			break;
		case 3:
			// Volume (skip triangle)
			if (Vol == 0x10 || Channel == 2 || Channel == 4)
				DrawChar(OffsetX, LineY, '-', ShadedCol, dc);
			else
				DrawChar(OffsetX, LineY, HEX[Vol & 0x0F], Color, dc);
			break;
		case 4: case 7: case 10: case 13:
			// Effect type
			if (EffNumber == 0)
				DrawChar(OffsetX, LineY, '-', ShadedCol, dc);
			else
				DrawChar(OffsetX, LineY, EFF_CHAR[(EffNumber - 1) & 0x0F], Color, dc);
			break;
		case 5: case 8: case 11: case 14:
			// Effect param x
			if (EffNumber == 0)
				DrawChar(OffsetX, LineY, '-', ShadedCol, dc);
			else
				DrawChar(OffsetX, LineY, HEX[(EffParam >> 4) & 0x0F], Color, dc);
			break;
		case 6: case 9: case 12: case 15:
			// Effect param y
			if (EffNumber == 0)
				DrawChar(OffsetX, LineY, '-', ShadedCol, dc);
			else
				DrawChar(OffsetX, LineY, HEX[EffParam & 0x0F], Color, dc);
			break;
	}

}

void CFamiTrackerView::DrawChar(int x, int y, char c, int Color, CDC *dc) 
{
	static CString Text;
	Text.Format("%c", c);
	dc->SetTextColor(Color);
	dc->TextOut(x, y, Text);	
}

void CFamiTrackerView::DrawChannelHeaders(CDC *pDC)
{
	// Only draw the channel names
	//
	
	const char *CHANNEL_NAMES[] = {"Square 1", "Square 2", "Triangle", "Noise", "DPCM"};
	const char *CHANNEL_NAMES_VRC6[] = {"Square 1", "Square 2", "Sawtooth"};

	CFamiTrackerDoc* pDoc = GetDocument();
	unsigned int Offset = CHANNEL_START + CHANNEL_SPACING;
	unsigned int AvailableChannels = pDoc->GetAvailableChannels();
	unsigned int CurTopColor, i;

	LOGFONT LogFont;
	CFont	Font, *OldFont;
	LPCSTR	FontName = theApp.m_pSettings->General.strFont;

	pDC->SetBkMode(TRANSPARENT);

	// Create the font
	memset(&LogFont, 0, sizeof LOGFONT);
	memcpy(LogFont.lfFaceName, FontName, strlen(FontName));

	LogFont.lfHeight = -12;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;

	Font.CreateFontIndirect(&LogFont);

	OldFont = pDC->SelectObject(&Font);

	for (i = m_iStartChan; i < AvailableChannels; i++) {

		// If channel will go outside the window, break
		if (Offset + m_iChannelWidths[i] > m_iWindowWidth) {
			m_iChannelsVisible = i;
			break;
		}

		CurTopColor = m_bMuteChannels[i] ? COLOR_SCHEME.TEXT_MUTED : COLOR_SCHEME.TEXT_HILITE;

		if (m_bMuteChannels[i]) {
			if (FadeCol[i] < 100)
				FadeCol[i] += 20;
			if (FadeCol[i] > 100)
				FadeCol[i] = 100;
		}
		else {
			if (FadeCol[i] > 0)
				FadeCol[i] -= 20;
			if (FadeCol[i] < 0)
				FadeCol[i] = 0;
		}

		CurTopColor = DIM_TO(COLOR_SCHEME.TEXT_MUTED, COLOR_SCHEME.TEXT_HILITE, FadeCol[i]);

		// Print the text
//		pDC->SetTextColor(m_bMuteChannels[i] ? COLOR_SCHEME.TEXT_MUTED : COLOR_SCHEME.TEXT_HILITE);
//		pDC->SetBkColor(ColHiBackground);
		
		if (i > 4) {	// expansion names
			switch (pDoc->GetExpansionChip()) {
				case CHIP_VRC6: 
					pDC->TextOut(Offset, TEXT_POSITION, CHANNEL_NAMES_VRC6[i - 5]);
					break;
			}
		}
		else { // internal names
			pDC->SetTextColor(DIM(CurTopColor, 50));
			pDC->TextOut(Offset + 1, TEXT_POSITION + 1, CHANNEL_NAMES[i]);

			pDC->SetTextColor(CurTopColor);
			pDC->TextOut(Offset, TEXT_POSITION, CHANNEL_NAMES[i]);
		}

		Offset += m_iChannelWidths[i];
	}

	pDC->SelectObject(OldFont);
}

void CFamiTrackerView::DrawMeters(CDC *pDC)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CBrush	*OldBrush, Black;
	CPen	*OldPen, Green;

	CBrush	GreenBrush, HalfGreenBrush, GreyBrush;
	CPoint	points[5];

	const int BAR_TOP		= TEXT_POSITION + 21;
	const int BAR_LEFT		= CHANNEL_START + CHANNEL_SPACING - 2;
	const int BAR_SIZE		= 8;
	const int BAR_SPACE		= 1;
	const int BAR_HEIGHT	= 5;

	unsigned int Offset = BAR_LEFT;
	unsigned int i, c;

	if (!pDoc->IsFileLoaded())
		return;

	Black.CreateSolidBrush(0xFF000000);
	Green.CreatePen(0, 0, 0x103030);

	GreenBrush.CreateSolidBrush(0x20F020);
	GreyBrush.CreateSolidBrush(0x406040);

	OldBrush = pDC->SelectObject(&Black);
	OldPen = pDC->SelectObject(&Green);

	for (c = m_iStartChan; c < m_iChannelsVisible; c++) {

		points[0].x = Offset; points[0].y = BAR_TOP - 1;
		points[1].x = Offset + (15 * BAR_SIZE) - 1; points[1].y = BAR_TOP - 1;
		points[2].x = Offset + (15 * BAR_SIZE) - 1; points[2].y = BAR_TOP + BAR_HEIGHT;
		points[3].x = Offset - 1; points[3].y = BAR_TOP + BAR_HEIGHT;
		points[4].x = Offset - 1; points[4].y = BAR_TOP - 2;

		pDC->Polyline(points, 5);

		for (i = 0; i < 15; i++) {
			if (i < m_iVolLevels[c])
				pDC->FillRect(CRect(Offset + (i * BAR_SIZE), BAR_TOP, Offset + (i * BAR_SIZE) + (BAR_SIZE - BAR_SPACE), BAR_TOP + BAR_HEIGHT), &GreenBrush);
			else
				pDC->FillRect(CRect(Offset + (i * BAR_SIZE), BAR_TOP, Offset + (i * BAR_SIZE) + (BAR_SIZE - BAR_SPACE), BAR_TOP + BAR_HEIGHT), &GreyBrush);
		}
		Offset += CHANNEL_WIDTH + pDoc->GetEffColumns(c) * (CHAR_WIDTH * 3 + COLUMN_SPACING) + CHANNEL_SPACING * 2;
	}

	pDC->SelectObject(OldBrush);
	pDC->SelectObject(OldPen);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tracker playing routines
////////////////////////////////////////////////////////////////////////////////////////////////////////////

int CFamiTrackerView::PlayerCommand(char Command, int Value)
{
	// Handle commands from the player

	CFamiTrackerDoc* pDoc = GetDocument();
	stChanNote NoteData;

	switch (Command) {
		case CMD_MOVE_TO_TOP:
			m_iCursorRow = 0;
			m_bForceRedraw = true;
			break;

		case CMD_STEP_DOWN:
			m_iCursorRow++;

			if (m_iCursorRow >= pDoc->GetPatternLength()) {
				m_iCursorRow = 0;

				if (Value == 0) {
					m_iCurrentFrame++;
					if (m_iCurrentFrame < 0) {
						m_iCurrentFrame += pDoc->GetFrameCount();
					}
					else if (m_iCurrentFrame > pDoc->GetFrameCount() - 1) {
						m_iCurrentFrame -= pDoc->GetFrameCount();
					}
				}
			}
			m_bForceRedraw = true;
			break;

		case CMD_JUMP_TO:
			m_iCurrentFrame = Value + 1;
			m_iCursorRow = 0;
			if (m_iCurrentFrame > signed(pDoc->GetFrameCount() - 1))
				m_iCurrentFrame = 0;
			m_bForceRedraw = true;
			break;

		case CMD_SKIP_TO:
			m_iCurrentFrame++;
			if (m_iCurrentFrame < 0) {
				m_iCurrentFrame += pDoc->GetFrameCount();
			}
			else if (m_iCurrentFrame > signed(pDoc->GetFrameCount() - 1)) {
				m_iCurrentFrame -= pDoc->GetFrameCount();
			}
			m_iCursorRow = Value;
			m_bForceRedraw = true;
			break;

		case CMD_UPDATE_ROW:
			for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
				pDoc->GetNoteData(m_iCurrentFrame, i, m_iCursorRow, &NoteData);
				if (!m_bMuteChannels[i]) {
					FeedNote(i, &NoteData);
				}
				else {					
					NoteData.Note = 0;
					NoteData.Octave = 0;
					NoteData.Instrument = 0;
					for (int j = 0; j < 4; j++) {
						if (NoteData.EffNumber[i] != EF_HALT &&
							NoteData.EffNumber[i] != EF_JUMP &&
							NoteData.EffNumber[i] != EF_SPEED &&
							NoteData.EffNumber[i] != EF_SKIP)
							NoteData.EffNumber[i] = EF_NONE;
					}
					FeedNote(i, &NoteData);
				}
			}
			break;

		case CMD_TIME:
			m_iPlayTime = Value;
			break;

		case CMD_TICK:
			// This would probably be cool, sometime...

			if (KeyCount == 1 || !theApp.m_pSettings->Midi.bMidiArpeggio)
				return 0;

			int OldPtr = ArpeggioPtr;
			do {
				ArpeggioPtr = (ArpeggioPtr + 1) & 127;
				
				if (ArpeggioNotes[ArpeggioPtr] == 1 /*&& LastArpPtr != ArpeggioPtr*/) {
					LastArpPtr = ArpeggioPtr;
					Arpeggiate[m_iCursorChannel] = ArpeggioPtr;
					break;
				}
 				else if (ArpeggioNotes[ArpeggioPtr] == 2) {
					ArpeggioNotes[ArpeggioPtr] = 0;
				//	stChanNote Note;
				//	memset(&Note, 0, sizeof(stChanNote));
				//	FeedNote(m_iCursorChannel, &Note);
				}
			}
			while (ArpeggioPtr != OldPtr);
			
			break;
	}

	return 0;
}

void CFamiTrackerView::GetRow(CFamiTrackerDoc *pDaoc)
{
	stChanNote NoteData;
	CFamiTrackerDoc* pDoc = GetDocument();

	for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
		if (!m_bMuteChannels[i]) {
			pDoc->GetNoteData(m_iCurrentFrame, i, m_iCursorRow, &NoteData);
			FeedNote(i, &NoteData);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General
////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CFamiTrackerView::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

void CFamiTrackerView::CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType)
{
	m_iWindowWidth	= (lpClientRect->right - lpClientRect->left) - 17;
	m_iWindowHeight	= (lpClientRect->bottom - lpClientRect->top) - 17;
	m_iVisibleRows	= (((m_iWindowHeight - 8) + (ROW_HEIGHT / 2)) / ROW_HEIGHT) - 3;

	CView::CalcWindowRect(lpClientRect, nAdjustType);
}

BOOL CFamiTrackerView::PreTranslateMessage(MSG* pMsg)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	switch (pMsg->message) {
		case WM_USER + 1:
			if (m_bInitialized)
				TranslateMidiMessage();
			break;
	}

	return CView::PreTranslateMessage(pMsg);
}

void CFamiTrackerView::SetMessageText(LPCSTR lpszText)
{
	GetParentFrame()->SetMessageText(lpszText);
}

void CFamiTrackerView::ForceRedraw()
{
	GetDocument()->UpdateAllViews(NULL);
}

void CFamiTrackerView::SetSpeed(int Speed)
{
	m_iTickPeriod = Speed;
}

void CFamiTrackerView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	switch (nSBCode) {
		case SB_LINEDOWN:	MoveCursorDown();		break;
		case SB_LINEUP:		MoveCursorUp();			break;
		case SB_PAGEDOWN:	MoveCursorPageDown();	break;
		case SB_PAGEUP:		MoveCursorPageUp();		break;
		case SB_TOP:		MoveCursorToTop();		break;
		case SB_BOTTOM:		MoveCursorToBottom();	break;

		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			m_iCursorRow = nPos;
			ForceRedraw();
			break;
		case SB_ENDSCROLL:
			return;
	}

	CView::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CFamiTrackerView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int i, x, count = 0;

	switch (nSBCode) {
		case SB_LINERIGHT:	MoveCursorRight();			break;
		case SB_LINELEFT:	MoveCursorLeft();			break;
		case SB_PAGERIGHT:	MoveCursorNextChannel();	break;
		case SB_PAGELEFT:	MoveCursorPrevChannel();	break;
		case SB_RIGHT:		MoveCursorFirstChannel();	break;
		case SB_LEFT:		MoveCursorLastChannel();	break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			for (i = 0; i < pDoc->GetAvailableChannels(); i++) {
				for (x = 0; x < signed(COLUMNS + pDoc->GetEffColumns(i) * 3); x++) {
					if (count++ == nPos) {
						m_iCursorColumn = x;
						m_iCursorChannel = i;
						ForceRedraw();
						CView::OnHScroll(nSBCode, nPos, pScrollBar);
						return;
					}
				}
			}
			break;
		case SB_ENDSCROLL:
			return;
	}

	CView::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CFamiTrackerView::CheckSelectionBeforeMove()
{
	if (m_bShiftPressed) {
		// If there was no selection
		if (m_iSelectStart == -1) {
			m_iSelectStart = m_iCursorRow;
			m_iSelectColStart = m_iCursorColumn;
		}
	}
}

void CFamiTrackerView::CheckSelectionAfterMove()
{
	if (m_bShiftPressed) {
		m_iSelectEnd = m_iCursorRow;
		m_iSelectColEnd	= m_iCursorColumn;
		m_iSelectChannel = m_iCursorChannel;
	}
}

void CFamiTrackerView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{	
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	switch (nChar) {
		case VK_UP:
			CheckSelectionBeforeMove();
			MoveCursorUp();
			CheckSelectionAfterMove();
			break;
		case VK_DOWN:
			CheckSelectionBeforeMove();
			MoveCursorDown();
			CheckSelectionAfterMove();
			break;
		case VK_HOME:
			CheckSelectionBeforeMove();
			MoveCursorToTop();		
			CheckSelectionAfterMove();
			break;
		case VK_END:	
			CheckSelectionBeforeMove();
			MoveCursorToBottom();	
			CheckSelectionAfterMove();
			break;
		case VK_PRIOR:	
			CheckSelectionBeforeMove();
			MoveCursorPageUp();		
			CheckSelectionAfterMove();
			break;
		case VK_NEXT:	
			CheckSelectionBeforeMove();
			MoveCursorPageDown();	
			CheckSelectionAfterMove();
			break;
		case VK_LEFT:
			CheckSelectionBeforeMove();
			MoveCursorLeft();
			CheckSelectionAfterMove();
			break;
		case VK_RIGHT:
			CheckSelectionBeforeMove();
			MoveCursorRight();
			CheckSelectionAfterMove();
			break;

		case VK_SPACE:	
			OnTrackerEdit();	
			break;

		case VK_ADD:
			if (m_bEditEnable) {
				AddUndo(m_iCursorChannel);
				switch (m_iCursorColumn) {
					case C_NOTE: 
						IncreaseCurrentPattern(); 
						break;
					case C_INSTRUMENT1:
					case C_INSTRUMENT2: 
						pDoc->IncreaseInstrument(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow); 	
						ForceRedraw(); 
						break;
					case C_VOLUME: 
						pDoc->IncreaseVolume(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow);	
						ForceRedraw(); 
						break;
					case C_EFF_NUM: case C_EFF_PARAM1: case C_EFF_PARAM2: 
						pDoc->IncreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, 0);
						ForceRedraw(); 
						break;
					case C_EFF2_NUM: case C_EFF2_PARAM1: case C_EFF2_PARAM2: 
						pDoc->IncreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, 1);
						ForceRedraw(); 
						break;
					case C_EFF3_NUM: case C_EFF3_PARAM1: case C_EFF3_PARAM2: 
						pDoc->IncreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, 2);
						ForceRedraw(); 
						break;
					case C_EFF4_NUM: case C_EFF4_PARAM1: case C_EFF4_PARAM2: 
						pDoc->IncreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, 3);
						ForceRedraw(); 
						break;
				}
			}
			break;

		case VK_SUBTRACT:
			if (m_bEditEnable) {
				AddUndo(m_iCursorChannel);
				switch (m_iCursorColumn) {
					case C_NOTE: 
						DecreaseCurrentPattern(); 
						break;
					case C_INSTRUMENT1:
					case C_INSTRUMENT2:
						pDoc->DecreaseInstrument(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow); 	
						ForceRedraw(); 
						break;
					case C_VOLUME: 
						pDoc->DecreaseVolume(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow);	
						ForceRedraw(); 
						break;
					case C_EFF_NUM: case C_EFF_PARAM1: case C_EFF_PARAM2: 
						pDoc->DecreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, 0);
						ForceRedraw(); 
						break;
					case C_EFF2_NUM: case C_EFF2_PARAM1: case C_EFF2_PARAM2: 
						pDoc->DecreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, 1);
						ForceRedraw(); 
						break;
					case C_EFF3_NUM: case C_EFF3_PARAM1: case C_EFF3_PARAM2: 
						pDoc->DecreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, 2); 
						ForceRedraw(); 
						break;
					case C_EFF4_NUM: case C_EFF4_PARAM1: case C_EFF4_PARAM2: 					
						pDoc->DecreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, 3); 
						ForceRedraw(); 
						break;
				}
			}
			break;

		case VK_SHIFT:
			m_bShiftPressed = true;
			break;

		case VK_TAB:
			if (m_bShiftPressed)
				MoveCursorPrevChannel();
			else
				MoveCursorNextChannel();
			ForceRedraw();
			break;

		case VK_DELETE:
			if (m_iSelectStart != -1) {
				if (PreventRepeat(VK_DELETE))
					return;
				OnEditDelete();
			}
			else {
				if (PreventRepeat(VK_DELETE) || !m_bEditEnable)
					return;
				AddUndo(m_iCursorChannel);
				if (pDoc->DeleteNote(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, m_iCursorColumn)) {
					StepDown();
					ForceRedraw();
				}
			}
			break;

		case VK_INSERT:
			if (!m_bEditEnable || PreventRepeat(VK_INSERT))
				return;
			AddUndo(m_iCursorChannel);
			pDoc->InsertNote(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow);
			ForceRedraw();
			break;

		case VK_BACK:
			if (m_iSelectStart != -1) {
				RemoveWithoutDelete();
			}
			else {
				if (m_iCursorRow == 0 || !m_bEditEnable || PreventRepeat(VK_BACK))
					return;
				AddUndo(m_iCursorChannel);
				if (pDoc->RemoveNote(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow)) {
					MoveCursorUp();
					ForceRedraw();
				}
			}
			break;

		case VK_F2: m_iOctave = 0; break;
		case VK_F3: m_iOctave = 1; break;
		case VK_F4: m_iOctave = 2; break;
		case VK_F5: m_iOctave = 3; break;
		case VK_F6: m_iOctave = 4; break;
		case VK_F7: m_iOctave = 5; break;
		case VK_F8: m_iOctave = 6; break;
		case VK_F9: m_iOctave = 7; break;

		case VK_ESCAPE:
			m_iSelectStart = -1;
			ForceRedraw();
			break;

		case 255:
			break;

		default:
			HandleKeyboardInput(nChar);
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CFamiTrackerView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_SHIFT)
		m_bShiftPressed = false;

	KeyReleased(int(nChar));

	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

BOOL CFamiTrackerView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// FIX THIS!!!

	if (zDelta > 0) {
		m_iCursorRow -= zDelta / 28;
		if (signed(m_iCursorRow) < 0) {
			if (theApp.m_pSettings->General.bWrapCursor)
				m_iCursorRow = (GetCurrentPatternLength() - 1);
			else
				m_iCursorRow = 0;
		}
	}
	else if (zDelta < 0) {
		m_iCursorRow -= zDelta / 28;
		if (m_iCursorRow > signed(GetCurrentPatternLength() - 1)) {
			if (theApp.m_pSettings->General.bWrapCursor)
				m_iCursorRow = 0;
			else
				m_iCursorRow = (GetCurrentPatternLength() - 1);
		}
	}

	ForceRedraw();

	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

unsigned int CFamiTrackerView::GetChannelAtPoint(unsigned int PointX)
{
	unsigned int i, ChanSize, Offset = 0, Channel = -1;

	for (i = m_iStartChan; i < m_iChannelsVisible; i++) {
		ChanSize = m_iChannelWidths[i];
		Offset += ChanSize;
		if ((PointX - CHANNEL_START) < Offset) {
			Channel = i;
			break;
		}
	}

	return Channel;
}

unsigned int CFamiTrackerView::GetColumnAtPoint(unsigned int PointX, unsigned int MaxColumns)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int i, Offset;
	int ColumnOffset = -1, Column = -1;
	int ChanSize, Channel = -1;

	Offset = CHANNEL_START + CHANNEL_SPACING;

	if (PointX < Offset)
		return -1;

	for (i = m_iStartChan; i < pDoc->GetAvailableChannels(); i++) {
		
		ChanSize = CHANNEL_WIDTH + (pDoc->GetEffColumns(i) * (CHAR_WIDTH * 3 + COLUMN_SPACING));
		Offset += ChanSize;
		if (PointX < Offset) {
			ColumnOffset = ChanSize - (Offset - PointX);
			Channel = i;
			break;
		}
		Offset += CHANNEL_SPACING * 2;
	}

	if (ColumnOffset < 0)
		return -1;

	if (Channel == -1)
		return -1;

	Offset = 0;

	for (i = 0; i < MaxColumns; i++) {
		Offset += COLUMN_SPACE[i];
		if (ColumnOffset < signed(Offset)) {
			Column = i;
			break;
		}
	}

	return Column;
}

void CFamiTrackerView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Channel = 0, Column = 0;
	int	DeltaY, DeltaRow;

	IgnoreFirst = true;

	Channel = GetChannelAtPoint(point.x);

	if (Channel == -1)
		return;

	DeltaY = point.y - (m_iWindowHeight / 2);

	if (DeltaY < 0)
		DeltaRow = DeltaY / signed(ROW_HEIGHT) - 1;
	else
		DeltaRow = DeltaY / signed(ROW_HEIGHT);

	Column	= GetColumnAtPoint(point.x, COLUMNS + pDoc->GetEffColumns(Channel) * 3);

	if (Column == -1)
		return;
	
	m_iSelectColStart = Column;
	m_iSelectColEnd = Column;

	// The top row (channel name and meters)
	if (point.y < signed(ROW_HEIGHT * 2) && Channel < signed(pDoc->GetAvailableChannels())) {
		// Silence channel
		if (Column < 5) {
			m_bMuteChannels[Channel] = !m_bMuteChannels[Channel];
			if (m_iSoloChannel != -1 && m_iSoloChannel == Channel)
				m_iSoloChannel = -2;
			if (m_bMuteChannels[Channel]) {
				stChanNote NoteData;
				NoteData.Note = HALT;
				FeedNote(Channel, &NoteData);
			}
			ForceRedraw();
			return;
		}
		// Remove one track effect column
		else if (Column == 5) {
			if (pDoc->GetEffColumns(Channel) > 0)
				pDoc->SetEffColumns(Channel, pDoc->GetEffColumns(Channel) - 1);
		}
		// Add one track effect column
		else if (Column == 6) {
			if (pDoc->GetEffColumns(Channel) < (MAX_EFFECT_COLUMNS - 1))
				pDoc->SetEffColumns(Channel, pDoc->GetEffColumns(Channel) + 1);
		}
		ForceRedraw();
	}

	int Sel = signed(m_iCurrentRow) + DeltaRow;

	if (m_iSelectStart != -1) {
		if (m_bShiftPressed) {
			m_iSelectEnd = Sel;
			ForceRedraw();
		}
		else {
			m_iSelectStart = -1;
			ForceRedraw();
		}
	}

	CView::OnLButtonDown(nFlags, point);
}

void CFamiTrackerView::OnLButtonUp(UINT nFlags, CPoint point)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Channel, Column;
	int DeltaY, DeltaRow;

	DeltaY = point.y - signed(m_iWindowHeight / 2) - (ROW_HEIGHT / 2) - 4;

	if (DeltaY < 0)
		DeltaRow = DeltaY / signed(ROW_HEIGHT) - 1;
	else
		DeltaRow = DeltaY / signed(ROW_HEIGHT);

	Channel = GetChannelAtPoint(point.x);

	if (Channel == -1) {
		if (((DeltaRow + signed(m_iCurrentRow)) >= 0 && (DeltaRow + signed(m_iCurrentRow)) < signed(GetCurrentPatternLength())) && point.y > ROW_HEIGHT * 2) {
			m_iCursorRow = m_iCurrentRow + DeltaRow;
			if (!theApp.m_pSettings->General.bFreeCursorEdit)
				m_iCurrentRow = m_iCursorRow;
			ForceRedraw();
		}
		return;
	}

	Column	= GetColumnAtPoint(point.x, COLUMNS + pDoc->GetEffColumns(Channel) * 3);

	if (Column == -1)
		return;

	if (m_iSelectStart == -1) {
		if (((DeltaRow + signed(m_iCurrentRow)) >= 0 && (DeltaRow + signed(m_iCurrentRow)) < signed(GetCurrentPatternLength())) && point.y > ROW_HEIGHT * 2) {
			m_iCursorRow = m_iCurrentRow + DeltaRow;
			if (!theApp.m_pSettings->General.bFreeCursorEdit)
				m_iCurrentRow = m_iCursorRow;

			if (Channel < signed(pDoc->GetAvailableChannels()) && point.y > ROW_HEIGHT * 2) {
				m_iCursorChannel = Channel;
				m_iCursorColumn	= Column;
			}

			ForceRedraw();
		}
	}

	CView::OnLButtonUp(nFlags, point);
}

void CFamiTrackerView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Channel, Column;

	IgnoreFirst = true;

	Channel = GetChannelAtPoint(point.x);
	
	if (Channel == -1)
		return;

	Column	= GetColumnAtPoint(point.x, COLUMNS + pDoc->GetEffColumns(Channel) * 3);

	if (Column == -1)
		return;

	if (point.y < ROW_HEIGHT * 2 && Channel < signed(pDoc->GetAvailableChannels())) {
		if (Column < 5) {
			if (m_iSoloChannel == -2) {
				for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++)
					m_bMuteChannels[i] = false;
				m_iSoloChannel = -1;
			}
			else {
				m_bMuteChannels[Channel] = false;
				for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
					if (i != Channel) {
						m_bMuteChannels[i] = true;
						MuteNote(i);
					}
				}
				m_iSoloChannel = Channel;
			}
			ForceRedraw();
		}
		// Remove one track effect column
		else if (Column == 5) {
			if (pDoc->GetEffColumns(Channel) > 0)
				pDoc->SetEffColumns(Channel, pDoc->GetEffColumns(Channel) - 1);
		}
		// Add one track effect column
		else if (Column == 6) {
			if (pDoc->GetEffColumns(Channel) < (MAX_EFFECT_COLUMNS - 1))
				pDoc->SetEffColumns(Channel, pDoc->GetEffColumns(Channel) + 1);
		}
		ForceRedraw();
	}
	else
		SelectWholePattern(Channel);

	ForceRedraw();

	CView::OnLButtonDblClk(nFlags, point);
}

void CFamiTrackerView::OnMouseMove(UINT nFlags, CPoint point)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
	CString String;
	int DeltaRow, DeltaY = point.y - (m_iWindowHeight / 2) - 10;
	int Channel, Column;

	static int LastX, LastY;

	if (!(nFlags & MK_LBUTTON))
		return;

	if (IgnoreFirst) {
		IgnoreFirst = false;
		LastX = point.x;
		LastY = point.y;
		return;
	}

	if (point.y < ROW_HEIGHT * 2)
		return;

	if ((LastX == point.x) && (LastY == point.y))
		return;

	if ((Channel = GetChannelAtPoint(point.x)) == -1)
		return;

	if ((Column = GetColumnAtPoint(point.x, COLUMNS + pDoc->GetEffColumns(Channel) * 3)) == -1)
		return;

	if (Channel == m_iSelectChannel)
		m_iSelectColEnd = Column;

	if (DeltaY < 0)
		DeltaRow = DeltaY / signed(ROW_HEIGHT) - 1;
	else
		DeltaRow = DeltaY / signed(ROW_HEIGHT);
	
	int Sel = m_iCurrentRow + DeltaRow;

	if (((DeltaRow + m_iCurrentRow) >= 0 && (DeltaRow + m_iCurrentRow) < signed(GetCurrentPatternLength())) && Channel >= 0) {
		
		if (m_iSelectStart != -1) {
			m_iSelectEnd = Sel;
			ForceRedraw();
		}
		else {
			m_iSelectStart = Sel;
			m_iSelectChannel = Channel;
		}
		
		if (DeltaRow >= signed((m_iWindowHeight / ROW_HEIGHT) / 2) - 2)
			MoveCursorDown();
		else if (DeltaRow <= (-signed((m_iWindowHeight / ROW_HEIGHT) / 2) + 2))
			MoveCursorUp();
	}

	if (m_iSelectStart != -1) {
		String.Format("Selection {%i; %i}", m_iSelectStart, m_iSelectEnd);
		SetMessageText(String);
	}

	CView::OnMouseMove(nFlags, point);
}

void CFamiTrackerView::OnEditCopy()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stClipData	*ClipData;
	stChanNote	Note;
	HGLOBAL		hMem;
	int			Ptr = 0;
	int			i;
	int			SelStart, SelEnd, Columns = 0;
	int			SelColStart, SelColEnd;

	if (m_iSelectStart == -1)
		return;

	SelStart	= GetMin(m_iSelectStart, m_iSelectEnd);
	SelEnd		= GetMax(m_iSelectStart, m_iSelectEnd);
	SelColStart	= GetMin(m_iSelectColStart, m_iSelectColEnd);
	SelColEnd	= GetMax(m_iSelectColStart, m_iSelectColEnd);

	if (!OpenClipboard()) {
		theApp.DisplayError(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	EmptyClipboard();

	hMem = GlobalAlloc(GMEM_MOVEABLE, sizeof(stClipData) * 2);

	if (hMem == NULL)
		return;

	ClipData				= (stClipData*)GlobalLock(hMem);
	ClipData->Size			= SelEnd - SelStart + 1;
	ClipData->NumCols		= 0;
	ClipData->FirstColumn	= false;

	if (SelColStart == 0)
		ClipData->FirstColumn = true;

	int ColPtr = 0;

	for (i = SelStart; i < SelEnd + 1; i++) {
		pDoc->GetNoteData(m_iCurrentFrame, m_iSelectChannel, i, &Note);

		if (SelColStart == 0) {
			ClipData->Notes[Ptr]	= Note.Note;
			ClipData->Octaves[Ptr]	= Note.Octave;
		}

		ColPtr = 0;
		for (int n = 1; n < COLUMNS + MAX_EFFECT_COLUMNS * 3; n++) {
			if (n >= SelColStart && n <= SelColEnd) {
				switch (n - 1) {
					case 0: ClipData->ColumnData[ColPtr][Ptr] = Note.Instrument >> 4;		break;		// Instrument
					case 1: ClipData->ColumnData[ColPtr][Ptr] = Note.Instrument & 0x0F;		break;
					case 2: ClipData->ColumnData[ColPtr][Ptr] = Note.Vol;					break;		// Volume
					case 3: ClipData->ColumnData[ColPtr][Ptr] = Note.EffNumber[0];			break;		// Effect column 1
					case 4: ClipData->ColumnData[ColPtr][Ptr] = Note.EffParam[0] >> 4;		break;
					case 5: ClipData->ColumnData[ColPtr][Ptr] = Note.EffParam[0] & 0x0F;	break;
					case 6: ClipData->ColumnData[ColPtr][Ptr] = Note.EffNumber[1];			break;		// Effect column 2
					case 7: ClipData->ColumnData[ColPtr][Ptr] = Note.EffParam[1] >> 4;		break;
					case 8: ClipData->ColumnData[ColPtr][Ptr] = Note.EffParam[1] & 0x0F;	break;
					case 9: ClipData->ColumnData[ColPtr][Ptr] = Note.EffNumber[2];			break;		// Effect column 3
					case 10: ClipData->ColumnData[ColPtr][Ptr] = Note.EffParam[2] >> 4;		break;
					case 11: ClipData->ColumnData[ColPtr][Ptr] = Note.EffParam[2] & 0x0F;	break;
					case 12: ClipData->ColumnData[ColPtr][Ptr] = Note.EffNumber[3];			break;		// Effect column 4
					case 13: ClipData->ColumnData[ColPtr][Ptr] = Note.EffParam[3] >> 4;		break;
					case 14: ClipData->ColumnData[ColPtr][Ptr] = Note.EffParam[3] & 0x0F;	break;
				}
				ColPtr++;
				ClipData->NumCols = ColPtr;
			}
		}
		
		Ptr++;
	}

	GlobalUnlock(hMem);
	SetClipboardData(m_iClipBoard, hMem);
	CloseClipboard();
}

void CFamiTrackerView::OnEditCut()
{
	if (m_iSelectStart == -1)
		return;

	AddUndo(m_iSelectChannel);
	OnEditCopy();
	OnEditDelete();

	ForceRedraw();
}

void CFamiTrackerView::OnEditPaste()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	HGLOBAL			hMem;
	stChanNote		ChanNote;
	stClipData		*ClipData;
	unsigned int	i;

	if (m_bEditEnable == false)
		return;

	OpenClipboard();

	if (!IsClipboardFormatAvailable(m_iClipBoard)) {
		theApp.DisplayError(IDS_CLIPBOARD_NOT_AVALIABLE);
	}

	hMem = GetClipboardData(m_iClipBoard);

	if (hMem == NULL)
		return;

	AddUndo(m_iCursorChannel);

	ClipData = (stClipData*)GlobalLock(hMem);

	for (i = 0; i < ClipData->Size; i++) {
		if ((m_iCursorRow + i) < GetCurrentPatternLength()) {

			if (!m_bPasteOverwrite && !m_bPasteMix)
				pDoc->InsertNote(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow + i);

			pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow + i, &ChanNote);

			if (ClipData->FirstColumn) {
				if (m_bPasteMix) {
					if (ChanNote.Note == 0) {
						ChanNote.Octave	= ClipData->Octaves[i];
						ChanNote.Note	= ClipData->Notes[i];
					}
				}
				else {
					ChanNote.Octave	= ClipData->Octaves[i];
					ChanNote.Note	= ClipData->Notes[i];
				}
			}

			unsigned int ColPtr = 0;

			for (int n = m_iCursorColumn; n < (COLUMNS + MAX_EFFECT_COLUMNS * 3); n++) {
				if (ColPtr < ClipData->NumCols && n > 0) {
					switch (n - 1) {
						case 0: 		// Instrument
							if (!m_bPasteMix || ChanNote.Instrument == 0x40)
								ChanNote.Instrument = (ChanNote.Instrument & 0x0F) | ClipData->ColumnData[ColPtr][i] << 4;
							if (ChanNote.Instrument != 64)
								ChanNote.Instrument &= 0x3F;
							break;
						case 1: 
							if (!m_bPasteMix || ChanNote.Instrument == 0x40)
								ChanNote.Instrument = (ChanNote.Instrument & 0xF0) | ClipData->ColumnData[ColPtr][i]; 
							if (ChanNote.Instrument != 64)
								ChanNote.Instrument &= 0x3F; 
							break;
						case 2:			// Volume
							if (!m_bPasteMix || ChanNote.Vol == 0x10)
								ChanNote.Vol = ClipData->ColumnData[ColPtr][i]; 
							if (ChanNote.Vol != 0x10)
								ChanNote.Vol &= 0x0F;
							break;
						case 3: ChanNote.EffNumber[0]	= ClipData->ColumnData[ColPtr][i]; break;											// First effect column
						case 4: ChanNote.EffParam[0]	= (ChanNote.EffParam[0] & 0x0F) | (ClipData->ColumnData[ColPtr][i] << 4);	break;
						case 5: ChanNote.EffParam[0]	= (ChanNote.EffParam[0] & 0xF0) | (ClipData->ColumnData[ColPtr][i] & 0x0F); break;
						case 6: ChanNote.EffNumber[1]	= ClipData->ColumnData[ColPtr][i]; break;											// Second effect column
						case 7: ChanNote.EffParam[1]	= (ChanNote.EffParam[1] & 0x0F) | (ClipData->ColumnData[ColPtr][i] << 4);	break;
						case 8: ChanNote.EffParam[1]	= (ChanNote.EffParam[1] & 0xF0) | (ClipData->ColumnData[ColPtr][i] & 0x0F); break;
						case 9: ChanNote.EffNumber[2]	= ClipData->ColumnData[ColPtr][i]; break;											// Third effect column
						case 10: ChanNote.EffParam[2]	= (ChanNote.EffParam[2] & 0x0F) | (ClipData->ColumnData[ColPtr][i] << 4);	break;
						case 11: ChanNote.EffParam[2]	= (ChanNote.EffParam[2] & 0xF0) | (ClipData->ColumnData[ColPtr][i] & 0x0F); break;
						case 12: ChanNote.EffNumber[3]	= ClipData->ColumnData[ColPtr][i]; break;											// Fourth effect column
						case 13: ChanNote.EffParam[3]	= (ChanNote.EffParam[3] & 0x0F) | (ClipData->ColumnData[ColPtr][i] << 4);	break;
						case 14: ChanNote.EffParam[3]	= (ChanNote.EffParam[3] & 0xF0) | (ClipData->ColumnData[ColPtr][i] & 0x0F); break;
					}
					ColPtr++;
				}
			}

			pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow + i, &ChanNote);
		}
	}

	GlobalUnlock(hMem);
	CloseClipboard();
	ForceRedraw();
}

void CFamiTrackerView::OnEditDelete()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int	i;
	int	SelStart, SelEnd;
	int SelColStart, SelColEnd;

	if (m_iSelectStart == -1 || m_bEditEnable == false)
		return;

	if (m_iSelectStart > m_iSelectEnd) {
		SelStart	= m_iSelectEnd;
		SelEnd		= m_iSelectStart;
	}
	else {
		SelStart	= m_iSelectStart;
		SelEnd		= m_iSelectEnd;
	}

	if (m_iSelectColStart > m_iSelectColEnd) {
		SelColStart = m_iSelectColEnd;
		SelColEnd	= m_iSelectColStart;
	}
	else {
		SelColStart = m_iSelectColStart;
		SelColEnd	= m_iSelectColEnd;
	}

	AddUndo(m_iSelectChannel);

	stChanNote NoteData;

	for (i = SelStart; i < SelEnd + 1; i++) {
		pDoc->GetNoteData(m_iCurrentFrame, m_iSelectChannel, i, &NoteData);
		if (IsColumnSelected(SelColStart, SelColEnd, 0)) {
			NoteData.Note = 0; NoteData.Octave = 0;
		}
		if (IsColumnSelected(SelColStart, SelColEnd, 1))
			NoteData.Instrument = MAX_INSTRUMENTS;
		if (IsColumnSelected(SelColStart, SelColEnd, 2))
			NoteData.Vol = 0x10;
		if (IsColumnSelected(SelColStart, SelColEnd, 3)) {
			NoteData.EffNumber[0]	= EF_NONE;
			NoteData.EffParam[0]	= 0;
		}
		if (IsColumnSelected(SelColStart, SelColEnd, 4)) {
			NoteData.EffNumber[1]	= EF_NONE;
			NoteData.EffParam[1]	= 0;
		}
		if (IsColumnSelected(SelColStart, SelColEnd, 5)) {
			NoteData.EffNumber[2]	= EF_NONE;
			NoteData.EffParam[2]	= 0;
		}
		if (IsColumnSelected(SelColStart, SelColEnd, 6)) {
			NoteData.EffNumber[3]	= EF_NONE;
			NoteData.EffParam[3]	= 0;
		}

		pDoc->SetNoteData(m_iCurrentFrame, m_iSelectChannel, i, &NoteData);
	}

	m_iSelectStart = -1;

	ForceRedraw();
}

void CFamiTrackerView::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_iSelectStart == -1 ? 0 : 1);
}

void CFamiTrackerView::OnUpdateEditCut(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_iSelectStart == -1 ? 0 : 1);
}

void CFamiTrackerView::OnUpdateEditPaste(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(IsClipboardFormatAvailable(m_iClipBoard) ? 1 : 0);
}

void CFamiTrackerView::OnUpdateEditDelete(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_iSelectStart == -1 ? 0 : 1);
}

void CFamiTrackerView::OnTrackerEdit()
{
	m_bEditEnable = !m_bEditEnable;

	if (m_bEditEnable)
		SetMessageText("Edit mode");
	else
		SetMessageText("Normal mode");
	
	ForceRedraw();
}

void CFamiTrackerView::OnUpdateTrackerEdit(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bEditEnable ? 1 : 0);
}

void CFamiTrackerView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
	switch (lHint) {
		case UPDATE_SONG_TRACK:		// changed song
			m_iCurrentFrame = 0;
			m_iCurrentRow = 0;
			m_iCursorChannel = 0;
			m_iCursorColumn = 0;
			m_iCursorRow = 0;
			m_bForceRedraw = true;
			break;
		case UPDATE_SONG_TRACKS:
			((CMainFrame*)GetParentFrame())->UpdateTrackBox();
			break;				 
		case UPDATE_CLEAR:
			((CMainFrame*)GetParentFrame())->ClearInstrumentList();
			((CMainFrame*)GetParentFrame())->CloseInstrumentSettings();
			break;
		default:
			// Document has changed, redraw the view
			m_bForceRedraw = true;
			break;
	}
}

void CFamiTrackerView::OnTrackerPal()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CMainFrame*	pMainFrm = static_cast<CMainFrame*>(GetParentFrame());

	int Speed = (pDoc->GetEngineSpeed() == 0) ? 50 : pDoc->GetEngineSpeed();

	pDoc->SetMachine(PAL);

	theApp.SetMachineType(pDoc->GetMachine(), Speed);
}

void CFamiTrackerView::OnTrackerNtsc()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CMainFrame*	pMainFrm = static_cast<CMainFrame*>(GetParentFrame());

	int Speed = (pDoc->GetEngineSpeed() == 0) ? 60 : pDoc->GetEngineSpeed();

	pDoc->SetMachine(NTSC);

	theApp.SetMachineType(pDoc->GetMachine(), Speed);
}

void CFamiTrackerView::OnUpdateTrackerPal(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
	pCmdUI->SetCheck(pDoc->GetMachine() == PAL);
}

void CFamiTrackerView::OnUpdateTrackerNtsc(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
	pCmdUI->SetCheck(pDoc->GetMachine() == NTSC);
}

void CFamiTrackerView::OnUpdateSpeedDefault(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
	pCmdUI->SetCheck(pDoc->GetEngineSpeed() == 0);
}

void CFamiTrackerView::OnUpdateSpeedCustom(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
	pCmdUI->SetCheck(pDoc->GetEngineSpeed() != 0);
}

void CFamiTrackerView::OnSpeedCustom()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CSpeedDlg SpeedDlg;
	int Speed;

	Speed = pDoc->GetEngineSpeed();

	if (Speed == 0)
		Speed = (pDoc->GetMachine() == NTSC ? 60 : 50);

	Speed = SpeedDlg.GetSpeedFromDlg(Speed);

	if (Speed == 0)
		return;

	pDoc->SetEngineSpeed(Speed);

	theApp.SetMachineType(pDoc->GetMachine(), Speed);
}

void CFamiTrackerView::OnSpeedDefault()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Speed = (pDoc->GetMachine() == NTSC) ? 60 : 50;

	pDoc->SetEngineSpeed(0);

	theApp.SetMachineType(pDoc->GetMachine(), Speed);
}

void CFamiTrackerView::OnEditPasteoverwrite()
{
	m_bPasteOverwrite = !m_bPasteOverwrite;
	
	if (m_bPasteOverwrite)
		m_bPasteMix = false;
}

void CFamiTrackerView::OnUpdateEditPasteoverwrite(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bPasteOverwrite);
}

void CFamiTrackerView::OnTransposeDecreasenote()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote Note;

	if (!m_bEditEnable)
		return;

	if (m_iSelectStart == -1) {
		AddUndo(m_iCursorChannel);
		pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
		if (Note.Note > 0 && Note.Note != HALT) {
			if (Note.Note > 1) 
				Note.Note--;
			else {
				if (Note.Octave > 0) {
					Note.Note = B;
					Note.Octave--;
				}
			}
		}
		pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
	}
	else {
		unsigned int SelStart = GetSelectStart();
		unsigned int SelEnd = GetSelectEnd();

		AddUndo(m_iSelectChannel);
		
		for (unsigned int i = SelStart; i < SelEnd + 1; i++) {
			pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);
			if (Note.Note > 0 && Note.Note != HALT) {
				if (Note.Note > 1)
					Note.Note--;
				else {
					if (Note.Octave > 0) {
						Note.Note = B;
						Note.Octave--;
					}
				}
			}
			pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);
		}
	}

//	ForceRedraw();
}

void CFamiTrackerView::OnTransposeDecreaseoctave()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote Note;

	if (!m_bEditEnable)
		return;

	if (m_iSelectStart == -1) {
		AddUndo(m_iCursorChannel);
		pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
		if (Note.Octave > 0) 
			Note.Octave--;
		pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
	}
	else {
		unsigned int SelStart = GetSelectStart();
		unsigned int SelEnd = GetSelectEnd();

		AddUndo(m_iSelectChannel);

		for (unsigned int i = SelStart; i < SelEnd + 1; i++) {
			pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);
			if (Note.Octave > 0)
				Note.Octave--;
			pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);
		}
	}
}

void CFamiTrackerView::OnTransposeIncreasenote()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote Note;

	if (!m_bEditEnable)
		return;

	if (m_iSelectStart == -1) {
		AddUndo(m_iCursorChannel);
		pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
		if (Note.Note > 0 && Note.Note != HALT) {
			if (Note.Note < B)
				Note.Note++;
			else {
				if (Note.Octave < 8) {
					Note.Note = C;
					Note.Octave++;
				}
			}
		}
		pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
	}
	else {
		unsigned int SelStart = GetSelectStart();
		unsigned int SelEnd = GetSelectEnd();

		AddUndo(m_iSelectChannel);

		for (unsigned int i = SelStart; i < SelEnd + 1; i++) {
			pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);
			if (Note.Note > 0 && Note.Note != HALT) {
				if (Note.Note < B)
					Note.Note++;
				else {
					if (Note.Octave < 8) {
						Note.Note = C;
						Note.Octave++;
					}
				}
			}
			pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);
		}
	}
}

void CFamiTrackerView::OnTransposeIncreaseoctave()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	stChanNote Note;

	if (!m_bEditEnable)
		return;

	if (m_iSelectStart == -1) {
		AddUndo(m_iCursorChannel);
		pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
		if (Note.Octave < 8) 
			Note.Octave++;
		pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
	}
	else {
		unsigned int SelStart = GetSelectStart();
		unsigned int SelEnd = GetSelectEnd();

		AddUndo(m_iSelectChannel);

		for (unsigned int i = SelStart; i < SelEnd + 1; i++) {
			pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);
			if (Note.Octave < 8)
				Note.Octave++;
			pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, i, &Note);
		}
	}
}

void CFamiTrackerView::OnEditUndo()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Channel, Pattern, x;
	CString Text;

	if (!m_iUndoLevel)
		return;

	if (!m_iRedoLevel) {
		int Channel = m_UndoStack[m_iUndoLevel - 1].Channel;
		int Pattern = pDoc->GetPatternAtFrame(m_iCurrentFrame, m_iCursorChannel);
		int x;

		m_UndoStack[m_iUndoLevel].Channel	= Channel;
		m_UndoStack[m_iUndoLevel].Pattern	= Pattern;
		m_UndoStack[m_iUndoLevel].Row		= m_iCursorRow;
		m_UndoStack[m_iUndoLevel].Column	= m_iCursorColumn;
		m_UndoStack[m_iUndoLevel].Frame		= m_iCurrentFrame;

		for (x = 0; x < MAX_PATTERN_LENGTH; x++) {
			pDoc->GetDataAtPattern(Pattern, Channel, x, &m_UndoStack[m_iUndoLevel].ChannelData[x]);
		}
	}

	m_iRedoLevel++;
	m_iUndoLevel--;

	Channel = m_UndoStack[m_iUndoLevel].Channel;
	Pattern = m_UndoStack[m_iUndoLevel].Pattern;

	for (x = 0; x < MAX_PATTERN_LENGTH; x++) {
		pDoc->SetDataAtPattern(Pattern, Channel, x, &m_UndoStack[m_iUndoLevel].ChannelData[x]);
	}

	m_iCursorChannel	= m_UndoStack[m_iUndoLevel].Channel;
	m_iCursorRow		= m_UndoStack[m_iUndoLevel].Row;
	m_iCursorColumn		= m_UndoStack[m_iUndoLevel].Column;
	m_iCurrentFrame		= m_UndoStack[m_iUndoLevel].Frame;

	pDoc->SetPatternAtFrame(m_iCurrentFrame, m_iCursorChannel, Pattern);

	m_iCurrentRow = m_iCursorRow;

	Text.Format("Undo (%i / %i)", m_iUndoLevel, m_iUndoLevel + m_iRedoLevel);
	SetMessageText(Text);

	ForceRedraw();
}

void CFamiTrackerView::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_iUndoLevel > 0 ? 1 : 0);
}

void CFamiTrackerView::OnEditRedo()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (!m_iRedoLevel)
		return;

	m_iUndoLevel++;
	m_iRedoLevel--;

	int Channel, Pattern, x;

	Channel = m_UndoStack[m_iUndoLevel].Channel;
	Pattern = m_UndoStack[m_iUndoLevel].Pattern;

	for (x = 0; x < MAX_PATTERN_LENGTH; x++) {
		pDoc->SetDataAtPattern(Pattern, Channel, x, &m_UndoStack[m_iUndoLevel].ChannelData[x]);
	}

	m_iCursorChannel	= Channel;
	m_iCursorRow		= m_UndoStack[m_iUndoLevel].Row;
	m_iCursorColumn		= m_UndoStack[m_iUndoLevel].Column;
	m_iCurrentFrame		= m_UndoStack[m_iUndoLevel].Frame;

	pDoc->SetPatternAtFrame(m_iCurrentFrame, m_iCursorChannel, Pattern);

	m_iCurrentRow = m_iCursorRow;

	CString Text;
	Text.Format("Redo (%i / %i)", m_iUndoLevel, m_iUndoLevel + m_iRedoLevel);
	SetMessageText(Text);

	ForceRedraw();
}

void CFamiTrackerView::OnUpdateEditRedo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_iRedoLevel > 0 ? 1 : 0);
}

void CFamiTrackerView::OnInitialUpdate()
{
	// Called when a new document is loaded
	//

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CMainFrame	*MainFrame = static_cast<CMainFrame*>(GetParentFrame());

	unsigned int i;
	char Text[256];

	CView::OnInitialUpdate();

	theApp.StopPlayer();

	m_iLastCursorColumn = 0;
	m_iLastRowState		= 0;
	m_bInitialized		= true;
	m_bForceRedraw		= true;
	m_iInstrument		= 0;
	m_iUndoLevel		= 0;
	m_iRedoLevel		= 0;

	m_iCurrentFrame		= 0;
	m_iCurrentRow		= 0;
	m_iCursorChannel	= 0;
	m_iCursorColumn		= 0;
	m_iCursorRow		= 0;

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (pDoc->IsInstrumentUsed(i)) {
			pDoc->GetInstrumentName(i, Text);
			MainFrame->AddInstrument(i, Text);
		}
	}

	if (pDoc->GetEngineSpeed() == 0) {
		if (pDoc->GetMachine() == NTSC)
			theApp.SetMachineType(pDoc->GetMachine(), 60);
		else
			theApp.SetMachineType(pDoc->GetMachine(), 50);
	}
	else
		theApp.SetMachineType(pDoc->GetMachine(), pDoc->GetEngineSpeed());

	MainFrame->SetSongInfo(pDoc->GetSongName(), pDoc->GetSongArtist(), pDoc->GetSongCopyright());
	MainFrame->UpdateTrackBox();
}

void CFamiTrackerView::RemoveWithoutDelete()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote	Note;

	int	i;
	int	SelStart, SelEnd;

	if (m_iSelectStart == -1 || m_bEditEnable == false)
		return;

	if (m_iSelectStart > m_iSelectEnd) {
		SelStart = m_iSelectEnd;
		SelEnd = m_iSelectStart;
	}
	else {
		SelStart = m_iSelectStart;
		SelEnd = m_iSelectEnd;
	}

	AddUndo(m_iSelectChannel);

	Note.Note = 0;
	Note.Octave = 0;
	Note.Instrument = 0;
	Note.EffNumber[0] = 0;
	Note.EffParam[0] = 0;

	for (i = SelStart; i < SelEnd + 1; i++) {
		pDoc->SetNoteData(m_iCurrentFrame, m_iSelectChannel, i, &Note);
	}

	m_iSelectStart = -1;

	ForceRedraw();
}

void CFamiTrackerView::OnEditSelectall()
{
	SelectWholePattern(m_iCursorChannel);
}

void CFamiTrackerView::SelectWholePattern(unsigned int Channel)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_iSelectStart == -1) {
		m_iSelectChannel	= Channel;
		m_iSelectStart		= 0;
		m_iSelectEnd		= GetCurrentPatternLength() - 1;
		m_iSelectColStart	= 0;
		m_iSelectColEnd		= COLUMNS - 1 + (pDoc->GetEffColumns(Channel) * 3);
	}
	else
		m_iSelectStart = -1;

	ForceRedraw();
}

void CFamiTrackerView::OnKillFocus(CWnd* pNewWnd)
{
	CView::OnKillFocus(pNewWnd);
	m_bHasFocus = false;
	Invalidate(TRUE);
	ForceRedraw();
}

void CFamiTrackerView::OnSetFocus(CWnd* pOldWnd)
{
	CView::OnSetFocus(pOldWnd);
	m_bHasFocus = true;
	Invalidate(TRUE);
	ForceRedraw();
}

void CFamiTrackerView::TranslateMidiMessage()
{
	unsigned char Message, Channel, Note, Octave, Velocity;
	static unsigned char LastNote, LastOctave;
	CString Status;

	CMIDI *pMIDI = theApp.GetMIDI();

	if (!m_bInitialized)
		return;

	while (pMIDI->ReadMessage(Message, Channel, Note, Octave, Velocity)) {
	
		if (!theApp.m_pSettings->Midi.bMidiChannelMap)
			Channel = m_iCursorChannel;

		switch (Message) {
			case MIDI_MSG_NOTE_ON:
				if (Channel > 4)
					return;

				Status.Format("MIDI message: Note on (note = %02i, octave = %02i, velocity = %02X)", Note, Octave, Velocity);
				SetMessageText(Status);

				if (Velocity == 0)
					ReleaseMIDINote(Channel, MIDI_NOTE(Octave, Note));
				else
					TriggerMIDINote(Channel, MIDI_NOTE(Octave, Note), Velocity, true);

				/*
				if (Velocity == 0) {

					ArpeggioNotes[(Note - 1) + Octave * 12] = 2;

					if (LastNote == Note && LastOctave == Octave) {
						// Cut note
						stChanNote Note;
						memset(&Note, 0, sizeof(stChanNote));
						Note.Note	= HALT;
						LastOctave	= 0;
						LastNote	= 0;
						FeedNote(Channel, &Note);
					}
				}
				else {
					if (!theApp.m_pSettings->Midi.bMidiVelocity)
						Velocity = 127;

					InsertNote(Note, Octave, Channel, Velocity + 1);

					ArpeggioNotes[(Note - 1) + Octave * 12] = 1;
					ArpeggioPtr = (Note - 1) + Octave * 12;
					LastArpPtr = ArpeggioPtr;

					LastNote	= Note;
					LastOctave	= Octave;

					KeyCount = 0;

					for (int i = 0; i < 128; i++) {
						if (ArpeggioNotes[i] == 1)
							KeyCount++;
					}
				}*/
				break;

			case MIDI_MSG_NOTE_OFF:
				if (Channel > 4)
					return;
				Status.Format("MIDI message: Note off");
				SetMessageText(Status);
				/*
				StopNote(Channel);
				ArpeggioNotes[(Note - 1) + Octave * 12] = 2;
				*/
				ReleaseMIDINote(Channel, MIDI_NOTE(Octave, Note));
				break;
			
			case 0x0F:
				if (Channel == 0x08) {
					StepDown();
				}
				break;
		}
	}
}

void CFamiTrackerView::PostNcDestroy()
{
	// Called at exit
	m_bInitialized = false;
	CView::PostNcDestroy();
}

void CFamiTrackerView::OnTimer(UINT nIDEvent)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CMainFrame *pMainFrm;
	static int LastNoteState;

	int TicksPerSec = pDoc->GetFrameRate();

	static_cast<CMainFrame*>(GetParentFrame())->SetIndicatorTime(m_iPlayTime / 600, (m_iPlayTime / 10) % 60, m_iPlayTime % 10);

	if (nIDEvent == 0) {

		pMainFrm = (CMainFrame*)GetParentFrame();

		// Fetch channel levels

		m_iVolLevels[0] = theApp.GetOutput(0);
		m_iVolLevels[1] = theApp.GetOutput(1);
		m_iVolLevels[2] = theApp.GetOutput(2);
		m_iVolLevels[3] = theApp.GetOutput(3);
		m_iVolLevels[4] = theApp.GetOutput(4) / 8;

		// Redraw screen
//		if (m_iCurrentRow != m_iLastRowState || m_iCurrentFrame != m_iLastFrameState || m_bForceRedraw) {

		if (m_bForceRedraw) {

//			if (m_iCurrentRow != m_iLastRowState || m_iCurrentFrame != m_iLastFrameState || m_iCursorColumn != m_iLastCursorColumn)
//				RefreshStatusMessage();

			m_iLastRowState		= m_iCurrentRow;
			m_iLastFrameState	= m_iCurrentFrame;
			m_iLastCursorColumn	= m_iCursorColumn;
			m_bForceRedraw		= false;

			Invalidate(FALSE);
			RedrawWindow();
			//PostMessage(WM_PAINT);

			((CMainFrame*)GetParent())->RefreshPattern();
		}
		else {
			if (pDoc->IsFileLoaded()) {
				CDC *pDC = this->GetDC();
				DrawMeters(pDC);
				DrawChannelHeaders(pDC);
				ReleaseDC(pDC);
			}
		}

		if (LastNoteState != m_iKeyboardNote)
			pMainFrm->ChangeNoteState(m_iKeyboardNote);

//		if (m_bPlaying) {
			//if ((m_iPlayTime & 0x01) == 0x01)
//			pMainFrm->SetIndicatorTime((m_iPlayTime / TicksPerSec) / 60, (m_iPlayTime / TicksPerSec) % 60, ((m_iPlayTime * 10) / TicksPerSec) % 10);
//		}

		LastNoteState = m_iKeyboardNote;

//		if (iStillAlive == 0) {
//			pMainFrm->SetMessageText("Sound thread is not responding, please save your work and restart the tracker!");
//		}
//		else
//			iStillAlive--;
	}

	CView::OnTimer(nIDEvent);
}

int CFamiTrackerView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Install a timer for screen updates
	SetTimer(0, 20, NULL);

	return 0;
}

void CFamiTrackerView::OnEditEnablemidi()
{
	theApp.GetMIDI()->Toggle();
}

void CFamiTrackerView::OnUpdateEditEnablemidi(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(theApp.GetMIDI()->IsAvailable());
	pCmdUI->SetCheck(theApp.GetMIDI()->IsOpened());
}

void CFamiTrackerView::OnFrameInsert()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int i, x;

	if (pDoc->GetFrameCount() == MAX_FRAMES) {
		return;
	}

	pDoc->SetFrameCount(pDoc->GetFrameCount() + 1);

	for (x = pDoc->GetFrameCount(); x > unsigned(m_iCurrentFrame - 1); x--) {
		for (i = 0; i < pDoc->GetAvailableChannels(); i++) {
			pDoc->SetPatternAtFrame(x + 1, i, pDoc->GetPatternAtFrame(x, i));
		}
	}

	for (i = 0; i < pDoc->GetAvailableChannels(); i++) {
		pDoc->SetPatternAtFrame(m_iCurrentFrame, i, 0);
	}
}

void CFamiTrackerView::OnFrameRemove()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int i, x;

	if (pDoc->GetFrameCount() == 1) {
		return;
	}

	for (i = 0; i < pDoc->GetAvailableChannels(); i++) {
		pDoc->SetPatternAtFrame(m_iCurrentFrame, i, 0);
	}

	for (x = m_iCurrentFrame; x < pDoc->GetFrameCount(); x++) {
		for (i = 0; i < pDoc->GetAvailableChannels(); i++) {
			pDoc->SetPatternAtFrame(x, i, pDoc->GetPatternAtFrame(x + 1, i));
		}
	}

	pDoc->SetFrameCount(pDoc->GetFrameCount() - 1);

	if (m_iCurrentFrame > (pDoc->GetFrameCount() - 1))
		m_iCurrentFrame = (pDoc->GetFrameCount() - 1);
}

void CFamiTrackerView::RegisterKeyState(int Channel, int Note)
{
	if (Channel == m_iCursorChannel)
		m_iKeyboardNote	= Note;
}

void CFamiTrackerView::OnTrackerPlayrow()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	stChanNote Note;

	ASSERT_VALID(pDoc);

	for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
		pDoc->GetNoteData(m_iCurrentFrame, i, m_iCursorRow, &Note);
		if (!m_bMuteChannels[i])
			FeedNote(i, &Note);
	
	}

	MoveCursorDown();
}

void CFamiTrackerView::OnUpdateFrameRemove(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	pCmdUI->Enable(pDoc->GetFrameCount() > 1);
}

void CFamiTrackerView::RefreshStatusMessage()
{

	return;

	// This is not good....

	CFamiTrackerDoc* pDoc = GetDocument();
	int Effect, Param;
	stChanNote Note;
	CString Text;
	bool IsEffect = false;

	if (m_iSelectStart != -1 || m_bPlaying)
		return;

	pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);

	switch (m_iCursorColumn) {
		case 0:
			Text = "Note column";
			break;
		case 1:
		case 2:
			if (Note.Instrument == 0x40) {
				Text = "Instrument column";
			}
			else if (pDoc->IsInstrumentUsed(Note.Instrument)) {
				char InstName[256];
				pDoc->GetInstrumentName(Note.Instrument, InstName);
				Text.Format("Instrument column: %02X - %s", Note.Instrument, InstName);
			}
			else
				Text = "Instrument column (instrument doesn't exist)";

			break;
		case 3:
			Text = "Volume column";
			break;
		case 4:		// First
		case 5:
		case 6:
			Effect		= Note.EffNumber[0];
			Param		= Note.EffParam[0];
			IsEffect	= true;
			break;
		case 7:		// Second
		case 8:
		case 9:
			Effect		= Note.EffNumber[1];
			Param		= Note.EffParam[1];
			IsEffect	= true;
			break;
		case 10:	// Third
		case 11:
		case 12:
			Effect		= Note.EffNumber[2];
			Param		= Note.EffParam[2];
			IsEffect	= true;
			break;
		case 13:	// Fourth
		case 14:
		case 15:
			Effect		= Note.EffNumber[3];
			Param		= Note.EffParam[3];
			IsEffect	= true;
			break;
	}

	if (IsEffect) {
		switch (Effect) {
			case EF_ARPEGGIO:	Text.Format("Effect: Arpeggio, %02X (%i)", Param, Param); break;
			case EF_VIBRATO:	Text.Format("Effect: Vibrato, %02X (%i)", Param, Param); break;
			case EF_TREMOLO:	Text.Format("Effect: Tremolo, %02X (%i)", Param, Param); break;
			case EF_SPEED:		Text.Format("Effect: Speed, %02X (%i)", Param, Param); break;
			case EF_JUMP:		Text.Format("Effect: Jump, %02X (%i)", Param, Param); break;
			case EF_SKIP:		Text.Format("Effect: Skip, %02X (%i)", Param, Param); break;
			case EF_HALT:		Text.Format("Effect: Halt"); break;
			case EF_VOLUME:		Text.Format("Effect: Volume, %02X (%i)", Param, Param); break;
			case EF_PORTAON:	Text.Format("Effect: Portamento on, %02X (%i)", Param, Param); break;
			case EF_PORTAOFF:	Text.Format("Effect: Portamento off"); break;
			case EF_SWEEPUP:	Text.Format("Effect: Hardware sweep up, %02X (%i)", Param, Param); break;
			case EF_SWEEPDOWN:	Text.Format("Effect: Hardware sweep down, %02X (%i)", Param, Param); break;
			case EF_PITCH:		Text.Format("Effect: Fine pitch setting"); break;
			default:
				Text = "Effect column";
		}
	}

	SetMessageText(Text);
}

void CFamiTrackerView::FeedNote(int Channel, stChanNote *NoteData)
{
	CFamiTrackerDoc* pDoc = GetDocument();

	if (NewNoteData[Channel])
		return;

	CurrentNotes[Channel] = *NoteData;
	NewNoteData[Channel] = true;
	
	theApp.GetMIDI()->WriteNote(Channel, NoteData->Note, NoteData->Octave, NoteData->Vol);
}

void CFamiTrackerView::GetStartSpeed()
{
	CFamiTrackerDoc* pDoc = GetDocument();

	m_iTempoAccum	= 60 * pDoc->GetFrameRate();		// 60 as in 60 seconds per minute
	m_iTickPeriod	= pDoc->GetSongSpeed();
	m_iSpeed		= DEFAULT_SPEED;
	
	m_iTempo = m_iTempoAccum / 24;

	if (m_iTickPeriod > 20)
		m_iTempo = m_iTickPeriod;
	else
		m_iSpeed = m_iTickPeriod;
}

void CFamiTrackerView::OnRButtonUp(UINT nFlags, CPoint point)
{
	CRect WinRect;
	CMenu *PopupMenu, PopupMenuBar;
	int Item = 0;

	GetWindowRect(WinRect);
	PopupMenuBar.LoadMenu(IDR_POPUP);

	PopupMenu = PopupMenuBar.GetSubMenu(0);
	PopupMenu->TrackPopupMenu(TPM_RIGHTBUTTON, point.x + WinRect.left, point.y + WinRect.top, this);

	CView::OnRButtonUp(nFlags, point);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::SelectFrame(unsigned int Frame)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Frame < GetDocument()->GetFrameCount());

	m_iCurrentFrame = Frame;
	ForceRedraw();
}

void CFamiTrackerView::SelectNextFrame()
{
	/*
	if (m_iSelectStart != -1 && theApp.m_pSettings->General.bKeySelect) {
		// quick hack (as everything else)
		if (m_iSelectColEnd < (m_iChannelColumns[m_iCursorChannel] - 1))
			m_iSelectColEnd++;
	}
	else {
	*/
		if (m_iCurrentFrame < (GetDocument()->GetFrameCount() - 1))
			m_iCurrentFrame++;
		else
			m_iCurrentFrame = 0;
	//}

	ForceRedraw();
}

void CFamiTrackerView::SelectPrevFrame()
{
	/*
	if (m_iSelectStart != -1 && theApp.m_pSettings->General.bKeySelect) {
		if (m_iSelectColEnd > 0)
			m_iSelectColEnd--;
	}
	else {
	*/
		if (m_iCurrentFrame > 0)
			m_iCurrentFrame--;
		else
			m_iCurrentFrame = GetDocument()->GetFrameCount() - 1;
//	}

	ForceRedraw();
}

void CFamiTrackerView::SelectFirstFrame()
{
	m_iCurrentFrame = 0;
	ForceRedraw();
}

void CFamiTrackerView::SelectLastFrame()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_iCurrentFrame = pDoc->GetFrameCount() - 1;
	ForceRedraw();
}

void CFamiTrackerView::SelectChannel(unsigned int Channel)
{
	ASSERT(Channel < MAX_CHANNELS);
	m_iCursorChannel = Channel;
	m_iCursorColumn = 0;
	ForceRedraw();
}

void CFamiTrackerView::MoveCursorLeft()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int MaxColumns = GetCurrentColumnCount();

	if (m_iCursorColumn > 0) {
		m_iCursorColumn--;
	}
	else {
		if (m_iCursorChannel > 0) {
			m_iCursorColumn = MaxColumns - 1;
			m_iCursorChannel--;
		}
		else {
			if (theApp.m_pSettings->General.bWrapCursor) {
				m_iCursorChannel = pDoc->GetAvailableChannels() - 1;
				m_iCursorColumn = GetCurrentColumnCount() - 1;
			}
		}
	}

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorRight()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int MaxColumns = GetCurrentColumnCount();

	if (m_iCursorColumn < (MaxColumns - 1)) {
		m_iCursorColumn++;
	}
	else {
		if (m_iCursorChannel < (pDoc->GetAvailableChannels() - 1)) {
			m_iCursorColumn = 0;
			m_iCursorChannel++;
		}
		else {
			if (theApp.m_pSettings->General.bWrapCursor) {
				m_iCursorColumn	= 0;
				m_iCursorChannel = 0;
			}
		}
	}

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorNextChannel()
{
	m_iCursorColumn = 0;

	if (m_iCursorChannel < GetDocument()->GetAvailableChannels() - 1)
		m_iCursorChannel++;
	else
		m_iCursorChannel = 0;

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorPrevChannel()
{
	m_iCursorColumn = 0;

	if (m_iCursorChannel > 0)
		m_iCursorChannel--;
	else
		m_iCursorChannel = GetDocument()->GetAvailableChannels() - 1;
	
	ForceRedraw();
}

void CFamiTrackerView::MoveCursorFirstChannel()
{
	m_iCursorChannel = 0;
	m_iCursorColumn = 0;

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorLastChannel()
{
	m_iCursorChannel = GetDocument()->GetAvailableChannels() - 1;
	m_iCursorColumn	= 0;

	ForceRedraw();
}

void CFamiTrackerView::MuteNote(int Channel)
{
	stChanNote NoteData;
	memset(&NoteData, 0, sizeof(stChanNote));
	NoteData.Note = HALT;
	FeedNote(Channel, &NoteData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Revised things
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::SetInstrument(int Instrument)
{
	m_iInstrument = Instrument;
}

void CFamiTrackerView::MoveCursorUp()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_iCursorRow > 0)
		m_iCursorRow--;
	else if (theApp.m_pSettings->General.bWrapCursor)
		m_iCursorRow = GetCurrentPatternLength() - 1;

	//m_iCursorRow = m_iCurrentRow;

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorDown()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_iCursorRow < (GetCurrentPatternLength() - 1))
		m_iCursorRow++; 
	else if (theApp.m_pSettings->General.bWrapCursor)
		m_iCursorRow = 0;

	//m_iCursorRow = m_iCurrentRow;

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorPageUp()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_iCursorRow > (PAGE_SIZE - 1))
		m_iCursorRow -= PAGE_SIZE;
	else if (theApp.m_pSettings->General.bWrapCursor)
		m_iCursorRow += GetCurrentPatternLength() - PAGE_SIZE;
	else
		m_iCursorRow = 0;

//	m_iCursorRow = m_iCurrentRow;

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorPageDown()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_iCursorRow < GetCurrentPatternLength() - PAGE_SIZE)
		m_iCursorRow += PAGE_SIZE; 
	else if (theApp.m_pSettings->General.bWrapCursor)
		m_iCursorRow -= GetCurrentPatternLength() - PAGE_SIZE;
	else 
		m_iCursorRow = GetCurrentPatternLength() - 1;

//	m_iCursorRow = m_iCurrentRow;

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorToTop()
{
	m_iCursorRow = 0;
//	m_iCursorRow = m_iCurrentRow;
	ForceRedraw();
}

void CFamiTrackerView::MoveCursorToBottom()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_iCursorRow = GetCurrentPatternLength() - 1;
//	m_iCursorRow = m_iCurrentRow;

	ForceRedraw();
}

void CFamiTrackerView::IncreaseCurrentPattern()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_bChangeAllPattern) {
		for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
			pDoc->IncreasePattern(m_iCurrentFrame, i);
		}
	}
	else
		pDoc->IncreasePattern(m_iCurrentFrame, m_iCursorChannel);
}

void CFamiTrackerView::DecreaseCurrentPattern()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_bChangeAllPattern) {
		for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
			pDoc->DecreasePattern(m_iCurrentFrame, i);
		}
	}
	else
		pDoc->DecreasePattern(m_iCurrentFrame, m_iCursorChannel);
}

void CFamiTrackerView::SetSongSpeed(unsigned int Speed)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	ASSERT(Speed <= MAX_TEMPO);

	m_iTickPeriod = Speed;
	
	if (m_iTickPeriod > 20)
		m_iTempo = m_iTickPeriod;
	else
		m_iSpeed = m_iTickPeriod;
		
	pDoc->SetSongSpeed(Speed);
}

void CFamiTrackerView::StepDown()
{
	unsigned int i = 0;

	while (i++ < m_iKeyStepping)
		MoveCursorDown();
}

void CFamiTrackerView::AddUndo(unsigned int Channel)
{
	// Channel - Channel that contains the pattern that should be added ontop of undo stack
	//

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int Pattern = pDoc->GetPatternAtFrame(m_iCurrentFrame, Channel);
	int x;

	stUndoBlock UndoBlock;
	
	UndoBlock.Channel	= Channel;
	UndoBlock.Pattern	= Pattern;
	UndoBlock.Row		= m_iCursorRow;
	UndoBlock.Column	= m_iCursorColumn;
	UndoBlock.Frame		= m_iCurrentFrame;

	for (x = 0; x < MAX_PATTERN_LENGTH; x++) {
		pDoc->GetDataAtPattern(Pattern, Channel, x, &UndoBlock.ChannelData[x]);
	}

	if (m_iUndoLevel < UNDO_LEVELS)
		m_iUndoLevel++;
	else {
		if (m_iUndoLevel == (UNDO_LEVELS - 0)) {
			for (x = 1; x < (UNDO_LEVELS+1); x++) {
				m_UndoStack[x - 1] = m_UndoStack[x];
			}
		}
	}

	m_UndoStack[m_iUndoLevel - 1] = UndoBlock;

	m_iRedoLevel = 0;
}

void CFamiTrackerView::HandleKeyboardInput(int Char)
{
	InterpretKey(Char);
}

void CFamiTrackerView::InterpretKey(int Key)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	static bool FirstChar = false;

	int EditedColumn = m_iCursorColumn;
	int EditStyle = theApp.m_pSettings->General.iEditStyle;

	stChanNote	Note;

	pDoc->GetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);

	if (PreventRepeat(Key))
		return;

	int Value, Index;

	switch (EditedColumn) {
		case C_EFF_NUM:		EditedColumn = C_EFF_NUM;	Index = 0; break;
		case C_EFF2_NUM:	EditedColumn = C_EFF_NUM;	Index = 1; break;
		case C_EFF3_NUM:	EditedColumn = C_EFF_NUM;	Index = 2; break;
		case C_EFF4_NUM:	EditedColumn = C_EFF_NUM;	Index = 3; break;
		case C_EFF_PARAM1:	EditedColumn = C_EFF_PARAM1; Index = 0; break;
		case C_EFF2_PARAM1:	EditedColumn = C_EFF_PARAM1; Index = 1; break;
		case C_EFF3_PARAM1:	EditedColumn = C_EFF_PARAM1; Index = 2; break;
		case C_EFF4_PARAM1:	EditedColumn = C_EFF_PARAM1; Index = 3; break;
		case C_EFF_PARAM2:	EditedColumn = C_EFF_PARAM2; Index = 0; break;
		case C_EFF2_PARAM2:	EditedColumn = C_EFF_PARAM2; Index = 1; break;
		case C_EFF3_PARAM2:	EditedColumn = C_EFF_PARAM2; Index = 2; break;
		case C_EFF4_PARAM2:	EditedColumn = C_EFF_PARAM2; Index = 3; break;			
	}

	switch (EditedColumn) {
		case C_NOTE:
			Note = TranslateKey(Key);
			if (Note.Note > 0)
				InsertNote(Note.Note, Note.Octave, m_iCursorChannel, 0x80);
			return;
		case C_INSTRUMENT1:
			Value = ConvertKeyToHex(Key);
			if (m_bEditEnable && Value != -1) {
				if (Note.Instrument == MAX_INSTRUMENTS)
					Note.Instrument = 0;
				if (EditStyle == EDIT_STYLE1) {
					Note.Instrument = (Note.Instrument & 0x0F) | (Value << 4);
				}
				else if (EditStyle == EDIT_STYLE2) {
					if (Note.Instrument == (MAX_INSTRUMENTS - 1))
						Note.Instrument = 0;
					Note.Instrument = ((Note.Instrument & 0x0F) << 4) | Value & 0x0F;
				}
				if (Note.Instrument > (MAX_INSTRUMENTS - 1))
					Note.Instrument = (MAX_INSTRUMENTS - 1);
			}
			else
				return;
			break;
		case C_INSTRUMENT2:
			Value = ConvertKeyToHex(Key);
			if (m_bEditEnable && Value != -1) {
				if (Note.Instrument == MAX_INSTRUMENTS)
					Note.Instrument = 0;
				if (EditStyle == EDIT_STYLE1) {
					Note.Instrument = (Note.Instrument & 0xF0) | Value;
				}
				else if (EditStyle == EDIT_STYLE2) {
					if (Note.Instrument == (MAX_INSTRUMENTS - 1))
						Note.Instrument = 0;
					Note.Instrument = ((Note.Instrument & 0x0F) << 4) | Value & 0x0F;
				}
				if (Note.Instrument > (MAX_INSTRUMENTS - 1))
					Note.Instrument = (MAX_INSTRUMENTS - 1);
			}
			else
				return;
			break;
		case C_VOLUME:
			Value = ConvertKeyToHex(Key);
			if (Value != -1 && m_bEditEnable) {
				Note.Vol = Value;
			}
			else
				return;
			break;
		case C_EFF_NUM:
			if (m_bEditEnable) {
				bool Found = false;
				for (int i = 0; i < EF_COUNT; i++) {
					if (Key == EFF_CHAR[i]) {
						Note.EffNumber[Index] = i + 1;
						Found = true;
						break;
					}
				}
				if (!Found)
					return;
			}
			break;
		case C_EFF_PARAM1:
			Value = ConvertKeyToHex(Key);
			if (m_bEditEnable && Value != -1) {
				if (EditStyle == EDIT_STYLE1) {
					Note.EffParam[Index] = (Note.EffParam[Index] & 0x0F) | Value << 4;
				}
				else if (EditStyle == EDIT_STYLE2) {
					Note.EffParam[Index] = ((Note.EffParam[Index] & 0x0F) << 4) | Value & 0x0F;
				}
			}
			else
				return;
			break;
		case C_EFF_PARAM2:
			Value = ConvertKeyToHex(Key);
			if (m_bEditEnable && Value != -1) {
				if (EditStyle == EDIT_STYLE1) {
					Note.EffParam[Index] = (Note.EffParam[Index] & 0xF0) | Value;
				}
				else if (EditStyle == EDIT_STYLE2) {
					Note.EffParam[Index] = ((Note.EffParam[Index] & 0x0F) << 4) | Value & 0x0F;
				}
			}
			else
				return;
			break;
	}

	if (m_bEditEnable) {
		AddUndo(m_iCursorChannel);
		pDoc->SetNoteData(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow, &Note);
		
		if (theApp.m_pSettings->General.iEditStyle == EDIT_STYLE1)
			StepDown();

		ForceRedraw();
	}
}

bool CFamiTrackerView::PreventRepeat(int Key)
{
	if (m_cKeyList[Key] == 0)
		m_cKeyList[Key] = 1;
	else {
		if ((!theApp.m_pSettings->General.bKeyRepeat || !m_bEditEnable))
			return true;
	}

	return false;
}

void CFamiTrackerView::KeyReleased(int Key)
{
	// When user released a key

	stChanNote Note;
	int i;
	
	if (m_cKeyList[Key] != 0) {

		for (i = 0; i < 256; i++) {
			if (m_cKeyList[i] != 0 && i != Key) {
				m_cKeyList[Key] = 0;
				return;
			}
		}

		m_cKeyList[Key] = 0;

		Note.Instrument		= 0;
		Note.Octave			= 0;
		Note.Note			= RELEASE;
		Note.EffNumber[0]	= 0;
		Note.EffParam[0]	= 0;
		
		FeedNote(m_iCursorChannel, &Note);
	}
}

void CFamiTrackerView::StopNote(int Channel)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote Note;
	
	memset(&Note, 0, sizeof(stChanNote));

	Note.Note = HALT;

	if (m_bEditEnable) {
		AddUndo(Channel);
		pDoc->SetNoteData(m_iCurrentFrame, Channel, m_iCursorRow, &Note);
		pDoc->SetModifiedFlag();
		ForceRedraw();
	}

	FeedNote(Channel, &Note);
}

void CFamiTrackerView::InsertNote(int Note, int Octave, int Channel, int Velocity)
{
	stChanNote Cell;

	if (m_bMuteChannels[Channel]) {
		memset(&Cell, 0, sizeof(stChanNote));
		Cell.Vol = 0x0F;
	}
	else
		GetDocument()->GetNoteData(m_iCurrentFrame, Channel, m_iCursorRow, &Cell);

	Cell.Note		= Note;
	Cell.Octave		= Octave;
	Cell.Instrument = m_iInstrument;
	
	if (Velocity < 128)
		Cell.Vol = (Velocity / 8);

	if (m_bEditEnable) {
		AddUndo(Channel);
		GetDocument()->SetNoteData(m_iCurrentFrame, Channel, m_iCursorRow, &Cell);
		if (m_iCursorColumn == 0 && m_bPlaying == false)
			StepDown();
	}

	FeedNote(Channel, &Cell);
}

void CFamiTrackerView::PlayNote(int Key)
{
	// Quick hack, fix this sometime

	if (PreventRepeat(Key))
		return;

	switch (Key) {
		case VK_F2: m_iOctave = 0; break;
		case VK_F3: m_iOctave = 1; break;
		case VK_F4: m_iOctave = 2; break;
		case VK_F5: m_iOctave = 3; break;
		case VK_F6: m_iOctave = 4; break;
		case VK_F7: m_iOctave = 5; break;
		case VK_F8: m_iOctave = 6; break;
		case VK_F9: m_iOctave = 7; break;
	}

	stChanNote Cell = TranslateKey(Key);

	Cell.Instrument = m_iInstrument;
	Cell.Vol		= 15;

	m_cKeyList[Key] = 1;

	FeedNote(m_iCursorChannel, &Cell);
}

// Translates a keyboard character into a note
stChanNote CFamiTrackerView::TranslateKey(int Key)
{
	int	KeyNote = 0, KeyOctave = 0;
	stChanNote Note;
	
	memset(&Note, 0, sizeof(stChanNote));

	// Convert key to a note
	switch (Key) {
		case 50:	KeyNote = Cb;	KeyOctave = m_iOctave + 1;	break;	// 2
		case 51:	KeyNote = Db;	KeyOctave = m_iOctave + 1;	break;	// 3
		case 53:	KeyNote = Fb;	KeyOctave = m_iOctave + 1;	break;	// 5
		case 54:	KeyNote = Gb;	KeyOctave = m_iOctave + 1;	break;	// 6
		case 55:	KeyNote = Ab;	KeyOctave = m_iOctave + 1;	break;	// 7
		case 57:	KeyNote = Cb;	KeyOctave = m_iOctave + 2;	break;	// 9
		case 48:	KeyNote = Db;	KeyOctave = m_iOctave + 2;	break;	// 0
		case 81:	KeyNote = C;	KeyOctave = m_iOctave + 1;	break;	// Q
		case 87:	KeyNote = D;	KeyOctave = m_iOctave + 1;	break;	// W
		case 69:	KeyNote = E;	KeyOctave = m_iOctave + 1;	break;	// E
		case 82:	KeyNote = F;	KeyOctave = m_iOctave + 1;	break;	// R
		case 84:	KeyNote = G;	KeyOctave = m_iOctave + 1;	break;	// T
		case 89:	KeyNote = A;	KeyOctave = m_iOctave + 1;	break;	// Y
		case 85:	KeyNote = B;	KeyOctave = m_iOctave + 1;	break;	// U
		case 73:	KeyNote = C;	KeyOctave = m_iOctave + 2;	break;	// I
		case 79:	KeyNote = D;	KeyOctave = m_iOctave + 2;	break;	// O
		case 80:	KeyNote = E;	KeyOctave = m_iOctave + 2;	break;	// P
		case 221:	KeyNote = F;	KeyOctave = m_iOctave + 2;	break;	// Å
		case 219:	KeyNote = Fb;	KeyOctave = m_iOctave + 2;	break;	// ´
		//case 186:	KeyNote = G;	KeyOctave = m_iOctave + 2;	break;	// ¨
		case 83:	KeyNote = Cb;	KeyOctave = m_iOctave;		break;	// S
		case 68:	KeyNote = Db;	KeyOctave = m_iOctave;		break;	// D
		case 71:	KeyNote = Fb;	KeyOctave = m_iOctave;		break;	// G
		case 72:	KeyNote = Gb;	KeyOctave = m_iOctave;		break;	// H
		case 74:	KeyNote = Ab;	KeyOctave = m_iOctave;		break;	// J
		case 76:	KeyNote = Cb;	KeyOctave = m_iOctave + 1;	break;	// L
		case 192:	KeyNote = Db;	KeyOctave = m_iOctave + 1;	break;	// Ö
		case 226:	KeyNote = HALT;	KeyOctave = 0;				break;	// < /stop
		case 186:	KeyNote = HALT;	KeyOctave = 0;				break;	// ¨ /stop (for american keyboards)
		case 49:	KeyNote = HALT;	KeyOctave = 0;				break;	// 1 /stop (again)
		case 90:	KeyNote = C;	KeyOctave = m_iOctave;		break;	// Z
		case 88:	KeyNote = D;	KeyOctave = m_iOctave;		break;	// X
		case 67:	KeyNote = E;	KeyOctave = m_iOctave;		break;	// C
		case 86:	KeyNote = F;	KeyOctave = m_iOctave;		break;	// V
		case 66:	KeyNote = G;	KeyOctave = m_iOctave;		break;	// B
		case 78:	KeyNote = A;	KeyOctave = m_iOctave;		break;	// N
		case 77:	KeyNote = B;	KeyOctave = m_iOctave;		break;	// M
		case 188:	KeyNote = C;	KeyOctave = m_iOctave + 1;	break;	// ,
		case 190:	KeyNote = D;	KeyOctave = m_iOctave + 1;	break;	// .
		case 189:	KeyNote = E;	KeyOctave = m_iOctave + 1;	break;	// -
	}

	Note.Note = KeyNote;
	Note.Octave = KeyOctave;

	return Note;
}

void CFamiTrackerView::OnEditPastemix()
{
	m_bPasteMix = !m_bPasteMix;
	
	if (m_bPasteMix)
		m_bPasteOverwrite = false;
}

void CFamiTrackerView::OnUpdateEditPastemix(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bPasteMix);
}

void CFamiTrackerView::ScanActualLengths()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote Note;
	
	unsigned int FrameCount		= pDoc->GetFrameCount();
	unsigned int Channels		= pDoc->GetAvailableChannels();
	unsigned int PatternLength	= pDoc->GetPatternLength();

	for (unsigned int i = 0; i < FrameCount; i++) {
		m_iActualLengths[i] = PatternLength;
		m_iNextPreviewFrame[i] = i + 1;
		if (theApp.m_pSettings->General.bFramePreview) {
			for (unsigned int j = 0; j < Channels; j++) {
				for (unsigned int k = 0; k < PatternLength; k++) {
					pDoc->GetNoteData(i, j, k, &Note);
					for (unsigned int l = 0; l < pDoc->GetEffColumns(j) + 1; l++) {
						if (Note.EffNumber[l] == EF_SKIP || 
							Note.EffNumber[l] == EF_JUMP ||
							Note.EffNumber[l] == EF_HALT && (k + 1) > m_iCursorRow) {
							m_iActualLengths[i] = k + 1;
							switch (Note.EffNumber[l]) {
								case EF_SKIP:
									m_iNextPreviewFrame[i] = i + 1;
									break;
								case EF_JUMP:
									m_iNextPreviewFrame[i] = Note.EffParam[l];
									break;
								case EF_HALT:
									m_iNextPreviewFrame[i] = i + 1;
									break;
							}
							if (m_iNextPreviewFrame[i] >= FrameCount)
								m_iNextPreviewFrame[i] = FrameCount - 1;
							break;
						}
					}
				}
			}
		}
	}
}

unsigned int CFamiTrackerView::GetCurrentPatternLength()
{
	return m_iActualLengths[m_iCurrentFrame];
}

void CFamiTrackerView::OnModuleMoveframedown()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Pattern;

	if (m_iCurrentFrame == (pDoc->GetFrameCount() - 1))
		return;

	for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
		Pattern = pDoc->GetPatternAtFrame(m_iCurrentFrame, i);
		pDoc->SetPatternAtFrame(m_iCurrentFrame, i, pDoc->GetPatternAtFrame(m_iCurrentFrame + 1, i));
		pDoc->SetPatternAtFrame(m_iCurrentFrame + 1, i, Pattern);
	}

	m_iCurrentFrame++;

	ForceRedraw();
}

void CFamiTrackerView::OnModuleMoveframeup()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int Pattern, i;

	if (m_iCurrentFrame == 0)
		return;

	for (i = 0; i < pDoc->GetAvailableChannels(); i++) {
		Pattern = pDoc->GetPatternAtFrame(m_iCurrentFrame, i);
		pDoc->SetPatternAtFrame(m_iCurrentFrame, i, pDoc->GetPatternAtFrame(m_iCurrentFrame - 1, i));
		pDoc->SetPatternAtFrame(m_iCurrentFrame - 1, i, Pattern);
	}

	m_iCurrentFrame--;

	ForceRedraw();
}

void CFamiTrackerView::OnUpdateModuleMoveframedown(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!(m_iCurrentFrame == (((CFamiTrackerDoc*)GetDocument())->GetFrameCount() - 1)));
}

void CFamiTrackerView::OnUpdateModuleMoveframeup(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!(m_iCurrentFrame == 0));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MIDI note functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::TriggerMIDINote(unsigned int Channel, unsigned int MidiNote, unsigned int Velocity, bool Insert)
{
	// Play a note (when pressing a key)

	unsigned int Octave = GET_OCTAVE(MidiNote);
	unsigned int Note = GET_NOTE(MidiNote);

	if (!theApp.m_pSettings->Midi.bMidiVelocity)
		Velocity = 127;

	InsertNote(Note, Octave, Channel, Velocity + 1);

	ArpeggioNotes[MidiNote] = 1;
	ArpeggioPtr = MidiNote;
	LastArpPtr = ArpeggioPtr;

	m_iLastMIDINote = MidiNote;

	KeyCount = 0;

	for (int i = 0; i < 128; i++) {
		if (ArpeggioNotes[i] == 1)
			KeyCount++;
	}
}

void CFamiTrackerView::ReleaseMIDINote(unsigned int Channel, unsigned int MidiNote)
{
	// A key is released

	unsigned int Octave = GET_OCTAVE(MidiNote);
	unsigned int Note = GET_NOTE(MidiNote);

	ArpeggioNotes[MidiNote] = 2;

	if (m_iLastMIDINote == MidiNote) {
		// Cut note
		stChanNote Note;
		memset(&Note, 0, sizeof(stChanNote));
		Note.Note = HALT;
		m_iLastMIDINote	= 0;
		FeedNote(Channel, &Note);
	}
}
