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

#pragma once

class CPCMImport : public CDialog
{
	DECLARE_DYNAMIC(CPCMImport)

public:
	CPCMImport(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPCMImport();

// Dialog Data
	enum { IDD = IDD_PCMIMPORT };

	CDSample *ShowDialog();

protected:
	CDSample *m_pImported;

	CString		m_strPath, m_strFileName;
	CFile		m_fSampleFile;
	ULONGLONG	m_ullSampleStart;

	int m_iQuality;
	int m_iVolume;
	int m_iSampleSize;
	int m_iChannels;
	int m_iWaveSize;
	int m_iBlockAlign;
	int m_iAvgBytesPerSec;
	int m_iSamplesPerSec;

protected:
	static const int MAX_QUALITY;
	static const int MIN_QUALITY;
	static const int MAX_VOLUME;
	static const int MIN_VOLUME;

//	static const int DPCM_RATES[];
//	static const int BASE_FREQ;
	static const int SAMPLES_MAX;

protected:
	CDSample *ConvertFile();
	int ReadSample(void);
	bool OpenWaveFile();
	void UpdateFileInfo();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedPreview();
};
