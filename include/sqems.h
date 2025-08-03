#pragma once

struct SQEMS
{
    static constexpr u32 EMS_SIZE = 0x800001;  // 8MB
    static constexpr u8 PAGE_COUNT = 52;
    static constexpr u8 WRITABLE_PAGE_COUNT = 36;
    static constexpr u8 PAGE_SHIFT = 14;
    static constexpr u16 PAGE_MASK = 0xFC00;
    static constexpr u16 BASE_MASK = 0x3FFF;

    // I/O ports
    static constexpr u8 PAGE_SELECT_REGISTER = 0xE8;
    static constexpr u8 PAGE_SET_REGISTER_LO = 0xEA;
    static constexpr u8 PAGE_SET_REGISTER_HI = 0xEB;
    static constexpr u8 AUTOINCREMENT_FLAG = 0x40;

    u8 memory[EMS_SIZE] = {};
    u32 pages[PAGE_COUNT] = {};

    u8 current_page_index = 0;
    u8 current_page_set_lo = 0;
    bool auto_increment = false;

    // Page lookup table - translates memory address to page index
    static constexpr u8 page_lookup[64] =
    {
        36, 37, 38, 39,     // 0x00000 (inaccessible)
        40, 41, 42, 43,     // 0x10000 (inaccessible)
        44, 45, 46, 47,     // 0x20000 (inaccessible)
        48, 49, 50, 51,     // 0x30000 (inaccessible)
        12, 13, 14, 15,     // 0x40000
        16, 17, 18, 19,     // 0x50000
        20, 21, 22, 23,     // 0x60000
        24, 25, 26, 27,     // 0x70000
        28, 29, 30, 31,     // 0x80000
        32, 33, 34, 35,     // 0x90000
        0, 0, 0, 0,         // 0xA0000
        0, 0, 0, 0,         // 0xB0000
        0, 1, 2, 3,         // 0xC0000
        4, 5, 6, 7,         // 0xD0000
        8, 9, 10, 11,       // 0xE0000
        0, 0, 0, 0          // 0xF0000
    };

    SQEMS()
    {
        // Initialize memory with 0xAA pattern
        for (u32 i = 0; i < EMS_SIZE; i++)
        {
            memory[i] = 0xAA;
        }

        // Initialize page registers to default mappings
        for (u8 i = 0; i < PAGE_COUNT; i++)
        {
            pages[i] = (0x28 + i) << PAGE_SHIFT;  // Default page mappings
        }
    }

    void write(u16 port, u8 data)
    {
        switch (port)
        {
            case PAGE_SELECT_REGISTER:
                if (data >= WRITABLE_PAGE_COUNT)
                {
                    current_page_index = 0;
                }
                else
                {
                    current_page_index = data & 0x3F;  // Mask off auto-increment bit
                }
                auto_increment = (data & AUTOINCREMENT_FLAG) != 0;
                break;

            case PAGE_SET_REGISTER_LO:
                current_page_set_lo = data;
                break;

            case PAGE_SET_REGISTER_HI:
                {
                    u16 combined = (u16(data) << 8) | current_page_set_lo;
                    if (combined == 0xFFFF)
                    {
                        // Unmap page - restore default
                        pages[current_page_index] = (0x28 + current_page_index) << PAGE_SHIFT;
                    }
                    else
                    {
                        // Set page
                        pages[current_page_index] = u32(combined) << PAGE_SHIFT;
                    }

                    if (auto_increment)
                    {
                        current_page_index++;
                        if (current_page_index >= WRITABLE_PAGE_COUNT)
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
            case PAGE_SELECT_REGISTER:
                return current_page_index;

            case PAGE_SET_REGISTER_LO:
                return u8(pages[current_page_index] >> PAGE_SHIFT);

            case PAGE_SET_REGISTER_HI:
                return u8(pages[current_page_index] >> (PAGE_SHIFT + 8));

            default:
                return 0;
        }
    }

    u8& _8(u32 address)
    {
        u8 page_idx = page_lookup[(address & PAGE_MASK) >> PAGE_SHIFT];
        u32 ems_addr = pages[page_idx] + (address & BASE_MASK);
        return memory[ems_addr];
    }

    u16& _16(u32 address)
    {
        u8 page_idx = page_lookup[(address & PAGE_MASK) >> PAGE_SHIFT];
        u32 ems_addr = pages[page_idx] + (address & BASE_MASK);
        return *(u16*)(void*)(memory + ems_addr);
    }
};
