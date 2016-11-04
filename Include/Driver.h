/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2007  Jonathan Liss
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

// The NSF driver binary

// Change this for new versions
const char DRIVER_ID[] = "NSF-driver v2.0";

#pragma warning( disable : 4309 ) // disable warning 4309: 'initializing' : truncation of constant value

struct stNSFHeader {
	unsigned char	Ident[5];
	unsigned char	Version;
	unsigned char	TotalSongs;
	unsigned char	StartSong;
	unsigned short	LoadAddr;
	unsigned short	InitAddr;
	unsigned short	PlayAddr;
	unsigned char	SongName[32];
	unsigned char	ArtistName[32];
	unsigned char	Copyright[32];
	unsigned short	Speed_NTSC;
	unsigned char	BankValues[8];
	unsigned short	Speed_PAL;
	unsigned char	Flags;
	unsigned char	SoundChip;
	unsigned char	Reserved[4];
};

const unsigned short NSF_CALLER_SIZE = 128;	// bytes

// NES program for running a NSF
const char NSF_CALLER_BIN[] = {
	0xD8,0x78,0xA9,0x00,0x8D,0x00,0x20,0xAD,0x02,0x20,0x10,0xFB,0xAD,0x02,0x20,0x10,
	0xFB,0xA9,0x00,0xA2,0xFF,0x95,0x00,0x9D,0x00,0x01,0x9D,0x00,0x02,0x9D,0x00,0x03,
	0x9D,0x00,0x04,0x9D,0x00,0x05,0x9D,0x00,0x06,0x9D,0x00,0x07,0xCA,0xD0,0xE6,0xA9,
	0x00,0xA2,0x00,0x9D,0x00,0x40,0xE8,0xE0,0x0F,0xD0,0xF8,0xA9,0x10,0x8D,0x10,0x40,
	0xA9,0x00,0x8D,0x11,0x40,0x8D,0x12,0x40,0x8D,0x13,0x40,0xA9,0x0F,0x8D,0x15,0x40,
	0xA9,0xC0,0x8D,0x17,0x40,0xA9,0x00,0xA2,0x00,0x20,0x99,0x99,0xA9,0x80,0x8D,0x00,
	0x20,0x4C,0xE1,0xFF,0xAD,0x02,0x20,0xA9,0x00,0x8D,0x00,0x20,0xA9,0x80,0x8D,0x00,
	0x20,0x20,0x88,0x88,0x40,0x00,0x00,0x00,0x00,0x00,0xE4,0xFF,0x80,0xFF,0xF4,0xFF
};

//
// The driver binaries
//
// created with a binary->c-array program

const unsigned int DRIVER1_LOCATION = 0xB200;
const unsigned int DRIVER2_LOCATION = 0x8000;

#define DRIVER_SIZE (sizeof(DRIVER_MODE1))	// both have the same size

// These are not covered by the GNU GPL license

const char DRIVER_MODE1[] = {
	#include "drv_mode1.h"
};

const char DRIVER_MODE2[] = {
	#include "drv_mode2.h"
};
