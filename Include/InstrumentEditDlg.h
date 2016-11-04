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

#include "InstrumentSettings.h"
#include "InstrumentDPCM.h"

// CInstrumentEditDlg dialog

class CInstrumentEditDlg : public CDialog
{
	DECLARE_DYNAMIC(CInstrumentEditDlg)

public:
	CInstrumentEditDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInstrumentEditDlg();

	void ChangeNoteState(int Note);
	void SetCurrentInstrument(int Index);

	bool m_bOpened;

// Dialog Data
	enum { IDD = IDD_INSTRUMENT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void SwitchOnNote(int x, int y);
	void SwitchOffNote();

	int	m_iLastNote;
	int m_iLightNote;

	CInstrumentSettings InstrumentSettings;
	CInstrumentDPCM		InstrumentDPCM;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedClose();
	virtual BOOL OnInitDialog();
	afx_msg void OnTcnSelchangeInstTab(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnPaint();
	void ChangeNoteOn(int Note, int Octave);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
//	virtual BOOL DestroyWindow();
	virtual BOOL DestroyWindow();
protected:
	virtual void OnOK();
	virtual void OnCancel();
};
