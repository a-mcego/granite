# granite

Granite is a PC emulator written in C++23. 

Requires libglfw3. 

Should compile on any platform that supports C++23.

# Features

* Runs through 8088 MPH and Area 5150 without crashing or freezing
* Supports GLaBIOS (PC & XT version) + FreeDOS for a fully open source stack
* Uses *glfw3* for video and *miniaudio* for audio

# Done

* 8088/8086/80188/80186. Not cycle correct, but the prefetch queue is emulated.
* Partial 286 protected mode. Award 2.07A boots.
* CGA and EGA working 99%
* Floppy drive and arbitrary hard disk sizes. (Automatic ROM patching for Xebec)
* PC Speaker, AdLib (OPL2), Sound Blaster, Creative Music System
* Bus mouse

# To do

* VGA / VESA modes
* 286 (protected mode) / 386 / 486
* FPU
* Sound Blaster AWE32
* "Dumb mode" MPU-401 passthrough to the OS's MIDI interface.
* Adjustable resolution and filtering options
* Overlay window when pressing F12
* Serial and parallel ports with passthrough to host
