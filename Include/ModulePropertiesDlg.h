#pragma once


// CModulePropertiesDlg dialog

class CModulePropertiesDlg : public CDialog
{
	DECLARE_DYNAMIC(CModulePropertiesDlg)

private:
	unsigned int m_iInitialSongCount;

public:
	CModulePropertiesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CModulePropertiesDlg();

// Dialog Data
	enum { IDD = IDD_PROPERTIES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
