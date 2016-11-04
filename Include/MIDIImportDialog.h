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
