# granite

Granite is a PC emulator written in C++23. 

Requires libglfw3. 

Should compile on any platform that supports C++23.

# Features

* Runs through 8088 MPH and Area 5150 without crashing or freezing
* Supports GLAbios (PC version) + FreeDOS for a fully open source stack
* Uses *glfw3* for video and *miniaudio* for audio

# Done

* 8088/8086 that's mostly correct
* Basic CGA (with 640x200 1bit composite color)
* Floppy drive
* Beeper
* AdLib (OPL2)

# To do

* Better CGA
* Hard disk
* 186 / 286 / 386
* FPU
* Sound Blaster (up to AWE32)
* Support for XT / AT bioses and more
* Adjustable resolution and filtering options