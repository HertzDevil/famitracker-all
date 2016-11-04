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


// Default font
const static char *FONT_FACE = "Fixedsys";	// fixedsys is good because it is fast

// Default colors
const struct {
	const static int BACKGROUND			= 0x00000000;		// Background color
	const static int BACKGROUND_HILITE	= 0x00001010;		// Background color
	const static int BACKGROUND_HILITE2	= 0x00002020;		// Background color
	const static int TEXT_NORMAL		= 0x0000FF00;		// Normal text color
	const static int TEXT_HILITE		= 0x0000F0F0;		// Highlighted text color
	const static int TEXT_HILITE2		= 0x0060FFFF;		// Highlighted text color
	const static int TEXT_MUTED			= 0x000000FF;		// Channel muted color
	const static int TEXT_INSTRUMENT	= 0x0080FF80;
	const static int TEXT_VOLUME		= 0x00FF8080;
	const static int TEXT_EFFECT		= 0x008080FF;
	const static int BORDER_NORMAL		= 0x006671F4;		// Normal border, inactive
	const static int BORDER_EDIT		= 0x00000080;		// Edit border, inactive
	const static int BORDER_NORMAL_A	= 0x00C02020;		// Normal border, active
	const static int BORDER_EDIT_A		= 0x000000FF;		// Edit border, active
	const static int SELECTION			= 0x00FF0060;
	const static int CURSOR				= 0x00808080;
} COLOR_SCHEME;
