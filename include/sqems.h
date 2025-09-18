#pragma once

struct SQEMS
{
    static constexpr u8 PAGE_COUNT = 52;
    static constexpr u8 WRITABLE_PAGE_COUNT = 36;
    static constexpr u8 PAGE_SHIFT = 14; //16 kilobyte pages
    static constexpr u32 BASE_MASK = (1<<PAGE_SHIFT)-1; //0x3FFF

    // I/O ports
    static constexpr u8 PAGE_SELECT_REGISTER = 0;//0xE8;
    static constexpr u8 PAGE_SET_REGISTER_LO = 2;//0xEA;
    static constexpr u8 PAGE_SET_REGISTER_HI = 3;//0xEB;
    static constexpr u8 AUTOINCREMENT_FLAG = 0x40;

    static constexpr u32 MAX_PAGES = 512; //16kB pages
    static constexpr u32 MEMORY_SIZE = MAX_PAGES*16384;
    static constexpr u8 MAP_SIZE = 64;

    u8 memory[MEMORY_SIZE+1] = {};

    u32 page_map[MAP_SIZE] = {};

    u32 current_page_index = 0;
    u32 current_page_set_lo = 0;
    bool auto_increment = false;

    static constexpr u8 reverse_page_lookup[36] =
    {
        48,49,50,51,52,53,54,55,56,57,58,59,
        16,17,18,19,20,21,22,23,24,25,26,27,
        28,29,30,31,32,33,34,35,36,37,38,39,
    };

    SQEMS()
    {
        reset();
    }
    void reset()
    {
        // Initialize page registers to default mappings
        for (u32 i = 0; i < MAP_SIZE; i++)
        {
            page_map[i] = i<<PAGE_SHIFT;
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
                    current_page_index = data&0x3F;
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
                    u32 combined = (u32(data) << 8) | u32(current_page_set_lo);
                    if (combined >= MAX_PAGES)
                        combined = reverse_page_lookup[current_page_index];
                    page_map[reverse_page_lookup[current_page_index]] = u32(combined) << PAGE_SHIFT;

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
            //masks are setting unused bits to 1 according to SCAMP documentation
            case PAGE_SELECT_REGISTER:
                return current_page_index;// | (auto_increment?0x40:0x00) | 0x80;

            case PAGE_SET_REGISTER_LO:
                return u8(page_map[reverse_page_lookup[current_page_index]] >> PAGE_SHIFT);

            case PAGE_SET_REGISTER_HI:
                return u8(page_map[reverse_page_lookup[current_page_index]] >> (PAGE_SHIFT + 8));// | 0xFC;

            default:
                return 0;
        }
    }

    bool is_ems(u32 address)
    {
        //if beyond 1 MB
        /*if ((address >> PAGE_SHIFT) >= 64)
            return false;
        //if not EMS
        if (page_lookup[address >> PAGE_SHIFT] == 0xFF)
            return false;*/
        return true;
    }

    //sqems.get_ems_addr(address)

    u32 get_ems_addr(u32 address)
    {
        u32 ems_addr = page_map[address >> PAGE_SHIFT] | (address & BASE_MASK);
        //std::cout << std::hex << address << " -> EMS:" << ems_addr << std::endl;
        return ems_addr;
    }

    void w8(u32 address, u8 data)
    {
        memory[page_map[address >> PAGE_SHIFT] | (address & BASE_MASK)] = data;
    }
    void w16(u32 address, u16 data)
    {
        *(u16*)(&memory[page_map[address >> PAGE_SHIFT] | (address & BASE_MASK)]) = data;
    }
    u8 r8(u32 address)
    {
        return memory[page_map[address >> PAGE_SHIFT] | (address & BASE_MASK)];
    }
    u16 r16(u32 address)
    {
        return *(u16*)(&memory[page_map[address >> PAGE_SHIFT] | (address & BASE_MASK)]);
    }
};
