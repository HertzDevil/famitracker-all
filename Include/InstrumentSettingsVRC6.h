#pragma once

#include "SequenceEditorVRC6.h"
#include "InstrumentSettings.h"

//static const char			*INST_SETTINGS[]	= {"Volume", "Arpeggio", "Pitch",  "Hi-pitch", "Duty / Noise"};
//static const unsigned int	MAX_MML_ITEMS		= 254;

// CInstrumentSettingsVRC6 dialog

class CInstrumentSettingsVRC6 : public CDialog, public CMML
{
	DECLARE_DYNAMIC(CInstrumentSettingsVRC6)

public:
	CInstrumentSettingsVRC6(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInstrumentSettingsVRC6();

	void SetCurrentInstrument(int Index);
	void SelectSequence(int Sequence);
	void CompileSequence();

// Dialog Data
	enum { IDD = IDD_INSTRUMENT_VRC6 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CSequenceEditorVRC6	SequenceEditor;
	CSequence 			*m_SelectedSeq;

	CListCtrl			*m_pSettingsListCtrl;
	CListCtrl			*m_pSequenceListCtrl;
//	CWnd				*ParentWin;

	CFamiTrackerDoc		*pDoc;
	CFamiTrackerView	*pView;

//	unsigned int		SelectedSeqItem;
	unsigned int		m_iSelectedSequence;
	unsigned int		m_iInstrument;
	unsigned int		m_iCurrentEffect;

//	signed short		MML_Values[MAX_MML_ITEMS];		// Maximum allowed values?
//	unsigned int		MML_Count;

	bool				UpdatingSequenceItem;
	bool				m_bInitializing;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
public:
	afx_msg void OnLvnItemchangedInstsettings(NMHDR *pNMHDR, LRESULT *pResult);
public:
	virtual BOOL DestroyWindow();
};
