#pragma once


// CConfigMIDI dialog

class CConfigMIDI : public CPropertyPage
{
	DECLARE_DYNAMIC(CConfigMIDI)

public:
	CConfigMIDI();
	virtual ~CConfigMIDI();

// Dialog Data
	enum { IDD = IDD_CONFIG_MIDI };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
};
