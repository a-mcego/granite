#pragma once

#include <ctime>

struct CHIP146818 // RTC & CMOS
{
    u8 CMOSdata[64] = {};
    u8 current_reg = 0x0D;

    std::string filename;

    bool changed{};
    void save()
    {
        if (changed && !filename.empty())
        {
            FILE* filu = fopen(filename.c_str(),"wb");
            if (filu != NULL)
            {
                fwrite(CMOSdata, 64, 1, filu);
                fclose(filu);
            }
        }
        changed = false;
    }

    void load()
    {
        if (!filename.empty())
        {
            FILE* filu = fopen(filename.c_str(),"rb");
            if (filu != NULL)
            {
                fread(CMOSdata, 64, 1, filu);
                fclose(filu);
            }
        }
    }

    void update_time()
    {
        time_t now = time(nullptr);
        tm* t = localtime(&now);

        auto toBCD = [](int val) -> u8 { return ((val / 10) << 4) | (val % 10); };

        CMOSdata[0x00] = toBCD(t->tm_sec);
        CMOSdata[0x02] = toBCD(t->tm_min);
        CMOSdata[0x04] = toBCD(t->tm_hour);
        CMOSdata[0x06] = toBCD(t->tm_wday + 1);
        CMOSdata[0x07] = toBCD(t->tm_mday);
        CMOSdata[0x08] = toBCD(t->tm_mon + 1);
        CMOSdata[0x09] = toBCD(t->tm_year % 100);
        CMOSdata[0x32] = toBCD(t->tm_year / 100 + 19);

        //disable alarm - top two bits set to 1
        CMOSdata[0x01] = 0xFF;
        CMOSdata[0x03] = 0xFF;
        CMOSdata[0x05] = 0xFF;

        CMOSdata[0x0A] = 0b00100110;
        // Status Register B
        // bit 2: 1 = binary mode (0 = BCD)
        // bit 1: 1 = 24h mode (0 = 12h)
        CMOSdata[0x0B] = 0b00000010;
        CMOSdata[0x0D] = 0x80;
    }

    CHIP146818(std::string filename_=""):filename(filename_)
    {
        load();
        update_time();
    }

    void printtime()
    {
        std::cout << std::dec << "19" << u16(CMOSdata[0x09]) << "-" << u16(CMOSdata[0x08]) << "-" << u16(CMOSdata[0x07]) << " " << u16(CMOSdata[0x04]) << ":" << u16(CMOSdata[0x02]) << ":" << u16(CMOSdata[0x00]) << std::endl << std::hex;
        std::cout << u16(CMOSdata[0x0A]) << " ";
        std::cout << u16(CMOSdata[0x0B]) << " ";
        std::cout << u16(CMOSdata[0x0C]) << " ";
        std::cout << u16(CMOSdata[0x0D]) << endl;
    }

    u8 read(u8 port) //port from 0 to 1! inclusive
    {
        u8 ret=0;
        if (port == 1) //read
        {
            ret = CMOSdata[current_reg];
            //current_reg = 0x0D;
        }
        return ret;
    }

    void write(u8 port, u8 data) //port from 0 to 1! inclusive.
    {
        if (port == 0)
        {
            current_reg = (data&0x3F);
        }
        else if (port == 1)
        {
            if (current_reg != 0x0C && current_reg != 0x0D) //EXPLAIN: why this IF?
            {
                CMOSdata[current_reg] = data;
                changed = true;
            }
            //current_reg = 0x0D;
        }
    }

    void cycle()
    {
    }
};

