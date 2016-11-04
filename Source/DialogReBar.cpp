// OctaveDlgBar.cpp : implementation file
//

#include "stdafx.h"
#include "FamiTracker.h"
#include "DialogReBar.h"
#include "MainFrm.h"

// COctaveDlgBar dialog

IMPLEMENT_DYNAMIC(CDialogReBar, CDialogBar)
CDialogReBar::CDialogReBar(CWnd* pParent /*=NULL*/)
	: CDialogBar(/*COctaveDlgBar::IDD, pParent*/)
{
}

CDialogReBar::~CDialogReBar()
{
}

BEGIN_MESSAGE_MAP(CDialogReBar, CDialogBar)
	ON_WM_ERASEBKGND()
	ON_WM_MOVE()
	ON_WM_CTLCOLOR()
	ON_NOTIFY(UDN_DELTAPOS, IDC_HIGHLIGHTSPIN, OnDeltaposHighlight)
END_MESSAGE_MAP()


// COctaveDlgBar message handlers

BOOL CDialogReBar::OnEraseBkgnd(CDC* pDC)
{
	if (!theApp.IsThemeActive()) {
		CDialogBar::OnEraseBkgnd(pDC);
		return TRUE;
	}

	CWnd* pParent = GetParent();
    ASSERT_VALID(pParent);
    CPoint pt(0, 0);
    MapWindowPoints(pParent, &pt, 1);
    pt = pDC->OffsetWindowOrg(pt.x, pt.y);
    LRESULT lResult = pParent->SendMessage(WM_ERASEBKGND, (WPARAM)pDC->m_hDC, 0L);
    pDC->SetWindowOrg(pt.x, pt.y);
    return (BOOL)lResult;
}

void CDialogReBar::OnMove(int x, int y)
{
	Invalidate();
}

HBRUSH CDialogReBar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogBar::OnCtlColor(pDC, pWnd, nCtlColor);

	if (nCtlColor == CTLCOLOR_STATIC && theApp.IsThemeActive()) {
		pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)GetStockObject(NULL_BRUSH);
	}

	return hbr;
}

void CDialogReBar::OnDeltaposHighlight(NMHDR *pNMHDR, LRESULT *pResult)
{
	int Value = GetDlgItemInt(IDC_HIGHLIGHT, 0, 0) - ((NMUPDOWN*)pNMHDR)->iDelta;
	
	if (Value < 1)
		Value = 1;
	if (Value > 32)	// hard coded
		Value = 32;

	SetDlgItemInt(IDC_HIGHLIGHT, Value, 0);
	
	CDocument *pDoc = theApp.GetFirstDocument();
	POSITION Pos = pDoc->GetFirstViewPosition();
	pDoc->GetNextView(Pos)->RedrawWindow();
}
