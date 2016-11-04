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

#include "SequenceEditor.h"

static const char			*INST_SETTINGS[]	= {"Volume", "Arpeggio", "Pitch",  "Hi-pitch", "Duty / Noise"};
static const unsigned int	MAX_MML_ITEMS		= 512;

// CInstrumentSettings dialog

class CInstrumentSettings : public CDialog
{
	DECLARE_DYNAMIC(CInstrumentSettings)

public:
	CInstrumentSettings(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInstrumentSettings();

	// Public
	void SetCurrentInstrument(int Index);

	// Public, but used internal
	void CompileSequence();
	void TranslateMML();
	void TranslateRelativeMML();

// Dialog Data
	enum { IDD = IDD_INSTRUMENT_INTERNAL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void SelectSequence(int Sequence);

	CSequenceEditor		SequenceEditor;

	stSequence			*m_SelectedSeq;

	CListCtrl			*m_pSettingsListCtrl;
	CListCtrl			*m_pSequenceListCtrl;
	CWnd				*ParentWin;

	CFamiTrackerDoc		*pDoc;
	CFamiTrackerView	*pView;

	unsigned int		SelectedSeqItem;
	unsigned int		m_iSelectedSequence;
	unsigned int		m_iInstrument;
	unsigned int		m_iCurrentEffect;

	signed short		MML_Values[MAX_MML_ITEMS];		// Maximum allowed values?
	unsigned int		MML_Count;

	bool				UpdatingSequenceItem;
	bool				m_bInitializing;
	bool				m_bShiftPressed;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnLvnItemchangedInstsettings(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposModSelectSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeSeqIndex();
	afx_msg void OnPaint();
	afx_msg void OnBnClickedParseMml();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
public:
	afx_msg void OnBnClickedFreeSeq();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};
