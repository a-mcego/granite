#pragma once

struct MemoryManager8088
{
    CGA& cga;
    LTEMS& ltems;
    MemBytes& membytes;
    MemoryManager8088(CGA& cga_, LTEMS& ltems_, MemBytes& membytes_) : cga(cga_), ltems(ltems_), membytes(membytes_) {}

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
    u16 rw_segs[256] = {};
    u16 rw_offsets[256] = {};
    u8 rw_word{};

    bool testmode{};


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
        rw_words[rw_word] = _8(segment,index);
        rw_words[rw_word] |= (_8(segment,index+1) << 8);
        rw_segs[rw_word] = segment;
        rw_offsets[rw_word] = index;
        ++rw_word;
        return rw_words[rw_word-1];
    }

    void update()
    {
        for(int i=0; i<rw_word; ++i)
        {
            _8(rw_segs[i], rw_offsets[i]) = (rw_words[i]&0xFF);
            _8(rw_segs[i], rw_offsets[i]+1) = (rw_words[i]>>8);
        }
        rw_word = 0;
    }
};

