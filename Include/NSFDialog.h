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


// CNSFDialog dialog

class CNSFDialog : public CDialog
{
	DECLARE_DYNAMIC(CNSFDialog)

public:
	CNSFDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CNSFDialog();

	void	SaveInfo();
	int		ReadStartAddress();

	char m_strName[32];
	char m_strArtist[32];
	char m_strCopyright[32];

// Dialog Data
	enum { IDD = IDD_EXPORTNSF };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CDocument *GetDocument();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedWriteNSF();
	afx_msg void OnBnClickedCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedWriteBIN();
	afx_msg void OnBnClickedWritePrg();
	afx_msg void OnBnClickedBankswitch();
	afx_msg void OnBnClickedBankoptimize();
	afx_msg void OnBnClickedNewData();
};
