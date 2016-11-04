/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2007  Jonathan Liss
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

// This will be the new pattern editor

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerView.h"
#include "PatternView.h"

const unsigned int ROW_HEIGHT = 18;	
const unsigned int CHAR_WIDTH = 12;
const unsigned int HEADER_HEIGHT = 50;		// Top header (channel names etc)

const unsigned int PATTERN_START = 58;

const unsigned int CHANNEL_SPACING = 10;
const unsigned int COLUMN_SPACING = 4;

const unsigned int PATTERN_LEFT = 50;		// start of patterns to the left

CFont Font;

bool CPatternView::InitView()
{
	LOGFONT LogFont;
	LPCSTR	FontName = theApp.m_pSettings->General.strFont;

	// Create header font

	// Create pattern font
	memset(&LogFont, 0, sizeof LOGFONT);
	memcpy(LogFont.lfFaceName, FontName, strlen(FontName));

	LogFont.lfHeight = -12;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;

	Font.CreateFontIndirect(&LogFont);

	return true;
}

void CPatternView::SetDocument(CFamiTrackerDoc *pDoc)
{
	m_pDocument = pDoc;

	// Reset variables

}

void CPatternView::DrawScreen(CDC *pDC, CFamiTrackerView *pView)
{
	pView->SetScrollPos(SB_VERT, 0);
	pView->SetScrollRange(SB_VERT, 0, 100);

	pView->SetScrollPos(SB_HORZ, 0);
	pView->SetScrollRange(SB_HORZ, 0, 100);
}

void CPatternView::SetWindowSize(int width, int height)
{
	m_iWindowWidth = width;
	m_iWindowHeight = height;

	m_iVisibleRows = (m_iWindowHeight - HEADER_HEIGHT) / ROW_HEIGHT;
}

void CPatternView::DrawHeader(CDC *pDC)
{
	DrawChannelNames(pDC);
}

void CPatternView::DrawChannelNames(CDC *pDC)
{
	const char *CHANNEL_NAMES[]		 = {"Square 1", "Square 2", "Triangle", "Noise", "DPCM"};
	const char *CHANNEL_NAMES_VRC6[] = {"Square 1", "Square 2", "Sawtooth"};

	unsigned int Offset = PATTERN_LEFT + CHANNEL_SPACING;
	unsigned int AvailableChannels = m_pDocument->GetAvailableChannels();
	unsigned int i;

	CFont *OldFont;

	pDC->SetBkMode(TRANSPARENT);

	OldFont = pDC->SelectObject(&Font);

	for (i = 0; i < AvailableChannels; i++) {
		
		if (i > 4) {	// expansion names
		}
		else { // internal names
//			pDC->SetTextColor(CurTopColor);
//			pDC->TextOut(Offset, TEXT_POSITION, CHANNEL_NAMES[i]);
		}
	}

	pDC->SelectObject(OldFont);
}

// Called when the background is erased
void CPatternView::CreateBackground(CDC *pDC)
{
	CDC TempDC;
	CBitmap Bmp;

	Bmp.CreateCompatibleBitmap(pDC, m_iWindowWidth, m_iWindowHeight);
	TempDC.CreateCompatibleDC(pDC);
	TempDC.SelectObject(&Bmp)->DeleteObject();

	PrepareBackground(&TempDC);
	CacheBackground(&TempDC);

	TempDC.DeleteDC();
}

// Private //////////////////

void CPatternView::PrepareBackground(CDC *pDC)
{	
	const unsigned int DARK_GRAY	= 0x808080;
	const unsigned int LIGHT_GRAY	= 0xC0C0C0;

	unsigned int ColBackground		= theApp.m_pSettings->Appearance.iColBackground;
	unsigned int ColHiBackground	= theApp.m_pSettings->Appearance.iColBackgroundHilite;
	unsigned int ColText			= theApp.m_pSettings->Appearance.iColPatternText;
	unsigned int ColHiText			= theApp.m_pSettings->Appearance.iColPatternTextHilite;
	unsigned int ColSelection		= theApp.m_pSettings->Appearance.iColSelection;
	unsigned int ColCursor			= theApp.m_pSettings->Appearance.iColCursor;

	// Clear background
	pDC->FillSolidRect(0, 0, m_iWindowWidth - 4, m_iWindowHeight - 4, ColBackground);

	//pDC->FillSolidRect(0, 0, m_iWindowWidth, HEADER_HEIGHT, LIGHT_GRAY);

	pDC->FillSolidRect(0, HEADER_HEIGHT, m_iWindowWidth, 1, DARK_GRAY);
	pDC->FillSolidRect(0, HEADER_HEIGHT - 1, m_iWindowWidth, 1, LIGHT_GRAY);
}

// Draw all rows
void CPatternView::DrawRows(CDC *pDC)
{
	unsigned int i, Row;
	CFont *OldFont;

	OldFont = pDC->SelectObject(&Font);

	pDC->SetTextColor(0xFFFFFF);
	pDC->SetBkMode(TRANSPARENT);

	Row = 0;

	for (i = 0; i < m_iVisibleRows; i++) {
		DrawRow(Row, i, pDC);
		Row++;
	}

	pDC->SelectObject(OldFont);
}

// Draw a single row
void CPatternView::DrawRow(unsigned int Row, unsigned int Line, CDC *pDC)
{
	// Row is row from pattern to display, line is screen line

	CString Text;
	int i, j;

	Text.Format(_T("%02X"), Row);
	pDC->TextOut(10, (Line * ROW_HEIGHT) + PATTERN_START, Text);

	int m_iStartChan = 0;
	int m_iChannelsVisible = 2;

	stChanNote NoteData;

	int PosX = 50;
	int PosY = PATTERN_START + Row * ROW_HEIGHT;

	for (i = m_iStartChan; i < m_iChannelsVisible; i++) {
		m_pDocument->GetNoteData(0, i, Row, &NoteData);
		for (j = 0; j < 7; j++) {
			PosX += DrawPattern(PosX, PosY, j, i, 0xFFFFFF, false, false, &NoteData, pDC);
		}
		PosX += CHANNEL_SPACING;
	}
}

int CPatternView::DrawPattern(int PosX, int PosY, int Column, int Channel, int Color, bool bHighlight, bool bShaded, stChanNote *NoteData, CDC *dc)
{
	const char NOTES_A[] = {'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'};
	const char NOTES_B[] = {'-', '#', '-', '#', '-', '-', '#', '-', '#', '-', '#', '-'};
	const char NOTES_C[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	const char HEX[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

	int Vol			= NoteData->Vol;
	int Note		= NoteData->Note;
	int Octave		= NoteData->Octave;
	int Instrument	= NoteData->Instrument;

	if (Note < 0 || Note > HALT)
		return 0;

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

	unsigned int InstCol = theApp.m_pSettings->Appearance.iColPatternInstrument;
	unsigned int VolumeCol = theApp.m_pSettings->Appearance.iColPatternVolume;
	unsigned int EffCol = theApp.m_pSettings->Appearance.iColPatternEffect;

	if (!theApp.m_pSettings->General.bPatternColor) {
		InstCol = VolumeCol = EffCol = Color;
	}

	unsigned int ShadedCol = DIM(Color, 50);

	if (bHighlight) {
		/*
		VolumeCol	= 0xFFa020;
		EffCol		= 0x2080FF;
		InstCol		= 0x20FF80;
		*/
	}

	if (bShaded) {
		VolumeCol	= DIM(VolumeCol, 50);
		EffCol		= DIM(EffCol, 50);
		InstCol		= DIM(InstCol, 50);
	}

	switch (Column) {
		case 0:
			// Note and octave
			switch (Note) {
				case 0:
				case HALT:
					if (Note == 0) {
						DrawChar(PosX, PosY, '-', ShadedCol, dc);
						DrawChar(PosX + CHAR_WIDTH, PosY, '-', ShadedCol, dc);
						DrawChar(PosX + CHAR_WIDTH * 2, PosY, '-', ShadedCol, dc);
					}
					else {
						DrawChar(PosX, PosY, '*', Color, dc);
						DrawChar(PosX + CHAR_WIDTH, PosY, '*', Color, dc);
						DrawChar(PosX + CHAR_WIDTH * 2, PosY, '*', Color, dc);
					}
					break;
				default:
					if (Channel == 3) {
						char NoiseFreq = (Note - 1 + Octave * 12) & 0x0F;
						DrawChar(PosX, PosY, HEX[NoiseFreq], Color, dc);
						DrawChar(PosX + CHAR_WIDTH, PosY, '-', Color, dc);
						DrawChar(PosX + CHAR_WIDTH * 2, PosY, '#', Color, dc);
					}
					else {
						DrawChar(PosX, PosY, NOTES_A[Note - 1], Color, dc);
						DrawChar(PosX + CHAR_WIDTH, PosY, NOTES_B[Note - 1], Color, dc);
						DrawChar(PosX + CHAR_WIDTH * 2, PosY, NOTES_C[Octave], Color, dc);
					}
			}
			return CHAR_WIDTH * 3 + COLUMN_SPACING;
		case 1:
			// Instrument x0
			if (Instrument == MAX_INSTRUMENTS || Note == HALT)
				DrawChar(PosX, PosY, '-', ShadedCol, dc);
			else
				DrawChar(PosX, PosY, HEX[Instrument >> 4], InstCol, dc);
			return CHAR_WIDTH;
		case 2:
			// Instrument 0x
			if (Instrument == MAX_INSTRUMENTS || Note == HALT)
				DrawChar(PosX, PosY, '-', ShadedCol, dc);
			else
				DrawChar(PosX, PosY, HEX[Instrument & 0x0F], InstCol, dc);
			return CHAR_WIDTH + COLUMN_SPACING;
		case 3:
			// Volume (skip triangle)
			if (Vol == 0x10 || Channel == 2 || Channel == 4)
				DrawChar(PosX, PosY, '-', ShadedCol, dc);
			else
				DrawChar(PosX, PosY, HEX[Vol & 0x0F], VolumeCol, dc);
			return CHAR_WIDTH + COLUMN_SPACING;
		case 4: case 7: case 10: case 13:
			// Effect type
			if (EffNumber == 0)
				DrawChar(PosX, PosY, '-', ShadedCol, dc);
			else
				DrawChar(PosX, PosY, EFF_CHAR[EffNumber - 1], EffCol, dc);
			return CHAR_WIDTH;
		case 5: case 8: case 11: case 14:
			// Effect param x
			if (EffNumber == 0)
				DrawChar(PosX, PosY, '-', ShadedCol, dc);
			else
				DrawChar(PosX, PosY, HEX[(EffParam >> 4) & 0x0F], Color, dc);
			return CHAR_WIDTH;
		case 6: case 9: case 12: case 15:
			// Effect param y
			if (EffNumber == 0)
				DrawChar(PosX, PosY, '-', ShadedCol, dc);
			else
				DrawChar(PosX, PosY, HEX[EffParam & 0x0F], Color, dc);
			return CHAR_WIDTH + COLUMN_SPACING;
	}

	return 0;
}

// Draws a colored character
void CPatternView::DrawChar(int x, int y, char c, int Color, CDC *pDC)
{
	static CString Text;
	Text.Format(_T("%c"), c);
	pDC->SetTextColor(Color);
	pDC->TextOut(x, y, Text);
}

void CPatternView::CacheBackground(CDC *pDC)
{
	// Saves a DC
	if (m_pBackDC)
		m_pBackDC->DeleteDC();

	m_pBackDC = new CDC;

	m_bmpCache.DeleteObject();

	m_bmpCache.CreateCompatibleBitmap(pDC, m_iWindowWidth, m_iWindowHeight);
	m_pBackDC->CreateCompatibleDC(pDC);
	m_pOldCacheBmp = m_pBackDC->SelectObject(&m_bmpCache);

	m_pOldCacheBmp->DeleteObject();

//	m_pBackDC->CreateCompatibleDC(pDC);
	m_pBackDC->BitBlt(0, 0, m_iWindowWidth, m_iWindowHeight, pDC, 0, 0, SRCCOPY);
}

void CPatternView::RestoreBackground(CDC *pDC)
{
	// Copies the background to screen
	pDC->BitBlt(0, 0, m_iWindowWidth, m_iWindowHeight, m_pBackDC, 0, 0, SRCCOPY);
}
