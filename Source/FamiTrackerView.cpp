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
#include <cmath>
#include <afxmt.h>
#include "FontDrawer.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "MIDI.h"
#include "InstrumentEditDlg.h"
#include "SpeedDlg.h"
#include "SoundGen.h"
#include "PatternView.h"
#include "Settings.h"
#include "Accelerator.h"
#include "TrackerChannel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const char CLIPBOARD_ID[] = _T("FamiTracker");

const char *EFFECT_TEXTS[] = {
	{_T("FXX - Set speed/tempo, XX < 20 = speed, XX >= 20 = tempo")},
	{_T("BXX - Jump to frame")},
	{_T("DXX - Skip to next frame")},
	{_T("CXX - Halt")},
	{_T("EXX - Set volume")},
	{_T("3XX - Automatic portamento, XX = speed")},
	{_T("(not used)")},
	{_T("HXY - Hardware sweep up, x = speed, y = shift")},
	{_T("IXY - Hardware sweep down, x = speed, y = shift")},
	{_T("0XY - Arpeggio, X = second note, Y = third note")},
	{_T("4XY - Vibrato, x = speed, y = depth")},
	{_T("7XY - Tremolo, x = speed, y = depth")},
	{_T("PXX - Fine pitch")},
	{_T("GXX - Row delay, XX = number of frames")},
	{_T("ZXX - DPCM delta counter setting")},
	{_T("1XX - Slide up, XX = speed")},
	{_T("2XX - Slide down, XX = speed")},
	{_T("VXX - Square duty / Noise mode")},
	{_T("YXX - DPCM sample offset")},
	{_T("QXY - Portamento up, X = speed, Y = notes")},
	{_T("RXY - Portamento down, X = speed, Y = notes")},
	{_T("AXY - Volume slide, X = up, Y = down")},
};

const unsigned char KEY_DOT = 0xBD;		// '.'
const unsigned char KEY_DASH = 0xBE;	// '-'
const int KEY_REMOVE = -67;				// '-'

// Synchronization
//CMutex m_NoteDataMutex;
CMutex DrawMutex;

// Todo: fix this
int m_iLastNote;
int m_iLastInstrument, m_iLastVolume;
int m_iLastEffect, m_iLastEffectParam;

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
	ON_WM_MENUCHAR()
	ON_WM_SYSKEYDOWN()
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)	
	ON_COMMAND(ID_EDIT_SELECTALL, OnEditSelectall)
	ON_COMMAND(ID_EDIT_PASTEOVERWRITE, OnEditPasteoverwrite)
	ON_COMMAND(ID_EDIT_INSTRUMENTMASK, OnEditInstrumentMask)
	ON_COMMAND(ID_EDIT_INTERPOLATE, OnEditInterpolate)
	ON_COMMAND(ID_EDIT_REVERSE, OnEditReverse)
	ON_COMMAND(ID_EDIT_REPLACEINSTRUMENT, OnEditReplaceInstrument)
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
	ON_COMMAND(ID_TRACKER_TOGGLECHANNEL, OnTrackerToggleChannel)
	ON_COMMAND(ID_TRACKER_SOLOCHANNEL, OnTrackerSoloChannel)
	ON_COMMAND(ID_EDIT_PASTEMIX, OnEditPastemix)
	ON_COMMAND(ID_MODULE_MOVEFRAMEDOWN, OnModuleMoveframedown)
	ON_COMMAND(ID_MODULE_MOVEFRAMEUP, OnModuleMoveframeup)
	ON_COMMAND(ID_CMD_OCTAVE_NEXT, OnNextOctave)
	ON_COMMAND(ID_CMD_OCTAVE_PREVIOUS, OnPreviousOctave)
	ON_COMMAND(ID_CMD_PASTEOVERWRITE, OnPasteOverwrite)
	ON_COMMAND(ID_CMD_PASTEMIXED, OnPasteMixed)	
	ON_COMMAND(ID_CMD_NEXT_INSTRUMENT, OnNextInstrument)
	ON_COMMAND(ID_CMD_PREV_INSTRUMENT, OnPrevInstrument)
	ON_COMMAND(ID_CMD_INCREASESTEPSIZE, OnIncreaseStepSize)
	ON_COMMAND(ID_CMD_DECREASESTEPSIZE, OnDecreaseStepSize)
	ON_COMMAND(ID_POPUP_TOGGLECHANNEL, OnTrackerToggleChannel)
	ON_COMMAND(ID_POPUP_SOLOCHANNEL, OnTrackerSoloChannel)
	ON_COMMAND(ID_CMD_STEP_UP, OnOneStepUp)
	ON_COMMAND(ID_CMD_STEP_DOWN, OnOneStepDown)	
//	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
//	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditDelete)
	ON_UPDATE_COMMAND_UI(ID_EDIT_INSTRUMENTMASK, OnUpdateEditInstrumentMask)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_EDIT, OnUpdateTrackerEdit)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_PAL, OnUpdateTrackerPal)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_NTSC, OnUpdateTrackerNtsc)
	ON_UPDATE_COMMAND_UI(ID_SPEED_DEFAULT, OnUpdateSpeedDefault)
	ON_UPDATE_COMMAND_UI(ID_SPEED_CUSTOM, OnUpdateSpeedCustom)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTEOVERWRITE, OnUpdateEditPasteoverwrite)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_UPDATE_COMMAND_UI(ID_FRAME_REMOVE, OnUpdateFrameRemove)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTEMIX, OnUpdateEditPastemix)
	ON_UPDATE_COMMAND_UI(ID_MODULE_MOVEFRAMEDOWN, OnUpdateModuleMoveframedown)
	ON_UPDATE_COMMAND_UI(ID_MODULE_MOVEFRAMEUP, OnUpdateModuleMoveframeup)
	ON_WM_NCMOUSEMOVE()
	ON_WM_SYSKEYUP()
END_MESSAGE_MAP()

// Convert keys 0-F to numbers
int ConvertKeyToHex(int Key) {

	switch (Key) {
		case 48: case VK_NUMPAD0: return 0x00;
		case 49: case VK_NUMPAD1: return 0x01;
		case 50: case VK_NUMPAD2: return 0x02;
		case 51: case VK_NUMPAD3: return 0x03;
		case 52: case VK_NUMPAD4: return 0x04;
		case 53: case VK_NUMPAD5: return 0x05;
		case 54: case VK_NUMPAD6: return 0x06;
		case 55: case VK_NUMPAD7: return 0x07;
		case 56: case VK_NUMPAD8: return 0x08;
		case 57: case VK_NUMPAD9: return 0x09;
		case 65: return 0x0A;
		case 66: return 0x0B;
		case 67: return 0x0C;
		case 68: return 0x0D;
		case 69: return 0x0E;
		case 70: return 0x0F;

		case KEY_DOT:
		case KEY_DASH:
			return 0x80;
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

	m_iKeyStepping		= 1;
	m_iRealKeyStepping	= 1;
	m_bEditEnable		= false;

	m_bShiftPressed		= false;
	m_bControlPressed	= false;
	m_bChangeAllPattern	= false;
	m_iPasteMode		= PASTE_MODE_NORMAL;

	m_bMaskInstrument	= false;
	m_bSwitchToInstrument = false;

	m_iOctave			= 3;
	m_iInstrument		= 0;

	m_iTempo			= 150;
	m_iSpeed			= 6;
	m_iTickPeriod		= 6;

	m_iPlayTime			= 0;

	m_bUpdateBackground = true;

	m_bFollowMode		= true;

	m_iAutoArpPtr		= 0; 
	m_iLastAutoArpPtr	= 0;
	m_iAutoArpKeyCount	= 0;

	m_iMenuChannel = -1;

	m_iFrameQueue = -1;

	m_iKeyboardNote = -1;

	memset(m_iActiveNotes, 0, sizeof(int) * MAX_CHANNELS);

	for (i = 0; i < MAX_CHANNELS; i++) {
		m_bMuteChannels[i] = false;
	}

	for (i = 0; i < 256; i++)
		m_cKeyList[i] = 0;

	memset(Arpeggiate, 0, sizeof(int) * MAX_CHANNELS);

	m_pPatternView = new CPatternView();
}

CFamiTrackerView::~CFamiTrackerView()
{
}

BOOL CFamiTrackerView::PreCreateWindow(CREATESTRUCT& cs)
{
	m_iClipBoard = RegisterClipboardFormat(CLIPBOARD_ID);

	if (m_iClipBoard == 0)
		theApp.DisplayError(IDS_CLIPBOARD_ERROR);

	CreateFont();

	m_pPatternView->InitView(m_iClipBoard);

	return CView::PreCreateWindow(cs);
}

void CFamiTrackerView::CreateFont()
{
	m_pPatternView->CreateFonts();
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

void CFamiTrackerView::OnDraw(CDC* pDC)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// Check document
	if (!pDoc->IsFileLoaded()) {
		pDC->FillSolidRect(0, 0, m_iWindowWidth, m_iWindowHeight, 0x000000);
		pDC->SetTextColor(0xFFFFFF);
		pDC->TextOut(m_iWindowWidth / 2 - 84, m_iWindowHeight / 2 - 4, _T("No document is loaded"));
		return;
	}

	// Don't draw when rendering to wave file
	if (theApp.GetSoundGenerator()->IsRendering())
		return;

	m_pPatternView->SetHighlight(((CMainFrame*)GetParentFrame())->GetHighlightRow(), ((CMainFrame*)GetParentFrame())->GetSecondHighlightRow());

	//m_pPatternView->AdjustCursorChannel();
	m_pPatternView->DrawScreen(pDC, this);
}

BOOL CFamiTrackerView::OnEraseBkgnd(CDC* pDC)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// Check document
	if (!pDoc->IsFileLoaded())
		return FALSE;

	// Called when the background should be erased
	m_pPatternView->CreateBackground(pDC, m_bUpdateBackground);
	m_bUpdateBackground = false;
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General
////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CFamiTrackerView::PreTranslateMessage(MSG* pMsg)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	switch (pMsg->message) {
		case MSG_UPDATE:
			UpdateEditor(UPDATE_FAST);
			return FALSE;
			break;
		case MSG_MIDI_EVENT:
			if (m_bInitialized)
				TranslateMidiMessage();
			return FALSE;
			break;
	}

	// Todo: Temporary fix
//	return FALSE;

	return CView::PreTranslateMessage(pMsg);
}

void CFamiTrackerView::CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType)
{
	// Window size has changed
	m_iWindowWidth	= (lpClientRect->right - lpClientRect->left) - 17;
	m_iWindowHeight	= (lpClientRect->bottom - lpClientRect->top) - 17;

	m_bUpdateBackground = true;

	int Width = lpClientRect->right - lpClientRect->left;
	int Height = lpClientRect->bottom - lpClientRect->top;

	m_pPatternView->SetWindowSize(Width, Height);

	CView::CalcWindowRect(lpClientRect, nAdjustType);
}

// Scroll

void CFamiTrackerView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	switch (nSBCode) {
		case SB_LINEDOWN: 
			m_pPatternView->MoveDown(1); 
			OnUpdate(this, UPDATE_ENTIRE, NULL);
			break;
		case SB_LINEUP:
			m_pPatternView->MoveUp(1);
			OnUpdate(this, UPDATE_ENTIRE, NULL);
			break;
		case SB_PAGEDOWN:
			m_pPatternView->MovePageDown(); 
			OnUpdate(this, UPDATE_ENTIRE, NULL);
			break;
		case SB_PAGEUP:
			m_pPatternView->MovePageUp(); 
			OnUpdate(this, UPDATE_ENTIRE, NULL);
			break;
		case SB_TOP:
			m_pPatternView->MoveToTop(); 
			OnUpdate(this, UPDATE_ENTIRE, NULL);
			break;
		case SB_BOTTOM:	
			m_pPatternView->MoveToBottom(); 
			OnUpdate(this, UPDATE_ENTIRE, NULL);
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			m_pPatternView->MoveToRow(nPos);
			OnUpdate(this, UPDATE_ENTIRE, NULL);
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
		case SB_LINERIGHT: 
			m_pPatternView->MoveRight();
			OnUpdate(this, UPDATE_ENTIRE, NULL);
			break;
		case SB_LINELEFT: 
			m_pPatternView->MoveLeft();	
			OnUpdate(this, UPDATE_ENTIRE, NULL);
			break;
		case SB_PAGERIGHT: 
			m_pPatternView->NextChannel(); 
			OnUpdate(this, UPDATE_ENTIRE, NULL);
			break;
		case SB_PAGELEFT: 
			m_pPatternView->PreviousChannel(); 
			OnUpdate(this, UPDATE_ENTIRE, NULL);
			break;
		case SB_RIGHT: 
			m_pPatternView->FirstChannel(); 
			OnUpdate(this, UPDATE_ENTIRE, NULL);
			break;
		case SB_LEFT: 
			m_pPatternView->LastChannel(); 
			OnUpdate(this, UPDATE_ENTIRE, NULL);
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			for (i = 0; i < pDoc->GetAvailableChannels(); i++) {
				for (x = 0; x < signed(COLUMNS + pDoc->GetEffColumns(i) * 3); x++) {
					if (count++ == nPos) {
						m_pPatternView->MoveToChannel(i);
						m_pPatternView->MoveToColumn(x);
						//ForceRedraw();
						OnUpdate(this, UPDATE_ENTIRE, NULL);
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

// Mouse

void CFamiTrackerView::OnRButtonUp(UINT nFlags, CPoint point)
{
	// Popup menu

	CRect WinRect;
	CMenu *PopupMenu, PopupMenuBar;
	int Item = 0;

	if (m_pPatternView->CancelDragging()) {
		ForceRedraw();
		CView::OnRButtonUp(nFlags, point);
		return;
	}

	m_pPatternView->OnMouseRDown(point);

	if (point.y < HEADER_HEIGHT)
		m_iMenuChannel = m_pPatternView->GetChannelAtPoint(point.x);
	else
		m_iMenuChannel = -1;

	GetWindowRect(WinRect);
	PopupMenuBar.LoadMenu(IDR_PATTERN_POPUP);

	PopupMenu = PopupMenuBar.GetSubMenu(0);
	PopupMenu->TrackPopupMenu(TPM_RIGHTBUTTON, point.x + WinRect.left, point.y + WinRect.top, this);

	CView::OnRButtonUp(nFlags, point);
}

void CFamiTrackerView::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_pPatternView->OnMouseDown(point);
	SetCapture();	// Capture mouse 
	ForceRedraw();
	CView::OnLButtonDown(nFlags, point);
}

void CFamiTrackerView::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_pPatternView->OnMouseUp(point);
	ReleaseCapture();
	ForceRedraw();
	CView::OnLButtonUp(nFlags, point);
}

void CFamiTrackerView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	m_pPatternView->OnMouseDblClk(point);
	ForceRedraw();
	CView::OnLButtonDblClk(nFlags, point);
}

void CFamiTrackerView::OnMouseMove(UINT nFlags, CPoint point)
{
	static int LastX, LastY;
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (point.x == LastX && point.y == LastY)
		return;

	LastX = point.x;
	LastY = point.y;

	if (m_pPatternView->OnMouseHover(nFlags, point))
		ForceRedraw();

	if (!(nFlags & MK_LBUTTON))
		return;

	m_pPatternView->OnMouseMove(nFlags, point);
	ForceRedraw();

	CView::OnMouseMove(nFlags, point);
}

BOOL CFamiTrackerView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (m_bControlPressed && m_bShiftPressed) {
		if (zDelta < 0)
			m_pPatternView->NextFrame();
		else
			m_pPatternView->PreviousFrame();
	}
	if (m_bControlPressed) {
		if (zDelta > 0)
			m_pPatternView->Transpose(TRANSPOSE_INC_NOTES);
		else
			m_pPatternView->Transpose(TRANSPOSE_DEC_NOTES);
	}
	else if (m_bShiftPressed) {
		if (zDelta > 0)
			m_pPatternView->ScrollValues(1);
		else
			m_pPatternView->ScrollValues(-1);
	}
	else
		m_pPatternView->OnMouseScroll(zDelta);

	ForceRedraw();	

	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

// End of mouse

void CFamiTrackerView::OnKillFocus(CWnd* pNewWnd)
{
	CView::OnKillFocus(pNewWnd);
	m_bHasFocus = false;
	m_pPatternView->SetFocus(false);
	UpdateEditor(UPDATE_ENTIRE);
}

void CFamiTrackerView::OnSetFocus(CWnd* pOldWnd)
{
	CView::OnSetFocus(pOldWnd);
	m_bHasFocus = true;
	m_pPatternView->SetFocus(true);
	m_bControlPressed = false;
	m_bShiftPressed = false;
	UpdateEditor(UPDATE_ENTIRE);
}

void CFamiTrackerView::PostNcDestroy()
{
	// Called on exit
	m_bInitialized = false;
	CView::PostNcDestroy();
}

void CFamiTrackerView::OnTimer(UINT nIDEvent)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CMainFrame *pMainFrm;
	static int LastNoteState;

	switch (nIDEvent) {
		case 0: {
			int TicksPerSec = pDoc->GetFrameRate();

//			static_cast<CMainFrame*>(GetParentFrame())->SetIndicatorTime(m_iPlayTime / 600, (m_iPlayTime / 10) % 60, m_iPlayTime % 10);

			pMainFrm = (CMainFrame*)GetParentFrame();

			pMainFrm->SetIndicatorTime(m_iPlayTime / 600, (m_iPlayTime / 10) % 60, m_iPlayTime % 10);

			// DPCM info
			CSoundGen *pSoundGen = theApp.GetSoundGenerator();

			if (pSoundGen) {
				stDPCMState DPCMState = pSoundGen->GetDPCMState();
				m_pPatternView->SetDPCMState(DPCMState);
			}

			bool bDraw = false;

			// Synchronized access to m_bForceRedraw
			DrawMutex.Lock();
			bDraw = m_bForceRedraw;
			m_bForceRedraw = false;
			DrawMutex.Unlock();

			if (bDraw) {
				UpdateEditor(UPDATE_ENTIRE);	// Todo: Use something like CHANGED_PLAYING
	//			((CMainFrame*)GetParent())->DrawFrameWindow();
			}
			else {
				if (pDoc->IsFileLoaded()) {
					// Todo: Change this to use the ordinary drawing routines
					CDC *pDC = this->GetDC();
					m_pPatternView->DrawMeters(pDC);
					ReleaseDC(pDC);
				}
			}

			if (LastNoteState != m_iKeyboardNote)
				pMainFrm->ChangeNoteState(m_iKeyboardNote);

			LastNoteState = m_iKeyboardNote;

			theApp.CheckSynth();

			}
			break;
		case 1:
			if (m_pPatternView->ScrollTimer()) {
				UpdateEditor(UPDATE_ENTIRE);
			}
			break;
	}

	CView::OnTimer(nIDEvent);
}

int CFamiTrackerView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Install a timer for screen updates
	SetTimer(0, 20, NULL);

	// Install a timer for scrolling
	SetTimer(1, 30, NULL);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Menu commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::OnEditCopy()
{
	stClipData	*pClipData;
	HGLOBAL		hMem;

	if (GetFocus() != this)
		return;

	if (!OpenClipboard()) {
		theApp.DisplayError(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	EmptyClipboard();

	hMem = GlobalAlloc(GMEM_MOVEABLE, sizeof(stClipData));

	if (hMem == NULL) {
		CloseClipboard();
		return;
	}

	pClipData = (stClipData*)GlobalLock(hMem);

	m_pPatternView->Copy(pClipData);

	GlobalUnlock(hMem);

	// Set clipboard for internal data
	SetClipboardData(m_iClipBoard, hMem);

	CString MML = m_pPatternView->GetMMLString();

	hMem = GlobalAlloc(GMEM_MOVEABLE, MML.GetLength() + 1);
	
	if (hMem == NULL) {
		CloseClipboard();
		return;
	}

	char *pStr = (char*)GlobalLock(hMem);

	strcpy(pStr, MML.GetBuffer());

	GlobalUnlock(hMem);

	// Set clipboard for text data
	SetClipboardData(CF_TEXT, hMem);

	CloseClipboard();
}

void CFamiTrackerView::OnEditCut()
{
	OnEditCopy();
	OnEditDelete();
	ForceRedraw();
}

void CFamiTrackerView::OnEditPaste()
{
	stClipData	*pClipData;
	HGLOBAL		hMem;

	if (GetFocus() != this)
		return;

	OpenClipboard();

	if (!IsClipboardFormatAvailable(m_iClipBoard)) {
		theApp.DisplayError(IDS_CLIPBOARD_NOT_AVALIABLE);
	}

	hMem = GetClipboardData(m_iClipBoard);

	if (hMem == NULL)
		return;

	pClipData = (stClipData*)GlobalLock(hMem);

	m_pPatternView->AddUndo();

	if (m_iPasteMode == PASTE_MODE_MIX)
		m_pPatternView->PasteMix(pClipData);
	else
		m_pPatternView->Paste(pClipData);

	GlobalUnlock(hMem);
	CloseClipboard();
	ForceRedraw();
}

void CFamiTrackerView::OnEditDelete()
{
	m_pPatternView->AddUndo();
	m_pPatternView->Delete();
	ForceRedraw();
}

void CFamiTrackerView::OnTrackerEdit()
{
	m_bEditEnable = !m_bEditEnable;

	if (m_bEditEnable)
		SetMessageText(_T("Changed to edit mode"));
	else
		SetMessageText(_T("Changed to normal mode"));
	
	UpdateEditor(UPDATE_ENTIRE);
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
	pDoc->SetEngineSpeed(0);
	theApp.SetMachineType(pDoc->GetMachine(), (pDoc->GetMachine() == NTSC) ? 60 : 50);
}

void CFamiTrackerView::OnEditPasteoverwrite()
{
	if (m_iPasteMode == PASTE_MODE_OVERWRITE)
		m_iPasteMode = PASTE_MODE_NORMAL;
	else
		m_iPasteMode = PASTE_MODE_OVERWRITE;
}

void CFamiTrackerView::OnTransposeDecreasenote()
{
	m_pPatternView->Transpose(TRANSPOSE_DEC_NOTES);
	ForceRedraw();
}

void CFamiTrackerView::OnTransposeDecreaseoctave()
{
	m_pPatternView->Transpose(TRANSPOSE_DEC_OCTAVES);
	ForceRedraw();
}

void CFamiTrackerView::OnTransposeIncreasenote()
{
	m_pPatternView->Transpose(TRANSPOSE_INC_NOTES);
	ForceRedraw();
}

void CFamiTrackerView::OnTransposeIncreaseoctave()
{
	m_pPatternView->Transpose(TRANSPOSE_INC_OCTAVES);
	ForceRedraw();
}

void CFamiTrackerView::OnEditInstrumentMask()
{
	m_bMaskInstrument = !m_bMaskInstrument;
}

void CFamiTrackerView::OnEditUndo()
{
	m_pPatternView->Undo();
	ForceRedraw();
}

void CFamiTrackerView::OnEditRedo()
{
	m_pPatternView->Redo();
	UpdateEditor(UPDATE_ENTIRE);
}

void CFamiTrackerView::OnEditSelectall()
{
	m_pPatternView->SelectAll();
	ForceRedraw();
}

void CFamiTrackerView::OnFrameInsert()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int i, x, frames = pDoc->GetFrameCount(), channels = pDoc->GetAvailableChannels();
	int Frame = m_pPatternView->GetFrame();

	if (frames == MAX_FRAMES)
		return;

	pDoc->SetFrameCount(frames + 1);

	for (x = (frames + 1); x > (Frame + 1); x--) {
		for (i = 0; i < channels; i++) {
			pDoc->SetPatternAtFrame(x, i, pDoc->GetPatternAtFrame(x - 1, i));
		}
	}

	for (i = 0; i < channels; i++) {
		pDoc->SetPatternAtFrame(Frame + 1, i, pDoc->GetFirstFreePattern(i));
	}

	pDoc->UpdateAllViews(NULL, CHANGED_FRAMES);
}

void CFamiTrackerView::OnFrameRemove()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int i, x;

	int Frame = m_pPatternView->GetFrame();

	if (pDoc->GetFrameCount() == 1) {
		return;
	}

	for (i = 0; i < pDoc->GetAvailableChannels(); i++) {
		pDoc->SetPatternAtFrame(Frame, i, 0);
	}

	for (x = Frame; x < pDoc->GetFrameCount(); x++) {
		for (i = 0; i < pDoc->GetAvailableChannels(); i++) {
			pDoc->SetPatternAtFrame(x, i, pDoc->GetPatternAtFrame(x + 1, i));
		}
	}

	pDoc->SetFrameCount(pDoc->GetFrameCount() - 1);

	if (Frame > (signed)(pDoc->GetFrameCount() - 1))
		Frame = (pDoc->GetFrameCount() - 1);

	pDoc->UpdateAllViews(NULL, CHANGED_FRAMES);
}

void CFamiTrackerView::OnTrackerPlayrow()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote Note;
	int Frame = m_pPatternView->GetFrame();
	int Row = m_pPatternView->GetRow();

	for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
		pDoc->GetNoteData(Frame, i, Row, &Note);
		if (!m_bMuteChannels[i])
			FeedNote(i, &Note);
	}

	m_pPatternView->MoveDown(1);
	ForceRedraw();
}

void CFamiTrackerView::OnEditPastemix()
{
	if (m_iPasteMode == PASTE_MODE_MIX)
		m_iPasteMode = PASTE_MODE_NORMAL;
	else
		m_iPasteMode = PASTE_MODE_MIX;
}

void CFamiTrackerView::OnModuleMoveframedown()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Pattern;
	int Frame = m_pPatternView->GetFrame();

	if (Frame == (pDoc->GetFrameCount() - 1))
		return;

	for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
		Pattern = pDoc->GetPatternAtFrame(Frame, i);
		pDoc->SetPatternAtFrame(Frame, i, pDoc->GetPatternAtFrame(Frame + 1, i));
		pDoc->SetPatternAtFrame(Frame + 1, i, Pattern);
	}

	m_pPatternView->NextFrame();

	ForceRedraw();
}

void CFamiTrackerView::OnModuleMoveframeup()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	unsigned int Pattern, i;
	int Frame = m_pPatternView->GetFrame();

	if (Frame == 0)
		return;

	for (i = 0; i < pDoc->GetAvailableChannels(); i++) {
		Pattern = pDoc->GetPatternAtFrame(Frame, i);
		pDoc->SetPatternAtFrame(Frame, i, pDoc->GetPatternAtFrame(Frame - 1, i));
		pDoc->SetPatternAtFrame(Frame - 1, i, Pattern);
	}

	m_pPatternView->PreviousFrame();
	ForceRedraw();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UI updates
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::OnInitialUpdate()
{
	// Called when a new document is loaded
	//

	// Todo: clean up

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CMainFrame *MainFrame = static_cast<CMainFrame*>(GetParentFrame());

	unsigned int i;
	char Text[256];

	TRACE("OnInitialUpdate()\n");

	m_pPatternView->SetDocument(pDoc, this);

	CView::OnInitialUpdate();

	m_bInitialized	= true;
	m_bForceRedraw	= false;
	m_iInstrument	= 0;

	MainFrame->ClearInstrumentList();

	for (i = 0; i < MAX_INSTRUMENTS; i++) {
		if (pDoc->IsInstrumentUsed(i)) {
			pDoc->GetInstrumentName(i, Text);
			MainFrame->AddInstrument(i/*, Text, pDoc->GetInstrument(i)->GetType()*/);
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
	MainFrame->DisplayOctave();
	MainFrame->DrawFrameWindow();

	RedrawWindow();
}

void CFamiTrackerView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
	static DWORD LastUpdate;

	CMainFrame *pMainFrame = (CMainFrame*)GetParentFrame();

	switch (lHint) {
		case CHANGED_TRACK:			// Changed track
			m_pPatternView->MoveToChannel(0);
			m_pPatternView->MoveToColumn(0);
			m_pPatternView->MoveToRow(0);
			m_pPatternView->MoveToFrame(0);
			if (theApp.IsPlaying())
				theApp.ResetPlayer();
			m_pPatternView->Invalidate(false);
			Invalidate();
			RedrawWindow();
			break;
		case CHANGED_TRACKCOUNT:	// Changed track count
			pMainFrame->UpdateTrackBox();
			m_pPatternView->Invalidate(false);
			Invalidate();
			RedrawWindow();
			break;
		case CHANGED_FRAMES:
			pMainFrame->DrawFrameWindow();
			break;
		case RELOAD_COLORS:
			// Clear cache
			m_bUpdateBackground = true;
			m_pPatternView->Invalidate(true);
			RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
			break;
			
			// Remove these 

		case CHANGED_ERASE:
			// Invalidate, erase and redraw entire area
			m_bUpdateBackground = true;
			m_pPatternView->Invalidate(false);
			RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
			break;
		case UPDATE_FAST:
			m_pPatternView->Invalidate(false);
//			RedrawWindow(NULL, NULL, RDW_INVALIDATE);

			if (m_pPatternView->FullErase())
				RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
			else
				RedrawWindow(m_pPatternView->GetActiveRect(), NULL, RDW_INVALIDATE);

			pMainFrame->DrawFrameWindow();
			break;
		case UPDATE_ENTIRE:
			// Invalidate and redraw entire area
			m_pPatternView->Modified();
			m_pPatternView->Invalidate(true);
			RedrawWindow(NULL, NULL, RDW_INVALIDATE);
			pMainFrame->DrawFrameWindow();
			break;
		case CHANGED_CLEAR_SEL:
			m_pPatternView->ClearSelection();
			break;
			
			//
			//

		// Redraw instrument list
		case UPDATE_INSTRUMENTS:
			pMainFrame->UpdateInstrumentList();
			break;


			// This last option should be intelligent instead
		default:
			// Invalidate the tracker field, this should be removed
			m_pPatternView->Invalidate(false);
//			m_pPatternView->AdjustCursorChannel();
			if (m_pPatternView->FullErase())
				RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
			else
				RedrawWindow(m_pPatternView->GetActiveRect(), NULL, RDW_INVALIDATE);
			pMainFrame->DrawFrameWindow();
			break;
	}
}

void CFamiTrackerView::OnUpdateEditInstrumentMask(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bMaskInstrument ? 1 : 0);
}

void CFamiTrackerView::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_pPatternView->IsSelecting() ? 0 : 1);
}

void CFamiTrackerView::OnUpdateEditCut(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_pPatternView->IsSelecting() ? 0 : 1);
}

void CFamiTrackerView::OnUpdateEditPaste(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(IsClipboardFormatAvailable(m_iClipBoard) ? 1 : 0);
}

void CFamiTrackerView::OnUpdateEditDelete(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_pPatternView->IsSelecting() ? 0 : 1);
}

void CFamiTrackerView::OnUpdateTrackerEdit(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bEditEnable ? 1 : 0);
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

void CFamiTrackerView::OnUpdateEditPasteoverwrite(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_iPasteMode == PASTE_MODE_OVERWRITE);
}

void CFamiTrackerView::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pPatternView->CanUndo() ? 1 : 0);
}

void CFamiTrackerView::OnUpdateEditRedo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pPatternView->CanRedo() ? 1 : 0);
}

void CFamiTrackerView::OnUpdateFrameRemove(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	if (!pDoc->IsFileLoaded())
		return;
	pCmdUI->Enable(pDoc->GetFrameCount() > 1);
}

void CFamiTrackerView::OnUpdateEditPastemix(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_iPasteMode == PASTE_MODE_MIX);
}

void CFamiTrackerView::OnUpdateModuleMoveframedown(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	if (!pDoc->IsFileLoaded())
		return;
	pCmdUI->Enable(!(m_pPatternView->GetFrame() == (pDoc->GetFrameCount() - 1)));
}

void CFamiTrackerView::OnUpdateModuleMoveframeup(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pPatternView->GetFrame() > 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tracker playing routines
////////////////////////////////////////////////////////////////////////////////////////////////////////////

int CFamiTrackerView::PlayerCommand(char Command, int Value)
{
	// Handle commands from the player

	const int MUTEX_TIMEOUT = 100; 	// probably even 100ms is too much but I don't think this will ever fail

	CFamiTrackerDoc* pDoc = GetDocument();
	stChanNote NoteData;
	int Row, Frame;
	bool bDoUpdate = true;

	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	switch (Command) {
		case CMD_MOVE_TO_TOP:
			m_pPatternView->JumpToRow(0);
			m_pPatternView->JumpToFrame(m_pPatternView->GetFrame());
			break;
		case CMD_MOVE_TO_START:
			m_pPatternView->JumpToFrame(0);
			m_pPatternView->JumpToRow(0);
			break;
		case CMD_MOVE_TO_CURSOR:
			m_pPatternView->JumpToFrame(m_pPatternView->GetFrame());
			m_pPatternView->JumpToRow(m_pPatternView->GetRow());
			break;
		case CMD_STEP_DOWN:
			// Value = 0: not looping
			// Value = 1: looping
			if (m_pPatternView->StepRow()) {
				pSoundGen->FrameIsDone(1);
				if (m_iFrameQueue == -1) {
					if (!Value) {
						if (m_pPatternView->StepFrame()) {
							if (m_pPatternView->GetPlayFrame() == 0) {
								pSoundGen->SongIsDone();
							}
						}
					}
				}
				else {
					// Jump to queued frame
					m_pPatternView->JumpToFrame(m_iFrameQueue);
					m_iFrameQueue = -1;
				}
			}
			else {
				// Skip screen updates of play cursor isn't visible
				if (!m_pPatternView->IsPlayCursorVisible())
					bDoUpdate = false;
			}
			break;
		// Jump to frame
		case CMD_JUMP_TO:
			m_pPatternView->JumpToFrame(Value + 1);
			m_pPatternView->JumpToRow(0);
			break;
		// Skip to next frame
		case CMD_SKIP_TO:
			m_pPatternView->StepFrame();
			m_pPatternView->JumpToRow(Value);
			break;
		// Play next row
		case CMD_READ_ROW:
			for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
				Frame = m_pPatternView->GetPlayFrame();
				Row = m_pPatternView->GetPlayRow();
				pDoc->GetNoteData(Frame, i, Row, &NoteData);
				if (!m_bMuteChannels[i]) {
					if (NoteData.Instrument < 0x40 && NoteData.Note > 0 && i == m_pPatternView->GetChannel() && m_bSwitchToInstrument) {
						m_iInstrument = NoteData.Instrument;
					}
					FeedNote(i, &NoteData);
				}
				else {
					// These effects will pass even if the channel is muted
					const int PASS_EFFECTS[] = {EF_HALT, EF_JUMP, EF_SPEED, EF_SKIP};
					bool ValidCommand = false;
					unsigned int j, k;
					NoteData.Note		= 0;
					NoteData.Octave		= 0;
					NoteData.Instrument = 0;
					for (j = 0; j < (pDoc->GetEffColumns(i) + 1); j++) {
						for (k = 0; k < 4; k++) {
							if (NoteData.EffNumber[j] == PASS_EFFECTS[k])
								ValidCommand = true;
						}
						if (!ValidCommand)
							NoteData.EffNumber[j] = EF_NONE;
					}
					if (ValidCommand)
						FeedNote(i, &NoteData);
				}
			}
			return 0;
		case CMD_TIME:
			m_iPlayTime = Value;
			return 0;
		case CMD_GET_FRAME:
			return m_pPatternView->GetPlayFrame();
		case CMD_TICK:
			if (m_iAutoArpKeyCount == 1 || !theApp.m_pSettings->Midi.bMidiArpeggio)
				return 0;
			// auto arpeggio
			int OldPtr = m_iAutoArpPtr;
			do {
				m_iAutoArpPtr = (m_iAutoArpPtr + 1) & 127;				
				if (m_iAutoArpNotes[m_iAutoArpPtr] == 1) {
					m_iLastAutoArpPtr = m_iAutoArpPtr;
					Arpeggiate[m_pPatternView->GetChannel()] = m_iAutoArpPtr;
					break;
				}
 				else if (m_iAutoArpNotes[m_iAutoArpPtr] == 2) {
					m_iAutoArpNotes[m_iAutoArpPtr] = 0;
				}
			}
			while (m_iAutoArpPtr != OldPtr);
			// check this
			return 0;
	}

	if (bDoUpdate) {
		if (m_bHasFocus) {
			// Direct update
			PostMessage(MSG_UPDATE);
		}
		else {
			// Delayed update
			DrawMutex.Lock(100);
			m_bForceRedraw = true;
			DrawMutex.Unlock();
		}
	}

	return 0;
}

void CFamiTrackerView::GetRow(CFamiTrackerDoc *pDaoc)
{
	stChanNote NoteData;
	CFamiTrackerDoc* pDoc = GetDocument();

	for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
		if (!m_bMuteChannels[i]) {
			pDoc->GetNoteData(m_pPatternView->GetFrame(), i, m_pPatternView->GetRow(), &NoteData);
			FeedNote(i, &NoteData);
		}
	}
}

unsigned int CFamiTrackerView::GetSelectedFrame() const 
{ 
	return m_pPatternView->GetFrame(); 
}

unsigned int CFamiTrackerView::GetPlayFrame() const
{ 
	return m_pPatternView->GetPlayFrame();
}

unsigned int CFamiTrackerView::GetSelectedChannel() const 
{ 
	return m_pPatternView->GetChannel();
}

bool CFamiTrackerView::GetFollowMode() const
{ 
	return m_bFollowMode; 
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::UpdateEditor(LPARAM lHint)
{
	//CFamiTrackerDoc* pDoc = GetDocument();
	//ASSERT_VALID(pDoc);
	//pDoc->UpdateAllViews(NULL, lHint);
	OnUpdate(NULL, lHint, NULL);
}

void CFamiTrackerView::SetMessageText(LPCSTR lpszText)
{
	CFrameWnd *pFrame = GetParentFrame();

	if (pFrame)
		pFrame->SetMessageText(lpszText);
}

// Kill this method eventually
void CFamiTrackerView::ForceRedraw()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pDoc->UpdateAllViews(NULL);
}

void CFamiTrackerView::RemoveWithoutDelete()
{
	m_pPatternView->AddUndo();
	m_pPatternView->RemoveSelectedNotes();

	ForceRedraw();
}

void CFamiTrackerView::RegisterKeyState(int Channel, int Note)
{
	if (Channel == m_pPatternView->GetChannel())
		m_iKeyboardNote = Note;
}

void CFamiTrackerView::GetStartSpeed()
{
	unsigned int TempoAccum;
	CFamiTrackerDoc* pDoc = GetDocument();

	TempoAccum		= 60 * pDoc->GetFrameRate();		// 60 as in 60 seconds per minute
	m_iTickPeriod	= pDoc->GetSongSpeed();
	m_iSpeed		= DEFAULT_SPEED;
	
	m_iTempo = TempoAccum / 24;

	if (m_iTickPeriod > 20)
		m_iTempo = m_iTickPeriod;
	else
		m_iSpeed = m_iTickPeriod;
}

void CFamiTrackerView::SelectFrame(unsigned int Frame)
{
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Frame < GetDocument()->GetFrameCount());
	m_pPatternView->MoveToFrame(Frame);
	ForceRedraw();
}

void CFamiTrackerView::SelectNextFrame()
{
	m_pPatternView->NextFrame();
	ForceRedraw();
}

void CFamiTrackerView::SelectPrevFrame()
{
	m_pPatternView->PreviousFrame();
	ForceRedraw();
}

void CFamiTrackerView::SelectFirstFrame()
{
	m_pPatternView->MoveToFrame(0);
	ForceRedraw();
}

void CFamiTrackerView::SelectLastFrame()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_pPatternView->MoveToFrame(pDoc->GetFrameCount() - 1);
	ForceRedraw();
}

void CFamiTrackerView::SelectChannel(unsigned int Channel)
{
	ASSERT(Channel < MAX_CHANNELS);
	m_pPatternView->MoveToChannel(Channel);
	ForceRedraw();
}


void CFamiTrackerView::MoveCursorNextChannel()
{
	m_pPatternView->NextChannel();
	UpdateEditor(UPDATE_CURSOR);
}

void CFamiTrackerView::MoveCursorPrevChannel()
{
	m_pPatternView->PreviousChannel();
	UpdateEditor(UPDATE_CURSOR);
}

void CFamiTrackerView::ToggleChannel(unsigned int Channel)
{
	m_bMuteChannels[Channel] = !m_bMuteChannels[Channel];
	HaltNote(Channel, true);
	UpdateEditor(CHANGED_HEADER);
}

void CFamiTrackerView::SoloChannel(unsigned int Channel)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (IsChannelSolo(Channel)) {
		// Revert channels
		for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
			m_bMuteChannels[i] = false;
		}
	}
	else {
		// Solo selected channel
		for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
			if (i != Channel) {
				m_bMuteChannels[i] = true;
				HaltNote(i, true);
			}
		}
		m_bMuteChannels[Channel] = false;
	}
	UpdateEditor(CHANGED_HEADER);
}

bool CFamiTrackerView::IsChannelSolo(unsigned int Channel)
{
	// Returns true if Channel is the only active channel 
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
		if (m_bMuteChannels[i] == false && i != Channel)
			return false;
	}
	return true;
}

bool CFamiTrackerView::IsChannelMuted(unsigned int Channel)
{
	return m_bMuteChannels[Channel];
}

void CFamiTrackerView::SetInstrument(int Instrument)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (Instrument < 0 || Instrument >= MAX_INSTRUMENTS)
		return;

	if (pDoc->IsInstrumentUsed(Instrument))
		m_iInstrument = Instrument;
}

const int ADD_MORE_DELAY = 20;
const int ADD_MORE_TIME = 200;

bool CheckRepeat()
{
	static UINT LastTime, RepeatCounter;
	UINT CurrentTime = GetTickCount();

	if ((CurrentTime - LastTime) < ADD_MORE_TIME) {
		if (RepeatCounter < ADD_MORE_DELAY)
			RepeatCounter++;
	}
	else {
		RepeatCounter = 0;
	}

	LastTime = CurrentTime;

	return RepeatCounter == ADD_MORE_DELAY;
}

void CFamiTrackerView::IncreaseCurrentPattern()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Frame = m_pPatternView->GetFrame();
	int Channel = m_pPatternView->GetChannel();
	int Add = (CheckRepeat() ? 4 : 1);

	if (m_bChangeAllPattern) {
		for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
			pDoc->IncreasePattern(Frame, i, Add);
		}
	}
	else
		pDoc->IncreasePattern(Frame, Channel, Add);
}

void CFamiTrackerView::DecreaseCurrentPattern()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Frame = m_pPatternView->GetFrame();
	int Channel = m_pPatternView->GetChannel();
	int Remove = (CheckRepeat() ? 4 : 1);

	if (m_bChangeAllPattern) {
		for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); i++) {
			pDoc->DecreasePattern(Frame, i, Remove);
		}
	}
	else
		pDoc->DecreasePattern(Frame, Channel, Remove);
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
	if (m_iRealKeyStepping)
		m_pPatternView->MoveDown(m_iRealKeyStepping);

	UpdateEditor(UPDATE_CURSOR);
}

void CFamiTrackerView::StopNote(int Channel)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote Note;
	
	memset(&Note, 0, sizeof(stChanNote));

//	Note.Vol = 0x10;
	Note.Note = HALT;

	if (m_bEditEnable) {
		m_pPatternView->AddUndo();
		pDoc->SetNoteData(m_pPatternView->GetFrame(), Channel, m_pPatternView->GetRow(), &Note);
		pDoc->SetModifiedFlag();
		ForceRedraw();
	}

	FeedNote(Channel, &Note);
}

void CFamiTrackerView::InsertNote(int Note, int Octave, int Channel, int Velocity)
{
	// Inserts a note

	stChanNote Cell;

	int Frame = m_pPatternView->GetFrame();
	int Row = m_pPatternView->GetRow();

//	if (m_bMuteChannels[Channel]) {
//		memset(&Cell, 0, sizeof(stChanNote));
//		Cell.Vol = 0x10;
//	}
//	else
		GetDocument()->GetNoteData(Frame, Channel, Row, &Cell);

	Cell.Note = Note;

	if (Note != HALT && Note != RELEASE) {
		Cell.Octave	= Octave;

		if (!m_bMaskInstrument)
			Cell.Instrument = m_iInstrument;

		if (Velocity < 128)
			Cell.Vol = (Velocity / 8);
	}	

	if (m_bEditEnable) {
		if (Note == HALT)
			m_iLastNote = -1;
		else if (Note == RELEASE)
			m_iLastNote = -2;
		else
			m_iLastNote = (Note - 1) + Octave * 12;
		m_pPatternView->AddUndo();
		GetDocument()->SetNoteData(Frame, Channel, Row, &Cell);
		if (m_pPatternView->GetColumn() == 0 && !theApp.IsPlaying() && m_iRealKeyStepping > 0)
			StepDown();
		UpdateEditor(UPDATE_ENTIRE);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Note playing routines
//////////////////////////////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerView::PlayNote(unsigned int Channel, unsigned int Note, unsigned int Octave, unsigned int Velocity)
{
	// Play a note in a channel
	stChanNote NoteData;

	memset(&NoteData, 0, sizeof(stChanNote));

	NoteData.Note		= Note;
	NoteData.Octave		= Octave;
	NoteData.Vol		= Velocity / 8;
	NoteData.Instrument	= GetInstrument();
	
	FeedNote(Channel, &NoteData);
}

void CFamiTrackerView::HaltNote(unsigned int Channel, bool HardHalt)
{
	// Stop a channel
	stChanNote NoteData;
	memset(&NoteData, 0, sizeof(stChanNote));

	if (HardHalt)
		NoteData.Note = HALT;
	else
		NoteData.Note = RELEASE;

	NoteData.Vol = 0x10;

	FeedNote(Channel, &NoteData);
}

void CFamiTrackerView::FeedNote(int Channel, stChanNote *NoteData)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	
	CTrackerChannel *pChannel = pDoc->GetChannel(Channel);

	pChannel->SetNote(*NoteData);
	theApp.GetMIDI()->WriteNote(Channel, NoteData->Note, NoteData->Octave, NoteData->Vol);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MIDI note handling functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////

// Play a note
void CFamiTrackerView::TriggerMIDINote(unsigned int Channel, unsigned int MidiNote, unsigned int Velocity, bool Insert)
{
	// Play a MIDI note
	unsigned int Octave = GET_OCTAVE(MidiNote) - 2;
	unsigned int Note = GET_NOTE(MidiNote);

	if (Octave > 7)
		return;

	m_iActiveNotes[Channel] = MidiNote;

	if (!theApp.m_pSettings->Midi.bMidiVelocity)
		Velocity = 127;

	if (Insert)
		InsertNote(Note, Octave, Channel, Velocity + 1);

	PlayNote(Channel, Note, Octave, Velocity /*+ 1*/);

	m_iAutoArpNotes[MidiNote] = 1;
	m_iAutoArpPtr = MidiNote;
	m_iLastAutoArpPtr = m_iAutoArpPtr;

	UpdateArpDisplay();

	m_iLastMIDINote = MidiNote;

	m_iAutoArpKeyCount = 0;

	for (int i = 0; i < 128; i++) {
		if (m_iAutoArpNotes[i] == 1)
			m_iAutoArpKeyCount++;
	}
}

// Cut the currently playing note
void CFamiTrackerView::CutMIDINote(unsigned int Channel, unsigned int MidiNote, bool InsertCut)
{
	// Cut a MIDI note
	unsigned int Octave = GET_OCTAVE(MidiNote);
	unsigned int Note = GET_NOTE(MidiNote);

	m_iActiveNotes[Channel] = 0;
	m_iAutoArpNotes[MidiNote] = 2;

	UpdateArpDisplay();

	// Cut note
	if (m_iLastMIDINote == MidiNote)
		HaltNote(Channel, true);

	if (InsertCut)
		InsertNote(HALT, 0, Channel, 0);
}

// Release the currently playing note
void CFamiTrackerView::ReleaseMIDINote(unsigned int Channel, unsigned int MidiNote, bool InsertCut)
{
	// Release a MIDI note
	unsigned int Octave = GET_OCTAVE(MidiNote);
	unsigned int Note = GET_NOTE(MidiNote);

	m_iActiveNotes[Channel] = 0;
	m_iAutoArpNotes[MidiNote] = 2;

	UpdateArpDisplay();

	// Cut note
	if (m_iLastMIDINote == MidiNote)
		HaltNote(Channel, false);

	if (InsertCut)
		InsertNote(RELEASE, 0, Channel, 0);
}

void CFamiTrackerView::UpdateArpDisplay()
{
	if (theApp.m_pSettings->Midi.bMidiArpeggio == true) {
		int Base = -1;
		char Text[256];
		strcpy(Text, _T("Arpeggio "));

		for (int i = 0; i < 128; i++) {
			if (m_iAutoArpNotes[i] == 1) {
				if (Base == -1)
					Base = i;
				sprintf(Text, _T("%s %i"), Text, i - Base);
			}
		}

		if (Base != -1)
			GetParentFrame()->SetMessageText(Text);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Tracker input routines
//////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// API keyboard handling routines
//

void CFamiTrackerView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{	
	// Called when a key is pressed
	 
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (GetFocus() != this)
		return;

	CAccelerator *pAccel = theApp.GetAccelerator();

	if (pAccel->GetAction(nChar) != 0)
		return;

	if (nChar >= VK_NUMPAD0 && nChar <= VK_NUMPAD9) {
		// Switch instrument
		if (m_pPatternView->GetColumn() == C_NOTE) {
			SetInstrument(nChar - VK_NUMPAD0);
			((CMainFrame*)GetParentFrame())->UpdateInstrumentIndex();
			return;
		}
	}
	
	switch (nChar) {
		// Shift
		case VK_SHIFT:
			m_pPatternView->ShiftPressed(true);
			m_bShiftPressed = true;
			break;
		// Control
		case VK_CONTROL:
			m_pPatternView->ControlPressed(true);
			m_bControlPressed = true;
			break;
		// Movement
		case VK_UP:
			m_pPatternView->MoveUp(m_iKeyStepping);
			UpdateEditor(UPDATE_CURSOR);
			break;
		case VK_DOWN:
			m_pPatternView->MoveDown(m_iKeyStepping);
			UpdateEditor(UPDATE_CURSOR);
			break;
		case VK_LEFT:
			m_pPatternView->MoveLeft();
			UpdateEditor(UPDATE_CURSOR);
			break;
		case VK_RIGHT:
			m_pPatternView->MoveRight();
			UpdateEditor(UPDATE_CURSOR);
			break;
		case VK_HOME:
			OnKeyHome();
			UpdateEditor(UPDATE_CURSOR);
			//ForceRedraw();
			break;
		case VK_END:
			m_pPatternView->MoveToBottom();
			UpdateEditor(UPDATE_CURSOR);
			//ForceRedraw();
			break;
		case VK_PRIOR:
			m_pPatternView->MovePageUp();
			UpdateEditor(UPDATE_CURSOR);
			//ForceRedraw();
			break;
		case VK_NEXT:
			m_pPatternView->MovePageDown();
			UpdateEditor(UPDATE_CURSOR);
			//ForceRedraw();
			break;
		case VK_TAB:	// Move between channels
			if (m_bShiftPressed)
				m_pPatternView->PreviousChannel();
			else
				m_pPatternView->NextChannel();
			UpdateEditor(UPDATE_CURSOR);
			break;

		// Pattern editing
		case VK_ADD:
			KeyIncreaseAction();
			break;
		case VK_SUBTRACT:
			KeyDecreaseAction();
			break;
		case VK_DELETE:
			DeleteKey();
			break;
		case VK_INSERT:
			InsertKey();
			break;
		case VK_BACK:
			BackKey();
			break;

		// Octaves
		case VK_F2: SetOctave(0); break;
		case VK_F3: SetOctave(1); break;
		case VK_F4: SetOctave(2); break;
		case VK_F5: SetOctave(3); break;
		case VK_F6: SetOctave(4); break;
		case VK_F7: SetOctave(5); break;
		case VK_F8: SetOctave(6); break;
		case VK_F9: SetOctave(7); break;

		default:
			HandleKeyboardInput(nChar);
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CFamiTrackerView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// This is called when a key + ALT is pressed

	if (nChar >= VK_NUMPAD0 && nChar <= VK_NUMPAD9) {
		SetStepping(nChar - VK_NUMPAD0);
		return;
	}

	CView::OnSysKeyDown(nChar, nRepCnt, nFlags);
}

void CFamiTrackerView::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch (nChar) {
		case VK_SHIFT:
			m_bShiftPressed = false;
			m_pPatternView->ShiftPressed(false);
			break;
		case VK_CONTROL:
			m_bControlPressed = false;
			m_pPatternView->ControlPressed(false);
			break;
	}

	CView::OnSysKeyUp(nChar, nRepCnt, nFlags);
}

void CFamiTrackerView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Called when a key is released

	switch (nChar) {
		case VK_SHIFT:
			m_bShiftPressed = false;
			m_pPatternView->ShiftPressed(false);
			break;
		case VK_CONTROL:
			m_bControlPressed = false;
			m_pPatternView->ControlPressed(false);
			break;
	}

	HandleKeyboardNote(nChar, false);

	m_cKeyList[nChar] = 0;

	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

//
// Custom key handling routines
//

void CFamiTrackerView::OnKeyHome()
{
	if (m_bControlPressed) {
		m_pPatternView->MoveToTop();
	}
	else {
		if (m_pPatternView->GetColumn() != 0)
			m_pPatternView->MoveToColumn(0);
		else if (m_pPatternView->GetChannel() != 0)
			m_pPatternView->MoveToChannel(0);
		else if (m_pPatternView->GetRow() != 0)
			m_pPatternView->MoveToRow(0);
	}
	ForceRedraw();
}

void CFamiTrackerView::InsertKey()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Frame = m_pPatternView->GetFrame();
	int Row = m_pPatternView->GetRow();
	int Channel = m_pPatternView->GetChannel();
	int Column = m_pPatternView->GetColumn();

	if (PreventRepeat(VK_INSERT, true))
		return;

	m_pPatternView->AddUndo();

	pDoc->InsertNote(Frame, Channel, Row);
	ForceRedraw();
}

void CFamiTrackerView::BackKey()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Frame = m_pPatternView->GetFrame();
	int Row = m_pPatternView->GetRow();
	int Channel = m_pPatternView->GetChannel();
	int Column = m_pPatternView->GetColumn();

	if (m_pPatternView->IsSelecting()) 
		RemoveWithoutDelete();
	else {
		if (Row == 0 || PreventRepeat(VK_BACK, true))
			return;
		m_pPatternView->AddUndo();
		if (pDoc->RemoveNote(Frame, Channel, Row))
			m_pPatternView->MoveUp(1);
	}
	ForceRedraw();
}

void CFamiTrackerView::DeleteKey()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	int Frame = m_pPatternView->GetFrame();
	int Row = m_pPatternView->GetRow();
	int Channel = m_pPatternView->GetChannel();
	int Column = m_pPatternView->GetColumn();
	int PatternLen = pDoc->GetPatternLength();

	if (PreventRepeat(VK_DELETE, true))
		return;

	if (m_pPatternView->IsSelecting()) {
		OnEditDelete();
	}
	else {
		m_pPatternView->AddUndo();
		if (pDoc->DeleteNote(Frame, Channel, Row, Column)) {
			if (theApp.m_pSettings->General.bPullUpDelete || m_bShiftPressed) {
				stChanNote Data;
				for (int i = Row; i < PatternLen - 1; i++) {	// Pull upp rows
					pDoc->GetNoteData(Frame, Channel, i + 1, &Data);
					pDoc->SetNoteData(Frame, Channel, i, &Data);
				}
				// Last note on pattern
				pDoc->RemoveNote(Frame, Channel, PatternLen);
			}
			else
				StepDown();

			UpdateEditor(UPDATE_EDIT);
		}
	}
}

void CFamiTrackerView::KeyIncreaseAction()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_pPatternView->AddUndo();

	int Frame = m_pPatternView->GetFrame();
	int Row = m_pPatternView->GetRow();
	int Channel = m_pPatternView->GetChannel();
	int Column = m_pPatternView->GetColumn();

	switch (Column) {
		case C_INSTRUMENT1:
		case C_INSTRUMENT2: 
			pDoc->IncreaseInstrument(Frame, Channel, Row); 	
			break;
		case C_VOLUME: 
			pDoc->IncreaseVolume(Frame, Channel, Row);	
			break;
		case C_EFF_NUM: case C_EFF_PARAM1: case C_EFF_PARAM2: 
			pDoc->IncreaseEffect(Frame, Channel, Row, 0);
			break;
		case C_EFF2_NUM: case C_EFF2_PARAM1: case C_EFF2_PARAM2: 
			pDoc->IncreaseEffect(Frame, Channel, Row, 1);
			break;
		case C_EFF3_NUM: case C_EFF3_PARAM1: case C_EFF3_PARAM2: 
			pDoc->IncreaseEffect(Frame, Channel, Row, 2);
			break;
		case C_EFF4_NUM: case C_EFF4_PARAM1: case C_EFF4_PARAM2: 
			pDoc->IncreaseEffect(Frame, Channel, Row, 3);
			break;
	}

	UpdateEditor(UPDATE_EDIT);
}

void CFamiTrackerView::KeyDecreaseAction()
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_pPatternView->AddUndo();

	int Frame = m_pPatternView->GetFrame();
	int Row = m_pPatternView->GetRow();
	int Channel = m_pPatternView->GetChannel();
	int Column = m_pPatternView->GetColumn();

	switch (Column) {
		case C_INSTRUMENT1:
		case C_INSTRUMENT2:
			pDoc->DecreaseInstrument(Frame, Channel, Row); 	
			break;
		case C_VOLUME: 
			pDoc->DecreaseVolume(Frame, Channel, Row);	
			break;
		case C_EFF_NUM: case C_EFF_PARAM1: case C_EFF_PARAM2: 
			pDoc->DecreaseEffect(Frame, Channel, Row, 0);
			break;
		case C_EFF2_NUM: case C_EFF2_PARAM1: case C_EFF2_PARAM2: 
			pDoc->DecreaseEffect(Frame, Channel, Row, 1);
			break;
		case C_EFF3_NUM: case C_EFF3_PARAM1: case C_EFF3_PARAM2: 
			pDoc->DecreaseEffect(Frame, Channel, Row, 2); 
			break;
		case C_EFF4_NUM: case C_EFF4_PARAM1: case C_EFF4_PARAM2: 					
			pDoc->DecreaseEffect(Frame, Channel, Row, 3); 
			break;
	}

	UpdateEditor(UPDATE_EDIT);
}

bool CFamiTrackerView::EditInstrumentColumn(stChanNote &Note, int Key)
{
	unsigned char Mask, Shift;
	int EditStyle = theApp.m_pSettings->General.iEditStyle;
	int Column = m_pPatternView->GetColumn();
	int Value;	

	if (!m_bEditEnable)
		return false;

	if (CheckClearKey(Key)) {
		Note.Instrument = MAX_INSTRUMENTS;	// Indicate no instrument selected
		if (EditStyle != EDIT_STYLE2)
			StepDown();
		return true;
	}
	else if (CheckRepeatKey(Key)) {
		Note.Instrument = m_iLastInstrument;
		SetInstrument(m_iLastInstrument);
		if (EditStyle != EDIT_STYLE2)
			StepDown();
		return true;
	}

	Value = ConvertKeyToHex(Key);

	if (Value == -1)
		return false;

	if (Column == C_INSTRUMENT1) {
		Mask = 0x0F;
		Shift = 4;
	}
	else {
		Mask = 0xF0;
		Shift = 0;
	}

	if (Note.Instrument == MAX_INSTRUMENTS)
		Note.Instrument = 0;

	switch (EditStyle) {
		case EDIT_STYLE1: // FT2
			Note.Instrument = (Note.Instrument & Mask) | (Value << Shift);
			StepDown();
			break;
		case EDIT_STYLE2: // MPT
			if (Note.Instrument == (MAX_INSTRUMENTS - 1))
				Note.Instrument = 0;
			Note.Instrument = ((Note.Instrument & 0x0F) << 4) | Value & 0x0F;
			break;
		case EDIT_STYLE3: // IT
			Note.Instrument = (Note.Instrument & Mask) | (Value << Shift);
			if (Column == C_INSTRUMENT1)
				m_pPatternView->MoveRight();
			else if (Column == C_INSTRUMENT2) {
				m_pPatternView->MoveLeft();
				StepDown();
			}
			break;
	}

	if (Note.Instrument > (MAX_INSTRUMENTS - 1))
		Note.Instrument = (MAX_INSTRUMENTS - 1);

	if (Value == 0x80)
		Note.Instrument = MAX_INSTRUMENTS;

	m_iLastInstrument = Note.Instrument;

	SetInstrument(m_iLastInstrument);

	return true;
}

bool CFamiTrackerView::EditVolumeColumn(stChanNote &Note, int Key)
{
	int EditStyle = theApp.m_pSettings->General.iEditStyle;
	int Value;

	if (!m_bEditEnable)
		return false;

	if (CheckClearKey(Key)) {
		Note.Vol = 0x10;
		if (EditStyle != EDIT_STYLE2)
			StepDown();
		return true;
	}
	else if (CheckRepeatKey(Key)) {
		Note.Vol = m_iLastVolume;
		if (EditStyle != EDIT_STYLE2)
			StepDown();
		return true;
	}

	Value = ConvertKeyToHex(Key);

	if (Value == -1)
		return false;

	if (Value == 0x80)
		Note.Vol = 0x10;
	else
		Note.Vol = Value;

	m_iLastVolume = Note.Vol;

	if (EditStyle != EDIT_STYLE2)
		StepDown();

	return true;
}

bool CFamiTrackerView::EditEffNumberColumn(stChanNote &Note, unsigned char nChar, int EffectIndex)
{
	int EditStyle = theApp.m_pSettings->General.iEditStyle;

	if (!m_bEditEnable)
		return false;

	if (CheckRepeatKey(nChar)) {
		Note.EffNumber[EffectIndex] = m_iLastEffect;
		Note.EffParam[EffectIndex] = m_iLastEffectParam;
		return true;
	}

	if (CheckClearKey(nChar)) {
		Note.EffNumber[EffectIndex] = EF_NONE;
		if (EditStyle != EDIT_STYLE2)
			StepDown();
		return true;
	}

	for (int i = 0; i < EF_COUNT; i++) {
		if (nChar == EFF_CHAR[i]) {
			int Effect = i + 1;
			Note.EffNumber[EffectIndex] = Effect;
			switch (EditStyle) {
				case EDIT_STYLE2:	// Modplug
					if (Effect == m_iLastEffect)
						Note.EffParam[EffectIndex] = m_iLastEffectParam;
					break;
				default:
					StepDown();
			}
			m_iLastEffect = Effect;
			return true;
		}
	}

	return false;
}

bool CFamiTrackerView::EditEffParamColumn(stChanNote &Note, int Key, int EffectIndex)
{
	int EditStyle = theApp.m_pSettings->General.iEditStyle;
	unsigned char Mask, Shift;
	int Column = m_pPatternView->GetColumn();
	int Value = ConvertKeyToHex(Key);

	if (!m_bEditEnable)
		return false;

	if (CheckRepeatKey(Key)) {
		Note.EffNumber[EffectIndex] = m_iLastEffect;
		Note.EffParam[EffectIndex] = m_iLastEffectParam;
		return true;
	}

	if (CheckClearKey(Key)) {
		Note.EffParam[EffectIndex] = 0;
		if (EditStyle != EDIT_STYLE2)
			StepDown();
		return true;
	}

	if (Value == -1 || Value == 0x80)
		return false;

	if (Column == C_EFF_PARAM1 || Column == C_EFF2_PARAM1 || Column == C_EFF3_PARAM1 || Column == C_EFF4_PARAM1) {
		Mask = 0x0F;
		Shift = 4;
	}
	else {
		Mask = 0xF0;
		Shift = 0;
	}
	
	switch (EditStyle) {
		case EDIT_STYLE1:	// FT2
			Note.EffParam[EffectIndex] = (Note.EffParam[EffectIndex] & Mask) | Value << Shift;
			StepDown();
			break;
		case EDIT_STYLE2:	// Modplug
			Note.EffParam[EffectIndex] = ((Note.EffParam[EffectIndex] & 0x0F) << 4) | Value & 0x0F;
			UpdateEditor(UPDATE_ENTIRE);	// Todo: find a more appropriate hint
			break;
		case EDIT_STYLE3:	// IT
			Note.EffParam[EffectIndex] = (Note.EffParam[EffectIndex] & Mask) | Value << Shift;
			if (Mask == 0x0F)
				m_pPatternView->MoveRight();
			else {
				m_pPatternView->MoveLeft();
				StepDown();
			}
			break;
	}

	m_iLastEffectParam = Note.EffParam[EffectIndex];

	return true;
}

void CFamiTrackerView::HandleKeyboardInput(char nChar)
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	static bool FirstChar = false;

	stChanNote Note;

	int EditStyle = theApp.m_pSettings->General.iEditStyle;
	int Index;

	int Frame = m_pPatternView->GetFrame();
	int Row = m_pPatternView->GetRow();
	int Channel = m_pPatternView->GetChannel();
	int Column = m_pPatternView->GetColumn();

	// Watch for repeating keys
	if (PreventRepeat(nChar, m_bEditEnable))
		return;

	TRACE("Note down: %i\n", (unsigned int)nChar);

	// Get the note data
	pDoc->GetNoteData(Frame, Channel, Row, &Note);

	// Make all effect columns look the same, save an index instead
	switch (Column) {
		case C_EFF_NUM:		Column = C_EFF_NUM;	Index = 0; break;
		case C_EFF2_NUM:	Column = C_EFF_NUM;	Index = 1; break;
		case C_EFF3_NUM:	Column = C_EFF_NUM;	Index = 2; break;
		case C_EFF4_NUM:	Column = C_EFF_NUM;	Index = 3; break;
		case C_EFF_PARAM1:	Column = C_EFF_PARAM1; Index = 0; break;
		case C_EFF2_PARAM1:	Column = C_EFF_PARAM1; Index = 1; break;
		case C_EFF3_PARAM1:	Column = C_EFF_PARAM1; Index = 2; break;
		case C_EFF4_PARAM1:	Column = C_EFF_PARAM1; Index = 3; break;
		case C_EFF_PARAM2:	Column = C_EFF_PARAM2; Index = 0; break;
		case C_EFF2_PARAM2:	Column = C_EFF_PARAM2; Index = 1; break;
		case C_EFF3_PARAM2:	Column = C_EFF_PARAM2; Index = 2; break;
		case C_EFF4_PARAM2:	Column = C_EFF_PARAM2; Index = 3; break;			
	}

	switch (Column) {
		case C_NOTE:
			if (CheckRepeatKey(nChar)) {
				if (m_iLastNote == 0) {
					// Clear
					Note.Note = 0;
				}
				else if (m_iLastNote == -1) {
					Note.Note = HALT;
				}
				else if (m_iLastNote == -2) {
					Note.Note = RELEASE;
				}
				else {
					Note.Note = GET_NOTE(m_iLastNote);
					Note.Octave = GET_OCTAVE(m_iLastNote);
				}
			}
			else if (CheckClearKey(nChar)) {
				// Remove note
				Note.Note = 0;
				Note.Octave = 0;
				m_iLastNote = 0;
				if (EditStyle != EDIT_STYLE2)
					StepDown();
			}
			else {
				// This is special
				HandleKeyboardNote(nChar, true);
				return;
			}
			break;
		case C_INSTRUMENT1:
		case C_INSTRUMENT2:
			if (!EditInstrumentColumn(Note, nChar))
				return;
			break;
		case C_VOLUME:
			if (!EditVolumeColumn(Note, nChar))
				return;
			break;
		case C_EFF_NUM:
			if (!EditEffNumberColumn(Note, nChar, Index))
				return;
			break;
		case C_EFF_PARAM1:
		case C_EFF_PARAM2:
			if (!EditEffParamColumn(Note, nChar, Index))
				return;
			break;
	}

	// If something was edited, store it in the document again
	if (m_bEditEnable) {
		// Save undo state
		m_pPatternView->AddUndo();
		// Store it
		pDoc->SetNoteData(Frame, Channel, Row, &Note);
		// Update window
		UpdateEditor(UPDATE_ENTIRE);
	}
}

bool CFamiTrackerView::DoRelease() const
{
	// Return true if there are a valid release sequence for selected instrument
	CInstrument *pInst = GetDocument()->GetInstrument(GetInstrument());

	if (pInst) {
		switch (pInst->GetType()) {
			case INST_2A03:
				if (((CInstrument2A03*)pInst)->GetSeqEnable(0) != 0) {
					int index = ((CInstrument2A03*)pInst)->GetSeqIndex(0);
					if (GetDocument()->GetSequence(index, 0)->GetReleasePoint() != -1)
						return true;
				}
				break;
			case INST_VRC6:
				if (((CInstrumentVRC6*)pInst)->GetSeqEnable(0) != 0) {
					int index = ((CInstrumentVRC6*)pInst)->GetSeqIndex(0);
					if (GetDocument()->GetSequence(index, 0)->GetReleasePoint() != -1)
						return true;
				}
				break;
			case INST_FDS:
				if (((CInstrumentFDS*)pInst)->GetVolumeSeq()->GetItemCount() > 0) {
					if (((CInstrumentFDS*)pInst)->GetVolumeSeq()->GetReleasePoint() != -1) {
						return true;
					}
				}
				break;
			case INST_VRC7:
				return true;
		}
	}

	return false;
}

void CFamiTrackerView::HandleKeyboardNote(char nChar, bool Pressed) 
{
	// Play a note from the keyboard
	int Note = TranslateKey(nChar);
	
	if (Pressed) {
		if (CheckHaltKey(nChar))
			CutMIDINote(m_pPatternView->GetChannel(), Note, true);
		else if (CheckReleaseKey(nChar))										// Under development
			ReleaseMIDINote(m_pPatternView->GetChannel(), Note, true);			// Under development
		else {
			// Invalid key
			if (Note == -1)
				return;
			TriggerMIDINote(m_pPatternView->GetChannel(), Note, 0x7F, m_bEditEnable);
		}
	}
	else {
		if (Note == -1)
			return;
		// IT doesn't cut the note when key is released
		if (theApp.m_pSettings->General.iEditStyle != EDIT_STYLE3) {
			// Find if note release should be used
			// Todo: make this an option instead?
			if (DoRelease())
				ReleaseMIDINote(m_pPatternView->GetChannel(), Note, false);	
			else
				CutMIDINote(m_pPatternView->GetChannel(), Note, false);
		}
		else {
			m_iActiveNotes[m_pPatternView->GetChannel()] = 0;
			m_iAutoArpNotes[Note] = 2;
		}
	}
}

bool CFamiTrackerView::CheckClearKey(unsigned char Key)
{
	return (Key == theApp.m_pSettings->Keys.iKeyClear);
}

bool CFamiTrackerView::CheckReleaseKey(unsigned char Key)
{
	return (Key == theApp.m_pSettings->Keys.iKeyNoteRelease);
}

bool CFamiTrackerView::CheckHaltKey(unsigned char Key)
{
	if (Key == theApp.m_pSettings->Keys.iKeyNoteCut)
		return true;

	return false;
}

bool CFamiTrackerView::CheckRepeatKey(unsigned char Key)
{
	return (Key == theApp.m_pSettings->Keys.iKeyRepeat);
}

// Todo: fix azerty keys
int CFamiTrackerView::TranslateKeyAzerty(unsigned char Key)
{
	// Translates a keyboard character into a MIDI note
	int	KeyNote = 0, KeyOctave = 0;

	// For modplug users
	if (theApp.m_pSettings->General.iEditStyle == EDIT_STYLE2)
		return TranslateKey2(Key);

	// Convert key to a note
	switch (Key) {
		case 50:	KeyNote = Cs;	KeyOctave = m_iOctave + 1;	break;	// 2
		case 51:	KeyNote = Ds;	KeyOctave = m_iOctave + 1;	break;	// 3
		case 53:	KeyNote = Fs;	KeyOctave = m_iOctave + 1;	break;	// 5
		case 54:	KeyNote = Gs;	KeyOctave = m_iOctave + 1;	break;	// 6
		case 55:	KeyNote = As;	KeyOctave = m_iOctave + 1;	break;	// 7
		case 57:	KeyNote = Cs;	KeyOctave = m_iOctave + 2;	break;	// 9
		case 48:	KeyNote = Ds;	KeyOctave = m_iOctave + 2;	break;	// 0
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
		case 219:	KeyNote = Fs;	KeyOctave = m_iOctave + 2;	break;	// ´
		//case 186:	KeyNote = G;	KeyOctave = m_iOctave + 2;	break;	// ¨
		case 83:	KeyNote = Cs;	KeyOctave = m_iOctave;		break;	// S
		case 68:	KeyNote = Ds;	KeyOctave = m_iOctave;		break;	// D
		case 71:	KeyNote = Fs;	KeyOctave = m_iOctave;		break;	// G
		case 72:	KeyNote = Gs;	KeyOctave = m_iOctave;		break;	// H
		case 74:	KeyNote = As;	KeyOctave = m_iOctave;		break;	// J
		case 76:	KeyNote = Cs;	KeyOctave = m_iOctave + 1;	break;	// L
		case 192:	KeyNote = Ds;	KeyOctave = m_iOctave + 1;	break;	// Ö
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

	// Invalid
	if (KeyNote == 0)
		return -1;

	// Return a MIDI note
	return MIDI_NOTE(KeyOctave, KeyNote);		
}

int CFamiTrackerView::TranslateKey2(unsigned char Key)
{
	// Translates a keyboard character into a MIDI note
	int	KeyNote = 0, KeyOctave = 0;

	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	stChanNote NoteData;

	pDoc->GetNoteData(m_pPatternView->GetFrame(), m_pPatternView->GetChannel(), m_pPatternView->GetRow(), &NoteData);	

	// Convert key to a note, Modplug style
	switch (Key) {
		case 49:	KeyNote = NoteData.Note;	KeyOctave = 0;	break;	// 1
		case 50:	KeyNote = NoteData.Note;	KeyOctave = 1;	break;	// 2
		case 51:	KeyNote = NoteData.Note;	KeyOctave = 2;	break;	// 3
		case 52:	KeyNote = NoteData.Note;	KeyOctave = 3;	break;	// 4
		case 53:	KeyNote = NoteData.Note;	KeyOctave = 4;	break;	// 5
		case 54:	KeyNote = NoteData.Note;	KeyOctave = 5;	break;	// 6
		case 55:	KeyNote = NoteData.Note;	KeyOctave = 6;	break;	// 7
		case 56:	KeyNote = NoteData.Note;	KeyOctave = 7;	break;	// 8
		case 57:	KeyNote = NoteData.Note;	KeyOctave = 7;	break;	// 9
		case 48:	KeyNote = NoteData.Note;	KeyOctave = 7;	break;	// 0

		case 81:	KeyNote = C;	KeyOctave = m_iOctave;	break;	// Q
		case 87:	KeyNote = Cs;	KeyOctave = m_iOctave;	break;	// W
		case 69:	KeyNote = D;	KeyOctave = m_iOctave;	break;	// E
		case 82:	KeyNote = Ds;	KeyOctave = m_iOctave;	break;	// R
		case 84:	KeyNote = E;	KeyOctave = m_iOctave;	break;	// T
		case 89:	KeyNote = F;	KeyOctave = m_iOctave;	break;	// Y
		case 85:	KeyNote = Fs;	KeyOctave = m_iOctave;	break;	// U
		case 73:	KeyNote = G;	KeyOctave = m_iOctave;	break;	// I
		case 79:	KeyNote = Gs;	KeyOctave = m_iOctave;	break;	// O
		case 80:	KeyNote = A;	KeyOctave = m_iOctave;	break;	// P
		case 221:	KeyNote = As;	KeyOctave = m_iOctave;	break;	// Å
		case 219:	KeyNote = B;	KeyOctave = m_iOctave;	break;	// ´

		case 65:	KeyNote = C;	KeyOctave = m_iOctave + 1;	break;	// A
		case 83:	KeyNote = Cs;	KeyOctave = m_iOctave + 1;	break;	// S
		case 68:	KeyNote = D;	KeyOctave = m_iOctave + 1;	break;	// D
		case 70:	KeyNote = Ds;	KeyOctave = m_iOctave + 1;	break;	// F
		case 71:	KeyNote = E;	KeyOctave = m_iOctave + 1;	break;	// G
		case 72:	KeyNote = F;	KeyOctave = m_iOctave + 1;	break;	// H
		case 74:	KeyNote = Fs;	KeyOctave = m_iOctave + 1;	break;	// J
		case 75:	KeyNote = G;	KeyOctave = m_iOctave + 1;	break;	// K
		case 76:	KeyNote = Gs;	KeyOctave = m_iOctave + 1;	break;	// L
		case 192:	KeyNote = A;	KeyOctave = m_iOctave + 1;	break;	// Ö
		case 222:	KeyNote = As;	KeyOctave = m_iOctave + 1;	break;	// Ä
		case 191:	KeyNote = B;	KeyOctave = m_iOctave + 1;	break;	// '

		case 90:	KeyNote = C;	KeyOctave = m_iOctave + 2;	break;	// Z
		case 88:	KeyNote = Cs;	KeyOctave = m_iOctave + 2;	break;	// X
		case 67:	KeyNote = D;	KeyOctave = m_iOctave + 2;	break;	// C
		case 86:	KeyNote = Ds;	KeyOctave = m_iOctave + 2;	break;	// V
		case 66:	KeyNote = E;	KeyOctave = m_iOctave + 2;	break;	// B
		case 78:	KeyNote = F;	KeyOctave = m_iOctave + 2;	break;	// N
		case 77:	KeyNote = Fs;	KeyOctave = m_iOctave + 2;	break;	// M
		case 188:	KeyNote = G;	KeyOctave = m_iOctave + 2;	break;	// ,
		case 190:	KeyNote = Gs;	KeyOctave = m_iOctave + 2;	break;	// .
		case 189:	KeyNote = A;	KeyOctave = m_iOctave + 2;	break;	// -
	}

	// Invalid
	if (KeyNote == 0)
		return -1;

	// Return a MIDI note
	return MIDI_NOTE(KeyOctave, KeyNote);
}

int CFamiTrackerView::TranslateKey(unsigned char Key)
{
	// Translates a keyboard character into a MIDI note
	int	KeyNote = 0, KeyOctave = 0;

	// For modplug users
	if (theApp.m_pSettings->General.iEditStyle == EDIT_STYLE2)
		return TranslateKey2(Key);

	// Convert key to a note
	switch (Key) {
		case 50:	KeyNote = Cs;	KeyOctave = m_iOctave + 1;	break;	// 2
		case 51:	KeyNote = Ds;	KeyOctave = m_iOctave + 1;	break;	// 3
		case 53:	KeyNote = Fs;	KeyOctave = m_iOctave + 1;	break;	// 5
		case 54:	KeyNote = Gs;	KeyOctave = m_iOctave + 1;	break;	// 6
		case 55:	KeyNote = As;	KeyOctave = m_iOctave + 1;	break;	// 7
		case 57:	KeyNote = Cs;	KeyOctave = m_iOctave + 2;	break;	// 9
		case 48:	KeyNote = Ds;	KeyOctave = m_iOctave + 2;	break;	// 0
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
		case 219:	KeyNote = Fs;	KeyOctave = m_iOctave + 2;	break;	// ´
		//case 186:	KeyNote = G;	KeyOctave = m_iOctave + 2;	break;	// ¨
		case 83:	KeyNote = Cs;	KeyOctave = m_iOctave;		break;	// S
		case 68:	KeyNote = Ds;	KeyOctave = m_iOctave;		break;	// D
		case 71:	KeyNote = Fs;	KeyOctave = m_iOctave;		break;	// G
		case 72:	KeyNote = Gs;	KeyOctave = m_iOctave;		break;	// H
		case 74:	KeyNote = As;	KeyOctave = m_iOctave;		break;	// J
		case 76:	KeyNote = Cs;	KeyOctave = m_iOctave + 1;	break;	// L
		case 192:	KeyNote = Ds;	KeyOctave = m_iOctave + 1;	break;	// Ö
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

	// Invalid
	if (KeyNote == 0)
		return -1;

	// Return a MIDI note
	return MIDI_NOTE(KeyOctave, KeyNote);
}

bool CFamiTrackerView::PreventRepeat(unsigned char Key, bool Insert)
{
	if (m_cKeyList[Key] == 0)
		m_cKeyList[Key] = 1;
	else {
		if ((!theApp.m_pSettings->General.bKeyRepeat || !Insert))
			return true;
	}

	return false;
}

void CFamiTrackerView::RepeatRelease(unsigned char Key)
{
	memset(m_cKeyList, 0, 256);
}

//
// Note preview
//

void CFamiTrackerView::PreviewNote(unsigned char Key)
{
	if (PreventRepeat(Key, false))
		return;

	int Note = TranslateKey(Key);

	if (Note > 0)
		TriggerMIDINote(m_pPatternView->GetChannel(), Note, 0x7F, false);
}

void CFamiTrackerView::PreviewRelease(unsigned char Key)
{
	memset(m_cKeyList, 0, 256);

	int Note = TranslateKey(Key);

	if (Note > 0)
		CutMIDINote(m_pPatternView->GetChannel(), Note, false);
}

//
// MIDI in routines
//

void CFamiTrackerView::TranslateMidiMessage()
{
	// Check and handle MIDI messages

	unsigned char Message, Channel, Data1, Data2;
	static unsigned char LastNote, LastOctave;
	CString Status;

	CMIDI *pMIDI = theApp.GetMIDI();
	CFamiTrackerDoc* pDoc = GetDocument();

	if (!m_bInitialized)
		return;

	while (pMIDI->ReadMessage(Message, Channel, Data1, Data2)) {
		if (!theApp.m_pSettings->Midi.bMidiChannelMap)
			Channel = m_pPatternView->GetChannel();

		switch (Message) {
			case MIDI_MSG_NOTE_ON:
				if (Data2 == 0) {
					// MIDI key is released, don't input note break into pattern
					if (DoRelease())
						ReleaseMIDINote(Channel, Data1, false);
					else
						CutMIDINote(Channel, Data1, false);
				}
				else
					TriggerMIDINote(Channel, Data1, Data2, true);

				Status.Format(_T("MIDI message: Note on (note = %02i, octave = %02i, velocity = %02X)"), Data1 % 12, Data1 / 12, Data2);
				SetMessageText(Status);
				break;

			case MIDI_MSG_NOTE_OFF:
				CutMIDINote(Channel, Data1, false);
				Status.Format(_T("MIDI message: Note off"));
				SetMessageText(Status);
				break;
			
			case MIDI_MSG_PITCH_WHEEL: 
				{
					CTrackerChannel *pChannel = pDoc->GetChannel(Channel);
					int PitchValue = 0x2000 - ((Data1 & 0x7F) | ((Data2 & 0x7F) << 7));
					pChannel->SetPitch(-PitchValue / 0x10);
				}
				break;

			case 0x0F:
				if (Channel == 0x08)
					StepDown();
				break;
		}
	}
}

//
// Effects menu
//

void CFamiTrackerView::OnTrackerToggleChannel()
{
	if (m_iMenuChannel == -1)
		m_iMenuChannel = m_pPatternView->GetChannel();

	ToggleChannel(m_iMenuChannel);

	m_iMenuChannel = -1;
}

void CFamiTrackerView::OnTrackerSoloChannel()
{
	if (m_iMenuChannel == -1)
		m_iMenuChannel = m_pPatternView->GetChannel();

	SoloChannel(m_iMenuChannel);

	m_iMenuChannel = -1;
}

void CFamiTrackerView::OnNextOctave()
{
	if (m_iOctave < 7)
		SetOctave(m_iOctave + 1);
}

void CFamiTrackerView::OnPreviousOctave()
{
	if (m_iOctave > 0)
		SetOctave(m_iOctave - 1);
}

void CFamiTrackerView::OnPasteOverwrite()
{
	int SavedMode = m_iPasteMode;
	m_iPasteMode = PASTE_MODE_OVERWRITE;
	OnEditPaste();
	m_iPasteMode = SavedMode;
}

void CFamiTrackerView::OnPasteMixed()
{
	int SavedMode = m_iPasteMode;
	m_iPasteMode = PASTE_MODE_MIX;
	OnEditPaste();
	m_iPasteMode = SavedMode;
}

void CFamiTrackerView::OnNextInstrument()
{
	if (GetFocus() != this)
		return;
	SetInstrument(GetInstrument() + 1);
	((CMainFrame*)GetParentFrame())->UpdateInstrumentIndex();
}

void CFamiTrackerView::OnPrevInstrument()
{
	if (GetFocus() != this)
		return;
	SetInstrument(GetInstrument() - 1);
	((CMainFrame*)GetParentFrame())->UpdateInstrumentIndex();
}

void CFamiTrackerView::SetOctave(unsigned int iOctave)
{
	m_iOctave = iOctave;
	((CMainFrame*)GetParentFrame())->DisplayOctave();
}

int CFamiTrackerView::GetSelectedChipType() const
{
	CFamiTrackerDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	return pDoc->GetChipType(m_pPatternView->GetChannel());
}

void CFamiTrackerView::OnIncreaseStepSize()
{
	SetStepping(m_iKeyStepping + 1);
}

void CFamiTrackerView::OnDecreaseStepSize()
{
	SetStepping(m_iKeyStepping - 1);
}

void CFamiTrackerView::SetFollowMode(bool Mode)
{
	m_bFollowMode = Mode;
	m_pPatternView->SetFollowMove(Mode);
}

void CFamiTrackerView::SetStepping(int Step) 
{ 
	m_iRealKeyStepping = Step;

	if (Step > 0 && !theApp.m_pSettings->General.bNoStepMove) 
		m_iKeyStepping = Step; 
	else 
		m_iKeyStepping = 1;

	((CMainFrame*)GetParentFrame())->UpdateControls();
}

void CFamiTrackerView::OnEditInterpolate()
{
	m_pPatternView->Interpolate();
	ForceRedraw();
}

void CFamiTrackerView::OnEditReverse()
{
	m_pPatternView->Reverse();
	ForceRedraw();
}

void CFamiTrackerView::OnEditReplaceInstrument()
{
	m_pPatternView->ReplaceInstrument(GetInstrument());
	ForceRedraw();
}

void CFamiTrackerView::OnNcMouseMove(UINT nHitTest, CPoint point)
{
	if (m_pPatternView->OnMouseNcMove())
		ForceRedraw();

	CView::OnNcMouseMove(nHitTest, point);
}

void CFamiTrackerView::OnOneStepUp()
{
	m_pPatternView->MoveUp(1);
	UpdateEditor(UPDATE_CURSOR);	
}

void CFamiTrackerView::OnOneStepDown()
{
	m_pPatternView->MoveDown(1);
	UpdateEditor(UPDATE_CURSOR);
}

void CFamiTrackerView::MakeSilent()
{
	m_iAutoArpPtr		= 0; 
	m_iLastAutoArpPtr	= 0;
	m_iAutoArpKeyCount	= 0;

	memset(m_iActiveNotes, 0, sizeof(int) * MAX_CHANNELS);

	for (int i = 0; i < 256; i++)
		m_cKeyList[i] = 0;

	memset(Arpeggiate, 0, sizeof(int) * MAX_CHANNELS);
}

bool CFamiTrackerView::IsControlPressed() const
{
	return m_bControlPressed;
}

void CFamiTrackerView::SetFrameQueue(int Frame)
{
	m_iFrameQueue = Frame;
}

int CFamiTrackerView::GetFrameQueue() const 
{
	return m_iFrameQueue;
}

bool CFamiTrackerView::IsSelecting() const
{
	return m_pPatternView->IsSelecting();
}

bool CFamiTrackerView::IsClipboardAvailable() const
{
	return IsClipboardFormatAvailable(m_iClipBoard) == TRUE;
}
