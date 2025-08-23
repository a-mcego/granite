#pragma once

struct CPUZ80
{
    CPUZ80() {};

    u8 mem[1<<16] = {};

    u8 registers[26] = {};


    enum REG8
    {
        B ,C ,D ,E ,H ,L ,F ,A ,
        B2,C2,D2,E2,H2,L2,F2,A2,
        I, R,
    };
    enum REG16
    {
        BC=0,DE,HL,AF,
        BC2,DE2,HL2,AF2,
        IR,

        IX, IY, SP, PC
    };

    u8 read(REG8 reg)
    {
        //TODO
    }
    void write(REG8 reg, u8 value)
    {
        //TODO
    }
    u16 read(REG16 reg)
    {
        //TODO, have to combine bytes from separate bytes
    }
    void write(REG16 reg, u16 value)
    {
        //TODO, have to combine bytes from separate bytes
    }
    u8 read(u16 addr)
    {
        //TODO
    }
    void write(u16 addr, u8 value)
    {
        //TODO
    }


    void cycle()
    {
        //one cycle

        u16 pc_value = read(PC);
        u8 opcode = read(pc_value);
        write(PC, pc_value+1);

        //TODO: parse opcode etc.


    }

}
