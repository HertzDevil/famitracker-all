/*
** NSF Importer
** Copyright (C) 2011 Brad Smith (rainwarrior@gmail.com)
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

#include "../stdafx.h"
#include "../FamiTracker.h"
#include "../FamiTrackerDoc.h"
#include "../SoundGen.h"
#include "../MainFrm.h"
#include "NSF_Core.h"
#include "NSF_File.h"
#include "NSF_Import.h"
#include "NSF_Import_Dlg.h"

// ============================================================================

const unsigned int FAKE_SAMPLERATE = 44100; // NotSo Fatso internal sound samplerate
const bool bUseVsync = true;
// true - read registers just before vsync interrupt (recommended)
// false - read registers after every (FAKE_SAMPLERARTE / FRAMERATE) samples
//         not recommended; might read half-updated registers depending on where
//         the 6502 execution has reached.

// this is the current write position of the famitracker document
static unsigned int iFrameCount;
static unsigned int iExpectedFrameCount;
static unsigned int iPattern;
static unsigned int iRow;
static bool bSkipFirst = true; // skip reading the first row, NotSo Fatso is still initializing

// stores the last written row for comparison and redundancy elimination
static const unsigned int MAX_IMPORT_CHANNELS = 11;
static stChanNote aLast[MAX_IMPORT_CHANNELS];

// global data objects for interaction with FamiTracker / NotSo Fatso
static CNSFCore* pCore = NULL;
static CFamiTrackerDoc* pDocument;
static CMainFrame* pMainFrame;

// PAL/Expansion status
static int iPAL = 0; // 0 = NTSC, 1 = PAL
static unsigned char Chip = SNDCHIP_NONE;

// ============================================================================
// Instrument entry data for DPCM/FDS/VRC7

struct DPCM_Entry
{
    // defines a sample
    unsigned int iBank;
    unsigned int iAddress;
    unsigned int iLength;
    // defines playbaack
    unsigned int iFreq;
    bool bLoop;
    // DPCM sample index in FamiTracker
    int DPCM;
};

struct FDS_Entry
{
    // defines wavetable
    BYTE nWaveTable[0x40];
    // TODO when 0.3.7 source is available, also read mod table and use Hxx for mod depth,
    //      and stop using frame average added to regular note frequency 
    // famitracker instrument
    int iInstrument;
};

struct VRC7_Entry
{
    // VRC7 registers
    BYTE nRegisters[8];
    // famitracker instrument
    int iInstrument;
};

static const unsigned int MAX_SAMPLES = 12*8;
static DPCM_Entry aSamples[MAX_SAMPLES];
static unsigned int iSamples = 0;

static const unsigned int MAX_FDS = MAX_INSTRUMENTS - 1;
static FDS_Entry aFDS[MAX_FDS];
static unsigned int iFDS = 0;
static unsigned int iFDS_Last = 0;

static const unsigned int MAX_VRC7 = MAX_INSTRUMENTS - 16;
static VRC7_Entry aVRC7[MAX_VRC7];
static unsigned int iVRC7 = 0;
static unsigned int iVRC7_Last = 0;
static int iVRC7_Note[6];
static int iVRC7_Octave[6];

// ============================================================================
// Forward function declarations

void GenerateRow(CNSFCore& NSFCore);
void ReadSquares(CSquareWaves& Squares, stChanNote& S1, stChanNote& S2);
void ReadTND(CTNDWaves& TND, stChanNote& T, stChanNote& N, stChanNote& D);
void ReadVRC6(CVRC6PulseWave* Squares, CVRC6SawWave& Saw, stChanNote& S1, stChanNote& S2, stChanNote& W);
void ReadMMC5(CMMC5SquareWave* Squares, stChanNote& S1, stChanNote& S2);
void ReadFDS(CFDSWave& FDS, stChanNote& F);
void ReadVRC7(CNSFCore& NSFCore, stChanNote* F, stChanNote* Last);
void GetNote(int Register, int& Note, int& Octave, int& Pitch);
void GetNoteSaw(int Register, int& Note, int& Octave, int& Pitch);
void GetNoteFDS(int Register, int& Note, int& Octave, int& Pitch);
void GetNoteVRC7(BYTE Reg1, BYTE Reg2, int& Note, int& Octave, int& Pitch);
int GetBendVRC7(BYTE Reg1, BYTE Reg2, int Note, int Octave);
int GetDuty(int Duty);
int GetDutyVRC6(int Duty);
void GetNoise(unsigned int Freq, int& Note, int& Octave);
unsigned int GetDMCFreq(unsigned int Freq);
int GetDPCM(DPCM_Entry& Entry, CTNDWaves& TND);
int GetFDS(CFDSWave& FDS);
int GetVRC7(CNSFCore& NSFCore, bool bPatch);

// ============================================================================
// Main interface

bool NSF_Import(LPCSTR FileName)
{
	CNSFFile NSFFile;
    CNSFCore NSFCore;

    // load the NSF
    if (NSFFile.LoadFile(FileName,1,1) != 0)
    {
        MessageBox(GetActiveWindow(),
            "Could not parse NSF file.",
            "NSF Import",
            MB_OK | MB_ICONWARNING);
        return false;
    }

    // get track/time information from settings dialog
    NSF_Import_Settings Settings;
    if (!Do_NSF_Import_Dlg(FileName, NSFFile, Settings))
    {
        return true;
    }

    // setup NSF player
    if (
        NSFCore.Initialize() == 0 ||
        NSFCore.SetPlaybackOptions(FAKE_SAMPLERATE, 1) == 0 ||
        NSFCore.LoadNSF(&NSFFile) == 0
        )
    {
        MessageBox(GetActiveWindow(),
            "Could not initialize NotSo Fatso Core.",
            "NSF Import",
            MB_OK | MB_ICONWARNING);
        return false;
    }
    for(unsigned int i = 0; i < 29; i++)
    {
        NSFCore.SetChannelOptions(i,1,255,0,0);
    }
    NSFCore.SetMasterVolume(0.5f); // a lot of stuff gets distorted

    // get handles to FamiTracker
    pDocument = (CFamiTrackerDoc*)theApp.GetActiveDocument();
    pMainFrame = (CMainFrame*)theApp.GetMainWnd();

    // create the new document
    if (!pDocument->OnNewDocument())
    {
        MessageBox(GetActiveWindow(),
            "Could not create new FamiTracker document.",
            "NSF Import",
            MB_OK | MB_ICONWARNING);
        return false;
    }
    pMainFrame->ClearInstrumentList();

    // setup for PAL or NTSC
    iPAL = (NSFFile.nIsPal != 1) ? 0 : 1;
    unsigned int FrameLength = iPAL ? (FAKE_SAMPLERATE / 50) : (FAKE_SAMPLERATE / 60);
    pDocument->SetEngineSpeed(0);
    pDocument->SetMachine(iPAL ? PAL : NTSC);
    theApp.GetSoundGenerator()->LoadMachineSettings(
        pDocument->GetMachine(), iPAL ? CAPU::FRAME_RATE_PAL : CAPU::FRAME_RATE_NTSC);

    // set expansion chip
    Chip = SNDCHIP_NONE;
    if (NSFFile.nChipExtensions & 0x04) Chip = SNDCHIP_FDS;
    if (NSFFile.nChipExtensions & 0x08) Chip = SNDCHIP_MMC5;
    if (NSFFile.nChipExtensions & 0x01) Chip = SNDCHIP_VRC6;
    if (NSFFile.nChipExtensions & 0x02) Chip = SNDCHIP_VRC7;
    pDocument->SelectExpansionChip(Chip);

    // setup tempo and patterns
	pDocument->SetSongSpeed(1);
    pDocument->SetSongTempo(iPAL ? 125 : 150);
	pDocument->SetPatternLength(256);
	pDocument->SetFrameCount(1);
    for (int Channel = 0; Channel < pDocument->GetChannelCount(); ++Channel)
    {
        pDocument->SetPatternAtFrame(0, Channel, 0);
    }
    pDocument->SetEffColumns(0,1);
    pDocument->SetEffColumns(1,1);

    // setup title/artist/copyright
    pDocument->SetSongInfo(NSFFile.szGameTitle, NSFFile.szArtist, NSFFile.szCopyright);
	pMainFrame->SetSongInfo(pDocument->GetSongName(), pDocument->GetSongArtist(), pDocument->GetSongCopyright());

    // add default instruments for sound chip
    pDocument->AddInstrument("blank", SNDCHIP_NONE);
    pMainFrame->AddInstrument(0);
    if (Chip == SNDCHIP_MMC5)
    {
        pDocument->AddInstrument("blank", SNDCHIP_MMC5);
        pMainFrame->AddInstrument(1);
        pDocument->SetEffColumns(5,1);
        pDocument->SetEffColumns(6,1);
    }
    else if(Chip == SNDCHIP_VRC6)
    {
        pDocument->AddInstrument("blank", SNDCHIP_VRC6);
        pMainFrame->AddInstrument(1);
        pDocument->SetEffColumns(5,1);
        pDocument->SetEffColumns(6,1);
    }
    else if(Chip == SNDCHIP_VRC7)
    {
        for (unsigned int i=1; i < 0x10; ++i)
        {
            pDocument->AddInstrument("vrc7 preset", SNDCHIP_VRC7);
            pMainFrame->AddInstrument(i);
            CInstrumentVRC7* pInst = (CInstrumentVRC7*)pDocument->GetInstrument(i);
            pInst->SetPatch(i);
        }
        pDocument->SetEffColumns( 5, 1);
        pDocument->SetEffColumns( 6, 1);
        pDocument->SetEffColumns( 7, 1);
        pDocument->SetEffColumns( 8, 1);
        pDocument->SetEffColumns( 9, 1);
        pDocument->SetEffColumns(10, 1);
    }

    // settings taken from settings dialog
    int iTrack = Settings.iTrack;
    unsigned int iFrames = Settings.iSeconds * (iPAL ? 50 : 60);

    // reset counters and log data
    iFrameCount = 0;
    iExpectedFrameCount = 0;
    iPattern = 0;
    iRow = 0;
    iSamples = 0;
    iFDS = 0;
    iFDS_Last = 0;
    iVRC7 = 0;
    iVRC7_Last = 0;
    for(unsigned int i=0; i < 6; ++i)
    {
        iVRC7_Note[i] = 0;
        iVRC7_Octave[i] = 0;
    }
    bSkipFirst = true;
    for(unsigned int i=0; i < MAX_IMPORT_CHANNELS; ++i)
    {
        ::memset(&aLast[i], 0, sizeof(aLast[i]));
        aLast[i].Vol = 0x10;
    }

    // WAV output for debugging
    FILE* fd = NULL;
    if(Settings.bDebugWAV)
    {
        // just a really simple WAV header
        ::fopen_s(&fd, "debug_sound.wav", "wb");
        if(fd)
        {
            unsigned int data_size = FrameLength * iFrames * 2;
            unsigned int file_size =  44 + data_size;

            unsigned char header[44] = {
                'R','I','F','F',
                (file_size >> 0 ) & 0xFF,
                (file_size >> 8 ) & 0xFF,
                (file_size >> 16) & 0xFF,
                (file_size >> 24) & 0xFF,
                'W','A','V','E',
                'f','m','t',' ', // fmt chunk
                16,0,0,0, // chunk size
                1,0, // PCM
                1,0, // channels
                0x44,0xAC,0x00,0x00, // 44100
                0x88,0x58,0x01,0x00, // 88200
                8, 0, // bytes per sample
                16, 0, // bits per sample
                'd','a','t','a',
                (data_size >> 0 ) & 0xFF,
                (data_size >> 8 ) & 0xFF,
                (data_size >> 16) & 0xFF,
                (data_size >> 24) & 0xFF
            };
            ::fwrite(header,44,1,fd);
        }
    }

    // play the song and log registers to the document
    if (iTrack > NSFFile.nTrackCount || iTrack < 1) iTrack = 1; // for safety
    bool bSyncWarning = true;
    NSFCore.SetTrack((BYTE)iTrack-1);
    NSFCore.mWave_TND.bEdit_LoopReset = false;
    NSFCore.mWave_TND.bEdit_DMCTriggered = false;
    pCore = &NSFCore;
    while (!NSFCore.SongCompleted() && (iFrameCount < iFrames))
    {
        // read and log registers
        if (!bUseVsync)
        {
            GenerateRow(NSFCore);
            NSFCore.mWave_TND.bEdit_DMCTriggered = false;
            ++iFrameCount;
        }

        // just in case we aren't getting frames from the vsync callback
        // (haven't seen it happening, but if it happens, don't want
        // an infinite loop without at least warning)
        if (bSyncWarning)
        {
            ++iExpectedFrameCount;
            if (iExpectedFrameCount > (iFrameCount + 120))
            {
                if (IDYES !=
                    MessageBox(GetActiveWindow(),
                        "Frame counter is out of sync.\n"
                        "The importer may hang.\n"
                        "Continue?",
                        "NSF Import",
                        MB_YESNO | MB_ICONWARNING))
                {
                    return true;
                }
                bSyncWarning = false;
            }
        }

        // advance sound engine
        static BYTE DummyBuffer[(4*FAKE_SAMPLERATE/50)+2]; // note +2, NotSo Fatso has a bug, goes over (??!)
        if (0 == NSFCore.GetSamples(DummyBuffer,2 * FrameLength))
        {
            break;
        }

        // write WAV
        if(Settings.bDebugWAV)
        {
            if (fd) ::fwrite(DummyBuffer, FrameLength * 2, 1, fd);
        }
    }
    pCore = NULL;

    // save WAV
    if(Settings.bDebugWAV)
    {
        if (fd) ::fclose(fd);
    }

    // finish
    NSFFile.Destroy();
    NSFCore.Destroy();
    pDocument->SetModifiedFlag();
    pDocument->UpdateAllViews(NULL, UPDATE_ENTIRE);
    pMainFrame->RedrawWindow();

    return true;
}

// ============================================================================

// callback from NotSo Fatso when a vsync interrupt is about to occur
// (this is the best time to read the hardware registers)
void NSF_Import_Vsync()
{
    if (bUseVsync && pCore)
    {
        GenerateRow(*pCore);
        pCore->mWave_TND.bEdit_DMCTriggered = false;
        ++iFrameCount;
    }
}

// ============================================================================

// generates a single row from the current state of the NSFCore registers
// makes several calls to ReadXXX (XXX = various chip/channels)
// then cleans up redundant data and writes it to FamiTracker's document
void GenerateRow(CNSFCore& NSFCore)
{
    // first row is skipped, NotSo Fatso hasn't played any music yet
    if (bSkipFirst)
    {
        bSkipFirst = false;
        return;
    }

    // this is the row data we are about to fill in
    stChanNote Data[MAX_IMPORT_CHANNELS];
    for(unsigned int i=0; i < MAX_IMPORT_CHANNELS; ++i)
    {
        Data[i] = aLast[i]; // fill with last row by default
    }

    // read hardware state to fill row data
    ReadSquares(NSFCore.mWave_Squares, Data[0], Data[1]);
    ReadTND(NSFCore.mWave_TND, Data[2], Data[3], Data[4]);
    if (Chip == SNDCHIP_VRC6)
    {
        ReadVRC6(NSFCore.mWave_VRC6Pulse, NSFCore.mWave_VRC6Saw, Data[5], Data[6], Data[7]);
    }
    else if (Chip == SNDCHIP_MMC5)
    {
        ReadMMC5(NSFCore.mWave_MMC5Square, Data[5], Data[6]);
    }
    else if (Chip == SNDCHIP_FDS)
    {
        ReadFDS(NSFCore.mWave_FDS, Data[5]);
    }
    else if (Chip == SNDCHIP_VRC7)
    {
        ReadVRC7(NSFCore, Data+5, aLast+5);
    }

    // remove redundant data, and write to the FamiTracker document
    for(int i=0; i < pDocument->GetChannelCount(); ++i)
    {
        stChanNote& Last = aLast[i];
        stChanNote Temp = Data[i];

        // hide rows already accounted for
        if( Temp.Note == Last.Note &&
            Temp.Octave == Last.Octave )
        {
            if ((Chip == SNDCHIP_VRC7 && i >= 5) ? // VRC7 has special rule for hiding unnecessary rows
                (Temp.Instrument == MAX_INSTRUMENTS) : true)
            {
                Temp.Note = 0;
                Temp.Octave = 0;
            }
        }
        if(Temp.Note == 0 || Temp.Note == HALT)
        {
            Temp.Instrument = MAX_INSTRUMENTS;
        }
        if(Temp.Vol == Last.Vol && (Temp.Note == 0 || Temp.Note == HALT))
        {
            Temp.Vol = 0x10;
        }
        for (unsigned int j=0; j < MAX_EFFECT_COLUMNS; ++j)
        {
            if (Temp.EffNumber[j] == Last.EffNumber[j] &&
                Temp.EffParam[j] == Last.EffParam[j])
            {
                Temp.EffNumber[j] = 0;
                Temp.EffParam[j] = 0;
            }
        }

        // write cell of data
        pDocument->SetDataAtPattern(0, iPattern, i, iRow, &Temp);

        // store last state
        Last = Data[i];
    }

    // advance row, generate new pattern if necessary
    ++iRow;
    if (iRow >= 256)
    {
        iRow = 0;
        ++iPattern;
        pDocument->SetFrameCount(iPattern+1);
        for (int Channel = 0; Channel < pDocument->GetChannelCount(); ++Channel)
        {
            pDocument->SetPatternAtFrame(iPattern, Channel, iPattern);
        }
    }
}

// ============================================================================
// hardware channel readers

void ReadSquares(CSquareWaves& Squares, stChanNote& S1, stChanNote& S2)
{
    int Note, Octave, Pitch;

    GetNote((int)Squares.nFreqTimer[0].W, Note, Octave, Pitch);
    S1.Note = Note;
    S1.Octave = Octave;
    S1.Instrument = 0;
    S1.Vol = (Squares.nLengthCount[0] && !Squares.bSweepForceSilence[0]) ? Squares.nVolume[0] : 0;
    S1.EffNumber[0] = EF_PITCH;
    S1.EffParam[0] = (unsigned char)(Pitch + 0x80);
    S1.EffNumber[1] = EF_DUTY_CYCLE;
    S1.EffParam[1] = GetDuty(Squares.nDutyCycle[0]);

    GetNote((int)Squares.nFreqTimer[1].W, Note, Octave, Pitch);
    S2.Note = Note;
    S2.Octave = Octave;
    S2.Instrument = 0;
    S2.Vol = (Squares.nLengthCount[1] && !Squares.bSweepForceSilence[1]) ? Squares.nVolume[1] : 0;
    S2.EffNumber[0] = EF_PITCH;
    S2.EffParam[0] = (unsigned char)(Pitch + 0x80);
    S2.EffNumber[1] = EF_DUTY_CYCLE;
    S2.EffParam[1] = GetDuty(Squares.nDutyCycle[1]);
}

void ReadTND(CTNDWaves& TND, stChanNote& T, stChanNote& N, stChanNote& D)
{
    int Note, Octave, Pitch;

    GetNote((int)TND.nTriFreqTimer.W, Note, Octave, Pitch);
    T.Note = Note;
    T.Octave = Octave;
    if (!TND.nTriLengthCount || !TND.nTriLinearCount)
    {
        T.Note = HALT;
        T.Octave = 0;
    }
    T.Instrument = 0;
    T.EffNumber[0] = EF_PITCH;
    T.EffParam[0] = (unsigned char)(Pitch + 0x80);

    GetNoise(TND.nNoiseFreqTimer, Note, Octave);
    N.Note = Note;
    N.Octave = Octave;
    N.Instrument = 0;
    N.Vol = TND.nNoiseLengthCount ? TND.nNoiseVolume : 0;
    N.EffNumber[0] = EF_DUTY_CYCLE;
    N.EffParam[0] = TND.bNoiseRandomMode == 6 ? 1 : 0;

    if (TND.bEdit_DMCTriggered)
    {
        unsigned int iAddr = TND.nDMCDMAAddr_Load;
        unsigned int iBank = TND.nDMCDMABank_Load;
        unsigned int iLen = TND.nDMCLength;
        bool bLoop = TND.bDMCLoop != 0;
        unsigned int iFreq = GetDMCFreq(TND.nDMCFreqTimer);

        DPCM_Entry Entry = { iBank, iAddr, iLen, iFreq, bLoop, 0 };
        unsigned int Index = GetDPCM(Entry, TND);

        if (Index >= MAX_SAMPLES)
        {
            D.Note = HALT;
            D.Octave = 0;
        }
        else
        {
            D.Note = (Index % 12) + 1;
            D.Octave = Index / 12;
        }
    }
    else
    {
        D.Note = 0;
        D.Octave = 0;
    }
    D.Instrument = 0;
}

void ReadVRC6(CVRC6PulseWave* Squares, CVRC6SawWave& Saw, stChanNote& S1, stChanNote& S2, stChanNote& W)
{
    int Note, Octave, Pitch;

    GetNote((int)Squares[0].nFreqTimer.W, Note, Octave, Pitch);
    S1.Note = Note;
    S1.Octave = Octave;
    if (!Squares[0].bChannelEnabled)
    {
        S1.Note = HALT;
        S1.Octave = 0;
    }
    S1.Instrument = 1;
    S1.Vol = Squares[0].nVolume;
    S1.EffNumber[0] = EF_PITCH;
    S1.EffParam[0] = (unsigned char)(Pitch + 0x80);
    S1.EffNumber[1] = EF_DUTY_CYCLE;
    S1.EffParam[1] = GetDutyVRC6(Squares[0].nDutyCycle);

    GetNote((int)Squares[1].nFreqTimer.W, Note, Octave, Pitch);
    S2.Note = Note;
    S2.Octave = Octave;
    if (!Squares[1].bChannelEnabled)
    {
        S2.Note = HALT;
        S2.Octave = 0;
    }
    S2.Instrument = 1;
    S2.Vol = Squares[1].nVolume;
    S2.EffNumber[0] = EF_PITCH;
    S2.EffParam[0] = (unsigned char)(Pitch + 0x80);
    S2.EffNumber[1] = EF_DUTY_CYCLE;
    S2.EffParam[1] = GetDutyVRC6(Squares[1].nDutyCycle);

    GetNoteSaw((int)Saw.nFreqTimer.W, Note, Octave, Pitch);
    W.Note = Note;
    W.Octave = Octave;
    if (!Saw.bChannelEnabled)
    {
        W.Note = HALT;
        W.Octave = 0;
    }
    W.Instrument = 1;
    W.Vol = Saw.nAccumRate >> 1;
    if (W.Vol > 0xF) W.Vol = 0xF; // for safety (higher levels unsupported)
    W.EffNumber[0] = EF_PITCH;
    W.EffParam[0] = (unsigned char)(Pitch + 0x80);
}

void ReadMMC5(CMMC5SquareWave* Squares, stChanNote& S1, stChanNote& S2)
{
    int Note, Octave, Pitch;

    GetNote((int)Squares[0].nFreqTimer.W, Note, Octave, Pitch);
    S1.Note = Note;
    S1.Octave = Octave;
    S1.Instrument = 1;
    S1.Vol = (Squares[0].nLengthCount && Squares[0].nFreqTimer.W >= 8) ? Squares[0].nVolume : 0;
    S1.EffNumber[0] = EF_PITCH;
    S1.EffParam[0] = (unsigned char)(Pitch + 0x80);
    S1.EffNumber[1] = EF_DUTY_CYCLE;
    S1.EffParam[1] = GetDutyVRC6(Squares[0].nDutyCycle);

    GetNote((int)Squares[1].nFreqTimer.W, Note, Octave, Pitch);
    S2.Note = Note;
    S2.Octave = Octave;
    S2.Instrument = 1;
    S2.Vol = (Squares[1].nLengthCount && Squares[1].nFreqTimer.W >= 8) ? Squares[1].nVolume : 0;
    S2.EffNumber[0] = EF_PITCH;
    S2.EffParam[0] = (unsigned char)(Pitch + 0x80);
    S2.EffNumber[1] = EF_DUTY_CYCLE;
    S2.EffParam[1] = GetDutyVRC6(Squares[1].nDutyCycle);
}

void ReadFDS(CFDSWave& FDS, stChanNote& F)
{
    int Note, Octave, Pitch;

    int Freq = (int)FDS.nFreq.W;
    if (FDS.nEdit_FreqCount)
    {
        int Avg = FDS.nEdit_FreqAccum / FDS.nEdit_FreqCount;
        Freq += Avg; // average LFO value this frame
    }
    FDS.nEdit_FreqAccum = 0; // clear accumulator for next frame
    FDS.nEdit_FreqCount = 0;

    int Volume = ((FDS.nVolume * 0xF) / 0x20);
    if (Volume > 0xF) Volume = 0xF;
    if (Volume < 0x0) Volume = 0x0;

    GetNoteFDS(Freq, Note, Octave, Pitch);
    F.Note = Note;
    F.Octave = Octave;
    F.Instrument = GetFDS(FDS);
    F.Vol = Volume;
    F.EffNumber[0] = EF_PITCH;
    F.EffParam[0] = (unsigned char)(Pitch + 0x80);
}

void ReadVRC7(CNSFCore& NSFCore, stChanNote* F, stChanNote* Last)
{
    bool bPatch = NSFCore.bEdit_VRC7_Patch; // patch change this frame
    NSFCore.bEdit_VRC7_Patch = false;

    int iLastInstrument = iVRC7_Last;
    int iCustomInstrument = GetVRC7(NSFCore, bPatch); // find custom instrument matching current patch
    if (iCustomInstrument == iLastInstrument) bPatch = false; // same patch was reuploaded

    for (unsigned int i=0; i < 6; ++i)
    {
        bool bTrigger = NSFCore.bEdit_VRC7_Trigger[i]; // note triggered this frame
        bool bRelease = NSFCore.bEdit_VRC7_Release[i]; // note triggered this frame
        NSFCore.bEdit_VRC7_Trigger[i] = false;
        NSFCore.bEdit_VRC7_Release[i] = false;

        // status register
        // Reg0 = custom patch data, not needed here
        BYTE Reg1 = NSFCore.VRC7Chan[0][i]; // ffffffff = freq low bits
        BYTE Reg2 = NSFCore.VRC7Chan[1][i]; // ???tooof = ?, trigger, octave, freq high bit
        BYTE Reg3 = NSFCore.VRC7Chan[2][i]; // iiiivvvv = instrument, volume
        bool bKeyOn = (Reg2 & 0x10) != 0;

        int Note, Octave, Pitch;
        GetNoteVRC7(Reg1, Reg2, Note, Octave, Pitch);
        F[i].Note = Note;
        F[i].Octave = Octave;

        int iInstrument = (Reg3 & 0xF0) >> 4;
        F[i].Instrument = (iInstrument == 0) ? iCustomInstrument : iInstrument;

        F[i].Vol = 0xF - (Reg3 & 0x0F);

        F[i].EffNumber[0] = EF_PITCH;
        F[i].EffParam[0] = (unsigned char)(Pitch + 0x80);

        bool bUseNote = bTrigger ||
            (!bTrigger && bPatch && (iInstrument == 0)) || // custom patch change without retrigger
            (F[i].Note != Last[i].Note) || (F[i].Octave != Last[i].Octave); // portamento

        if (!bKeyOn)
        {
            F[i].Note = bRelease ? HALT : 0;
            F[i].Octave = 0;

            // pitch bend after note release; Famitracker doesn't support 3FF after a release
            // so we need to do this purely through Pxx (might be an approximation due to range/octave)
            Pitch = GetBendVRC7(Reg1, Reg2, iVRC7_Note[i], iVRC7_Octave[i]);
            F[i].EffParam[0] = (unsigned char)(Pitch + 0x80);
            bUseNote = false;
        }
        else
        {
            // always remember last note before note release
            iVRC7_Note[i] = Note;
            iVRC7_Octave[i] = Octave;
        }

        if (bUseNote)
        {
            // use 3FF to allow patch/porta without retrigger
            F[i].EffNumber[1] = EF_PORTAMENTO;
            F[i].EffParam[1] = bTrigger ? 0x00 : 0xFF;
        }
        else
        {
            F[i].Instrument = MAX_INSTRUMENTS; // signifies row should have no note, cleaned up in GenerateRow
            F[i].EffNumber[1] = 0;
            F[i].EffParam[1] = 0;
        }
    }
}

// ============================================================================
// register pitch setting to famitracker note converters
// in general, these compare a register value to the pitch tables used
// internally by FamiTracker to find the closest match, then
// applies a Pxx effect to make up the difference.

void GetNote(int Register, int& Note, int& Octave, int& Pitch)
{
    if (Register == 0)
    {
        Note = HALT;
        Octave = 0;
        Pitch = 0;
        return;
    }

    CSoundGen* pSoundGen = theApp.GetSoundGenerator();
    int MidiNote = 0;
    int MinDiff = 9999999;
    for (unsigned int i=1; i < 95; ++i)
    {
        int Val = pSoundGen->m_iNoteLookupTable[i];
        int Diff = Val - Register;
        int AbsDiff = Diff < 0 ? -Diff : Diff;
        if (AbsDiff < MinDiff)
        {
            MinDiff = AbsDiff;
            MidiNote = i;
            Pitch = Diff;
        }
    }

    Note = (MidiNote % 12) + 1;
    Octave = MidiNote / 12;
}

void GetNoteSaw(int Register, int& Note, int& Octave, int& Pitch)
{
    if (Register == 0)
    {
        Note = HALT;
        Octave = 0;
        Pitch = 0;
        return;
    }

    CSoundGen* pSoundGen = theApp.GetSoundGenerator();
    int MidiNote = 0;
    int MinDiff = 9999999;
    for (unsigned int i=1; i < 95; ++i)
    {
        int Val = pSoundGen->m_iNoteLookupTableSaw[i];
        int Diff = Val - Register;
        int AbsDiff = Diff < 0 ? -Diff : Diff;
        if (AbsDiff < MinDiff)
        {
            MinDiff = AbsDiff;
            MidiNote = i;
            Pitch = Diff;
        }
    }

    Note = (MidiNote % 12) + 1;
    Octave = MidiNote / 12;
}

void GetNoteFDS(int Register, int& Note, int& Octave, int& Pitch)
{
    if (Register == 0)
    {
        Note = HALT;
        Octave = 0;
        Pitch = 0;
        return;
    }

    CSoundGen* pSoundGen = theApp.GetSoundGenerator();
    int MidiNote = 0;
    int MinDiff = 9999999;
    for (unsigned int i=1; i < 95; ++i)
    {
        int Val = pSoundGen->m_iNoteLookupTableFDS[i];
        int Diff = Register - Val;
        int AbsDiff = Diff < 0 ? -Diff : Diff;
        if (AbsDiff < MinDiff)
        {
            MinDiff = AbsDiff;
            MidiNote = i;
            Pitch = Diff;
        }
    }

    Note = (MidiNote % 12) + 1;
    Octave = MidiNote / 12;
}

void GetNoteVRC7(BYTE RegF, BYTE RegO, int& Note, int& Octave, int& Pitch)
{
    // octave register high 4 bits ignored
    RegO = RegO & 0x0F;

    if (RegO == 0 && RegF == 0)
    {
        Note = HALT;
        Octave = 0;
        Pitch = 0;
        return;
    }

    int Register = RegF + ((RegO & 0x01) << 8); // frequency includes low bit from octave register
	const int FREQ_TABLE[] = {172, 181, 192, 204, 216, 229, 242, 257, 272, 288, 305, 323};

    Note = 0;
    Octave = 0;
    int MinDiff = 9999999;
    for (unsigned int i=0; i < 12; ++i)
    {
        int Val = FREQ_TABLE[i];
        int Diff = Register - Val;
        int AbsDiff = Diff < 0 ? -Diff : Diff;
        if (AbsDiff < MinDiff)
        {
            MinDiff = AbsDiff;
            Note = i + 1;
            Octave = RegO >> 1;
            Pitch = Diff;
        }
    }
}

// VRC7 can execute arbitrary pitch bends after a note release,
// but FamiTracker cannot (can't change note/octave without retriggering the note),
// so I attempt to get as close as I can with a Pxx bend. In most
// cases the bend is shallow enough that this is still completely accurate.
int GetBendVRC7(BYTE RegF, BYTE RegO, int Note, int Octave)
{
    // octave register high 4 bits ignored
    RegO = RegO & 0x0F;
    int Register = RegF + ((RegO & 0x01) << 8); // frequency includes low bit from octave register

    const int FREQ_TABLE[] = {172, 181, 192, 204, 216, 229, 242, 257, 272, 288, 305, 323};

    // adjust register frequency to match the desired octave (note: loses accuracy)
    int NewOctave = RegO >> 1;
    while (NewOctave < Octave)
    {
        ++NewOctave;
        Register /= 2;
    }
    while (NewOctave > Octave)
    {
        --NewOctave;
        Register *= 2;
    }

    // safety clamp
    if (Note < 1) Note = 1;
    if (Note > 12) Note = 12;

    int Original = FREQ_TABLE[Note-1]; // original register frequency

    // clamp to maximum range
    int Diff = Register - Original;
    if (Diff > 127) Diff = 127;
    if (Diff < -128) Diff = -128;

    return Diff;
}

// ============================================================================
// duty/noise/DPCM frequency settings, conversion from NotSo Fatso internal
// representation to FamiTracker.

int GetDuty(int Duty)
{
    static const int RESULT[0x10] = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3 };
    return RESULT[Duty & 0x0F];
}

int GetDutyVRC6(int Duty)
{
    static const int RESULT[0x10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 8, 8, 8, 8, 8, 8 };
    return RESULT[Duty & 0x0F];
}

void GetNoise(unsigned int Freq, int& Note, int& Octave)
{
    if(Freq == 0)
    {
        Note = HALT;
        Octave = 0;
        return;
    }

    unsigned int TABLE[0x10] = {
        0xFE4, 0x752, 0x3F8, 0x2FA,
        0x1FC, 0x17C, 0x0FE, 0x0CA,
        0x0A0, 0x080, 0x060, 0x040,
        0x020, 0x010, 0x008, 0x004 };
    int Val = 0xF;
    for(unsigned int i=0; i<0x10; ++i)
    {
        if (TABLE[i] <= Freq)
        {
            Val = i;
            break;
        }
    }
    Val += 4 * 12;
    Note = (Val % 12) + 1;
    Octave = Val / 12;
}

unsigned int GetDMCFreq(unsigned int Freq)
{
    unsigned int TABLE[2][0x10] = {
    /* NTSC */ {0x1AC,0x17C,0x154,0x140,0x11E,0x0FE,0x0E2,0x0D6,0x0BE,0x0A0,0x08E,0x080,0x06A,0x054,0x048,0x036},
    /* PAL  */ {0x18C,0x160,0x13A,0x128,0x108,0x0EA,0x0D0,0x0C6,0x0B0,0x094,0x082,0x076,0x062,0x04E,0x042,0x032}
    };
    for (unsigned int i=0; i < 0x10; ++i)
    {
        if (TABLE[iPAL][i] <= Freq)
        {
            return i;
        }
    }
    return 0;
}

// ============================================================================
// instrument builders, these create a new instrument when necessary

int GetDPCM(DPCM_Entry& Entry, CTNDWaves& TND)
{
    // see if we've already made this sample entry
    for (unsigned int i=0; i < iSamples; ++i)
    {
        DPCM_Entry& Sample = aSamples[i];
        if (Sample.iBank == Entry.iBank &&
            Sample.iAddress == Entry.iAddress &&
            Sample.iLength == Entry.iLength &&
            Sample.iFreq == Entry.iFreq &&
            Sample.bLoop == Entry.bLoop)
        {
            return i;
        }
    }

    // if not, see if we've already saved the DPCM
    for (unsigned int i=0; i < iSamples; ++i)
    {
        DPCM_Entry& Sample = aSamples[i];
        if (Sample.iBank == Entry.iBank &&
            Sample.iAddress == Entry.iAddress &&
            Sample.iLength == Entry.iLength)
        {
            // reuse to make a new sample
            if (iSamples >= MAX_SAMPLES)
            {
                return MAX_SAMPLES; // out of sample slots
            }

            DPCM_Entry& NewSample = aSamples[iSamples];
            NewSample = Entry;
            NewSample.iFreq = Entry.iFreq;
            NewSample.bLoop = Entry.bLoop;
            NewSample.DPCM = Sample.DPCM;

            unsigned int Index = iSamples;
            unsigned int Note = Index % 12;
            unsigned int Octave = Index / 12;
            CInstrument2A03* Instrument = (CInstrument2A03*)pDocument->GetInstrument(0);
            Instrument->SetSample(Octave, Note, NewSample.DPCM);
            Instrument->SetSamplePitch(Octave, Note, NewSample.iFreq);
            Instrument->SetSampleLoop(Octave, Note, NewSample.bLoop);
            Instrument->SetSampleLoopOffset(Octave, Note, 0);

            ++iSamples;
            return Index;
        }
    }

    // DPCM does not exist yet, make a new one
    int DPCM = pDocument->GetFreeDSample();
    if (DPCM < 0)
    {
        return MAX_SAMPLES; // out of samples
    }
    else
    {
        DPCM_Entry NewSample = Entry;
        NewSample.DPCM = DPCM + 1; // 0 = no sample?

        CDSample* pDS = pDocument->GetDSample(DPCM);
        pDS->Allocate(Entry.iLength, (char*)(&TND.pDMCDMAPtr[Entry.iBank][0] + Entry.iAddress));

        unsigned int Index = iSamples;
        unsigned int Note = Index % 12;
        unsigned int Octave = Index / 12;
        CInstrument2A03* Instrument = (CInstrument2A03*)pDocument->GetInstrument(0);
        Instrument->SetSample(Octave, Note, NewSample.DPCM);
        Instrument->SetSamplePitch(Octave, Note, NewSample.iFreq);
        Instrument->SetSampleLoop(Octave, Note, NewSample.bLoop);
        Instrument->SetSampleLoopOffset(Octave, Note, 0);

        aSamples[iSamples] = NewSample;
        ++iSamples;
        return Index;
    }
}

int GetFDS(CFDSWave& FDS)
{
    if (!FDS.bEdit_TableWrite)
    {
        return iFDS_Last; // wavetable hasn't changed
    }
    FDS.bEdit_TableWrite = false; // clear table write flag

    // check if this wavetable already exists
    for (unsigned int i=0; i < iFDS; ++i)
    {
        FDS_Entry& Entry = aFDS[i];
        bool bMatch = true;
        for (unsigned int s=0; s<0x40; ++s)
        {
            if(FDS.nWaveTable[s] != Entry.nWaveTable[s])
            {
                bMatch = false;
                break;
            }
        }

        if (bMatch)
        {
            iFDS_Last = Entry.iInstrument;
            return iFDS_Last;
        }
    }

    // check if there is room for a new entry
    if (iFDS >= MAX_FDS)
    {
        iFDS_Last = 0;
        return 0; // instrument 0 is 2A03, shows that we couldn't make the sample
    }

    // build a new instrument
    {
        FDS_Entry& Entry = aFDS[iFDS];
        ++iFDS;

        Entry.iInstrument = pDocument->AddInstrument("FDS", SNDCHIP_FDS);
        pMainFrame->AddInstrument(Entry.iInstrument);
        CInstrumentFDS* pInst = (CInstrumentFDS*)pDocument->GetInstrument(Entry.iInstrument);

        ::memcpy(Entry.nWaveTable, FDS.nWaveTable, 0x40);
        for (unsigned int s=0; s < 0x40; ++s)
        {
            pInst->SetSample(s, Entry.nWaveTable[s]);
        }

        iFDS_Last = Entry.iInstrument;
    }

    return iFDS_Last;
}

// found in Wave_VRC7.cpp, don't know why it isn't part of NSFCore.
extern BYTE VRC7Instrument[16][8];

int GetVRC7(CNSFCore& NSFCore, bool bPatch)
{
    if (!bPatch)
    {
        return iVRC7_Last;
    }

    // search for existing patch
    for (unsigned int i=0; i < iVRC7; ++i)
    {
        VRC7_Entry& Entry = aVRC7[i];
        bool bMatch = true;
        for (unsigned int s=0; s<8; ++s)
        {
            if(VRC7Instrument[0][s] != Entry.nRegisters[s])
            {
                bMatch = false;
                break;
            }
        }

        if (bMatch)
        {
            iVRC7_Last = Entry.iInstrument;
            return iVRC7_Last;
        }
    }

    // check if there is room for a new entry
    if (iVRC7 >= MAX_VRC7)
    {
        iVRC7_Last = 0;
        return 0; // instrument 0 is 2A03, shows that we couldn't make the new patch
    }

    // build a new instrument
    {
        VRC7_Entry& Entry = aVRC7[iVRC7];
        ++iVRC7;

        Entry.iInstrument = pDocument->AddInstrument("vrc7 custom", SNDCHIP_VRC7);
        pMainFrame->AddInstrument(Entry.iInstrument);
        CInstrumentVRC7* pInst = (CInstrumentVRC7*)pDocument->GetInstrument(Entry.iInstrument);

        pInst->SetPatch(0);
        for (unsigned int s=0; s < 8; ++s)
        {
            Entry.nRegisters[s] = VRC7Instrument[0][s];
            pInst->SetCustomReg(s, (char)Entry.nRegisters[s]);
        }

        iVRC7_Last = Entry.iInstrument;
    }

    return iVRC7_Last;
}

// ============================================================================
// end of file
