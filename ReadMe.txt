FamiTracker source readme
-------------------------

 Source is avaliable under GPL. (You should have received the GPL license by this package) 
 This source is created using Microsoft Visual C++ .NET
 The NSF driver code is assembled using the CC65 assembler. (Not GPL:ed, that would be unconvenient)

 This is my first MFC app, things may look messy. (It may be awful to learn from)

* New stuff

This version
------------
 I organized the source to make it readable and less problematic to debug.
 A lot of asserts was used in the process.
 Effect P, fine pitch tuning.

 I implemented blargg's way to change frequency without resetting duty phase,
 but I cannot use it in NSF's since no players support it. I ended up removing it =(

Version 0.2.4
-------------
 Biggest difference is the way NSFs are created. No assembly file is generated anymore.
 
 
The ToDo-list
-------------
 - Clean the source. (as always, but much of it is done)
 - Note delay effect
 - Multiple tracks
 - Expansion chips
 - Vibrato (& tremolo) will use a full wave oscillator instead of only subtracting. (as an option)
 - Better MIDI file handling
 - Wave or/and MP3 exporting
 - It should be possible to make the famitracker sound player cycle perfect in respect to the nsf driver
 
Currently known bugs
--------------------
 - Note Off in DPCM
 
 - I have received reports that DPCM and triangle is too loud. However, I compared
   the triangle between this and a real NES and the difference was almost non-existent.
   It could be reports from a bad NSF player, or I will need more accurate data about it 
   to look into.
   
 - Noise emulation. I don't know if it is correct.
 
 
Effects list (new one)
-----------
0xy - Arpeggio
1xx - Portamento on, xx = speed
2xx = Portamento off
4xy - Vibrato, x = speed, y = depth
7xy - Tremolo, x = speed, y = depth
Axx - (unassigned)
Bxx - Jump to frame xx
Cxx - Halt
Dxx - Skip to next frame and jump to row xx
Exx - Set volume (xx = 00 - 0F)
Fxx - Speed
Gxx - (unassigned)
Hxy - Trigger hardware sweep up, x = period, y = shift
Ixy - Trigger hardware sweep down, x = period, y = shift
Pxx - Set fine pitch

