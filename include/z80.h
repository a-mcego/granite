#pragma once

#include "interrupt.h"

struct CPUZ80
{
    //MemoryManagerZ80& mem;
    CHIP8259& pic;
    CHIP8259& pic2;
    IOSystem& iosystem;
    CPUZ80(MemoryManager286& mem_, CHIP8259& pic_, CHIP8259& pic2_, IOSystem& iosystem_) : mem(mem_), pic(pic_), pic2(pic2_), iosystem(iosystem_) {}

    u8 mem[1<<16] = {};

    u8 registers[16] = {};


    enum REG8
    {
        B ,C ,D ,E ,H ,L ,F ,A ,
        B2,C2,D2,E2,H2,L2,F2,A2,
        I, R,
    }
    enum REG16
    {
        BC=0,DE,HL,AF,
        BC2,DE2,HL2,AF2,
        IR,

        IX, IY, SP, PC
    };


    void cycle()
    {

    }

}
