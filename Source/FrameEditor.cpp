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

#include "stdafx.h"
#include <cmath>
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "Accelerator.h"
#include "FrameEditor.h"
#include "PatternView.h"
#include "SoundGen.h"
#include "Settings.h"
#include "Graphics.h"

//
// CFrameEditor - This is the frame(order) editor to the left in the control panel
//

const TCHAR CFrameEditor::DEFAULT_FONT[] = _T("System");

IMPLEMENT_DYNAMIC(CFrameEditor, CWnd)

CFrameEditor::CFrameEditor(CMainFrame *pMainFrm):
	m_iFirstChannel(0),
	m_iFramesVisible(0),
	m_bInputEnable(false),
	m_pMainFrame(pMainFrm),
	m_pDocument(NULL),
	m_pView(NULL)
{
	memset(m_iCopiedValues, 0, MAX_CHANNELS * sizeof(int));
}

CFrameEditor::~CFrameEditor()
{
}

BEGIN_MESSAGE_MAP(CFrameEditor, CWnd)
	ON_WM_PAINT()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_NCMOUSEMOVE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KILLFOCUS()
	ON_WM_KEYDOWN()
	ON_WM_TIMER()
	ON_WM_RBUTTONUP()
	ON_COMMAND(ID_FRAME_COPY, OnFrameCopy)
	ON_COMMAND(ID_FRAME_PASTE, OnFramePaste)
	ON_COMMAND(ID_MODULE_INSERTFRAME, OnModuleInsertFrame)
	ON_COMMAND(ID_MODULE_REMOVEFRAME, OnModuleRemoveFrame)
	ON_COMMAND(ID_MODULE_DUPLICATEFRAME, OnModuleDuplicateFrame)
	ON_COMMAND(ID_MODULE_DUPLICATEFRAMEPATTERNS, OnModuleDuplicateFramePatterns)
	ON_COMMAND(ID_MODULE_MOVEFRAMEDOWN, OnModuleMoveFrameDown)
	ON_COMMAND(ID_MODULE_MOVEFRAMEUP, OnModuleMoveFrameUp)
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
	ON_WM_SIZE()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

void CFrameEditor::AssignDocument(CFamiTrackerDoc *pDoc, CFamiTrackerView *pView)
{
	m_pDocument = pDoc;
	m_pView		= pView;
}

// CFrameEditor message handlers

int CFrameEditor::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_hAccel = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_FRAMEWND));

	CreateGdiObjects();

	return 0;
}

void CFrameEditor::CreateGdiObjects()
{
	m_Font.DeleteObject();
	m_Font.CreateFont(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, DEFAULT_FONT);
}

void CFrameEditor::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// Do not call CWnd::OnPaint() for painting messages

	const COLORREF QUEUE_COLOR	  = 0x108010;	// Colour of row in play queue
	const COLORREF RED_BAR_COLOR  = 0x4030A0;

	// Cache settings
	const COLORREF ColBackground	= theApp.GetSettings()->Appearance.iColBackground;
	const COLORREF ColText			= theApp.GetSettings()->Appearance.iColPatternText;
	const COLORREF ColTextHilite	= theApp.GetSettings()->Appearance.iColPatternTextHilite;
	const COLORREF ColCursor		= theApp.GetSettings()->Appearance.iColCursor;
	const COLORREF ColCursor2		= DIM(theApp.GetSettings()->Appearance.iColCursor, 70);
	const bool bHexRows				= theApp.GetSettings()->General.bRowInHex;

	LPCSTR ROW_FORMAT = bHexRows ? _T("%02X") : _T("%02i");

	// Get window size
	CRect WinRect;
	GetClientRect(&WinRect);

	if (!m_pDocument || !m_pView)  {
		dc.FillSolidRect(WinRect, 0);
		return;
	}
	else if (!m_pDocument->IsFileLoaded()) {
		dc.FillSolidRect(WinRect, 0);
		return;
	}
	else if (theApp.GetSoundGenerator()->IsRendering())
		return;

	unsigned int Width = WinRect.Width();
	unsigned int Height = WinRect.Height();

	// Check if width has changed, delete objects then
	if (m_bmpBack.m_hObject != NULL) {
		CSize size = m_bmpBack.GetBitmapDimension();
		if (size.cx != Width) {
			m_bmpBack.DeleteObject();
			m_dcBack.DeleteDC();
		}
	}

	// Allocate object
	if (m_dcBack.m_hDC == NULL) {
		m_bmpBack.CreateCompatibleBitmap(&dc, Width, Height);
		m_bmpBack.SetBitmapDimension(Width, Height);
		m_dcBack.CreateCompatibleDC(&dc);
	}

	CBitmap *pOldBmp = m_dcBack.SelectObject(&m_bmpBack);
	CFont *pOldFont = m_dcBack.SelectObject(&m_Font);

	// Setup
	int ItemsToDraw = (Height - TOP_OFFSET) / ROW_HEIGHT;

	int ActiveFrame			= m_pView->GetSelectedFrame();
	int ActiveChannel		= m_pView->GetSelectedChannel();
	int FrameCount			= m_pDocument->GetFrameCount();
	int TotalChannelCount	= m_pDocument->GetAvailableChannels();
	int ChannelCount		= TotalChannelCount;

	if (ActiveChannel < m_iFirstChannel)
		m_iFirstChannel = ActiveChannel;
	if (ActiveChannel >= (m_iFirstChannel + (ChannelCount - 1)))
		m_iFirstChannel = ActiveChannel - (ChannelCount - 1);

	if (ActiveFrame > (FrameCount - 1))
		ActiveFrame = (FrameCount - 1);
	if (ActiveFrame < 0)
		ActiveFrame = 0;

	m_dcBack.SetBkMode(TRANSPARENT);

	// Draw background
	m_dcBack.FillSolidRect(0, 0, Width, Height, ColBackground);
	
	int Frame = 0;

	if (ActiveFrame > (ItemsToDraw / 2))
		Frame = ActiveFrame - (ItemsToDraw / 2);

	// Selected row
	COLORREF RowColor = BLEND(ColCursor, ColBackground, 50);

	if (m_bInputEnable)
		RowColor = BLEND(RED_BAR_COLOR, 0, 80);
//		RowColor = 0xFE8090;

	// Draw selected row
	GradientBar(&m_dcBack, 0, SY((ItemsToDraw / 2) * ROW_HEIGHT + 3), Width, SY(16), RowColor, ColBackground);
	
	CString Text;

	for (int i = 0; i < ItemsToDraw; ++i) {
		if ((ActiveFrame - (ItemsToDraw / 2) + i) >= 0 && (ActiveFrame - (ItemsToDraw / 2) + i) < FrameCount) {
			
			// Play cursor
			if (m_pView->GetPlayFrame() == Frame && !m_pView->GetFollowMode() && theApp.IsPlaying()) {
				GradientBar(&m_dcBack, 0, SY(i * ROW_HEIGHT + 4), SX(Width), SY(ROW_HEIGHT - 1), CPatternView::ROW_PLAY_COLOR, ColBackground);
			}

			// Queue cursor
			if (m_pView->GetFrameQueue() == Frame) {
				GradientBar(&m_dcBack, 0, SY(i * ROW_HEIGHT + 4), SX(Width), SY(ROW_HEIGHT - 1), QUEUE_COLOR, ColBackground);
			}

			if (i == ItemsToDraw / 2) {
				// Cursor box
				int x = ((ActiveChannel - m_iFirstChannel) * 20);
				int y = (ItemsToDraw / 2) * ROW_HEIGHT + 3;

				GradientBar(&m_dcBack, SX(28 + x), SY(y), SX(20), SY(16), ColCursor, ColBackground);
				m_dcBack.Draw3dRect(SX(28 + x), SY(y), SX(20), SY(16), BLEND(ColCursor, 0xFFFFFF, 90), BLEND(ColCursor, ColBackground, 60));

				if (m_bInputEnable && m_bCursor) {
					// Flashing black box indicating that input is active
					m_dcBack.FillSolidRect(SX(30 + x + 8 * m_iCursorPos), SY(y + 2), SX(8), SY(12), ColBackground);
				}
			}

			Text.Format(ROW_FORMAT, Frame); 
			m_dcBack.SetTextColor(ColTextHilite);
			m_dcBack.TextOut(SX(5), SY(i * ROW_HEIGHT + 3), Text);

			COLORREF CurrentColor;

			if (i == m_iHiglightLine || m_iHiglightLine == -1)
				CurrentColor = ColText;
			else
				CurrentColor = DIM(ColText, 90);

			for (int j = 0; j < ChannelCount; ++j) {
				int Chan = j + m_iFirstChannel;

				// Dim patterns that are different from current
				if (m_pDocument->GetPatternAtFrame(Frame, Chan) == m_pDocument->GetPatternAtFrame(ActiveFrame, Chan))
					m_dcBack.SetTextColor(CurrentColor);
				else
					m_dcBack.SetTextColor(DIM(CurrentColor, 70));

				Text.Format(_T("%02X"), m_pDocument->GetPatternAtFrame(Frame, Chan));
				m_dcBack.DrawText(Text, CRect(SX(30 + j * FRAME_ITEM_WIDTH), SY(i * ROW_HEIGHT + 3), SX(28 + j * FRAME_ITEM_WIDTH + FRAME_ITEM_WIDTH), SY(i * ROW_HEIGHT + 3 + 20)), DT_LEFT | DT_TOP | DT_NOCLIP);
			}

			Frame++;
		}
	}

	// Row number separator
	m_dcBack.FillSolidRect(SX(25), 0, SY(1), Height, 0x404040);
	m_dcBack.FillSolidRect(SX(25) - 1, 0, SY(1), Height, 0x606060);

	dc.BitBlt(0, 0, Width, Height, &m_dcBack, 0, 0, SRCCOPY);

	m_dcBack.SelectObject(pOldBmp);
	m_dcBack.SelectObject(pOldFont);

	if (FrameCount == 1)
		SetScrollRange(SB_VERT, 0, 1);
	else
		SetScrollRange(SB_VERT, 0, FrameCount - 1);
	
	SetScrollPos(SB_VERT, ActiveFrame);
	SetScrollRange(SB_HORZ, 0, TotalChannelCount - 1);
	SetScrollPos(SB_HORZ, ActiveChannel);
}

void CFrameEditor::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	switch (nSBCode) {
		case SB_ENDSCROLL:
			return;
		case SB_LINEDOWN:
		case SB_PAGEDOWN:
			m_pView->SelectNextFrame();
			break;
		case SB_PAGEUP:
		case SB_LINEUP:
			m_pView->SelectPrevFrame();
			break;
		case SB_TOP:
			m_pView->SelectFirstFrame();
			break;
		case SB_BOTTOM:
			m_pView->SelectLastFrame();
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			if (m_pDocument->GetFrameCount() > 1)
				m_pView->SelectFrame(nPos);
			break;	
	}

	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CFrameEditor::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	switch (nSBCode) {
		case SB_ENDSCROLL:
			return;
		case SB_LINERIGHT:
		case SB_PAGERIGHT:
			m_pView->MoveCursorNextChannel();
			break;
		case SB_PAGELEFT:
		case SB_LINELEFT:
			m_pView->MoveCursorPrevChannel();
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			m_pView->SelectChannel(nPos);
			break;	
	}

	CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CFrameEditor::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);
	m_bInputEnable = false;
	Invalidate();
	RedrawWindow();
	theApp.GetAccelerator()->SetAccelerator(NULL);
}

void CFrameEditor::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CMainFrame *pMainFrame = (CMainFrame*)GetParentFrame();
	
	int Num = -1;

	if (m_bInputEnable) {
		if (nChar > 47 && nChar < 58)		// 0 - 9
			Num = nChar - 48;
		else if (nChar >= VK_NUMPAD0 && nChar <= VK_NUMPAD9)
			Num = nChar - VK_NUMPAD0;
		else if (nChar > 64 && nChar < 71)	// A - F
			Num = nChar - 65 + 0x0A;

		int Channel = m_pView->GetSelectedChannel();
		int Frame = m_pView->GetSelectedFrame();

		switch (nChar) {
			case VK_LEFT:
				Channel = (Channel == 0) ? (m_pDocument->GetAvailableChannels() - 1) : (Channel - 1);
				m_pView->SelectChannel(Channel);
				m_iCursorPos = 0;
				m_iNewPattern = m_pDocument->GetPatternAtFrame(Frame, Channel);
				break;
			case VK_RIGHT:
				Channel = (Channel == m_pDocument->GetAvailableChannels() - 1) ? 0 : (Channel + 1);
				m_pView->SelectChannel(Channel);
				m_iCursorPos = 0;
				m_iNewPattern = m_pDocument->GetPatternAtFrame(Frame, Channel);
				break;
			case VK_UP:
				Frame = (Frame == 0) ? (m_pDocument->GetFrameCount() - 1) : (Frame - 1);
				m_pView->SelectFrame(Frame);
				m_iCursorPos = 0;
				m_iNewPattern = m_pDocument->GetPatternAtFrame(Frame, Channel);
				break;
			case VK_DOWN:
				Frame = (Frame == m_pDocument->GetFrameCount() - 1) ? 0 : (Frame + 1);
				m_pView->SelectFrame(Frame);
				m_iCursorPos = 0;
				m_iNewPattern = m_pDocument->GetPatternAtFrame(Frame, Channel);
				break;
			case VK_RETURN:
				m_pView->SetFocus();
				break;
			case VK_INSERT:
				OnModuleInsertFrame();
				break;
			case VK_DELETE:
				OnModuleRemoveFrame();
				break;
		}

		if (Num != -1) {
			if (m_iCursorPos == 0)
				m_iNewPattern = (m_iNewPattern & 0x0F) | (Num << 4);
			else if (m_iCursorPos == 1)
				m_iNewPattern = (m_iNewPattern & 0xF0) | Num;

			if (m_iNewPattern >= MAX_PATTERN)
				m_iNewPattern = MAX_PATTERN - 1;

			if (m_pView->ChangeAllPatterns()) {
				CFrameAction *pAction = new CFrameAction(CFrameAction::ACT_SET_PATTERN_ALL);
				pAction->SetPattern(m_iNewPattern);
				pMainFrame->AddAction(pAction);
			}
			else {
				CFrameAction *pAction = new CFrameAction(CFrameAction::ACT_SET_PATTERN);
				pAction->SetPattern(m_iNewPattern);
				pMainFrame->AddAction(pAction);
			}

			m_pDocument->SetModifiedFlag();

			m_iCursorPos++;
			m_bCursor = true;

			if (m_iCursorPos == 2) {
				if (m_pView->GetSelectedChannel() == m_pDocument->GetAvailableChannels() - 1)
					m_pView->SetFocus();
				else {
					m_pView->SelectChannel(m_pView->GetSelectedChannel() + 1);
					m_iNewPattern = m_pDocument->GetPatternAtFrame(m_pView->GetSelectedFrame(), m_pView->GetSelectedChannel());
					m_iCursorPos = 0;
				}
			}
		}
	}

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CFrameEditor::OnTimer(UINT_PTR nIDEvent)
{
	if (m_bInputEnable) {
		m_bCursor = !m_bCursor;
		Invalidate();
		RedrawWindow();
	}

	CWnd::OnTimer(nIDEvent);
}

BOOL CFrameEditor::PreTranslateMessage(MSG* pMsg)
{
	// Temporary fix, accelerated messages must be sent to the main window
	if (theApp.GetAccelerator()->Translate(theApp.m_pMainWnd->m_hWnd, pMsg)) {
		return TRUE;
	}

	if (pMsg->message == WM_KEYDOWN) {
		OnKeyDown(pMsg->wParam, pMsg->lParam & 0xFFFF, pMsg->lParam & 0xFF0000);
		// Remove the beep
		pMsg->message = WM_NULL;
	}

	return CWnd::PreTranslateMessage(pMsg);
}

void CFrameEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
	ScaleMouse(point);

	int FrameDelta	= ((point.y - TOP_OFFSET) / ROW_HEIGHT) - (m_iFramesVisible / 2);
	int Channel		= (point.x - 28) / 20;
	int NewFrame	= m_pView->GetSelectedFrame() + FrameDelta;
	int FrameCount  = m_pDocument->GetFrameCount();

	if ((NewFrame >= FrameCount) || (NewFrame < 0))
		return;

	if (m_pView->IsControlPressed() && theApp.IsPlaying()) {
		// Queue this frame
		if (NewFrame == m_pView->GetFrameQueue())
			// Remove
			m_pView->SetFrameQueue(-1);	
		else
			// Set new
			m_pView->SetFrameQueue(NewFrame);

		RedrawWindow();
	}
	else {
		// Switch to frame
		m_pView->SelectFrame(NewFrame);
		if (Channel >= 0)
			m_pView->SelectChannel(Channel);
	}

	CWnd::OnLButtonUp(nFlags, point);
}

void CFrameEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	ScaleMouse(point);
	int LastHighlightLine = m_iHiglightLine;
	m_iHiglightLine = (point.y - TOP_OFFSET) / ROW_HEIGHT;
	if (LastHighlightLine != m_iHiglightLine)
		RedrawWindow();
	CWnd::OnMouseMove(nFlags, point);
}

void CFrameEditor::OnNcMouseMove(UINT nHitTest, CPoint point)
{
	m_iHiglightLine = -1;
	RedrawWindow();
	CWnd::OnNcMouseMove(nHitTest, point);
}

void CFrameEditor::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// Select channel and enable edit mode
	ScaleMouse(point);

	int FrameDelta	= ((point.y - TOP_OFFSET) / ROW_HEIGHT) - (m_iFramesVisible / 2);
	int Channel		= (point.x - 28) / 20;
	int NewFrame	= m_pView->GetSelectedFrame() + FrameDelta;
	int FrameCount	= m_pDocument->GetFrameCount();

	if ((NewFrame >= FrameCount) || (NewFrame < 0))
		return;

	m_pView->SelectFrame(NewFrame);

	if (Channel >= 0)
		m_pView->SelectChannel(Channel);

	if (m_bInputEnable) {
		m_pView->SetFocus();
		return;
	}

	EnableInput();

	CWnd::OnLButtonDblClk(nFlags, point);
}

void CFrameEditor::OnRButtonUp(UINT nFlags, CPoint point)
{
	ScaleMouse(point);

	int FrameDelta	= ((point.y - TOP_OFFSET) / ROW_HEIGHT) - (m_iFramesVisible / 2);
	int Channel		= (point.x - 28) / 20;
	int NewFrame	= m_pView->GetSelectedFrame() + FrameDelta;
	int FrameCount	= m_pDocument->GetFrameCount();

	if (NewFrame < 0)
		NewFrame = 0;
	if (NewFrame >= FrameCount)
		NewFrame = FrameCount - 1;

	m_pView->SelectFrame(NewFrame);

	if (Channel >= 0)
		m_pView->SelectChannel(Channel);

	CWnd::OnRButtonUp(nFlags, point);
}

void CFrameEditor::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	// Popup menu
	CMenu *pPopupMenu, PopupMenuBar;
	PopupMenuBar.LoadMenu(IDR_FRAME_POPUP);
	pPopupMenu = PopupMenuBar.GetSubMenu(0);
	pPopupMenu->TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);
}

void CFrameEditor::EnableInput()
{
	SetFocus();

	m_bInputEnable = true;
	m_bCursor = true;
	m_iCursorPos = 0;
	m_iNewPattern = m_pDocument->GetPatternAtFrame(m_pView->GetSelectedFrame(), m_pView->GetSelectedChannel());

	theApp.GetAccelerator()->SetAccelerator(m_hAccel);

	SetTimer(0, 500, NULL);	// Cursor timer

	Invalidate();
	RedrawWindow();
}

void CFrameEditor::OnFrameCopy()
{
	int Frame = m_pView->GetSelectedFrame();
	int Channels = m_pDocument->GetAvailableChannels();

	for (int i = 0; i < Channels; ++i) {
		m_iCopiedValues[i] = m_pDocument->GetPatternAtFrame(Frame, i);
	}

	m_pMainFrame->SetStatusText(_T("Copied frame values"));
}

void CFrameEditor::OnFramePaste()
{
	CFrameAction *pAction = new CFrameAction(CFrameAction::ACT_PASTE);
	pAction->SetPasteData(m_iCopiedValues);
	m_pMainFrame->AddAction(pAction);
	m_pDocument->SetModifiedFlag();
}

bool CFrameEditor::InputEnabled() const
{
	return m_bInputEnable;
}

void CFrameEditor::OnModuleInsertFrame()
{
	m_pMainFrame->OnModuleInsertFrame();
}

void CFrameEditor::OnModuleRemoveFrame()
{
	m_pMainFrame->OnModuleRemoveFrame();
}

void CFrameEditor::OnModuleDuplicateFrame()
{
	m_pMainFrame->OnModuleDuplicateFrame();
}

void CFrameEditor::OnModuleDuplicateFramePatterns()
{
	m_pMainFrame->OnModuleDuplicateFramePatterns();
}

void CFrameEditor::OnModuleMoveFrameDown()
{
	m_pMainFrame->OnModuleMoveframedown();
}

void CFrameEditor::OnModuleMoveFrameUp()
{
	m_pMainFrame->OnModuleMoveframeup();
}

void CFrameEditor::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// Get number of rows visible
	m_iFramesVisible = (cy - TOP_OFFSET) / ROW_HEIGHT;

	// Delete the back buffer
	m_bmpBack.DeleteObject();
	m_dcBack.DeleteDC();
}

BOOL CFrameEditor::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (zDelta > 0) {
		// Up
		m_pView->SelectPrevFrame();
	}
	else {
		// Down
		m_pView->SelectNextFrame();
	}

	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

//
// CFrameAction ///////////////////////////////////////////////////////////////////
//

CFrameAction::CFrameAction(int iAction) : CAction(iAction)
{
}

CFrameAction::~CFrameAction()
{
}

void CFrameAction::SetFrameCount(unsigned int FrameCount)
{
	m_iNewFrameCount = FrameCount;
}

void CFrameAction::SetPattern(unsigned int Pattern)
{
	m_iNewPattern = Pattern;
}

void CFrameAction::SetPatternDelta(int Delta, bool ChangeAll)
{
	m_iPatternDelta = Delta;
	m_bChangeAll = ChangeAll;
}

void CFrameAction::SetPasteData(int *pData)
{
	for (int i = 0; i < MAX_CHANNELS; ++i) {
		m_iPasteData[i] = pData[i];
	}
}

void CFrameAction::SaveFrame(CFamiTrackerDoc *pDoc)
{
	for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); ++i) {
		m_iPatterns[i] = pDoc->GetPatternAtFrame(m_iUndoFramePos, i);
	}
}

void CFrameAction::RestoreFrame(CFamiTrackerDoc *pDoc)
{
	for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); ++i) {
		pDoc->SetPatternAtFrame(m_iUndoFramePos, i, m_iPatterns[i]);
	}
}

bool CFrameAction::SaveState(CMainFrame *pMainFrm)
{
	// Perform action

	CFrameEditor *pFrameEditor = pMainFrm->GetFrameEditor();
	CFamiTrackerView *pView = (CFamiTrackerView*)pMainFrm->GetActiveView();
	CFamiTrackerDoc *pDocument = pView->GetDocument();

	m_iUndoFramePos = pView->GetSelectedFrame();
	m_iUndoChannelPos = pView->GetSelectedChannel();

	switch (m_iAction) {
		case ACT_ADD:
			if (!pDocument->InsertFrame(m_iUndoFramePos + 1))
				return false;
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_REMOVE:
			SaveFrame(pDocument);
			if (!pDocument->RemoveFrame(m_iUndoFramePos))
				return false;
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_DUPLICATE:
			if (!pDocument->DuplicateFrame(m_iUndoFramePos))
				return false;
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_DUPLICATE_PATTERNS:
			if (!pDocument->DuplicatePatterns(m_iUndoFramePos + 1))
				return false;
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_CHANGE_COUNT:
			m_iUndoFrameCount = pDocument->GetFrameCount();
			pDocument->SetFrameCount(m_iNewFrameCount);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_SET_PATTERN:
			m_iOldPattern = pDocument->GetPatternAtFrame(m_iUndoFramePos, m_iUndoChannelPos);
			pDocument->SetPatternAtFrame(m_iUndoFramePos, m_iUndoChannelPos, m_iNewPattern);
			pDocument->UpdateAllViews(NULL, CHANGED_PATTERN);
			break;
		case ACT_SET_PATTERN_ALL:
			{
				int Channels = pDocument->GetAvailableChannels();
				for (int i = 0; i < Channels; ++i) {
					m_iPatterns[i] = pDocument->GetPatternAtFrame(m_iUndoFramePos, i);
					pDocument->SetPatternAtFrame(m_iUndoFramePos, i, m_iNewPattern);
				}
				pDocument->UpdateAllViews(NULL, CHANGED_PATTERN);
			}
			break;
		case ACT_CHANGE_PATTERN:
			{
				m_iOldPattern = pDocument->GetPatternAtFrame(m_iUndoFramePos, m_iUndoChannelPos);
				int NewPattern = m_iOldPattern + m_iPatternDelta;
				if (NewPattern < 0)
					NewPattern = 0;
				if (NewPattern >= MAX_FRAMES)
					NewPattern = MAX_FRAMES - 1;
				if (NewPattern == m_iOldPattern)
					return false;
				pDocument->SetPatternAtFrame(m_iUndoFramePos, m_iUndoChannelPos, NewPattern);
				pDocument->UpdateAllViews(NULL, CHANGED_PATTERN);
			}
			break;
		case ACT_CHANGE_PATTERN_ALL:
			{
				int Channels = pDocument->GetAvailableChannels();
				for (int i = 0; i < Channels; ++i) {
					m_iPatterns[i] = pDocument->GetPatternAtFrame(m_iUndoFramePos, i);
					if (m_iPatterns[i] + m_iPatternDelta < 0 || m_iPatterns[i] + m_iPatternDelta >= MAX_FRAMES)
						return false;
				}
				for (int i = 0; i < Channels; ++i)
					pDocument->SetPatternAtFrame(m_iUndoFramePos, i, m_iPatterns[i] + m_iPatternDelta);
				pDocument->UpdateAllViews(NULL, CHANGED_PATTERN);
			}
			break;
		case ACT_MOVE_DOWN:
			if (!pDocument->MoveFrameDown(m_iUndoFramePos))
				return false;
			pView->SelectFrame(m_iUndoFramePos + 1);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_MOVE_UP:
			if (!pDocument->MoveFrameUp(m_iUndoFramePos))
				return false;
			pView->SelectFrame(m_iUndoFramePos - 1);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_PASTE:
			SaveFrame(pDocument);
			{
				int Channels = pDocument->GetAvailableChannels();
				for (int i = 0; i < Channels; ++i)
					pDocument->SetPatternAtFrame(m_iUndoFramePos, i, m_iPasteData[i]);
			}
			pDocument->UpdateAllViews(NULL, CHANGED_PATTERN);
			break;
	}

	return true;
}

void CFrameAction::Undo(CMainFrame *pMainFrm)
{
	// Undo action

	CFrameEditor *pFrameEditor = pMainFrm->GetFrameEditor();
	CFamiTrackerView *pView = (CFamiTrackerView*)pMainFrm->GetActiveView();
	CFamiTrackerDoc *pDocument = pView->GetDocument();

	m_iRedoFramePos = pView->GetSelectedFrame();
	m_iRedoChannelPos = pView->GetSelectedChannel();

	pView->SelectFrame(m_iUndoFramePos);
	pView->SelectChannel(m_iUndoChannelPos);

	switch (m_iAction) {
		case ACT_ADD:
			pDocument->RemoveFrame(m_iUndoFramePos + 1);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_REMOVE:
			pDocument->InsertFrame(m_iUndoFramePos);
			RestoreFrame(pDocument);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_DUPLICATE:
			pDocument->RemoveFrame(m_iUndoFramePos);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_DUPLICATE_PATTERNS:
			for (unsigned int i = 0; i < pDocument->GetAvailableChannels(); ++i) {
				pDocument->ClearPattern(m_iUndoFramePos + 1, i);
			}
			pDocument->RemoveFrame(m_iUndoFramePos + 1);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_CHANGE_COUNT:
			pDocument->SetFrameCount(m_iUndoFrameCount);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_SET_PATTERN:
			pDocument->SetPatternAtFrame(m_iUndoFramePos, m_iUndoChannelPos, m_iOldPattern);
			pDocument->UpdateAllViews(NULL, CHANGED_PATTERN);
			pMainFrm->UpdateControls();
			break;
		case ACT_SET_PATTERN_ALL:
			for (unsigned int i = 0; i < pDocument->GetAvailableChannels(); ++i) {
				pDocument->SetPatternAtFrame(m_iUndoFramePos, i, m_iPatterns[i]);
			}
			pDocument->UpdateAllViews(NULL, CHANGED_PATTERN);
			break;
		case ACT_CHANGE_PATTERN:
			pDocument->SetPatternAtFrame(m_iUndoFramePos, m_iUndoChannelPos, m_iOldPattern);
			pDocument->UpdateAllViews(NULL, CHANGED_PATTERN);
			break;
		case ACT_CHANGE_PATTERN_ALL:
			for (unsigned int i = 0; i < pDocument->GetAvailableChannels(); ++i) {
				pDocument->SetPatternAtFrame(m_iUndoFramePos, i, m_iPatterns[i]);
			}
			pDocument->UpdateAllViews(NULL, CHANGED_PATTERN);
			break;
		case ACT_MOVE_DOWN:
			pDocument->MoveFrameUp(m_iUndoFramePos + 1);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_MOVE_UP:
			pDocument->MoveFrameDown(m_iUndoFramePos - 1);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_PASTE:
			RestoreFrame(pDocument);
			pDocument->UpdateAllViews(NULL, CHANGED_PATTERN);
			break;
	}

	pView->SelectFrame(m_iUndoFramePos);
	pView->SelectChannel(m_iUndoChannelPos);
}

void CFrameAction::Redo(CMainFrame *pMainFrm)
{
	// Redo action

	CFamiTrackerView *pView = (CFamiTrackerView*)pMainFrm->GetActiveView();
	CFamiTrackerDoc *pDocument = pView->GetDocument();

	switch (m_iAction) {
		case ACT_ADD:
			pDocument->InsertFrame(m_iUndoFramePos + 1);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_REMOVE:
			pDocument->RemoveFrame(m_iUndoFramePos);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_DUPLICATE:
			pDocument->DuplicateFrame(m_iUndoFramePos);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_DUPLICATE_PATTERNS:
			pDocument->DuplicatePatterns(m_iUndoFramePos + 1);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_CHANGE_COUNT:
			pDocument->SetFrameCount(m_iNewFrameCount);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_SET_PATTERN:
			pDocument->SetPatternAtFrame(m_iUndoFramePos, m_iUndoChannelPos, m_iNewPattern);
			pDocument->UpdateAllViews(NULL, CHANGED_PATTERN);
			pMainFrm->UpdateControls();
			break;
		case ACT_SET_PATTERN_ALL:
			{
				int Channels = pDocument->GetAvailableChannels();
				for (int i = 0; i < Channels; ++i) {
					pDocument->SetPatternAtFrame(m_iUndoFramePos, i, m_iNewPattern);
				}
				pDocument->UpdateAllViews(NULL, CHANGED_PATTERN);
			}
			break;
		case ACT_CHANGE_PATTERN:
			pDocument->SetPatternAtFrame(m_iUndoFramePos, m_iUndoChannelPos, m_iOldPattern + m_iPatternDelta);
			pDocument->UpdateAllViews(NULL, CHANGED_PATTERN);
			break;
		case ACT_CHANGE_PATTERN_ALL:
			{
				int Channels = pDocument->GetAvailableChannels();
				for (int i = 0; i < Channels; ++i) {
					pDocument->SetPatternAtFrame(m_iUndoFramePos, i, m_iPatterns[i] + m_iPatternDelta);
				}
				pDocument->UpdateAllViews(NULL, CHANGED_PATTERN);
			}
			break;
		case ACT_MOVE_DOWN:
			pDocument->MoveFrameDown(m_iUndoFramePos);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_MOVE_UP:
			pDocument->MoveFrameUp(m_iUndoFramePos);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		case ACT_PASTE:
			{
				int Channels = pDocument->GetAvailableChannels();
				for (int i = 0; i < Channels; ++i)
					pDocument->SetPatternAtFrame(m_iUndoFramePos, i, m_iPasteData[i]);
			}
			pDocument->UpdateAllViews(NULL, CHANGED_PATTERN);
			break;
	}

	pView->SelectFrame(m_iRedoFramePos);
	pView->SelectChannel(m_iRedoChannelPos);
}

void CFrameAction::Update(CMainFrame *pMainFrm)
{
	CFamiTrackerView *pView = (CFamiTrackerView*)pMainFrm->GetActiveView();
	CFamiTrackerDoc *pDocument = pView->GetDocument();

	switch (m_iAction) {
		case ACT_CHANGE_COUNT:
			pDocument->SetFrameCount(m_iNewFrameCount);
			pDocument->UpdateAllViews(NULL, CHANGED_FRAMES);
			break;
		// TODO add change pattern 
	}
}
