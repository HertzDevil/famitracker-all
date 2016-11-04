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
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "ConfigAppearance.h"
#include "Settings.h"
#include "ColorScheme.h"

const char *COLOR_ITEMS[] = {
	"Background", 
	"Higlighed background",
	"Higlighed background 2",
	"Pattern text", 
	"Highlighted pattern text",
	"Highlighted pattern text 2",
	"Instrument column",
	"Volume column",
	"Effect number column",
	"Selection",
	"Cursor"
};

const char *COLOR_SCHEMES[] = {
	"Default",
	"Monochrome",
	"Renoise",
	"White"
};

const int NUM_COLOR_SCHEMES = 4;

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

	BrushColor.CreateSolidBrush(m_iColors[m_iSelectedItem]);

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

	dc.FillSolidRect(Rect, GetColor(COL_BACKGROUND));

	ShadedCol = DIM(GetColor(COL_PATTERN_TEXT), 50);
	ShadedHiCol = DIM(GetColor(COL_PATTERN_TEXT_HILITE), 50);

	for (int j = 0; j < 3; j++) {

		for (int i = 0; i < 4; i++) {
			int OffsetTop = Rect.top + i * 18 + (j * 54);
			int OffsetLeft = Rect.left + 9;

			if (i == 0) {
				if (j == 0)
					dc.SetTextColor(GetColor(COL_PATTERN_TEXT_HILITE));
				else
					dc.SetTextColor(ShadedHiCol);
				dc.SetBkColor(GetColor(COL_BACKGROUND_HILITE));
				dc.FillSolidRect(Rect.left, OffsetTop, Rect.right - Rect.left, 18, GetColor(COL_BACKGROUND_HILITE));
				if (j == 0) {
					dc.SetBkColor(GetColor(COL_CURSOR));
					dc.FillSolidRect(Rect.left + 5, OffsetTop + 1, 40, 16, GetColor(COL_CURSOR));
				}
			}
			else {
				if (j == 0)
					dc.SetTextColor(GetColor(COL_PATTERN_TEXT));
				else
					dc.SetTextColor(ShadedCol);
				dc.SetBkColor(GetColor(COL_BACKGROUND));
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
				dc.SetBkColor(GetColor(COL_BACKGROUND_HILITE));
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

	m_strFont = theApp.GetSettings()->General.strFont;

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

	m_iColors[COL_BACKGROUND]			= theApp.GetSettings()->Appearance.iColBackground;
	m_iColors[COL_BACKGROUND_HILITE]	= theApp.GetSettings()->Appearance.iColBackgroundHilite;
	m_iColors[COL_BACKGROUND_HILITE2]	= theApp.GetSettings()->Appearance.iColBackgroundHilite2;
	m_iColors[COL_PATTERN_TEXT]			= theApp.GetSettings()->Appearance.iColPatternText;
	m_iColors[COL_PATTERN_TEXT_HILITE]	= theApp.GetSettings()->Appearance.iColPatternTextHilite;
	m_iColors[COL_PATTERN_TEXT_HILITE2]	= theApp.GetSettings()->Appearance.iColPatternTextHilite2;
	m_iColors[COL_PATTERN_INSTRUMENT]	= theApp.GetSettings()->Appearance.iColPatternInstrument;
	m_iColors[COL_PATTERN_VOLUME]		= theApp.GetSettings()->Appearance.iColPatternVolume;
	m_iColors[COL_PATTERN_EFF_NUM]		= theApp.GetSettings()->Appearance.iColPatternEffect;
	m_iColors[COL_SELECTION]			= theApp.GetSettings()->Appearance.iColSelection;
	m_iColors[COL_CURSOR]				= theApp.GetSettings()->Appearance.iColCursor;

		/*
	m_iColBackground		= theApp.m_pSettings->Appearance.iColBackground;
	m_iColBackgroundHilite	= theApp.m_pSettings->Appearance.iColBackgroundHilite;
	m_iColText				= theApp.m_pSettings->Appearance.iColPatternText;
	m_iColTextHilite		= theApp.m_pSettings->Appearance.iColPatternTextHilite;
	m_iColSelection			= theApp.m_pSettings->Appearance.iColSelection;
	m_iColCursor			= theApp.m_pSettings->Appearance.iColCursor;
	*/

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
	theApp.GetSettings()->General.strFont = m_strFont;

	theApp.GetSettings()->Appearance.iColBackground			= m_iColors[COL_BACKGROUND];
	theApp.GetSettings()->Appearance.iColBackgroundHilite	= m_iColors[COL_BACKGROUND_HILITE];
	theApp.GetSettings()->Appearance.iColBackgroundHilite2	= m_iColors[COL_BACKGROUND_HILITE2];
	theApp.GetSettings()->Appearance.iColPatternText		= m_iColors[COL_PATTERN_TEXT];
	theApp.GetSettings()->Appearance.iColPatternTextHilite	= m_iColors[COL_PATTERN_TEXT_HILITE];
	theApp.GetSettings()->Appearance.iColPatternTextHilite2	= m_iColors[COL_PATTERN_TEXT_HILITE2];
	theApp.GetSettings()->Appearance.iColPatternInstrument	= m_iColors[COL_PATTERN_INSTRUMENT];
	theApp.GetSettings()->Appearance.iColPatternVolume		= m_iColors[COL_PATTERN_VOLUME];
	theApp.GetSettings()->Appearance.iColPatternEffect		= m_iColors[COL_PATTERN_EFF_NUM];
	theApp.GetSettings()->Appearance.iColSelection			= m_iColors[COL_SELECTION];
	theApp.GetSettings()->Appearance.iColCursor				= m_iColors[COL_CURSOR];
/*
	theApp.m_pSettings->Appearance.iColBackground			= m_iColBackground;
	theApp.m_pSettings->Appearance.iColBackgroundHilite		= m_iColBackgroundHilite;
	theApp.m_pSettings->Appearance.iColPatternText			= m_iColText;
	theApp.m_pSettings->Appearance.iColPatternTextHilite	= m_iColTextHilite;
	theApp.m_pSettings->Appearance.iColSelection			= m_iColSelection;
	theApp.m_pSettings->Appearance.iColCursor				= m_iColCursor;
*/
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
	ColorDialog.m_cc.rgbResult = m_iColors[m_iSelectedItem];
	ColorDialog.DoModal();

	m_iColors[m_iSelectedItem] = ColorDialog.GetColor();

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

	SetColor(COL_PATTERN_INSTRUMENT, 0x80FF80);
	SetColor(COL_PATTERN_VOLUME, 0xFF8080);
	SetColor(COL_PATTERN_EFF_NUM, 0x8080FF);

	switch (Index) {
		case 0:	// Default
			SetColor(COL_BACKGROUND, COLOR_SCHEME.BACKGROUND);
			SetColor(COL_BACKGROUND_HILITE, COLOR_SCHEME.BACKGROUND_HILITE);
			SetColor(COL_BACKGROUND_HILITE2, COLOR_SCHEME.BACKGROUND_HILITE2);
			SetColor(COL_PATTERN_TEXT, COLOR_SCHEME.TEXT_NORMAL);
			SetColor(COL_PATTERN_TEXT_HILITE, COLOR_SCHEME.TEXT_HILITE);
			SetColor(COL_PATTERN_TEXT_HILITE2, COLOR_SCHEME.TEXT_HILITE2);
			SetColor(COL_SELECTION, COLOR_SCHEME.SELECTION);
			SetColor(COL_CURSOR, COLOR_SCHEME.CURSOR);

			m_strFont = "Fixedsys";
			FontList->SelectString(0, m_strFont);
			break;
		case 1:
			SetColor(COL_BACKGROUND, 0x00181818);
			SetColor(COL_BACKGROUND_HILITE, 0x00202020);
			SetColor(COL_BACKGROUND_HILITE2, 0x00303030);
			SetColor(COL_PATTERN_TEXT, 0x00C0C0C0);
			SetColor(COL_PATTERN_TEXT_HILITE, 0x00F0F0F0);
			SetColor(COL_PATTERN_TEXT_HILITE2, 0x00FFFFFF);
			SetColor(COL_SELECTION, 0x00454550);
			SetColor(COL_CURSOR, 0x00908080);
			m_strFont = "Fixedsys";
			FontList->SelectString(0, m_strFont);
			break;
		case 2:
			SetColor(COL_BACKGROUND, 0x00131313);
			SetColor(COL_BACKGROUND_HILITE, 0x00231A18);
			SetColor(COL_BACKGROUND_HILITE2, 0x00342b29);
			SetColor(COL_PATTERN_TEXT, 0x00FBF4F0);
			SetColor(COL_PATTERN_TEXT_HILITE, 0x00FFD6B9);
			SetColor(COL_PATTERN_TEXT_HILITE2, 0x00FFF6C9);
			SetColor(COL_SELECTION, 0xFF8080);
			SetColor(COL_CURSOR, 0x00707070);
			m_strFont = "Fixedsys";
			FontList->SelectString(0, m_strFont);
			break;
		case 3:	// White
			SetColor(COL_BACKGROUND, 0xFFFFFF);
			SetColor(COL_BACKGROUND_HILITE, 0xF0F0F0);
			SetColor(COL_BACKGROUND_HILITE2, 0xE0E0E0);
			SetColor(COL_PATTERN_TEXT, 0x00);
			SetColor(COL_PATTERN_TEXT_HILITE, 0xFF0000);
			SetColor(COL_PATTERN_TEXT_HILITE2, 0xFF2020);
			SetColor(COL_SELECTION, 0xFF8080);
			SetColor(COL_CURSOR, 0x00D0A0A0);
			SetColor(COL_PATTERN_INSTRUMENT, 0x00);
			SetColor(COL_PATTERN_VOLUME, 0x00);
			SetColor(COL_PATTERN_EFF_NUM, 0x00);
			break;
	}

	SetModified();

	RedrawWindow();
}

void CConfigAppearance::SetColor(int Index, int Color) 
{
	m_iColors[Index] = Color;
}

int CConfigAppearance::GetColor(int Index) const
{
	return m_iColors[Index];
}