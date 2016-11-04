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

#pragma once

enum E_COLOR_ITEMS {
	COL_BACKGROUND,
	COL_BACKGROUND_HILITE,
	COL_PATTERN_TEXT,
	COL_PATTERN_TEXT_HILITE,
	COL_PATTERN_INSTRUMENT,
	COL_PATTERN_VOLUME,
	COL_PATTERN_EFF_NUM,
	COL_SELECTION,
	COL_CURSOR,
	COLOR_ITEM_COUNT
};

// CConfigAppearance dialog

class CConfigAppearance : public CPropertyPage
{
	DECLARE_DYNAMIC(CConfigAppearance)

public:
	CConfigAppearance();
	virtual ~CConfigAppearance();

	void AddFontName(char *Name);

// Dialog Data
	enum { IDD = IDD_CONFIG_APPEARANCE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void SetColor(int Index, int Color);
	int GetColor(int Index) const;

	CComboBox*	m_pFontList;
	CString		m_strFont;
	
	int m_iSelectedItem;
/*
	int	m_iColBackground;
	int m_iColBackgroundHilite;
	int m_iColText;
	int m_iColTextHilite;
	int m_iColSelection;
	int m_iColCursor;
*/
	int m_iColors[COLOR_ITEM_COUNT];

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	afx_msg void OnCbnSelchangeFont();
	virtual BOOL OnSetActive();
	afx_msg void OnBnClickedPickCol();
	afx_msg void OnCbnSelchangeColItem();
	afx_msg void OnCbnSelchangeScheme();
};
