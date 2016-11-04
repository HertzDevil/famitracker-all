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
#include "ConfigAppearance.h"
#include ".\configappearance.h"

//const int COLOR_ITEM_COUNT = 5;

enum E_COLOR_ITEMS {
	COL_BACKGROUND,
	COL_BACKGROUND_HILITE,
	COL_PATTERN_TEXT,
	COL_PATTERN_TEXT_HILITE,
	COL_SELECTION,
	COL_CURSOR,

	COLOR_ITEM_COUNT
};

const char *COLOR_ITEMS[] = {
	"Background", 
	"Higlighed background",
	"Pattern text", 
	"Highlighted pattern text", 
	"Selection",
	"Cursor"
};

const int NUM_COLOR_SCHEMES = 3;

const char *COLOR_SCHEMES[] = {
	"Default",
	"Monochrome",
	"Renoise"
};

CConfigAppearance *pCallback;

int CALLBACK EnumFontFamExProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam)
{
	if (lpelfe->elfLogFont.lfCharSet == ANSI_CHARSET && lpelfe->elfFullName[0] != '@')
		pCallback->AddFontName((char*)&lpelfe->elfFullName);

	return 1;
}

// CConfigAppearance dialog

IMPLEMENT_DYNAMIC(CConfigAppearance, CPropertyPage)
CConfigAppearance::CConfigAppearance()
	: CPropertyPage(CConfigAppearance::IDD)
{
}

CConfigAppearance::~CConfigAppearance()
{
}

void CConfigAppearance::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConfigAppearance, CPropertyPage)
	ON_WM_PAINT()
	ON_CBN_SELCHANGE(IDC_FONT, OnCbnSelchangeFont)
	ON_BN_CLICKED(IDC_PICK_COL, OnBnClickedPickCol)
	ON_CBN_SELCHANGE(IDC_COL_ITEM, OnCbnSelchangeColItem)
	ON_CBN_SELCHANGE(IDC_SCHEME, OnCbnSelchangeScheme)
END_MESSAGE_MAP()


// CConfigAppearance message handlers

void CConfigAppearance::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// Do not call CPropertyPage::OnPaint() for painting messages

	const char PREV_LINE[] = "--- -- - ---";

	CWnd *pWnd;
	CRect Rect, ParentRect;
	CBrush	BrushColor, *OldBrush;

	int ShadedCol, ShadedHiCol;

	GetWindowRect(ParentRect);

	pWnd = GetDlgItem(IDC_COL_PREVIEW);
	pWnd->GetWindowRect(Rect);

	Rect.top -= ParentRect.top;
	Rect.bottom -= ParentRect.top;
	Rect.left -= ParentRect.left;
	Rect.right -= ParentRect.left;

	switch (m_iSelectedItem) {
		case COL_BACKGROUND:
			BrushColor.CreateSolidBrush(m_iColBackground);
			break;
		case COL_BACKGROUND_HILITE:
			BrushColor.CreateSolidBrush(m_iColBackgroundHilite);
			break;
		case COL_PATTERN_TEXT:
			BrushColor.CreateSolidBrush(m_iColText);
			break;
		case COL_PATTERN_TEXT_HILITE:
			BrushColor.CreateSolidBrush(m_iColTextHilite);
			break;
		case COL_SELECTION:
			BrushColor.CreateSolidBrush(m_iColSelection);
			break;
		case COL_CURSOR:
			BrushColor.CreateSolidBrush(m_iColCursor);
			break;
	}

	OldBrush = dc.SelectObject(&BrushColor);

	dc.Rectangle(Rect);

	dc.SelectObject(OldBrush);

	// Preview all colors

	pWnd = GetDlgItem(IDC_PREVIEW);
	pWnd->GetWindowRect(Rect);

	Rect.top -= ParentRect.top;
	Rect.bottom -= ParentRect.top;
	Rect.left -= ParentRect.left;
	Rect.right -= ParentRect.left;

	CFont Font, *OldFont;
	LOGFONT LogFont;

	memset(&LogFont, 0, sizeof(LOGFONT));
	memcpy(LogFont.lfFaceName, m_strFont, m_strFont.GetLength());

	LogFont.lfHeight = -12;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;

	Font.CreateFontIndirect(&LogFont);

	OldFont = dc.SelectObject(&Font);

	dc.FillSolidRect(Rect, m_iColBackground);

	ShadedCol = DIM(m_iColText, 50);
	ShadedHiCol = DIM(m_iColTextHilite, 50);

	for (int j = 0; j < 3; j++) {

		for (int i = 0; i < 4; i++) {
			int OffsetTop = Rect.top + i * 18 + (j * 54);
			int OffsetLeft = Rect.left + 9;

			if (i == 0) {
				if (j == 0)
					dc.SetTextColor(m_iColTextHilite);
				else
					dc.SetTextColor(ShadedHiCol);
				dc.SetBkColor(m_iColBackgroundHilite);
				dc.FillSolidRect(Rect.left, OffsetTop, Rect.right - Rect.left, 18, m_iColBackgroundHilite);
				if (j == 0) {
					dc.SetBkColor(m_iColCursor);
					dc.FillSolidRect(Rect.left + 5, OffsetTop + 1, 40, 16, m_iColCursor);
				}
			}
			else {
				if (j == 0)
					dc.SetTextColor(m_iColText);
				else
					dc.SetTextColor(ShadedCol);
				dc.SetBkColor(m_iColBackground);
			}

			if (j == 0) {
				dc.TextOut(OffsetLeft, OffsetTop, "C");
				dc.TextOut(OffsetLeft + 12, OffsetTop, "-");
				dc.TextOut(OffsetLeft + 24, OffsetTop, "4");
			}
			else {
				dc.TextOut(OffsetLeft, OffsetTop, "-");
				dc.TextOut(OffsetLeft + 12, OffsetTop, "-");
				dc.TextOut(OffsetLeft + 24, OffsetTop, "-");
			}

			if (j == 0 && i == 0) {
				dc.SetBkColor(m_iColBackgroundHilite);
			}

			if (i == 0) {
				dc.SetTextColor(ShadedHiCol);
			}
			else {
				dc.SetTextColor(ShadedCol);
			}

			dc.TextOut(OffsetLeft + 40, OffsetTop, "-");
			dc.TextOut(OffsetLeft + 52, OffsetTop, "-");
			dc.TextOut(OffsetLeft + 68, OffsetTop, "-");
			dc.TextOut(OffsetLeft + 84, OffsetTop, "-");
			dc.TextOut(OffsetLeft + 96, OffsetTop, "-");
			dc.TextOut(OffsetLeft + 108, OffsetTop, "-");
		}
	}

	dc.SelectObject(OldFont);
}

BOOL CConfigAppearance::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CComboBox *ItemsBox;
	int i;

	CDC *pDC = GetDC();
	LOGFONT LogFont;

	m_strFont = theApp.m_pSettings->General.strFont;

	memset(&LogFont, 0, sizeof(LOGFONT));
	LogFont.lfCharSet = DEFAULT_CHARSET;

	m_pFontList = (CComboBox*)GetDlgItem(IDC_FONT);
	pCallback = this;

	EnumFontFamiliesEx(pDC->m_hDC, &LogFont, (FONTENUMPROC)EnumFontFamExProc, 0, 0);

	ReleaseDC(pDC);

	ItemsBox = (CComboBox*)GetDlgItem(IDC_COL_ITEM);

	for (i = 0; i < COLOR_ITEM_COUNT; i++) {
		ItemsBox->AddString(COLOR_ITEMS[i]);
	}

	ItemsBox->SelectString(0, COLOR_ITEMS[0]);

	m_iSelectedItem = 0;

	m_iColBackground		= theApp.m_pSettings->Appearance.iColBackground;
	m_iColBackgroundHilite	= theApp.m_pSettings->Appearance.iColBackgroundHilite;
	m_iColText				= theApp.m_pSettings->Appearance.iColPatternText;
	m_iColTextHilite		= theApp.m_pSettings->Appearance.iColPatternTextHilite;
	m_iColSelection			= theApp.m_pSettings->Appearance.iColSelection;
	m_iColCursor			= theApp.m_pSettings->Appearance.iColCursor;

	ItemsBox = (CComboBox*)GetDlgItem(IDC_SCHEME);

	for (i = 0; i < NUM_COLOR_SCHEMES; i++) {
		ItemsBox->AddString(COLOR_SCHEMES[i]);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigAppearance::AddFontName(char *Name)
{
	m_pFontList->AddString(Name);

	if (m_strFont.Compare(Name) == 0)
		m_pFontList->SelectString(0, Name);
}

BOOL CConfigAppearance::OnApply()
{
	theApp.m_pSettings->General.strFont = m_strFont;
	
	theApp.m_pSettings->Appearance.iColBackground			= m_iColBackground;
	theApp.m_pSettings->Appearance.iColBackgroundHilite		= m_iColBackgroundHilite;
	theApp.m_pSettings->Appearance.iColPatternText			= m_iColText;
	theApp.m_pSettings->Appearance.iColPatternTextHilite	= m_iColTextHilite;
	theApp.m_pSettings->Appearance.iColSelection			= m_iColSelection;
	theApp.m_pSettings->Appearance.iColCursor				= m_iColCursor;

	theApp.ReloadColorScheme();

	return CPropertyPage::OnApply();
}

void CConfigAppearance::OnCbnSelchangeFont()
{
	m_pFontList->GetLBText(m_pFontList->GetCurSel(), m_strFont);
	RedrawWindow();
	SetModified();
}

BOOL CConfigAppearance::OnSetActive()
{
	return CPropertyPage::OnSetActive();
}

void CConfigAppearance::OnBnClickedPickCol()
{
	CColorDialog ColorDialog;

	ColorDialog.m_cc.Flags |= CC_FULLOPEN | CC_RGBINIT;

	switch (m_iSelectedItem) {
		case COL_BACKGROUND:
			ColorDialog.m_cc.rgbResult = m_iColBackground;
			break;
		case COL_BACKGROUND_HILITE:
			ColorDialog.m_cc.rgbResult = m_iColBackgroundHilite;
			break;
		case COL_PATTERN_TEXT:
			ColorDialog.m_cc.rgbResult = m_iColText;
			break;
		case COL_PATTERN_TEXT_HILITE:
			ColorDialog.m_cc.rgbResult = m_iColTextHilite;
			break;
		case COL_SELECTION:
			ColorDialog.m_cc.rgbResult = m_iColSelection;
			break;
		case COL_CURSOR:
			ColorDialog.m_cc.rgbResult = m_iColCursor;
			break;
	}

	ColorDialog.DoModal();

	switch (m_iSelectedItem) {
		case COL_BACKGROUND:
			m_iColBackground = ColorDialog.GetColor();
			break;
		case COL_BACKGROUND_HILITE:
			m_iColBackgroundHilite = ColorDialog.GetColor();
			break;
		case COL_PATTERN_TEXT:
			m_iColText = ColorDialog.GetColor();
			break;
		case COL_PATTERN_TEXT_HILITE:
			m_iColTextHilite = ColorDialog.GetColor();
			break;
		case COL_SELECTION:
			m_iColSelection = ColorDialog.GetColor();
			break;
		case COL_CURSOR:
			m_iColCursor = ColorDialog.GetColor();
			break;
	}

	SetModified();
	RedrawWindow();
}

void CConfigAppearance::OnCbnSelchangeColItem()
{
	CComboBox *List = (CComboBox*)GetDlgItem(IDC_COL_ITEM);
	m_iSelectedItem = List->GetCurSel();
	RedrawWindow();
}

void CConfigAppearance::OnCbnSelchangeScheme()
{
	CComboBox *List = (CComboBox*)GetDlgItem(IDC_SCHEME);
	CComboBox *FontList = (CComboBox*)GetDlgItem(IDC_FONT);
	
	int Index = List->GetCurSel();

	switch (Index) {
		case 0:
			m_iColBackground		= COLOR_SCHEME.BACKGROUND;
			m_iColBackgroundHilite	= COLOR_SCHEME.BACKGROUND_HILITE;
			m_iColText				= COLOR_SCHEME.TEXT_NORMAL;
			m_iColTextHilite		= COLOR_SCHEME.TEXT_HILITE;
			m_iColSelection			= COLOR_SCHEME.SELECTION;
			m_iColCursor			= COLOR_SCHEME.CURSOR;
			m_strFont				= "Fixedsys";
			FontList->SelectString(0, m_strFont);
			break;
		case 1:
			m_iColBackground		= 0x00181818;
			m_iColBackgroundHilite	= 0x00202020;
			m_iColText				= 0x00C0C0C0;
			m_iColTextHilite		= 0x00F0F0F0;
			m_iColSelection			= 0x00454550;
			m_iColCursor			= 0x00908080;
			m_strFont				= "Fixedsys";
			FontList->SelectString(0, m_strFont);
			break;
		case 2:
			m_iColBackground		= 0x00131313;
			m_iColBackgroundHilite	= 0x00231A18;
			m_iColText				= 0x00FBF4F0;
			m_iColTextHilite		= 0x00FFD6B9;
			m_iColSelection			= 0x00355D93;
			m_iColCursor			= 0x00464646;
			m_strFont				= "Fixedsys";
			FontList->SelectString(0, m_strFont);
			break;
	}

	SetModified();

	RedrawWindow();
}
