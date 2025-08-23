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

    u8 r8(u32 address)
    {
        u8 data = 0xFF;
        //if (!globalsettings.A20)
        //    address &= 0xFFEFFFFF;

        if (!testmode)
        {
            if (globalsettings.graphics == GlobalSettings::CGA && cga.address_in_memory_map(address))
                data = cga.memory8(address-cga.MEMORY_MAP_START());
            else if (globalsettings.graphics == GlobalSettings::VGA && address >= 0xA0000 && address <= 0xBFFFF)
                data = vga.r8(address&0x1FFFF);
            else if (globalsettings.graphics == GlobalSettings::HEGA && address >= 0xA0000 && address <= 0xBFFFF)
                data = hega.r8(address&0x1FFFF);
            //else if (address >= 0xE0000 && address <= 0xEFFFF)
            //    data = ltems._8(address&0xFFFF);
            else
            {
                if (sqems.is_ems(address))
                {
                    data = sqems.r8(sqems.translate_addr(address));
                }
                else if (address < membytes.size)
                {
                    data = membytes.bytes[address];
                }
            }
        }
        else
        {
            data = membytes.bytes[address];
        }
        return data;
    }
    void w8(u32 address, u8 data)
    {
        //if (!globalsettings.A20)
        //    address &= 0xFFEFFFFF;

        if (!testmode)
        {
            if (globalsettings.graphics == GlobalSettings::CGA && cga.address_in_memory_map(address))
                cga.memory8(address-cga.MEMORY_MAP_START()) = data;
            else if (globalsettings.graphics == GlobalSettings::HEGA && address >= 0xA0000 && address <= 0xBFFFF)
                hega.w8(address&0x1FFFF, data);
            else if (globalsettings.graphics == GlobalSettings::VGA && address >= 0xA0000 && address <= 0xBFFFF)
                vga.w8(address&0x1FFFF, data);
            //else if (address >= 0xE0000 && address <= 0xEFFFF)
            //    ltems._8(address&0xFFFF) = data;
            else
            {
                if (sqems.is_ems(address))
                {
                    sqems.w8(sqems.translate_addr(address), data);
                }
                else if
                    (
                     !(address >= 0xC0000 && address < 0xC8000) &&
                     !(address >= 0xF0000 && address < 0x100000) &&
                     address < membytes.size
                    )
                {
                    membytes.bytes[address] = data;
                }
            }
        }
        else
        {
            membytes.bytes[address] = data;
        }
    }
    u16 r16(u32 address)
    {
        u16 data{};
        data |= r8(address);
        data |= u16(r8(address+1))<<8;
        return data;
    }
    void w16(u32 address, u16 data)
    {
        w8(address,data&0xFF);
        w8(address+1,(data>>8));
    }
};

