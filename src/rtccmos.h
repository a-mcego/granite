#pragma once


struct CHIP146818 // RTC & CMOS
{
    u8 CMOSdata[256] = {};
    u8 current_reg = 0x0D;

    u8 read(u8 port) //port from 0 to 1! inclusive
    {
        u8 ret=0;
        if (port == 1) //read
        {
            ret = CMOSdata[current_reg];
            std::cout << "CMOS READ: " << u32(current_reg) << ":" << u32(ret) << std::endl;

            //current_reg = 0x0D;
        }
        return ret;
    }

    void write(u8 port, u8 data) //port from 0 to 1! inclusive.
    {
        if (port == 0)
        {
            current_reg = (data&0x7F);
        }
        else if (port == 1)
        {
            CMOSdata[current_reg] = data;
            //current_reg = 0x0D;
            std::cout << "CMOS WRITE: " << u32(current_reg) << ":" << u32(data) << std::endl;
        }
    }

    void cycle()
    {
    }
};

