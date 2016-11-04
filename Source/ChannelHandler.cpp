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

//
// This is the base class for all classes that takes care of 
// playing the channels.
//

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "SoundGen.h"
#include "ChannelHandler.h"

// Range for the pitch wheel command in notes
int PITCH_RANGE = 6;

CChannelHandler::CChannelHandler(int ChanID) : 
	m_iChannelID(ChanID), 
	m_bEnabled(false), 
	m_iInstrument(0), 
	m_iLastInstrument(MAX_INSTRUMENTS),
	m_pNoteLookupTable(NULL),
	m_pDocument(NULL)
{ 
	m_iPitch = 0; 
	m_iNote = 0; 
}

void CChannelHandler::InitChannel(CAPU *pAPU, int *pVibTable, CFamiTrackerDoc *pDoc)
{
	m_pAPU = pAPU;
	m_pVibratoTable = pVibTable;
	m_pDocument = pDoc;

	m_pDelayedNote = NULL;
	m_bDelayEnabled = false;

	m_iEffect = 0;

	KillChannel();

	m_iVibratoStyle = VIBRATO_NEW;
}

// todo: make this pure virtual
int CChannelHandler::LimitFreq(int Freq)
{
	if (Freq > 0x7FF)
		Freq = 0x7FF;

	if (Freq < 0)
		Freq = 0;

	return Freq;
}

void CChannelHandler::SetPitch(int Pitch)
{
	// Pitch ranges from -511 to +512
	m_iPitch = Pitch;
	if (m_iPitch == 512)
		m_iPitch = 511;
}

int CChannelHandler::GetPitch() const 
{ 
	if (m_iPitch != 0 && m_iNote != 0) {
		// Interpolate pitch
		int LowNote = m_iNote - PITCH_RANGE;
		int HighNote = m_iNote + PITCH_RANGE;

		if (LowNote < 0)
			LowNote = 0;
		if (HighNote > 95)
			HighNote = 95;

		int Freq = m_pNoteLookupTable[m_iNote];
		int Lower = m_pNoteLookupTable[LowNote];
		int Higher = m_pNoteLookupTable[HighNote];
		int Pitch;

		if (m_iPitch < 0)
			Pitch = (Freq - Lower);
		else
			Pitch = (Higher - Freq);

		return (Pitch * m_iPitch) / 511;
	}

	return 0;
}

void CChannelHandler::SetVibratoStyle(int Style)
{
	m_iVibratoStyle = Style;
}

void CChannelHandler::Arpeggiate(unsigned int Note)
{
	m_iFrequency = TriggerNote(Note);
}

// Todo: document this
void CChannelHandler::MakeSilent()
{
	m_iVolume			= MAX_VOL;
	m_iPortaSpeed		= 0;
	m_cArpeggio			= 0;
	m_cArpVar			= 0;
	m_iVibratoSpeed		= 0;
	m_iVibratoPhase		= (m_iVibratoStyle == VIBRATO_OLD) ? 48 : 0;
	m_iTremoloSpeed		= m_iTremoloPhase = 0;
	m_iFinePitch		= 0x80;
	m_iFrequency		= 0;
	m_iVolSlide			= 0;
//	m_iLastFrequency	= 0xFFFF;
	m_bDelayEnabled		= false;

	m_iDefaultDuty		= 0;

	m_iNoteCut			= 0;

	m_iVibratoDepth		= 0;
	m_iTremoloDepth		= 0;

	KillChannel();
}

// Todo: remove this and use note cut instead. Should not clear channel registers
void CChannelHandler::KillChannel()
{
	m_bEnabled			= false;
	m_iLastFrequency	= 0xFFFF;
	m_iOutVol			= 0x00;
	m_iPortaTo			= 0;

	for (int i = 0; i < SEQ_COUNT; ++i)
		m_iSeqEnabled[i] = 0;

	theApp.RegisterKeyState(m_iChannelID, -1);

	ClearRegisters();
}

// Resets the channel, restore volume, instrument & duty
void CChannelHandler::ResetChannel()
{
	m_iInstrument = 0;
	m_iLastInstrument = MAX_INSTRUMENTS;
	m_iVolume = MAX_VOL;
	m_iDefaultDuty = 0;

	for (int i = 0; i < SEQ_COUNT; ++i)
		m_iSeqEnabled[i] = 0;

	ClearRegisters();
}

// Handle common things before letting the channels play the notes
void CChannelHandler::PlayNote(stChanNote *NoteData, int EffColumns)
{
	ASSERT (NoteData != NULL);

	// Handle delay commands
	if (HandleDelay(NoteData, EffColumns))
		return;

	// Handle global effects
	theApp.GetSoundGenerator()->EvaluateGlobalEffects(NoteData, EffColumns);

	// Let the channel play
	PlayChannelNote(NoteData, EffColumns);
}

void CChannelHandler::SetNoteTable(unsigned int *pNoteLookupTable)
{
	// Installs the note lookup table
	m_pNoteLookupTable = pNoteLookupTable;
}

unsigned int CChannelHandler::TriggerNote(int Note)
{
	// Trigger a note, return note period
	theApp.RegisterKeyState(m_iChannelID, Note);

	if (!m_pNoteLookupTable)
		return Note;

	return m_pNoteLookupTable[Note];
}

void CChannelHandler::CutNote()
{
	// Cut currently playing note
//	MakeSilent();

	KillChannel();
}

void CChannelHandler::ReleaseNote()
{
	// Release currently playing note
	TriggerNote(-1);
}

int CChannelHandler::RunNote(int Octave, int Note)
{
	// Run the note and handle portamento
	int NewNote, NesFreq;

	// And note
	NewNote = MIDI_NOTE((Octave - 2), Note);
	NesFreq = TriggerNote(NewNote);

	if (m_iPortaSpeed > 0 && m_iEffect == EF_PORTAMENTO) {
		if (m_iFrequency == 0)
			m_iFrequency = NesFreq;
		m_iPortaTo = NesFreq;
	}
	else
		m_iFrequency = NesFreq;

	return NewNote;
}

void CChannelHandler::SetupSlide(int Type, int EffParam)
{
	#define GET_SLIDE_SPEED(x) (((x & 0xF0) >> 3) + 1)

	m_iPortaSpeed = GET_SLIDE_SPEED(EffParam);
	m_iEffect = Type;

	if (Type == EF_SLIDE_UP)
		m_iNote = m_iNote + (EffParam & 0xF);
	else
		m_iNote = m_iNote - (EffParam & 0xF);

	m_iPortaTo = TriggerNote(m_iNote);
}

bool CChannelHandler::CheckCommonEffects(unsigned char EffCmd, unsigned char EffParam)
{
	// Handle common effects for all channels

	switch (EffCmd) {
		case EF_PORTAMENTO:
			m_iPortaSpeed = EffParam;
			m_iEffect = EF_PORTAMENTO;
			if (!EffParam)
				m_iPortaTo = 0;
			break;
		case EF_VIBRATO:
			// m_iVibratoDepth = (EffParam & 0x0F) + 2;
			m_iVibratoDepth = (EffParam & 0x0F) << 4;
			m_iVibratoSpeed = EffParam >> 4;
			if (!EffParam)
				m_iVibratoPhase = (m_iVibratoStyle == VIBRATO_OLD) ? 48 : 0;
			break;
		case EF_TREMOLO:
			//m_iTremoloDepth = (EffParam & 0x0F) + 2;
			m_iTremoloDepth = (EffParam & 0x0F) << 4;
			m_iTremoloSpeed = EffParam >> 4;
			if (!EffParam)
				m_iTremoloPhase = 0;
			break;
		case EF_ARPEGGIO:
			m_cArpeggio = EffParam;
			m_iEffect = EF_ARPEGGIO;
			break;
		case EF_PITCH:
			m_iFinePitch = EffParam;
			break;
		case EF_PORTA_DOWN:
			m_iPortaSpeed = EffParam;
			m_iEffect = EF_PORTA_DOWN;
			break;
		case EF_PORTA_UP:
			m_iPortaSpeed = EffParam;
			m_iEffect = EF_PORTA_UP;
			break;
		case EF_VOLUME_SLIDE:
			m_iVolSlide = EffParam;
			break;
		case EF_NOTE_CUT:
			m_iNoteCut = EffParam + 1;
			break;
		default:
			return false;
	}
	
	return true;
}

bool CChannelHandler::HandleDelay(stChanNote *NoteData, int EffColumns)
{
	// Handle note delay, Gxx

	if (m_bDelayEnabled) {
		m_bDelayEnabled = false;
		PlayChannelNote(m_pDelayedNote, m_iDelayEffColumns);
		delete m_pDelayedNote;
	}
	
	// Check delay
	for (int n = 0; n < EffColumns; n++) {
		if (NoteData->EffNumber[n] == EF_DELAY) {
			m_bDelayEnabled = true;
			m_cDelayCounter = NoteData->EffParam[n];
			m_pDelayedNote = new stChanNote;
			m_iDelayEffColumns = EffColumns;
			memcpy(m_pDelayedNote, NoteData, sizeof(stChanNote));
			m_pDelayedNote->EffNumber[n] = EF_NONE;
			m_pDelayedNote->EffParam[n] = 0;
			return true;
		}
	}

	return false;
}

void CChannelHandler::ProcessChannel()
{
	// Run all default and common channel processing
	// This gets called each frame
	//

	// Note cut ()
	if (m_iNoteCut > 0) {
		m_iNoteCut--;
		if (m_iNoteCut == 0) {
			CutNote();
		}
	}

	// Delay (Gxx)
	if (m_bDelayEnabled) {
		if (!m_cDelayCounter) {
			m_bDelayEnabled = false;
			PlayNote(m_pDelayedNote, m_iDelayEffColumns);
			delete m_pDelayedNote;
		}
		else
			m_cDelayCounter--;
	}

	if (!m_bEnabled)
		return;

	// Volume slide (Axx)
	m_iVolume -= (m_iVolSlide & 0x0F);
	if (m_iVolume < 0)
		m_iVolume = 0;

	m_iVolume += (m_iVolSlide & 0xF0) >> 4;
	if (m_iVolume < 0)
		m_iVolume = MAX_VOL;

	// Vibrato and tremolo
#ifdef NEW_VIBRATO
	m_iVibratoPhase = (m_iVibratoPhase + m_iVibratoSpeed) & 63;
	m_iTremoloPhase = (m_iTremoloPhase + m_iTremoloSpeed) & 63;
#else
	m_iVibratoPhase = (m_iVibratoPhase + m_iVibratoSpeed) & (VIBRATO_LENGTH - 1);
	m_iTremoloPhase = (m_iTremoloPhase + m_iTremoloSpeed) & (TREMOLO_LENGTH - 1);
#endif

	// Handle other effects
	switch (m_iEffect) {
		case EF_ARPEGGIO:
			if (m_cArpeggio != 0 && m_iNote != 0) {
				switch (m_cArpVar) {
					case 0:
						m_iFrequency = TriggerNote(m_iNote);
						break;
					case 1:
						m_iFrequency = TriggerNote(m_iNote + (m_cArpeggio >> 4));
						if ((m_cArpeggio & 0x0F) == 0)
							m_cArpVar = 2;
						break;
					case 2:
						m_iFrequency = TriggerNote(m_iNote + (m_cArpeggio & 0x0F));
						break;
				}
				if (++m_cArpVar > 2)
					m_cArpVar = 0;
			}
			break;
		case EF_PORTAMENTO:
		case EF_SLIDE_UP:
		case EF_SLIDE_DOWN:
			// Automatic portamento
			if (m_iPortaSpeed > 0 && m_iPortaTo > 0) {
				if (m_iFrequency > m_iPortaTo) {
					m_iFrequency -= m_iPortaSpeed;
					// todo: check this
//					if (m_iFrequency > 0x1000)	// it was negative
//						m_iFrequency = 0x00;
					if (m_iFrequency < m_iPortaTo)
						m_iFrequency = m_iPortaTo;
				}
				else if (m_iFrequency < m_iPortaTo) {
					m_iFrequency += m_iPortaSpeed;
					if (m_iFrequency > m_iPortaTo)
						m_iFrequency = m_iPortaTo;
				}
			}
			break;
		case EF_PORTA_DOWN:
			m_iFrequency += m_iPortaSpeed;
			m_iFrequency = LimitFreq(m_iFrequency);
			break;
		case EF_PORTA_UP:
			m_iFrequency -= m_iPortaSpeed;
			m_iFrequency = LimitFreq(m_iFrequency);
			break;
	}
}

// Used to see that everything is ok right before playing a note
bool CChannelHandler::CheckNote(stChanNote *pNoteData, int InstrumentType)
{
	CInstrument	*pInstrument;

	// No note data
	if (!pNoteData)
		return false;

	int Instrument = pNoteData->Instrument;

//	if ((m_iInstrument = pNoteData->Instrument) == MAX_INSTRUMENTS)
//		m_iInstrument = m_iLastInstrument;

	// Halt and release
	if (pNoteData->Note == HALT || pNoteData->Note == RELEASE) {
//		m_iVolume = 0x10;
//		KillChannel();
		// Allow incorrect instruments for note off
		return true;
	}

	// Save instrument index

	if (Instrument != MAX_INSTRUMENTS)
		m_iInstrument = pNoteData->Instrument;

	pInstrument = m_pDocument->GetInstrument(m_iInstrument);

	// No instrument
	if (!pInstrument)
		return false;

	// Wrong type of instrument
	if (pInstrument->GetType() != InstrumentType)
		return false;

	return true;
}

int CChannelHandler::GetVibrato() const
{
	// Vibrato offset (4xx)
	int VibFreq;

#ifdef NEW_VIBRATO
	if ((m_iVibratoPhase & 0xF0) == 0x00)
		VibFreq = m_pVibratoTable[m_iVibratoPhase + m_iVibratoDepth];
	else if ((m_iVibratoPhase & 0xF0) == 0x10)
		VibFreq = m_pVibratoTable[15 - (m_iVibratoPhase - 16) + m_iVibratoDepth];
	else if ((m_iVibratoPhase & 0xF0) == 0x20)
		VibFreq = -m_pVibratoTable[(m_iVibratoPhase - 32) + m_iVibratoDepth];
	else if ((m_iVibratoPhase & 0xF0) == 0x30)
		VibFreq = -m_pVibratoTable[15 - (m_iVibratoPhase - 48) + m_iVibratoDepth];
#else
	VibFreq	= m_pVibratoTable[m_iVibratoPhase] >> (0x8 - (m_iVibratoDepth >> 1));
	if ((m_iVibratoDepth & 1) == 0)
		VibFreq -= (VibFreq >> 1);
#endif
	
	if (m_pDocument->GetVibratoStyle() == VIBRATO_OLD) {
		VibFreq += m_pVibratoTable[m_iVibratoDepth + 15] + 1;
		VibFreq >>= 1;
	}

	return VibFreq;
}

int CChannelHandler::GetTremolo() const
{
	// Tremolo offset (7xx)
	int TremVol;
	int Phase = m_iTremoloPhase >> 1;

#ifdef NEW_VIBRATO
	if ((Phase & 0xF0) == 0x00)
		TremVol = m_pVibratoTable[Phase + m_iTremoloDepth];
	else if ((Phase & 0xF0) == 0x10)
		TremVol = m_pVibratoTable[15 - (Phase - 16) + m_iTremoloDepth];
#else
	TremVol	= (m_pVibratoTable[m_iTremoloPhase] >> 4) >> (4 - (m_iTremoloDepth >> 1));

	if ((m_iTremoloDepth & 1) == 0)
		TremVol -= (TremVol >> 1);
#endif

	return TremVol;
}

int CChannelHandler::GetFinePitch() const
{
	// Fine pitch setting (Pxx)
	return (0x80 - m_iFinePitch);
}

// Sequence routines

void CChannelHandler::RunSequence(int Index, CSequence *pSequence)
{
	int Value;

	if (m_iSeqEnabled[Index] == 1 && pSequence->GetItemCount() > 0) {

		Value = pSequence->GetItem(m_iSeqPointer[Index]);

		switch (Index) {
			// Volume modifier
			case SEQ_VOLUME:
				m_iOutVol = Value;
				break;
			// Arpeggiator
			case SEQ_ARPEGGIO:
				if (pSequence->GetSetting() == 0)
					m_iFrequency = TriggerNote(m_iNote + Value);
				else
					m_iFrequency = TriggerNote(Value);
				break;
			// Pitch
			case SEQ_PITCH:
				m_iFrequency += Value;
				m_iFrequency = LimitFreq(m_iFrequency);
				break;
			// Hi-pitch
			case SEQ_HIPITCH:
				m_iFrequency += Value << 4;
				m_iFrequency = LimitFreq(m_iFrequency);
				break;
			// Duty cycling
			case SEQ_DUTYCYCLE:
				m_iDutyPeriod = Value;
				break;
		}

		m_iSeqPointer[Index]++;

		int Release = pSequence->GetReleasePoint();
		int Items = pSequence->GetItemCount();
		int Loop = pSequence->GetLoopPoint();

		/*
		if (m_bRelease) {
			if (m_iSeqPointer[Index] == Items) {
				// End of sequence 
				m_iSeqEnabled[Index] = 0;
			}
		}
		else {
		*/
			if (m_iSeqPointer[Index] == (Release + 1) || m_iSeqPointer[Index] == Items) {
				// End point reached
				if (Loop != -1 && !(m_bRelease && Release != -1)) {
					m_iSeqPointer[Index] = Loop;
				}
				else {
					if (m_iSeqPointer[Index] == Items) {
						// End of sequence 
						m_iSeqEnabled[Index] = 0;
					}
					else if (!m_bRelease)
						// Waiting for release
						m_iSeqPointer[Index]--;
				}
			}
//		}

		pSequence->SetPlayPos(m_iSeqPointer[Index]);

//		if (Index == MOD_ARPEGGIO)
//			m_bArpEffDone = false;
	}
	else if (m_iSeqEnabled[Index] == 0) {
		/*
		if (Index == MOD_ARPEGGIO && pSequence->GetSetting() == 1) {
			// Absolute arpeggio notes
			m_bArpEffDone = false;
			if (m_bArpEffDone == false) {
				m_iFrequency = TriggerNote(m_iNote);
				m_bArpEffDone = true;
			}
		}
		*/
		pSequence->SetPlayPos(-1);
	}
}

CSequence *CChannelHandler::GetSequence(int Index, int Type)
{
	// Return a sequence, must be overloaded
	return NULL;
}
