#pragma once

#include "cga.h"
#include "ltems.h"
#include "membytes.h"
#include "vga.h" // Added VGA

struct MemoryManager286
{
    HEGA& hega;
    CGA& cga;
    LTEMS& ltems;
    MemBytes& membytes;
    VGA& vga; // Added VGA reference
    MemoryManager286(HEGA& hega_, CGA& cga_, LTEMS& ltems_, MemBytes& membytes_, VGA& vga_) :
        hega(hega_), cga(cga_), ltems(ltems_), membytes(membytes_), vga(vga_) {}

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
            address &= 0xFFEFFFFF; // Apply A20 gate if disabled

        if (!testmode)
        {
            // VGA Memory Range: 0xA0000 - 0xBFFFF
            if (address >= 0xA0000 && address <= 0xBFFFF) {
                bool vga_ram_enabled = (vga.misc_output_register & 0x02) != 0;
                // Further checks based on vga.graphics_controller_registers[6] (Memory Map) might be needed here
                // to decide if VGA or another card (CGA/HEGA) handles the range.
                // For now, if VGA RAM is enabled, assume VGA handles A0000-BFFFF.
                if (vga_ram_enabled) {
                    return vga.read_memory(address);
                }
            }

            // CGA/HEGA/LTEMS/Main Memory (if VGA not active or address is outside VGA specific ranges)
            if (globalsettings.graphics == GlobalSettings::CGA && cga.address_in_memory_map(address))
                data = cga.memory8(address-cga.MEMORY_MAP_START());
            else if (globalsettings.graphics == GlobalSettings::HEGA && address >= 0xA0000 && address <= 0xBFFFF) // HEGA only if VGA not active for this range
                data = hega.r8(address&0x1FFFF);
            else if (address >= 0xE0000 && address <= 0xEFFFF) // LTEMS typically in high memory
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
    void w8(u32 address, u8 data)
    {
        if (!globalsettings.A20)
            address &= 0xFFEFFFFF; // Apply A20 gate if disabled

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

            // CGA/HEGA/LTEMS/Main Memory (if VGA not active or address is outside VGA specific ranges)
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

