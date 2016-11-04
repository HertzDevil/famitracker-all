/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2006  Jonathan Liss
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
#include "PCMImport.h"
#include ".\pcmimport.h"

const int MAX_QUALITY	= 16;
const int MIN_QUALITY	= 1;
const int MAX_VOLUME	= 32;
const int MIN_VOLUME	= 1;

const int DPCM_RATES[]	= {0x01AC, 0x017C, 0x0154, 0x0140, 0x011E, 0x00FE, 0x00E2, 0x00D6, 0x00BE, 0x00A0, 0x008E, 0x0080, 0x006A, 0x0054, 0x0048, 0x0036};
const int BASE_FREQ		= 1789773;
const int ZERO_ADJUST	= 0x41;			// Amount of zeroes after sample (pushing delta counter back to 0)
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
END_MESSAGE_MAP()

stImportedPCM *CPCMImport::ShowDialog()
{
	// Return import parameters, 0 if cancel

	CFileSoundDialog OpenFileDialog(TRUE, 0, 0, OFN_HIDEREADONLY, "Microsoft PCM files (*.wav)|*.wav|All files (*.*)|*.*||");

	Imported.Data = NULL;
	Imported.Name = NULL;
	Imported.Size = 0;

	OpenFileDialog.m_pOFN->lpstrInitialDir = theApp.m_pSettings->GetPath(PATH_WAV);

	if (OpenFileDialog.DoModal() == IDCANCEL)
		return &Imported;

	theApp.m_pSettings->SetPath(OpenFileDialog.GetPathName(), PATH_WAV);

	m_strPath		= OpenFileDialog.GetPathName();
	m_strFileName	= OpenFileDialog.GetFileName();

	CDialog::DoModal();

	return &Imported;
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
	OnCancel();
}

void CPCMImport::OnBnClickedOk()
{
	const int		MAXINAMP = 24;

	CProgressCtrl	*ProgressCtrl = (CProgressCtrl*)GetDlgItem(IDC_IMPORT_BAR);
	
	PCMWAVEFORMAT	WaveFormat;
	int				WaveSize;

	unsigned int	Samples = 0;
	char			*SampleBuf;

	bool	Scanning = true;
	char	Header[4];
	int		BlockSize, Dec, i;
	int		Progress = 0;

	int		dmcbits = 0, dmcshift = 8;
	int		subsample = 0, oversampling = 75;
	int		k = 128;
	int		dmcpos;

	if (!m_fSampleFile.Open(m_strPath, CFile::modeRead)) {
		MessageBox("Could not open file!");
		return;
	}

	m_fSampleFile.Read(Header, 4);

	if (memcmp(Header, "RIFF", 4) != 0) {
		MessageBox("File is not a valid RIFF!");
		m_fSampleFile.Close();
		return;
	}

	m_fSampleFile.Read(&BlockSize, 4);
	WaveSize = BlockSize;

	// This is not perfect, but seems work for most of the files I tried
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
					AfxMessageBox("Cannot load this sample!\nOnly uncompressed PCM is supported.");
					m_fSampleFile.Close();
					return;
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

	int BlockAlign;

	BlockAlign = WaveFormat.wf.nBlockAlign;
/*
	if (BlockAlign > 4)
		BlockAlign = 4;
*/
	m_iChannels		= WaveFormat.wf.nChannels;
	m_iSampleSize	= BlockAlign / m_iChannels;
	Dec				= BlockAlign;
	SampleBuf		= new char[0x4000];
	dmcpos			= 0;//MAXINAMP / 2; //0;//MAXINAMP;

	oversampling = ((BASE_FREQ / DPCM_RATES[m_iQuality - 1]) * 100) / WaveFormat.wf.nSamplesPerSec;

	// Length of PCM samples in ms
	int Len = WaveSize / (WaveFormat.wf.nAvgBytesPerSec / 1000);

	// Maximum length of DPCM samples in ms
	int MaxLen = (SAMPLES_MAX * 8000) / (BASE_FREQ / DPCM_RATES[m_iQuality - 1]);

	if (Len > MaxLen) {
		ProgressCtrl->SetRange(0, SAMPLES_MAX);
		WaveSize = (WaveFormat.wf.nAvgBytesPerSec * MaxLen) / 1000;
	}
	else
		ProgressCtrl->SetRange(0, (SAMPLES_MAX * ((Len * 100) / MaxLen)) / 100);

	int Delta, AccReady, DeltaAcc, SubSample, ScaledSample, LastAcc;

	Delta = 0;
	ScaledSample = 127;
	AccReady = 8;
	DeltaAcc = 0;
	SubSample = 0;

	bool SampleDone = false;

	int Step = 0;

	int Min = -32;
	int Max = 32;

	if (IsDlgButtonChecked(IDC_CLIP)) {
		Min = 0;
		Max = 63;
	}
	else {
		Min = -32;
		Max = 32;
	}

	// Do the conversion, this is based of Damian Yerrick's program
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

		while (SubSample > 0 && WaveSize > 0 && Samples < (SAMPLES_MAX - ZERO_ADJUST)) {
			ScaledSample = (ReadSample() * m_iVolume) / 16;
			WaveSize -= Dec;
			Progress += Dec;
			SubSample -= oversampling;
		}

		if (WaveSize <= 0 || Samples >= (SAMPLES_MAX - ZERO_ADJUST)) {
			// Revert to zero
			if (Delta > -32) {
				Step -= (Delta / 6) - 1;
				if (Step > 40) {
					ScaledSample--;
					Step = 0;
				}
			}
			else {
				SampleDone = true;
				LastAcc = 0;
			}

			if (!IsDlgButtonChecked(IDC_RESTORE) || Min == 0)
				SampleDone = true;
		}

		ProgressCtrl->SetPos(Samples);

	} while (!SampleDone && (Samples < SAMPLES_MAX));
	
	/*(SAMPLES_MAX - ZERO_ADJUST)*/
	
	if (Min == 0) {
		// Add zeroes to the end, adjusting the delta counter back to zero (remove clicks)
		for (i = (ZERO_ADJUST + 50) - 1; i > 0 && Samples < SAMPLES_MAX; i--) {
			SampleBuf[Samples++] = 0;
		}
		LastAcc = 0;
	}

	// Adjust to make it even to x * $10 + 1 bytes 
	while (((Samples & 0x0F) - 1) != 0 && Samples < SAMPLES_MAX) {
		SampleBuf[Samples++] = LastAcc /*LastAcc*/ /*0*/;
	}

	if (Samples > SAMPLES_MAX)
		Samples = SAMPLES_MAX;

	Imported.Data = SampleBuf;
	Imported.Size = Samples;
	Imported.Name = (char*)(LPCSTR)m_strFileName;

	m_fSampleFile.Close();

	OnOK();
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
	}

	return Sample;
}

