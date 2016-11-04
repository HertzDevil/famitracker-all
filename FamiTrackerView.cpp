/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005  Jonathan Liss
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
#include ".\famitrackerview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define GET_CHANNEL(x)	((x - LEFT_OFFSET - 40) / COLUMN_WIDTH)
#define GET_COLUMN(x)	((x - LEFT_OFFSET - 40) - (GET_CHANNEL(x) * COLUMN_WIDTH))

const int TEXT_POSITION		= 10;
const int TOP_OFFSET		= 42;
const int LEFT_OFFSET		= 10;
const int ROW_HEIGHT		= 18;
const int COLUMN_WIDTH		= 150;
const int COLUMNS			= 5;

const int CHAR_WIDTH		= 12;

const int COLUMN_START[] = {0, 40, 68, 84, 96};
const int COLUMN_SPACE[] = {CHAR_WIDTH * 3, CHAR_WIDTH * 2, CHAR_WIDTH * 1, CHAR_WIDTH, CHAR_WIDTH * 2};

enum eCOLUMNS {C_NOTE, C_INSTRUMENT, C_VOLUME, C_EFF_NUM, C_EFF_PARAM};

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
	ON_COMMAND(ID_FILE_MIDISETTINGS, OnFileMidisettings)
	ON_WM_TIMER()
	ON_WM_CREATE()
	ON_COMMAND(ID_EDIT_ENABLEMIDI, OnEditEnablemidi)
	ON_UPDATE_COMMAND_UI(ID_EDIT_ENABLEMIDI, OnUpdateEditEnablemidi)
	ON_COMMAND(ID_FRAME_INSERT, OnFrameInsert)
	ON_COMMAND(ID_FRAME_REMOVE, OnFrameRemove)
	ON_COMMAND(ID_TRACKER_PLAYROW, OnTrackerPlayrow)
END_MESSAGE_MAP()

CMIDI MIDI;

struct stClipData {
	int			Size;
	stChanNote	Data[MAX_PATTERN_LENGTH];
};

bool IgnoreFirst;

int m_iKeyboardNote;

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

	m_iSelectStart		= -1;
	m_iSelectEnd		= 0;
	m_iSelectChannel	= 0;

	m_iOctave			= 3;
	m_iInstrument		= 0;

	m_iUndoLevel		= 0;
	m_iRedoLevel		= 0;

	m_iKeyboardNote		= -1;

	for (i = 0; i < 5; i++)
		m_bMuteChannels[i] = false;

	for (i = 0; i < 256; i++)
		m_cKeyList[i] = 0;
}

CFamiTrackerView::~CFamiTrackerView()
{
	MIDI.CloseDevice();
}

BOOL CFamiTrackerView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

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

	const int VERY_DARK_RED	= 0x000068;
	const int DARK_RED		= 0x0000B8;
	const int LIGHT_RED		= 0x1013FF;

	LOGFONT LogFont;
	CBitmap Bmp, *OldBmp;
	CFont	Font, *OldFont;
	CPen	Pen, BlackPen, SelectPen, *OldPen;
	CBrush	Brush, SelectBrush, *OldBrush;

	int TrackerFieldRows = pDoc->m_iPatternLength;
	int ChanOffset;
	int i, y, Nr;

	char Text[256];

	static DWORD LastTime;

	LPCSTR FontName = theApp.m_strFont;

	if ((GetTickCount() - LastTime) < 30 && m_bPlaying)
		return;

	LastTime = GetTickCount();

	if (!pDoc)
		return;

	if (!pDoc->m_bFileLoaded) {
		pDC->Rectangle(0, 0, WindowWidth, WindowHeight);
		pDC->TextOut(5, 5, "No document is loaded");
		return;
	}

	if (!theApp.m_bFreeCursorEdit)
		m_iCursorRow = m_iCurrentRow;

	if (m_iCurrentRow > (TrackerFieldRows - 1))
		m_iCurrentRow = (TrackerFieldRows - 1);

	dcBack = new CDC;

	Bmp.CreateCompatibleBitmap(pDC, WindowWidth, WindowHeight);
	dcBack->CreateCompatibleDC(pDC);
	OldBmp = dcBack->SelectObject(&Bmp);

	memset(&LogFont, 0, sizeof LOGFONT);
	memcpy(LogFont.lfFaceName, FontName, strlen(FontName));

	LogFont.lfHeight = -12; //-12
	//LogFont.lfWeight = FW_BOLD;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;

	Brush.CreateSolidBrush(0xFF000000);

	Font.CreateFontIndirect(&LogFont);
	BlackPen.CreatePen(0, 5, 0xFF000000);

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

	dcBack->Rectangle(0, 0, WindowWidth - 23, WindowHeight - 5);

	dcBack->SelectObject(&BlackPen);
	dcBack->SetBkColor(COLOR_SCHEME.BACKGROUND);
	
	if (m_iLastRowState > (VisibleRows / 2))
		Nr = m_iLastRowState - VisibleRows / 2;
	else
		Nr = 0;

	if (m_iCurrentFrame < 0)
		m_iCurrentFrame = 0;

	dcBack->Rectangle(LEFT_OFFSET + 35, 9, LEFT_OFFSET + 40 + 120, 160);

	dcBack->SetTextColor(m_bMuteChannels[0] ? COLOR_SCHEME.TEXT_MUTED : COLOR_SCHEME.TEXT_HILITE);
	dcBack->TextOut(LEFT_OFFSET + 40, TEXT_POSITION, "Square 1");

	dcBack->SetTextColor(m_bMuteChannels[1] ? COLOR_SCHEME.TEXT_MUTED : COLOR_SCHEME.TEXT_HILITE);	
	dcBack->TextOut(LEFT_OFFSET + 40 + COLUMN_WIDTH * 1, TEXT_POSITION, "Square 2");

	dcBack->SetTextColor(m_bMuteChannels[2] ? COLOR_SCHEME.TEXT_MUTED : COLOR_SCHEME.TEXT_HILITE);	
	dcBack->TextOut(LEFT_OFFSET + 40 + COLUMN_WIDTH * 2, TEXT_POSITION, "Triangle");
	
	dcBack->SetTextColor(m_bMuteChannels[3] ? COLOR_SCHEME.TEXT_MUTED : COLOR_SCHEME.TEXT_HILITE);	
	dcBack->TextOut(LEFT_OFFSET + 40 + COLUMN_WIDTH * 3, TEXT_POSITION, "Noise");

	dcBack->SetTextColor(m_bMuteChannels[4] ? COLOR_SCHEME.TEXT_MUTED : COLOR_SCHEME.TEXT_HILITE);	
	dcBack->TextOut(LEFT_OFFSET + 40 + COLUMN_WIDTH * 4, TEXT_POSITION, "DPCM");

	SelectBrush.CreateSolidBrush(0x00FF0000);
	SelectPen.CreatePen(0, 0, 0x00FF0000);

	dcBack->SelectObject(&SelectBrush);
	dcBack->SelectObject(&SelectPen);

	int SelStart	= m_iSelectStart;
	int SelEnd		= m_iSelectEnd;

	if (SelStart > SelEnd) {
		SelStart	= m_iSelectEnd;
		SelEnd		= m_iSelectStart;
	}

	for (y = 0; y < VisibleRows; y++) {
		if ((m_iLastRowState - (VisibleRows / 2) + y) >= 0 && 
			(m_iLastRowState - (VisibleRows / 2) + y) < TrackerFieldRows) {

			if ((Nr & 3) == 0)
				dcBack->SetTextColor(COLOR_SCHEME.TEXT_HILITE);
			else
				dcBack->SetTextColor(COLOR_SCHEME.TEXT_NORMAL);

			dcBack->SetBkColor(0x000000);

			sprintf(Text, "%03i", Nr);
			dcBack->TextOut(LEFT_OFFSET + 2, TOP_OFFSET + y * ROW_HEIGHT, Text, (int)strlen(Text));

			for (i = 0; i < pDoc->m_iChannelsAvaliable; i++) {
				if (Nr >= SelStart && Nr <= SelEnd && i == m_iSelectChannel && m_iSelectStart != -1) {
					int SelStart	= TOP_OFFSET + ROW_HEIGHT * (Nr + ((VisibleRows / 2) - m_iLastRowState));
					int SelEnd		= SelStart + ROW_HEIGHT - 1;
					int SelChan		= LEFT_OFFSET + 36 + (m_iSelectChannel * COLUMN_WIDTH);
					dcBack->Rectangle(SelChan, SelStart - 1, SelChan + COLUMN_WIDTH - 25, SelEnd);
					dcBack->SetBkColor(0xFF0000);
				}
				else
					dcBack->SetBkColor(0x000000);

				DrawLine(pDoc->GetPatternData(m_iCurrentFrame, Nr, i), y, i);
			}
		Nr++;
		}
	}

	// Selected row
	dcBack->Draw3dRect(7, TOP_OFFSET + (VisibleRows / 2) * ROW_HEIGHT - 2, WindowWidth - 35, ROW_HEIGHT + 2, VERY_DARK_RED, VERY_DARK_RED);
	dcBack->Draw3dRect(8, TOP_OFFSET + (VisibleRows / 2) * ROW_HEIGHT - 1, WindowWidth - 37, ROW_HEIGHT, DARK_RED, DARK_RED);
	dcBack->Draw3dRect(9, TOP_OFFSET + (VisibleRows / 2) * ROW_HEIGHT, WindowWidth - 39, ROW_HEIGHT - 2, LIGHT_RED, LIGHT_RED);

	ChanOffset = m_iCursorChannel * COLUMN_WIDTH + LEFT_OFFSET + 40;

	if (m_iCursorRow >= (m_iCurrentRow - (VisibleRows / 2)) && m_iCursorRow <= (m_iCurrentRow + (VisibleRows / 2))) {

		int CursorOffset = TOP_OFFSET + ROW_HEIGHT * (m_iCursorRow + ((VisibleRows / 2) - m_iCurrentRow));
		// Selected column
		dcBack->Draw3dRect(ChanOffset + COLUMN_START[m_iCursorColumn] - 3, CursorOffset, COLUMN_SPACE[m_iCursorColumn] + 4, ROW_HEIGHT - 2, LIGHT_RED, LIGHT_RED); 
		dcBack->Draw3dRect(ChanOffset + COLUMN_START[m_iCursorColumn] - 4, CursorOffset - 1, COLUMN_SPACE[m_iCursorColumn] + 6, ROW_HEIGHT, DARK_RED, DARK_RED); 
	}

	DrawMeters(dcBack);

	pDC->BitBlt(0, 0, WindowWidth, WindowHeight, dcBack, 0, 0, SRCCOPY);

	dcBack->SelectObject(OldBmp);

	dcBack->SelectObject(OldBrush);
	dcBack->SelectObject(OldFont);
	dcBack->SelectObject(OldPen);

	SetScrollRange(SB_VERT, 0, TrackerFieldRows - 1);
	SetScrollPos(SB_VERT, m_iCurrentRow);

	delete dcBack;

	m_bRedrawed = true;
}

void CFamiTrackerView::DrawLine(stChanNote *NoteData, int Line, int Channel)
{
	const char NOTES_A[] = {'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'};
	const char NOTES_B[] = {'-', '#', '-', '#', '-', '-', '#', '-', '#', '-', '#', '-'};
	const char NOTES_C[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	const char HEX[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

	int LineY		= TOP_OFFSET + Line * ROW_HEIGHT;
	int OffsetX		= LEFT_OFFSET + 40 + Channel * COLUMN_WIDTH;

	int Vol			= NoteData->Vol;
	int Note		= NoteData->Note;
	int Octave		= NoteData->Octave;
	int Instrument	= NoteData->Instrument;
	int ExtraStuff1 = NoteData->ExtraStuff1;
	int ExtraStuff2 = NoteData->ExtraStuff2;

	switch (Note) {
		case 0:
		case HALT:
			if (Note == 0) {
				DrawChar(OffsetX + COLUMN_START[0], LineY, '-');
				DrawChar(OffsetX + COLUMN_START[0] + CHAR_WIDTH, LineY, '-');
				DrawChar(OffsetX + COLUMN_START[0] + CHAR_WIDTH * 2, LineY, '-');
			}
			else 
			{
				DrawChar(OffsetX + COLUMN_START[0], LineY, '*');
				DrawChar(OffsetX + COLUMN_START[0] + CHAR_WIDTH, LineY, '*');
				DrawChar(OffsetX + COLUMN_START[0] + CHAR_WIDTH * 2, LineY, '*');
			}
			DrawChar(OffsetX + COLUMN_START[1], LineY, '-');
			DrawChar(OffsetX + COLUMN_START[1] + CHAR_WIDTH, LineY, '-');
			break;
		default:
			DrawChar(OffsetX + COLUMN_START[0], LineY, NOTES_A[Note - 1]);
			DrawChar(OffsetX + COLUMN_START[0] + CHAR_WIDTH, LineY, NOTES_B[Note - 1]);
			DrawChar(OffsetX + COLUMN_START[0] + CHAR_WIDTH * 2, LineY, NOTES_C[Octave]);

			if (Instrument == MAX_INSTRUMENTS) {
				DrawChar(OffsetX + COLUMN_START[1], LineY, '-');
				DrawChar(OffsetX + COLUMN_START[1] + CHAR_WIDTH, LineY, '-');
			}
			else {
				DrawChar(OffsetX + COLUMN_START[1], LineY, (Instrument / 10) + '0');
				DrawChar(OffsetX + COLUMN_START[1] + CHAR_WIDTH, LineY, (Instrument % 10) + '0');
			}
	}

	// Volume

	if (Vol == 0)
		DrawChar(OffsetX + COLUMN_START[2], LineY, '-');
	else
		DrawChar(OffsetX + COLUMN_START[2], LineY, HEX[(Vol - 1) & 0x0F]);

	// Effects
	if (ExtraStuff1 == 0) {
		DrawChar(OffsetX + COLUMN_START[3], LineY, '-');
		DrawChar(OffsetX + COLUMN_START[3] + CHAR_WIDTH, LineY, '-');
		DrawChar(OffsetX + COLUMN_START[3] + CHAR_WIDTH * 2, LineY, '-');
	}
	else {
		DrawChar(OffsetX + COLUMN_START[3], LineY, EFF_CHAR[ExtraStuff1 - 1]);
		DrawChar(OffsetX + COLUMN_START[3] + CHAR_WIDTH, LineY, HEX[ExtraStuff2 >> 4]);
		DrawChar(OffsetX + COLUMN_START[3] + CHAR_WIDTH * 2, LineY, HEX[ExtraStuff2 & 0x0F]);
		/*
		DrawChar(OffsetX + COLUMN_START[3], LineY, EFF_CHAR[ExtraStuff1 - 1]);
		DrawChar(OffsetX + COLUMN_START[3] + CHAR_WIDTH, LineY, (ExtraStuff2 / 10) + '0');
		DrawChar(OffsetX + COLUMN_START[3] + CHAR_WIDTH * 2, LineY, (ExtraStuff2 % 10) + '0');
		*/
	}
}

void CFamiTrackerView::DrawChar(int x, int y, char c) {	
	static CString Text;
	Text.Format("%c", c);
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
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	return FALSE; // CView::OnEraseBkgnd(pDC);
}

void CFamiTrackerView::CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType)
{
	WindowWidth		= lpClientRect->right - lpClientRect->left;
	WindowHeight	= lpClientRect->bottom - lpClientRect->top;
	VisibleRows		= (((WindowHeight - 20) + (ROW_HEIGHT / 2)) / ROW_HEIGHT) - 2;
	CView::CalcWindowRect(lpClientRect, nAdjustType);
}

BOOL CFamiTrackerView::PreTranslateMessage(MSG* pMsg)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	switch (pMsg->message) {
		case WM_USER:
			PlaybackTick();
			break;
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
	const int BAR_LEFT		= LEFT_OFFSET + 38;
	const int BAR_SIZE		= 8;
	const int BAR_SPACE		= 1;
	const int BAR_HEIGHT	= 5;

	unsigned int i, c;

	if (!pDoc->m_bFileLoaded)
		return;

	Black.CreateSolidBrush(0xFF000000);
	Green.CreatePen(0, 0, /*0x00FF00*/ 0x106060);

	GreenBrush.CreateSolidBrush(0x00F000);
	GreyBrush.CreateSolidBrush(0x608060);

	OldBrush = pDC->SelectObject(&Black);
	OldPen = pDC->SelectObject(&Green);

	for (c = 0; c < 5; c++) {		
		for (i = 0; i < 15; i++) {
			if (i < m_iVolLevels[c])
				pDC->FillRect(CRect(BAR_LEFT + COLUMN_WIDTH * c + (i * BAR_SIZE), BAR_TOP, BAR_LEFT + COLUMN_WIDTH * c + (i * BAR_SIZE) + (BAR_SIZE - BAR_SPACE), BAR_TOP + BAR_HEIGHT), &GreenBrush);
			else
				pDC->FillRect(CRect(BAR_LEFT + COLUMN_WIDTH * c + (i * BAR_SIZE), BAR_TOP, BAR_LEFT + COLUMN_WIDTH * c + (i * BAR_SIZE) + (BAR_SIZE - BAR_SPACE), BAR_TOP + BAR_HEIGHT), &GreyBrush);
		}
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
	TickPeriod = Speed;
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

	TickPeriod = Speed;

	pDoc->SetSongSpeed(Speed);

}

void CFamiTrackerView::PlayPattern()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (!m_bPlaying) {
		m_bPlaying		= true;
		m_bPlayLooped	= true;	
		PlayerSyncTick	= 0;
		TickPeriod		= pDoc->m_iSongSpeed;

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

void CFamiTrackerView::PlaybackTick(void)
{
	// Called 50/60 (or user) times per second
	// Divide the time here to get the real play speed

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CMainFrame	*pMainFrm = (CMainFrame*)GetParentFrame();

	static bool	Jump = false;
	static bool	Skip = false;
	static int	JumpToPattern;
	static int	SkipToRow;
	int TicksPerSec;

	TicksPerSec = pDoc->m_iEngineSpeed;

	if (TicksPerSec == 0) {
		if (pDoc->m_iMachine == NTSC)
			TicksPerSec = 60;
		else
			TicksPerSec = 50;
	}

	m_iCurrentTempo = (int)((60 / double((TickPeriod + 1) * 4)) * TicksPerSec);

	if (!m_bPlaying) {
		theApp.SendBackSyncTick();
		return;
	}

	m_iPlayTime++;

	if ((m_iPlayTime & 0x01) == 0x01)
		pMainFrm->SetIndicatorTime((m_iPlayTime / TicksPerSec) / 60, (m_iPlayTime / TicksPerSec) % 60, ((m_iPlayTime * 10) / TicksPerSec) % 10);

	if (!PlayerSyncTick) {

		Jump = false;
		Skip = false;

		PlayRow(m_iCurrentRow, m_iCurrentFrame);

		// Evaluate effects
		for (int i = 0; i < pDoc->m_iChannelsAvaliable; i++) {
			switch (pDoc->GetNoteEffect(i, m_iCurrentRow, m_iCurrentFrame)) {
				// Axx Sets speed ticks to xx
				case EF_SPEED:
					TickPeriod = pDoc->GetNoteEffectValue(i, m_iCurrentRow, m_iCurrentFrame);
					break;
				// Bxx Jump to pattern xx
				case EF_JUMP:
					Jump = true;
					JumpToPattern = pDoc->GetNoteEffectValue(i, m_iCurrentRow, m_iCurrentFrame) - 1;
					//m_iCurrentRow = -1;
					if (m_bPlayLooped)
						Jump = false;
						//TogglePlayback();
					break;
				// Cxx Skip to next track and start at row xx
				case EF_SKIP:
					Skip = true;
					SkipToRow = pDoc->GetNoteEffectValue(i, m_iCurrentRow, m_iCurrentFrame);
					if (m_bPlayLooped)
						Skip = false;
					break;
				// Dxx Halt playback
				case EF_HALT:
					TogglePlayback();
					break;
			}
		}
	}
	if (PlayerSyncTick >= TickPeriod) {

		if (Jump) {
			m_iCurrentFrame = JumpToPattern;
			m_iCurrentRow = -1;
			if (m_iCurrentFrame > (pDoc->m_iFrameCount - 1))
				m_iCurrentFrame = 0;
		}

		if (Skip) {
			if (SkipToRow > 0)
				SelectNextPattern();
			m_iCurrentRow = SkipToRow - 1;
		}

		PlayerSyncTick = 0;
		ScrollRowDown();

		if (m_iCurrentRow == 0) {
			// Change pattern
			ScrollToTop();
			if (!m_bPlayLooped)
				SelectNextPattern();
		}
	}
	else
		PlayerSyncTick++;

	theApp.SendBackSyncTick();
}

void CFamiTrackerView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	switch (nSBCode) {
		case SB_ENDSCROLL:
			return;
		case SB_LINEDOWN:
			ScrollRowDown();
			break;
		case SB_LINEUP:
			ScrollRowUp();
			break;
		case SB_PAGEDOWN:
			ScrollPageDown();
			break;
		case SB_PAGEUP:
			ScrollPageUp();
			break;
		case SB_TOP:
			ScrollToTop();
			break;
		case SB_BOTTOM:
			ScrollToBottom();
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			m_iCurrentRow = nPos;
			ForceRedraw();
			break;
	}

	CView::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CFamiTrackerView::UpdateTrackerView()
{
//	m_bRedrawed = false;
//	Invalidate(FALSE);
//	PostMessage(WM_PAINT);
//	((CMainFrame*)GetParent())->RefreshPattern();

//	m_iLastFrameState = !m_iCurrentFrame;
}

void CFamiTrackerView::WrapSelectedLine()		// or row
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (theApp.m_bWrapCursor || m_bPlaying) {
		if (m_iCurrentRow < 0) {
			m_iCurrentRow += pDoc->m_iPatternLength;
			m_iCursorRow += pDoc->m_iPatternLength;
		}
		else if (m_iCurrentRow > (pDoc->m_iPatternLength - 1)) {
			m_iCurrentRow -= pDoc->m_iPatternLength;
			m_iCursorRow -= pDoc->m_iPatternLength;
		}
	}

	if (/*m_iCursorRow < 0 ||*/ m_iCurrentRow < 0) {
		m_iCursorRow = 0;
		m_iCurrentRow = 0;
	}
	else if (/*m_iCursorRow > (pDoc->m_iPatternLength - 1) ||*/ m_iCurrentRow > (pDoc->m_iPatternLength - 1)) {
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
	if (/*!LockCursor*/ theApp.m_bFreeCursorEdit) {
		m_iCursorRow--;
		if ((m_iCursorRow - m_iCurrentRow) < -(VisibleRows / 2))
			m_iCurrentRow--;
	}
	else
		m_iCurrentRow--; 
	WrapSelectedLine();
	ForceRedraw();
}

void CFamiTrackerView::ScrollRowDown()
{
	if (/*!LockCursor*/ theApp.m_bFreeCursorEdit && !m_bPlaying) {
		m_iCursorRow++;
		if ((m_iCursorRow - m_iCurrentRow) > (VisibleRows / 2))
			m_iCurrentRow++;
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

void CFamiTrackerView::MoveCursorLeft()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_iCursorColumn > 0) {
		m_iCursorColumn--;
	}
	else {
		if (m_iCursorChannel > 0) {
			m_iCursorColumn = COLUMNS - 1;
			m_iCursorChannel--;
		}
		else {
			if (theApp.m_bWrapCursor) {
				m_iCursorColumn = COLUMNS - 1;
				m_iCursorChannel = pDoc->m_iChannelsAvaliable - 1;
			}
		}
	}
}

void CFamiTrackerView::MoveCursorRight()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_iCursorColumn < (COLUMNS - 1)) {
		m_iCursorColumn++;
	}
	else {
		if (m_iCursorChannel < (pDoc->m_iChannelsAvaliable - 1)) {
			m_iCursorColumn = 0;
			m_iCursorChannel++;
		}
		else {
			if (theApp.m_bWrapCursor) {
				m_iCursorColumn = 0;
				m_iCursorChannel = 0;
			}
		}
	}
}

void CFamiTrackerView::MoveCursorNextChannel()
{
	m_iCursorChannel++;
	m_iCursorColumn = 0;

	if (m_iCursorChannel > (GetDocument()->m_iChannelsAvaliable - 1)) {
		m_iCursorChannel = 0;
	}

	ForceRedraw();
}

void CFamiTrackerView::MoveCursorPrevChannel()
{

	m_iCursorChannel--;
	m_iCursorColumn = 0;

	if (m_iCursorChannel < 0) {
		m_iCursorChannel = GetDocument()->m_iChannelsAvaliable - 1;
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
	m_iCursorChannel	= pDoc->m_iChannelsAvaliable - 1;
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
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_iCurrentFrame++;

	WrapSelectedPattern();
	ForceRedraw();
}

void CFamiTrackerView::SelectPrevPattern()
{
	m_iCurrentFrame--;
	WrapSelectedPattern();
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
		for (int i = 0; i < pDoc->m_iChannelsAvaliable; i++) {
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
		for (int i = 0; i < pDoc->m_iChannelsAvaliable; i++) {
			pDoc->DecreaseFrame(m_iCurrentFrame, i);
		}
	}
	else
		pDoc->DecreaseFrame(m_iCurrentFrame, m_iCursorChannel);

	ForceRedraw();
}

void CFamiTrackerView::PlayRow(int RowPos, int Frame)
{
	// Play all channels in a row
	//

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	for (int i = 0; i < pDoc->m_iChannelsAvaliable; i++) {
		if (!m_bMuteChannels[i])
			pDoc->PlayNote(i, RowPos, Frame);
	}
}

void CFamiTrackerView::StopNote(int Channel)
{
	stChanNote Note;

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	Note.Instrument		= 0;
	Note.Octave			= 0;
	Note.Note			= HALT;
	Note.ExtraStuff1	= 0;
	Note.ExtraStuff2	= 0;
	Note.Vol			= 0;

	if (m_bEditEnable) {
		AddUndo(Channel);
		pDoc->ChannelData[Channel][pDoc->FrameList[m_iCurrentFrame][Channel]][m_iCursorRow] = Note;
		pDoc->SetModifiedFlag();
		RedrawWindow();
	}

	theApp.PlayNote(Channel, &Note);
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
	else
		Cell.Vol = 0;

	if (m_bEditEnable) {
		AddUndo(Channel);
		pDoc->ChannelData[Channel][pDoc->FrameList[m_iCurrentFrame][Channel]][m_iCursorRow] = Cell;
		pDoc->SetModifiedFlag();
		RedrawWindow();

		if (m_iCursorColumn == 0 && m_bPlaying == false) {
			StepDown();
		}
	}

	theApp.PlayNote(Channel, &Cell);
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

void CFamiTrackerView::InterpretKey(int Key)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	static bool FirstChar = false;

	stChanNote	Note;

	Note = pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow];

	if (PreventRepeat(Key))
		return;

	int Value;

	switch (m_iCursorColumn) {
		case C_NOTE:
			TranslateKey(Key, &Note);
			return;
		case C_INSTRUMENT:
			if (m_bEditEnable && Note.Note != 0) {
				if (Key > 47 && Key < 58) {
					
					if (!FirstChar)
						Note.Instrument = (Key - 48) * 10;
					else
						Note.Instrument += (Key - 48);

					if (Note.Instrument > (MAX_INSTRUMENTS - 1))
						Note.Instrument = MAX_INSTRUMENTS - 1;
					FirstChar = !FirstChar;
				}
				else
					FirstChar = false;
			}
			break;
		case C_VOLUME:
			Value = ConvertKeyToHex(Key);
			if (Value != -1 && m_bEditEnable) {
				Note.Vol = Value + 1;
			}
			break;
		case C_EFF_NUM:
			if (m_bEditEnable) {
				switch (Key) {
					case 65: Note.ExtraStuff1 = EF_SPEED;		break;		// A
					case 66: Note.ExtraStuff1 = EF_JUMP;		break;		// B
					case 67: Note.ExtraStuff1 = EF_SKIP;		break;		// C
					case 68: Note.ExtraStuff1 = EF_HALT;		break;		// D
					case 69: Note.ExtraStuff1 = EF_VOLUME;		break;		// E
					case 70: Note.ExtraStuff1 = EF_PORTAON;		break;		// F
					case 71: Note.ExtraStuff1 = EF_PORTAOFF;	break;		// G
					case 72: Note.ExtraStuff1 = EF_SWEEPUP;		break;		// H
					case 73: Note.ExtraStuff1 = EF_SWEEPDOWN;	break;		// I
				}
			}
			break;
		case C_EFF_PARAM:
			Value = ConvertKeyToHex(Key);
			if (!FirstChar)
				Note.ExtraStuff2 = Value << 4;
			else
				Note.ExtraStuff2 |= Value;
			FirstChar = !FirstChar;
			break;
	}

	if (m_bEditEnable) {
		AddUndo(m_iCursorChannel);
		pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow] = Note;
		pDoc->SetModifiedFlag();

		if (m_iCursorColumn == C_VOLUME)
			StepDown();

		RedrawWindow();
	}
}

bool CFamiTrackerView::PreventRepeat(int Key)
{
	if (m_cKeyList[Key] == 0)
		m_cKeyList[Key] = 1;
	else {
		if ((!theApp.m_bKeyRepeat || !m_bEditEnable) /*&& m_iCursorColumn == 0*/)
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

		Note.Instrument = 0;
		Note.Octave = 0;
		Note.Note = HALT;
		Note.ExtraStuff1 = 0;
		Note.ExtraStuff2 = 0;
		
		theApp.PlayNote(m_iCursorChannel, &Note);
	}
}

void CFamiTrackerView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{	
	CFamiTrackerDoc* pDoc = GetDocument();
	int i;

	ASSERT_VALID(pDoc);

	switch (nChar) {
		case VK_UP:		ScrollRowUp();		break;
		case VK_DOWN:	ScrollRowDown();	break;
		case VK_HOME:	ScrollToTop();		break;
		case VK_END:	ScrollToBottom();	break;
		case VK_PRIOR:	ScrollPageUp();		break;
		case VK_NEXT:	ScrollPageDown();	break;

		case VK_SPACE:
			OnTrackerEdit(); 
			break;

		case VK_ADD:
			if (m_bEditEnable) {
				AddUndo(m_iCursorChannel);
				switch (m_iCursorColumn) {
					case 0: IncreaseCurrentFrame(); break;
					case 1: pDoc->IncreaseInstrument(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow); 	ForceRedraw(); break;
					case 2: pDoc->IncreaseVolume(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow);	ForceRedraw(); break;
					case 3:
					case 4: pDoc->IncreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow); 	ForceRedraw(); break;
				}
			}
			break;

		case VK_SUBTRACT:
			if (m_bEditEnable) {
				AddUndo(m_iCursorChannel);
				switch (m_iCursorColumn) {
					case 0: DecreaseCurrentFrame(); break;
					case 1: pDoc->DecreaseInstrument(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow); 	ForceRedraw(); break;
					case 2: pDoc->DecreaseVolume(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow);		ForceRedraw(); break;
					case 3:
					case 4: pDoc->DecreaseEffect(m_iCurrentFrame, m_iCursorChannel, m_iCursorRow); 		ForceRedraw(); break;
				}
			}
			break;

		case VK_LEFT:
			if (!m_bShiftPressed) {
				MoveCursorLeft();
				ForceRedraw();
			}
			break;

		case VK_RIGHT:
			if (!m_bShiftPressed) {
				MoveCursorRight();
				ForceRedraw();
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
					//ScrollRowDown();
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

		case 255:
			break;

		default:
			InterpretKey(nChar);
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CFamiTrackerView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (nChar == VK_SHIFT) {
		m_bShiftPressed = false;
	}

	KeyReleased(int(nChar));

	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

BOOL CFamiTrackerView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: Add your message handler code here and/or call default

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (zDelta > 0) {
		m_iCurrentRow -= zDelta / 28;
		if (m_iCurrentRow < 0)
			m_iCurrentRow = 0;
	}
	else if (zDelta < 0) {
		m_iCurrentRow -= zDelta / 28;
		if (m_iCurrentRow > (pDoc->m_iPatternLength - 1))
			m_iCurrentRow = (pDoc->m_iPatternLength - 1);
	}

	ForceRedraw();

	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void CFamiTrackerView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Channel, Column, Row, i, DeltaY;
	int RowsInWindow = (WindowHeight / ROW_HEIGHT) - 2;

	IgnoreFirst = true;

	Channel = GET_CHANNEL(point.x);
	Column	= GET_COLUMN(point.x);
	
	if (Channel < 0)
		Channel = 0;
	if (Column < 0)
		Column = 0;

	DeltaY = point.y - (WindowHeight / 2) - 10;

	if (DeltaY < 0)
		Row = DeltaY / ROW_HEIGHT - 1;
	else
		Row = DeltaY / ROW_HEIGHT;

	for (i = COLUMNS - 1; i > -1; i--) {
		if (Column > COLUMN_START[i]) {
			Column = i;
			break;
		}
	}

	if (point.y < ROW_HEIGHT * 2 && Channel < pDoc->m_iChannelsAvaliable) {
		m_bMuteChannels[Channel] = !m_bMuteChannels[Channel];
		ForceRedraw();
		if (m_bPlaying) {
			stChanNote Note;
			Note.Note		= HALT;
			Note.Instrument = 0;
			Note.Octave		= 0;
			theApp.PlayNote(Channel, &Note);
		}
	}

	int Sel = m_iCurrentRow + Row;

	if (m_iSelectStart != -1) {
		if (m_bShiftPressed) {
			m_iSelectEnd = Sel;
			ForceRedraw();
		}
		else {
			m_iSelectStart = -1;
			m_iSelectEnd = 0;
			ForceRedraw();
		}
	}

	if (Channel >= 0 && Channel < pDoc->m_iChannelsAvaliable && Row == 0 && point.y > ROW_HEIGHT * 2) {
		m_iCursorChannel = Channel;
		m_iCursorColumn = Column;
		ForceRedraw();
	}

	CView::OnLButtonDown(nFlags, point);
}

void CFamiTrackerView::OnMouseMove(UINT nFlags, CPoint point)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
	CString String;
	int DeltaY = point.y - (WindowHeight / 2) - 10;
	int Row, Channel;

	if (!(nFlags & MK_LBUTTON))
		return;

	if (IgnoreFirst) {
		IgnoreFirst = false;
		return;
	}

	Channel = (point.x - 50) / 130;

	if (DeltaY < 0)
		Row = DeltaY / ROW_HEIGHT - 1;
	else
		Row = DeltaY / ROW_HEIGHT;
	
	int Sel = m_iCurrentRow + Row;

	if (((Row + m_iCurrentRow) >= 0 && (Row + m_iCurrentRow) < pDoc->m_iPatternLength)) {

		if (m_iSelectStart != -1) {
			m_iSelectEnd = Sel;
			RedrawWindow();
		}
		else {
			m_iSelectStart = Sel;
			m_iSelectChannel = Channel;
		}

		if (Row >= ((WindowHeight / ROW_HEIGHT) / 2) - 1)
			ScrollRowDown();
		else if (Row <= -((WindowHeight / ROW_HEIGHT) / 2) - 1)
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
	int			SelStart, SelEnd;

	if (m_iSelectStart == -1)
		return;

	if (m_iSelectStart > m_iSelectEnd) {
		SelStart = m_iSelectEnd;
		SelEnd = m_iSelectStart;
	}
	else {
		SelStart = m_iSelectStart;
		SelEnd = m_iSelectEnd;
	}

	if (!OpenClipboard()) {
		AfxMessageBox("Cannot open the Clipboard");
		return;
	}

	EmptyClipboard();

	hMem = GlobalAlloc(GMEM_MOVEABLE, sizeof(stClipData) + 1);

	if (hMem == NULL)
		return;

	ClipData = (stClipData*)GlobalLock(hMem);
	ClipData->Size = SelEnd - SelStart + 1;

	for (i = SelStart; i < SelEnd + 1; i++) {
		Note = pDoc->GetNoteData(m_iSelectChannel, i, m_iCurrentFrame);
		ClipData->Data[Ptr].ExtraStuff1 = Note->ExtraStuff1;
		ClipData->Data[Ptr].ExtraStuff2 = Note->ExtraStuff2;
		ClipData->Data[Ptr].Instrument	= Note->Instrument;
		ClipData->Data[Ptr].Note		= Note->Note;
		ClipData->Data[Ptr].Octave		= Note->Octave;
		ClipData->Data[Ptr].Vol			= Note->Vol;
		Ptr++;
	}

	GlobalUnlock(hMem);
	SetClipboardData(m_iClipBoard, hMem);
	CloseClipboard();
}

void CFamiTrackerView::OnEditCut()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stClipData	*ClipData;
	stChanNote	*Note;
	HGLOBAL		hMem;
	int			Ptr = 0;
	int			i;
	int			SelStart, SelEnd;

	if (m_iSelectStart == -1)
		return;

	if (m_iSelectStart > m_iSelectEnd) {
		SelStart = m_iSelectEnd;
		SelEnd = m_iSelectStart;
	}
	else {
		SelStart = m_iSelectStart;
		SelEnd = m_iSelectEnd;
	}

	if (!OpenClipboard()) {
		AfxMessageBox("Cannot open the Clipboard");
		return;
	}

	EmptyClipboard();

	hMem = GlobalAlloc(GMEM_MOVEABLE, sizeof(stClipData) + 1);

	if (hMem == NULL)
		return;

	ClipData = (stClipData*)GlobalLock(hMem);
	ClipData->Size = SelEnd - SelStart + 1;

	for (i = SelStart; i < SelEnd + 1; i++) {
		Note = pDoc->GetNoteData(m_iSelectChannel, i, m_iCurrentFrame);
		ClipData->Data[Ptr].ExtraStuff1 = Note->ExtraStuff1;
		ClipData->Data[Ptr].ExtraStuff2 = Note->ExtraStuff2;
		ClipData->Data[Ptr].Instrument	= Note->Instrument;
		ClipData->Data[Ptr].Note		= Note->Note;
		ClipData->Data[Ptr].Octave		= Note->Octave;
		ClipData->Data[Ptr].Vol			= Note->Vol;
		Ptr++;
	}

	AddUndo(m_iCursorChannel);

	for (i = SelStart; i < SelEnd + 1; i++) {
		pDoc->RemoveNote(SelStart + 1, m_iCurrentFrame, m_iCursorChannel);
	}

	m_iSelectStart = -1;

	GlobalUnlock(hMem);
	SetClipboardData(m_iClipBoard, hMem);
	CloseClipboard();
	RedrawWindow();
}

void CFamiTrackerView::OnEditPaste()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	HGLOBAL		hMem;
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

	if (m_bPasteOverwrite) {
		for (i = 0; i < ClipData->Size; i++) {
			pDoc->RemoveNote((m_iCursorRow + 1), m_iCurrentFrame, m_iCursorChannel);
		}
	}

	for (i = 0; i < ClipData->Size; i++) {
		if ((m_iCursorRow + i) < pDoc->m_iPatternLength) {
			pDoc->InsertNote(m_iCursorRow + i, m_iCurrentFrame, m_iCursorChannel);
			pDoc->SetNoteData(m_iCursorChannel, m_iCursorRow + i, m_iCurrentFrame, &ClipData->Data[i]);
		}
	}

	GlobalUnlock(hMem);
	CloseClipboard();
	RedrawWindow();
}

void CFamiTrackerView::OnEditDelete()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

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

	for (i = SelStart; i < SelEnd + 1; i++) {
		pDoc->RemoveNote(SelStart + 1, m_iCurrentFrame, m_iSelectChannel);
	}

	m_iSelectStart = -1;

	RedrawWindow();
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

	m_bPlaying		= true;
	m_bPlayLooped	= false;
	FirstTick		= true;
	m_iPlayTime		= 0;
	
	PlayerSyncTick	= 0;
	TickPeriod		= pDoc->m_iSongSpeed;

	theApp.SilentEverything();

	ScrollToTop();
	RedrawWindow();

	theApp.SetMachineType(pDoc->m_iMachine, pDoc->m_iEngineSpeed);
}

void CFamiTrackerView::OnTrackerPlaypattern()
{
	CMainFrame *pMainFrm = (CMainFrame*)GetParentFrame();
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_bPlaying)
		return;

	m_bPlaying		= true;
	m_bPlayLooped	= true;	
	PlayerSyncTick	= 0;
	m_iPlayTime		= 0;
	TickPeriod		= pDoc->m_iSongSpeed;

	theApp.SilentEverything();
}

void CFamiTrackerView::OnTrackerStop()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	theApp.SilentEverything();
	m_bPlaying = false;
	TickPeriod = pDoc->m_iSongSpeed;
}

void CFamiTrackerView::OnTrackerEdit()
{
	m_bEditEnable = !m_bEditEnable;

	if (m_bEditEnable)
		SetMessageText("Edit mode");
	else
		SetMessageText("Normal mode");
	
	Invalidate(TRUE);
	RedrawWindow();
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

void CFamiTrackerView::SetupMidi()
{
	MIDI.Init();
	m_bInitialized = true;
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

void CFamiTrackerView::OnLButtonUp(UINT nFlags, CPoint point)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Channel, Column, Row, DeltaY, i;
	int RowsInWindow = (WindowHeight / ROW_HEIGHT) - 1;

	Channel = GET_CHANNEL(point.x);
	Column	= GET_COLUMN(point.x);

	if (Channel < 0)
		Channel = 0;
	if (Column < 0)
		Column = 0;

	DeltaY = point.y - (WindowHeight / 2) - 10;

	if (DeltaY < 0)
		Row = DeltaY / ROW_HEIGHT - 1;
	else
		Row = DeltaY / ROW_HEIGHT;

	for (i = COLUMNS - 1; i > -1; i--) {
		if (Column > COLUMN_START[i]) {
			Column = i;
			break;
		}
	}

	if (m_iSelectStart == -1) {
		if (((Row + m_iCurrentRow) >= 0 && (Row + m_iCurrentRow) < pDoc->m_iPatternLength) && point.y > ROW_HEIGHT * 2) {
			m_iCursorRow = m_iCurrentRow + Row;
			if (/*LockCursor*/ !theApp.m_bFreeCursorEdit)
				m_iCurrentRow = m_iCursorRow;

			if (Channel >= 0 && Channel < pDoc->m_iChannelsAvaliable /*&& Row == 1*/ && point.y > ROW_HEIGHT * 2) {
				m_iCursorChannel = Channel;
				m_iCursorColumn = Column;
			}

			RedrawWindow();
		}
	}

	CView::OnLButtonUp(nFlags, point);
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
		if (pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Note > 0) {
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
			if (pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Note > 0) {
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

	RedrawWindow();
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

	RedrawWindow();
}

void CFamiTrackerView::OnTransposeIncreasenote()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (!m_bEditEnable)
		return;

	if (m_iSelectStart == -1) {
		AddUndo(m_iCursorChannel);
		if (pDoc->ChannelData[m_iCursorChannel][pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel]][m_iCursorRow].Note > 0) {
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
			if (pDoc->ChannelData[m_iSelectChannel][pDoc->FrameList[m_iCurrentFrame][m_iSelectChannel]][i].Note > 0) {
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

	RedrawWindow();

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

	RedrawWindow();
}

void CFamiTrackerView::OnEditUndo()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (!m_iUndoLevel)
		return;

	if (!m_iRedoLevel) {
		int Channel = UndoStack[m_iUndoLevel - 1].Channel;
		int Pattern = pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel];
		int x;

		UndoStack[m_iUndoLevel].Channel	= Channel;
		UndoStack[m_iUndoLevel].Pattern	= Pattern;
		UndoStack[m_iUndoLevel].Row		= m_iCursorRow;
		UndoStack[m_iUndoLevel].Column	= m_iCursorColumn;
		UndoStack[m_iUndoLevel].Frame	= m_iCurrentFrame;

		for (x = 0; x < MAX_PATTERN_LENGTH; x++) {
			UndoStack[m_iUndoLevel].ChannelData[x] = pDoc->ChannelData[Channel][Pattern][x];
		}
	}

	m_iRedoLevel++;
	m_iUndoLevel--;

	int Channel, Pattern, x;

	Channel = UndoStack[m_iUndoLevel].Channel;
	Pattern = UndoStack[m_iUndoLevel].Pattern;

	for (x = 0; x < MAX_PATTERN_LENGTH; x++) {
		pDoc->ChannelData[Channel][Pattern][x] = UndoStack[m_iUndoLevel].ChannelData[x];
	}

	m_iCursorChannel	= UndoStack[m_iUndoLevel].Channel;
	m_iCursorRow		= UndoStack[m_iUndoLevel].Row;
	m_iCursorColumn		= UndoStack[m_iUndoLevel].Column;
	m_iCurrentFrame		= UndoStack[m_iUndoLevel].Frame;

	pDoc->FrameList[m_iCurrentFrame][m_iCursorChannel] = Pattern;

	m_iCurrentRow = m_iCursorRow;

	CString Text;
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
				UndoStack[x - 1] = UndoStack[x];
			}
		}
	}

	UndoStack[m_iUndoLevel - 1] = UndoBlock;

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

	Channel = UndoStack[m_iUndoLevel].Channel;
	Pattern = UndoStack[m_iUndoLevel].Pattern;

	for (x = 0; x < MAX_PATTERN_LENGTH; x++) {
		pDoc->ChannelData[Channel][Pattern][x] = UndoStack[m_iUndoLevel].ChannelData[x];
	}

	m_iCursorChannel	= /*UndoStack[m_iUndoLevel].Channel*/ Channel;
	m_iCursorRow		= UndoStack[m_iUndoLevel].Row;
	m_iCursorColumn		= UndoStack[m_iUndoLevel].Column;
	m_iCurrentFrame		= UndoStack[m_iUndoLevel].Frame;

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

	m_iUndoLevel = 0;
	m_iRedoLevel = 0;

	m_iLastRowState = 0;
	m_bForceRedraw	= true;

	m_iInstrument	= 0;
}

void CFamiTrackerView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Channel = GET_CHANNEL(point.x);

	if (point.y < ROW_HEIGHT * 2 && Channel < pDoc->m_iChannelsAvaliable && Channel > 0) {

		m_bMuteChannels[Channel] = false;

		for (int i = 0; i < pDoc->m_iChannelsAvaliable; i++) {

			if (i != Channel) {
				if (m_bPlaying) {
					stChanNote Note;
					Note.Note		= HALT;
					Note.Instrument = 0;
					Note.Octave		= 0;
					theApp.PlayNote(Channel, &Note);
				}
				m_bMuteChannels[i] = true;
			}
		}

		ForceRedraw();
	}
	else
		SelectWholePattern((point.x - 50) / 130);

	CView::OnLButtonDblClk(nFlags, point);
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
	Note.ExtraStuff1 = 0;
	Note.ExtraStuff2 = 0;

	for (i = SelStart; i < SelEnd + 1; i++) {
		//pDoc->RemoveNote(SelStart + 1, m_iCurrentFrame, m_iSelectChannel);
		pDoc->SetNoteData(m_iSelectChannel, i, m_iCurrentFrame, &Note);
	}

	m_iSelectStart = -1;

	RedrawWindow();
}

void CFamiTrackerView::OnEditSelectall()
{
	SelectWholePattern(m_iCursorChannel);
}

void CFamiTrackerView::SelectWholePattern(int Channel)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_iSelectEnd == 0 || m_iSelectStart == -1) {
		m_iSelectChannel	= Channel/*(point.x - 50) / 130*/;
		m_iSelectStart		= 0;
		m_iSelectEnd		= pDoc->m_iPatternLength - 1;
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

	if (!m_bInitialized)
		return;

	while (MIDI.ReadMessage(Message, Channel, Note, Octave, Velocity)) {
	
		if (!theApp.m_bMidiChannelMap)
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
						Note.Note = HALT;
						theApp.PlayNote(Channel, &Note);
						LastOctave = 0;
						LastNote = 0;
					}
				}
				else {
					if (!theApp.m_bMidiVelocity)
						Velocity = 127;
					InsertNote(Note, Octave, Channel, Velocity + 1);
					LastNote = Note;
					LastOctave = Octave;
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

void CFamiTrackerView::OnFileMidisettings()
{
	MIDI.OpenConfigDialog();
}

void CFamiTrackerView::PostNcDestroy()
{
	// Called at end of program

	m_bInitialized = false;

	MIDI.Shutdown();
	CView::PostNcDestroy();
}

void CFamiTrackerView::OnTimer(UINT nIDEvent)
{
	CMainFrame *pMainFrm;
	static int LastNoteState;

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
			m_iLastRowState		= m_iCurrentRow;
			m_iLastFrameState	= m_iCurrentFrame;
			m_bForceRedraw		= false;

			Invalidate(FALSE);
			PostMessage(WM_PAINT);

			((CMainFrame*)GetParent())->RefreshPattern();
		}
		else {
			CDC *pDC = this->GetDC();
			DrawMeters(pDC);
			this->ReleaseDC(pDC);
			//pDC->DeleteDC();
		}

		if (LastNoteState != m_iKeyboardNote)
			pMainFrm->ChangeNoteState(m_iKeyboardNote);

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
	MIDI.Toggle();
}

void CFamiTrackerView::OnUpdateEditEnablemidi(CCmdUI *pCmdUI)
{
	if (MIDI.IsAvaliable())
		pCmdUI->Enable(1);
	else
		pCmdUI->Enable(0);

	if (MIDI.IsOpened())
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

	for (x = pDoc->m_iFrameCount; x > (m_iCurrentFrame - 1); x--) {
		for (i = 0; i < pDoc->m_iChannelsAvaliable; i++) {
			pDoc->FrameList[x + 1][i] = pDoc->FrameList[x][i];
		}
	}

	for (i = 0; i < pDoc->m_iChannelsAvaliable; i++) {
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

	for (i = 0; i < pDoc->m_iChannelsAvaliable; i++) {
		pDoc->FrameList[m_iCurrentFrame][i] = 0;
	}

	for (x = m_iCurrentFrame; x < pDoc->m_iFrameCount; x++) {
		for (i = 0; i < pDoc->m_iChannelsAvaliable; i++) {
			pDoc->FrameList[x][i] = pDoc->FrameList[x + 1][i];
		}
	}

	pDoc->m_iFrameCount--;

	if (m_iCurrentFrame > (pDoc->m_iFrameCount - 1))
		m_iCurrentFrame = (pDoc->m_iFrameCount - 1);
}

void CFamiTrackerView::RegisterKeyState(int Channel, int Note)
{
	if (Channel == m_iCursorChannel) {
		m_iKeyboardNote	= Note;
	}
}

void CFamiTrackerView::OnTrackerPlayrow()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	int i;

	ASSERT_VALID(pDoc);

	for (i = 0; i < pDoc->m_iChannelsAvaliable; i++) {
		pDoc->PlayNote(i, m_iCurrentRow, m_iCurrentFrame);
	}

	ScrollRowDown();
}
