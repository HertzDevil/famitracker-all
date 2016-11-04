// Source\ModulePropertiesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FamiTracker.h"
#include "Include\ModulePropertiesDlg.h"


// CModulePropertiesDlg dialog

IMPLEMENT_DYNAMIC(CModulePropertiesDlg, CDialog)
CModulePropertiesDlg::CModulePropertiesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CModulePropertiesDlg::IDD, pParent)
{
}

CModulePropertiesDlg::~CModulePropertiesDlg()
{
}

void CModulePropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CModulePropertiesDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


// CModulePropertiesDlg message handlers

BOOL CModulePropertiesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.pDocument;
	CSliderCtrl *SubTuneSlider = (CSliderCtrl*)GetDlgItem(IDC_SUBTUNE);
	CString Text;

	m_iInitialSongCount = pDoc->GetTrackCount();

	SubTuneSlider->SetRange(0, MAX_TRACKS - 1);
	SubTuneSlider->SetPos(m_iInitialSongCount);
	
	Text.Format("%i", pDoc->GetTrackCount() + 1);
	SetDlgItemText(IDC_SUBTUNES, Text);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CModulePropertiesDlg::OnBnClickedOk()
{
	CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)theApp.pDocument;
	CSliderCtrl *SubTuneSlider = (CSliderCtrl*)GetDlgItem(IDC_SUBTUNE);

	unsigned int NewCount = SubTuneSlider->GetPos();

	pDoc->SetTracks(NewCount);
	
	OnOK();
}

void CModulePropertiesDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CSliderCtrl *SubTuneSlider = (CSliderCtrl*)GetDlgItem(IDC_SUBTUNE);
	CString Text;
	
	Text.Format("%i", SubTuneSlider->GetPos() + 1);
	GetDlgItem(IDC_SUBTUNES)->SetWindowText(Text);

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}
