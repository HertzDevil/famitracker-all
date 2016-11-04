#pragma once


// CDPCMDialog dialog

class CDPCMDialog : public CDialog
{
	DECLARE_DYNAMIC(CDPCMDialog)

public:
	CDPCMDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDPCMDialog();

// Dialog Data
	enum { IDD = IDD_DPCM };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	void SetDocument(CFamiTrackerDoc *pDoc);
	void FillList();
	afx_msg void OnBnClickedClose();
	afx_msg void OnBnClickedOk2();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedAdd();
	afx_msg void OnBnClickedRemove();
protected:
	virtual void OnCancel();
public:
	afx_msg void OnBnClickedImport();
	afx_msg void OnBnClickedSave();
};
