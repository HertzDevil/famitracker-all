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
#include "..\include\famitrackerview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define GET_CHANNEL(x)	((x - LEFT_OFFSET - 40) / CHANNEL_WIDTH)
#define GET_COLUMN(x)	((x - LEFT_OFFSET - 40) - (GET_CHANNEL(x) * CHANNEL_WIDTH))

const int DEFAULT_TEMPO_NTSC	= 150;
const int DEFAULT_TEMPO_PAL		= 125;
const int DEFAULT_SPEED			= 6;

enum { COL_NOTE, COL_INSTRUMENT, COL_VOLUME, COL_EFFECT };

const unsigned int TEXT_POSITION	= 10;
const unsigned int TOP_OFFSET		= 46;
const unsigned int LEFT_OFFSET		= 10;
const unsigned int ROW_HEIGHT		= 18;
const unsigned int COLUMNS			= 7;

const unsigned int COLUMN_SPACING	= 4;
const unsigned int CHAR_WIDTH		= 12;

const unsigned int ROW_NUMBER_WIDTH	= 20;
const unsigned int CHANNEL_SPACING	= 10;

const unsigned int CHANNEL_START	= LEFT_OFFSET * 2 + ROW_NUMBER_WIDTH - 3;

const unsigned int CHANNEL_WIDTH	= CHAR_WIDTH * 9 + COLUMN_SPACING * 3;
const unsigned int CHAN_AREA_WIDTH	= CHANNEL_WIDTH + CHANNEL_SPACING * 2;

const unsigned int COLUMN_SPACE[]	= {CHAR_WIDTH * 3 + COLUMN_SPACING,
									   CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING, 
									   CHAR_WIDTH + COLUMN_SPACING,  
									   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING,
									   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING,
									   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING,
									   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH + COLUMN_SPACING};

const unsigned int CURSOR_SPACE[]	= {CHAR_WIDTH * 3,
									   CHAR_WIDTH, CHAR_WIDTH,
									   CHAR_WIDTH,
									   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH,
									   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH,
									   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH,
									   CHAR_WIDTH, CHAR_WIDTH, CHAR_WIDTH};

const unsigned int COLUMN_SEL[]		= {0, CHAR_WIDTH * 3 + COLUMN_SPACING,
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

const unsigned int GET_APPR_START_SEL[] = {0, 1, 1, 3, 4, 4, 4, 7, 7, 7, 10, 10, 10, 13, 13, 13};
const unsigned int GET_APPR_END_SEL[]	= {0, 2, 2, 3, 6, 6, 6, 9, 9, 9, 12, 12, 12, 15, 15, 15};

// CFamiTrackerView

IMPLEMENT_DYNCREATE(CFamiTrackerView, CView)

BEGIN_MESSAGE_MAP(CFamiTrackerView, CView)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_VSCROLL()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditDelete)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_STOP, OnUpdateTrackerStop)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_PLAY, OnUpdateTrackerPlay)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_PLAYPATTERN, OnUpdateTrackerPlaypattern)
	ON_COMMAND(ID_TRACKER_PLAY, OnTrackerPlay)
	ON_COMMAND(ID_TRACKER_PLAYPATTERN, OnTrackerPlaypattern)
	ON_COMMAND(ID_TRACKER_STOP, OnTrackerStop)
	ON_COMMAND(ID_TRACKER_EDIT, OnTrackerEdit)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_EDIT, OnUpdateTrackerEdit)
	ON_COMMAND(ID_TRACKER_PAL, OnTrackerPal)
	ON_COMMAND(ID_TRACKER_NTSC, OnTrackerNtsc)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_PAL, OnUpdateTrackerPal)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_NTSC, OnUpdateTrackerNtsc)
	ON_UPDATE_COMMAND_UI(ID_SPEED_DEFALUT, OnUpdateSpeedDefalut)
	ON_UPDATE_COMMAND_UI(ID_SPEED_CUSTOM, OnUpdateSpeedCustom)
	ON_COMMAND(ID_SPEED_CUSTOM, OnSpeedCustom)
	ON_COMMAND(ID_SPEED_DEFALUT, OnSpeedDefalut)
	ON_WM_LBUTTONUP()
	ON_COMMAND(ID_EDIT_PASTEOVERWRITE, OnEditPasteoverwrite)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTEOVERWRITE, OnUpdateEditPasteoverwrite)
	ON_COMMAND(ID_TRANSPOSE_DECREASENOTE, OnTransposeDecreasenote)
	ON_COMMAND(ID_TRANSPOSE_DECREASEOCTAVE, OnTransposeDecreaseoctave)
	ON_COMMAND(ID_TRANSPOSE_INCREASENOTE, OnTransposeIncreasenote)
	ON_COMMAND(ID_TRANSPOSE_INCREASEOCTAVE, OnTransposeIncreaseoctave)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(ID_EDIT_SELECTALL, OnEditSelectall)
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_TIMER()
	ON_WM_CREATE()
	ON_COMMAND(ID_EDIT_ENABLEMIDI, OnEditEnablemidi)
	ON_UPDATE_COMMAND_UI(ID_EDIT_ENABLEMIDI, OnUpdateEditEnablemidi)
	ON_COMMAND(ID_FRAME_INSERT, OnFrameInsert)
	ON_COMMAND(ID_FRAME_REMOVE, OnFrameRemove)
	ON_COMMAND(ID_TRACKER_PLAYROW, OnTrackerPlayrow)
	ON_UPDATE_COMMAND_UI(ID_FRAME_REMOVE, OnUpdateFrameRemove)
	ON_WM_HSCROLL()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

struct stClipData {
	int			Size;
	bool		FirstColumn;
	int			NumCols;
	char		Notes[MAX_PATTERN_LENGTH];
	char		Octaves[MAX_PATTERN_LENGTH];
	int			ColumnData[COLUMNS + 3 * 4][MAX_PATTERN_LENGTH];
};

bool IgnoreFirst;

int m_iKeyboardNote;
int m_iSoloChannel;

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

// CFamiTrackerView construction/destruction

CFamiTrackerView::CFamiTrackerView()
{
	int i;

	m_bInitialized		= false;
	m_bRedrawed			= false;

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
}

CFamiTrackerView::~CFamiTrackerView()
{
}

BOOL CFamiTrackerView::PreCreateWindow(CREATESTRUCT& cs)
{
	m_iClipBoard = RegisterClipboardFormat("FamiTracker");

	if (m_iClipBoard == 0)
		MessageBox("Could not register clipboard format");

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

CDC		*dcBack;
CDC		*dcFont;

void CFamiTrackerView::OnDraw(CDC* pDC)
{	
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	const int TextWidth = 150;
	const int TextOffset = 60;

	LOGFONT LogFont;
	CBitmap Bmp, *OldBmp;
	CFont	Font, *OldFont;
	CPen	Pen, HiBgPen, BlackPen, SelectPen, CursorPen, *OldPen;
	CBrush	Brush, HiBgBrush, SelectBrush, CursorBrush, *OldBrush;

	int ColumnCount = 0, CurrentColumn = 0;

	unsigned int ColBackground		= theApp.m_pSettings->Appearance.iColBackground;
	unsigned int ColHiBackground	= theApp.m_pSettings->Appearance.iColBackgroundHilite;
	unsigned int ColText			= theApp.m_pSettings->Appearance.iColPatternText;
	unsigned int ColHiText			= theApp.m_pSettings->Appearance.iColPatternTextHilite;
	unsigned int ColSelection		= theApp.m_pSettings->Appearance.iColSelection;
	unsigned int ColCursor			= theApp.m_pSettings->Appearance.iColCursor;
	
	unsigned int CurrentBgCol, TextColor;

	int TrackerFieldRows = pDoc->m_iPatternLength;
	int i, y;
	
	int c, Nr;

	CString ChanName;
	char	Text[256];

	static DWORD LastTime;

	LPCSTR FontName = theApp.m_pSettings->General.strFont;

	if ((GetTickCount() - LastTime) < 30 && m_bPlaying)
		return;

	LastTime = GetTickCount();

	if (!pDoc)
		return;

	if (!pDoc->m_bFileLoaded) {
		pDC->Rectangle(0, 0, ViewAreaWidth, ViewAreaHeight);
		pDC->TextOut(5, 5, "No document is loaded");
		return;
	}

	if (!theApp.m_pSettings->General.bFreeCursorEdit)
		m_iCursorRow = m_iCurrentRow;

	if (m_iCurrentRow > (TrackerFieldRows - 1)) {
		m_iCurrentRow = (TrackerFieldRows - 1);
		m_iCursorRow = (TrackerFieldRows - 1);
	}

	dcBack = new CDC;

	Bmp.CreateCompatibleBitmap(pDC, WindowWidth, WindowHeight);
	dcBack->CreateCompatibleDC(pDC);
	OldBmp = dcBack->SelectObject(&Bmp);

	memset(&LogFont, 0, sizeof LOGFONT);
	memcpy(LogFont.lfFaceName, FontName, strlen(FontName));

	LogFont.lfHeight = -12;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;

	Font.CreateFontIndirect(&LogFont);

	// Create brushes
	Brush.CreateSolidBrush(ColBackground);
	HiBgBrush.CreateSolidBrush(ColHiBackground);
	SelectBrush.CreateSolidBrush(ColSelection);
	CursorBrush.CreateSolidBrush(ColCursor);

	// Create pens
	BlackPen.CreatePen(0, 5, 0xFF000000);
	HiBgPen.CreatePen(0, 0, ColHiBackground);
	SelectPen.CreatePen(0, 0, ColSelection);
	CursorPen.CreatePen(0, 0, ColCursor);

	if (m_bEditEnable) {
		if (m_bHasFocus)
			Pen.CreatePen(0, 5, COLOR_SCHEME.BORDER_EDIT_A);
		else
			Pen.CreatePen(0, 5, COLOR_SCHEME.BORDER_EDIT);
	}
	else {
		if (m_bHasFocus)
			Pen.CreatePen(0, 5, COLOR_SCHEME.BORDER_NORMAL_A);
		else
			Pen.CreatePen(0, 5, COLOR_SCHEME.BORDER_NORMAL);
	}

	OldBrush	= dcBack->SelectObject(&Brush);
	OldFont		= dcBack->SelectObject(&Font);
	OldPen		= dcBack->SelectObject(&Pen);

	dcBack->Rectangle(0, 0, WindowWidth - 22, WindowHeight - 22);

	dcBack->FillSolidRect(4, 4, WindowWidth - 31, TOP_OFFSET - 6, ColHiBackground);

	dcBack->SelectObject(&BlackPen);
	dcBack->SetBkColor(ColHiBackground);
	
	if (m_iCurrentRow > (VisibleRows / 2))
		Nr = m_iCurrentRow - VisibleRows / 2;
	else
		Nr = 0;

	if (m_iCurrentFrame < 0)
		m_iCurrentFrame = 0;

	int Offset, ChanWidth;

	// Determine weither the view needs to be scrolled horizontally
	Offset = CHANNEL_START + CHANNEL_SPACING;

	if (m_iStartChan > m_iCursorChannel)
		m_iStartChan = m_iCursorChannel;

	for (i = 0; i < pDoc->m_iChannelsAvailable; i++) {
		ChanWidth = CHANNEL_WIDTH + pDoc->m_iEffectColumns[i] * ((CHAR_WIDTH * 3) + COLUMN_SPACING) + CHANNEL_SPACING * 2;		
		
		m_iChannelWidths[i]		= ChanWidth;
		m_iChannelColumns[i]	= COLUMNS + pDoc->m_iEffectColumns[i] * 3;

		Offset += ChanWidth;

		if (m_iCursorChannel == i)
			CurrentColumn = ColumnCount + m_iCursorColumn;

		ColumnCount += COLUMNS + pDoc->m_iEffectColumns[i] * 3;
	}

	Offset = CHANNEL_START + CHANNEL_SPACING;

	for (i = m_iStartChan; i < pDoc->m_iChannelsAvailable; i++) {
		if (m_iCursorChannel == i) {
			if ((Offset + ChanWidth) > ViewAreaWidth) {
				m_iStartChan++;
				Offset = CHANNEL_START + CHANNEL_SPACING;
				i = m_iStartChan;
				continue;
			}
		}
		Offset += m_iChannelWidths[i];
	}


	m_iChannelsVisible = pDoc->m_iChannelsAvailable;

	int Width;

	// Draw channel headers
	Offset = CHANNEL_START + CHANNEL_SPACING;

	if (m_iStartChan != 0) {
		CString Text;
		Text = "<<";
		dcBack->SetTextColor(COLOR_SCHEME.TEXT_HILITE);
		dcBack->SetBkColor(ColHiBackground);
		dcBack->TextOut(10, TEXT_POSITION, Text);
	}

	for (i = m_iStartChan; i < pDoc->m_iChannelsAvailable; i++) {
		switch (i) {
			case 0: ChanName = "Square 1";		break;
			case 1: ChanName = "Square 2";		break;
			case 2: ChanName = "Triangle";		break;
			case 3: ChanName = "Noise";			break;
			case 4: ChanName = "DPCM";			break;
			case 5: ChanName = "VRC6 Square 1"; break;
			case 6: ChanName = "VRC6 Square 1"; break;
			case 7: ChanName = "VRC6 Sawtooth"; break;
		}

		dcBack->FillSolidRect(Offset - CHANNEL_SPACING - 3, 3, 1, ViewAreaHeight - 24, 0x808080);

		Width = CHANNEL_WIDTH + pDoc->m_iEffectColumns[i] * ((CHAR_WIDTH * 3) + COLUMN_SPACING) + CHANNEL_SPACING * 2;

		if ((Offset + Width) > (ViewAreaWidth - 3)) {
			m_iChannelsVisible = i;
			break;
		}

		dcBack->SetTextColor(m_bMuteChannels[i] ? COLOR_SCHEME.TEXT_MUTED : COLOR_SCHEME.TEXT_HILITE);
		dcBack->SetBkColor(ColHiBackground);
		dcBack->TextOut(Offset, TEXT_POSITION, ChanName);

		if (pDoc->m_iEffectColumns[i] > 0)
			dcBack->DrawIcon(Offset + CHANNEL_WIDTH - 27, 9, theApp.LoadIcon(IDI_ARROW_LEFT));
		if (pDoc->m_iEffectColumns[i] < (MAX_EFFECT_COLUMNS - 1))
			dcBack->DrawIcon(Offset + CHANNEL_WIDTH - 16, 9, theApp.LoadIcon(IDI_ARROW_RIGHT));

		Offset += Width;
	}

	if (m_iChannelsVisible != pDoc->m_iChannelsAvailable && (ViewAreaWidth - Offset) > 20) {
		CString Text;
		Text = ">>";
		dcBack->SetTextColor(COLOR_SCHEME.TEXT_HILITE);
		dcBack->SetBkColor(ColHiBackground);
		dcBack->TextOut(Offset, TEXT_POSITION, Text);
	}

	dcBack->FillSolidRect(Offset - CHANNEL_SPACING - 3, 3, 1, ViewAreaHeight - 24, 0x808080);

	Width = Offset - 15;

	if (Width > (ViewAreaWidth - 2))
		Width = (ViewAreaWidth - 2);

	dcBack->FillSolidRect(3, TOP_OFFSET - 2, Width, 1, 0x808080);

	dcBack->SetBkColor(ColBackground);

	int SelStart	= m_iSelectStart;
	int SelEnd		= m_iSelectEnd;

	if (SelStart > SelEnd) {
		SelStart	= m_iSelectEnd;
		SelEnd		= m_iSelectStart;
	}

	int SelColStart = m_iSelectColStart;
	int SelColEnd	= m_iSelectColEnd;

	if (SelColStart > SelColEnd) {
		SelColStart = m_iSelectColEnd; 
		SelColEnd	= m_iSelectColStart;
	}

	for (y = 0; y < VisibleRows; y++) {
		if ((m_iCurrentRow - (VisibleRows / 2) + y) >= 0 && 
			(m_iCurrentRow - (VisibleRows / 2) + y) < TrackerFieldRows) {

			if ((Nr & 3) == 0) {
				int RectWidth = WindowWidth - 27;
				if (RectWidth > 770)
					RectWidth = 770;
				dcBack->SetTextColor(ColHiText);
				TextColor = ColHiText;
				CurrentBgCol = ColHiBackground;
				dcBack->SelectObject(HiBgBrush);
				dcBack->SelectObject(HiBgPen);
				dcBack->FillSolidRect(3, TOP_OFFSET + (ROW_HEIGHT * y), CHANNEL_START - 6, ROW_HEIGHT - 2, ColHiBackground);
			}
			else {
				dcBack->SetTextColor(ColText);
				TextColor = ColText;
				CurrentBgCol = ColBackground;
			}

			dcBack->SetBkColor(CurrentBgCol);

			if (theApp.m_pSettings->General.bRowInHex)
				sprintf(Text, "%02X", Nr);
			else
				sprintf(Text, "%03i", Nr);

			dcBack->TextOut(LEFT_OFFSET, TOP_OFFSET + y * ROW_HEIGHT, Text, (int)strlen(Text));

			int CursorColor = -1;
			bool SelectionActive = false;

			int Offset = CHANNEL_START + CHANNEL_SPACING;

			for (i = m_iStartChan; i < m_iChannelsVisible; i++) {

				// Highlight row
				if (!(Nr & 3) &&  i < 5) {
					int Width = CHAN_AREA_WIDTH + pDoc->m_iEffectColumns[i] * (CHAR_WIDTH * 3 + COLUMN_SPACING) - 1;
					dcBack->FillSolidRect(Offset - 2 - CHANNEL_SPACING, TOP_OFFSET + (ROW_HEIGHT * y), Width, ROW_HEIGHT - 2, ColHiBackground);
				}

				// Selection
				if (Nr >= SelStart && Nr <= SelEnd && i == m_iSelectChannel && m_iSelectStart != -1) {
					int SelStart	= TOP_OFFSET + ROW_HEIGHT * (Nr + ((VisibleRows / 2) - m_iCurrentRow));
					int SelChan			= Offset + COLUMN_SEL[SelColStart] - 4;
					int SelChanEnd		= Offset + COLUMN_SEL[SelColEnd + 1] - SelChan - 4;
					dcBack->FillSolidRect(SelChan, SelStart - 1, SelChanEnd, ROW_HEIGHT, ColSelection);
					SelectionActive = true;
				}

				int ColOffset = 0;

				for (c = 0; c < signed(COLUMNS + pDoc->m_iEffectColumns[i] * 3); c++) {

					// Cursor box
					if (Nr == m_iCursorRow && i == m_iCursorChannel && c == m_iCursorColumn) {
						int BoxTop	= TOP_OFFSET + ROW_HEIGHT * y;
						int SelChan = Offset + ColOffset - 2;

						dcBack->FillSolidRect(SelChan, BoxTop - 1, CURSOR_SPACE[m_iCursorColumn], ROW_HEIGHT, ColCursor);
						dcBack->SetBkColor(ColCursor);
					}
					else {
						if (SelectionActive && i == m_iSelectChannel && c >= SelColStart && c <= SelColEnd)
							dcBack->SetBkColor(ColSelection);
						else
							dcBack->SetBkColor(CurrentBgCol);
					}

					DrawLine(Offset + ColOffset, pDoc->GetPatternData(m_iCurrentFrame, Nr, i), y, c, TextColor);

					ColOffset += COLUMN_SPACE[c];
				}

				Offset += m_iChannelWidths[i];
			}
			Nr++;
		}
	}

	int CursorCol1 = DIM(ColCursor, 100);
	int CursorCol2 = DIM(ColCursor, 80);
	int CursorCol3 = DIM(ColCursor, 60);

	// Selected row
	dcBack->Draw3dRect(3, TOP_OFFSET + (VisibleRows / 2) * ROW_HEIGHT - 2, WindowWidth - 30, ROW_HEIGHT + 2, CursorCol3, CursorCol3);
	dcBack->Draw3dRect(4, TOP_OFFSET + (VisibleRows / 2) * ROW_HEIGHT - 1, WindowWidth - 32, ROW_HEIGHT, CursorCol2, CursorCol2);
	dcBack->Draw3dRect(5, TOP_OFFSET + (VisibleRows / 2) * ROW_HEIGHT, WindowWidth - 34, ROW_HEIGHT - 2, CursorCol1, CursorCol1);

	DrawMeters(dcBack);

#ifdef WIP
	dcBack->SetBkColor(COLOR_SCHEME.CURSOR);
	dcBack->SetTextColor(COLOR_SCHEME.TEXT_HILITE);
	dcBack->TextOut(WindowWidth - 66, WindowHeight - 41, "WIP 7");
#endif

	pDC->BitBlt(0, 0, WindowWidth, WindowHeight, dcBack, 0, 0, SRCCOPY);

	dcBack->SelectObject(OldBmp);

	dcBack->SelectObject(OldBrush);
	dcBack->SelectObject(OldFont);
	dcBack->SelectObject(OldPen);

	if (TrackerFieldRows > 1)
		SetScrollRange(SB_VERT, 0, TrackerFieldRows - 1);
	else
		SetScrollRange(SB_VERT, 0, 1);

	SetScrollPos(SB_VERT, m_iCurrentRow);

	SetScrollRange(SB_HORZ, 0, ColumnCount - 1);
	SetScrollPos(SB_HORZ, CurrentColumn);

	delete dcBack;

	m_bRedrawed = true;
}

void CFamiTrackerView::DrawLine(int Offset, stChanNote *NoteData, int Line, int Column, int Color)
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
						DrawChar(OffsetX, LineY, '-', ShadedCol);
						DrawChar(OffsetX + CHAR_WIDTH, LineY, '-', ShadedCol);
						DrawChar(OffsetX + CHAR_WIDTH * 2, LineY, '-', ShadedCol);
					}
					else 
					{
						DrawChar(OffsetX, LineY, '*', Color);
						DrawChar(OffsetX + CHAR_WIDTH, LineY, '*', Color);
						DrawChar(OffsetX + CHAR_WIDTH * 2, LineY, '*', Color);
					}
					break;
				default:
					DrawChar(OffsetX, LineY, NOTES_A[Note - 1], Color);
					DrawChar(OffsetX + CHAR_WIDTH, LineY, NOTES_B[Note - 1], Color);
					DrawChar(OffsetX + CHAR_WIDTH * 2, LineY, NOTES_C[Octave], Color);
			}
			break;
		case 1:
			// Instrument x0
			if (Instrument == MAX_INSTRUMENTS || Note == HALT)
				DrawChar(OffsetX, LineY, '-', ShadedCol);
			else
				DrawChar(OffsetX, LineY, HEX[Instrument >> 4], Color);
			break;
		case 2:
			// Instrument 0x
			if (Instrument == MAX_INSTRUMENTS || Note == HALT)
				DrawChar(OffsetX, LineY, '-', ShadedCol);
			else
				DrawChar(OffsetX, LineY, HEX[Instrument & 0x0F], Color);
			break;
		case 3:
			// Volume
			if (Vol == 0x10)
				DrawChar(OffsetX, LineY, '-', ShadedCol);
			else
				DrawChar(OffsetX, LineY, HEX[Vol & 0x0F], Color);
			break;
		case 4: case 7: case 10: case 13:
			// Effect type
			if (EffNumber == 0)
				DrawChar(OffsetX, LineY, '-', ShadedCol);
			else
				DrawChar(OffsetX, LineY, EFF_CHAR[(EffNumber - 1) & 0x0F], Color);
			break;
		case 5: case 8: case 11: case 14:
			// Effect param x
			if (EffNumber == 0)
				DrawChar(OffsetX, LineY, '-', ShadedCol);
			else
				DrawChar(OffsetX, LineY, HEX[(EffParam >> 4) & 0x0F], Color);
			break;
		case 6: case 9: case 12: case 15:
			// Effect param y
			if (EffNumber == 0)
				DrawChar(OffsetX, LineY, '-', ShadedCol);
			else
				DrawChar(OffsetX, LineY, HEX[EffParam & 0x0F], Color);
			break;
	}

}

void CFamiTrackerView::DrawChar(int x, int y, char c, int Color) {	
	static CString Text;
	Text.Format("%c", c);
	dcBack->SetTextColor(Color);
	dcBack->TextOut(x, y, Text);
	
/*
	int SrcX = 0, SrcY = 0;

	if (c >= '0' && c <= '9') {
		SrcX = (c - '0') * 16;
		SrcY = 16;
	}
	else if (c >= 'A' && C <= 'O') {
		SrcX = 16 + (c - 'A') * 16;
		SrcY = 32;
	}
	else if (c == '#') {
		SrcX = 42;
	}

	dcBack->BitBlt(x, y, 16, 16, dcFont, SrcX, SrcY, SRCCOPY);
*/
}

BOOL CFamiTrackerView::OnEraseBkgnd(CDC* pDC)
{
	return FALSE; // CView::OnEraseBkgnd(pDC);
}

void CFamiTrackerView::CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType)
{
	WindowWidth		= lpClientRect->right - lpClientRect->left;
	WindowHeight	= lpClientRect->bottom - lpClientRect->top;

	VisibleRows		= (((WindowHeight - 23) + (ROW_HEIGHT / 2)) / ROW_HEIGHT) - 3;
	ViewAreaHeight	= WindowHeight - 4;
	ViewAreaWidth	= WindowWidth - 26;

	CView::CalcWindowRect(lpClientRect, nAdjustType);
}

BOOL CFamiTrackerView::PreTranslateMessage(MSG* pMsg)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	switch (pMsg->message) {
		case WM_USER + 1:
			TranslateMidiMessage();
			break;
	}

	return CView::PreTranslateMessage(pMsg);
}

void CFamiTrackerView::SetMessageText(LPCSTR lpszText)
{
	GetParentFrame()->SetMessageText(lpszText);
}

void CFamiTrackerView::DrawMeters(CDC *pDC)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CBrush	*OldBrush, Black;
	CPen	*OldPen, Green;

	CBrush	GreenBrush, HalfGreenBrush, GreyBrush;

	const int BAR_TOP		= TEXT_POSITION + 21;
	const int BAR_LEFT		= CHANNEL_START + CHANNEL_SPACING - 2;
	const int BAR_SIZE		= 8;
	const int BAR_SPACE		= 1;
	const int BAR_HEIGHT	= 5;

	int i, c;

	unsigned int Offset = BAR_LEFT;

	if (!pDoc->m_bFileLoaded)
		return;

	Black.CreateSolidBrush(0xFF000000);
	Green.CreatePen(0, 0, 0x106060);

	GreenBrush.CreateSolidBrush(0x00F000);
	GreyBrush.CreateSolidBrush(0x608060);

	OldBrush = pDC->SelectObject(&Black);
	OldPen = pDC->SelectObject(&Green);

	for (c = m_iStartChan; c < m_iChannelsVisible; c++) {	
		for (i = 0; i < 15; i++) {
			if (i < signed(m_iVolLevels[c]))
				pDC->FillRect(CRect(Offset + (i * BAR_SIZE), BAR_TOP, Offset + (i * BAR_SIZE) + (BAR_SIZE - BAR_SPACE), BAR_TOP + BAR_HEIGHT), &GreenBrush);
			else
				pDC->FillRect(CRect(Offset + (i * BAR_SIZE), BAR_TOP, Offset + (i * BAR_SIZE) + (BAR_SIZE - BAR_SPACE), BAR_TOP + BAR_HEIGHT), &GreyBrush);
		}
		Offset += CHANNEL_WIDTH + pDoc->m_iEffectColumns[c] * (CHAR_WIDTH * 3 + COLUMN_SPACING) + CHANNEL_SPACING * 2;
	}

	pDC->SelectObject(OldBrush);
	pDC->SelectObject(OldPen);
}

void CFamiTrackerView::ForceRedraw()
{
	m_bForceRedraw = true;
}

void CFamiTrackerView::SetSpeed(int Speed)
{
	m_iTickPeriod = Speed;
}

void CFamiTrackerView::SetInstrument(int Instrument)
{
	m_iInstrument = Instrument;
}

void CFamiTrackerView::TogglePlayback(void)
{
	if (!m_bPlaying)
		OnTrackerPlay();
	else
		OnTrackerStop();
}

void CFamiTrackerView::SetSongSpeed(int Speed)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_iTickPeriod = Speed;
	
	if (m_iTickPeriod > 20)
		m_iTempo = m_iTickPeriod;
	else
		m_iSpeed = m_iTickPeriod;
		
	pDoc->SetSongSpeed(Speed);
}

void CFamiTrackerView::PlayPattern()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (!m_bPlaying) {
		m_bPlaying			= true;
		m_bPlayLooped		= true;	
		m_iPlayerSyncTick	= 0;
		m_iTickPeriod		= pDoc->m_iSongSpeed;

		theApp.SilentEverything();
	}
	else {
		theApp.SilentEverything();
		m_bPlaying = false;
	}	
}

bool CFamiTrackerView::IsPlaying(void)
{
	return m_bPlaying;
}

void CFamiTrackerView::PlaybackTick(CFamiTrackerDoc *pDoc)
{
	// Called 50/60 (or other) times per second
	// Divide the time here to get the real play speed

	static bool	Jump = false;
	static bool	Skip = false;
	static int	JumpToPattern;
	static int	SkipToRow;
	int i;
	int TicksPerSec;
	stChanNote NoteData;

	TicksPerSec = pDoc->GetFrameRate();

	if (!m_bPlaying)
		GetStartSpeed();

	m_iCurrentTempo = (m_iTempo * 6) / m_iSpeed;

	if (!m_bPlaying)
		return;

	m_iPlayTime++;

	if (!m_iPlayerSyncTick) {

		Jump = false;
		Skip = false;

		for (i = 0; i < pDoc->m_iChannelsAvailable; i++) {
			if (!m_bMuteChannels[i])
				pDoc->GetNoteData(&NoteData, i, m_iCurrentRow, m_iCurrentFrame);
			else {
				NoteData.Note = HALT;
			}
			FeedNote(i, &NoteData);
		}

		// Evaluate effects
		for (int i = 0; i < pDoc->m_iChannelsAvailable; i++) {
			for (unsigned int n = 0; n < pDoc->m_iEffectColumns[i] + 1; n++) {
				switch (pDoc->GetNoteEffect(i, m_iCurrentRow, m_iCurrentFrame, n)) {
					// Axx Sets speed ticks to xx
					case EF_SPEED:
						m_iTickPeriod = pDoc->GetNoteEffectValue(i, m_iCurrentRow, m_iCurrentFrame, n);
						if (!m_iTickPeriod)
							m_iTickPeriod++;
						if (m_iTickPeriod > 20)
							m_iTempo = m_iTickPeriod;
						else
							m_iSpeed = m_iTickPeriod;
						break;
					// Bxx Jump to pattern xx
					case EF_JUMP:
						Jump = true;
						JumpToPattern = pDoc->GetNoteEffectValue(i, m_iCurrentRow, m_iCurrentFrame, n) - 1;
						if (m_bPlayLooped)
							Jump = false;
						break;
					// Cxx Skip to next track and start at row xx
					case EF_SKIP:
						Skip = true;
						SkipToRow = pDoc->GetNoteEffectValue(i, m_iCurrentRow, m_iCurrentFrame, n);
						if (m_bPlayLooped)
							Skip = false;
						break;
					// Dxx Halt playback
					case EF_HALT:
						//TogglePlayback();
						m_bPlaying = false;
						theApp.SilentEverything();
						//SetMessageText("Halted");		// multithreading with mfc?, an enigma???
						break;
				}
			}
		}
	}

	m_iTempoAccum -= (m_iTempo * 24) / m_iSpeed;	// 24 = 4 * 6

	if (m_iTempoAccum <= 0) {
		m_iTempoAccum += (60 * TicksPerSec);

		if (Jump) {
			m_iCurrentFrame = JumpToPattern + 1;
			m_iCurrentRow = -1;
			if (m_iCurrentFrame > (pDoc->m_iFrameCount - 1))
				m_iCurrentFrame = 0;
		}

		if (Skip) {
			m_iCurrentFrame++;
			if (m_iCurrentFrame < 0) {
				m_iCurrentFrame += pDoc->m_iFrameCount;
			}
			else if (m_iCurrentFrame > (pDoc->m_iFrameCount - 1)) {
				m_iCurrentFrame -= pDoc->m_iFrameCount;
			}
			m_iCurrentRow = SkipToRow/* - 1*/;
		}
		else
			m_iCurrentRow++;

		m_iPlayerSyncTick = 0;

		if (m_iCurrentRow == pDoc->m_iPatternLength) {
			ScrollToTop();
			if (!m_bPlayLooped) {
				m_iCurrentFrame++;
				if (m_iCurrentFrame < 0) {
					m_iCurrentFrame += pDoc->m_iFrameCount;
				}
				else if (m_iCurrentFrame > (pDoc->m_iFrameCount - 1)) {
					m_iCurrentFrame -= pDoc->m_iFrameCount;
				}
			}
		}
	}
	else {
		m_iPlayerSyncTick = 1;
	}
}

void CFamiTrackerView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	switch (nSBCode) {
		case SB_LINEDOWN:	ScrollRowDown();	break;
		case SB_LINEUP:		ScrollRowUp();		break;
		case SB_PAGEDOWN:	ScrollPageDown();	break;
		case SB_PAGEUP:		ScrollPageUp();		break;
		case SB_TOP:		ScrollToTop();		break;
		case SB_BOTTOM:		ScrollToBottom();	break;

		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			m_iCurrentRow = nPos;
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

	int i, x, count = 0;

	switch (nSBCode) {
		case SB_LINERIGHT:	MoveCursorRight();			break;
		case SB_LINELEFT:	MoveCursorLeft();			break;
		case SB_PAGERIGHT:	MoveCursorNextChannel();	break;
		case SB_PAGELEFT:	MoveCursorPrevChannel();	break;
		case SB_RIGHT:		MoveCursorFirstChannel();	break;
		case SB_LEFT:		MoveCursorLastChannel();	break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			for (i = 0; i < pDoc->m_iChannelsAvailable; i++) {
				for (x = 0; x < signed(COLUMNS + pDoc->m_iEffectColumns[i] * 3); x++) {
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

void CFamiTrackerView::WrapSelectedLine()		// or row
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (theApp.m_pSettings->General.bWrapCursor || m_bPlaying) {
		if (m_iCurrentRow < 0) {
			m_iCurrentRow += pDoc->m_iPatternLength;
			m_iCursorRow += pDoc->m_iPatternLength;
		}
		else if (m_iCurrentRow > (pDoc->m_iPatternLength - 1)) {
			m_iCurrentRow -= pDoc->m_iPatternLength;
			m_iCursorRow -= pDoc->m_iPatternLength;
		}
	}

	if (m_iCurrentRow < 0) {
		m_iCursorRow = 0;
		m_iCurrentRow = 0;
	}
	else if (m_iCurrentRow > (pDoc->m_iPatternLength - 1)) {
		m_iCursorRow = (pDoc->m_iPatternLength - 1);
		m_iCurrentRow = (pDoc->m_iPatternLength - 1);
	}
}

void CFamiTrackerView::WrapSelectedPattern()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_iCurrentFrame < 0) {
		m_iCurrentFrame += pDoc->m_iFrameCount;
	}
	else if (m_iCurrentFrame > (pDoc->m_iFrameCount - 1)) {
		m_iCurrentFrame -= pDoc->m_iFrameCount;
	}
}

void CFamiTrackerView::ScrollRowUp()
{
	if (theApp.m_pSettings->General.bFreeCursorEdit) {
		if (m_iCursorRow > 0) {
			m_iCursorRow--;
			if (m_iCursorRow - m_iCurrentRow < -signed(VisibleRows / 2))
				m_iCurrentRow--;
		}
	}
	else
		m_iCurrentRow--;

	WrapSelectedLine();
	ForceRedraw();
}

void CFamiTrackerView::ScrollRowDown()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (theApp.m_pSettings->General.bFreeCursorEdit && !m_bPlaying) {
		if (m_iCursorRow < (pDoc->m_iPatternLength - 1)) {
			m_iCursorRow++;
			if ((m_iCursorRow - m_iCurrentRow) > signed(VisibleRows / 2))
				m_iCurrentRow++;
		}
	}
	else
		m_iCurrentRow++; 

	WrapSelectedLine();
	ForceRedraw();
}

void CFamiTrackerView::StepDown()
{
	int i = 0;
	while (i++ < m_iKeyStepping)
		ScrollRowDown();
}

void CFamiTrackerView::ScrollPageUp()
{
	m_iCurrentRow -= 4; 
	WrapSelectedLine();
	ForceRedraw();
}

void CFamiTrackerView::ScrollPageDown()
{
	m_iCurrentRow += 4; 
	WrapSelectedLine();
	ForceRedraw();
}

void CFamiTrackerView::ScrollToTop()
{
	m_iCurrentRow = 0;
	ForceRedraw();
}

void CFamiTrackerView::ScrollToBottom()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_iCurrentRow = pDoc->m_iPatternLength - 1;
	ForceRedraw();
}

int CFamiTrackerView::GetCurrentColumnCount()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	return COLUMNS + pDoc->m_iEffectColumns[m_iCursorChannel] * 3;
}

void CFamiTrackerView::MoveCursorLeft()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int MaxColumns = GetCurrentColumnCount();

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
				m_iCursorColumn = MaxColumns - 1;
				m_iCursorChannel = pDoc->m_iChannelsAvailable - 1;
			}
		}
	}

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorRight()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int MaxColumns = GetCurrentColumnCount();

	if (m_iCursorColumn < (MaxColumns - 1)) {
		m_iCursorColumn++;
	}
	else {
		if (m_iCursorChannel < (pDoc->m_iChannelsAvailable - 1)) {
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
	m_iCursorChannel++;
	m_iCursorColumn = 0;

	if (m_iCursorChannel > (GetDocument()->m_iChannelsAvailable - 1)) {
		m_iCursorChannel = 0;
	}

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorPrevChannel()
{
	m_iCursorChannel--;
	m_iCursorColumn = 0;

	if (m_iCursorChannel < 0) {
		m_iCursorChannel = GetDocument()->m_iChannelsAvailable - 1;
	}

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
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	m_iCursorChannel	= pDoc->m_iChannelsAvailable - 1;
	m_iCursorColumn		= 0;
	ForceRedraw();
}

void CFamiTrackerView::SelectChannel(int Index)
{
	m_iCursorChannel	= Index;
	m_iCursorColumn		= 0;
	ForceRedraw();
}

void CFamiTrackerView::SelectPattern(int Index)
{
	m_iCurrentFrame = Index;
	WrapSelectedPattern();
	ForceRedraw();
}

void CFamiTrackerView::SelectNextPattern()
{
	if (m_iSelectStart != -1 && theApp.m_pSettings->General.bKeySelect) {
		// quick hack (as everything else)
		if (m_iSelectColEnd < (m_iChannelColumns[m_iCursorChannel] - 1))
			m_iSelectColEnd++;
	}
	else {
		m_iCurrentFrame++;
		WrapSelectedPattern();
	}
	ForceRedraw();
}

void CFamiTrackerView::SelectPrevPattern()
{
	if (m_iSelectStart != -1 && theApp.m_pSettings->General.bKeySelect) {
		if (m_iSelectColEnd > 0)
			m_iSelectColEnd--;
	}
	else {
		m_iCurrentFrame--;
		WrapSelectedPattern();
	}
	ForceRedraw();
}

void CFamiTrackerView::SelectFirstPattern()
{
	m_iCurrentFrame = 0;
	ForceRedraw();
}

void CFamiTrackerView::SelectLastPattern()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_iCurrentFrame = pDoc->m_iFrameCount - 1;
	ForceRedraw();
}

void CFamiTrackerView::IncreaseCurrentFrame()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_bChangeAllPattern) {
		for (int i = 0; i < pDoc->m_iChannelsAvailable; i++) {
			pDoc->IncreaseFrame(m_iCurrentFrame, i);
		}
	}
	else
		pDoc->IncreaseFrame(m_iCurrentFrame, m_iCursorChannel);

	ForceRedraw();
}

void CFamiTrackerView::DecreaseCurrentFrame()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_bChangeAllPattern) {
		for (int i = 0; i < pDoc->m_iChannelsAvailable; i++) {
			pDoc->DecreaseFrame(m_iCurrentFrame, i);
		}
	}
	else
		pDoc->DecreaseFrame(m_iCurrentFrame, m_iCursorChannel);

	ForceRedraw();
}

void CFamiTrackerView::StopNote(int Channel)
{
	stChanNote Note;

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	Note.Instrument		= 0;
	Note.Octave			= 0;
	Note.Note			= HALT;
	Note.EffNumber[0]	= 0;
	Note.EffParam[0]	= 0;
	Note.Vol			= 0;

	if (m_bEditEnable) {
		AddUndo(Channel);
		pDoc->ChannelData[Channel][pDoc->FrameList[m_iCurrentFrame][Channel]][m_iCursorRow] = Note;
		pDoc->SetModifiedFlag();
		ForceRedraw();
	}

	FeedNote(Channel, &Note);
}

void CFamiTrackerView::InsertNote(int Note, int Octave, int Channel, int Velocity)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote Cell;

	Cell = pDoc->ChannelData[Channel][pDoc->FrameList[m_iCurrentFrame][Channel]][m_iCursorRow];

	Cell.Note		= Note;
	Cell.Octave		= Octave;
	Cell.Instrument = m_iInstrument;
	
	if (Velocity < 128)
		Cell.Vol = (Velocity / 8);
//	else
//		Cell.Vol = 0;

	if (m_bEditEnable) {
		AddUndo(Channel);
		pDoc->ChannelData[Channel][pDoc->FrameList[m_iCurrentFrame][Channel]][m_iCursorRow] = Cell;
		pDoc->SetModifiedFlag();
		//ForceRedraw();

		if (m_iCursorColumn == 0 && m_bPlaying == false)
			StepDown();
	}

	FeedNote(Channel, &Cell);
}

bool CFamiTrackerView::TranslateKey(int Key, stChanNote *Note)
{
	// Translate a keyboard key into a note and fill it in *Note

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int	Octave = m_iOctave;
	int	KeyNote = 0, KeyOctave = 0;
		
	// Convert key into a note
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

	if (KeyNote == 0)
		return false;

	InsertNote(KeyNote, KeyOctave, m_iCursorChannel, 128);

	return true;
}

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

int ConvertKeyToDec(int Key)
{
	if (Key > 47 && Key < 58)
		return Key - 48;

	return -1;
}

void CFamiTrackerView::InterpretKey(int Key)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	static bool FirstChar = false;

	int EditedColumn = m_iCursorColumn;
	int EditStyle = theApp.m_pSettings->General.iEditStyle;

	stChanNote	Note;

	Note = pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow];

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
			TranslateKey(Key, &Note);
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
				switch (Key) {
					case '0': Note.EffNumber[Index] = EF_ARPEGGIO;	break;
					case '1': Note.EffNumber[Index] = EF_PORTAON;	break;
					case '2': Note.EffNumber[Index] = EF_PORTAOFF;	break;
					case '4': Note.EffNumber[Index] = EF_VIBRATO;	break;
					case '7': Note.EffNumber[Index] = EF_TREMOLO;	break;

					case 'B': Note.EffNumber[Index] = EF_JUMP;		break;
					case 'C': Note.EffNumber[Index] = EF_HALT;		break;
					case 'D': Note.EffNumber[Index] = EF_SKIP;		break;
					case 'E': Note.EffNumber[Index] = EF_VOLUME;	break;
					case 'F': Note.EffNumber[Index] = EF_SPEED;		break;

					case 'H': Note.EffNumber[Index] = EF_SWEEPUP;	break;
					case 'I': Note.EffNumber[Index] = EF_SWEEPDOWN;	break;
/*
					case 'A': Note.ExtraStuff1 = EF_SPEED;		break;		// A
					case 'B': Note.ExtraStuff1 = EF_JUMP;		break;		// B
					case 'C': Note.ExtraStuff1 = EF_SKIP;		break;		// C
					case 'D': Note.ExtraStuff1 = EF_HALT;		break;		// D
					case 'E': Note.ExtraStuff1 = EF_VOLUME;		break;		// E
					case 'F': Note.ExtraStuff1 = EF_PORTAON;	break;		// F
					case 'G': Note.ExtraStuff1 = EF_PORTAOFF;	break;		// G
					case 'H': Note.ExtraStuff1 = EF_SWEEPUP;	break;		// H
					case 'I': Note.ExtraStuff1 = EF_SWEEPDOWN;	break;		// I
*/
					default:
						return;
				}
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
		pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow] = Note;
		pDoc->SetModifiedFlag();

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
		Note.Note			= HALT;
		Note.EffNumber[0]	= 0;
		Note.EffParam[0]	= 0;
		
		FeedNote(m_iCursorChannel, &Note);
	}
}

void CFamiTrackerView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{	
	CFamiTrackerDoc* pDoc = GetDocument();
	int i;

	ASSERT_VALID(pDoc);

	switch (nChar) {
		case VK_UP:
			ScrollRowUp();
			if (m_bShiftPressed) {
				if (m_iSelectStart == -1 || m_iSelectEnd != m_iCursorRow) {
					m_iSelectStart		= m_iCursorRow;
					m_iSelectChannel	= m_iCursorChannel;
					m_iSelectColStart	= m_iCursorColumn;
					m_iSelectColEnd		= m_iCursorColumn;
				}
				if (m_iCursorRow > 0)
					m_iSelectEnd = m_iCursorRow - 1;
				else
					m_iSelectEnd = m_iCursorRow;
			}
			break;

		case VK_DOWN:
			ScrollRowDown();
			if (m_bShiftPressed) {
				if (m_iSelectStart == -1 || m_iSelectEnd != m_iCursorRow) {
					m_iSelectStart		= m_iCursorRow;
					m_iSelectChannel	= m_iCursorChannel;
					m_iSelectColStart	= m_iCursorColumn;
					m_iSelectColEnd		= m_iCursorColumn;
				}
				if (m_iCursorRow < pDoc->m_iPatternLength)
					m_iSelectEnd = m_iCursorRow + 1;
				else
					m_iSelectEnd = m_iCursorRow;

			}
			break;

		case VK_HOME:	ScrollToTop();		break;
		case VK_END:	ScrollToBottom();	break;
		case VK_PRIOR:	ScrollPageUp();		break;
		case VK_NEXT:	ScrollPageDown();	break;

		case VK_SPACE:	OnTrackerEdit();	break;

		case VK_ADD:
			if (m_bEditEnable) {
				AddUndo(m_iCursorChannel);
				switch (m_iCursorColumn) {
					case C_NOTE: 
						IncreaseCurrentFrame(); 
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
						DecreaseCurrentFrame(); 
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

		case VK_LEFT:
			if (!m_bShiftPressed) {
				MoveCursorLeft();
				ForceRedraw();
			}
			else {
				if (m_iSelectStart != -1) {
					if (m_iSelectColEnd > 0)
						m_iSelectColEnd--;
				}
			}
			break;

		case VK_RIGHT:
			if (!m_bShiftPressed) {
				MoveCursorRight();
				ForceRedraw();
			}
			else {
				if (m_iSelectStart != -1) {
					if (m_iSelectColEnd < signed(COLUMNS + pDoc->m_iEffectColumns[m_iCursorChannel] * 3))
						m_iSelectColEnd++;
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
				if (pDoc->DeleteNote(m_iCursorRow, m_iCurrentFrame, m_iCursorChannel, m_iCursorColumn)) {
					i = 0;
					while (i++ < m_iKeyStepping)
						ScrollRowDown();
					ForceRedraw();
				}
			}
			break;

		case VK_INSERT:
			if (!m_bEditEnable || PreventRepeat(VK_INSERT))
				return;
			AddUndo(m_iCursorChannel);
			pDoc->InsertNote(m_iCursorRow, m_iCurrentFrame, m_iCursorChannel);
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
				if (pDoc->RemoveNote(m_iCursorRow, m_iCurrentFrame, m_iCursorChannel)) {
					ScrollRowUp();
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
			InterpretKey(nChar);
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

	if (zDelta > 0) {
		m_iCurrentRow -= zDelta / 28;
		if (signed(m_iCurrentRow) < 0) {
			if (theApp.m_pSettings->General.bWrapCursor)
				m_iCurrentRow = (pDoc->m_iPatternLength - 1);
			else
				m_iCurrentRow = 0;
		}
	}
	else if (zDelta < 0) {
		m_iCurrentRow -= zDelta / 28;
		if (m_iCurrentRow > (pDoc->m_iPatternLength - 1)) {
			if (theApp.m_pSettings->General.bWrapCursor)
				m_iCurrentRow = 0;
			else
				m_iCurrentRow = (pDoc->m_iPatternLength - 1);
		}
	}

	ForceRedraw();

	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

int CFamiTrackerView::GetChannelAtPoint(int PointX)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int i, ChanSize, Offset = 0, Channel = -1;

	for (i = m_iStartChan; i < pDoc->m_iChannelsAvailable; i++) {
		ChanSize = m_iChannelWidths[i];
		Offset += ChanSize;
		if ((PointX - signed(CHANNEL_START)) < Offset) {
			Channel = i;
			break;
		}
	}

	return Channel;
}

int CFamiTrackerView::GetColumnAtPoint(int PointX, int MaxColumns)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int ColumnOffset = -1;
	int i;
	int Column = -1;

	int ChanSize, Offset = 0, Channel = -1;

	Offset = CHANNEL_START + CHANNEL_SPACING;

	for (i = m_iStartChan; i < pDoc->m_iChannelsAvailable; i++) {
		
		ChanSize = CHANNEL_WIDTH + (pDoc->m_iEffectColumns[i] * (CHAR_WIDTH * 3 + COLUMN_SPACING));
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
		if (ColumnOffset < Offset) {
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

	DeltaY = point.y - (ViewAreaHeight / 2);

	if (DeltaY < 0)
		DeltaRow = DeltaY / signed(ROW_HEIGHT) - 1;
	else
		DeltaRow = DeltaY / signed(ROW_HEIGHT);

	Column	= GetColumnAtPoint(point.x, COLUMNS + pDoc->m_iEffectColumns[Channel] * 3);

	if (Column == -1)
		return;
	
	m_iSelectColStart = Column;
	m_iSelectColEnd = Column;

	// The top row (channel name and meters)
	if (point.y < signed(ROW_HEIGHT * 2) && Channel < pDoc->m_iChannelsAvailable) {
		// Silence channel
		if (Column < 5) {
			m_bMuteChannels[Channel] = !m_bMuteChannels[Channel];
			if (m_iSoloChannel != -1 && m_iSoloChannel == Channel)
				m_iSoloChannel = -2;
			ForceRedraw();
			return;
		}
		// Remove one track effect column
		else if (Column == 5) {
			if (pDoc->m_iEffectColumns[Channel] > 0)
				pDoc->m_iEffectColumns[Channel]--;
		}
		// Add one track effect column
		else if (Column == 6) {
			if (pDoc->m_iEffectColumns[Channel] < (MAX_EFFECT_COLUMNS - 1))
				pDoc->m_iEffectColumns[Channel]++;
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

	Channel = GetChannelAtPoint(point.x);
	Column	= GetColumnAtPoint(point.x, COLUMNS + pDoc->m_iEffectColumns[Channel] * 3);

	if (Channel == -1 || Column == -1)
		return;

	DeltaY = point.y - signed(ViewAreaHeight / 2) - 5;

	if (DeltaY < 0)
		DeltaRow = DeltaY / signed(ROW_HEIGHT) - 1;
	else
		DeltaRow = DeltaY / signed(ROW_HEIGHT);

	if (m_iSelectStart == -1) {
		if (((DeltaRow + m_iCurrentRow) >= 0 && (DeltaRow + m_iCurrentRow) < signed(pDoc->m_iPatternLength)) && point.y > ROW_HEIGHT * 2) {
			m_iCursorRow = m_iCurrentRow + DeltaRow;
			if (!theApp.m_pSettings->General.bFreeCursorEdit)
				m_iCurrentRow = m_iCursorRow;

			if (Channel < pDoc->m_iChannelsAvailable && point.y > ROW_HEIGHT * 2) {
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
	Column	= GetColumnAtPoint(point.x, COLUMNS + pDoc->m_iEffectColumns[Channel] * 3);

	if (Channel == -1 || Column == -1)
		return;

	if (point.y < ROW_HEIGHT * 2 && Channel < pDoc->m_iChannelsAvailable) {
		if (Column < 5) {
			if (m_iSoloChannel == -2) {
				for (int i = 0; i < pDoc->m_iChannelsAvailable; i++)
					m_bMuteChannels[i] = false;
				m_iSoloChannel = -1;
			}
			else {
				m_bMuteChannels[Channel] = false;
				for (int i = 0; i < pDoc->m_iChannelsAvailable; i++) {
					if (i != Channel)
						m_bMuteChannels[i] = true;
				}
				m_iSoloChannel = Channel;
			}
			ForceRedraw();
		}
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
	int DeltaRow, DeltaY = point.y - (ViewAreaHeight / 2) - 10;
	int Channel, Column;

	if (!(nFlags & MK_LBUTTON))
		return;

	if (IgnoreFirst) {
		IgnoreFirst = false;
		return;
	}

	if (point.y < ROW_HEIGHT * 2)
		return;

	Channel = GetChannelAtPoint(point.x);
	Column	= GetColumnAtPoint(point.x, COLUMNS + pDoc->m_iEffectColumns[Channel] * 3);

	if (Channel == -1 || Column == -1)
		return;

	if (Channel == m_iSelectChannel)
		m_iSelectColEnd = Column;

	if (DeltaY < 0)
		DeltaRow = DeltaY / signed(ROW_HEIGHT) - 1;
	else
		DeltaRow = DeltaY / signed(ROW_HEIGHT);
	
	int Sel = m_iCurrentRow + DeltaRow;

	if (((DeltaRow + m_iCurrentRow) >= 0 && (DeltaRow + m_iCurrentRow) < signed(pDoc->m_iPatternLength)) && Channel >= 0) {
		
		if (m_iSelectStart != -1) {
			m_iSelectEnd = Sel;
			ForceRedraw();
		}
		else {
			m_iSelectStart = Sel;
			m_iSelectChannel = Channel;
		}
		
		if (DeltaRow >= signed((ViewAreaHeight / ROW_HEIGHT) / 2) - 2)
			ScrollRowDown();
		else if (DeltaRow <= (-signed((ViewAreaHeight / ROW_HEIGHT) / 2) + 2))
			ScrollRowUp();
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
	stChanNote	*Note;
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
		AfxMessageBox("Cannot open the clipboard");
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
		Note = pDoc->GetNoteData(m_iSelectChannel, i, m_iCurrentFrame);

		if (SelColStart == 0) {
			ClipData->Notes[Ptr]	= Note->Note;
			ClipData->Octaves[Ptr]	= Note->Octave;
		}

		ColPtr = 0;
		for (int n = 1; n < COLUMNS + MAX_EFFECT_COLUMNS * 3; n++) {
			if (n >= SelColStart && n <= SelColEnd) {
				switch (n - 1) {
					case 0: ClipData->ColumnData[ColPtr][Ptr] = Note->Instrument >> 4;		break;		// Instrument
					case 1: ClipData->ColumnData[ColPtr][Ptr] = Note->Instrument & 0x0F;	break;
					case 2: ClipData->ColumnData[ColPtr][Ptr] = Note->Vol;					break;		// Volume
					case 3: ClipData->ColumnData[ColPtr][Ptr] = Note->EffNumber[0];			break;		// Effect column 1
					case 4: ClipData->ColumnData[ColPtr][Ptr] = Note->EffParam[0] >> 4;		break;
					case 5: ClipData->ColumnData[ColPtr][Ptr] = Note->EffParam[0] & 0x0F;	break;
					case 6: ClipData->ColumnData[ColPtr][Ptr] = Note->EffNumber[1];			break;		// Effect column 2
					case 7: ClipData->ColumnData[ColPtr][Ptr] = Note->EffParam[1] >> 4;		break;
					case 8: ClipData->ColumnData[ColPtr][Ptr] = Note->EffParam[1] & 0x0F;	break;
					case 9: ClipData->ColumnData[ColPtr][Ptr] = Note->EffNumber[2];			break;		// Effect column 3
					case 10: ClipData->ColumnData[ColPtr][Ptr] = Note->EffParam[2] >> 4;	break;
					case 11: ClipData->ColumnData[ColPtr][Ptr] = Note->EffParam[2] & 0x0F;	break;
					case 12: ClipData->ColumnData[ColPtr][Ptr] = Note->EffNumber[3];		break;		// Effect column 4
					case 13: ClipData->ColumnData[ColPtr][Ptr] = Note->EffParam[3] >> 4;	break;
					case 14: ClipData->ColumnData[ColPtr][Ptr] = Note->EffParam[3] & 0x0F;	break;
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

	HGLOBAL		hMem;
	stChanNote	*ChanNote;
	stClipData	*ClipData;
	int			i;

	if (m_bEditEnable == false)
		return;

	OpenClipboard();

	if (!IsClipboardFormatAvailable(m_iClipBoard)) {
		MessageBox("Clipboard format is not avaliable");
	}

	hMem = GetClipboardData(m_iClipBoard);

	if (hMem == NULL)
		return;

	AddUndo(m_iCursorChannel);

	ClipData = (stClipData*)GlobalLock(hMem);
/*
	if (m_bPasteOverwrite) {
		for (i = 0; i < ClipData->Size; i++) {
			pDoc->RemoveNote((m_iCursorRow + 1), m_iCurrentFrame, m_iCursorChannel);
		}
	}
*/
	for (i = 0; i < ClipData->Size; i++) {
		if ((m_iCursorRow + i) < pDoc->m_iPatternLength) {

			if (!m_bPasteOverwrite)
				pDoc->InsertNote(m_iCursorRow + i, m_iCurrentFrame, m_iCursorChannel);

			//pDoc->GetNoteData(&ChanNote, m_iCursorChannel, m_iCursorRow + i, m_iCurrentFrame);
			ChanNote = pDoc->GetNoteData(m_iCursorChannel, m_iCursorRow + i, m_iCurrentFrame);

			if (ClipData->FirstColumn) {
				ChanNote->Octave	= ClipData->Octaves[i];
				ChanNote->Note		= ClipData->Notes[i];
			}

			int ColPtr = 0;

			for (int n = m_iCursorColumn; n < (COLUMNS + MAX_EFFECT_COLUMNS * 3); n++) {
				if (ColPtr < ClipData->NumCols && n > 0) {
					switch (n - 1) {
						case 0: 		// Instrument
							ChanNote->Instrument = (ChanNote->Instrument & 0x0F) | ClipData->ColumnData[ColPtr][i] << 4;
							if (ChanNote->Instrument != 64)
								ChanNote->Instrument &= 0x3F;
							break;
						case 1: 
							ChanNote->Instrument = (ChanNote->Instrument & 0xF0) | ClipData->ColumnData[ColPtr][i]; 
							if (ChanNote->Instrument != 64)
								ChanNote->Instrument &= 0x3F; 
							break;
						case 2:			// Volume
							ChanNote->Vol = ClipData->ColumnData[ColPtr][i]; 
							if (ChanNote->Vol != 0x10)
								ChanNote->Vol &= 0x0F;
							break;
						case 3: ChanNote->EffNumber[0]	= ClipData->ColumnData[ColPtr][i]; break;											// First effect column
						case 4: ChanNote->EffParam[0]	= (ChanNote->EffParam[0] & 0x0F) | (ClipData->ColumnData[ColPtr][i] << 4);	break;
						case 5: ChanNote->EffParam[0]	= (ChanNote->EffParam[0] & 0xF0) | (ClipData->ColumnData[ColPtr][i] & 0x0F); break;
						case 6: ChanNote->EffNumber[1]	= ClipData->ColumnData[ColPtr][i]; break;											// Second effect column
						case 7: ChanNote->EffParam[1]	= (ChanNote->EffParam[1] & 0x0F) | (ClipData->ColumnData[ColPtr][i] << 4);	break;
						case 8: ChanNote->EffParam[1]	= (ChanNote->EffParam[1] & 0xF0) | (ClipData->ColumnData[ColPtr][i] & 0x0F); break;
						case 9: ChanNote->EffNumber[2]	= ClipData->ColumnData[ColPtr][i]; break;											// Third effect column
						case 10: ChanNote->EffParam[2]	= (ChanNote->EffParam[2] & 0x0F) | (ClipData->ColumnData[ColPtr][i] << 4);	break;
						case 11: ChanNote->EffParam[2]	= (ChanNote->EffParam[2] & 0xF0) | (ClipData->ColumnData[ColPtr][i] & 0x0F); break;
						case 12: ChanNote->EffNumber[3]	= ClipData->ColumnData[ColPtr][i]; break;											// Fourth effect column
						case 13: ChanNote->EffParam[3]	= (ChanNote->EffParam[3] & 0x0F) | (ClipData->ColumnData[ColPtr][i] << 4);	break;
						case 14: ChanNote->EffParam[3]	= (ChanNote->EffParam[3] & 0xF0) | (ClipData->ColumnData[ColPtr][i] & 0x0F); break;
					}
					ColPtr++;
				}
			}

			//pDoc->SetNoteData(m_iCursorChannel, m_iCursorRow + i, m_iCurrentFrame, ChanNote);
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
		pDoc->GetNoteData(&NoteData, m_iSelectChannel, i, m_iCurrentFrame);
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

		pDoc->SetNoteData(m_iSelectChannel, i, m_iCurrentFrame, &NoteData);

		/*
		IsColumnSelected(SelColStart, SelColEnd, 0);
		pDoc->RemoveNote(SelStart + 1, m_iCurrentFrame, m_iSelectChannel);
		*/
	}

	m_iSelectStart = -1;

	ForceRedraw();
}

void CFamiTrackerView::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	if (m_iSelectStart == -1)
		pCmdUI->Enable(0);
	else
		pCmdUI->Enable(1);
}

void CFamiTrackerView::OnUpdateEditCut(CCmdUI *pCmdUI)
{
	if (m_iSelectStart == -1)
		pCmdUI->Enable(0);
	else
		pCmdUI->Enable(1);
}

void CFamiTrackerView::OnUpdateEditPaste(CCmdUI *pCmdUI)
{
	if (IsClipboardFormatAvailable(m_iClipBoard))
		pCmdUI->Enable(1);
	else
		pCmdUI->Enable(0);
}

void CFamiTrackerView::OnUpdateEditDelete(CCmdUI *pCmdUI)
{
	if (m_iSelectStart == -1)
		pCmdUI->Enable(0);
	else
		pCmdUI->Enable(1);
}

void CFamiTrackerView::ResetView()
{
	m_iCurrentFrame	= 0;
	m_iCurrentRow	= 0;
	m_iCursorRow	= 0;
}

void CFamiTrackerView::OnUpdateTrackerStop(CCmdUI *pCmdUI)
{
	if (m_bPlaying)
		pCmdUI->Enable(1);
	else
		pCmdUI->Enable(0);
}

void CFamiTrackerView::OnUpdateTrackerPlay(CCmdUI *pCmdUI)
{
	if (m_bPlaying)
		pCmdUI->Enable(0);
	else
		pCmdUI->Enable(1);
}

void CFamiTrackerView::OnUpdateTrackerPlaypattern(CCmdUI *pCmdUI)
{
	if (m_bPlaying)
		pCmdUI->Enable(0);
	else
		pCmdUI->Enable(1);
}

void CFamiTrackerView::OnTrackerPlay()
{
	CMainFrame *pMainFrm = (CMainFrame*)GetParentFrame();
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_bPlaying)
		return;

	m_bPlaying			= true;
	m_bPlayLooped		= false;
	m_bFirstTick		= true;
	m_iPlayTime			= 0;
	m_iPlayerSyncTick	= 0;

	GetStartSpeed();

	theApp.SilentEverything();

	ScrollToTop();
	ForceRedraw();

	SetMessageText("Playing");

	theApp.SetMachineType(pDoc->m_iMachine, pDoc->m_iEngineSpeed);
}

void CFamiTrackerView::OnTrackerPlaypattern()
{
	CMainFrame *pMainFrm = (CMainFrame*)GetParentFrame();
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_bPlaying)
		return;

	m_bPlaying			= true;
	m_bPlayLooped		= true;	
	m_iPlayTime			= 0;
	m_iPlayerSyncTick	= 0;

	GetStartSpeed();

	SetMessageText("Playing pattern");

	theApp.SilentEverything();
}

void CFamiTrackerView::OnTrackerStop()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	theApp.SilentEverything();

	SetMessageText("Stopped");

	m_bPlaying		= false;
	m_iTickPeriod	= pDoc->m_iSongSpeed;
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
	if (m_bEditEnable)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}

void CFamiTrackerView::OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int i;

	CMainFrame	*MainFrame = (CMainFrame*)GetParentFrame();

	m_iCurrentFrame		= 0;
	m_iCurrentRow		= 0;
	m_iCursorChannel	= 0;
	m_iCursorColumn		= 0;

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (!pDoc->Instruments[i].Free)
			MainFrame->AddInstrument(i, pDoc->Instruments[i].Name);
	}

	if (pDoc->m_iEngineSpeed == 0) {
		if (pDoc->m_iMachine == NTSC)
			theApp.SetMachineType(pDoc->m_iMachine, 60);
		else
			theApp.SetMachineType(pDoc->m_iMachine, 50);
	}
	else
		theApp.SetMachineType(pDoc->m_iMachine, pDoc->m_iEngineSpeed);

	MainFrame->SetSongInfo(pDoc->m_strName, pDoc->m_strArtist, pDoc->m_strCopyright);

	ForceRedraw();
}

void CFamiTrackerView::OnTrackerPal()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CMainFrame*	pMainFrm = (CMainFrame*)GetParentFrame();

	int Speed = (pDoc->m_iEngineSpeed == 0) ? 50 : pDoc->m_iEngineSpeed;

	pDoc->m_iMachine = PAL;

	theApp.SetMachineType(pDoc->m_iMachine, Speed);
}

void CFamiTrackerView::OnTrackerNtsc()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CMainFrame*	pMainFrm = (CMainFrame*)GetParentFrame();

	int Speed = (pDoc->m_iEngineSpeed == 0) ? 60 : pDoc->m_iEngineSpeed;

	pDoc->m_iMachine = NTSC;

	theApp.SetMachineType(pDoc->m_iMachine, Speed);
}

void CFamiTrackerView::OnUpdateTrackerPal(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
	if (pDoc->m_iMachine == PAL)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}

void CFamiTrackerView::OnUpdateTrackerNtsc(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
	if (pDoc->m_iMachine == NTSC)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}

void CFamiTrackerView::OnUpdateSpeedDefalut(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
	if (pDoc->m_iEngineSpeed == 0)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}

void CFamiTrackerView::OnUpdateSpeedCustom(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
	if (pDoc->m_iEngineSpeed != 0)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}

void CFamiTrackerView::OnSpeedCustom()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CSpeedDlg SpeedDlg;
	int Speed;

	Speed = pDoc->m_iEngineSpeed;

	if (Speed == 0)
		Speed = (pDoc->m_iMachine == NTSC ? 60 : 50);

	Speed = SpeedDlg.GetSpeedFromDlg(Speed);

	if (Speed == 0)
		return;

	pDoc->m_iEngineSpeed = Speed;

	theApp.SetMachineType(pDoc->m_iMachine, Speed);
}

void CFamiTrackerView::OnSpeedDefalut()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Speed = (pDoc->m_iMachine == NTSC) ? 60 : 50;

	pDoc->m_iEngineSpeed = 0;

	theApp.SetMachineType(pDoc->m_iMachine, Speed);
}

void CFamiTrackerView::OnEditPasteoverwrite()
{
	m_bPasteOverwrite = !m_bPasteOverwrite;
}

void CFamiTrackerView::OnUpdateEditPasteoverwrite(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bPasteOverwrite);
}

void CFamiTrackerView::OnTransposeDecreasenote()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (!m_bEditEnable)
		return;

	if (m_iSelectStart == -1) {
		AddUndo(m_iCursorChannel);
		if (pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Note > 0 &&
			pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Note != HALT) {
			if (pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Note > 1) 
				pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Note--;
			else {
				if (pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Octave > 0) {
					pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Note = B;
					pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Octave--;
				}
			}
		}
	}
	else {
		int SelStart;
		int SelEnd;

		AddUndo(m_iSelectChannel);

		if (m_iSelectStart > m_iSelectEnd) {
			SelStart = m_iSelectEnd;
			SelEnd = m_iSelectStart;
		}
		else {
			SelStart = m_iSelectStart;
			SelEnd = m_iSelectEnd;
		}
		
		for (int i = SelStart; i < SelEnd + 1; i++) {
			if (pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Note > 0 && 
				pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][i].Note != HALT) {
				if (pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Note > 1)
					pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Note--;
				else {
					if (pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Octave > 0) {
						pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Note = B;
						pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Octave--;
					}
				}
			}
		}
	}

	ForceRedraw();
}

void CFamiTrackerView::OnTransposeDecreaseoctave()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (!m_bEditEnable)
		return;

	if (m_iSelectStart == -1) {
		AddUndo(m_iCursorChannel);
		if (pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Octave > 0) 
			pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Octave--;
	}
	else {
		int SelStart;
		int SelEnd;

		AddUndo(m_iSelectChannel);

		if (m_iSelectStart > m_iSelectEnd) {
			SelStart = m_iSelectEnd;
			SelEnd = m_iSelectStart;
		}
		else {
			SelStart = m_iSelectStart;
			SelEnd = m_iSelectEnd;
		}

		for (int i = SelStart; i < SelEnd + 1; i++) {
			if (pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Octave > 0)
				pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Octave--;
		}
	}

	ForceRedraw();
}

void CFamiTrackerView::OnTransposeIncreasenote()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (!m_bEditEnable)
		return;

	if (m_iSelectStart == -1) {
		AddUndo(m_iCursorChannel);
		if (pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Note > 0 &&
			pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Note != HALT) {

			if (pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Note < B)
				pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Note++;
			else {
				if (pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Octave < 8) {
					pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Note = C;
					pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Octave++;
				}
			}
		}
	}
	else {
		int SelStart;
		int SelEnd;

		AddUndo(m_iSelectChannel);

		if (m_iSelectStart > m_iSelectEnd) {
			SelStart = m_iSelectEnd;
			SelEnd = m_iSelectStart;
		}
		else {
			SelStart = m_iSelectStart;
			SelEnd = m_iSelectEnd;
		}

		for (int i = SelStart; i < SelEnd + 1; i++) {
			if (pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Note > 0 &&
				pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][i].Note != HALT) {
				if (pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Note < B)
					pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Note++;
				else {
					if (pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Octave < 8) {
						pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Note = C;
						pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Octave++;
					}
				}
			}
		}
	}

	ForceRedraw();

}

void CFamiTrackerView::OnTransposeIncreaseoctave()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (!m_bEditEnable)
		return;

	if (m_iSelectStart == -1) {
		AddUndo(m_iCursorChannel);
		if (pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Octave < 8) 
			pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Octave++;
	}
	else {
		int SelStart;
		int SelEnd;

		AddUndo(m_iSelectChannel);

		if (m_iSelectStart > m_iSelectEnd) {
			SelStart = m_iSelectEnd;
			SelEnd = m_iSelectStart;
		}
		else {
			SelStart = m_iSelectStart;
			SelEnd = m_iSelectEnd;
		}

		for (int i = SelStart; i < SelEnd + 1; i++) {
			if (pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Octave < 8)
				pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Octave++;
		}
	}

	ForceRedraw();
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
		int Pattern = pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel];
		int x;

		m_UndoStack[m_iUndoLevel].Channel	= Channel;
		m_UndoStack[m_iUndoLevel].Pattern	= Pattern;
		m_UndoStack[m_iUndoLevel].Row		= m_iCursorRow;
		m_UndoStack[m_iUndoLevel].Column	= m_iCursorColumn;
		m_UndoStack[m_iUndoLevel].Frame		= m_iCurrentFrame;

		for (x = 0; x < MAX_PATTERN_LENGTH; x++) {
			m_UndoStack[m_iUndoLevel].ChannelData[x] = pDoc->ChannelData[Channel][Pattern][x];
		}
	}

	m_iRedoLevel++;
	m_iUndoLevel--;

	Channel = m_UndoStack[m_iUndoLevel].Channel;
	Pattern = m_UndoStack[m_iUndoLevel].Pattern;

	for (x = 0; x < MAX_PATTERN_LENGTH; x++) {
		pDoc->ChannelData[Channel][Pattern][x] = m_UndoStack[m_iUndoLevel].ChannelData[x];
	}

	m_iCursorChannel	= m_UndoStack[m_iUndoLevel].Channel;
	m_iCursorRow		= m_UndoStack[m_iUndoLevel].Row;
	m_iCursorColumn		= m_UndoStack[m_iUndoLevel].Column;
	m_iCurrentFrame		= m_UndoStack[m_iUndoLevel].Frame;

	pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel] = Pattern;

	m_iCurrentRow = m_iCursorRow;

	Text.Format("Undo (%i / %i)", m_iUndoLevel, m_iUndoLevel + m_iRedoLevel);
	SetMessageText(Text);

	ForceRedraw();
}

void CFamiTrackerView::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_iUndoLevel > 0 ? 1 : 0);
}

void CFamiTrackerView::AddUndo(int Channel)
{
	// Channel - Channel that contains the pattern that should be added ontop of undo stack
	//

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Pattern = pDoc->FrameList[m_iCurrentFrame][Channel];
	int x;

	stUndoBlock UndoBlock;
	
	UndoBlock.Channel	= Channel;
	UndoBlock.Pattern	= Pattern;
	UndoBlock.Row		= m_iCursorRow;
	UndoBlock.Column	= m_iCursorColumn;
	UndoBlock.Frame		= m_iCurrentFrame;

	for (x = 0; x < MAX_PATTERN_LENGTH; x++) {
		UndoBlock.ChannelData[x] = pDoc->ChannelData[Channel][Pattern][x];
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
		pDoc->ChannelData[Channel][Pattern][x] = m_UndoStack[m_iUndoLevel].ChannelData[x];
	}

	m_iCursorChannel	= Channel;
	m_iCursorRow		= m_UndoStack[m_iUndoLevel].Row;
	m_iCursorColumn		= m_UndoStack[m_iUndoLevel].Column;
	m_iCurrentFrame		= m_UndoStack[m_iUndoLevel].Frame;

	pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel] = Pattern;

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

	CView::OnInitialUpdate();

	if (m_bPlaying) {
		OnTrackerStop();
	}

	m_iLastCursorColumn = 0;
	m_iLastRowState		= 0;
	m_bInitialized		= true;
	m_bForceRedraw		= true;
	m_iInstrument		= 0;
	m_iUndoLevel		= 0;
	m_iRedoLevel		= 0;
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
		pDoc->SetNoteData(m_iSelectChannel, i, m_iCurrentFrame, &Note);
	}

	m_iSelectStart = -1;

	ForceRedraw();
}

void CFamiTrackerView::OnEditSelectall()
{
	SelectWholePattern(m_iCursorChannel);
}

void CFamiTrackerView::SelectWholePattern(int Channel)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_iSelectStart == -1) {
		m_iSelectChannel	= Channel;
		m_iSelectStart		= 0;
		m_iSelectEnd		= pDoc->m_iPatternLength - 1;
		m_iSelectColStart	= 0;
		m_iSelectColEnd		= COLUMNS - 1 + (pDoc->m_iEffectColumns[Channel] * 3);
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
				Status.Format("MIDI message: Note on (note = %02i, octave = %02i, vel = %03i)", Note, Octave, Velocity);
				SetMessageText(Status);
				if (Velocity == 0) {
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
					LastNote	= Note;
					LastOctave	= Octave;
				}
				break;

			case MIDI_MSG_NOTE_OFF:
				if (Channel > 4)
					return;
				Status.Format("MIDI message: Note off");
				SetMessageText(Status);
				StopNote(Channel);
				break;
			
			case 0x0F:
				if (Channel == 0x08) {
					int i = 0;
					while (i++ < m_iKeyStepping)
						ScrollRowDown();
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

	if (nIDEvent == 0) {

		pMainFrm = (CMainFrame*)GetParentFrame();

		// Fetch channel levels

		m_iVolLevels[0] = theApp.GetOutput(0);
		m_iVolLevels[1] = theApp.GetOutput(1);
		m_iVolLevels[2] = theApp.GetOutput(2);
		m_iVolLevels[3] = theApp.GetOutput(3);
		m_iVolLevels[4] = theApp.GetOutput(4) / 8;

		// Redraw screen
		if (m_iCurrentRow != m_iLastRowState || m_iCurrentFrame != m_iLastFrameState || m_bForceRedraw) {

			if (m_iCurrentRow != m_iLastRowState || m_iCurrentFrame != m_iLastFrameState || m_iCursorColumn != m_iLastCursorColumn)
				RefreshStatusMessage();

			m_iLastRowState		= m_iCurrentRow;
			m_iLastFrameState	= m_iCurrentFrame;
			m_iLastCursorColumn	= m_iCursorColumn;
			m_bForceRedraw		= false;

			Invalidate(FALSE);
			PostMessage(WM_PAINT);

			((CMainFrame*)GetParent())->RefreshPattern();
		}
		else {
			CDC *pDC = this->GetDC();
			DrawMeters(pDC);
			ReleaseDC(pDC);
		}

		if (LastNoteState != m_iKeyboardNote)
			pMainFrm->ChangeNoteState(m_iKeyboardNote);

		if (m_bPlaying) {
			//if ((m_iPlayTime & 0x01) == 0x01)
			pMainFrm->SetIndicatorTime((m_iPlayTime / TicksPerSec) / 60, (m_iPlayTime / TicksPerSec) % 60, ((m_iPlayTime * 10) / TicksPerSec) % 10);
		}

		LastNoteState	= m_iKeyboardNote;
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
	//MIDI.Toggle();
}

void CFamiTrackerView::OnUpdateEditEnablemidi(CCmdUI *pCmdUI)
{
	if (theApp.GetMIDI()->IsAvailable())
		pCmdUI->Enable(1);
	else
		pCmdUI->Enable(0);

	if (theApp.GetMIDI()->IsOpened())
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}

void CFamiTrackerView::OnFrameInsert()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int i, x;

	if (pDoc->m_iFrameCount == MAX_FRAMES) {
		return;
	}

	pDoc->m_iFrameCount++;

	for (x = signed(pDoc->m_iFrameCount); x > (m_iCurrentFrame - 1); x--) {
		for (i = 0; i < pDoc->m_iChannelsAvailable; i++) {
			pDoc->FrameList[x + 1][i] = pDoc->FrameList[x][i];
		}
	}

	for (i = 0; i < pDoc->m_iChannelsAvailable; i++) {
		pDoc->FrameList[m_iCurrentFrame][i] = 0;
	}
}

void CFamiTrackerView::OnFrameRemove()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int i, x;

	if (pDoc->m_iFrameCount == 1) {
		return;
	}

	for (i = 0; i < pDoc->m_iChannelsAvailable; i++) {
		pDoc->FrameList[m_iCurrentFrame][i] = 0;
	}

	for (x = m_iCurrentFrame; x < pDoc->m_iFrameCount; x++) {
		for (i = 0; i < pDoc->m_iChannelsAvailable; i++) {
			pDoc->FrameList[x][i] = pDoc->FrameList[x + 1][i];
		}
	}

	pDoc->m_iFrameCount--;

	if (m_iCurrentFrame > (signed(pDoc->m_iFrameCount) - 1))
		m_iCurrentFrame = (pDoc->m_iFrameCount - 1);
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

	for (int i = 0; i < pDoc->m_iChannelsAvailable; i++) {
		pDoc->GetNoteData(&Note, i, m_iCurrentRow, m_iCurrentFrame);
		if (!m_bMuteChannels[i])
			FeedNote(i, &Note);
	
	}
	ScrollRowDown();
}

void CFamiTrackerView::OnUpdateFrameRemove(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	pCmdUI->Enable(pDoc->m_iFrameCount > 1);
}

void CFamiTrackerView::RefreshStatusMessage()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	int Effect, Param;
	CString Text;
	bool IsEffect = false;

	if (m_iSelectStart != -1 || m_bPlaying)
		return;

	switch (m_iCursorColumn) {
		case 0:
			Text = "Note column";
			break;
		case 1:
		case 2:
			Text = "Instrument column";
			break;
		case 3:
			Text = "Volume column";
			break;
		case 4:		// First
		case 5:
		case 6:
			Effect		= pDoc->GetNoteData(m_iCursorChannel, m_iCurrentRow, m_iCurrentFrame)->EffNumber[0];
			Param		= pDoc->GetNoteData(m_iCursorChannel, m_iCurrentRow, m_iCurrentFrame)->EffParam[0];
			IsEffect	= true;
			break;
		case 7:		// Second
		case 8:
		case 9:
			Effect		= pDoc->GetNoteData(m_iCursorChannel, m_iCurrentRow, m_iCurrentFrame)->EffNumber[1];
			Param		= pDoc->GetNoteData(m_iCursorChannel, m_iCurrentRow, m_iCurrentFrame)->EffParam[1];
			IsEffect	= true;
			break;
		case 10:	// Third
		case 11:
		case 12:
			Effect		= pDoc->GetNoteData(m_iCursorChannel, m_iCurrentRow, m_iCurrentFrame)->EffNumber[2];
			Param		= pDoc->GetNoteData(m_iCursorChannel, m_iCurrentRow, m_iCurrentFrame)->EffParam[2];
			IsEffect	= true;
			break;
		case 13:	// Fourth
		case 14:
		case 15:
			Effect		= pDoc->GetNoteData(m_iCursorChannel, m_iCurrentRow, m_iCurrentFrame)->EffNumber[3];
			Param		= pDoc->GetNoteData(m_iCursorChannel, m_iCurrentRow, m_iCurrentFrame)->EffParam[3];
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
			default:
				Text = "Effect column";
		}
	}

	SetMessageText(Text);
}

void CFamiTrackerView::FeedNote(int Channel, stChanNote *NoteData)
{
	CFamiTrackerDoc* pDoc = GetDocument();

	CurrentNotes[Channel]	= *NoteData;
	NewNoteData[Channel]	= true;
	EffectColumns[Channel]	= pDoc->m_iEffectColumns[Channel] + 1;
}

void CFamiTrackerView::GetStartSpeed()
{
	CFamiTrackerDoc* pDoc = GetDocument();

	m_iTempoAccum	= 60 * pDoc->GetFrameRate();		// 60 as in 60 seconds per minute
	m_iTickPeriod	= pDoc->m_iSongSpeed;
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
