/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2009  Jonathan Liss
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
#include "FrameBoxWnd.h"
#include "SoundGen.h"

// CFrameBoxWnd

IMPLEMENT_DYNAMIC(CFrameBoxWnd, CWnd)
CFrameBoxWnd::CFrameBoxWnd()
{
//	Document = NULL;
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
END_MESSAGE_MAP()

// CFrameBoxWnd message handlers

int m_iFirstChannel = 0;

void CFrameBoxWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// Do not call CWnd::OnPaint() for painting messages

	CFamiTrackerDoc *pDoc = reinterpret_cast<CFamiTrackerDoc*>(theApp.GetDocument());
	CFamiTrackerView *pView = reinterpret_cast<CFamiTrackerView*>(theApp.GetView());

	CBrush	Brush, *OldBrush;
	CPen	Pen, *OldPen;
	CDC		dcBack;
	CBitmap	Bmp, *OldBmp;
	CFont	Font, *OldFont;
	CRect	WinRect;

	const int OFFSET_LEFT	= 0;
	const int OFFSET_TOP	= 0;

	unsigned int Width, Height;

	int ColBackground	= theApp.m_pSettings->Appearance.iColBackground;
	int ColText			= theApp.m_pSettings->Appearance.iColPatternText;
	int ColTextHilite	= theApp.m_pSettings->Appearance.iColPatternTextHilite;
	int ColCursor		= theApp.m_pSettings->Appearance.iColCursor;
	int ColCursor2		= DIM(theApp.m_pSettings->Appearance.iColCursor, 70);

	unsigned int i;

	int ItemsToDraw;
	int FrameCount, ChannelCount, TotalChannelCount;
	int ActiveFrame, ActiveChannel;
	int Nr;

	char Text[256];
	int c;

	DWORD dwVersion = GetVersion();

	GetClientRect(&WinRect);

	if (!pDoc || !pView) {
		dc.FillSolidRect(WinRect, 0);
		return;
	}

	if (!pDoc->IsFileLoaded())
		return;

	if (((CSoundGen*)theApp.GetSoundGenerator())->IsRendering())
		return;

	Width	= WinRect.right - WinRect.left;
	Height	= WinRect.bottom - WinRect.top;

	Bmp.CreateCompatibleBitmap(&dc, Width, Height);
	dcBack.CreateCompatibleDC(&dc);
	OldBmp = dcBack.SelectObject(&Bmp);

	Font.CreateFont(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "System");

	ItemsToDraw = 9;

	ActiveFrame			= pView->GetSelectedFrame();
	ActiveChannel		= pView->GetSelectedChannel();
	FrameCount			= pDoc->GetFrameCount();
	TotalChannelCount	= pDoc->GetAvailableChannels();

	ChannelCount		= TotalChannelCount;	// 5

	if (ActiveChannel < m_iFirstChannel)
		m_iFirstChannel = ActiveChannel;
	if (ActiveChannel >= (m_iFirstChannel + (ChannelCount - 1)))
		m_iFirstChannel = ActiveChannel - (ChannelCount - 1);

	if (ActiveFrame > (FrameCount - 1))
		ActiveFrame = (FrameCount - 1);
	if (ActiveFrame < 0)
		ActiveFrame = 0;

	Brush.CreateSolidBrush(ColBackground);
	Pen.CreatePen(0, 0, ColBackground);

	OldBrush = dcBack.SelectObject(&Brush);
	OldPen	 = dcBack.SelectObject(&Pen);

	dcBack.SetBkMode(TRANSPARENT);

	for (i = 0; i < Height; i++) {
		float Angle = (float) ((i * 100) / Height) * 3.14f * 2;
		int Level = (int)(cosf(Angle / 100.0f) * 20.0f);
		if (Level < 0)
			Level = 0;
		dcBack.FillSolidRect(0, i, Width, 1, DIM_TO(ColTextHilite, ColBackground, Level));
	}
	
	dcBack.SetBkColor(ColBackground);

	OldFont = dcBack.SelectObject(&Font);

	if (ActiveFrame > (ItemsToDraw / 2))
		Nr = ActiveFrame - (ItemsToDraw / 2);
	else
		Nr = 0;
	
	dcBack.FillSolidRect(28 + ((ActiveChannel - m_iFirstChannel) * 20), (ItemsToDraw / 2) * 15 + 5, 20, 12, ColCursor);

	unsigned int CurrentColor;
/*
	for (c = 0; c < ChannelCount; c++) {
		int Chan = c + m_iFirstChannel;
		dcBack.SetTextColor(0x4000FF);
		sprintf(Text, "%02i", Chan);
		dcBack.DrawText(Text, CRect(30 + c * 20, 3, 28 + c * 20 + 20, 3 + 20), DT_LEFT | DT_TOP | DT_NOCLIP);
	}
*/
	for (i = 0; i < (unsigned)ItemsToDraw; i++) {

		if ((ActiveFrame - (ItemsToDraw / 2) + (signed)i) >= 0 && 
			(ActiveFrame - (ItemsToDraw / 2) + (signed)i) < FrameCount) {

			// Play cursor
			if (pView->GetPlayFrame() == Nr && !pView->GetFollowMode() && theApp.IsPlaying()) {
				dcBack.FillSolidRect(0, i * 15 + 4, Width, 14, 0x200060);
			}

			if (theApp.m_pSettings->General.bRowInHex)
				sprintf(Text, "%02X", Nr);
			else
				sprintf(Text, "%02i", Nr);
			
			dcBack.SetTextColor(ColTextHilite);
			dcBack.TextOut(6, i * 15 + 3, Text);

			if (i == m_iHiglightLine || m_iHiglightLine == -1)
				CurrentColor = ColText;
			else
				CurrentColor = DIM(ColText, 90);

			for (c = 0; c < ChannelCount; c++) {
				int Chan = c + m_iFirstChannel;
				if (pDoc->GetPatternAtFrame(Nr, Chan) == pDoc->GetPatternAtFrame(ActiveFrame, Chan))
					dcBack.SetTextColor(CurrentColor);
				else
					dcBack.SetTextColor(DIM(CurrentColor, 70));

				sprintf(Text, "%02X", pDoc->GetPatternAtFrame(Nr, Chan));
				dcBack.DrawText(Text, CRect(30 + c * FRAME_ITEM_WIDTH, i * 15 + 3, 28 + c * FRAME_ITEM_WIDTH + FRAME_ITEM_WIDTH, i * 15 + 3 + 20), DT_LEFT | DT_TOP | DT_NOCLIP);
			}
			Nr++;
		}
	}

	dcBack.FillSolidRect(25, 0, 1, Height, 0x808080);

	dcBack.Draw3dRect(2, (ItemsToDraw / 2) * 15 + 4, Width - 4, 14, ColCursor, ColCursor);
	dcBack.Draw3dRect(1, (ItemsToDraw / 2) * 15 + 3, Width - 2, 16, ColCursor2, ColCursor2);

	dcBack.SelectObject(OldBrush);
	dcBack.SelectObject(OldPen);

	dc.BitBlt(0, 0, Width, Height, &dcBack, 0, 0, SRCCOPY);

	dcBack.SelectObject(OldBmp);
	dcBack.SelectObject(OldFont);

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
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(theApp.GetDocument());
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(theApp.GetView());

	switch (nSBCode) {
		case SB_ENDSCROLL:
			return;
		case SB_LINEDOWN:
		case SB_PAGEDOWN:
			pView->SelectNextFrame();
			break;
		case SB_PAGEUP:
		case SB_LINEUP:
			pView->SelectPrevFrame();
			break;
		case SB_TOP:
			pView->SelectFirstFrame();
			break;
		case SB_BOTTOM:
			pView->SelectLastFrame();
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			if (pDoc->GetFrameCount() > 1)
				pView->SelectFrame(nPos);
			break;	
	}

	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CFrameBoxWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(theApp.GetView());

	switch (nSBCode) {
		case SB_ENDSCROLL:
			return;
		case SB_LINERIGHT:
		case SB_PAGERIGHT:
			pView->MoveCursorNextChannel();
			break;
		case SB_PAGELEFT:
		case SB_LINELEFT:
			pView->MoveCursorPrevChannel();
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			pView->SelectChannel(nPos);
			break;	
	}

	CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CFrameBoxWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerDoc*>(theApp.GetDocument());
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(theApp.GetView());

	int FrameDelta, Channel;
	int NewFrame;

	FrameDelta	= ((point.y - 3) / 15) - 4;
	Channel		= (point.x - 28) / 20;
	NewFrame	= pView->GetSelectedFrame() + FrameDelta;

	if ((NewFrame >= signed(pDoc->GetFrameCount())) || (NewFrame < 0))
		return;

	pView->SelectFrame(NewFrame);

	if (Channel >= 0)
		pView->SelectChannel(Channel);

	SetFocus();

	CWnd::OnLButtonDown(nFlags, point);
}

void CFrameBoxWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	m_iHiglightLine = point.y / 15;
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
	// cant get this to work
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}
