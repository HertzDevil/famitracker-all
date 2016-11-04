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
#include <mmsystem.h>
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "PCMImport.h"
#include "Settings.h"
#include "SoundGen.h"

const int MAX_QUALITY	= 15;
const int MIN_QUALITY	= 0;
const int MAX_VOLUME	= 16;
const int MIN_VOLUME	= 1;

const int DPCM_RATES[]	= {0x01AC, 0x017C, 0x0154, 0x0140, 0x011E, 0x00FE, 0x00E2, 0x00D6, 0x00BE, 0x00A0, 0x008E, 0x0080, 0x006A, 0x0054, 0x0048, 0x0036};
const int BASE_FREQ		= 1789773;
const int SAMPLES_MAX	= 0x0FF1;		// Max amount of 8-bit DPCM samples

// Derive a new class from CFileDialog with implemented preview of audio files

class CFileSoundDialog : public CFileDialog
{
public:
	CFileSoundDialog(BOOL bOpenFileDialog, LPCTSTR lpszDefExt = NULL, LPCTSTR lpszFileName = NULL, DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, LPCTSTR lpszFilter = NULL, CWnd* pParentWnd = NULL, DWORD dwSize = 0);
	~CFileSoundDialog();

protected:
	virtual void OnFileNameChange();
};

//	CFileSoundDialog

CFileSoundDialog::CFileSoundDialog(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName, DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd, DWORD dwSize) 
	: CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, dwSize)
{
}

CFileSoundDialog::~CFileSoundDialog()
{
	// Stop any possible playing sound
	PlaySound(NULL, NULL, SND_NODEFAULT | SND_SYNC);
}

void CFileSoundDialog::OnFileNameChange()
{
	// Preview wave file

	if (!GetFileExt().CompareNoCase("wav") && theApp.m_pSettings->General.bWavePreview)
		PlaySound(GetPathName(), NULL, SND_FILENAME | SND_NODEFAULT | SND_ASYNC | SND_NOWAIT);
	
	CFileDialog::OnFileNameChange();
}

// CPCMImport dialog

IMPLEMENT_DYNAMIC(CPCMImport, CDialog)
CPCMImport::CPCMImport(CWnd* pParent /*=NULL*/)
	: CDialog(CPCMImport::IDD, pParent)
{
}

CPCMImport::~CPCMImport()
{
}

void CPCMImport::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPCMImport, CDialog)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_PREVIEW, &CPCMImport::OnBnClickedPreview)
END_MESSAGE_MAP()

CDSample *CPCMImport::ShowDialog()
{
	// Return import parameters, 0 if cancel

	CFileSoundDialog OpenFileDialog(TRUE, 0, 0, OFN_HIDEREADONLY, "Microsoft PCM files (*.wav)|*.wav|All files (*.*)|*.*||");
/*
	strcpy(pImported->Name, "");
	Imported.SampleData = NULL;
	Imported.SampleSize = 0;
*/
	OpenFileDialog.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_WAV);

	if (OpenFileDialog.DoModal() == IDCANCEL)
		return NULL;

	theApp.m_pSettings->SetPath(OpenFileDialog.GetPathName(), PATH_WAV);

	m_strPath		= OpenFileDialog.GetPathName();
	m_strFileName	= OpenFileDialog.GetFileName();

	m_pImported = NULL;

	CDialog::DoModal();

	return m_pImported;
}

// CPCMImport message handlers

BOOL CPCMImport::OnInitDialog()
{
	CDialog::OnInitDialog();

	CSliderCtrl *QualitySlider = (CSliderCtrl*)GetDlgItem(IDC_QUALITY);
	CSliderCtrl *VolumeSlider = (CSliderCtrl*)GetDlgItem(IDC_VOLUME);
	CString Text;

	m_iQuality = MAX_QUALITY;
	m_iVolume = MAX_VOLUME / 4;

	QualitySlider->SetRange(MIN_QUALITY, MAX_QUALITY);
	QualitySlider->SetPos(m_iQuality);

	VolumeSlider->SetRange(MIN_VOLUME, MAX_VOLUME);
	VolumeSlider->SetPos(m_iVolume);

	Text.Format("Quality: %i", m_iQuality);
	SetDlgItemText(IDC_QUALITY_FRM, Text);

	Text.Format("Volume: %i", m_iVolume);
	SetDlgItemText(IDC_VOLUME_FRM, Text);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPCMImport::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CSliderCtrl *QualitySlider = (CSliderCtrl*)GetDlgItem(IDC_QUALITY);
	CSliderCtrl *VolumeSlider = (CSliderCtrl*)GetDlgItem(IDC_VOLUME);
	CString Text;

	m_iQuality = QualitySlider->GetPos();
	m_iVolume = VolumeSlider->GetPos();

	Text.Format("Quality: %i", m_iQuality);
	SetDlgItemText(IDC_QUALITY_FRM, Text);

	Text.Format("Volume: %i", m_iVolume);
	SetDlgItemText(IDC_VOLUME_FRM, Text);

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPCMImport::OnBnClickedCancel()
{
	m_iQuality = 0;
	m_iVolume = 0;
	m_pImported = NULL;
	OnCancel();
}

void CPCMImport::OnBnClickedOk()
{
	CDSample *pSample = ConvertFile();

	if (pSample == NULL)
		return;

	// remove .wav
	m_strFileName.Truncate(m_strFileName.GetLength() - 4);
	
	// Set the name
	strcpy(pSample->Name, (char*)(LPCSTR)m_strFileName);

	m_pImported = pSample;

	OnOK();
}

void CPCMImport::OnBnClickedPreview()
{
	CDSample *pSample = ConvertFile();

	if (!pSample)
		return;

	// Play sample
	theApp.GetSoundGenerator()->PreviewSample(pSample, 0, m_iQuality);
}

CDSample *CPCMImport::ConvertFile()
{
	const int		MAXINAMP = 24;

	PCMWAVEFORMAT	WaveFormat;
	int				WaveSize;

	unsigned int	Samples = 0;
	char			*SampleBuf;

	bool	Scanning = true;
	char	Header[4];
	int		BlockSize, Dec;
	int		Progress = 0;

	int		dmcbits = 0, dmcshift = 8;
	int		subsample = 0, oversampling = 75;
	int		k = 128;
	int		dmcpos;

	CDSample *pImported;

	if (!m_fSampleFile.Open(m_strPath, CFile::modeRead)) {
		MessageBox("Could not open file!");
		return NULL;
	}

	m_fSampleFile.Read(Header, 4);

	if (memcmp(Header, "RIFF", 4) != 0) {
		MessageBox("File is not a valid RIFF!");
		m_fSampleFile.Close();
		return NULL;
	}

	m_fSampleFile.Read(&BlockSize, 4);
	WaveSize = BlockSize;

	// This is not perfect, but seems to work for the files I tried
	while (Scanning) {
		m_fSampleFile.Read(Header, 4);
		
		if (memcmp(Header, "WAVE", 4)) {
			BlockSize = 0;
			m_fSampleFile.Read(&BlockSize, 4);

			if (!memcmp(Header, "fmt ", 4)) {
				// Read the wave-format
				int ReadSize = BlockSize;
				if (ReadSize > sizeof(PCMWAVEFORMAT))
					ReadSize = sizeof(PCMWAVEFORMAT);

				m_fSampleFile.Read(&WaveFormat, ReadSize);
				m_fSampleFile.Seek(BlockSize - ReadSize, CFile::current);

				if (WaveFormat.wf.wFormatTag != WAVE_FORMAT_PCM) {
					theApp.DisplayError(ID_INVALID_WAVEFILE);
					m_fSampleFile.Close();
					return NULL;
				}
			}
			else if (!memcmp(Header, "data", 4)) {
				WaveSize = BlockSize;
				Scanning = false;
			}
			else if (!memcmp(Header, "fact", 4)) {
				m_fSampleFile.Seek(BlockSize, CFile::current);
			}
			else {
				Scanning = false;
			}
		}
	}

	// Display wait cursor
	SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));

	m_iChannels		= WaveFormat.wf.nChannels;
	m_iSampleSize	= WaveFormat.wf.nBlockAlign / m_iChannels;
	Dec				= WaveFormat.wf.nBlockAlign;
	SampleBuf		= new char[SAMPLES_MAX];
	dmcpos			= 0;

	oversampling = ((BASE_FREQ / DPCM_RATES[m_iQuality]) * 100) / WaveFormat.wf.nSamplesPerSec;

	// Length of PCM samples in ms
	int Len = WaveSize / (WaveFormat.wf.nAvgBytesPerSec / 1000);

	// Maximum length of DPCM samples in ms
	int MaxLen = (SAMPLES_MAX * 8000) / (BASE_FREQ / DPCM_RATES[m_iQuality]);

	if (Len > MaxLen) {
		WaveSize = (WaveFormat.wf.nAvgBytesPerSec * MaxLen) / 1000;
	}

	int Delta, AccReady, DeltaAcc, SubSample, ScaledSample, LastAcc;

	Delta = 0;
	ScaledSample = 127;
	AccReady = 8;
	DeltaAcc = 0;
	SubSample = 0;

	bool SampleDone = false;

	int Min = -32;
	int Max = 32;
/*
	if (IsDlgButtonChecked(IDC_CLIP)) {
		Min = 0;
		Max = 63;
	}
	else {
		Min = -32;
		Max = 32;
	}
*/
	// Do the conversion, this is based on Damian Yerrick's program
	do {
		DeltaAcc >>= 1;

		if (ScaledSample > Delta) {
			Delta++;
			DeltaAcc |= 0x80;
			if (Delta > Max)
				Delta = Max;
		}
		else {
			Delta--;
			if (Delta < Min)
				Delta = Min;
		}

		AccReady--;

		if (AccReady == 0) {
			SampleBuf[Samples++] = DeltaAcc;
			LastAcc = DeltaAcc;
			DeltaAcc = 0;
			AccReady = 8;
		}
		
		SubSample += 100;

		while (SubSample > 0 && WaveSize > 0 && Samples < SAMPLES_MAX) {
			ScaledSample = (ReadSample() * m_iVolume) / 16;
			WaveSize -= Dec;
			Progress += Dec;
			SubSample -= oversampling;
		}

		if (WaveSize <= 0 && SubSample <= 0)
			SampleDone = true;

	} while (!SampleDone && (Samples < SAMPLES_MAX));
	
	
	// Adjust to make it even to x * $10 + 1 bytes
	while (Samples < SAMPLES_MAX && ((Samples & 0x0F) - 1) != 0) {
		SampleBuf[Samples++] = 0;
	}

	if (Samples > SAMPLES_MAX)
		Samples = SAMPLES_MAX;

	// Allocate sample object
	pImported = new CDSample();
	pImported->SampleData = SampleBuf;
	pImported->SampleSize = Samples;
	memset(pImported->Name, 0, 256);

	m_fSampleFile.Close();

	return pImported;
}

int CPCMImport::ReadSample(void)
{
	short SampleLeft, SampleRight;
	short Sample = 0;

	if (m_iSampleSize == 2) {
		if (m_iChannels == 2) {
			m_fSampleFile.Read(&SampleLeft, 2);
			m_fSampleFile.Read(&SampleRight, 2);
			Sample = (SampleLeft + SampleRight) / 512;
		}
		else {
			m_fSampleFile.Read(&Sample, 2);
			Sample /= 256;
		}
	}
	else {
		if (m_iChannels == 2) {
			m_fSampleFile.Read(&SampleLeft, 1);
			m_fSampleFile.Read(&SampleRight, 1);
			Sample = (SampleLeft + SampleRight) / 2;
		}
		else {
			m_fSampleFile.Read(&Sample, 1);
		}
		if (Sample & 0x80)
			Sample = -(Sample & 0x7F);
	}

	return Sample;
}
