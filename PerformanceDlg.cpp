/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005  Jonathan Liss
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

#include "stdafx.h"
#include "FamiTracker.h"
#include "PerformanceDlg.h"
#include ".\performancedlg.h"


// CPerformanceDlg dialog

IMPLEMENT_DYNAMIC(CPerformanceDlg, CDialog)
CPerformanceDlg::CPerformanceDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPerformanceDlg::IDD, pParent)
{
}

CPerformanceDlg::~CPerformanceDlg()
{
}

void CPerformanceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPerformanceDlg, CDialog)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CPerformanceDlg message handlers

BOOL CPerformanceDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here

	SetTimer(1, 1000, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPerformanceDlg::OnTimer(UINT nIDEvent)
{
	// TODO: Add your message handler code here and/or call default

	CString Text;
	CProgressCtrl *pBar;

	unsigned int Usage = theApp.GetCPUUsage();
	unsigned int Rate = theApp.GetFrameRate();
	unsigned int Underruns = theApp.GetUnderruns();

	Text.Format("%i%%", Usage / 100);
	SetDlgItemText(IDC_CPU, Text);

	Text.Format("Framerate: %i Hz", Rate);
	SetDlgItemText(IDC_FRAMERATE, Text);

	Text.Format("Underruns: %i", Underruns);
	SetDlgItemText(IDC_UNDERRUN, Text);

	pBar = (CProgressCtrl*)GetDlgItem(IDC_CPU_BAR);

	pBar->SetRange(0, 100);
	pBar->SetPos(Usage / 100);

	CDialog::OnTimer(nIDEvent);
}

void CPerformanceDlg::OnBnClickedOk()
{
	DestroyWindow();
}

BOOL CPerformanceDlg::DestroyWindow()
{
	// TODO: Add your specialized code here and/or call the base class

	KillTimer(1);
	return CDialog::DestroyWindow();
}
