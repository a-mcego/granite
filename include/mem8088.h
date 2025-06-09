#pragma once

#include "vga.h" // Added VGA

struct MemoryManager8088
{
    HEGA& hega;
    CGA& cga;
    LTEMS& ltems;
    MemBytes& membytes;
    VGA& vga; // Added VGA reference
    MemoryManager8088(HEGA& hega_, CGA& cga_, LTEMS& ltems_, MemBytes& membytes_, VGA& vga_) :
        hega(hega_), cga(cga_), ltems(ltems_), membytes(membytes_), vga(vga_) {}

    void dump_memory(const char* filename)
    {
        FILE* filu = fopen(filename, "wb");
        fwrite(membytes.bytes, membytes.size, 1, filu);
        fclose(filu);
    }

    bool testmode{};


    bool cga_used{};

    u8 r8(u16 segment, u16 index)
    {
        u8 data = 0xFF;
        u32 address = ((segment<<4)+index)&0xFFFFF; // Calculate physical address

        if (!testmode)
        {
            // VGA Memory Range: 0xA0000 - 0xBFFFF
            if (address >= 0xA0000 && address <= 0xBFFFF) {
                bool vga_ram_enabled = (vga.misc_output_register & 0x02) != 0;
                if (vga_ram_enabled) {
                    return vga.read_memory(address);
                }
            }

            // CGA/HEGA/LTEMS/Main Memory
            if (globalsettings.graphics == GlobalSettings::CGA && cga.address_in_memory_map(address))
                data = cga.memory8(address-cga.MEMORY_MAP_START());
            else if (globalsettings.graphics == GlobalSettings::HEGA && address >= 0xA0000 && address <= 0xBFFFF) // HEGA only if VGA not active
                data = hega.r8(address&0x1FFFF);
            else if (address >= 0xE0000 && address <= 0xEFFFF) // LTEMS
                data = ltems._8(address&0xFFFF);
            else if (address < membytes.size) // General RAM
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
    void w8(u16 segment, u16 index, u8 data)
    {
        u32 address = ((segment<<4)+index)&0xFFFFF; // Calculate physical address
        if (!testmode)
        {
            // VGA Memory Range: 0xA0000 - 0xBFFFF
            if (address >= 0xA0000 && address <= 0xBFFFF) {
                bool vga_ram_enabled = (vga.misc_output_register & 0x02) != 0;
                if (vga_ram_enabled) {
                    vga.write_memory(address, data);
                    return; // VGA handled the write
                }
            }

            // CGA/HEGA/LTEMS/Main Memory
            if (globalsettings.graphics == GlobalSettings::CGA && cga.address_in_memory_map(address))
                cga.memory8(address-cga.MEMORY_MAP_START()) = data;
            else if (globalsettings.graphics == GlobalSettings::HEGA && address >= 0xA0000 && address <= 0xBFFFF) // HEGA only if VGA not active
                hega.w8(address&0x1FFFF, data);
            else if (address >= 0xE0000 && address <= 0xEFFFF) // LTEMS
                ltems._8(address&0xFFFF) = data;
            else if (address < membytes.size) // General RAM
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
    u16 r16(u16 segment, u16 index)
    {
        u16 data{};
        data |= r8(segment, index);
        data |= u16(r8(segment, index+1))<<8;
        return data;
    }
    void w16(u16 segment, u16 index, u16 data)
    {
        w8(segment,index,data&0xFF);
        w8(segment,index+1,(data>>8));
    }
};

