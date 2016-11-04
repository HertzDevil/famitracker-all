#pragma once


// COctaveDlgBar dialog

class CDialogReBar : public CDialogBar
{
	DECLARE_DYNAMIC(CDialogReBar)

public:
	CDialogReBar(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDialogReBar();

// Dialog Data
	enum { IDD = IDD_OCTAVE };

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMove(int x, int y);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	virtual BOOL Create(CWnd* pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID);
};
