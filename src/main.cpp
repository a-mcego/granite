#if defined(__clang__)
    #define PRETTY_FUNCTION __PRETTY_FUNCTION__
#elif defined(__GNUC__) || defined(__GNUG__)
    #define PRETTY_FUNCTION __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
    #define PRETTY_FUNCTION __FUNCSIG__
#else
    #define PRETTY_FUNCTION "()"
#endif

#include "glad/gl.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <deque>
#include <concepts>
#include <vector>
#include <thread>

#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_WAV
#define MA_NO_FLAC
#define MA_NO_MP3
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION
#include "miniaudio.h"

#include "opl2.h"

using namespace std;

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;
using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;
using i8 = int8_t;

double startTime{};

constexpr bool FLOPPY_DEBUG = false;

bool startprinting=false;

bool turbo = false;
bool lockstep = true;

bool file_exists(const std::string& filename)
{
    bool exists{};
    FILE* filu = fopen(filename.c_str(), "rb");
    exists = (filu!=nullptr);
    if (exists)
        fclose(filu);
    return exists;
}

#include "cgabios.h" //cga character ROM

struct GlobalSettings
{
    bool functionkeypress{false};
    bool sound_on{true};
    bool entertrace{false};
    bool A20{true};
    bool ctrl_alt{false};

    void SetA20(bool value)
    {
        A20 = value;
        //std::cout << "A20 is now: " << A20 << std::endl;
    }

    u32 current_IP{};

    enum MACHINE
    {
        MACHINE_PC,
        MACHINE_XT,
        MACHINE_AT,

        MACHINE_COUNT
    } machine=MACHINE_PC;

    enum GRAPHICS
    {
        CGA,
        HEGA,
        VGA
    } graphics=CGA;

    bool opl_enabled{true};
    bool gblast_enabled{true};
    bool sblast_enabled{true};
} globalsettings{};

    u8 global_port0x61{0x00}; //system control port B
    u64 cycles{};
    std::string machineName;


const u32 DEBUG_LEVEL = 0;

const u32 PRINT_START = 0;

const char* r8_names[8] = {"AL","CL","DL","BL","AH","CH","DH","BH"};
const char* r16_names[8] = {"AX","CX","DX","BX","SP","BP","SI","DI"};
const char* seg_names[8] = {"ES", "CS", "SS", "DS", "(invalid segment register #4)", "(invalid segment register #5)", "(invalid segment register #6)", "(invalid segment register #7)"};

const u8 byte_parity[256] =
{
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
};

#include "sampleplayer.h"
#include "screen.h"
#include "YM3812.h"
#include "gameport.h"
#include "cga.h"
#include "hega.h"
#include "vga.h"
#include "ltems.h"
#include "sqems.h"
#include "interrupt.h"
#include "mem286.h"
#include "mem186.h"
#include "mem8088.h"
#include "beeper.h"
#include "gameblaster.h"
#include "soundblaster.h"
#include "audio.h"
#include "rtccmos.h"
#include "keyboard_at.h"
#include "keyboard_xt.h"
#include "dmapage.h"
#include "dma.h"
#include "pit.h"
#include "harddisk.h"
#include "harddisk_ata.h"
#include "diskette.h"
#include "busmouse.h"


struct IOSystem
{
    Gameport gameport;
    CGA cga;
    HEGA hega;
    VGA vga;
    LTEMS ltems;
    SQEMS sqems;
    CHIP8259 pic, pic2;
    MemBytes membytes;
    MemoryManager8088 mem88{vga, hega, cga, ltems, sqems, membytes};
    MemoryManager186 mem186{hega, cga, ltems, membytes};
    MemoryManager286 mem286{vga, hega, cga, ltems, sqems, membytes};
    BEEPER beeper;
    YM3812 ym3812;
    GameBlaster gameblaster;
    CHIP146818 cmos;
    CHIP8042 kbd_at{pic};
    CHIP8255 kbd_xt{pic};
    BusMouse busmouse{pic};
    CHIPLS612N dmapage;
    CHIP8237 dma{0, dmapage, mem286}, dma2{1, dmapage, mem286};
    CHIP8253 pit{pic, beeper, dma};
    SoundBlaster soundblaster{dma, pic};
    DISKS disks; //two disks
    HARDDISK_XEBEC harddisk{disks, dma, pic};
    HARDDISK_ATA harddisk_ata{disks, dma, pic, pic2};
    DISKETTECONTROLLER diskettecontroller{dma, pic};
    MiniAudio miniaudio{beeper, ym3812, gameblaster, soundblaster};

    template<typename IOSIZE> requires (std::same_as<IOSIZE, u8> || std::same_as<IOSIZE, u16>)
    void io_out(u16 port, IOSIZE data)
    {
        if constexpr(std::same_as<IOSIZE,u16>)
        {
            //TODO: 16-bit I/O properly
            if (port == 0x1F0)
            {
                harddisk_ata.write(port-0x1F0, data);
                return;
            }


            io_out<u8>(port, data&0xFF);
            io_out<u8>(port+1, data>>8);
            return;
        }

        //if (startprinting)
        //cout << "Write Port 0x" << u32(port) << " ----> 0x" << u32(data) << endl;
        if (false);
        /*else if (port == 0xA0)
        {
            cout << "NMI interrupt setting: " << data << endl;
        }*/
        else if (port >= 0xE8 && port <= 0xEF)
        {
            sqems.write(port,data&0xFF);
        }
        else if (port >= 0x40 && port <= 0x43)
        {
            pit.write(port-0x40, data&0xFF);
        }
        else if (port >= 0x20 && port <= 0x21)
        {
            pic.write(port-0x20, data&0xFF);
        }
        else if (port >= 0xa0 && port <= 0xa1)
        {
            pic2.write(port-0xa0, data&0xFF);
        }
        else if (port >= 0x00 && port <= 0x0F)
        {
            dma.write(port-0x00, data&0xFF);
        }
        else if (port >= 0xC0 && port <= 0xDF)
        {
            dma2.write((port-0xC0)>>1, data&0xFF);
        }
        else if (port >= 0x60 && port <= 0x64)
        {
            if (globalsettings.machine == GlobalSettings::MACHINE_AT)
                kbd_at.write(port-0x60, data&0xFF);
            else
                kbd_xt.write(port-0x60, data&0xFF);
        }
        else if (port >= 0x23C && port <= 0x23F)
        {
            busmouse.write(port-0x23C, data&0xFF);
        }
        else if (globalsettings.gblast_enabled && port >= 0x220 && port <= 0x22F)
        {
            if (globalsettings.sblast_enabled && port >= 0x224 && port <= 0x22F)
                soundblaster.write(port-0x220, data&0xFF);
            gameblaster.write(port-0x220, data&0xFF);
        }
        else if (globalsettings.sblast_enabled && port >= 0x220 && port <= 0x22F)
        {
            soundblaster.write(port-0x220, data&0xFF);
        }
        else if (port >= 0x3B0 && port <= 0x3DF)
        {
            if (globalsettings.graphics == GlobalSettings::HEGA)
                hega.write(port-0x3B0, data&0xFF);
            else if (globalsettings.graphics == GlobalSettings::VGA)
                vga.write(port-0x3B0, data&0xFF);
            else if (globalsettings.graphics == GlobalSettings::CGA && port >= 0x3D0)
                cga.write(port-0x3D0, data&0xFF);
        }
        else if (port >= 0x3F0 && port <= 0x3F7)
        {
            if (port != 0x3F6)
                diskettecontroller.write(port-0x3F0, data&0xFF);
            if (port >= 0x3F6)
                harddisk_ata.write(port-0x3F0+8, data&0xFF);
        }
        else if (globalsettings.opl_enabled && port >= 0x388 && port <= 0x389)
        {
            ym3812.write(port-0x388, data&0xFF);
        }
        else if (port >= 0x320 && port <= 0x323)
        {
            harddisk.write(port-0x320, data&0xFF);
        }
        else if (port >= 0x1F0 && port <= 0x1F7)
        {
            harddisk_ata.write(port-0x1F0, data&0xFF);
        }
        else if (port == 0x201)
        {
            gameport.write(port-0x201, data&0xFF);
        }
        else if (port >= 0x260 && port <= 0x263)
        {
            ltems.write(port-0x260, data&0xFF);
        }
        else if (port >= 0x70 && port <= 0x71)
        {
            cmos.write(port-0x70, data&0xFF);
        }
        else if (port >= 0x80 && port <= 0x8F)
        {
            dmapage.write(port-0x80, data&0xFF);
        }
        else
        {
            //if constexpr(DEBUG_LEVEL > 0)
                //cout << "Writing Unknown port " << u32(port) << " data=" << u32(data&0xFF) << endl;
            //std::abort();
        }
    }

    template<typename IOSIZE> requires (std::is_same_v<IOSIZE, u8> || std::is_same_v<IOSIZE, u16>)
    IOSIZE io_in(u16 port)
    {
        if constexpr(std::same_as<IOSIZE,u16>)
        {
            //TODO: 16-bit I/O properly
            if (port == 0x1F0)
                return harddisk_ata.read(port-0x1F0);

            return io_in<u8>(port) | (io_in<u8>(port+1) << 8);
        }

        IOSIZE data = 0xff;

        if (false);
        else if (sqems.is_port(port))
        {
            data = sqems.read(port);
        }
        else if (port >= 0x40 && port <= 0x43)
        {
            data = pit.read(port-0x40);
        }
        else if (port >= 0x20 && port <= 0x21)
        {
            data = pic.read(port-0x20);
        }
        else if (port >= 0xA0 && port <= 0xA1)
        {
            data = pic2.read(port-0xA0);
        }
        else if (port >= 0x00 && port <= 0x0F)
        {
            data = dma.read(port-0x00);
        }
        else if (port >= 0xC0 && port <= 0xDF)
        {
            data = dma2.read((port-0xC0)>>1);
        }
        else if (port >= 0x60 && port <= 0x64)
        {
            if (globalsettings.machine == GlobalSettings::MACHINE_AT)
                data = kbd_at.read(port-0x60);
            else
                data = kbd_xt.read(port-0x60);
        }
        else if (globalsettings.gblast_enabled && port >= 0x220 && port <= 0x22F)
        {
            if (globalsettings.sblast_enabled && port >= 0x224 && port <= 0x22F)
                data = soundblaster.read(port-0x220);
            else
                data = gameblaster.read(port-0x220);
        }
        else if (port >= 0x23C && port <= 0x23F)
        {
            data = busmouse.read(port-0x23C);
        }
        else if (globalsettings.sblast_enabled && port >= 0x220 && port <= 0x22F)
        {
            data = soundblaster.read(port-0x220);
        }
        else if (port >= 0x3B0 && port <= 0x3DF)
        {
            if (globalsettings.graphics == GlobalSettings::HEGA)
                data = hega.read(port-0x3B0);
            else if (globalsettings.graphics == GlobalSettings::VGA)
                data = vga.read(port-0x3B0);
            else if (globalsettings.graphics == GlobalSettings::CGA && port >= 0x3D0)
                data = cga.read(port-0x3D0);
        }
        else if (port >= 0x3F0 && port <= 0x3F7)
        {
            if (port != 0x3F6)
                data = diskettecontroller.read(port-0x3F0);
            if (port >= 0x3F6)
                data |= harddisk_ata.read(port-0x3F0+8);
        }
        else if (globalsettings.opl_enabled && port >= 0x388 && port <= 0x389)
        {
            data = ym3812.read(port-0x388);
        }
        else if (port >= 0x320 && port <= 0x323)
        {
            data = harddisk.read(port-0x320);
        }
        else if (port >= 0x1F0 && port <= 0x1F7)
        {
            data = harddisk_ata.read(port-0x1F0);
        }
        else if (port == 0x201)
        {
            data = gameport.read(port-0x201);
        }
        else if (port >= 0x260 && port <= 0x263)
        {
            data = ltems.read(port-0x260);
        }
        else if (port >= 0x70 && port <= 0x71)
        {
            data = cmos.read(port-0x70);
        }
        else if (port >= 0x80 && port <= 0x8F)
        {
            data = dmapage.read(port-0x80);
        }
        else
        {
            //if constexpr(DEBUG_LEVEL > 0)
                //cout << "Reading unknown port " << u32(port) << endl;
            //std::abort();
        }
        //cout << "Read  Port 0x" << u32(port) << " <---- 0x" << u32(data) << endl;
        return data;
    }
};

#include "8088mc.h"
#include "808x_microcoded.h"
#include "808x.h"
#include "80186.h"
#include "80286.h"

struct Machine;
using CPUCycleFn = void (Machine::*)();
using CPUResetFn = void (Machine::*)();
using CPUIrqFn = bool (Machine::*)(int);
struct Machine
{
    IOSystem p;

    CPU8086 cpu8086{p.mem88, p.pic, p.pic2, p};
    CPU8088MC cpu8088mc{p.mem88, p.pic, p.pic2, p};
    CPU80186 cpu80186{p.mem186, p.pic, p.pic2, p};
    CPU80286 cpu80286{p.mem286, p.pic, p.pic2, p};

    u32 current_cpu{};
    u64 cpu_steps{};

    CPUCycleFn cycle_fn;
    CPUResetFn reset_fn;
    CPUIrqFn irq_fn;

    void init_cpu(u32 cpu_type, i32 prefetch_queue_size)
    {
        if (cpu_type == 0)
        {
            cycle_fn = &Machine::cycle_8086;
            reset_fn = &Machine::reset_8086;
            irq_fn = &Machine::irq_if_accept_8086;
            cpu8086.prefetch_queue_size = prefetch_queue_size;
        }
        else if (cpu_type == 1)
        {
            cycle_fn = &Machine::cycle_8088mc;
            reset_fn = &Machine::reset_8088mc;
            irq_fn = &Machine::irq_if_accept_8088mc;
            cpu8088mc.prefetch_queue_size = prefetch_queue_size;
        }
        else if (cpu_type == 2)
        {
            cycle_fn = &Machine::cycle_80186;
            reset_fn = &Machine::reset_80186;
            irq_fn = &Machine::irq_if_accept_80186;
            cpu80186.prefetch_queue_size = prefetch_queue_size;
        }
        else if (cpu_type == 3)
        {
            cycle_fn = &Machine::cycle_80286;
            reset_fn = &Machine::reset_80286;
            irq_fn = &Machine::irq_if_accept_80286;
        }

        p.pic2.main_pic = &p.pic;
    }

    void cycle_8086() { cpu8086.cycle(); ++cpu_steps; }
    void cycle_8088mc() { cpu8088mc.cycle(); ++cpu_steps; }
    void cycle_80186() { cpu80186.cycle(); ++cpu_steps; }
    void cycle_80286() { cpu80286.cycle(); ++cpu_steps; }

    void reset_8086() { cpu8086.reset(); }
    void reset_8088mc() { cpu8088mc.reset(); }
    void reset_80186() { cpu80186.reset(); }
    void reset_80286() { cpu80286.reset(); }

    bool irq_if_accept_8086(int irq) { bool ret = cpu8086.accepts_interrupts(); if (ret) cpu8086.irq(irq); return ret; }
    bool irq_if_accept_8088mc(int irq) { bool ret = cpu8088mc.accepts_interrupts(); if (ret) cpu8088mc.irq(irq); return ret; }
    bool irq_if_accept_80186(int irq) { bool ret = cpu80186.accepts_interrupts(); if (ret) cpu80186.irq(irq); return ret; }
    bool irq_if_accept_80286(int irq) { bool ret = cpu80286.accepts_interrupts(); if (ret) cpu80286.irq(irq); return ret; }

    void cycle_cpu()
    {
        (this->*cycle_fn)();
    }
    void reset_cpu()
    {
        (this->*reset_fn)();
    }
    bool irq_if_accept(int irq)
    {
        return (this->*irq_fn)(irq);
    }

    u64 cpumult_num{1};
    u64 cpumult_denom{3};
    i64 cpu_cycle_accum{};

    u32 hega_counter{};

    void fast_stuff([[maybe_unused]] u64 clock)
    {
        cpu_cycle_accum += cpumult_num;
        while(cpu_cycle_accum >= 0)
        {
            cpu_cycle_accum -= cpumult_denom;
            cycle_cpu();
        }
    }

    void gfx_stuff(u64 clock)
    {
        if (clock%8 == 0)
        {
            if (globalsettings.graphics == GlobalSettings::VGA)
            {
                hega_counter += p.vga.clock_numer();
                while (hega_counter >= p.vga.clock_denom())
                {
                    p.vga.cycle();
                    hega_counter -= p.vga.clock_denom();
                }
            }
        }
    }

    void real_stuff(u64 clock)
    {
        if (clock%8 == 0)
        {
            if (globalsettings.graphics == GlobalSettings::HEGA)
            {
                hega_counter += p.hega.clock_numer();
                while (hega_counter >= p.hega.clock_denom())
                {
                    p.hega.cycle();
                    hega_counter -= p.hega.clock_denom();
                }
            }
            else if (globalsettings.graphics == GlobalSettings::CGA)
                p.cga.cycle();
        }
        if (clock%16 == 0)
        {
            if (globalsettings.machine == GlobalSettings::MACHINE_AT)
            {
                p.kbd_at.cycle();
                if (p.kbd_at.is_reset())
                    reset_cpu();
            }
            else
            {
                p.kbd_xt.cycle();
                if (p.kbd_xt.is_reset())
                    reset_cpu();
            }
            p.harddisk.cycle();
        }


        if (clock%215 == 0) //ca. every 15 microseconds.
        {
            global_port0x61 ^= 0x10;
            p.harddisk_ata.cycle();
        }
        if (clock%12 == 0)
        {
            p.pit.cycle();
        }
        if (clock%512 == 0)
            p.diskettecontroller.cycle();
        if (clock%298 == 0) //ca. 48kHz. handles sound output in general
            p.miniaudio.cycle();
        if (globalsettings.opl_enabled && clock%288 == 0)
        {
            p.ym3812.cycle();
            p.ym3812.cycle_timers();
        }
        if (clock%GAMEPORT_CYCLE == 0)
            p.gameport.cycle();
        if (globalsettings.gblast_enabled && clock%256 == 0)
            p.gameblaster.cycle();
        if (globalsettings.sblast_enabled)
            p.soundblaster.cycle();
    }

} mac;

unsigned char key_lookup_xt[GLFW_KEY_LAST+1] = {};
unsigned char key_lookup_at[GLFW_KEY_LAST+1] = {};
void initialize_key_lookup()
{
    key_lookup_at[GLFW_KEY_F9] = 0x01;
    key_lookup_at[GLFW_KEY_F5] = 0x03;
    key_lookup_at[GLFW_KEY_F3] = 0x04;
    key_lookup_at[GLFW_KEY_F1] = 0x05;
    key_lookup_at[GLFW_KEY_F2] = 0x06;
    //key_lookup_at[GLFW_KEY_F12] = 0x07;
    key_lookup_at[GLFW_KEY_F10] = 0x09;
    key_lookup_at[GLFW_KEY_F8] = 0x0A;
    key_lookup_at[GLFW_KEY_F6] = 0x0B;
    key_lookup_at[GLFW_KEY_F4] = 0x0C;
    key_lookup_at[GLFW_KEY_TAB] = 0x0D;
    key_lookup_at[GLFW_KEY_GRAVE_ACCENT] = 0x0E; // ` (back tick)
    key_lookup_at[GLFW_KEY_LEFT_ALT] = 0x11;
    key_lookup_at[GLFW_KEY_LEFT_SHIFT] = 0x12;
    key_lookup_at[GLFW_KEY_LEFT_CONTROL] = 0x14;
    key_lookup_at[GLFW_KEY_Q] = 0x15;
    key_lookup_at[GLFW_KEY_1] = 0x16;
    key_lookup_at[GLFW_KEY_Z] = 0x1A;
    key_lookup_at[GLFW_KEY_S] = 0x1B;
    key_lookup_at[GLFW_KEY_A] = 0x1C;
    key_lookup_at[GLFW_KEY_W] = 0x1D;
    key_lookup_at[GLFW_KEY_2] = 0x1E;
    key_lookup_at[GLFW_KEY_C] = 0x21;
    key_lookup_at[GLFW_KEY_X] = 0x22;
    key_lookup_at[GLFW_KEY_D] = 0x23;
    key_lookup_at[GLFW_KEY_E] = 0x24;
    key_lookup_at[GLFW_KEY_4] = 0x25;
    key_lookup_at[GLFW_KEY_3] = 0x26;
    key_lookup_at[GLFW_KEY_SPACE] = 0x29;
    key_lookup_at[GLFW_KEY_V] = 0x2A;
    key_lookup_at[GLFW_KEY_F] = 0x2B;
    key_lookup_at[GLFW_KEY_T] = 0x2C;
    key_lookup_at[GLFW_KEY_R] = 0x2D;
    key_lookup_at[GLFW_KEY_5] = 0x2E;
    key_lookup_at[GLFW_KEY_N] = 0x31;
    key_lookup_at[GLFW_KEY_B] = 0x32;
    key_lookup_at[GLFW_KEY_H] = 0x33;
    key_lookup_at[GLFW_KEY_G] = 0x34;
    key_lookup_at[GLFW_KEY_Y] = 0x35;
    key_lookup_at[GLFW_KEY_6] = 0x36;
    key_lookup_at[GLFW_KEY_M] = 0x3A;
    key_lookup_at[GLFW_KEY_J] = 0x3B;
    key_lookup_at[GLFW_KEY_U] = 0x3C;
    key_lookup_at[GLFW_KEY_7] = 0x3D;
    key_lookup_at[GLFW_KEY_8] = 0x3E;
    key_lookup_at[GLFW_KEY_COMMA] = 0x41;
    key_lookup_at[GLFW_KEY_K] = 0x42;
    key_lookup_at[GLFW_KEY_I] = 0x43;
    key_lookup_at[GLFW_KEY_O] = 0x44;
    key_lookup_at[GLFW_KEY_0] = 0x45;
    key_lookup_at[GLFW_KEY_9] = 0x46;
    key_lookup_at[GLFW_KEY_PERIOD] = 0x49;
    key_lookup_at[GLFW_KEY_SLASH] = 0x4A;
    key_lookup_at[GLFW_KEY_L] = 0x4B;
    key_lookup_at[GLFW_KEY_SEMICOLON] = 0x4C;
    key_lookup_at[GLFW_KEY_P] = 0x4D;
    key_lookup_at[GLFW_KEY_MINUS] = 0x4E;
    key_lookup_at[GLFW_KEY_APOSTROPHE] = 0x52;
    key_lookup_at[GLFW_KEY_LEFT_BRACKET] = 0x54;
    key_lookup_at[GLFW_KEY_EQUAL] = 0x55;
    key_lookup_at[GLFW_KEY_CAPS_LOCK] = 0x58;
    key_lookup_at[GLFW_KEY_RIGHT_SHIFT] = 0x59;
    key_lookup_at[GLFW_KEY_ENTER] = 0x5A;
    key_lookup_at[GLFW_KEY_RIGHT_BRACKET] = 0x5B;
    key_lookup_at[GLFW_KEY_BACKSLASH] = 0x5D;
    key_lookup_at[GLFW_KEY_BACKSPACE] = 0x66;
    key_lookup_at[GLFW_KEY_KP_1] = 0x69;
    key_lookup_at[GLFW_KEY_KP_4] = 0x6B;
    key_lookup_at[GLFW_KEY_KP_7] = 0x6C;
    key_lookup_at[GLFW_KEY_KP_0] = 0x70;
    key_lookup_at[GLFW_KEY_KP_DECIMAL] = 0x71;
    key_lookup_at[GLFW_KEY_KP_2] = 0x72;
    key_lookup_at[GLFW_KEY_KP_5] = 0x73;
    key_lookup_at[GLFW_KEY_KP_6] = 0x74;
    key_lookup_at[GLFW_KEY_KP_8] = 0x75;
    key_lookup_at[GLFW_KEY_ESCAPE] = 0x76;
    key_lookup_at[GLFW_KEY_NUM_LOCK] = 0x77;
    //key_lookup_at[GLFW_KEY_F11] = 0x78;
    key_lookup_at[GLFW_KEY_KP_ADD] = 0x79;
    key_lookup_at[GLFW_KEY_KP_3] = 0x7A;
    key_lookup_at[GLFW_KEY_KP_SUBTRACT] = 0x7B;
    key_lookup_at[GLFW_KEY_KP_MULTIPLY] = 0x7C;
    key_lookup_at[GLFW_KEY_KP_9] = 0x7D;
    key_lookup_at[GLFW_KEY_SCROLL_LOCK] = 0x7E;
    key_lookup_at[GLFW_KEY_F7] = 0x83;

    key_lookup_xt[GLFW_KEY_ESCAPE] = 0x01;
    key_lookup_xt[GLFW_KEY_1] = 0x02;
    key_lookup_xt[GLFW_KEY_2] = 0x03;
    key_lookup_xt[GLFW_KEY_3] = 0x04;
    key_lookup_xt[GLFW_KEY_4] = 0x05;
    key_lookup_xt[GLFW_KEY_5] = 0x06;
    key_lookup_xt[GLFW_KEY_6] = 0x07;
    key_lookup_xt[GLFW_KEY_7] = 0x08;
    key_lookup_xt[GLFW_KEY_8] = 0x09;
    key_lookup_xt[GLFW_KEY_9] = 0x0A;
    key_lookup_xt[GLFW_KEY_0] = 0x0B;
    key_lookup_xt[GLFW_KEY_MINUS] = 0x0C;
    key_lookup_xt[GLFW_KEY_EQUAL] = 0x0D;
    key_lookup_xt[GLFW_KEY_BACKSPACE] = 0x0E;
    key_lookup_xt[GLFW_KEY_TAB] = 0x0F;
    key_lookup_xt[GLFW_KEY_Q] = 0x10;
    key_lookup_xt[GLFW_KEY_W] = 0x11;
    key_lookup_xt[GLFW_KEY_E] = 0x12;
    key_lookup_xt[GLFW_KEY_R] = 0x13;
    key_lookup_xt[GLFW_KEY_T] = 0x14;
    key_lookup_xt[GLFW_KEY_Y] = 0x15;
    key_lookup_xt[GLFW_KEY_U] = 0x16;
    key_lookup_xt[GLFW_KEY_I] = 0x17;
    key_lookup_xt[GLFW_KEY_O] = 0x18;
    key_lookup_xt[GLFW_KEY_P] = 0x19;
    key_lookup_xt[GLFW_KEY_LEFT_BRACKET] = 0x1A;
    key_lookup_xt[GLFW_KEY_RIGHT_BRACKET] = 0x1B;
    key_lookup_xt[GLFW_KEY_ENTER] = 0x1C;
    key_lookup_xt[GLFW_KEY_LEFT_CONTROL] = 0x1D;
    key_lookup_xt[GLFW_KEY_A] = 0x1E;
    key_lookup_xt[GLFW_KEY_S] = 0x1F;
    key_lookup_xt[GLFW_KEY_D] = 0x20;
    key_lookup_xt[GLFW_KEY_F] = 0x21;
    key_lookup_xt[GLFW_KEY_G] = 0x22;
    key_lookup_xt[GLFW_KEY_H] = 0x23;
    key_lookup_xt[GLFW_KEY_J] = 0x24;
    key_lookup_xt[GLFW_KEY_K] = 0x25;
    key_lookup_xt[GLFW_KEY_L] = 0x26;
    key_lookup_xt[GLFW_KEY_SEMICOLON] = 0x27;
    key_lookup_xt[GLFW_KEY_APOSTROPHE] = 0x28;
    key_lookup_xt[GLFW_KEY_GRAVE_ACCENT] = 0x29;
    key_lookup_xt[GLFW_KEY_LEFT_SHIFT] = 0x2A;
    key_lookup_xt[GLFW_KEY_BACKSLASH] = 0x2B;
    key_lookup_xt[GLFW_KEY_Z] = 0x2C;
    key_lookup_xt[GLFW_KEY_X] = 0x2D;
    key_lookup_xt[GLFW_KEY_C] = 0x2E;
    key_lookup_xt[GLFW_KEY_V] = 0x2F;
    key_lookup_xt[GLFW_KEY_B] = 0x30;
    key_lookup_xt[GLFW_KEY_N] = 0x31;
    key_lookup_xt[GLFW_KEY_M] = 0x32;
    key_lookup_xt[GLFW_KEY_COMMA] = 0x33;
    key_lookup_xt[GLFW_KEY_PERIOD] = 0x34;
    key_lookup_xt[GLFW_KEY_SLASH] = 0x35;
    key_lookup_xt[GLFW_KEY_RIGHT_SHIFT] = 0x36;
    key_lookup_xt[GLFW_KEY_PRINT_SCREEN] = 0x37;
    key_lookup_xt[GLFW_KEY_LEFT_ALT] = 0x38;
    key_lookup_xt[GLFW_KEY_SPACE] = 0x39;
    key_lookup_xt[GLFW_KEY_CAPS_LOCK] = 0x3A;
    key_lookup_xt[GLFW_KEY_F1] = 0x3B;
    key_lookup_xt[GLFW_KEY_F2] = 0x3C;
    key_lookup_xt[GLFW_KEY_F3] = 0x3D;
    key_lookup_xt[GLFW_KEY_F4] = 0x3E;
    key_lookup_xt[GLFW_KEY_F5] = 0x3F;
    key_lookup_xt[GLFW_KEY_F6] = 0x40;
    key_lookup_xt[GLFW_KEY_F7] = 0x41;
    key_lookup_xt[GLFW_KEY_F8] = 0x42;
    key_lookup_xt[GLFW_KEY_F9] = 0x43;
    key_lookup_xt[GLFW_KEY_F10] = 0x44;
    key_lookup_xt[GLFW_KEY_NUM_LOCK] = 0x45;
    key_lookup_xt[GLFW_KEY_SCROLL_LOCK] = 0x46;
    key_lookup_xt[GLFW_KEY_KP_7] = 0x47;
    key_lookup_xt[GLFW_KEY_KP_8] = 0x48;
    key_lookup_xt[GLFW_KEY_KP_9] = 0x49;
    key_lookup_xt[GLFW_KEY_KP_SUBTRACT] = 0x4A;
    key_lookup_xt[GLFW_KEY_KP_4] = 0x4B;
    key_lookup_xt[GLFW_KEY_KP_5] = 0x4C;
    key_lookup_xt[GLFW_KEY_KP_6] = 0x4D;
    key_lookup_xt[GLFW_KEY_KP_ADD] = 0x4E;
    key_lookup_xt[GLFW_KEY_KP_1] = 0x4F;
    key_lookup_xt[GLFW_KEY_KP_2] = 0x50;
    key_lookup_xt[GLFW_KEY_KP_3] = 0x51;
    key_lookup_xt[GLFW_KEY_KP_0] = 0x52;
    key_lookup_xt[GLFW_KEY_KP_DECIMAL] = 0x53;
}


namespace fs = std::filesystem;

std::vector<std::string> list_all_files(const fs::path& directory)
{
    std::vector<std::string> files;

    if (fs::exists(directory) && fs::is_directory(directory))
    {
        for (const auto& entry : fs::recursive_directory_iterator(directory))
        {
            if (fs::is_regular_file(entry.path()))
            {
                files.push_back(entry.path().string());
            }
        }
    }
    return files;
}

bool cursor_inited{};
double prev_mouse_x{};
double prev_mouse_y{};
void cursor_pos_callback([[maybe_unused]] GLFWwindow* window, double x, double y)
{
    if (!cursor_inited)
    {
        cursor_inited = true;
    }
    else
    {
        mac.p.busmouse.update_pos((x-prev_mouse_x)*1.0, (y-prev_mouse_y)*1.0);
    }
    prev_mouse_x = x;
    prev_mouse_y = y;
}

void mouse_button_callback([[maybe_unused]] GLFWwindow* window, int button, int action, [[maybe_unused]] int mods)
{
    if (button == GLFW_MOUSE_BUTTON_1)
        mac.p.busmouse.update_button(2, action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_2)
        mac.p.busmouse.update_button(0, action == GLFW_PRESS);
    //if (button == GLFW_MOUSE_BUTTON_3)
    //    mac.p.busmouse.update_button(0, action == GLFW_PRESS);


    /*if (button == GLFW_MOUSE_BUTTON_1)
        mac.p.gameport.set_button_state(0, action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_2)
        mac.p.gameport.set_button_state(1, action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_3)
        mac.p.gameport.set_button_state(2, action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_4)
        mac.p.gameport.set_button_state(3, action == GLFW_PRESS);*/
}

void key_callback([[maybe_unused]] GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods)
{
    if (action != GLFW_PRESS && action != GLFW_RELEASE)
        return;
    if (globalsettings.functionkeypress)
    {
        if (action == GLFW_PRESS)
        {
            if (false);
            else if (key == GLFW_KEY_S)
            {
                globalsettings.sound_on = !globalsettings.sound_on;
                cout << "Sound " << (globalsettings.sound_on?"on":"off") << endl;
            }
            else if (key == GLFW_KEY_M)
            {
                mac.p.hega.debugprint = !mac.p.hega.debugprint;
                mac.p.vga.debugprint = mac.p.hega.debugprint;
            }
            else if (key == GLFW_KEY_D)
            {
                globalsettings.entertrace = !globalsettings.entertrace;
                if (globalsettings.entertrace)
                    cout << "Tracer armed. Press enter to start trace." << endl;
                else
                    cout << "Tracer disarmed." << endl;
            }
            else if (key == GLFW_KEY_R) //reset
            {
                cout << "Reset!" << endl;
                mac.reset_cpu();
            }
            else if (key == GLFW_KEY_F)
            {
                cout << "Flushing disks." << endl;
                mac.p.harddisk.disks.disk[0].flush();
                mac.p.harddisk.disks.disk[1].flush();
                cout << "Disks flushed." << endl;
            }
            else if (key == GLFW_KEY_Q)
            {
                lockstep = true;
                cout << "Lockstep engaged." << endl;
            }
            else if (key == GLFW_KEY_W)
            {
                lockstep = false;
                cout << "Lockstep disengaged." << endl;
            }
            else if (key == GLFW_KEY_B)
            {
                globalsettings.ctrl_alt = true;
                cout << "Ctrl+Alt engaged." << endl;
            }
            else if (key == GLFW_KEY_V)
            {
                if (mac.p.cga.output == mac.p.cga.OUTPUT::RGB)
                    mac.p.cga.output = mac.p.cga.OUTPUT::COMPOSITE;
                else
                    mac.p.cga.output = mac.p.cga.OUTPUT::RGB;
                cout << "Cga output changed to " << (mac.p.cga.output==mac.p.cga.OUTPUT::RGB?"RGB.":"composite.") << endl;
            }
            else if (key == GLFW_KEY_L)
            {
                std::vector<std::string> files = list_all_files("disk/");
                int start_id=0;
                int chosen_id=-1;
                int chosen_drive=0;
                while(true)
                {
                    cout << "Load floppy to " << char('A'+chosen_drive) << ":" << endl;
                    for(int i=0; i<10; ++i)
                    {
                        if (start_id+i >= int(files.size()))
                            break;
                        cout << "[" << i << "] " << files[start_id+i] << endl;
                    }
                    cout << "[,] previous  ";
                    cout << "[.] next  ";
                    cout << "[-] eject  ";
                    cout << "[A/B] choose drive" << endl;
                    std::string n;
                    cin >> n;

                    if (n.empty())
                        continue;
                    if (n[0] == ',')
                    {
                        start_id -= 10;
                        if (start_id < 0)
                            start_id = 0;
                    }
                    else if (n[0] == '.')
                    {
                        start_id += 10;
                        if (start_id >= int(files.size()))
                            start_id -= 10;
                    }
                    else if (n[0] == '-')
                    {
                        break;
                    }
                    else if (n[0] >= '0' && n[0] <= '9')
                    {
                        chosen_id = start_id+(n[0]-'0');
                        break;
                    }
                    else if (n[0] == 'A' || n[0] == 'a')
                    {
                        chosen_drive = 0;
                    }
                    else if (n[0] == 'B' || n[0] == 'b')
                    {
                        chosen_drive = 1;
                    }
                }
                if (chosen_id != -1)
                {
                    cout << "Loading " << files[chosen_id] << " to " << char('A'+chosen_drive) << ":" << endl;
                    mac.p.diskettecontroller.drives[chosen_drive].diskette = DISKETTECONTROLLER::Drive::DISKETTE(files[chosen_id]);
                }
                else
                {
                    cout << "Ejecting " << char('A'+chosen_drive) << ":" << endl;
                    mac.p.diskettecontroller.drives[chosen_drive].diskette.eject();
                }
            }
        }
    }
    else
    {
        if (globalsettings.entertrace && key == GLFW_KEY_ENTER)
        {
            startprinting = true;
            turbo = false;
            globalsettings.entertrace = false;
            //mem.dump_memory("memory.raw");
        }
        u8 pc_scancode = key_lookup_xt[key];
        if (pc_scancode != 0)
        {
            /*if (globalsettings.machine == GlobalSettings::MACHINE_AT)
            {
                if (action == GLFW_RELEASE)
                    mac.p.kbd_at.press(0xF0);
                mac.p.kbd_at.press(pc_scancode);
            }*/
            if (globalsettings.machine == GlobalSettings::MACHINE_AT)
            {
                if (globalsettings.ctrl_alt)
                {
                    mac.p.kbd_at.press(key_lookup_xt[GLFW_KEY_LEFT_CONTROL] | (action == GLFW_RELEASE ? 0x80 : 0));
                    mac.p.kbd_at.press(key_lookup_xt[GLFW_KEY_LEFT_ALT] | (action == GLFW_RELEASE ? 0x80 : 0));
                    if (action == GLFW_RELEASE)
                        globalsettings.ctrl_alt = false;
                }
                mac.p.kbd_at.press(pc_scancode | (action == GLFW_RELEASE ? 0x80 : 0));
            }
            else
            {
                if (globalsettings.ctrl_alt)
                {
                    mac.p.kbd_xt.press(key_lookup_xt[GLFW_KEY_LEFT_CONTROL] | (action == GLFW_RELEASE ? 0x80 : 0));
                    mac.p.kbd_xt.press(key_lookup_xt[GLFW_KEY_LEFT_ALT] | (action == GLFW_RELEASE ? 0x80 : 0));
                    if (action == GLFW_RELEASE)
                        globalsettings.ctrl_alt = false;
                }
                mac.p.kbd_xt.press(pc_scancode | (action == GLFW_RELEASE ? 0x80 : 0));
            }
        }
    }


    if (key == GLFW_KEY_F11)
    {
        turbo = (action == GLFW_PRESS);
    }

    if (key == GLFW_KEY_F12)
    {
        globalsettings.functionkeypress = (action == GLFW_PRESS);
    }
}

vector<u8> readfile(const std::string& filename)
{
    std::vector<u8> buffer;
    std::ifstream file(filename, std::ios::binary);

    if (!file)
    {
        std::cerr << "Error: Unable to open file '" << filename << "'" << std::endl;
        return buffer;
    }

    file.seekg(0, std::ios::end);
    buffer.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    buffer.insert(buffer.begin(),
                  std::istreambuf_iterator<char>(file),
                  std::istreambuf_iterator<char>());

    if (!file)
    {
        std::cerr << "Error: Failed to read file '" << filename << "'" << std::endl;
        buffer.clear();
    }

    return buffer;
}

u64 tests_totalfailed{}, tests_totaldone{};

struct ROM
{
    u32 start_address{};
    std::vector<u8> data;
    u32 stride{1};

    //patches checksum in a chosen byte (or last byte if not chosen).
    void patch_checksum(int index=-1)
    {
        if (index == -1)
        {
            index = data.size()-1;
        }

        if (index < 0 || index >= data.size())
        {
            return;
        }

        u8 checksum{};
        for(int i=0; i<data.size(); ++i)
        {
            if (i != index)
                checksum += data[i];
        }
        data[index] = -checksum;
    }
};
std::vector<ROM> roms;

inline const std::vector<std::string> ExplodeCopy(std::string s, const char& c)
{
    std::vector<std::string> v;

    size_t start=0, len=0;//, i=0;
    for(auto n:s)
    {
        ++len;
        if(n == c)
        {
            v.push_back(s.substr(start, len-1));
            start += len;
            len = 0;
        }
    }
    v.push_back(s.substr(start));
    return v;
}

void configline(std::string line)
{
    if (line.empty())
        return;
    if (line[0] == '#')
        return;

    std::istringstream iss(line);
    std::string command;
    iss >> command;

    if (false);
    else if (command == "cpu")
    {
        std::string cputype;
        iss >> cputype;

        if (cputype == "8086" || cputype == "86")
            mac.init_cpu(0,6);
        else if (cputype == "8088" || cputype == "88")
            mac.init_cpu(0,4);
        else if (cputype == "80186" || cputype == "186")
            mac.init_cpu(2,6);
        else if (cputype == "80188" || cputype == "188")
            mac.init_cpu(2,4);
        else if (cputype == "8088mc" || cputype == "88mc")
            mac.init_cpu(1,4);
        else if (cputype == "80286" || cputype == "286")
            mac.init_cpu(3,-1);
        else
            std::cout << "ERROR unknown cpu: " << cputype << std::endl;
    }
    else if (command == "cpu_multiplier")
    {
        std::string fraction;
        iss >> fraction;
        size_t slashPos = fraction.find('/');
        if (slashPos == std::string::npos)
        {
            std::cout << "CPU mult: Invalid fraction format: missing '/'. don't use spaces." << std::endl;
        }
        else
        {
            std::string numeratorStr = fraction.substr(0, slashPos);
            std::string denominatorStr = fraction.substr(slashPos + 1);

            int numerator = std::stoi(numeratorStr);
            int denominator = std::stoi(denominatorStr);

            if (denominator == 0)
            {
                std::cout << "CPU multiplier: Invalid fraction: denominator cannot be zero" << std::endl;
            }
            else if (denominator < 0 || numerator < 0)
            {
                std::cout << "CPU multiplier: fraction cannot have negative numbers" << std::endl;
            }
            else
            {
                mac.cpumult_num = numerator;
                mac.cpumult_denom = denominator;
            }
        }
    }
    else if (command == "gpu")
    {
        std::string gputype;
        iss >> gputype;

        if (gputype == "cga" || gputype == "cga80" || gputype == "cga40")
        {
            globalsettings.graphics = GlobalSettings::GRAPHICS::CGA;
            if (gputype == "cga40")
                mac.p.kbd_xt.set_video_type(CHIP8255::CGA40);
            else
                mac.p.kbd_xt.set_video_type(CHIP8255::CGA80);
        }
        else if (gputype == "ega" || gputype == "hega")
        {
            globalsettings.graphics = GlobalSettings::GRAPHICS::HEGA;
            mac.p.kbd_xt.set_video_type(CHIP8255::V_OTHER);
        }
        else if (gputype == "vga")
        {
            globalsettings.graphics = GlobalSettings::GRAPHICS::VGA;
            mac.p.kbd_xt.set_video_type(CHIP8255::V_OTHER);
        }
        else
            std::cout << "ERROR unknown gpu: " << gputype << std::endl;
    }
    else if (command == "rom")
    {
        std::string address_str, filename;
        iss >> address_str >> filename;

        ROM rom;

        rom.stride = 1;
        while(true)
        {
            std::string option;
            iss >> option;
            if (option.substr(0,7) == "stride=")
            {
                rom.stride = atoi(option.substr(7).c_str());
            }
            if (iss.eof())
                break;
        }


        // Convert hex address to integer
        rom.start_address = std::stoul(address_str, nullptr, 16);

        // Read the ROM file
        std::ifstream file(filename, std::ios::binary);
        if (!file)
        {
            std::cerr << "Error: Unable to open ROM file: " << filename << std::endl;
            std::abort();
            return;
        }

        // Get file size
        file.seekg(0, std::ios::end);
        std::streampos fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        rom.data.assign(fileSize,0);

        // Read file contents into memory

        for(int i=0; i<fileSize; ++i)
        {
            file.read(reinterpret_cast<char*>(&rom.data[i]),1);
        }
        roms.push_back(rom);
    }
    else if (command == "load")
    {
        char drive_letter;
        std::string image_filename;
        iss >> drive_letter >> image_filename;

        int drive_number = tolower(drive_letter) - 'a';
        if (drive_number >= 0 && drive_number < 2)
        {
            mac.p.diskettecontroller.drives[drive_number].diskette = DISKETTECONTROLLER::Drive::DISKETTE(image_filename);
        }
        else if (drive_number >= 2 && drive_number < 4)
        {
            if (!file_exists(image_filename))
            {
                image_filename = "hd/" + image_filename;
            }

            cout << "Loading hard disk from " << image_filename << endl;

            int c = 375;
            int h = 8;
            int s = 17;
            while(true)
            {
                std::string option;
                iss >> option;
                if (option.substr(0,4) == "chs=")
                {
                    vector<string> chsvals = ExplodeCopy(option.substr(4), ',');
                    if (chsvals.size() == 3)
                    {
                        c = atoi(chsvals[0].c_str());
                        h = atoi(chsvals[1].c_str());
                        s = atoi(chsvals[2].c_str());
                    }
                }
                if (iss.eof())
                    break;
            }

            mac.p.disks.disk[drive_number-2] = DISK(image_filename, c, h, s);
        }
        else
        {
            std::cerr << "Error: Invalid drive letter: " << drive_letter << std::endl;
        }
    }
    else if (command == "machine")
    {
        string machine_type;
        iss >> machine_type;
        if (machine_type == "pc")
        {
            globalsettings.machine = globalsettings.MACHINE_PC;
        }
        else if (machine_type == "xt")
        {
            globalsettings.machine = globalsettings.MACHINE_XT;
        }
        else if (machine_type == "at")
        {
            globalsettings.machine = globalsettings.MACHINE_AT;
        }
        else
        {
            cout << "Unknown machine type. Supported machine types: pc,xt" << endl;
            std::abort();
        }
    }
    else if (command == "cmos_file")
    {
        std::string filename;
        iss >> filename;

        mac.p.cmos.filename = machineName + "/" + filename;

        mac.p.cmos.load();
        mac.p.cmos.update_time();
    }
    else if (command == "trace")
    {
        startprinting = true;
    }
    else if (command == "test")
    {
        //readonly_start = 0xFFFF0000;
        //TODO: make mem controller for test mode
        string test_filename;
        iss >> test_filename;

        cout << "Testing " << test_filename << endl;

        vector<u8> filedata = readfile(test_filename);
        u32 ptr = 0;

        auto data16 = [&]()->u16
        {
            u16 ret = *(u16*)(((u8*)(void*)filedata.data())+ptr);
            ptr += sizeof(u16);
            return ret;
        };
        auto data32 = [&]()->u32
        {
            u32 ret = *(u32*)(((u8*)(void*)filedata.data())+ptr);
            ptr += sizeof(u32);
            return ret;
        };


        u32 test_id = 0;
        u32 tests_failed = 0;

        u32 flags_failed[16] = {};
        u32 regs_failed[16] = {};
        u32 mem_failures{};
        while(ptr < filedata.size())
        {
            cycles = 0;
            //cout << std::dec << "---------------------------TEST #" << test_id << "---------------------------" << std::hex << std::endl;
            //startprinting=true;
            bool test_passed = true;
            CPU80286 testcpu(mac.p.mem286, mac.p.pic, mac.p.pic2, mac.p);
            //CPU8086 testcpu(mac.p.mem88, mac.p.pic, mac.p.pic2, mac.p);
            testcpu.mem.testmode = true;
            globalsettings.A20 = false;
            testcpu.reset();
            memset(mac.p.membytes.bytes, 0, (1<<20)+65536);

            u16 start_regs[14] = {};
            u16 final_regs[14] = {};

            for(int i=0; i<14; ++i)
            {
                start_regs[i] = data16();
                testcpu.registers[testcpu.registermap[i]] = start_regs[i];
            }

            u32 initial_ram_n = data32();
            for(u32 i=0; i<initial_ram_n; ++i)
            {
                u32 address = data32();
                u32 value = data32();
                mac.p.membytes.bytes[address] = value;
            }

            testcpu.load_tmp_segs_for_test();
            //testcpu.print_regs();
            do
            {
                testcpu.cycle();
            } while(testcpu.is_inside_multi_part_instruction || testcpu.delay > 0);
            do
            {
                testcpu.cycle();
            } while(testcpu.is_inside_multi_part_instruction || testcpu.delay > 0);

            //std::cout << "cycles:" << cycles << std::endl;
            testcpu.store_tmp_segs_for_test();
            //testcpu.print_regs();

            for(int i=0; i<14; ++i)
            {
                final_regs[i] = data16();
            }

            for(int i=0; i<14; ++i)
            {
                if (i==12)
                    continue;
                [[maybe_unused]] const char* const regnames[14] =
                {
                    "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI", "ES", "CS", "SS", "DS", "FL", "IP"
                };
                u16 test_reg = testcpu.registers[testcpu.registermap[i]];
                if (final_regs[i] != test_reg)
                {
                    //std::cout << "reg " << std::hex << regnames[i] << "=" << test_reg << " but supposed=" << final_regs[i] << std::endl;
                    test_passed = false;
                    if (i==12)
                    {
                        for(int flag=0; flag<16; ++flag)
                        {
                            flags_failed[flag] += bool(final_regs[i]&(1<<flag)) ^ bool(test_reg&(1<<flag));
                        }
                    }
                    regs_failed[i] += 1;
                }
                if ((test_reg^final_regs[i]))
                {
                    cout << test_filename << "#" << std::dec << test_id << std::hex <<  ": " << regnames[u32(i)] << ": " << start_regs[i] << "->" << final_regs[i] << " cpu gave " << test_reg << " , diff=" << (test_reg^final_regs[i]) << std::dec << endl;
                }
            }

            u32 final_ram_n = data32();
            for(u32 i=0; i<final_ram_n; ++i)
            {
                u32 address = data32();
                u32 value = data32();

                if (mac.p.membytes.bytes[address] != value)
                {
                    ++mem_failures;
                    test_passed = false;
                    //cout << test_filename << "#" << std::dec << test_id << ": Memory bytes at " << std::hex << address << " not correct: " << std::hex << u32(mac.p.membytes.bytes[address]) << ", should be " << u32(value) << std::dec << endl;
                }
            }

            if (!test_passed)
                tests_failed += 1, ++tests_totalfailed;

            ++test_id;
            ++tests_totaldone;

            //if (test_id == 1)
            //    std::abort();
        }

        if (tests_failed > 0)
        {
            cout << std::dec;
            cout << test_filename << ": " << tests_failed << " TESTS FAILED!" << endl;
            cout << "Reg failures:   ";
            for(int i=0; i<14; ++i)
                cout << regs_failed[i] << (i%4==3?"  ":" ");
            cout << endl;
            cout << "Flag failures:   ";
            for(int i=0; i<16; ++i)
                cout << flags_failed[i] << (i%4==3?"  ":" ");
            cout << endl;
            cout << "Mem failures:   " << std::dec << mem_failures << std::hex << std::endl;
        }
    }
    else if (command == "end_tests")
    {
        cout << "tests failed: " << std::dec << tests_totalfailed << "/" << tests_totaldone << endl;
        std::abort();
    }
    else if (command == "sound")
    {
        string setting;
        iss >> setting;

        if (setting == "yes"|| setting=="on"|| setting=="true")
            globalsettings.sound_on = true;
        else if (setting == "no"|| setting=="off"|| setting=="false")
            globalsettings.sound_on = false;
        else
        {
            cout << "Unknown " << command << " setting: " << setting << endl;
            std::abort();
        }
    }
    else
    {
        std::cerr << "Error: Unknown command: " << command << std::endl;
    }
}

void readConfigFile(const std::string& filename)
{
    std::ifstream configFile(filename);
    if (!configFile.is_open())
    {
        std::cerr << "Error: Could not open config file " << filename << std::endl;
        std::abort();
    }

    std::string line;
    while (std::getline(configFile, line))
    {
        configline(line);
    }

    //patch xebec :christ:
    const u8 xebecHeader[16] = { 0x55, 0xAA, 0x10, 0xEB, 0x1E, 0x35, 0x30, 0x30, 0x30, 0x30, 0x35, 0x39, 0x20, 0x28, 0x43, 0x29 };
    for(auto& rom: roms)
    {
        if (rom.data.size() >= 16 && memcmp(xebecHeader, rom.data.data(), 16) == 0)
        {
            std::cout << "xebec found" << std::endl;

            const u32 chs_data_offset = 0x3E7;

            for(int disk_i=0; disk_i<2; ++disk_i)
            {
                u16 c = mac.p.disks.disk[disk_i].type.cylinders;
                u16 h = mac.p.disks.disk[disk_i].type.heads;
                //u16 s = mac.p.disks.disk[disk_i].type.sectors; //ignored until i have xebec v3

                const u32 type_offset = chs_data_offset + 16*disk_i;

                rom.data[type_offset] = (c&0xFF);
                rom.data[type_offset+1] = (c>>8);

                rom.data[type_offset+2] = (h&0xFF);
                rom.data[type_offset+3] = (h>>8);

                rom.data[type_offset+4] = (c&0xFF);
                rom.data[type_offset+5] = (c>>8);
            }
            rom.patch_checksum();
        }
    }

    for(auto& rom: roms)
    {
        std::cout << "rom start=" << rom.start_address << ", stride=" << rom.stride << ", datasize=" << rom.data.size() << std::endl;

        for(int i=0; i<rom.data.size(); ++i)
            mac.p.membytes.bytes[rom.start_address+i*rom.stride] = rom.data[i];
    }
}

enum struct JOYSTICK
{
    NONE,
    JOYSTICK,
    GAMEPAD
};

enum struct InputEventSTATE
{
    UP,
    DOWN,
    N
};

void gamepadbuttonfun([[maybe_unused]] int joy_id, int key_id, InputEventSTATE action)
{
    if (key_id >= 0 && key_id <= 3)
    {
        mac.p.gameport.set_button_state(key_id, action == InputEventSTATE::DOWN);
    }
}
void gamepadaxisfun([[maybe_unused]] int joy_id, int axis_id, int amount)
{
    if (axis_id >= 0 && axis_id <= 3)
    {
        mac.p.gameport.axes[axis_id] = amount;
    }
    //InputEvent ie;
    //ie.type = InputEvent::TYPE::GAMEPAD_AXISMOVE;
    //ie.data = axis_id;
    //ie.data2 = amount;
    //GetEngine().input.inputqueue.push(ie);
}

const u32 JOYSTICKS_SIZE = 16;
JOYSTICK joysticks[JOYSTICKS_SIZE] = {};
const float DEADZONE = 0.04f;
int gp_buttonstate[GLFW_GAMEPAD_BUTTON_LAST+1] = {};
int gp_axisstate[GLFW_GAMEPAD_AXIS_LAST+1] = {};
int n_joysticks = 0;

void joystickfun(int jid, int event)
{
    if (event == GLFW_CONNECTED)
    {
        if (glfwJoystickIsGamepad(jid))
        {
            joysticks[jid] = JOYSTICK::GAMEPAD;
            std::string gamepadname;
            //avoid assigning possible nullptr
            if (auto name = glfwGetGamepadName(jid); name!=nullptr)
                gamepadname = name;
            cout << "Gamepad found! Name: " << gamepadname << endl;
        }
        else
            joysticks[jid] = JOYSTICK::JOYSTICK;
    }
    else if (event == GLFW_DISCONNECTED)
    {
        joysticks[jid] = JOYSTICK::NONE;
        // The joystick was disconnected

        //Xr::SetToZero(gp_buttonstate);
        //Xr::SetToZero(gp_axisstate);
        /*if (GetEngine().appstate.state == "INGAME"_sm)
            GetEngine().appstate.state = "INGAME_PAUSE"_sm;
        if (GetEngine().appstate.gamepad_id == jid)
            GetEngine().appstate.gamepad_id = -1;*/
    }

    n_joysticks = 0;
    for(int jid_counter=GLFW_JOYSTICK_1; jid_counter<GLFW_JOYSTICK_LAST; ++jid_counter)
    {
        if (glfwJoystickPresent(jid_counter))
        {
            ++n_joysticks;
        }
    }
}
void updatejoysticks()
{
    for(int j_id=0; j_id<JOYSTICKS_SIZE; ++j_id)
    {
         if (joysticks[j_id] != JOYSTICK::GAMEPAD)
            continue;
        GLFWgamepadstate state;
        if (!glfwGetGamepadState(j_id, &state))
            continue;
        for(int b_id=0; b_id<=GLFW_GAMEPAD_BUTTON_LAST; ++b_id)
        {
            int b_new = state.buttons[b_id];
            if (gp_buttonstate[b_id] != b_new)
            {
                gp_buttonstate[b_id] = b_new;
                gamepadbuttonfun(j_id, b_id, b_new?InputEventSTATE::DOWN:InputEventSTATE::UP);
            }
        }

        for(int a_id=0; a_id<=GLFW_GAMEPAD_AXIS_LAST; ++a_id)
        {
            int a_new = int(state.axes[a_id]*32767.0f);
            if (abs(a_new) > int(DEADZONE*32767.0f))
            {
                gp_axisstate[a_id] = a_new;
                gamepadaxisfun(j_id, a_id, a_new);
            }
            else
            {
                gp_axisstate[a_id] = 0;
                gamepadaxisfun(j_id, a_id, 0);
            }
        }
    }
}
#include "synchapi.h"
void run_gfx()
{
    double previousTime=glfwGetTime();
    u64 loop_counter=0, clockgen_real=0;
    while(true)
    {
        //the loop is ca. ~14.31818 MHz
        //++loop_counter;
        Sleep(1);

        //if ((loop_counter&0x1F) == 0) //calculate how many cycles we need to do
        {
            double newTime = glfwGetTime();
            if (newTime-previousTime >= 0.1)
                previousTime = newTime-0.1;
            u64 cycles_done = (newTime-previousTime)*(14318180.0);
            for(u64 i=0; i<cycles_done; ++i)
            {
                ++clockgen_real;
                mac.gfx_stuff(clockgen_real);
            }
            previousTime += double(cycles_done)/14318180.0;
        }
    }
}

void run_emu()
{
    double previousTime=0.0;
    u64 loop_counter=0, clockgen_fast=0, clockgen_real=0;
    while(true)
    {
        //the loop is ca. ~14.31818 MHz
        ++loop_counter;

        if (!lockstep || turbo)
        {
            ++clockgen_fast;

            mac.fast_stuff(clockgen_fast);

            //realtime stuff
            if (lockstep) //implies turbo==true
            {
                mac.real_stuff(clockgen_fast);
            }
            if (!lockstep && turbo)
            {
                mac.real_stuff(clockgen_fast);
                mac.real_stuff(clockgen_fast);
                mac.real_stuff(clockgen_fast);
            }
        }

        //do realtime stuff
        if (!turbo && (loop_counter&0x3F) == 0) //calculate how many cycles we need to do
        {
            double newTime = glfwGetTime();
            if (newTime-previousTime >= 0.1)
                previousTime = newTime-0.1;
            u64 cycles_done = (newTime-previousTime)*(14318180.0);
            for(u64 i=0; i<cycles_done; ++i)
            {
                ++clockgen_real;
                //fast stuff
                if (lockstep)
                {
                    mac.fast_stuff(clockgen_real);
                }
                //realtime stuff
                mac.real_stuff(clockgen_real);
            }
            previousTime += double(cycles_done)/14318180.0;
        }
        /*if ((loop_counter&0xFFFF) == 0)
        {
            if ((loop_counter&0x3FFFF) == 0)
            {
                glfwPollEvents();
                updatejoysticks();
            }
            if (!lockstep || turbo)
            {
                previousTime = glfwGetTime();
            }
            if (glfwGetTime()-startTime >= 1.0)
            {
                startTime += 1.0;
                cout << mac.cpu_steps*3/14318180.0 << "x realtime ";
                cout << std::dec << mac.cpu_steps/1000000.0 << std::hex << " MHz ";
                //cout << std::dec << totalframes << " Hz audio " << std::hex;
                cout << std::dec << mac.p.cga.totalvsync << " Hz vsync, " << std::hex;
                //cout << std::hex << "flags=" << cpu.registers[cpu.FLAGS] << " " << std::hex;
                //cout << "halt=" << cpu.halt << " ";

                cout << endl;

                mac.p.disks.disk[0].flush();
                mac.p.disks.disk[1].flush();
                mac.cpu_steps = 0;
                totalframes = 0;
                mac.p.cga.totalvsync = 0;
                mac.p.pit.int0_count = 0;

                for(int i=0; i<256; ++i)
                {
                    if (cpu.interrupt_table[i] != 0)
                    {
                        cout << std::hex << "int" << i << "=" << std::dec << cpu.interrupt_table[i] << std::hex << "Hz ";
                        cpu.interrupt_table[i] = 0;
                    }
                }
                cout << endl;
            //}
        //}*/

    }
}

int main(int argc, char* argv[])
{
    FILE* filu = fopen("rom/8088mc.bin","rb");
    if (filu != NULL)
    {
        fread(mc8088, 4, 0x200, filu);
        fclose(filu);
        cout << "8088 microcode loaded!" << endl;
    }
    else
    {
        cout << "8088 microcode not found. put it in rom/8088mc.bin" << std::endl;
    }


    //glfwInit();
    mac.init_cpu(1,4);

    std::string configFilename = "config.txt";
    if (argc > 1)
    {
        machineName = std::string("machines/") + argv[1];
        configFilename = machineName+"/"+configFilename;
    }

    mac.p.membytes.set_size((2)<<20);
    readConfigFile(configFilename);
    mac.p.mem286.register_devs();
    initialize_key_lookup();
    screen.SCREEN_start();
    sampleplayer.load_sample("sounds/seek.raw");

    Opl2::Init();

    startTime = glfwGetTime();

    std::cout << std::uppercase << std::hex;

    glfwSetKeyCallback(screen.window, key_callback);
    glfwSetCursorPosCallback(screen.window, cursor_pos_callback);
    glfwSetMouseButtonCallback(screen.window, mouse_button_callback);
    for(int jid=GLFW_JOYSTICK_1; jid<GLFW_JOYSTICK_LAST; ++jid)
    {
        if (glfwJoystickPresent(jid))
        {
            joystickfun(jid, GLFW_CONNECTED);
        }
    }
    glfwSetJoystickCallback(joystickfun);
    mac.reset_cpu();

    std::thread emu_thread(run_emu);
    std::thread gfx_thread(run_gfx);

    while(true)
    {
        glfwPollEvents();
        updatejoysticks();
        screen.render();
        glfwWaitEventsTimeout(0.01);
        if (mac.p.cmos.changed)
            mac.p.cmos.save();
        //last_render = now;
        //screen.clear();
        if (glfwGetTime()-startTime >= 1.0)
        {
            startTime += 1.0;
            if (mac.p.hega.frames > 0)
            {
                std::cout << std::dec << mac.p.hega.frames << " FPS (HEGA)" << std::endl;
                mac.p.hega.frames = 0;
            }
            if (mac.p.vga.frames > 0)
            {
                std::stringstream ss;

                ss << "V=" << std::dec << mac.p.vga.frames << "Hz, H=" << mac.p.vga.hframes << "Hz (VGA)" << std::endl;
                std::cout << ss.str();
                mac.p.vga.frames = 0;
                mac.p.vga.hframes = 0;
            }
        }
    }


    emu_thread.join();
    gfx_thread.join();

    Opl2::Quit();
    glfwTerminate();
}
