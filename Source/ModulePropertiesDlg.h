#pragma once


// CModulePropertiesDlg dialog

class CModulePropertiesDlg : public CDialog
{
	DECLARE_DYNAMIC(CModulePropertiesDlg)

private:
	void SelectSong(int Song);

	unsigned int m_iSelectedSong;
	CListCtrl *m_pSongList;

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
	afx_msg void OnBnClickedSongAdd();
	afx_msg void OnBnClickedSongRemove();
	afx_msg void OnBnClickedSongUp();
	afx_msg void OnBnClickedSongDown();
	afx_msg void OnEnChangeSongname();
	afx_msg void OnClickSongList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedSongImport();
};
