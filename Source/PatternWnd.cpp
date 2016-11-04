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
#include "PatternWnd.h"
#include ".\patternwnd.h"


// CPatternWnd

IMPLEMENT_DYNAMIC(CPatternWnd, CWnd)
CPatternWnd::CPatternWnd()
{
	Document = NULL;
}

CPatternWnd::~CPatternWnd()
{
}


BEGIN_MESSAGE_MAP(CPatternWnd, CWnd)
	ON_WM_PAINT()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

void CPatternWnd::SetDocument(CDocument *pDoc, CView *pView)
{
	Document = pDoc;
	View = pView;
}


// CPatternWnd message handlers


void CPatternWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	// Do not call CWnd::OnPaint() for painting messages

	const int WND_WIDTH = 163;

	CBrush	Brush, *OldBrush;
	CPen	Pen, *OldPen;
	CDC		dcBack;
	CBitmap	Bmp, *OldBmp;

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)Document;
	CFamiTrackerView *pView = (CFamiTrackerView*)View;

	CFont	Font, *OldFont;

	int ColBackground	= theApp.m_pSettings->Appearance.iColBackground;
	int ColText			= theApp.m_pSettings->Appearance.iColPatternText;
	int ColTextHilite	= theApp.m_pSettings->Appearance.iColPatternTextHilite;
	int ColCursor		= theApp.m_pSettings->Appearance.iColCursor;
	int ColCursor2		= DIM(theApp.m_pSettings->Appearance.iColCursor, 70);

	int ItemsToDraw;
	int PatternCount;
	int ChannelCount;
	int ActivePattern;
	int ActiveChannel;
	int Nr;

	char Text[256];
	int i, c;

	if (!Document)
		return;

	Bmp.CreateCompatibleBitmap(&dc, WND_WIDTH - 33, 140);
	dcBack.CreateCompatibleDC(&dc);
	OldBmp = dcBack.SelectObject(&Bmp);

	Font.CreateFont(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "System");

	ItemsToDraw = 9;

	ActivePattern	= pView->GetCurrentPattern();
	ActiveChannel	= pView->GetCurrentChannel();
	PatternCount	= pDoc->m_iFrameCount;
	ChannelCount	= pDoc->m_iChannelsAvailable;

	if (ActivePattern > (PatternCount - 1))
		ActivePattern = (PatternCount - 1);
	if (ActivePattern < 0)
		ActivePattern = 0;

	Brush.CreateSolidBrush(ColBackground);
	Pen.CreatePen(0, 0, ColBackground);

	OldBrush	= dcBack.SelectObject(&Brush);
	OldPen		= dcBack.SelectObject(&Pen);

	dcBack.Rectangle(0, 0, WND_WIDTH - 33, 140);
	dcBack.SetBkColor(ColBackground);

	OldFont = dcBack.SelectObject(&Font);

	if (ActivePattern > (ItemsToDraw / 2))
		Nr = ActivePattern - (ItemsToDraw / 2);
	else
		Nr = 0;
	
	dcBack.FillSolidRect(25 + (ActiveChannel * 20), (ItemsToDraw / 2) * 15 + 2, 20, 12, ColCursor);

	for (i = 0; i < ItemsToDraw; i++) {

		if ((ActivePattern - (ItemsToDraw / 2) + i) >= 0 && 
			(ActivePattern - (ItemsToDraw / 2) + i) < PatternCount) {

			if (theApp.m_pSettings->General.bRowInHex)
				sprintf(Text, "%02X", Nr);
			else
				sprintf(Text, "%02i", Nr);

			dcBack.SetTextColor(ColTextHilite);
			dcBack.SetBkColor(ColBackground);
			dcBack.TextOut(4, i * 15 + 0, Text);

			dcBack.SetTextColor(ColText);

			for (c = 0; c < ChannelCount; c++) {
				if (Nr == ActivePattern && c == ActiveChannel)
					dcBack.SetBkColor(ColCursor);
				else
					dcBack.SetBkColor(ColBackground);
				sprintf(Text, "%02i", pDoc->FrameList[Nr][c]);
				dcBack.TextOut(27 + c * 20, i * 15 + 0, Text);
			}
			Nr++;
		}
	}

	dcBack.Draw3dRect(2, (ItemsToDraw / 2) * 15 + 1, WND_WIDTH - 40, 14, ColCursor, ColCursor);
	dcBack.Draw3dRect(1, (ItemsToDraw / 2) * 15 + 0, WND_WIDTH - 38, 16, ColCursor2, ColCursor2);

	dcBack.SelectObject(OldBrush);
	dcBack.SelectObject(OldPen);

	dc.BitBlt(0, 0, WND_WIDTH - 33, 140, &dcBack, 0, 0, SRCCOPY);
	dcBack.SelectObject(OldBmp);

	dcBack.SelectObject(OldFont);

	if (PatternCount == 1)
		SetScrollRange(SB_VERT, 0, 1);
	else
		SetScrollRange(SB_VERT, 0, PatternCount - 1);
	
	SetScrollPos(SB_VERT, ActivePattern);
	SetScrollRange(SB_HORZ, 0, ChannelCount - 1);
	SetScrollPos(SB_HORZ, ActiveChannel);
}

void CPatternWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CFamiTrackerDoc		*pDoc = (CFamiTrackerDoc*)Document;
	CFamiTrackerView	*pView = (CFamiTrackerView*)View;

	switch (nSBCode) {
		case SB_ENDSCROLL:
			return;
		case SB_LINEDOWN:
		case SB_PAGEDOWN:
			pView->SelectNextPattern();
			break;
		case SB_PAGEUP:
		case SB_LINEUP:
			pView->SelectPrevPattern();
			break;
		case SB_TOP:
			pView->SelectFirstPattern();
			break;
		case SB_BOTTOM:
			pView->SelectLastPattern();
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			pView->SelectPattern(nPos);
			break;	
	}

	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CPatternWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)Document;
	CFamiTrackerView *pView = (CFamiTrackerView*)View;

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
