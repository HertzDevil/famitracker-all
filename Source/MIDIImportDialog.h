/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2012  Jonathan Liss
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


// CMIDIImportDialog dialog

class CMIDIImportDialog : public CDialog
{
	DECLARE_DYNAMIC(CMIDIImportDialog)

public:
	CMIDIImportDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMIDIImportDialog();

// Dialog Data
	enum { IDD = IDD_MIDIIMPORT };

	int m_iChannelMap[MAX_CHANNELS];
	int m_iPatLen;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	int m_iChannels;

	DECLARE_MESSAGE_MAP()
public:
	virtual INT_PTR DoModal(int Channels);
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
};
