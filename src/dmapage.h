#pragma once


struct CHIPLS612N //DMA page registers, POST card value
{
    u8 pages[16] = {};

    u8 read(u8 port)
    {
        std::cout << "---LS612 READ--- " << u32(port) << ":" << u32(pages[port]) << std::endl;
        return pages[port];
    }

    void write(u8 port, u8 data)
    {
        if (port == 0 && data != pages[0])
        {
            std::cout << "---POSTCARD--- " << u32(data) << std::endl;
            //if (data==0x0C)
            //    startprinting=true;
        }
        else
        {
            std::cout << "---LS612 WRITE--- " << u32(port) << ":" << u32(data) << std::endl;
        }
        pages[port] = data;
    }
};
