#pragma once

struct SQEMS
{
    static constexpr u8 PAGE_COUNT = 52;
    static constexpr u8 WRITABLE_PAGE_COUNT = 36;
    static constexpr u8 PAGE_SHIFT = 14; //16 kilobyte pages
    static constexpr u32 BASE_MASK = (1<<PAGE_SHIFT)-1; //0x3FFF

    // I/O ports
    static constexpr u8 PAGE_SELECT_REGISTER = 0xE8;
    static constexpr u8 PAGE_SET_REGISTER_LO = 0xEA;
    static constexpr u8 PAGE_SET_REGISTER_HI = 0xEB;
    static constexpr u8 AUTOINCREMENT_FLAG = 0x40;

    static constexpr u32 MAX_PAGES = 512; //16kB pages
    static constexpr u32 MEMORY_SIZE = MAX_PAGES*16384;

    u8 memory[MEMORY_SIZE] = {};

    bool is_port(u16 port)
    {
        return (port == PAGE_SELECT_REGISTER || port == PAGE_SET_REGISTER_LO || port == PAGE_SET_REGISTER_HI);
    }

    u32 pages[WRITABLE_PAGE_COUNT] = {};

    u32 current_page_index = 0;
    u32 current_page_set_lo = 0;
    bool auto_increment = false;

    //16kB chunk number for each page by default.
    static constexpr u8 default_pages[PAGE_COUNT] =
    {
        0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
        0x38,0x39,0x3A,0x3B,0x10,0x11,0x12,0x13,
        0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,
        0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,
        0x24,0x25,0x26,0x27,0x00,0x01,0x02,0x03,
        0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,
        0x0C,0x0D,0x0E,0x0F
    };

    // Page lookup table - translates memory address to page index. 16 kB chunks
    static constexpr u8 page_lookup[64] =
    {
        0xFF,0xFF,0xFF,0xFF,// 0x00000 (inaccessible)
        0xFF,0xFF,0xFF,0xFF,// 0x10000 (inaccessible)
        0xFF,0xFF,0xFF,0xFF,// 0x20000 (inaccessible)
        0xFF,0xFF,0xFF,0xFF,// 0x30000 (inaccessible)
        12, 13, 14, 15,     // 0x40000
        16, 17, 18, 19,     // 0x50000
        20, 21, 22, 23,     // 0x60000
        24, 25, 26, 27,     // 0x70000
        28, 29, 30, 31,     // 0x80000
        32, 33, 34, 35,     // 0x90000
        0xFF,0xFF,0xFF,0xFF,// 0xA0000
        0xFF,0xFF,0xFF,0xFF,// 0xB0000
        0xFF,0xFF,0xFF,0xFF,// 0xC0000
        4, 5, 6, 7,         // 0xD0000
        8, 9, 10, 11,       // 0xE0000
        0xFF,0xFF,0xFF,0xFF,// 0xF0000
    };

    SQEMS()
    {
        reset();
    }
    void reset()
    {
        // Initialize page registers to default mappings
        for (u32 i = 0; i < WRITABLE_PAGE_COUNT; i++)
        {
            pages[i] = default_pages[i] << PAGE_SHIFT;  // Default page mappings
        }
        current_page_index = 0;
        current_page_set_lo = 0;
        auto_increment = false;
    }

    void write(u16 port, u8 data)
    {
        switch (port)
        {
            case PAGE_SELECT_REGISTER:
                if ((data&0x3F) >= WRITABLE_PAGE_COUNT)
                {
                    current_page_index = 0;
                }
                else
                {
                    current_page_index = data;  // Mask off auto-increment bit
                }
                auto_increment = (data & AUTOINCREMENT_FLAG) != 0;
                //std::cout << "D" << std::hex << u16(data) << " -> P" << std::hex << u32(current_page_index) << std::endl;
                break;

            case PAGE_SET_REGISTER_LO:
                current_page_set_lo = data;
                //std::cout << "Datalo " << u16(data) << std::endl;
                break;

            case PAGE_SET_REGISTER_HI:
                {
                    //std::cout << "Datahi " << u16(data) << std::endl;
                    u32 combined = (u32(data) << 8) | current_page_set_lo;
                    if (combined == 0xFFFF)
                    {
                        // Unmap page - restore default
                        pages[current_page_index&0x3F] = (default_pages[current_page_index&0x3F]) << PAGE_SHIFT;
                        //std::cout << "P 0x" << std::hex << u32(current_page_index) << " set to default: " << (u32(default_pages[current_page_index])<<PAGE_SHIFT) << std::endl;
                    }
                    else
                    {
                        // Set page
                        pages[current_page_index&0x3F] = u32(combined) << PAGE_SHIFT;
                        //std::cout << "P 0x" << std::hex << u32(current_page_index) << " set to: " << (u32(combined)<<PAGE_SHIFT) << std::endl;
                        //if ((u32(combined)<<PAGE_SHIFT) == 0xE0000)
                        //    startprinting=true;
                    }

                    if (auto_increment)
                    {
                        current_page_index++;
                        if ((current_page_index) >= WRITABLE_PAGE_COUNT)
                        {
                            current_page_index = 0;
                        }
                    }
                }
                break;
        }
    }

    u8 read(u16 port)
    {
        switch (port)
        {
            //masks are setting unused bits to 1 according to SCAMP documentation
            case PAGE_SELECT_REGISTER:
                return current_page_index;// | (auto_increment?0x40:0x00) | 0x80;

            case PAGE_SET_REGISTER_LO:
                return u8(pages[current_page_index] >> PAGE_SHIFT);

            case PAGE_SET_REGISTER_HI:
                return u8(pages[current_page_index] >> (PAGE_SHIFT + 8)) | 0xFC;

            default:
                return 0;
        }
    }

    bool is_ems(u32 address)
    {
        //if beyond 1 MB
        if ((address >> PAGE_SHIFT) >= 64)
            return false;
        //if not EMS
        if (page_lookup[address >> PAGE_SHIFT] == 0xFF)
            return false;
        return true;
    }

    u32 translate_addr(u32 address)
    {
        if (!is_ems(address))
            return address;
        if (startprinting)
            std::cout << std::hex << address << " --EMS-> " << get_ems_addr(address) << std::endl;
        return get_ems_addr(address);
    }

    u32 get_ems_addr(u32 address)
    {
        u32 page_idx = page_lookup[address >> PAGE_SHIFT];
        u32 ems_addr = pages[page_idx] | (address & BASE_MASK);
        //std::cout << std::hex << address << " -> EMS:" << ems_addr << std::endl;
        return ems_addr;
    }

    void w8(u32 address, u8 data)
    {
        if (address < MEMORY_SIZE)
            memory[address] = data;
    }
    u8 r8(u32 address)
    {
        u8 ret = 0xFF;
        if (address < MEMORY_SIZE)
            ret = memory[address];
        return ret;
    }
};
