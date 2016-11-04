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
 * TODO: move this to windows internal accelerator functions?
 *
 */

// List of modifier strings
LPCTSTR CAccelerator::MOD_NAMES[] = {
	_T("None"), 
	_T("Alt"), 
	_T("Ctrl"), 
	_T("Ctrl+Alt"), 
	_T("Shift"), 
	_T("Shift+Alt"), 
	_T("Shift+Ctrl"), 
	_T("Shift+Ctrl+Alt")
};

// Default shortcut table
const stAccelEntry CAccelerator::DEFAULT_TABLE[] = {
	{_T("Decrease octave"),				MOD_NONE,		VK_DIVIDE,		ID_CMD_OCTAVE_PREVIOUS},
	{_T("Increase octave"),				MOD_NONE,		VK_MULTIPLY,	ID_CMD_OCTAVE_NEXT},
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
	{_T("Next song"),					MOD_NONE,		0,				ID_NEXT_SONG},
	{_T("Previous song"),				MOD_NONE,		0,				ID_PREV_SONG},
	

};

const int CAccelerator::ACCEL_COUNT = sizeof(CAccelerator::DEFAULT_TABLE) / sizeof(stAccelEntry);

// Shortcut table
static stAccelEntry EntriesTable[CAccelerator::ACCEL_COUNT];

// Registry key
LPCTSTR CAccelerator::SHORTCUTS_SECTION = _T("Shortcuts");

CAccelerator::CAccelerator() : m_iModifier(0)
{
	ATLTRACE2(atlTraceGeneral, 0, "Accelerator: Accelerator table contains %d items\n", ACCEL_COUNT);
}

CAccelerator::~CAccelerator()
{
}
/*
int CAccelerator::GetItemCount() const
{
	return ACCEL_COUNT;
}
*/
LPCTSTR CAccelerator::GetItemName(int Item) const
{
	ASSERT(Item < ACCEL_COUNT);
	return EntriesTable[Item].name;
}

int CAccelerator::GetItemKey(int Item) const
{
	ASSERT(Item < ACCEL_COUNT);
	return EntriesTable[Item].key;
}

int CAccelerator::GetItemMod(int Item) const
{
	ASSERT(Item < ACCEL_COUNT);
	return EntriesTable[Item].mod;
}

int CAccelerator::GetDefaultKey(int Item) const
{
	ASSERT(Item < ACCEL_COUNT);
	return DEFAULT_TABLE[Item].key;
}

int CAccelerator::GetDefaultMod(int Item) const
{
	ASSERT(Item < ACCEL_COUNT);
	return DEFAULT_TABLE[Item].mod;
}

LPCTSTR CAccelerator::GetItemModName(int Item) const
{
	ASSERT(Item < ACCEL_COUNT);
	return MOD_NAMES[EntriesTable[Item].mod];
}

LPCTSTR CAccelerator::GetItemKeyName(int Item) const
{
	if (EntriesTable[Item].key > 0) {
		return GetVKeyName(EntriesTable[Item].key);
	}

	return _T("None");
}

LPCTSTR CAccelerator::GetVKeyName(int virtualKey) const
{
    unsigned int scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);

    // because MapVirtualKey strips the extended bit for some keys
    switch (virtualKey) {
        case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN: // arrow keys
        case VK_PRIOR: case VK_NEXT: // page up and page down
        case VK_END: case VK_HOME:
        case VK_INSERT: case VK_DELETE:
        case VK_DIVIDE:	 // numpad slash
        case VK_NUMLOCK:
            scanCode |= 0x100; // set extended bit
            break;
    }

    static char keyName[50];

    if (GetKeyNameText(scanCode << 16, keyName, sizeof(keyName)) != 0)
        return keyName;

	return "";
}


void CAccelerator::StoreShortcut(int Item, int Key, int Mod)
{
	ASSERT(Item < ACCEL_COUNT);
	EntriesTable[Item].key = Key;
	EntriesTable[Item].mod = Mod;
}

// Registry storage/loading

void CAccelerator::SaveShortcuts(CSettings *pSettings) const
{
	// Save values
	for (int i = 0; i < ACCEL_COUNT; ++i) {
		pSettings->StoreSetting(SHORTCUTS_SECTION, EntriesTable[i].name, (EntriesTable[i].mod << 8) | EntriesTable[i].key);
	}
}

void CAccelerator::LoadShortcuts(CSettings *pSettings)
{
	// Set up names and default values
	LoadDefaults();

	// Load custom values, if exists
	for (int i = 0; i < ACCEL_COUNT; ++i) {
		int Default = (EntriesTable[i].mod << 8) | EntriesTable[i].key;
		int Setting = pSettings->LoadSetting(SHORTCUTS_SECTION, EntriesTable[i].name, Default);
		EntriesTable[i].key = Setting & 0xFF;
		EntriesTable[i].mod = Setting >> 8;
	}
}

void CAccelerator::LoadDefaults()
{
	memcpy(EntriesTable, DEFAULT_TABLE, sizeof(stAccelEntry) * ACCEL_COUNT);
}

// Following handles key -> ID translation

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
	for (int i = 0; i < ACCEL_COUNT; ++i) {
		unsigned char Mod = EntriesTable[i].mod;
		unsigned char Key = EntriesTable[i].key;

		if ((Mod == m_iModifier) && nChar == Key) {
			return EntriesTable[i].id;
		}
	}

	// Find partial match
	for (int i = 0; i < ACCEL_COUNT; ++i) {
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

void CAccelerator::LostFocus()
{
	// Clear mod keys
	m_iModifier = 0;
}
