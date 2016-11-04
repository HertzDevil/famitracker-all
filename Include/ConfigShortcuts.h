#pragma once


// CConfigShortcuts dialog

class CConfigShortcuts : public CPropertyPage
{
	DECLARE_DYNAMIC(CConfigShortcuts)

public:
	CConfigShortcuts();   // standard constructor
	virtual ~CConfigShortcuts();

// Dialog Data
	enum { IDD = IDD_CONFIG_SHORTCUTS };

private:
	int		m_iSelectedItem;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnLvnItemchangedShortcuts(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickShortcuts(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCbnSelchangeModifiers();
	afx_msg void OnCbnSelchangeKeys();
	afx_msg void OnBnClickedDefault();
	virtual BOOL OnApply();
};
