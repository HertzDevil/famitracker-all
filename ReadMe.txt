
 FamiTracker source readme
---------------------------

 This software is avaliable under GPL version 2. 
 (You should have received the GPL license by this package) 
 This project and source is compiled using Microsoft Visual C++ 2008, required libraries are MFC and DirectX SDK
 The NSF driver code is assembled using the CA65 assembler.

 The export plugin support is written by Gradualore
 An example exporter can be found here: http://famitracker.shoodot.net/files/exporterplugin.zip

 Icon is made by Kuhneghetz
 Toolbar icons are made by ilkke

Other code that is not mine:

 blip_buffer 0.4.0 by blargg (LGPL)
 FFT code by Reliable Software (this is not GPL and cannot actually be spread with this, so please remove it if you republish this source package)
 YM2413 emulator by Mitsutaka Okazaki (doesn't seem to be GPL either)


Note:
 Assert errors in blip-buffer is most likely caused by thread synchronization failure
 between the main thread and the sound player. (todo)


A quick file guide for beginners:
 - FamiTracker.cpp:			contains the main app class and is responsible for initializing and shutting down the app
 - MainFrm.cpp:				contains the main window code
 - FamiTrackerDoc.cpp:		holds the module data and is responsible for file saving/loading
 - FamiTrackerView.cpp:		used to draw the module on screen and handles keyboard input
 
 - PatternView.cpp:			used to display the pattern display
 - FrameBoxWnd.cpp:			the frame (order) window
 - SampleWindow.cpp:		draws the sample graph

 - Accelerator.cpp:			translates key shortcuts into commands
 - Settings.cpp:			responsible for loading/saving program settings
 
 - SoundGen.cpp:			contains the main class for pattern playing and sound emulation
 - TrackerChannel.cpp:		the interface between doc/view and sound player
 - ChannelMap:				contains info about the expansion chips (and will later contain the active channel set, currently located in document)

 This project is based on MFC so learning the basics about that is a good start.

-----------------------------------

Bugs:
 - Release parts being skipped in saved files? (dunno if solved)

Todo:
 - VRC7 vibrato & pitch in NSF (probably fixed?)
 - FDS modulation reset on note triggers in NSF
 - Remove the FFT code 
 
Feature requests:
 - (on hold until existing bugs are fixed)
 
 