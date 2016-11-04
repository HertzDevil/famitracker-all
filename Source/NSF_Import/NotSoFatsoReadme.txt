/* 
 * Copyright (C) 2004      Disch
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
 

--------------------
NotSo Fatso Source!
--------------------


Notice:
===================================

	I only scanned and changed sections of this file for this version (version 0.7).
However, the internal NSF core underwent a complete rewrite from previous versions, and
I may not have corrected everything that needs it.  So if you find something misleading
or inaccurate in this readme, my apologies ^^.

    Notice:  the only lines I changed for version 0.80 in this file are these two.
Guess I'm getting lazy... sorry.



Quick info:
===================================

	I tried to remember to put comments throughout the source.  I probably went a little
overkill on the 6502 core (NSF_6502.cpp); comments are plentiful there, but are lacking
in other areas.  Hopefully things won't be too difficult to understand.

	I made this readme to sort of be a guide as to how the source works.  A lot of source
available is just source and nothing else... making it hard to decipher.  Hopefully
this will ease the process a bit.




Organization:
====================================

	If you're running this from the VS.NET solution file, the files are already somewhat
seperated into categories in the solution explorer.  Here's a brief quicky of what each
file's purpose is:

* Header Files / Source files
**	NSF.h/cpp - Home of the CNSF class.  CNSF basically -is- the plugin.  It interacts with
		Winamp, parents the GUI, saves/loads config options, etc.  One thing to note
		is that CNSF -doesn't- generate the samples created by the nsf...as that's done
		by the NSF core (NSF_Core.h/cpp).  See that file's description for more details.

**	Winamp.h/cpp - declarations for the 2 Winamp module structures (In_Module and Out_Module)
		Pretty much taken directly from the in_minisdk from http://classic.winamp.com
		Everything in Winamp.h was created by Nullsoft
				
**	NotsoFatso.def - the defintion file for the plugin (if you're using Visual Studio, it's
		used to export symbols... in this case the function winampGetInModule2)
		
**	GUI (****Dlg.h) - Files for the GUI control dialogs.  Each dialog has it's own h/cpp
		file (with it's own class derived from CDDialog).  See the DFC section for more
		details on how CDDialog and family work.  There are 2 parent dialogs, each with
		several children.  Here's the basic blueprint:
		
		MainControlDlg	-> PlayControlDlg		("Play Control" tab)
						-> ConfigDlg			("Config", "Config 2" tabs)
						-> ChannelsDlg			("Channels" tab)
						-> VRC6Dlg				("VRC6" tab... very similar to ChannelsDlg)
						-> MMC5Dlg				("MMC5" tab...  "		"	"		"	  )
						-> N106Dlg				("N106" tab...  "		"	"		"	  )
						-> VRC7Dlg				("VRC7" tab...  "		"	"		"	  )
						-> FME07Dlg				("FME-07" tab...  "		"	"		"	  )
						-> AboutDlg				("About" tab)
		
		FileInfoDlg		-> GeneralFileInfoDlg	("File Info" tab)
						-> TrackInfoDlg			("Track Info" tab)
						-> PlaylistDlg			("Playlist Info" tab)


* Resource Files
**	Resource.h - resource header

**	Resource.rc - Resource script.  Contains all the dialogs used by the app

**	NotSoLogo.bmp - Logo image courtesy of my old buddy Tzar.  Can't thank him enough.

**  wannabewinamp.ico - Icon that's a blatant ripoff of Winamp's =P.  Gets displayed
		as the window's icon for NotSo's dialogs.


* DFC Files
**	DFC.h, DFC_*.h/cpp - Various files managing DFC.  See the DFC section
		for more details.
		
* NSF Core
**	NSF_File.h/cpp - Home of the CNSFFile class.  Used for reading (and saving) NSF/NSFE files.
		See the CNSFEFile for more details.
		
**	NSF_Core.h/cpp - The NSF core used by NotSo Fatso.  It's seperated to allow seperation and
		dropping into another app (I had thoughts to use it a game).  With it I suppose someone
		could drop it into a game they're making... or an NSF player... or maybe a plugin
		for a web-browser or something.  Who knows.  The core is what generates ALL the sound.
		All sound options are passed to it.. and it feeds back PCM samples for output.
		
**	NSF_6502.cpp - The 6502 Core (part of the NSF Core).
		
**	Wave_Square.h / Wave_TND.h - The sound channel objects.  They manage each waveform's generated
		sound (Wave_Square covers the 2 Squares, Wave_TND handles the Tri/Noise/DMC).  Register
		writes are managed by CNSFCore, NOT by these classes (see the WriteMemory_pAPU function
		in NSF_Core.cpp)
		
**	Wave_VRC6.h - Same deal as the above Wave header files.  These are the custom VRC6 chip instruments
		(custom square/pulse wave, sawtooth wave).  Again, register writes are managed by
		CNSFCore, not these classes (see WriteMemory_VRC6 in NSFCore.cpp).
		
**	Wave_N106.h - Namcot-106 chip.

**	Wave_MMC5.h - Nintendo MMC5 chip.

**	Wave_FME07.h - Sunsoft FME-07 chip.

**  Wave_VRC7.cpp - Converts VRC7 register writes to OPL2 register writes and mixes VRC7 output.
		Big thanks to Xodnizel and Quietust for this ^^.

**  fmopl.h/c - OPL2 emulator made by Tatsuyuki Satoh, which is used for VRC7 chip emulation.
		Tremendous thanks to Tatsuyuki Satoh ^^.
		
		
		
How the Core Works:
====================================

	The CNSFCore class (NSF_Core.h/cpp) is what produces all of the sound in NotSo Fatso.  CNSF (NSF.h/cpp)
just interacts with the core, GUI, and Winamp... it doesn't produce, or even alter any sound... the core
does it all.

	The core was designed to be unattached to the DLL... as well as be object oriented (ie, multiple instances
are possible/easy).  Therefore it may be extracted from NotSo and put into any app you wish to play NSF
sound.  The necessary files for operation are:

NSF_File.h / cpp
NSF_Core.h / cpp
NSF_6502.cpp
Wave_VRC7.cpp
Wave_Square.h
Wave_TND.h
Wave_VRC6.h
Wave_FME07.h
Wave_MMC5.h
Wave_N106.h
fmopl.h / c

	NSF_Core.h is the only file that needs to be #included by your project... NSF_File.h is only needed if you
want to use that class.  If you don't want to, it's not necessary.  Do not include the channel header files
(they are already included by NSF_Core.h).  NSF_Core.cpp, NSF_File.cpp, NSF_6502.cpp, Wave_VRC7.cpp, fmopl.c
all need to be compiled and linked if you wish to use the core.

	The core has a lifetime which *should* operate like so:
	
1) Constructor - various variables initialized, as well as some pointers getting pointed to the
	proper place.
	
2) Initialize - allocates some memory needed by the core.  If this call returns failure the core will
	not be able to be used.
	
3) LoadNSF - loads the NSF info into memory.

4) SetTrack - sets the track to play (The initial track of the NSF is NOT selected by default, a track
	must be given to the core before getting samples)
	
5) Use various functions to set options the way you want them

6) GetSamples - generates the PCM data for output

7) Destructor or call Destroy - cleans up allocated memory

Notes:
- 1 must be done first
- 2 must be done second
- 3 must be done third
- 4 must be done after every 3, and at least once before 6
- 5 is optional.
- A lot of config functions can be called before 3 (some even before 2)
- SetPlaybackSpeed is reset to default after 4.  If you want a non-standard playback speed, you have
	to call SetPlaybackSpeed after every call to SetTrack
- Ditto for SetFade, SetFadeTime
- GetSamples will NOT produce samples if the song is complete (fade fully out)


	The Core feeds the output signal gotten by each channel's GetOutput function though an output table.
There are 2 output tables per channel (one left, one right).  The purpose of these tables is to allow
for channel volume/pan control.  The tables are rebuilt via RebuildOutputTables() by the core everytime
channel volume/pan is changed, or the master volume is changed, or stereo mode is changed.

DFC:
====================================

	DFC is a ripoff of MFC (Microsoft Foundation Classes).  MFC was an addon that made things
like Dialogs easier to work with.. but also tacked on a lot of shit you don't want.  So I made
DFC to sort of mimic it, but without all the extra DLLs and other pains that come with MFC.

	If you are familiar with MFC, you can look at DFC and some of the GUI dialogs to figure out
how it works.  If you are unfamilir with MFC.. don't bother =P

	I planned on making a bigger section for this... but it's kind of silly.  DFC isn't an
important part of NotSo.