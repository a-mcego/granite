#pragma once

#include "cga.h"
#include "vga.h"
#include "ltems.h"
#include "membytes.h"

struct MemoryManager286
{
    VGA& vga;
    HEGA& hega;
    CGA& cga;
    LTEMS& ltems;
    SQEMS& sqems;
    MemBytes& membytes;
    MemoryManager286(VGA& vga_, HEGA& hega_, CGA& cga_, LTEMS& ltems_, SQEMS& sqems_, MemBytes& membytes_) : vga(vga_), hega(hega_), cga(cga_), ltems(ltems_), sqems(sqems_), membytes(membytes_) {}

    bool testmode{};

    void dump_memory(const char* filename)
    {
        FILE* filu = fopen(filename, "wb");
        fwrite(membytes.bytes, membytes.size, 1, filu);
        fclose(filu);
    }

    void reset()
    {
        sqems.reset();
    }

    enum struct DEVICETYPE : u8
    {
        NONE,
        VIDEO_CGA,
        VIDEO_EGA,
        VIDEO_VGA,
        ROM,
        LTEMS,
        SQEMS,
        BOARD_MEMORY
    };

    DEVICETYPE devicemap[2048] = {};

    void register_device(DEVICETYPE device, u32 address)
    {
        devicemap[(address>>13)&0x7FF] = device;
    }

    void register_devs()
    {
        if (testmode)
        {
            for(u32 addr=0x00000; addr<0x100000; addr+=8192)
                register_device(DEVICETYPE::BOARD_MEMORY, addr);
        }
        else
        {
            DEVICETYPE video_device = DEVICETYPE::VIDEO_CGA;
            if (globalsettings.graphics == GlobalSettings::HEGA)
                video_device = DEVICETYPE::VIDEO_EGA;
            else if (globalsettings.graphics == GlobalSettings::VGA)
                video_device = DEVICETYPE::VIDEO_VGA;
            for(u32 addr=0x00000; addr<0x40000; addr+=8192)
                register_device(DEVICETYPE::BOARD_MEMORY, addr);
            for(u32 addr=0x40000; addr<0xA0000; addr+=8192)
                register_device(DEVICETYPE::SQEMS, addr);
            for(u32 addr=0xA0000; addr<0xC0000; addr+=8192)
                register_device(video_device, addr);
            for(u32 addr=0xC0000; addr<0xC8000; addr+=8192)
                register_device(DEVICETYPE::ROM, addr);
            for(u32 addr=0xC8000; addr<0xD0000; addr+=8192)
                register_device(DEVICETYPE::BOARD_MEMORY, addr);
            for(u32 addr=0xD0000; addr<0xE0000; addr+=8192)
                register_device(DEVICETYPE::SQEMS, addr);
            for(u32 addr=0xE0000; addr<0xF0000; addr+=8192)
                register_device(DEVICETYPE::BOARD_MEMORY, addr);
            for(u32 addr=0xF0000; addr<0x100000; addr+=8192)
                register_device(DEVICETYPE::ROM, addr);

            for(u32 addr=0x100000;;addr+=8192)
            {
                if (membytes.size < addr+8192)
                    break;
                register_device(DEVICETYPE::BOARD_MEMORY, addr);
            }


        }
    }

    u8 r8(u64 address)
    {
        u8 data = 0xFF;
        u16 map_index = ((address>>13)&globalsettings.A20mask);
        switch(devicemap[map_index])
        {
        case DEVICETYPE::NONE:
            break;
        case DEVICETYPE::VIDEO_CGA:
            if (cga.address_in_memory_map(address))
                data = cga.memory8(address-cga.MEMORY_MAP_START());
            break;
        case DEVICETYPE::VIDEO_EGA:
            data = hega.r8(address&0x1FFFF);
            break;
        case DEVICETYPE::VIDEO_VGA:
            data = vga.r8(address&0x1FFFF);
            break;
        case DEVICETYPE::LTEMS:
            data = ltems._8(address&0xFFFF);
            break;
        case DEVICETYPE::SQEMS:
            data = sqems.r8(address);
            break;
        case DEVICETYPE::ROM:
        case DEVICETYPE::BOARD_MEMORY:
            data = membytes.bytes[address];
            break;
        }

        return data;
    }
    void w8(u64 address, u8 data)
    {
        u16 map_index = ((address>>13)&globalsettings.A20mask);
        switch(devicemap[map_index])
        {
        case DEVICETYPE::NONE:
        case DEVICETYPE::ROM:
            break;
        case DEVICETYPE::VIDEO_CGA:
            if (cga.address_in_memory_map(address))
                cga.memory8(address-cga.MEMORY_MAP_START()) = data;
            break;
        case DEVICETYPE::VIDEO_EGA:
            hega.w8(address&0x1FFFF, data);
            break;
        case DEVICETYPE::VIDEO_VGA:
            vga.w8(address&0x1FFFF, data);
            break;
        case DEVICETYPE::LTEMS:
            ltems._8(address&0xFFFF) = data;
            break;
        case DEVICETYPE::SQEMS:
            sqems.w8(address, data);
            break;
        case DEVICETYPE::BOARD_MEMORY:
            membytes.bytes[address] = data;
            break;
        }
    }
    u16 r16(u64 address)
    {
        u16 data{};
        u16 map_index = ((address>>13)&globalsettings.A20mask);
        switch(devicemap[map_index])
        {
        case DEVICETYPE::NONE:
            break;
        case DEVICETYPE::VIDEO_CGA:
            if (cga.address_in_memory_map(address))
            {
                data = cga.memory8(address-cga.MEMORY_MAP_START());
                data |= cga.memory8(address+1-cga.MEMORY_MAP_START())<<8;
            }
            break;
        case DEVICETYPE::VIDEO_EGA:
            data = hega.r8(address&0x1FFFF);
            data |= hega.r8((address+1)&0x1FFFF)<<8;
            break;
        case DEVICETYPE::VIDEO_VGA:
            data = vga.r8(address&0x1FFFF);
            data |= vga.r8((address+1)&0x1FFFF)<<8;
            break;
        case DEVICETYPE::LTEMS:
            data = ltems._8(address&0xFFFF);
            data |= ltems._8((address+1)&0xFFFF)<<8;
            break;
        case DEVICETYPE::SQEMS:
            data = sqems.r16(address);
            break;
        case DEVICETYPE::ROM:
        case DEVICETYPE::BOARD_MEMORY:
            data = *(u16*)(&membytes.bytes[address]);
            //data |= membytes.bytes[address+1]<<8;
            break;
        }

        return data;

        /*u16 data{};
        data |= r8(address);
        data |= u16(r8(address+1))<<8;
        return data;*/
    }
    void w16(u64 address, u16 data)
    {
        w8(address,data&0xFF);
        w8(address+1,(data>>8));
    }
};

