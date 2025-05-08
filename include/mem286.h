#pragma once

#include "cga.h"
#include "ltems.h"
#include "membytes.h"

struct MemoryManager286
{
    HEGA& hega;
    CGA& cga;
    LTEMS& ltems;
    MemBytes& membytes;
    MemoryManager286(HEGA& hega_, CGA& cga_, LTEMS& ltems_, MemBytes& membytes_) : hega(hega_), cga(cga_), ltems(ltems_), membytes(membytes_) {}

    bool testmode{};

    void dump_memory(const char* filename)
    {
        FILE* filu = fopen(filename, "wb");
        fwrite(membytes.bytes, membytes.size, 1, filu);
        fclose(filu);
    }

    u8 r8(u32 address)
    {
        u8 data = 0xFF;
        if (!globalsettings.A20)
            address &= 0xFFEFFFFF;

        if (!testmode)
        {
            if (globalsettings.graphics == GlobalSettings::CGA && address >= 0xB8000 && address <= 0xBFFFF)
                data = cga.memory8(address&0x7FFF);
            else if (globalsettings.graphics == GlobalSettings::HEGA && address >= 0xA0000 && address <= 0xBFFFF)
                data = hega.r8(address&0x1FFFF);
            else if (address >= 0xE0000 && address <= 0xEFFFF)
                data = ltems._8(address&0xFFFF);
            else if (address < membytes.size)
            {
                data = membytes.bytes[address];
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
        if (!globalsettings.A20)
            address &= 0xFFEFFFFF;

        if (!testmode)
        {
            if (globalsettings.graphics == GlobalSettings::CGA && address >= 0xB8000 && address <= 0xBFFFF)
                cga.memory8(address&0x7FFF) = data;
            else if (globalsettings.graphics == GlobalSettings::HEGA && address >= 0xA0000 && address <= 0xBFFFF)
                hega.w8(address&0x1FFFF, data);
            else if (address >= 0xE0000 && address <= 0xEFFFF)
                ltems._8(address&0xFFFF) = data;
            else if (address < membytes.size)
            {
                if (address >= 0xC0000 && address < 0x100000);
                else
                    membytes.bytes[address] = data;
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

