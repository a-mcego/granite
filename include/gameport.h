#pragma once

const u32 GAMEPORT_CYCLE = 64;

struct Gameport
{
    u8 reg{0xF0};
    /*
    bit 7: button 4 off
    bit 6: button 3 off
    bit 5: button 2 off
    bit 4: button 1 off

    bit 3: #2 axis y state
    bit 2: #2 axis x state
    bit 1: #1 axis y state
    bit 0: #1 axis x state
    */

    i16 axes[400] = {};
    u32 counters[400] = {};

    u32 axis_to_counter(i16 axis_value)
    {
        //u32 resistor = u32((u64(axis_value+32768)*100000ULL)>>16); //ohms
        u32 gameport_cycles = u32((i32(axis_value)+32768)>>2) + 384; //gameport_cycles
        //cout << std::dec << axis_value << " -> " << gameport_cycles/(14.318180) << "us" << std::hex << endl;
        return u32(gameport_cycles);
    }

    void set_button_state(u8 button, bool is_on)
    {
        reg = (reg&~(0x10<<button)) | (is_on?0:(0x10<<button));
    }

    void write([[maybe_unused]] u8 port, [[maybe_unused]] u8 data) //port from 0 to 0! inclusive.
    {
        reg |= 0x0F;
        for(int axis=0; axis<4; ++axis)
            counters[axis] = axis_to_counter(axes[axis]);
    }

    u8 read([[maybe_unused]] u8 port) //port from 0 to 0! inclusive.
    {
        return reg;
    }

    void cycle() //called at 14318180/GAMEPORT_CYCLE Hz
    {
        reg &= 0xF0;

        for (int i = 0; i < 4; ++i)
        {
            counters[i] -= (counters[i]>=GAMEPORT_CYCLE ? GAMEPORT_CYCLE : 0);
            reg |= (counters[i]>=GAMEPORT_CYCLE ? (1 << i): 0);
        }
    }
};
