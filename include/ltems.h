#pragma once


struct LTEMS
{
    u8 memory[4*1024*1024+1] = {};

    u32 pages[4] = {};

    void write(u8 port, u8 data) // port from 0 to 3 inclusive
    {
        pages[port] = u32(data)*0x4000U;
    }
    u8 read([[maybe_unused]] u8 port) // no port is readable
    {
        return 0;
    }

    u8& _8(u16 index)
    {
        u16 page = (index>>14);
        u16 address = (index&0x3FFFU);
        return memory[pages[page]+address];
    }
    u16& _16(u16 index)
    {
        u16 page = (index>>14);
        u16 address = (index&0x3FFFU);
        return *(u16*)(void*)(memory+(pages[page]+address));
    }
};

