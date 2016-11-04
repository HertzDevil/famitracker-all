
 FamiTracker source readme
---------------------------

 This software is available under GPL version 2, the license is included in this package (GPL.txt).
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
 YM2149 emulator by Mitsutaka Okazaki (same)
 FDS sound emulator from nezplug (improved by rainwarrior)
 PCM import resampler by Jarhmander

A quick file guide for beginners:
 - FamiTracker.cpp:			contains the main app class and is responsible for initializing and shutting down the app
 - MainFrm.cpp:				contains the main window code
 - FamiTrackerDoc.cpp:		holds the module data and is responsible for file saving/loading
 - FamiTrackerView.cpp:		used to draw the module on screen and handles keyboard input
 
 - PatternView.cpp:			used to display the pattern
 - FrameEditor.cpp:			the frame (order) window
 - SampleWindow.cpp:		draws the sample graph

 - Accelerator.cpp:			translates key shortcuts into commands
 - Settings.cpp:			responsible for loading/saving program settings
 
 - SoundGen.cpp:			contains the main class for the sound player
 - TrackerChannel.cpp:		the interface between doc/view and sound player
 - ChannelMap:				contains info about the expansion chips (and will later contain the active channel set, currently located in document)

 This project is based on MFC so learning the basics about that is a good start.
 
 If you got any questions then you may contact me
 
 E-mail: jsr@famitracker.com
 IRC: #famitracker on espernet

-----------------------------------
