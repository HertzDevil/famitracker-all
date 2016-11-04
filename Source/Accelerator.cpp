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
#include "accelerator.h"
#include "Settings.h"

/*
 * This class is used to translate key shortcuts -> command messages
 *
 * Todo: move this to windows accelerator functions?
 *
 */

static const TCHAR *KEY_NAMES[] = {
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

LPCTSTR CAccelerator::MOD_NAMES[] = {
	_T("None"), 
	_T("Alt"), 
	_T("Ctrl"), 
	_T("Alt+Ctrl"), 
	_T("Shift"), 
	_T("Alt+Shift"), 
	_T("Ctrl+Shift"), 
	_T("Alt+Ctrl+Shift")
};

struct stAccelEntry {
	char *name;
	int	 mod;
	int	 key;
	int	 id;
	int	 focus;
};

// Default shortcut table
const stAccelEntry DEFAULT_TABLE[] = {
	{_T("Increase octave"),				MOD_NONE,		VK_DIVIDE,		ID_CMD_OCTAVE_PREVIOUS},
	{_T("Decrease octave"),				MOD_NONE,		VK_MULTIPLY,	ID_CMD_OCTAVE_NEXT},
	{_T("Play / Stop"),					MOD_NONE,		VK_RETURN,		ID_TRACKER_TOGGLE_PLAY},
	{_T("Play"),						MOD_NONE,		0,				ID_TRACKER_PLAY},
	{_T("Play from start"),				MOD_NONE,		VK_F5,			ID_TRACKER_PLAY_START},
	{_T("Play from cursor"),			MOD_NONE,		VK_F7,			ID_TRACKER_PLAY_CURSOR},
	{_T("Play and loop pattern"),		MOD_NONE,		VK_F6,			ID_TRACKER_PLAYPATTERN},
	{_T("Play row"),					MOD_CONTROL,	VK_RETURN,		ID_TRACKER_PLAYROW},
	{_T("Stop"),						MOD_NONE,		VK_F8,			ID_TRACKER_STOP},
	{_T("Edit enable/disable"),			MOD_NONE,		VK_SPACE,		ID_TRACKER_EDIT},
	{_T("Paste and overwrite"),			MOD_CONTROL,	'B',			ID_CMD_PASTEOVERWRITE},
	{_T("Paste and mix"),				MOD_CONTROL,	'M',			ID_CMD_PASTEMIXED},
	{_T("Select all"),					MOD_CONTROL,	'A',			ID_EDIT_SELECTALL},
	{_T("Toggle channel"),				MOD_ALT,		VK_F9,			ID_TRACKER_TOGGLECHANNEL},
	{_T("Solo channel"),				MOD_ALT,		VK_F10,			ID_TRACKER_SOLOCHANNEL},
	{_T("Interpolate"),					MOD_CONTROL,	'G',			ID_EDIT_INTERPOLATE},
	{_T("Go to next frame"),			MOD_CONTROL,	VK_RIGHT,		ID_NEXT_FRAME},
	{_T("Go to previous frame"),		MOD_CONTROL,	VK_LEFT,		ID_PREV_FRAME},
	{_T("Transpose, decrease notes"),	MOD_CONTROL,	VK_F1,			ID_TRANSPOSE_DECREASENOTE},
	{_T("Transpose, increase notes"),	MOD_CONTROL,	VK_F2,			ID_TRANSPOSE_INCREASENOTE},
	{_T("Transpose, decrease octaves"),	MOD_CONTROL,	VK_F3,			ID_TRANSPOSE_DECREASEOCTAVE},
	{_T("Transpose, increase octaves"),	MOD_CONTROL,	VK_F4,			ID_TRANSPOSE_INCREASEOCTAVE},
	{_T("Increase pattern"),			MOD_NONE,		VK_ADD,			IDC_FRAME_INC},
	{_T("Decrease pattern"),			MOD_NONE,		VK_SUBTRACT,	IDC_FRAME_DEC},
	{_T("Next instrument"),				MOD_CONTROL,	VK_DOWN,		ID_CMD_NEXT_INSTRUMENT},
	{_T("Previous instrument"),			MOD_CONTROL,	VK_UP,			ID_CMD_PREV_INSTRUMENT},
	{_T("Mask instruments"),			MOD_ALT,		'T',			ID_EDIT_INSTRUMENTMASK},
	{_T("Edit instrument"),				MOD_CONTROL,	'I',			ID_MODULE_EDITINSTRUMENT},
	{_T("Increase step size"),			MOD_CONTROL,	VK_ADD,			ID_CMD_INCREASESTEPSIZE},
	{_T("Decrease step size"),			MOD_CONTROL,	VK_SUBTRACT,	ID_CMD_DECREASESTEPSIZE},
	{_T("Follow mode"),					MOD_NONE,		VK_SCROLL,		IDC_FOLLOW_TOGGLE},
	{_T("Duplicate frame"),				MOD_CONTROL,	'D',			ID_MODULE_DUPLICATEFRAME},
	{_T("Insert frame"),				MOD_NONE,		0,				ID_FRAME_INSERT},
	{_T("Remove frame"),				MOD_NONE,		0,				ID_FRAME_REMOVE},
	{_T("Reverse"),						MOD_CONTROL,	'R',			ID_EDIT_REVERSE},
	{_T("Select frame editor"),			MOD_NONE,		VK_F3,			ID_FOCUS_FRAME_EDITOR},
	{_T("Select pattern editor"),		MOD_NONE,		VK_F2,			ID_FOCUS_PATTERN_EDITOR},
	{_T("Move one step up"),			MOD_ALT,		VK_UP,			ID_CMD_STEP_UP},
	{_T("Move one step down"),			MOD_ALT,		VK_DOWN,		ID_CMD_STEP_DOWN},
	{_T("Replace instrument"),			MOD_ALT,		'S',			ID_EDIT_REPLACEINSTRUMENT},
	{_T("Toggle control panel"),		MOD_NONE,		0,				ID_VIEW_CONTROLPANEL},
	{_T("Display effect list"),			MOD_NONE,		0,				ID_HELP_EFFECTTABLE},
	{_T("Select block start"),			MOD_ALT,		'B',			ID_BLOCK_START},
	{_T("Select block end"),			MOD_ALT,		'E',			ID_BLOCK_END},
	{_T("Pick up row settings"),		MOD_NONE,		0,				ID_POPUP_PICKUPROW},

};

const int ACCEL_COUNT = sizeof(DEFAULT_TABLE) / sizeof(stAccelEntry);

stAccelEntry EntriesTable[ACCEL_COUNT];


CAccelerator::CAccelerator() : m_iModifier(0)
{
	ATLTRACE2(atlTraceGeneral, 0, "Accelerator: Accelerator table contains %d items\n", ACCEL_COUNT);
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

	// Find exact match
	for (int i = 0; i < ACCEL_COUNT; i++) {
		unsigned char Mod = EntriesTable[i].mod;
		unsigned char Key = EntriesTable[i].key;

		if ((Mod == m_iModifier) && nChar == Key) {
			return EntriesTable[i].id;
		}
	}

	// Find partial match
	for (int i = 0; i < ACCEL_COUNT; i++) {
		unsigned char Mod = EntriesTable[i].mod;
		unsigned char Key = EntriesTable[i].key;

		if ((Mod & m_iModifier) == Mod && nChar == Key) {
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

LPCTSTR CAccelerator::GetModName(int Item) const
{
	return MOD_NAMES[EntriesTable[Item].mod];
}

LPCTSTR CAccelerator::GetKeyName(int Item) const
{
	if (EntriesTable[Item].key > 0) {
		return KEY_NAMES[EntriesTable[Item].key];
	}

	return _T("None");
}

LPCTSTR CAccelerator::EnumKeyNames(int Index) const
{
	return KEY_NAMES[Index];
}

int CAccelerator::GetItem(CString Name) const
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

	if (Key.Compare(_T("None")) == 0) {
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

void CAccelerator::SaveShortcuts(CSettings *pSettings) const
{
	// Save values
	for (int i = 0; i < ACCEL_COUNT; ++i) {
		pSettings->StoreSetting(_T("Shortcuts"), EntriesTable[i].name, (EntriesTable[i].mod << 8) | EntriesTable[i].key);
	}
}

void CAccelerator::LoadShortcuts(CSettings *pSettings)
{
	// Set up names and default values
	LoadDefaults();

	// Load custom values, if exists
	for (int i = 0; i < ACCEL_COUNT; ++i) {
		int Setting = pSettings->LoadSetting(_T("Shortcuts"), EntriesTable[i].name);
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