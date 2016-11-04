FamiTracker source readme
-------------------------

 This software is avaliable under GPL. (You should have received the GPL license by this package) 
 This source was created using Microsoft Visual C++ .NET
 The NSF driver code is assembled using the CC65 assembler.

 This is my first MFC app, things may look messy.

Version 0.2.6

* New stuff
-----------
 - New sequence editor with a few presets
 - Support for larger NSF's than 32kb (bankswitched NSFs)
 - Mix pasted notes with pattern notes
 - Preview of previous/next frame
 - Shift + Left/Right is replaced with Ctrl + Left/Right to make selection easier
 - Note delay effect
 - Multiple tracks
 - MIDI output
 
 
Fixed bugs
----------
 - File loading crashed when songs had 64 frames (in debug)
 - Save DPCM sample bug fixed
 - Hardware sweep
 - Sample loop bug (NSF)
 - E volume in NSF
 - DPCM loop in NSF
 - Problems with PRG according to SLiVeR

Yet to do:

 I tried blargg's way to change frequency without resetting duty phase,
 but I cannot use it in NSF's yet since almost no players supported it.
 
The ToDo-list
-------------
 - Clean the source. (as always, but much of it is done)
 - Expansion chips
 - Vibrato (& tremolo) will use a full wave oscillator instead of only subtracting. (as an option)
 - Better MIDI file handling
 - Wave or/and MP3 exporting
 
