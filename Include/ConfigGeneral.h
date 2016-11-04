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

#pragma once


// CConfigGeneral dialog

class CConfigGeneral : public CPropertyPage
{
	DECLARE_DYNAMIC(CConfigGeneral)

public:
	CConfigGeneral();
	virtual ~CConfigGeneral();

// Dialog Data
	enum { IDD = IDD_CONFIG_GENERAL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	bool	m_bWrapCursor;
	bool	m_bFreeCursorEdit;
	bool	m_bPreviewWAV;
	bool	m_bKeyRepeat;
	bool	m_bRowInHex;
	bool	m_bKeySelect;
	int		m_iEditStyle;
	
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnSetActive();
	virtual void OnOK();
	afx_msg void OnBnClickedOptWrapcursor();
	virtual BOOL OnApply();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOptFreecursor();
	afx_msg void OnBnClickedOptWavepreview();
	afx_msg void OnBnClickedOptKeyrepeat();
	afx_msg void OnBnClickedStyle1();
	afx_msg void OnBnClickedStyle2();
	afx_msg void OnBnClickedOptHexadecimal();
	afx_msg void OnBnClickedOptKeyselect();
	afx_msg void OnBnClickedSquarehack();
};
