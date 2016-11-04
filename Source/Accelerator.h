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

#define MOD_NONE 0

class CAccelerator {
public:
	CAccelerator();
	~CAccelerator();

	int				GetItemCount() const;
	char			*GetItemName(int Item) const;
	unsigned short	GetAction(unsigned char nChar);
	void			KeyReleased(unsigned char nChar);
	LPCTSTR			GetModName(int Item) const;
	LPCTSTR			GetKeyName(int Item) const;
	LPCTSTR			EnumKeyNames(int Index) const;
	int				GetItem(CString Name) const;
	void			SelectMod(int Item, int Mod);
	void			SelectKey(int Item, CString Key);
	void			SaveShortcuts(CSettings *pSettings) const;
	void			LoadShortcuts(CSettings *pSettings);
	void			LoadDefaults();
	void			LostFocus();

public:
	static LPCTSTR  MOD_NAMES[];

private:
	unsigned int	m_iModifier;
};
