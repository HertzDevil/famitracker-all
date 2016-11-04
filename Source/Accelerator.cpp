/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2009  Jonathan Liss
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
#include "accelerator.h"

#define MOD_NONE 0

static const char *KEY_NAMES[] = {
	"",			 "",		"",			"",			"",			"",			"",			"",			// 00-07
	"Backspace", "Tab",		"",			"",			"clear",	"Enter",	"",			"",			// 08-0F
	"",			 "",		"",			"Pause",	"",			"",			"",			"",			// 10-17
	"",			 "",		"Escape",	"",			"",			"",			"",			"",			// 18-1F
	"Space",	 "PgUp",	"PgDn",		"End",		"Home",		"Left",		"Up",		"Right",	// 20-27
	"Down",		 "Select",	"Print",	"",			"PrtScn",	"Insert",	"Del",		"Help",		// 28-2F
	"0",		 "1",		"2",		"3",		"4",		"5",		"6",		"7",		// 30-37
	"8",		 "9",		"",			"",			"",			"",			"",			"",			// 38-3F
	"",			 "A",		"B",		"C",		"D",		"E",		"F",		"G",		// 40-47
	"H",		 "I",		"J",		"K",		"L",		"M",		"N",		"O",		// 48-4F
	"P",		 "Q",		"R",		"S",		"T",		"U",		"V",		"W",		// 50-57
	"X",		 "Y",		"Z",		"",			"",			"",			"",			"",			// 58-5F
	"Num 0",	 "Num 1",	"Num 2",	"Num 3",	"Num 4",	"Num 5",	"Num 6",	"Num 7",	// 60-67
	"Num 8",	 "Num 9",	"Num *",	"Num +",	"",			"Num -",	"Num ,",	"Num /",	// 68-6F
	"F1",		 "F2",		"F3",		"F4",		"F5",		"F6",		"F7",		"F8",		// 70-77
	"F9",		 "F10",		"F11",		"F12",		"F13",		"F14",		"F15",		"F16",		// 78-7F
	"F17",		 "F18",		"F19",		"F20",		"F21",		"F22",		"F23",		"F24",		// 80-87
	"",			 "",		"",			"",			"",			"",			"",			"",			// 88-8F
	"Num",		 "Scroll",	"",			"",			"",			"",			"",			"",			// 90-97
	"",			 "",		"",			"",			"",			"",			"",			"",			// 98-9F
	"",			 "",		"",			"",			"",			"",			"",			"",			// A0-A7
	"",			 "",		"",			"",			"",			"",			"",			"",			// A8-AF
	"",			 "",		"",			"",			"",			"",			"",			"",			// B0-B7
	"",			 "",		";: (US)",	"+",		",",		"-",		".",		"/ (US)",	// B8-BF
	"~ (US)",	 "",		"",			"",			"",			"",			"",			"",			// C0-C7
	"",			 "",		"",			"",			"",			"",			"",			"",			// C8-CF
	"",			 "",		"",			"",			"",			"",			"",			"",			// D0-D7
	"",			 "",		"",			"[{ (US)",	"\\| (US)",	"]} (US)",	"\" (US)",	"OEM 8",	// D8-DF
	"",			 "OEM 102",	"",			"",			"",			"",			"",			"",			// E0-E7
	"",			 "",		"",			"",			"",			"",			"",			"",			// E8-EF
	"",			 "",		"",			"",			"",			"",			"",			"",			// F0-F7
	"",			 "",		"",			"",			"",			"",			"",			""			// F8-FF
};

struct stAccelEntry {
	char *name;
	int	 mod;
	int	 key;
	int	 id;
};

const stAccelEntry DEFAULT_TABLE[] = {
	{"Increase octave",				MOD_NONE,		VK_DIVIDE,		ID_CMD_OCTAVE_PREVIOUS},
	{"Decrease octave",				MOD_NONE,		VK_MULTIPLY,	ID_CMD_OCTAVE_NEXT},
	{"Play / Stop",					MOD_NONE,		VK_RETURN,		ID_TRACKER_TOGGLE_PLAY},
	{"Play Pattern",				MOD_SHIFT,		VK_RETURN,		ID_TRACKER_PLAYPATTERN},
	{"Play Row",					MOD_CONTROL,	VK_RETURN,		ID_TRACKER_PLAYROW},
	{"Edit enable/disable",			MOD_NONE,		VK_SPACE,		ID_TRACKER_EDIT},
	{"Paste and overwrite",			MOD_CONTROL,	'B',			ID_CMD_PASTEOVERWRITE},
	{"Paste and mix",				MOD_CONTROL,	'M',			ID_CMD_PASTEMIXED},
	{"Select all",					MOD_CONTROL,	'A',			ID_EDIT_SELECTALL},
	{"Toggle channel",				MOD_ALT,		VK_F9,			ID_TRACKER_TOGGLECHANNEL},
	{"Solo channel",				MOD_ALT,		VK_F10,			ID_TRACKER_SOLOCHANNEL},
	{"Interpolate",					MOD_CONTROL,	'G',			ID_EDIT_INTERPOLATE},
	{"Go to next frame",			MOD_CONTROL,	VK_RIGHT,		ID_NEXT_FRAME},
	{"Go to previous frame",		MOD_CONTROL,	VK_LEFT,		ID_PREV_FRAME},
	{"Transpose, decrease notes",	MOD_CONTROL,	VK_F1,			ID_TRANSPOSE_DECREASENOTE},
	{"Transpose, increase notes",	MOD_CONTROL,	VK_F2,			ID_TRANSPOSE_INCREASENOTE},
	{"Transpose, decrease octaves",	MOD_CONTROL,	VK_F3,			ID_TRANSPOSE_DECREASEOCTAVE},
	{"Transpose, increase octaves",	MOD_CONTROL,	VK_F4,			ID_TRANSPOSE_INCREASEOCTAVE},
	{"Increase pattern",			MOD_NONE,		VK_ADD,			IDC_FRAME_INC},
	{"Decrease pattern",			MOD_NONE,		VK_SUBTRACT,	IDC_FRAME_DEC},
	{"Next instrument",				MOD_CONTROL,	VK_DOWN,		ID_CMD_NEXT_INSTRUMENT},
	{"Previous instrument",			MOD_CONTROL,	VK_UP,			ID_CMD_PREV_INSTRUMENT},
	{"Mask instruments",			MOD_ALT,		'T',			ID_EDIT_INSTRUMENTMASK},
	{"Edit instrument",				MOD_CONTROL,	'I',			ID_MODULE_EDITINSTRUMENT},
	{"Increase step size",			MOD_CONTROL,	VK_ADD,			ID_CMD_INCREASESTEPSIZE},
	{"Decrease step size",			MOD_CONTROL,	VK_SUBTRACT,	ID_CMD_DECREASESTEPSIZE},
	{"Follow mode",					MOD_NONE,		VK_SCROLL,		IDC_FOLLOW_TOGGLE},
	{"Duplicate frame",				MOD_CONTROL,	'D',			ID_MODULE_DUPLICATEFRAME},
	{"Insert frame",				MOD_NONE,		0,				ID_FRAME_INSERT},
	{"Remove frame",				MOD_NONE,		0,				ID_FRAME_REMOVE},
	{"Reverse",						MOD_CONTROL,	'R',			ID_EDIT_REVERSE},
};

const int ACCEL_COUNT = sizeof(DEFAULT_TABLE) / sizeof(stAccelEntry);

stAccelEntry EntriesTable[ACCEL_COUNT];

//char *GetKeyName(int Key)

CAccelerator::CAccelerator()
{
	m_iModifier = 0;
}

CAccelerator::~CAccelerator()
{
}

int CAccelerator::GetItemCount() const
{
	return ACCEL_COUNT;
}

char *CAccelerator::GetItemName(int Item) const
{
	ASSERT(Item < ACCEL_COUNT);
	return EntriesTable[Item].name;
}

unsigned short CAccelerator::GetAction(unsigned char nChar)
{
	// Translates a key-stroke into an ID

	switch (nChar) {
		case VK_SHIFT:
			m_iModifier |= MOD_SHIFT;
			return 0;
		case VK_CONTROL:
			m_iModifier |= MOD_CONTROL;
			return 0;
		case VK_MENU:
			m_iModifier |= MOD_ALT;
			return 0;
	}

	for (int i = 0; i < ACCEL_COUNT; i++) {
		unsigned char Mod = EntriesTable[i].mod;
		unsigned char Key = EntriesTable[i].key;

		if ((Mod == m_iModifier) && nChar == Key) {
			return EntriesTable[i].id;
		}
	}

	return 0;
}

void CAccelerator::KeyReleased(unsigned char nChar)
{
	switch (nChar) {
		case VK_SHIFT:
			m_iModifier &= ~MOD_SHIFT;
			break;
		case VK_CONTROL:
			m_iModifier &= ~MOD_CONTROL;
			break;
		case VK_MENU:
			m_iModifier &= ~MOD_ALT;
			break;
	}
}

char *CAccelerator::GetModName(int Item)
{
	if (EntriesTable[Item].mod & MOD_ALT)
		return "Alt";
	else if (EntriesTable[Item].mod & MOD_CONTROL)
		return "Ctrl";
	else if (EntriesTable[Item].mod & MOD_SHIFT)
		return "Shift";

	return "None";
}

char *CAccelerator::GetKeyName(int Item)
{
	if (EntriesTable[Item].key > 0) {
		return (char*)KEY_NAMES[EntriesTable[Item].key];
	}

	return "None";
}

char *CAccelerator::EnumKeyNames(int Index)
{
	return (char*)KEY_NAMES[Index];
}

int CAccelerator::GetItem(CString Name)
{
	for (int i = 0; i < ACCEL_COUNT; i++) {
		if (Name.Compare(EntriesTable[i].name) == 0)
			return i;
	}

	return -1;
}

void CAccelerator::SelectMod(int Item, int Mod)
{
	EntriesTable[Item].mod = Mod;
}

void CAccelerator::SelectKey(int Item, CString Key)
{
	int i;

	if (Key.Compare("None") == 0) {
		EntriesTable[Item].key = 0;
		return;
	}

	for (i = 0; i < 0xFF; i++) {
		if (Key.Compare(KEY_NAMES[i]) == 0)
			break;
	}

	if (i == 0xFF)
		return;

	EntriesTable[Item].key = i;
}

void CAccelerator::SaveShortcuts(CSettings *pSettings)
{
	// Save values
	for (int i = 0; i < ACCEL_COUNT; i++) {
		pSettings->StoreSetting("Shortcuts", EntriesTable[i].name, (EntriesTable[i].mod << 8) | EntriesTable[i].key);
	}
}

void CAccelerator::LoadShortcuts(CSettings *pSettings)
{
	// Set up names and default values
	LoadDefaults();

	// Load custom values, if exists
	for (int i = 0; i < ACCEL_COUNT; i++) {
		int Setting = pSettings->LoadSetting("Shortcuts", EntriesTable[i].name);
		if (Setting > 0) {
			EntriesTable[i].key = Setting & 0xFF;
			EntriesTable[i].mod = Setting >> 8;
		}
	}	
}

void CAccelerator::LoadDefaults()
{
	memcpy(EntriesTable, DEFAULT_TABLE, sizeof(stAccelEntry) * ACCEL_COUNT);
}

void CAccelerator::LostFocus()
{
	// Clear mod keys
	m_iModifier = 0;
}