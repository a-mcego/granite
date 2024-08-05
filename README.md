# granite

Granite is a PC emulator written in C++23. 

Requires libglfw3. 

Should compile on any platform that supports C++23.

# Features

* Runs through 8088 MPH and Area 5150 without crashing or freezing
* Supports GLAbios (PC & XT version) + FreeDOS for a fully open source stack
* Uses *glfw3* for video and *miniaudio* for audio

# Done

* 8088/8086 that's mostly correct. Not cycle correct, though.
* Basic CGA (including 640x200 1bit composite color)
* 5 1/4" Floppy drive (up to 1.2 MB)
* Beeper
* AdLib (OPL2)
* 25 MB Hard disk

# Bugs

* Windows 1.04 launches but cannot start any applications, it often freezes.
* Keyboard keys get stuck down often

# To do

* Configurable hard disk size
* 3.5" floppy drive (make sure it works)
* Better CGA support (mainly for 8088mph & Area5150)
* EGA / VGA / VESA modes
* 186 / 286 / 386 / 486
* FPU
* Sound Blaster (up to AWE32)
* Support for IBM XT / AT bioses and more
* Adjustable resolution and filtering options
* Overlay window when pressing F12
* Mouse
* Serial and parallel ports
