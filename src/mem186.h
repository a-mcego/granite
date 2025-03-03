#pragma once

#pragma once

struct MemoryManager186
{
    HEGA& hega;
    CGA& cga;
    LTEMS& ltems;
    MemBytes& membytes;
    MemoryManager186(HEGA& hega_, CGA& cga_, LTEMS& ltems_, MemBytes& membytes_) : hega(hega_), cga(cga_), ltems(ltems_), membytes(membytes_) {}
    bool testmode{};

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

    bool cga_used{};

    u8& _8(u16 segment, u16 index)
    {
        u32 total_address = (((segment<<4)+index)&0xFFFFF);
        if (!testmode)
        {
            if (total_address >= 0xF0000)
            {
                ++readonly_byte;
                readonly_bytes[readonly_byte] = membytes.bytes[total_address];
                return readonly_bytes[readonly_byte];
            }

            if ((total_address&0xF0000) == 0xE0000)
                return ltems._8(total_address&0xFFFF);
            if ((total_address&0xF8000) == 0xB8000)
            {
                cga_used = true;
                return cga.memory8(total_address&0x7FFF);
            }
        }
        return membytes.bytes[total_address];
    }
    u16& _16(u16 segment, u16 index)
    {
        u32 total_address = (((segment<<4)+index)&0xFFFFF);
        if (!testmode)
        {
            if (total_address >= 0xF0000)
            {
                ++readonly_word;
                readonly_words[readonly_word] = *(u16*)(void*)(membytes.bytes+total_address);
                return readonly_words[readonly_word];
            }
            if ((total_address&0xF0000) == 0xE0000)
                return ltems._16(total_address&0xFFFF);
            if ((total_address&0xF8000) == 0xB8000)
            {
                cga_used = true;
                return cga.memory16(total_address&0x7FFF);
            }
        }
        return *(u16*)(void*)&membytes.bytes[total_address];
    }

    void update()
    {
    }
};
