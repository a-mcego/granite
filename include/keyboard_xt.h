#pragma once

struct CHIP8255 //PC/XT keyboard etc
{
    CHIP8259& pic;
    CHIP8255(CHIP8259& pic_) : pic(pic_) {}

    static const u8 FLOPPY_DRIVES = 2;
    static const u8 HAS_8087 = 0;
    static const u8 MEMORY_BANKS = 4;

    enum VIDEO_CARD_TYPES
    {
        V_OTHER=0x00,
        CGA40=0x10,
        CGA80=0x20,
        MDA=0x30
    };
    static const VIDEO_CARD_TYPES VIDEO_CARD_TYPE = CGA80;

    //onboard DIP switches

    static const u8 SW1 = (FLOPPY_DRIVES>0?0x01:0x00)|(HAS_8087?0x02:0x00)|((MEMORY_BANKS-1)<<2)|VIDEO_CARD_TYPE|(FLOPPY_DRIVES>0?(FLOPPY_DRIVES-1)<<6:0);
    static const u8 SW2 = 0b1'1'1'1'0'0'1'0;//TODO: make these into setuppable bools

    static const u8 XT_SW = (FLOPPY_DRIVES>0?0x01:0x00)|(HAS_8087?0x02:0x00)|((MEMORY_BANKS-1)<<2)|VIDEO_CARD_TYPE|(FLOPPY_DRIVES>0?(FLOPPY_DRIVES-1)<<6:0);

    u8 regs[4] = {};
    u8 keyboard_self_test{0}; //if > 0, is doing a self test
    bool keyboard_self_test_done{};
    static const u8 KEYBOARD_SELF_TEST_LENGTH = 16; //:peeposhrug: lol
    static const u8 KEYBOARD_KEY_WAIT = 512; //wait before sending more keys
    u8 current_scancode = 0;
    bool is_initialized{false};

    deque<u8> scancode_queue;
    u16 kbd_wait{};

    void press(u8 scancode)
    {
        if (is_initialized)
        {
            scancode_queue.push_back(scancode);
        }
    }

    u8 read(u8 port) //port from 0 to 4! inclusive
    {
        if constexpr (DEBUG_LEVEL > 1)
        {
            cout << PRETTY_FUNCTION << ": " << u32(port) << " read!" << endl;
            cout << u32(regs[0]) << endl;
            cout << u32(regs[1]) << endl;
            cout << u32(regs[2]) << endl;
            cout << u32(regs[3]) << endl;
        }

        if (port == 0)
        {
            if (globalsettings.machine == globalsettings.MACHINE_PC && regs[1]&0x80)
            {
                return SW1;
            }
            else
            {
                if (keyboard_self_test_done)
                {
                    keyboard_self_test_done = false;
                    return 0xAA;
                }
                else
                {
                    u8 ret = current_scancode;
                    return ret;
                }
            }
        }
        if (port==1)
        {
            return regs[1];
        }
        if (port == 2)
        {
            u8 value{};
            if (globalsettings.machine == globalsettings.MACHINE_XT)
            {
                if (regs[1]&0x08)
                {
                    value |= (XT_SW&0xF0)>>4;
                }
                else
                {
                    value |= XT_SW&0x0F;
                }
            }
            else if (globalsettings.machine == globalsettings.MACHINE_PC)
            {
                if (regs[1]&0x04)
                {
                    value |= SW2&0x0F;
                }
                else
                {
                    value |= (SW2&0xF0)>>4;
                }
            }
            return value;
        }
        if (port == 4)
        {
            return 0;
        }

        //what
        cout << PRETTY_FUNCTION << " read from port: " << std::hex << 0x60+port << "??" << endl;
        std::abort();
    }

    void write(u8 port, u8 data) //port from 0 to 3! inclusive.
    {
        if (port == 1)
        {
            if ((data&0x40) && (!(regs[port]&0x40)))
            {
                cout << "Setting keyboard self test." << endl;
                if (keyboard_self_test == 0)
                {
                    keyboard_self_test = KEYBOARD_SELF_TEST_LENGTH;
                    is_initialized = false;
                }
            }
            if (data&0x80)
            {
                current_scancode = 0;
                pic.cpu_ack_irq(1);
            }
        }
        regs[port] = data;
        global_port0x61 = regs[1];
        //TODO: do i need to add these somewhere else
        //beeper.pb0 = (regs[1]&0x01)?1:0;
        //beeper.pb1 = (regs[1]&0x02)?1:0;
    }

    void cycle()
    {
        if (keyboard_self_test > 0)
        {
            --keyboard_self_test;
            if (keyboard_self_test == 0) //finished the test :-)
            {
                keyboard_self_test_done = true;
                is_initialized = true;
                pic.request_interrupt(1);
            }
        }

        if (!scancode_queue.empty())
        {
            if (kbd_wait == 0 && current_scancode == 0)
            {
                current_scancode = scancode_queue.front();
                bool result = pic.request_interrupt(1);
                if (result)
                {
                    scancode_queue.pop_front();
                }
                kbd_wait = KEYBOARD_KEY_WAIT;
            }
            else
            {
                --kbd_wait;
            }
        }
    }

    bool is_reset() { return false; } //this controller doesnt do resets.
};

