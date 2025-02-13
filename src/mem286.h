#pragma once

#include "cga.h"
#include "ltems.h"
#include "membytes.h"

struct MemoryManager286
{
    CGA& cga;
    LTEMS& ltems;
    MemBytes& membytes;
    MemoryManager286(CGA& cga_, LTEMS& ltems_, MemBytes& membytes_) : cga(cga_), ltems(ltems_), membytes(membytes_) {}

    void dump_memory(const char* filename)
    {
        FILE* filu = fopen(filename, "wb");
        fwrite(membytes.bytes, membytes.size, 1, filu);
        fclose(filu);
    }

    u16 readonly_words[256] = {};
    u8 readonly_word{};
    u8 readonly_bytes[256] = {};
    u8 readonly_byte{};

    u16 rw_words[256] = {};
    u8 rw_word{};

    u16 INVALID_ADDRESS_16[16];
    u8 INVALID_ADDRESS_8[16];
    u16& direct16(u32 address)
    {
        //if (!kbd.A20())
        //    address &= 0xFFEFFFFF;
        if (address >= 0xB8000 && address <= 0xBFFFF)
            return cga.memory16(address&0x7FFF);
        if (address >= 0xE0000 && address <= 0xEFFFF)
            return ltems._16(address&0xFFFF);
        if (address >= 0xA0000 && address <= 0xFFFFF) //upper memory area, make read-only
        {
            ++readonly_word;
            readonly_words[readonly_word] = *(u16*)(void*)(membytes.bytes+address);
            return readonly_words[readonly_word];
        }
        if (address >= membytes.size)
        {
            INVALID_ADDRESS_16[0] = 0xFFFF;
            return INVALID_ADDRESS_16[0];
        }
        return *(u16*)(void*)(membytes.bytes+address);
    }
    u8& direct8(u32 address)
    {
        //std::cout << "direct8 " << std::hex << address << std::endl;
        if (address >= 0xB8000 && address <= 0xBFFFF)
        {
            //std::cout << '.';
            return cga.memory8(address&0x7FFF);
        }
        if (address >= 0xE0000 && address <= 0xEFFFF)
        {
            return ltems._8(address&0xFFFF);
        }
        if (address >= 0xF0000 && address <= 0xFFFFF)
        {
            ++readonly_byte;
            readonly_bytes[readonly_byte] = membytes.bytes[address];
            return readonly_bytes[readonly_byte];
        }
        if (address >= membytes.size)
        {
            //std::cout << "invalid addr " << u32(address) << std::endl;
            INVALID_ADDRESS_8[0] = 0xFF;
            return INVALID_ADDRESS_8[0];
        }
        //std::cout << "good addr " << u32(address) << std::endl;
        return *(u8*)(void*)(membytes.bytes+address);
    }

    void update()
    {

    }
};

