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
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "FrameBoxWnd.h"
#include "PatternView.h"
#include "SoundGen.h"
#include "Settings.h"
#include "Graphics.h"

//
// CFrameBoxWnd - This is the frame editor to the left in the control panel
//

const TCHAR CFrameBoxWnd::DEFAULT_FONT[] = _T("System");

IMPLEMENT_DYNAMIC(CFrameBoxWnd, CWnd)

CFrameBoxWnd::CFrameBoxWnd(CMainFrame *pMainFrm):
	m_iFirstChannel(0),
	m_bInputEnable(false),
	m_bControl(false),
	m_pMainFrame(pMainFrm),
	m_pDocument(NULL),
	m_pView(NULL)
{
	memset(m_iCopiedValues, 0, MAX_CHANNELS * sizeof(int));
}

CFrameBoxWnd::~CFrameBoxWnd()
{
}

BEGIN_MESSAGE_MAP(CFrameBoxWnd, CWnd)
	ON_WM_PAINT()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_NCMOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KILLFOCUS()
	ON_WM_KEYDOWN()
	ON_WM_TIMER()
	ON_WM_RBUTTONUP()
	ON_WM_KEYUP()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_FRAME_COPY, OnEditCopy)
	ON_COMMAND(ID_FRAME_PASTE, OnEditPaste)
	ON_WM_CREATE()
END_MESSAGE_MAP()

void CFrameBoxWnd::AssignDocument(CFamiTrackerDoc *pDoc, CFamiTrackerView *pView)
{
	m_pDocument = pDoc;
	m_pView		= pView;
}

// CFrameBoxWnd message handlers

int CFrameBoxWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_hAccel = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));

	CreateGdiObjects();

	return 0;
}

void CFrameBoxWnd::CreateGdiObjects()
{
	m_Font.DeleteObject();
	m_Font.CreateFont(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, DEFAULT_FONT);
}

void CFrameBoxWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// Do not call CWnd::OnPaint() for painting messages

	const unsigned int QUEUE_COLOR = 0x108010;	// Colour of row in play queue

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
	int RowColor = BLEND(ColCursor, ColBackground, 50);

	if (m_bInputEnable)
		RowColor = 0xFE8090;

	// Draw selected row
	GradientBar(&m_dcBack, 0, SY((ItemsToDraw / 2) * ROW_HEIGHT + 3), Width, SY(16), RowColor, ColBackground);
	
	CString Text;

	for (int i = 0; i < ItemsToDraw; ++i) {

		if ((ActiveFrame - (ItemsToDraw / 2) + i) >= 0 && 
			(ActiveFrame - (ItemsToDraw / 2) + i) < FrameCount) {

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

			unsigned int CurrentColor;

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

void CFrameBoxWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
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

void CFrameBoxWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
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

void CFrameBoxWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	int FrameDelta, Channel;
	int NewFrame;

	ScaleMouse(point);

	FrameDelta	= ((point.y - TOP_OFFSET) / ROW_HEIGHT) - 4;
	Channel		= (point.x - 28) / 20;
	NewFrame	= m_pView->GetSelectedFrame() + FrameDelta;

	if ((NewFrame >= signed(m_pDocument->GetFrameCount())) || (NewFrame < 0))
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

void CFrameBoxWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	ScaleMouse(point);
	m_iHiglightLine = (point.y - TOP_OFFSET) / ROW_HEIGHT;
	RedrawWindow();
	CWnd::OnMouseMove(nFlags, point);
}

void CFrameBoxWnd::OnNcMouseMove(UINT nHitTest, CPoint point)
{
	m_iHiglightLine = -1;
	RedrawWindow();
	CWnd::OnNcMouseMove(nHitTest, point);
}

BOOL CFrameBoxWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: Add your message handler code here and/or call default
	// I cant get this to work
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CFrameBoxWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// Select channel and enable edit mode
	int FrameDelta, Channel;
	int NewFrame;

	ScaleMouse(point);

	FrameDelta	= ((point.y - TOP_OFFSET) / ROW_HEIGHT) - 4;
	Channel		= (point.x - 28) / 20;
	NewFrame	= m_pView->GetSelectedFrame() + FrameDelta;

	if ((NewFrame >= signed(m_pDocument->GetFrameCount())) || (NewFrame < 0))
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

void CFrameBoxWnd::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);
	m_bInputEnable = false;
	Invalidate();
	RedrawWindow();
}

void CFrameBoxWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
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
			case VK_CONTROL:
				m_bControl = true;
				break;
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
				if (m_bControl)
					m_pView->OnModuleMoveframeup();
				else {
					Frame = (Frame == 0) ? (m_pDocument->GetFrameCount() - 1) : (Frame - 1);
					m_pView->SelectFrame(Frame);
					m_iCursorPos = 0;
					m_iNewPattern = m_pDocument->GetPatternAtFrame(Frame, Channel);
				}
				break;
			case VK_DOWN:
				if (m_bControl)
					m_pView->OnModuleMoveframedown();
				else {
					Frame = (Frame == m_pDocument->GetFrameCount() - 1) ? 0 : (Frame + 1);
					m_pView->SelectFrame(Frame);
					m_iCursorPos = 0;
					m_iNewPattern = m_pDocument->GetPatternAtFrame(Frame, Channel);
				}
				break;
			case VK_RETURN:
				m_pView->SetFocus();
				break;
			case VK_INSERT:
				m_pView->OnModuleFrameInsert();
				break;
			case VK_DELETE:
				m_pView->OnModuleFrameRemove();
				break;
		}

		if (m_bControl) {
			// Control pressed
			if (nChar == 'C') {
				/*
				// Copy
				for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); ++i) {
					m_iCopiedValues[i] = pDoc->GetPatternAtFrame(Frame, i);
				}
				((CMainFrame*) pView->GetParentFrame())->SetStatusText("Copied frame values");
				*/
			}
			else if (nChar == 'V') {
				// Paste
				/*
				for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); ++i) {
					pDoc->SetPatternAtFrame(Frame, i, m_iCopiedValues[i]);
				}
				*/
			}
		}
		else {
			// Control not pressed
			if (Num != -1) {
				if (m_iCursorPos == 0)
					m_iNewPattern = (m_iNewPattern & 0x0F) | (Num << 4);
				else if (m_iCursorPos == 1)
					m_iNewPattern = (m_iNewPattern & 0xF0) | Num;

				if (m_iNewPattern >= MAX_PATTERN)
					m_iNewPattern = MAX_PATTERN - 1;

				if (m_pView->ChangeAllPatterns()) {
					for (unsigned int i = 0; i < m_pDocument->GetAvailableChannels(); i++)
						m_pDocument->SetPatternAtFrame(m_pView->GetSelectedFrame(), i, m_iNewPattern);
				}
				else
					m_pDocument->SetPatternAtFrame(m_pView->GetSelectedFrame(), m_pView->GetSelectedChannel(), m_iNewPattern);

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
		Invalidate();
		RedrawWindow();
	}

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CFrameBoxWnd::OnTimer(UINT_PTR nIDEvent)
{
	if (m_bInputEnable) {
		m_bCursor = !m_bCursor;
		Invalidate();
		RedrawWindow();
	}

	CWnd::OnTimer(nIDEvent);
}


BOOL CFrameBoxWnd::PreTranslateMessage(MSG* pMsg)
{
	if (TranslateAccelerator(m_hWnd, m_hAccel, pMsg))
		return TRUE;

	if (pMsg->message == WM_KEYDOWN) {
		OnKeyDown(pMsg->wParam, pMsg->lParam & 0xFFFF, pMsg->lParam & 0xFF0000);
		// Remove the beep
		pMsg->message = WM_NULL;
	}

	return CWnd::PreTranslateMessage(pMsg);
}

void CFrameBoxWnd::OnRButtonUp(UINT nFlags, CPoint point)
{
	// Popup menu

	// TODO: Move this to the context menu command

	CMenu *PopupMenu, PopupMenuBar;
	int Item = 0;

	int FrameDelta, Channel;
	unsigned int NewFrame;

	ScaleMouse(point);

	FrameDelta	= ((point.y - TOP_OFFSET) / ROW_HEIGHT) - 4;
	Channel		= (point.x - 28) / 20;
	NewFrame	= m_pView->GetSelectedFrame() + FrameDelta;

	if (NewFrame < 0)
		NewFrame = 0;
	if (NewFrame > (m_pDocument->GetFrameCount() - 1))
		NewFrame = m_pDocument->GetFrameCount() - 1;

	m_pView->SelectFrame(NewFrame);

	if (Channel >= 0)
		m_pView->SelectChannel(Channel);

	ClientToScreen(&point);
	PopupMenuBar.LoadMenu(IDR_FRAME_POPUP);
	PopupMenu = PopupMenuBar.GetSubMenu(0);
	PopupMenu->TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, GetParent());

	CWnd::OnRButtonUp(nFlags, point);
}

void CFrameBoxWnd::EnableInput()
{
	SetFocus();

	m_bInputEnable = true;
	m_bCursor = true;
	m_iCursorPos = 0;
	m_iNewPattern = m_pDocument->GetPatternAtFrame(m_pView->GetSelectedFrame(), m_pView->GetSelectedChannel());

	SetTimer(0, 500, NULL);	// Cursor timer
}

void CFrameBoxWnd::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_CONTROL) {
		m_bControl = false;
	}

	CWnd::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CFrameBoxWnd::OnEditCopy()
{
	if (this != GetFocus())
		return;

	int Frame = m_pView->GetSelectedFrame();

	for (unsigned int i = 0; i < m_pDocument->GetAvailableChannels(); ++i) {
		m_iCopiedValues[i] = m_pDocument->GetPatternAtFrame(Frame, i);
	}

	m_pMainFrame->SetStatusText(_T("Copied frame values"));

	Invalidate();
	RedrawWindow();
}

void CFrameBoxWnd::OnEditPaste()
{
	if (this != GetFocus())
		return;

	int Frame = m_pView->GetSelectedFrame();

	// Paste
	for (unsigned int i = 0; i < m_pDocument->GetAvailableChannels(); ++i) {
		m_pDocument->SetPatternAtFrame(Frame, i, m_iCopiedValues[i]);
	}

	m_pDocument->SetModifiedFlag();

	Invalidate();
	RedrawWindow();
}

bool CFrameBoxWnd::InputEnabled() const
{
	return m_bInputEnable;
}
