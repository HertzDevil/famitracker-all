/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2010  Jonathan Liss
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
#include "FamiTrackerDoc.h"
#include "SoundGen.h"
#include "SampleEditorDlg.h"

// CSampleEditorDlg dialog

IMPLEMENT_DYNAMIC(CSampleEditorDlg, CDialog)

CSampleEditorDlg::CSampleEditorDlg(CWnd* pParent /*=NULL*/, CDSample *pSample)
	: CDialog(CSampleEditorDlg::IDD, pParent), m_pSampleView(NULL)
{
	// Create a copy of the sample
	m_pSample = new CDSample(*pSample);
	m_pOriginalSample = pSample;
	m_pSoundGen = theApp.GetSoundGenerator();
}

CSampleEditorDlg::~CSampleEditorDlg()
{
	if (m_pSampleView)
		delete m_pSampleView;
	
	if (m_pSample) {
		if (m_pSample->SampleData)
			delete [] m_pSample->SampleData;

		delete m_pSample;
	}
}

void CSampleEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSampleEditorDlg, CDialog)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_OK, &CSampleEditorDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_CANCEL, &CSampleEditorDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_PLAY, &CSampleEditorDlg::OnBnClickedPlay)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_DELETE, &CSampleEditorDlg::OnBnClickedDelete)
	ON_BN_CLICKED(IDC_DELTASTART, &CSampleEditorDlg::OnBnClickedDeltastart)
	ON_WM_KEYDOWN()
	ON_BN_CLICKED(IDC_TILT, &CSampleEditorDlg::OnBnClickedTilt)
END_MESSAGE_MAP()


// CSampleEditorDlg message handlers

BOOL CSampleEditorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_pSampleView = new CSampleView();
	m_pSampleView->SubclassDlgItem(IDC_SAMPLE, this);
	m_pSampleView->CalculateSample(m_pSample, IsDlgButtonChecked(IDC_DELTASTART) ? 64 : 0);
	m_pSampleView->UpdateInfo();

	CSliderCtrl *pitch = (CSliderCtrl*)GetDlgItem(IDC_PITCH);
	pitch->SetRange(0, 15);
	pitch->SetPos(15);

	MoveControls();

	// A timer for the flashing start cursor
	SetTimer(1, 500, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSampleEditorDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	MoveControls();
}

void CSampleEditorDlg::MoveControls()
{
	CRect rect;
	GetClientRect(&rect);

	if (m_pSampleView) {
		rect.top++;
		rect.left++;
		rect.bottom -= 80;
		rect.right -= 2;
		m_pSampleView->Invalidate();
		m_pSampleView->MoveWindow(rect);
	}

	CWnd *control;
	CRect controlRect;

	GetClientRect(&rect);

	if (control = GetDlgItem(IDC_PLAY)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(10, rect.bottom - 30);
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDC_POS)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(5, rect.bottom - 75);
		controlRect.InflateRect(0, 0, 2, 2);
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDC_INFO)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(130, rect.bottom - 75);
		controlRect.InflateRect(0, 0, 2, 2);
		controlRect.right = rect.right - 5;
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDC_STATIC_PITCH)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(93, rect.bottom - 27);
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDC_PITCH)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(120, rect.bottom - 30);
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDC_DELTASTART)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(10, rect.bottom - 50);
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDC_DELETE)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(250, rect.bottom - 30);
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDC_TILT)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(330, rect.bottom - 30);
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDC_CANCEL)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(rect.right - controlRect.right - 10, rect.bottom - 30);
		control->MoveWindow(controlRect);
	}

	if (control = GetDlgItem(IDC_OK)) {
		control->GetClientRect(&controlRect);
		controlRect.MoveToXY(rect.right - controlRect.right - 10, rect.bottom - 55);
		control->MoveWindow(controlRect);
	}

	Invalidate();
	RedrawWindow();
}

void CSampleEditorDlg::OnBnClickedOk()
{
	// Store new sample
	delete [] m_pOriginalSample->SampleData;

	m_pOriginalSample->SampleData = new char[m_pSample->SampleSize];
	m_pOriginalSample->SampleSize = m_pSample->SampleSize;
	memcpy(m_pOriginalSample->SampleData, m_pSample->SampleData, m_pSample->SampleSize);

	//m_pOriginalSample->SampleData = m_pSample->SampleData;
	EndDialog(0);
}

void CSampleEditorDlg::OnBnClickedCancel()
{
	EndDialog(0);
}

void CSampleEditorDlg::OnBnClickedPlay()
{
	if (m_pSample->SampleSize == 0)
		return;

	int Pitch = ((CSliderCtrl*)GetDlgItem(IDC_PITCH))->GetPos();
	m_pSoundGen->WriteAPU(0x4011, IsDlgButtonChecked(IDC_DELTASTART) ? 64 : 0);
	m_pSoundGen->PreviewSample(m_pSample, m_pSampleView->GetStartOffset(), Pitch);
	// Wait for sample to really play, but don't wait for more than 200ms
	DWORD time = GetTickCount() + 200;
	while (m_pSoundGen->PreviewDone() == true || GetTickCount() > time);
	// Start play cursor timer
	SetTimer(0, 10, NULL);
}

void CSampleEditorDlg::OnTimer(UINT_PTR nIDEvent)
{
	// Update play cursor

	if (nIDEvent == 0) {
		// Play cursor
		stDPCMState state = m_pSoundGen->GetDPCMState();

		// Pos is in bytes
		int Pos = state.SamplePos /*<< 6*/;

		if (m_pSoundGen->PreviewDone()) {
			KillTimer(0);
			Pos = -1;
		}

		m_pSampleView->DrawPlayCursor(Pos);
	}
	else if (nIDEvent == 1) {
		// Start cursor
		static bool bDraw = false;
		if (!bDraw)
			m_pSampleView->DrawStartCursor();
		else
			m_pSampleView->DrawPlayCursor(-1);
		bDraw = !bDraw;
	}

	CDialog::OnTimer(nIDEvent);
}

void CSampleEditorDlg::OnBnClickedDelete()
{
	int SelStart = m_pSampleView->GetSelStart();
	int SelEnd = m_pSampleView->GetSelEnd();

	if (SelStart == SelEnd)
		return;

	int StartSample = m_pSampleView->GetBlock(SelStart + 1) * 16;
	int EndSample = m_pSampleView->GetBlock(SelEnd + 1) * 16;
	int Diff = EndSample - StartSample;

	for (int i = 0; i < Diff; ++i) {
		if ((EndSample + i) < (int)m_pSample->SampleSize)
			m_pSample->SampleData[StartSample + i] = m_pSample->SampleData[EndSample + i];
	}

	m_pSample->SampleSize -= Diff;

	// Reallocate
	// todo: fix this, it crashes
	/*
	char *pData = new char[m_pSample->SampleSize];
	memcpy(pData, m_pSample->SampleData, m_pSample->SampleSize);
	delete [] m_pSample->SampleData;
	m_pSample->SampleData = pData;
	*/

	UpdateSampleView();
}

void CSampleEditorDlg::OnBnClickedDeltastart()
{
	UpdateSampleView();
}

void CSampleEditorDlg::UpdateSampleView()
{
	m_pSampleView->CalculateSample(m_pSample, IsDlgButtonChecked(IDC_DELTASTART) ? 64 : 0);
	m_pSampleView->UpdateInfo();
	m_pSampleView->Invalidate();
	m_pSampleView->RedrawWindow();
}

void CSampleEditorDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Todo: this never gets called
	if (nChar == VK_DELETE) {
		OnBnClickedDelete();
	}

	CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}

// CSampleView control

IMPLEMENT_DYNAMIC(CSampleView, CStatic)

BEGIN_MESSAGE_MAP(CSampleView, CStatic)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

CSampleView::CSampleView() : 
	m_iSelStart(-1), 
	m_iSelEnd(-1), 
	m_iStartCursor(0),
	m_pSamples(NULL),
	m_bClicked(false)
{
}

	// Todo: move these
CPen SolidPen(PS_SOLID, 1, (COLORREF)0);
CPen DashedPen(PS_DASH, 1, (COLORREF)0x00);
CPen GrayDashedPen(PS_DASHDOT, 1, (COLORREF)0xF0F0F0);

void CSampleView::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: Add your message handler code here
	// Do not call CStatic::OnPaint() for painting messages

	GetClientRect(&clientRect);

	int MaxY = clientRect.bottom - 2;
	int MaxX = clientRect.right - 2;
	int x, y;

	if (copy.m_hDC != NULL)
		copy.DeleteDC();

	if (copyBmp.m_hObject != NULL)
		copyBmp.DeleteObject();

	copyBmp.CreateCompatibleBitmap(&dc, clientRect.Width(), clientRect.Height());
	copy.CreateCompatibleDC(&dc);
	copy.SelectObject(&copyBmp);

	copy.FillSolidRect(clientRect, 0xFFFFFF);

	copy.SetViewportOrg(1, 1);

	if (m_iSize == 0) {
		copy.TextOutA(10, 10, CString("No sample"));
		dc.BitBlt(0, 0, clientRect.Width(), clientRect.Height(), &copy, 0, 0, SRCCOPY);
		return;
	}

	double Step = double(m_iSize) / double(MaxX);	// Samples / pixel
	double Pos = 0;

	m_dSampleStep = Step;
	m_iBlockSize = MaxX / (m_iSize / (8 * 16));

	CPen *oldPen = copy.SelectObject(&SolidPen);

	// Block markers
	copy.SelectObject(&GrayDashedPen);
	copy.SetBkMode(TRANSPARENT);
	int Blocks = (m_iSize / (8 * 16));
	if (Blocks < (MaxX / 2)) {
		for (int i = 1; i < Blocks; ++i) {
			x = int((i * 128) / m_dSampleStep) - 1;
			copy.MoveTo(x, 0);
			copy.LineTo(x, MaxY);
		}
	}

	// Selection, each step is 16 bytes, or 128 samples
	if (m_iSelStart != m_iSelEnd) {
		copy.FillSolidRect(m_iSelStart, 0, m_iSelEnd - m_iSelStart, MaxY, 0xFF80A0);
//		copy.FillSolidRect(m_iSelStart, 0, 1, MaxY, 0xFF6080);
//		copy.FillSolidRect(m_iSelEnd, 0, 1, MaxY, 0xFF6080);
	}

	// Draw the sample
	y = int((double(m_pSamples[0]) / 127.0) * double(MaxY));
	copy.MoveTo(0, y);
	copy.SelectObject(&SolidPen);

	for (x = 0; x < MaxX; ++x) {
//		y = int((double(m_pSamples[int(Pos)]) / 127.0) * double(MaxY));
		// Interpolate
		y = 0;
		for (int j = 0; j < Step; j++) {
			y += int((double(m_pSamples[int(Pos+j)]) / 127.0) * double(MaxY));
		}
		y=int(double(y)/Step);

		copy.LineTo(x, y);
		Pos += Step;
	}

	copy.SetViewportOrg(0, 0);
	copy.SelectObject(oldPen);

	dc.BitBlt(0, 0, clientRect.Width(), clientRect.Height(), &copy, 0, 0, SRCCOPY);
}

BOOL CSampleView::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

void CSampleView::OnMouseMove(UINT nFlags, CPoint point)
{
	double Sample = double(point.x) * m_dSampleStep;
	int Offset = int(Sample / (8.0 * 64.0));
	int Pos = int(Sample / (8.0 * 16.0));

	if (!m_iSize)
		return;

	if (nFlags & MK_LBUTTON && m_iSelStart != -1) {
		//m_iSelEnd = int((point.x + m_iBlockSize / 2) / m_iBlockSize) * m_iBlockSize;
		int Block = GetBlock(point.x + m_iBlockSize / 2);
		m_iSelEnd = GetPixel(Block);
		Invalidate();
		RedrawWindow();
	}

	static char Text[256];
	sprintf(Text, "Offset: %i, Pos: %i", Offset, Pos);
	GetParent()->SetDlgItemText(IDC_POS, Text);

	CStatic::OnMouseMove(nFlags, point);
}

void CSampleView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (!m_iSize)
		return;

	int Block = GetBlock(point.x);
	m_iSelStart = GetPixel(Block);
	m_iSelEnd = m_iSelStart;
	Invalidate();
	RedrawWindow();
	m_bClicked = true;
	CStatic::OnLButtonDown(nFlags, point);
}

void CSampleView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// Sets the start cursor

	if (!m_iSize)
		return;

	if (!m_bClicked)
		return;

	m_bClicked = false;

	double Sample = double(point.x) * m_dSampleStep;
	int Offset = int(Sample / (8.0 * 64.0));

	if (m_iSelEnd == m_iSelStart) {
		m_iStartCursor = Offset;
		m_iSelStart = m_iSelEnd = -1;
		DrawStartCursor();
	}
	else {
		if (m_iSelEnd < m_iSelStart) {
			int Temp = m_iSelEnd;
			m_iSelEnd = m_iSelStart;
			m_iSelStart = Temp;
		}
	}

	CStatic::OnLButtonUp(nFlags, point);
}

void CSampleView::DrawPlayCursor(int Pos)
{
	CDC *pDC = GetDC();
	int x = int(((double(Pos + m_iStartCursor) * 64.0) * 8.0) / m_dSampleStep);

	pDC->BitBlt(0, 0, clientRect.Width(), clientRect.Height(), &copy, 0, 0, SRCCOPY);

	if (Pos != -1) {
		CPen *oldPen = pDC->SelectObject(&DashedPen);
		pDC->MoveTo(x, 0);
		pDC->LineTo(x, clientRect.bottom);
		pDC->SelectObject(oldPen);
	}

	ReleaseDC(pDC);
}

void CSampleView::DrawStartCursor()
{
	CDC *pDC = GetDC();
	int x = int((double(m_iStartCursor) * 8.0 * 64.0) / m_dSampleStep);

	pDC->BitBlt(0, 0, clientRect.Width(), clientRect.Height(), &copy, 0, 0, SRCCOPY);

	if (m_iStartCursor != -1) {
		CPen *oldPen = pDC->SelectObject(&DashedPen);
		pDC->MoveTo(x, 0);
		pDC->LineTo(x, clientRect.bottom);
		pDC->SelectObject(oldPen);
	}

	ReleaseDC(pDC);
}

void CSampleView::CalculateSample(CDSample *pSample, int Start)
{
	int Samples = pSample->SampleSize;
	int Size = Samples * 8;

	if (m_pSamples)
		delete [] m_pSamples;

	if (pSample->SampleSize == 0) {
		m_iSize = 0;
		m_iStartCursor = 0;
		m_iSelStart = m_iSelEnd = -1;
		m_pSamples = NULL;
		return;
	}

	m_pSamples = new int[Size];
	m_iSize = Size;

	int cntr = Start;

	for (int i = 0; i < Size; ++i) {
		if (pSample->SampleData[i / 8] & (1 << (i % 8))) {
			if (cntr < 126)
				cntr += 2;
		}
		else {
			if (cntr > 1)
				cntr -= 2;
		}
		m_pSamples[i] = cntr;
	}	

	m_iSelStart = m_iSelEnd = -1;
	m_iStartCursor = 0;
}

int CSampleView::GetBlock(int Pixel) const
{
	double Sample = double(Pixel) * m_dSampleStep;
	int Pos = int(Sample / (8.0 * 16.0));
	return Pos;
}

int CSampleView::GetPixel(int Block) const
{
	return int((double(Block) * 128.0) / m_dSampleStep);
}

void CSampleView::UpdateInfo()
{
	static char Text[256];
	if (!m_iSize)
		return;
	sprintf(Text, "Delta end pos: %i, Size: %i bytes", m_pSamples[m_iSize - 1], m_iSize / 8);
	GetParent()->SetDlgItemText(IDC_INFO, Text);
}

void CSampleEditorDlg::OnBnClickedTilt()
{
	int SelStart = m_pSampleView->GetSelStart();
	int SelEnd = m_pSampleView->GetSelEnd();

	if (SelStart == SelEnd)
		return;

	int StartSample = m_pSampleView->GetBlock(SelStart + 1) * 16;
	int EndSample = m_pSampleView->GetBlock(SelEnd + 1) * 16;
	int Diff = EndSample - StartSample;

	int Nr = 10;
	int Step = (Diff * 8) / Nr;
	int Cntr = rand() % Step;

	for (int i = StartSample; i < EndSample; ++i) {
		for (int j = 0; j < 8; j++) {
			if (++Cntr == Step) {
				m_pSample->SampleData[i] &= (0xFF ^ (1 << j));
				Cntr = 0;
			}
		}
	}

	UpdateSampleView();
}
