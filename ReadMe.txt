FamiTracker source readme
-------------------------
 Source is avaliable under GPL. (You should have received the GPL license by this package) 
 This source is created with Microsoft Visual C++ .NET
 The NSF driver code is assembled using the CC65 assembler.

 This is my first MFC app, things may look messy.

Version 0.2.4
-------------
 Biggest difference is the way NSFs are created. No assembly file is generated anymore.
 
Version 0.2.3
-------------
 (This was a development version)
 
 
The ToDo-list
-----------------
 - Clean the source.
 - Noise volume in NSF (fixed)
 - Sweep still isn't accurate enough
 - 
 
 
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



Old:
Axx - Speed
Bxx - Jump to frame xx
Cxx - Skip to next frame and jump to row xx
Dxx - Halt
Exx - Set volume (xx = 00 - 0F)
Fxx - Portamento on (xx = speed)
Gxx - Portamento off
Hxy - Trigger hardware sweep up, x = period, y = shift
Ixy - Trigger hardware sweep down, x = period, y = shift

