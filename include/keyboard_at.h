#pragma once

#include "interrupt.h"

struct CHIP8042 //AT keyboard etc
{
    CHIP8259& pic;
    CHIP8042(CHIP8259& pic_) : pic(pic_) {}

    u8 current_scancode = 0;
    bool is_initialized{false};

    deque<u8> scancode_queue;
    u16 kbd_wait{};

    u16 clear_input_bit{}; //time to clear the input bit
    u16 set_output_bit{}; //time to set the output bit

    /* status byte documentation
    Bit 7: Parity error
    Bit 6: Timeout on kbd->ctrl
    Bit 5: Timeout on ctrl->kbd
    Bit 4: Keyboard lock
    Bit 3: Command/Data
        0: Last write to input buffer was data (port 0x60). 1: Last write to input buffer was a command (port 0x64).

    Bit 2: System flag. 0 after power on reset, 1 after ctrl self-test

    Bit 1: Input buffer status
        0: empty, can be written. 1: full, don't write yet.

    Bit 0: Output buffer status
        0: empty, don't read yet. 1: full, can be read.
    */
    u8 status_byte{0x10};
    u8 result{};
    u8 ram[32] = {};
    u16 command{0x100};

    /*
    P1 documentation
    bit 7 	Keyboard lock 	0: locked, 1: not locked
    bit 6 	Display 	0: CGA, 1: MDA
    bit 5 	Manufacturing jumper 	0: installed, 1: not installed
            with jumper the BIOS runs an infinite diagnostic loop
    bit 4 	RAM on motherboard 	0: 512 KB, 1: 256 KB
    bit 3 	  	Unused in ISA, EISA, PS/2 systems
            Can be configured for clock switching
    bit 2 	  	Unused in ISA, EISA, PS/2 systems
            Can be configured for clock switching
        Keyboard power 	PS/2 MCA: 0: keyboard power normal, 1: no power
    bit 1 	Mouse data in 	Unused in ISA
    bit 0 	Keyboard data in 	Unused in ISA
    */
    u8 P1{0xA0};
    /*
    P2 documentation
    bit 7 	Keyboard data 	data to keyboard
    bit 6 	Keyboard clock
    bit 5 	IRQ12 	0: IRQ12 not active, 1: active
    bit 4 	IRQ1 	0: IRQ1 not active, 1: active
    bit 3 	Mouse clock 	Unused in ISA
    bit 2 	Mouse data 	Unused in ISA. Data to mouse
    bit 1 	A20 	0: A20 line is forced 0, 1: A20 enabled
    bit 0 	Reset 	0: reset CPU, 1: normal
    */
    u8 P2{};

    bool A20()
    {
        return P2&2;
    }
    bool is_reset()
    {
        bool ret = P2&1;
        P2 |= 0x01;
        if (!ret)
        {
            ram[0] |= 0x04;
        }
        return !ret;
    }

    void press(u8 scancode)
    {
        if (is_initialized)
        {
            scancode_queue.push_back(scancode);
        }
    }

    u8 read(u8 port) //port from 0 to 4! inclusive
    {
        if (port == 0)
        {
            if (set_output_bit)
                return 0;
            status_byte &= ~0x08;
            if (command == 0x100)
            {
                result = current_scancode;
                current_scancode = 0;
                P2 &= ~0x10;
            }
            else if (command >= 0x00 && command <= 0x7F)
            {
                result = ram[command&0x1F];
            }
            else if (command == 0xC0)
            {
                result = P1;
            }
            else if (command == 0xD0)
            {
                result = P2;
            }
            else if (command == 0xAA)
            {
                result = 0x55; // self test OK!
            }
            else if (command == 0xAB)
            {
                result = 0;
            }
            else if (command == 0xA1) //firmware version! :-) AMI only so far.
            {
                result = 0x01;
            }
            else if (command == 0xAD);
            else if (command == 0xAE);
            else if (command == 0xD1); //**WRITE** P2..
            else if (command == 0xE0) //keyboard clock (bit 0), keyboard data (bit 1) wat do i do.
            {
                if (ram[0]&0x04) //kbd disabled
                {
                    result = 0;
                }
                else
                {
                    result = rand()&0x03;
                }
            }
            else
            {
                std::cout << globalsettings.current_IP << ": kbd_at: UNKNOWN command is: " << (u32)command << std::endl;
                std::abort();
            }
            //std::cout << "command is: " << (u32)command << std::endl;
            //std::cout << globalsettings.current_IP << ": keyboard read from 0x6" << u16(port) << ", with data " << u32(result) << std::endl;
            status_byte &= 0xFE; //clear "output byte available" bit
            command = 0x100;
            return result;
        }
        if (port == 1)
        {
            //std::cout << globalsettings.current_IP << ": keyboard read from 0x6" << u16(port) << ", with data " << u32(global_port0x61) << std::endl;
            return global_port0x61;
        }
        if (port == 4)
        {
            if (globalsettings.current_IP != 0xF9407)
            //std::cout << globalsettings.current_IP << ": keyboard read from 0x6" << u16(port) << ", with data " << u32(status_byte & (set_output_bit?0xFE:0xFF)) << " ," << u32(clear_input_bit) << std::endl;
            return status_byte;
        }

        //what
        cout << PRETTY_FUNCTION << " read from port: " << std::hex << 0x60+port << "??" << endl;
        std::abort();
    }

    void write(u8 port, u8 data) //port from 0 to 4! inclusive. 0 is port 0x80, 4 is port 0x84 etc.
    {
        //if (port != 1)
            std::cout << std::hex << globalsettings.current_IP << ": keyboard write to 0x6" << u16(port) << ", with data " << u16(data) << std::endl;
        if (port == 0)
        {
            status_byte &= ~0x0B;
            //status_byte = (status_byte&0b1111'1110);
            //status_byte |= 0x03; //input byte done - don't do more!
            //clear_input_bit = 128;
            //set_output_bit = 192;
            std::cout << "command is: " << (u32)command << std::endl;
            if (command >= 0x00 && command <= 0x3F); //read keyboard RAM???
            else if (command >= 0x40 && command <= 0x7F) //write keyboard RAM
            {
                ram[command&0x1F] = data;
                if ((command&0x1F) == 0x00)
                {
                    status_byte = (status_byte&~0x04) | (data&0x04);
                }
            }
            else if (command == 0xC1) //write P1
            {
                //P1 = (P1&~0x0F) | (data&0x0F);
            }
            else if (command == 0xD1) //write P2
            {
                //P2 = (P2&~0x0F) | (data&0x0F);
                P2 = data;
                globalsettings.SetA20(bool(P2&0x02));
            }
            else if (command == 0xAE); //enable kbd
            else if (command == 0xAD); //disable kbd
            else if (command == 0x100) //no cmd
            {
                if (data == 0xF2)
                    result = 0xFE;
                else
                    result = 0xFA; //ACK
                status_byte |= 0x01; //output byte available!
            }
            else if (command == 0xDF) //enable A20 (hp vectra) / (quadtel?)
            {
                P2 |= 0x02;
                globalsettings.SetA20(bool(P2&0x02));
            }
            else if (command == 0xDD) //disable A20 (hp vectra) / (quadtel?)
            {
                P2 &= ~0x02;
                globalsettings.SetA20(bool(P2&0x02));
            }
            else if (command == 0xE0) //read test inputs
            {
                //result = 0x03;
            }
            else
            {
                std::cout << "Unknown kbd command: " << u32(command) << std::endl;
                std::abort();
            }
            command = 0x100;
            return;
        }
        else if (port == 1)
        {
            global_port0x61 = (global_port0x61&~0x0F) | (data&0x0F);
            return;
        }
        else if (port == 4)
        {
            ram[0] &= 0xEF;
            status_byte = status_byte | 0x08;
            command = data;
            status_byte |= 0x02; //input byte done - don't do more!
            clear_input_bit = 128;
            set_output_bit = 192;
            if (false);
            else if (data >= 0x00 && data <= 0x7F) // write kbd ctrl ram
            {
            }
            else if (data == 0xA1) //read firmware version (unimplemented)
            {
            }
            else if (data == 0xAA)
            {
                result = 0x55;
                status_byte |= 0x04; //self-test done
                std::cout << "Keyboard self-test done!" << std::endl;
                is_initialized = true;
            }
            else if (command == 0xAB) // interface test - return 0 for success
            {
            }
            else if (command == 0xAD) //disable kbd
            {
                ram[0] |= 0x10; //set bit 4 -> disable kbd
                result = 0xFE;
            }
            else if (command == 0xAE) //enable kbd
            {
                ram[0] &= 0xEF; //clear bit 4 -> enable kbd
                result = 0xFE;
            }
            else if (data == 0xC0) //read P1
            {
                result = 0b0000'0000;
            }
            else if (data == 0xD1) //write P2
            {
            }
            else if (command == 0xE0) // show keyboard clock (bit0) and data (bit1) ???
            {
            }
            else if (data >= 0xF0 && data <= 0xFF && !(data&1)) //RESET
            {
                P2 &= ~0x01;
                set_output_bit = 0;
                command = 0x100;
            }
            else
            {
                std::cout << "unknown kbd: " << std::hex << "0x6" << u16(port) << ", with data " << u16(data) << std::endl;

            }
        }
    }

    u32 printer{};
    void cycle()
    {
        ++printer;
        if (printer&0x100000)
        {
            std::cout << u32(ram[0]) << " " << scancode_queue.size() << " " << u32(status_byte) << " " << u32(current_scancode) << " " << u32(kbd_wait) << std::endl;
            printer=0;
        }

        if (clear_input_bit > 0)
        {
            --clear_input_bit;
            if (clear_input_bit == 0)
            {
                status_byte &= ~0x02; //clear bit 1: input buffer bit
            }
        }
        if (set_output_bit > 0)
        {
            --set_output_bit;
            if (set_output_bit == 0)
            {
                status_byte |= 0x01;
            }
        }

        if (!scancode_queue.empty())
        {
            //if (set_output_bit == 0 && clear_input_bit == 0 && (status_byte&1) == 0 && kbd_wait == 0 && current_scancode == 0 && !(ram[0] & 0x10))
            {
                current_scancode = scancode_queue.front();
                //status_byte |= 1;
                scancode_queue.pop_front();
                std::cout << "-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";
                std::cout << "keyboard! " << u32(current_scancode) << " ";

                //if (ram[0]&0x01) //IRQ enabled?
                if (P2&0x10)
                {
                    P2 |= 0x10;
                    pic.request_interrupt(1);
                    std::cout << " int 1" << std::endl;
                    status_byte |= 1;
                }
                else
                {
                    status_byte |= 1;
                }
                std::cout << std::endl;

                kbd_wait = 2048;
            }
        }

        if (kbd_wait > 0)
        {
            --kbd_wait;
        }

        if (!(status_byte & 0x01))
        {
            P2 &= ~0x10;
        }
    }
};
