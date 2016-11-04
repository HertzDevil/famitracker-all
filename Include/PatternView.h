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

// The idea is to move big parts of CFamiTrackerView to this class

#pragma once

#include "FamiTrackerDoc.h"

class CPatternView {
public:
	bool InitView();
	void SetDocument(CFamiTrackerDoc *pDoc);
	void SetWindowSize(int width, int height);

	void DrawScreen(CDC *pDC, CFamiTrackerView *pView);
	void DrawHeader(CDC *pDC);

	void CreateBackground(CDC *pDC);

	/////
	void CacheBackground(CDC *pDC);
	void RestoreBackground(CDC *pDC);

private:
	void PrepareBackground(CDC *pDC);
	void DrawRows(CDC *pDC);
	void DrawRow(unsigned int Row, unsigned int Line, CDC *pDC);

	int DrawPattern(int PosX, int PosY, int Column, int Channel, int Color, bool bHighlight, bool bShaded, stChanNote *NoteData, CDC *dc);

	void DrawChar(int x, int y, char c, int Color, CDC *pDC);

	void DrawNoteColumn(unsigned int PosX, unsigned int PosY, CDC *pDC);
	void DrawInstrumentColumn(unsigned int PosX, unsigned int PosY, CDC *pDC);

	// Head
	void DrawChannelNames(CDC *pDC);

private:
	CFamiTrackerDoc		*m_pDocument;
	unsigned int		m_iWindowWidth, m_iWindowHeight;

	unsigned int		m_iVisibleRows;
	unsigned int		m_iCursorRow;

	CDC					*m_pBackDC;
	CBitmap				m_bmpCache, *m_pOldCacheBmp;
};
