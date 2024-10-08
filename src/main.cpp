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

#include "cgabios.h" //cga character ROM

struct GlobalSettings
{
    bool functionkeypress{false};
    bool sound_on{true};
    bool entertrace{false};

    enum MACHINE
    {
        MACHINE_PC,
        MACHINE_XT,

        MACHINE_COUNT
    } machine=MACHINE_PC;
} globalsettings{};

const u32 DEBUG_LEVEL = 0;


const u32 PRINT_START = 0;
u64 cycles{};
u32 readonly_start = 0xF0000;

const char* r8_names[8] = {"AL","CL","DL","BL","AH","CH","DH","BH"};
const char* r16_names[8] = {"AX","CX","DX","BX","SP","BP","SI","DI"};
const char* seg_names[8] = {"ES", "CS", "SS", "DS", "(invalid segment register #4)", "(invalid segment register #5)", "(invalid segment register #6)", "(invalid segment register #7)"};

const char* r_fullnames[14] =
{
    "AX","CX","DX","BX","SP","BP","SI","DI",
    "ES", "CS", "SS", "DS",
    "FLAGS","IP"
};

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

#include "screen.h"

struct YM3812
{
    Opl2 opl2;

    u8 current_register{};
    u8 status{};

    u8 timer1{};
    u8 timer2{};

    u8 counter{};

    i16 sample{};
    i16 previous_sample{};

    void write(u8 port, u8 data) //port from 0 to 1! inclusive.
    {
        //cout << "YM3812 WRITE! " << u32(port) << ":" << u32(data) << endl;
        if (port == 0x00)
        {
            current_register = data;
        }
        else if (port == 0x01)
        {
            if (current_register == 0x02)
            {
                timer1 = data;
            }
            else if (current_register == 0x03)
            {
                timer2 = data;
            }
            else if (current_register == 0x04) //reset IRQ
            {
                if (data&0x80)
                {
                    status = 0;
                }
            }
            else
            {
                opl2.write(current_register,data);
            }
        }
    }

    u8 read(u8 port) //port from 0 to 1! inclusive.
    {
        u8 ret{};
        if (port == 0)
        {
            ret = status;
        }
        //cout << "YM3812 READ!" << u32(port) << ":" << u32(ret) << endl;
        return ret;
    }

    void cycle_timers() //at circa 49715 hz
    {
        ++counter;
        if(timer1 != 0  && (counter&0x03) == 0) //fast timer
        {
            ++timer1;
            if (timer1==0)
            {
                status |= 0xC0;
                cout << "YM3812: TIMER 1 expired!" << endl;
            }
        }
        if(timer2 != 0  && (counter&0x0F) == 0) //slow timer
        {
            ++timer2;
            if (timer2==0)
            {
                status |= 0xA0;
                cout << "YM3812: TIMER 2 expired!" << endl;
            }
        }
    }
    void cycle() //at circa 49715 hz in real time!
    {
        previous_sample = sample;
        sample = opl2.update();
    }
} ym3812;

const u32 GAMEPORT_CYCLE = 64;

struct Gameport
{
    u8 reg{0xF0};
    /*
    bit 7: button 4 off
    bit 6: button 3 off
    bit 5: button 2 off
    bit 4: button 1 off

    bit 3: #2 axis y state
    bit 2: #2 axis x state
    bit 1: #1 axis y state
    bit 0: #1 axis x state
    */

    i16 axes[400] = {};
    u32 counters[400] = {};

    u32 axis_to_counter(i16 axis_value)
    {
        //u32 resistor = u32((u64(axis_value+32768)*100000ULL)>>16); //ohms
        u32 cycles = u32((i32(axis_value)+32768)>>2) + 384; //cycles
        //cout << std::dec << axis_value << " -> " << cycles/(14.318180) << "us" << std::hex << endl;
        return u32(cycles);
    }

    void set_button_state(u8 button, bool is_on)
    {
        reg = (reg&~(0x10<<button)) | (is_on?0:(0x10<<button));
    }

    void write(u8 port, u8 data) //port from 0 to 0! inclusive.
    {
        reg |= 0x0F;
        for(int axis=0; axis<4; ++axis)
            counters[axis] = axis_to_counter(axes[axis]);
    }

    u8 read(u8 port) //port from 0 to 0! inclusive.
    {
        return reg;
    }

    void cycle() //called at 14318180/GAMEPORT_CYCLE Hz
    {
        reg &= 0xF0;

        for (int i = 0; i < 4; ++i)
        {
            counters[i] -= (counters[i]>=GAMEPORT_CYCLE ? GAMEPORT_CYCLE : 0);
            reg |= (counters[i]>=GAMEPORT_CYCLE ? (1 << i): 0);
        }
    }
} gameport;

struct CGA
{
    static const u8 REGISTER_COUNT = 18;
    static const u8 COLORBURST_START = 240;
    u8 registers[REGISTER_COUNT] = {};
    u8 current_register{};
    u8 mode_select{};
    u8 color_select{};

    enum struct OUTPUT
    {
        RGB,
        COMPOSITE
    } output{OUTPUT::RGB};

    bool snow{false};
    bool snow_enabled{false};

    u8 mem[0x4000 + 1] = {};
    u8& memory8(u16 address)
    {
        snow = snow_enabled;
        return mem[address&0x3FFF];
    }
    u16& memory16(u16 address)
    {
        snow = snow_enabled;
        return *(u16*)(void*)(mem+(address&0x3FFF));
    }
    u8 memory8_internal(u16 address)
    {
        if (snow)
            return 0xFF;
        return mem[address&0x3FFF];
    }
    u16 memory16_internal(u16 address)
    {
        if (snow)
            return 0xFF;
        return *(u16*)(void*)(mem+(address&0x3FFF));
    }
    void print_regs()
    {
        for(int i=0; i<16; ++i)
            cout << u32(registers[i]) << (i%4==3?"  ":" ");
        cout << endl;
    }

    struct CompositeColor
    {
        float getlevel(u8 color, u8 pos)
        {
            //black  00000000
            //blue   00011110
            //green  11000011
            //cyan   10000111
            //red    01111000
            //mgnt   00111100
            //yellow 11100001
            //white  11111111

            //       00011110
            //       22211112

            //       21111222
            //       11222211

            //       BAA--AAB
            //       aa-AAA-a

            //const float curve[8] = {0,0,0,0,1,1,1,1};
            //const u8 add[8] = {0,1,1,1,1,1,1,0};
            const u8 start[8] = {0,1,6,7,3,2,5,4};
            //return curve[(start[color]+add[color]*pos)&7];

            const u8 mask[8] = {0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0};

            return ((start[color]+(mask[color]&pos))&4)?1.0f:0.0f;
        }

        u8 curr_idx[4] = {};
        bool intense[4] = {false, false, false, false};
        void Clear()
        {
            for(int i=0; i<4; ++i)
            {
                curr_idx[i] = 0;
                intense[i] = false;
            }
        }

        /*void Set(u8 position, u8 color_index)
        {
            curr_idx[position] = color_index&7;
            intense[position] = color_index&8;
        }*/
        float num[8] = {};

        u32 Get(u8 position, u8 color_index) //return type AABBGGRR
        {
            //Set(position, color_index);
            num[position*2+0] = getlevel(color_index&7,position*2+0)+(color_index&8?2.2f/5.6f:0.0f);
            num[position*2+1] = getlevel(color_index&7,position*2+1)+(color_index&8?2.2f/5.6f:0.0f);

            const float phase[8] =
            {
                //0, 1, 1, 0, 0, -1, -1, 0, //works worse
                1,1,1,1,-1,-1,-1,-1 //works better
            };

            float fy{}, fi{}, fq{};
            for(int i=0; i<8; ++i)
            {
                fy += num[i];
                fi += num[i]*phase[i];
                fq += num[i]*phase[(i+6)&7];
            }
            const float yiq2rgb[9] =
            {
                1.0f/12.0f, 0.5694/12.0f/0.5957f, 0.3234/12.0f/0.5226f,
                1.0f/12.0f, -0.1620/12.0f/0.5957f, -0.3381/12.0f/0.5226f,
                1.0f/12.0f, -0.6588/12.0f/0.5957f, 0.8900/12.0f/0.5226f,
            };
            float fr = fy*yiq2rgb[0] + fi*yiq2rgb[1] + fq*yiq2rgb[2];
            float fg = fy*yiq2rgb[3] + fi*yiq2rgb[4] + fq*yiq2rgb[5];
            float fb = fy*yiq2rgb[6] + fi*yiq2rgb[7] + fq*yiq2rgb[8];

            fr = std::min(std::max(fr,0.0f),1.0f);
            fg = std::min(std::max(fg,0.0f),1.0f);
            fb = std::min(std::max(fb,0.0f),1.0f);

            u8 r = fr*255.0f;
            u8 g = fg*255.0f;
            u8 b = fb*255.0f;

            return 0xFF000000+(b<<16)+(g<<8)+r;
        }
    } compositecolor;

    enum REGISTER
    {
        H_TOTAL,
        H_DISPLAYED,
        H_SYNC_POS,
        H_SYNC_WIDTH,

        V_TOTAL,
        V_TOTAL_ADJUST,
        V_DISPLAYED,
        V_SYNC_POS,

        INTERLACE,
        MAX_SCAN_LINE, //not "scanline" but "scan line" as per ibm's manual :-)
        CURSOR_START,
        CURSOR_END,

        START_ADDRESS_H,
        START_ADDRESS_L,
        LIGHT_PEN_H,
        LIGHT_PEN_L,
    };

    u8 horizontal_retrace{};
    u8 vertical_retrace{};
    bool retrace{};
    u16 current_startaddress{};
    u8 vcc{};

    u32 totalvsync{};

    double last_render{};

    void render()
    {
        double now = glfwGetTime();
        if (now-last_render > 0.01)
        {
            screen.render();
            last_render = now;
            screen.clear();
        }
    }

    u8 read(u8 port) //port from 0 to 15! inclusive
    {
        if constexpr (DEBUG_LEVEL > 0)
            cout << "CGA READ! " << u32(port) << endl;
        u8 readdata{};
        if (port == 0x05)
        {
            if (current_register < 0x12)
            {
                readdata = registers[current_register];
            }
            else
            {
                cout << "CGA current register bad: " << u32(current_register) << endl;
                std::abort();
            }
        }
        else if (port == 0x0A)
        {
            //bit 0 = we are in vert. or horiz. retrace
            readdata |= retrace;
            //bit 1 = light pen triggered (vs 0 = armed)
            //bit 2 = light pen switch open (vs 0 = closed)
            //bit 3 = vertical sync pulse!
            readdata |= (vertical_retrace<<3);
        }

        return readdata;
    }

    void write(u8 port, u8 data) //port from 0 to 15! inclusive.
    {
        if (port == 0x04)
        {
            current_register = data;
        }
        else if (port == 0x05)
        {
            if (current_register < 0x10)
            {
                registers[current_register] = data;
                //if (current_register != 0x0E && current_register != 0x0F)
                //      cout << "CGA " << u32(current_register) << "=" << u32(data) << " " << std::dec << column << ":" << line << "(" << logical_line << ")" << std::hex << "  ", print_regs();
            }
            else
            {
                cout << "CGA current register bad: " << u32(current_register) << endl;
                std::abort();
            }
        }
        else if (port == 0x08) //mode select register
        {
            mode_select = data;
        }
        else if (port == 0x09) //color select register (UWAGA!! documentation had a mistake here, said port is 8 but it is 9)
        {
            color_select = data;
        }
        if constexpr(DEBUG_LEVEL > 1)
        {
            for(int i=0; i<88; ++i)
                cout << i << "=" << u32(registers[i]) << ", ";
            cout << endl;
        }
    }

    u64 total_frames{};

    void HSBtoRGB(u8 hue, u8 saturation, u8 brightness, u8& red, u8& green, u8& blue)
    {
        float h = hue / 256.0f * 6.0f;
        float s = saturation / 255.0f;
        float v = brightness / 255.0f;

        int i = (hue*6)>>8;
        float f = h - i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - f * s);
        float t = v * (1.0f - (1.0f - f) * s);

        float r, g, b;
        switch (i)
        {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
        }

        r = min(r,1.0f);
        g = min(g,1.0f);
        b = min(b,1.0f);

        red = static_cast<u8>(r * 255.0f);
        green = static_cast<u8>(g * 255.0f);
        blue = static_cast<u8>(b * 255.0f);
    }

    u32 column{};
    u32 logical_line{};
    u32 scan_line{};
    u32 scan_column{};
    u32 line_inside_character{};
    u32 vsyncadjust{};
    bool hsync{}, vsync{};
    void cycle()
    {
        u8 textmode_40_80 = (mode_select>>0)&0x01;
        u8 is_graphics_mode = ((mode_select>>1)&0x01);
        u16 hsync_mult = 16;

        if (textmode_40_80)
        {
            hsync_mult = 8;
        }

        column += 8;
        column = (column>=(registers[H_TOTAL]+1)*hsync_mult?0:column);

        scan_column += 8;
        scan_column = (scan_column>=912?0:scan_column);

        if (column == 0) //new line
        {
            ++line_inside_character;
            if (line_inside_character > registers[MAX_SCAN_LINE])
            {
                line_inside_character = 0;
                ++logical_line;
                //cout << std::dec << physical_line << "-" << logical_line <<std::hex << endl;
            }
            bool all_lines_drawn = (logical_line > registers[V_TOTAL]);
            if (all_lines_drawn)
                ++vsyncadjust;
            else
                vsyncadjust = 0;

            if (all_lines_drawn && vsyncadjust > registers[V_TOTAL_ADJUST])
            {
                vsyncadjust = 0;
                logical_line = 0;
                line_inside_character = 0;
            }

            if (logical_line == 0 && line_inside_character == 0)
            {
                current_startaddress = ((registers[START_ADDRESS_H]<<8) | registers[START_ADDRESS_L])*2;
            }
        }

        bool old_vsync = vsync;
        vsync = (logical_line >= registers[V_SYNC_POS] && logical_line <= registers[V_SYNC_POS]+2);

        u16 hsync_start = (registers[H_SYNC_POS])*hsync_mult;
        u16 hsync_end = (registers[H_SYNC_POS]+registers[H_SYNC_WIDTH])*hsync_mult;

        bool old_hsync = hsync;

        hsync = (column >= hsync_start && column <= hsync_end);

        //if (old_hsync && !hsync && (scan_column == 0 || scan_column >= 656))
        if (hsync && (scan_column == 0 || scan_column >= 800))
        {
            scan_column = 0;
        }
        if (scan_column == 0)
        {
            scan_line += (scan_line>=261?-261:1);
        }
        if (!old_vsync && vsync)
        {
            render();
            ++totalvsync;
            scan_line = 0;
            scan_column = 0;
        }

        vertical_retrace = (logical_line >= registers[V_DISPLAYED]);
        horizontal_retrace = (column >= (registers[H_DISPLAYED])*hsync_mult);

        u8 no_colorburst = (mode_select>>2)&0x01;
        u8 resolution = (mode_select>>4)&0x01;
        bool output_enabled = (mode_select&0x08);

        const u8 add = ((color_select&0x10)?8:0) + ((color_select&0x20)?1:0);
        const u8 palette[4] = {u8(color_select&0x0F), u8(2+add), u8(4+(no_colorburst?0:add)), u8(6+add)};

        retrace = (vertical_retrace|horizontal_retrace);
        bool draw_bg = retrace|!output_enabled;

        if (output == OUTPUT::RGB)
        {
            if (is_graphics_mode && !textmode_40_80)
            {
                int x = column>>3;
                u32 offset = current_startaddress + (scan_line&1?0x2000:0) + logical_line*registers[H_DISPLAYED]*2+x;
                u8 gfx_byte = memory8_internal(offset);

                for(int i=0; i<8; i+=2)
                {
                    u8 p1 = (resolution?((gfx_byte&0x80)?palette[0]:0):palette[(gfx_byte&0xC0)>>6]);
                    u8 p2 = (resolution?((gfx_byte&0x40)?palette[0]:0):p1);

                    if (draw_bg)
                    {
                        if (!resolution)
                            p1 = palette[0], p2 = palette[0];
                        else
                            p1 = 0, p2 = 0;
                    }
                    if (vsync|hsync)
                        p1 = 0, p2 = 0;

                    screen.pixels[scan_line*screen.X + scan_column + i] = getpalette(p1);
                    screen.pixels[scan_line*screen.X + scan_column + i+1] = getpalette(p2);
                    gfx_byte <<= 2;
                }
            }
            else
            {
                int x = column>>(textmode_40_80?3:4);
                bool half = (textmode_40_80?0:(column&8));
                u32 offset = current_startaddress + logical_line*registers[H_DISPLAYED]*2 + x*2;
                u8 char_code = memory8_internal(offset);
                u8 attribute = memory8_internal(offset+1);
                u8 fg_color = attribute & 0x0F;
                u8 bg_color = (attribute >> 4) & 0x0F;
                u8 char_row = CGABIOS[(char_code<<3)+line_inside_character];

                for (u32 x_off = 0; x_off < 8; x_off++)
                {
                    u8 mask = (1 << ((half?3:7) - (x_off>>(textmode_40_80?0:1))));
                    u8 color = (char_row & mask) ? fg_color : bg_color;
                    if (draw_bg || is_graphics_mode)
                        color = palette[0];
                    if (vsync|hsync)
                        color = 0;

                    screen.pixels[scan_line * screen.X + scan_column + x_off] = getpalette(color); //screen.pixels is four bytes per pixel
                }
            }
        }
        else if (output == OUTPUT::COMPOSITE)
        {
            if (is_graphics_mode && !textmode_40_80)
            {
                int x = column>>3;
                u32 offset = current_startaddress + (scan_line&1?0x2000:0) + logical_line*registers[H_DISPLAYED]*2+x;
                u8 gfx_byte = memory8_internal(offset);

                for(int i=0; i<8; i+=2)
                {
                    u8 p1 = (resolution?((gfx_byte&0x80)?palette[0]:0):palette[(gfx_byte&0xC0)>>6]);
                    u8 p2 = (resolution?((gfx_byte&0x40)?palette[0]:0):p1);

                    if (draw_bg)
                    {
                        if (!resolution)
                            p1 = palette[0], p2 = palette[0];
                        else
                            p1 = 0, p2 = 0;
                    }
                    if (vsync|hsync)
                        p1 = 0, p2 = 0;

                    screen.pixels[scan_line*screen.X + scan_column + i] = compositecolor.Get((i)&0x03, p1);
                    screen.pixels[scan_line*screen.X + scan_column + i+1] = compositecolor.Get((i+1)&0x03, p2);
                    gfx_byte <<= 2;
                }
            }
            else
            {
                int x = column>>(textmode_40_80?3:4);
                bool half = (textmode_40_80?0:(column&8));
                u32 offset = current_startaddress + logical_line*registers[H_DISPLAYED]*2 + x*2;
                u8 char_code = memory8_internal(offset);
                u8 attribute = memory8_internal(offset+1);
                u8 fg_color = attribute & 0x0F;
                u8 bg_color = (attribute >> 4) & 0x0F;
                u8 char_row = CGABIOS[(char_code<<3)+line_inside_character];

                for (u32 x_off = 0; x_off < 8; x_off++)
                {
                    u8 mask = (1 << ((half?3:7) - (x_off>>(textmode_40_80?0:1))));
                    u8 color = (char_row & mask) ? fg_color : bg_color;
                    if (draw_bg || is_graphics_mode)
                        color = palette[0];
                    if (vsync|hsync)
                        color = 0;

                    screen.pixels[scan_line * screen.X + scan_column + x_off] = compositecolor.Get(x_off&0x03, color);
                }
            }
        }

        snow = false;
    }
} cga;

struct LTEMS
{
    u8 memory[4*1024*1024+1] = {};

    u32 pages[4] = {};

    void write(u8 port, u8 data) // port from 0 to 3 inclusive
    {
        pages[port] = u32(data)*0x4000U;
    }
    u8 read(u8 port) // no port is readable
    {
        return 0;
    }

    u8& _8(u16 index)
    {
        u16 page = (index>>14);
        u16 address = (index&0x3FFFU);
        return memory[pages[page]+address];
    }
    u16& _16(u16 index)
    {
        u16 page = (index>>14);
        u16 address = (index&0x3FFFU);
        return *(u16*)(void*)(memory+(pages[page]+address));
    }
} ltems;

/*struct MemoryManager
{
    u8 memory_bytes[(1<<20)+1] = {};
    void dump_memory(const char* filename)
    {
        FILE* filu = fopen(filename, "wb");
        fwrite(memory_bytes, 0x100000, 1, filu);
        fclose(filu);
    }

    u16 readonly_words[256] = {};
    u8 readonly_word{};
    u8 readonly_bytes[256] = {};
    u8 readonly_byte{};

    u16 rw_words[256] = {};
    u8 rw_word{};

    bool cga_used{};

    u8& _8(u16 segment, u16 index)
    {
        u32 total_address = (((segment<<4)+index)&0xFFFFF);
        if (total_address >= readonly_start)
        {
            ++readonly_byte;
            readonly_bytes[readonly_byte] = memory_bytes[total_address];
            return readonly_bytes[readonly_byte];
        }

        if (readonly_start < 0x100000 && (total_address&0xF0000) == 0xE0000)
            return ltems._8(total_address&0xFFFF);
        if (readonly_start < 0x100000 && (total_address&0xF8000) == 0xB8000)
        {
            cga_used = true;
            return cga.memory8(total_address&0x7FFF);
        }
       return memory_bytes[total_address];
    }
    u16& _16(u16 segment, u16 index)
    {
        if (index == 0xFFFF)
        {
            //cout << segment << ":FFFF goes BRRRRRRRRRRRRRRRRRRRRRRRRR" << endl;
        }
        u32 total_address = (((segment<<4)+index)&0xFFFFF);
        if (total_address >= readonly_start)
        {
            ++readonly_word;
            readonly_words[readonly_word] = *(u16*)(void*)(memory_bytes+total_address);
            return readonly_words[readonly_word];
        }
        if (readonly_start < 0x100000 && (total_address&0xF0000) == 0xE0000)
            return ltems._16(total_address&0xFFFF);
        if (readonly_start < 0x100000 && (total_address&0xF8000) == 0xB8000)
        {
            cga_used = true;
            return cga.memory16(total_address&0x7FFF);
        }
        return *(u16*)(void*)(memory_bytes+total_address);
    }

    void update()
    {

    }
} mem;*/

struct MemoryManager8088
{
    u8 memory_bytes[(1<<20)+1] = {};
    void dump_memory(const char* filename)
    {
        FILE* filu = fopen(filename, "wb");
        fwrite(memory_bytes, 0x100000, 1, filu);
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

    bool cga_used{};

    u8& _8(u16 segment, u16 index)
    {
        u32 total_address = (((segment<<4)+index)&0xFFFFF);
        if (total_address >= readonly_start)
        {
            ++readonly_byte;
            readonly_bytes[readonly_byte] = memory_bytes[total_address];
            return readonly_bytes[readonly_byte];
        }

        if (readonly_start < 0x100000 && (total_address&0xF0000) == 0xE0000)
            return ltems._8(total_address&0xFFFF);
        if (readonly_start < 0x100000 && (total_address&0xF8000) == 0xB8000)
        {
            cga_used = true;
            return cga.memory8(total_address&0x7FFF);
        }
        return memory_bytes[total_address];
    }
    u16& _16(u16 segment, u16 index)
    {
        u32 total_address = (((segment<<4)+index)&0xFFFFF);
        if (total_address >= readonly_start)
        {
            ++readonly_word;
            readonly_words[readonly_word] = *(u16*)(void*)(memory_bytes+total_address);
            return readonly_words[readonly_word];
        }
        if (readonly_start < 0x100000 && (total_address&0xF0000) == 0xE0000)
            return ltems._16(total_address&0xFFFF);
        if (readonly_start < 0x100000 && (total_address&0xF8000) == 0xB8000)
        {
            cga_used = true;
            return cga.memory16(total_address&0x7FFF);
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
} mem;

struct BEEPER
{
    i16 buffer[1<<16] = {};
    u16 write_offset{};
    u16 read_offset{};

    //u8 timer{};

    i16 sampleA{}, sampleB{}, sampleC{};

    u8 pb1{}; //speaker data
    u8 pb0{}; //timer gate

    void set_output_from_pit(bool value)
    {
        /*if (globalsettings.sound_on)
        {
            static FILE* filu = nullptr;
            if (filu == nullptr)
                filu = fopen("d:\\out2.raw", "wb");
            fwrite(&sampleA, 2, 1, filu);
        }*/
        sampleA = ((value&pb1)?60*256:0)*(pb0?-1:1);
        sampleB = ((sampleB<<5)-sampleB+sampleA)>>5; //crude lowpass
        sampleC = ((sampleC<<5)-sampleC+sampleB)>>5; //crude lowpass
    }

    void cycle()
    {
        i16 state = sampleC;
        i32 data = state+ym3812.sample;
        data = (data<-32768?-32768:data);
        data = (data>32767?32767:data);
        buffer[write_offset] = globalsettings.sound_on?i16(data):i16(0);
        ++write_offset;
    }
} beeper;

u64 totalframes = 0;

//we need to somehow sync the "real audio timing" to the "emulator timing"
//this takes some thinking.

//METHOD 1: always take the N last frames. might lead to misses or repetition, but has stable pitch!
void audio_method1(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    static bool started{false};
    if (!started)
    {
        started = true;
        beeper.read_offset = beeper.write_offset-256;
    }
    u16 offset_end = beeper.write_offset;
    u16 offset_start = beeper.read_offset;
    u16 done_count = u16(offset_end-offset_start);
    if (frameCount == 0 || done_count == 0)
        return;

    totalframes += frameCount;
    offset_start = offset_end-frameCount;
    i16* pi16Output = (i16*)pOutput;
    for(u32 done_frames=0; done_frames<frameCount; ++done_frames)
    {
        i16 data = beeper.buffer[u16(offset_start+done_frames)];
        *pi16Output = data;
        ++pi16Output;
    }
}

//METHOD 2: resample the M last frames to fit into frameCount. always uses every sample but has unstable pitch!
void audio_method2(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    static bool started{false};
    if (!started)
    {
        started = true;
        beeper.read_offset = beeper.write_offset-256;
    }
    u16 offset_end = beeper.write_offset;
    u16 offset_start = beeper.read_offset;
    u16 done_count = u16(offset_end-offset_start);
    if (frameCount == 0 || done_count == 0)
        return;

    totalframes += frameCount;
    u64 frame_counter=0;
    i16* pi16Output = (i16*)pOutput;
    for(u32 done_frames=0; done_frames<frameCount; ++done_frames)
    {
        i16 data = beeper.buffer[u16(offset_start+frame_counter/frameCount)];
        *pi16Output = data;
        ++pi16Output;
        frame_counter += done_count;
    }
    beeper.read_offset += frame_counter/frameCount;
}

//METHOD 3: dynamic resampling. experimental!
i16 additional_samples = 0;
const float SAMPLERATE = 48000.0;
float veer = (14318180.0/298.0)/SAMPLERATE;
void audio_method3(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    static bool started{false};
    u16 offset_end = beeper.write_offset;
    if (!started)
    {
        started = true;
        beeper.read_offset = offset_end-256;
    }
    u16 offset_start = beeper.read_offset;
    u16 done_count = u16(offset_end-offset_start);
    if (frameCount == 0 || done_count == 0)
        return;

    totalframes += frameCount;
    u64 frame_counter=0;

    done_count = frameCount*veer+additional_samples;
    i16* pi16Output = (i16*)pOutput;
    for(u32 done_frames=0; done_frames<frameCount; ++done_frames)
    {
        i16 data = beeper.buffer[offset_start];
        *pi16Output = data;
        ++pi16Output;
        frame_counter += done_count;
        while(frame_counter >= frameCount)
        {
            ++offset_start;
            frame_counter -= frameCount;
            if (offset_start == offset_end)
                goto double_break; //oh no :o
        }
    }
double_break: // oh no :O
    u16 left = (offset_end-offset_start);

    if (u16(offset_end-offset_start) >= 2048)
        offset_start = offset_end-256;

    if (left > 1024)
        additional_samples = 2;
    else if (left > 260)
        additional_samples = 1;
    else if (left < 252)
        additional_samples = -1;
    else
        additional_samples = 0;

    beeper.read_offset = offset_start;
}

struct MiniAudio
{
    ma_result result;
    ma_device_config deviceConfig;
    ma_device device;

    MiniAudio()
    {
        deviceConfig = ma_device_config_init(ma_device_type_playback);
        deviceConfig.playback.format   = ma_format_s16;
        deviceConfig.playback.channels = 1;
        deviceConfig.sampleRate        = u32(SAMPLERATE);
        deviceConfig.dataCallback      = audio_method3;

        deviceConfig.noPreSilencedOutputBuffer = true;
        deviceConfig.noClip = true;
        deviceConfig.noFixedSizedCallback = true;

        if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS)
        {
            cout << "MINIAUDIO init not succesful." << endl;
            std::abort();
        }
        if (ma_device_start(&device) != MA_SUCCESS)
        {
            cout << "MINIAUDIO device start not successful. No audio will be output." << endl;
            ma_device_uninit(&device);
        }
    }

    ~MiniAudio()
    {
        ma_device_uninit(&device);
    }
} miniaudio;

struct CHIP8259 //PIC
{
    u8 init_state{}; // are we initializing?
    u8 irr{}; // Interrupt Request Register
    u8 imr{}; // Interrupt Mask Register
    u8 isr{}; // In-Service Register
    u8 icw[5] = {}; // Initialization Command Words
    u8 ocw[4] = {}; // Operation Command Words
    bool is_initialized = false;

    u8 read(u8 port) // port from 0 to 1 inclusive
    {
        if (port == 0)
        {
            if (ocw[3]&1)
            {
                if constexpr (DEBUG_LEVEL > 0)
                    cout << __PRETTY_FUNCTION__ << ":" << std::dec << __LINE__ << std::hex << endl;
                return irr;
            }
            else
            {
                if constexpr (DEBUG_LEVEL > 0)
                    cout << __PRETTY_FUNCTION__ << ":" << std::dec << __LINE__ << std::hex << endl;
                return isr;
            }
        }
        else if (port == 1)
        {
            if constexpr (DEBUG_LEVEL > 0)
                cout << __PRETTY_FUNCTION__ << ":" << std::dec << __LINE__ << std::hex << endl;
            return imr;
        }
        return 0;
    }

    void write(u8 port, u8 data) // port from 0 to 1 inclusive
    {
        if (port == 0)
        {
            if (data & 0x10) // ICW1
            {
                init_state = 1;
                icw[1] = data;
                is_initialized = false;
                if constexpr (DEBUG_LEVEL > 0)
                    cout << __PRETTY_FUNCTION__ << ":" << std::dec << __LINE__ << std::hex << endl;
            }
            else
            {
                // OCW2 or OCW3
                if (data & 0x18) // OCW3
                {
                    ocw[3] = data;
                }
                else // OCW2
                {
                    ocw[2] = data;
                    if (data & 0x20) // End of Interrupt (EOI)
                    {
                        if ((data&0x07) != 0)
                            cout << "EOI isr " << u32(data & 0x07) << endl;
                        isr &= ~(1 << (data & 0x07));
                    }
                }
            }
        }
        else if (port == 1)
        {
            if (init_state == 1) // ICW2
            {
                icw[2] = data;
                init_state = 3; //TODO: support multiple DMA chips. we skip ICW3 when there's only one
            }
            else if (init_state == 2) // ICW3
            {
                icw[3] = data;
                init_state = 3;
            }
            else if (init_state == 3) // ICW4
            {
                icw[4] = data;
                init_state = 0;
                is_initialized = true;
            }
            else
            {
                // Write to Interrupt Mask Register (IMR)
                imr = data;
                if constexpr (DEBUG_LEVEL > 0)
                {
                    cout << "interrupt mask: " << u32(imr) << endl;
                }
            }
        }
    }

    void cpu_ack_irq(u8 irq)
    {
        isr &= ~(1 << irq);
    }

    void cycle() // one clock cycle running
    {
        if (!is_initialized)
        {
            return;
        }

        // Check for any pending interrupts
        for (int i = 0; i < 8; ++i)
        {
            if (!masked(i) && pending(i) && !serviced(i))
            {
                if (startprinting)
                    cout << "IRQ: SERVICE " << u32(i) << endl;
                isr |= (1 << i);
                irr &= ~(1 << i);
            }
        }
    }

    bool masked(u8 irq)
    {
        return (imr&(1<<irq));
    }
    bool pending(u8 irq)
    {
        return (irr&(1<<irq));
    }
    bool serviced(u8 irq)
    {
        return (isr&(1<<irq));
    }

    bool request_interrupt(u8 irq)
    {
        if (!is_initialized || irq >= 8)
            return false;

        if (irr&(1<<irq))
            return false;
        if (isr&(1<<irq))
            return false;

        irr |= (1<<irq);
        return true;
    }
} pic;

struct CHIP8255 //keyboard etc
{
    static const u8 FLOPPY_DRIVES = 2;
    static const u8 HAS_8087 = 0;
    static const u8 MEMORY_BANKS = 4;

    enum VIDEO_CARD_TYPES
    {
        V_OTHER=0x00,
        CGA40=0x10,
        CGA80=0x20,
        MDA=0x30
    } const static VIDEO_CARD_TYPE = CGA80;

    //onboard DIP switches

    static const u8 SW1 = (FLOPPY_DRIVES>0?0x01:0x00)|(HAS_8087?0x02:0x00)|((MEMORY_BANKS-1)<<2)|VIDEO_CARD_TYPE|(FLOPPY_DRIVES>0?(FLOPPY_DRIVES-1)<<6:0);
    static const u8 SW2 = 0b1'1'1'1'0'0'1'0;//TODO: make these into setuppable bools

    static const u8 XT_SW = (FLOPPY_DRIVES>0?0x01:0x00)|(HAS_8087?0x02:0x00)|((MEMORY_BANKS-1)<<2)|VIDEO_CARD_TYPE|(FLOPPY_DRIVES>0?(FLOPPY_DRIVES-1)<<6:0);

    u8 regs[4] = {};
    u8 keyboard_self_test{0}; //if > 0, is doing a self test
    bool keyboard_self_test_done{};
    static const u8 KEYBOARD_SELF_TEST_LENGTH = 16; //:peeposhrug: lol
    u8 current_scancode = 0;
    bool is_initialized{false};

    deque<u8> scancode_queue;
    u16 kbd_wait{};

    void press(u8 scancode)
    {
        if (is_initialized)
        {
            scancode_queue.push_back(scancode);
        }
    }

    u8 read(u8 port) //port from 0 to 3! inclusive
    {
        if constexpr (DEBUG_LEVEL > 1)
        {
            cout << __PRETTY_FUNCTION__ << ": " << u32(port) << " read!" << endl;
            cout << u32(regs[0]) << endl;
            cout << u32(regs[1]) << endl;
            cout << u32(regs[2]) << endl;
            cout << u32(regs[3]) << endl;
        }

        if (port == 0)
        {
            if (globalsettings.machine == globalsettings.MACHINE_PC && regs[1]&0x80)
            {
                return SW1;
            }
            else
            {
                if (keyboard_self_test_done)
                {
                    keyboard_self_test_done = false;
                    return 0xAA;
                }
                else
                {
                    u8 ret = current_scancode;
                    return ret;
                }
            }
        }
        if (port==1)
        {
            return regs[1];
        }
        if (port == 2)
        {
            u8 value{};
            if (globalsettings.machine == globalsettings.MACHINE_XT)
            {
                if (regs[1]&0x08)
                {
                    value |= (XT_SW&0xF0)>>4;
                }
                else
                {
                    value |= XT_SW&0x0F;
                }
            }
            else if (globalsettings.machine == globalsettings.MACHINE_PC)
            {
                if (regs[1]&0x04)
                {
                    value |= SW2&0x0F;
                }
                else
                {
                    value |= (SW2&0xF0)>>4;
                }
            }
            return value;
        }

        //what
        cout << __PRETTY_FUNCTION__ << " weird thing?" << endl;
        std::abort();
    }

    void write(u8 port, u8 data) //port from 0 to 3! inclusive.
    {
        if (port == 1)
        {
            if ((data&0x40) && (!(regs[port]&0x40)))
            {
                cout << "Setting keyboard self test." << endl;
                if (keyboard_self_test == 0)
                {
                    keyboard_self_test = KEYBOARD_SELF_TEST_LENGTH;
                    is_initialized = false;
                }
            }
            if (data&0x80)
            {
                current_scancode = 0;
                pic.cpu_ack_irq(1);
            }
        }
        regs[port] = data;
        beeper.pb0 = (regs[1]&0x01)?1:0;
        beeper.pb1 = (regs[1]&0x02)?1:0;
    }

    void cycle()
    {
        if (keyboard_self_test > 0)
        {
            --keyboard_self_test;
            if (keyboard_self_test == 0) //finished the test :-)
            {
                keyboard_self_test_done = true;
                is_initialized = true;
                pic.request_interrupt(1);
            }
        }

        if (!scancode_queue.empty())
        {
            if (kbd_wait == 0 && current_scancode == 0)
            {
                current_scancode = scancode_queue.front();
                bool result = pic.request_interrupt(1);
                if (result)
                {
                    scancode_queue.pop_front();
                }
                kbd_wait = 2048;
            }
            else
            {
                --kbd_wait;
            }
        }
    }
} kbd;

struct CHIP8237 //DMA
{
    struct Channel
    {
        u16 num{}; //which channel this is
        u16 page{};
        u16 start_addr{};
        u16 transfer_count{};
        vector<u8>* device_vector{nullptr};
        u32 device_vector_offset{};

        u16 curr_addr{};
        u16 curr_count{};
        u32 curr_vector_offset{};

        bool mask{};
        bool automatic{};
        bool down{};
        enum TRANSFER_DIRECTION
        {
            DIR_VERIFY=0,
            DIR_TO_MEMORY=1,
            DIR_FROM_MEMORY=2,
        } transfer_direction{};
        enum MODE
        {
            MODE_ON_DEMAND=0,
            MODE_SINGLE=1,
            MODE_BLOCK=2,
            MODE_CASCADE=3
        } mode{};
        bool pending{};
        bool is_complete{};

        void initiate_transfer()
        {
            if (!pending)
            {
                curr_addr = start_addr;
                curr_count = transfer_count;
                curr_vector_offset = device_vector_offset;
                pending = true;
                is_complete = false;
            }
        }

        void cycle_transfer()
        {
            if (device_vector == nullptr)
            {
                //nothing
            }
            else if (transfer_direction == DIR_TO_MEMORY)
            {
                mem._8(page<<12,curr_addr) = (*device_vector)[curr_vector_offset];
            }
            else if (transfer_direction == DIR_FROM_MEMORY)
            {
                (*device_vector)[curr_vector_offset] = mem._8(page<<12,curr_addr);
            }
            else if (transfer_direction == DIR_VERIFY)
            {
            }
            else
            {
                cout << "Weird transfer direction " << u32(transfer_direction) << endl;
            }
            //cout << "CYCLE TRANSFER " << curr_count << " data=" << u32(*curr_data) << endl;
            //cout << "transfer_direction=" << u32(transfer_direction) << endl;
            if (down)
                --curr_addr;
            else
                ++curr_addr;

            bool cross_seg_boundary = (down && curr_addr==0xFFFF) || (!down && curr_addr==0x0000);

            if (cross_seg_boundary)
            {
                cout << "DMA " << num << ": seg boundary crossed. :(" << endl;
            }

            ++curr_vector_offset;
            if (curr_count == 0 || cross_seg_boundary)
            {
                pending = false;
                is_complete = true;
                if (!automatic)
                {
                    start_addr = 0;
                    transfer_count = 0;
                }
            }
            else
            {
                curr_count -= 1;
            }
        }

        bool is_complete_and_reset()
        {
            bool ret = is_complete;
            is_complete = false;
            return ret;
        }
    } chans[4];

    CHIP8237()
    {
        chans[0].num = 0;
        chans[1].num = 1;
        chans[2].num = 2;
        chans[3].num = 3;
    }

    void print_params(u8 channel)
    {
        Channel& c = chans[channel];
        cout << std::hex;
        cout << ">DMA port " << u32(channel) << "!< ";
        cout << "p+addr=" << c.page*65536+c.start_addr << " ";
        cout << "n=" << c.transfer_count << " ";
        cout << "mode=" << u32(c.mode) << " ";
        cout << "direction=" << u32(c.transfer_direction) << endl;
    }

    bool enabled{};
    bool flip_flop{false};

    u8 read(u8 port) //port from 0 to 15! inclusive
    {
        u8 result{};
        if (port >= 0x00 && port <= 0x07)
        {
            u16 value = (port&0x01?chans[port>>1].transfer_count:chans[port>>1].start_addr);
            result = (value>>(flip_flop?8:0))&0xFF;
            flip_flop = !flip_flop;
        }
        else if (port == 0x08)
        {
            for(int i=0; i<4; ++i)
            {
                result |= (chans[i].is_complete)<<i;
                result |= (chans[i].pending)<<(i+4);

                chans[i].is_complete = false;
            }
        }
        else
        {
            std::cout << "Unsupported DMA read port " << u32(port) << endl;
            //std::abort();
            result = 0;
        }
        cout << "DMA READ " << u32(port) << " <- " << u32(result) <<     endl;
        return result;
    }

    void write(u8 port, u8 data) //port from 0 to 15! inclusive, also 0x87, 0x83, 0x81, 0x82
    {
        if (startprinting)
            cout << "DMA WRITE " << u32(port) << " <- " << u32(data) << endl;
        if (port == 0x87)
            chans[0].page = data;
        else if (port == 0x83)
            chans[1].page = data;
        else if (port == 0x81)
            chans[2].page = data;
        else if (port == 0x82)
            chans[3].page = data;
        else if (port >= 0x08 && port <= 0x0F)
        {
            if (false);
            else if (port == 0x08) //we're only interested in bit 2 as per osdev's article. hooray indeed
            {
                enabled = (data&0x04);
            }
            else if (port == 0x0A)
            {
                chans[data&0x03].mask = data&0x04;
            }
            else if (port == 0x0B)
            {
                u8 chan_n = data&0x03;
                Channel& c = chans[chan_n];
                c.transfer_direction = Channel::TRANSFER_DIRECTION((data>>2)&0x03);
                c.automatic = (data>>4)&0x01;
                c.down = ((data>>5)&0x01);
                c.mode = Channel::MODE(data>>6);
                if (startprinting)
                    cout << "DMA #" << u32(chan_n) << ": dir=" << u32(c.transfer_direction) << " auto=" << u32(c.automatic) << " down=" << u32(c.down) << " mode=" << u32(c.mode) << endl;

                if (chan_n == 0 && c.automatic)
                {
                    c.device_vector = nullptr;
                }
            }
            else if (port == 0x0C)
            {
                flip_flop = false;
            }
            else if (port == 0x0D) //master clear!
            {
                flip_flop = false;
                chans[0].mask = true;
                chans[1].mask = true;
                chans[2].mask = true;
                chans[3].mask = true;
            }
            else if (port == 0x0E)
            {
                chans[0].mask = false;
                chans[1].mask = false;
                chans[2].mask = false;
                chans[3].mask = false;
            }
            else if (port == 0x0F)
            {
                chans[0].mask = data&0x01;
                chans[1].mask = data&0x02;
                chans[2].mask = data&0x04;
                chans[3].mask = data&0x08;
            }
            else
            {
                std::cout << "Unsupported DMA write port " << u32(port) << endl;
                std::abort();
            }
        }
        else
        {
            u16& value = (port&0x01?chans[port>>1].transfer_count:chans[port>>1].start_addr);
            if (flip_flop)
                value = (value&(0xFF)) | (data<<8);
            else
                value = data;
            flip_flop = !flip_flop;
        }
    }

    void transfer(u8 channel, vector<u8>* device_vector, u32 device_vector_offset)
    {
        Channel& c = chans[channel];
        if (startprinting)
        {
            cout << ">DMA transfer on port " << u32(channel) << "!< ";
            cout << "p=" << c.page << " ";
            cout << "addr=" << c.start_addr << " ";
            cout << "n=" << c.transfer_count << endl;
            if (device_vector != nullptr)
            {
                cout << device_vector->size() << " total in device." << endl;
                cout << device_vector_offset << "+" << c.transfer_count+1 << "=" << device_vector_offset+c.transfer_count+1 << endl;
            }
        }

        //cout << "device dataptr: " << (void*)device_data << endl;
        c.device_vector = device_vector;
        c.device_vector_offset = device_vector_offset;

        c.initiate_transfer();
    }

    void cycle()
    {
        for(u64 i=0; i<4; ++i)
        {
            Channel& c = chans[i];
            if (c.pending)
            {
                c.cycle_transfer();
            }
        }
    }
} dma;


struct CHIP8253 //PIT
{
    static constexpr const u32 N_CHANNELS = 3;

    struct Channel
    {
        bool write_wait_for_second_byte{};
        u8 operating_mode{}; //0-5 inclusive
        u8 access_mode{}; //0-3 inclusive
        u16 reload{};
        u16 reload_loader{};
        u16 current{};
        bool stopped{};
        u8 output{};

        void Print()
        {
            #define paska(x) cout << #x ": " << u32(x) << endl;
            paska(write_wait_for_second_byte);
            paska(operating_mode);
            paska(access_mode);
            paska(reload);
            paska(current);
            #undef paska
        }

        /*u8 get_current()
        {
            if (access_mode == 1)
                return current&0xFF;
            if (access_mode == 2)
                return current>>8;
            cout << "Tried to get current but mode is " << u32(access_mode) << endl;
            std::abort();
        }*/

        u8 normal_data_state{};

        u8 latch_data_state{}; //0=no data, 1=has data, 2=access mode III, upper bytes
        u16 latch_data{};

    } channels[N_CHANNELS];

    u8 read(u8 port) //port from 0 to 3! inclusive
    {
        if (startprinting)
            cout << "READ PIT---- " << u32(port) << endl;
        if (port >= 3) //can't read port 3
        {
            return 0;
        }
        //TODO: do better
        Channel& c = channels[port];

        if (c.latch_data_state > 0)
        {
            if (c.access_mode == 1)
            {
                c.latch_data_state = 0;
                return c.latch_data;
            }
            else if (c.access_mode == 2)
            {
                c.latch_data_state = 0;
                return c.latch_data>>8;
            }
            else if (c.access_mode == 3)
            {
                if (c.latch_data_state == 1)
                {
                    c.latch_data_state = 2;
                    return c.latch_data;
                }
                else if (c.latch_data_state == 2)
                {
                    c.latch_data_state = 0;
                    return c.latch_data>>8;
                }
            }

        }


        if (c.access_mode == 1)
        {
            return c.current;
        }
        else if (c.access_mode == 2)
        {
            return c.current>>8;
        }
        else if (c.access_mode == 3)
        {
            if (c.normal_data_state == 0)
            {
                c.normal_data_state = 1;
                return c.current;
            }
            else if (c.normal_data_state == 1)
            {
                c.normal_data_state = 0;
                return c.current>>8;
            }
        }

        if (c.access_mode == 0)
            return 0;

        cout << "PIT WTF. reading port: " << u32(port) << endl;
        cout << u32(c.access_mode) << endl;
        std::abort();
    }

    void write(u8 port, u8 data) //port from 0 to 3! inclusive.
    {
        if (port == 3)
        {
            bool is_bcd = (data&0x01);
            if (is_bcd)
            {
                cout << "PIT doesn't support BCD mode yet!" << endl;
                std::abort();
            }
            u8 channel_n = (data>>6);
            if (channel_n == 3)
            {
                cout << "Channel 3 non-existent on PIT! (trying to run AT code? this is an PC emulator.)" << endl;
                std::abort();
            }
            Channel& c = channels[channel_n];
            if (((data>>4)&0x03) == 0)
            {
                c.latch_data_state = 1;
                c.latch_data = c.current;
                return;
            }
            c.access_mode = ((data>>4)&0x03);

            c.operating_mode = ((data>>1)&0x07);
            c.operating_mode = (c.operating_mode>=6?c.operating_mode^4:c.operating_mode);
            c.write_wait_for_second_byte = false;
            c.latch_data_state = 0;
            c.normal_data_state = 0;

            if (c.access_mode == 0)
            {
                c.output = false;
            }

            if constexpr (DEBUG_LEVEL > 0)
            {
                cout << "-----PIT Channel #" << u32(channel_n) << ":" << endl;
                c.Print();
            }
        }
        else
        {
            Channel& c = channels[port];

            if (c.access_mode == 1) //lobyte only
            {
                c.reload = data;
                c.stopped = false;
                if constexpr (DEBUG_LEVEL > 0)
                {
                    cout << "port " << u32(port) << " ACCESS MODE " << u32(c.access_mode) << ": new data " << c.reload << endl;                c.write_wait_for_second_byte = false;
                }
            }
            else if (c.access_mode == 2) //hibyte only
            {
                c.reload = (data<<8);
                c.stopped = false;
                if constexpr (DEBUG_LEVEL > 0)
                    cout << "port " << u32(port) << " ACCESS MODE " << u32(c.access_mode) << ": new data " << c.reload << endl;
            }
            else if (c.access_mode == 3) //lo, unless latch is, then hi
            {
                if (c.write_wait_for_second_byte)
                {
                    c.reload_loader = (c.reload_loader| (data<<8));
                    c.stopped = false;
                    c.reload = c.reload_loader;
                }
                else
                {
                    c.reload_loader = data;
                    c.stopped = false;
                }
                if constexpr (DEBUG_LEVEL > 0)
                    cout << "port " << u32(port) << " ACCESS MODE " << u32(c.access_mode) << ": new data " << c.reload << " & " << c.current << endl;
                c.write_wait_for_second_byte = !c.write_wait_for_second_byte;
            }
            if constexpr (DEBUG_LEVEL > 0)
                cout << "--- operating mode " << u32(c.operating_mode) << endl;

            if (c.operating_mode == 0 || c.operating_mode == 1 || c.operating_mode == 2)
            {
                c.current = c.reload;
                c.output = false;
                c.stopped = false;
            }
        }
    }

    u64 int0_count{};
    void cycle()
    {
        for(u32 i=0; i<3; ++i)
        {
            Channel& c = channels[i];
            if (c.operating_mode == 0)
            {
                c.current -= 1;
                if (c.current == 0)
                {
                    if (c.output == false && i==0)
                    {
                        if constexpr (DEBUG_LEVEL > 0)
                        {
                            cout << std::dec;
                            cout << __PRETTY_FUNCTION__ << ":" << __LINE__ << ": " << u32(c.reload) << " "  << u32(c.current) << " " << u32(c.operating_mode) << endl;
                            cout << std::hex;
                        }
                        ++int0_count;
                        pic.request_interrupt(0);
                    }
                    c.output = true;
                }
            }
            else if (c.operating_mode == 1)
            {
                if (c.current == 0)
                {
                    c.output = true;
                }
                else
                {
                    c.current -= 1;
                }
            }
            else if (c.operating_mode == 2)
            {
                //cout << "PIT #" << i << " opmode 2, curr " << u32(c.current) << endl;
                c.current -= 1;
                if (c.current <= 1)
                {
                    c.output = 0;
                    c.current = c.reload;
                    if (i==0)
                    {
                        if constexpr (DEBUG_LEVEL > 0)
                        {
                            cout << std::dec;
                            cout << __PRETTY_FUNCTION__ << ":" << __LINE__ << ": " << u32(c.reload) << " "  << u32(c.current) << " " << u32(c.operating_mode) << endl;
                            cout << std::hex;
                        }
                        ++int0_count;
                        pic.request_interrupt(0);
                    }
                    else if (i==1 && globalsettings.machine == globalsettings.MACHINE_XT)
                    {
                        //cout << "INITIATE TRANSFER XT 1" << endl;
                        //dma.chans[0].initiate_transfer();
                        //dma.chans[0].start_addr += 1;
                    }
                }
                else if (c.current == u32(c.reload-1))
                {
                    c.output = 1;
                }
                //cout << "PIT #" << i << " opmode 2, new  " << u32(c.current) << endl;
            }
            else if (c.operating_mode == 3)
            {
                if (c.current&1)
                {
                    c.current -= c.output?1:3;
                }
                else
                {
                    c.current -= 2;
                }
                if (c.current < 2)
                {
                    c.output = !c.output;
                    c.current = c.reload;
                    if (i==0)
                    {
                        if constexpr (DEBUG_LEVEL > 0)
                        {
                            cout << std::dec;
                            cout << __PRETTY_FUNCTION__ << ":" << __LINE__ << ": " << u32(c.reload) << " "  << u32(c.current) << " " << u32(c.operating_mode) << endl;
                            cout << std::hex;
                        }
                        if (c.output)
                        {
                            ++int0_count;
                            pic.request_interrupt(0);
                        }
                    }
                    else if (i==1 && globalsettings.machine == globalsettings.MACHINE_XT)
                    {
                        cout << "INITIATE TRANSFER XT 2" << endl;
                        //dma.chans[0].initiate_transfer();
                    }
                }
            }
            else
            {
                cout << "Unknown PIT operating mode " << u32(c.operating_mode) << endl;
                std::abort();
            }
            //TODO
        }
        beeper.set_output_from_pit(!channels[2].output);
    }
} pit;

struct HARDDISK
{
    struct DISK
    {
        struct DiskType
        {
            static const u32 BYTES_PER_SECTOR = 512;
            u32 cylinders=0;
            u32 heads=0;
            u32 sectors=0;

            u32 get_byte_offset(u32 cylinder, u32 head, u32 sector)
            {
                return ((cylinder*heads+head)*sectors+sector)*BYTES_PER_SECTOR;
            }

            bool is_valid(u32 cylinder, u32 head, u32 sector)
            {
                return (cylinder < cylinders) && (head < heads) && (sector < sectors);
            }

            u32 totalsize()
            {
                return cylinders*heads*sectors*BYTES_PER_SECTOR;
            }
        } static constexpr disktypes[4] =
        {
            {306, 2, 17},
            {375, 8, 17},
            {306, 6, 17},
            {306, 4, 17}
        };

        static const u32 DISKTYPE_ID = 1;
        DiskType type{disktypes[DISKTYPE_ID]};
        vector<u8> data;
        std::string filename;

        void flush()
        {
            if (data.empty())
            {
                return;
            }
            const u64 BLOCK_SIZE = 0x10000;

            FILE* filu = fopen(filename.c_str(), "rb+");

            if (filu == nullptr)
            {
                filu = fopen(filename.c_str(), "wb");
                fwrite(data.data(), data.size(), 1, filu);
                fclose(filu);
                return;
            }

            fseek(filu, 0, SEEK_END);
            u64 filesize = ftell(filu);
            if (data.size() != filesize)
            {
                cout << "Data size " << data.size() << " is not file size " << filesize << endl;
                std::abort();
            }
            fseek(filu, 0, SEEK_SET);

            vector<u8> filedata(BLOCK_SIZE,0);
            for(u64 pos=0; pos<data.size(); pos += BLOCK_SIZE)
            {
                fseek(filu, pos, SEEK_SET);
                int sectors_read = fread(filedata.data(), 512, BLOCK_SIZE/512, filu);

                if (memcmp(filedata.data(), data.data()+pos, sectors_read*512) != 0)
                {
                    cout << "Block " << pos/BLOCK_SIZE << " changed." << endl;
                    fseek(filu, pos, SEEK_SET);
                    fwrite(data.data()+pos, 512, BLOCK_SIZE/512, filu);
                }
            }
            fclose(filu);
        }

        DISK()
        {
            //data.assign(type.totalsize(),0);
            //cout << "HD: " << data.size() << " bytes." << endl;
            filename = "pieru";
        }

        DISK(const std::string& filename_):filename(filename_)
        {
            data.assign(type.totalsize(),0);
            cout << "HD: " << data.size() << " bytes." << endl;

            FILE* filu = fopen(filename.c_str(), "rb");
            if (filu != nullptr)
            {
                fseek(filu,0,SEEK_END);
                u32 size = ftell(filu);
                if (size == data.size())
                {
                    fseek(filu,0,SEEK_SET);
                    data.assign(size,0);
                    fread(data.data(), size, 1, filu);
                }
                else
                {
                    cout << "File " << filename << " doesnt contain an image of " << data.size() << " bytes." << endl;
                }
                fclose(filu);
            }
            else
            {
                cout << "File " << filename << " not found when loading harddisk." << endl;
            }
        }
    } disks[2];

    enum COMMAND
    {
        TEST_DRIVE_READY = 0x00,
        RECALIBRATE = 0x01,
        REQUEST_SENSE_STATUS = 0x03,
        FORMAT_DRIVE = 0x04,
        READY_VERIFY = 0x05,
        FORMAT_TRACK = 0x06,
        FORMAT_BAD_TRACK = 0x07,
        READ = 0x08,
        WRITE = 0x0A,
        SEEK = 0x0B,
        INITIALIZE_DRIVE_CHARACTERISTICS = 0x0C, //has 8 extra bytes!
        READ_ECC_BURST_ERROR_LENGTH = 0x0D,
        READ_DATA_FROM_SECTOR_BUFFER = 0x0E,
        WRITE_DATA_TO_SECTOR_BUFFER = 0x0F,
        RAM_DIAGNOSTIC = 0xE0,
        DRIVE_DIAGNOSTIC = 0xE3,
        CONTROLLER_INTERNAL_DIAGNOSTICS = 0xE4,
        READ_LONG = 0xE5,
        WRITE_LONG = 0xE6
    };
    enum ERROR //i commented out the ones that won't come up
    {
        NO_ERROR = 0x00,
        //NO_INDEX_SIGNAL = 0x01,
        //NO_SEEK_COMPLETE = 0x02,
        //WRITE_FAULT = 0x03,
        NO_READY_AFTER_SELECT = 0x04,
        //NO_TRACK_00_SIGNAL = 0x06,
        STILL_SEEKING = 0x08, //reported by TEST_DRIVE_READY
        //ID_READ_ERROR = 0x10,
        //DATA_ECC_ERROR = 0x11,
        //NO_TARGET_ADDRESS_MARK = 0x12,
        SECTOR_NOT_FOUND = 0x14,
        SEEK_ERROR = 0x15,
        //CORRECTABLE_DATA_ECC_ERROR = 0x18,
        //BAD_TRACK = 0x19,
        INVALID_COMMAND = 0x20,
        ILLEGAL_DISK_ADDRESS = 0x21,
        //RAM_ERROR = 0x30,
        //PROGRAM_MEMORY_CHECKSUM_ERROR = 0x31,
        //ECC_POLYNOMIAL_ERROR = 0x32,
    };

    bool error{false};
    bool logical_unit_number{0}; //0 or 1 (?? what is this)
    bool dma_enabled{};
    bool irq_enabled{};

    bool r1_busy{};
    bool r1_bus{};

    enum
    {
        IO_A = 0,
        IO_B = 1,
    };
    bool r1_iomode{IO_A};
    bool r1_req{};
    bool r1_int_occurred{};

    u16 interrupttime{};

    u8 data_in[6] = {};
    u8 current_data_in_index{};

    bool address_valid{};
    u8 errorcode{}; // look in the ERROR enum
    u8 current_drive{};
    u8 current_head{};
    u16 current_cylinder{};
    u8 current_sector{};

    void print_data_in()
    {
        cout << "HD data in:";
        cout << " command=" << u32(data_in[0]);
        cout << " drive=" << u32(data_in[1]>>5);
        cout << " head=" << u32(data_in[1]&0x1F);
        cout << " cylinder=" << u32(data_in[2]&0xC0)*4+u32(data_in[3]);
        cout << " sector=" << (u32(data_in[2])&0x3F);
        cout << " interleave=" << u32(data_in[4]&0x1F);
        cout << " step=" << u32(data_in[5]&0x07);
        cout << " retries=" << bool(data_in[5]&0x80);
        cout << " eccretry=" << bool(data_in[5]&0x40);
        cout << endl;
    }

    u8 sense[4] = {};
    bool do_drive_characteristics{};
    u8 drive_characteristics[8] = {};
    u8 dc_index{};

    vector<u8> sector_buffer = vector<u8>(512,0); //todo: verify size?

    bool dma_in_progress{false};

    deque<u8> output_bytes;

    void set_current_params()
    {
        current_cylinder = data_in[3]|((data_in[2]<<2)&0xFF00);
        current_sector = (data_in[2]&0x3F);
        current_head = (data_in[1]&0x1F);
        current_drive = (data_in[1]&0x20)>>5;

        address_valid = disks[current_drive].type.is_valid(current_cylinder,current_head,current_sector);
    }

    void write(u8 port, u8 data) //port from 0 to 3! inclusive.
    {
        if (port == 0) // data port
        {
            if (do_drive_characteristics)
            {
                r1_iomode = IO_B;
                r1_req = true;
                //cout << "HD: doing more drive characteristics! c[" << u32(dc_index) << "] = " << u32(data) << endl;
                drive_characteristics[dc_index] = data;
                ++dc_index;
                if (dc_index == 8)
                {
                    do_drive_characteristics = false;
                    dc_index = 0;
                    cout << "HD: drive characteristics gotten! ";
                    for(int i=0; i<8; ++i)
                        cout << u32(drive_characteristics[i]) << ' ';
                    cout << endl;
                    interrupttime = 0x300;
                    r1_req = false;
                }
            }
            else
            {
                r1_busy = true;
                data_in[current_data_in_index] = data;
                ++current_data_in_index;
                if (current_data_in_index == 6)
                {
                    //cout << "HD: All data collected! ";
                    //for(int i=0; i<6; ++i)
                    //    cout << u32(data_in[i]) << ' ';
                    //cout << endl;
                    //print_data_in();

                    if (false);
                    else if (data_in[0] == READ)
                    {
                        set_current_params();
                        u32 offset = disks[current_drive].type.get_byte_offset(current_cylinder, current_head, current_sector);
                        //cout << "HD READ offset: " << offset << endl;
                        if (address_valid)
                        {
                            dma.print_params(3);
                            dma.transfer(3, &disks[current_drive].data, offset);
                            dma_in_progress = true;
                        }
                        else
                        {
                            interrupttime = 0x300;
                        }
                        error = !address_valid;
                        r1_req = false;
                    }
                    else if (data_in[0] == WRITE)
                    {
                        set_current_params();
                        u32 offset = disks[current_drive].type.get_byte_offset(current_cylinder, current_head, current_sector);
                        //cout << "HD WRITE offset: " << offset << endl;
                        if (address_valid)
                        {
                            if (dma.chans[3].transfer_count != 0x1FF)
                            {
                                //cout << "----------------Transfer count: " << dma.chans[3].transfer_count << endl;
                            }

                            dma.print_params(3);
                            dma.transfer(3, &disks[current_drive].data, offset);
                            dma_in_progress = true;
                        }
                        else
                        {
                            interrupttime = 0x300;
                            errorcode = NO_READY_AFTER_SELECT;
                        }
                        error = !address_valid;
                        r1_req = false;
                    }
                    else if (data_in[0] == REQUEST_SENSE_STATUS)
                    {
                        output_bytes.clear();
                        output_bytes.push_back((address_valid<<7)|errorcode);
                        output_bytes.push_back((current_drive<<5)|current_head);
                        output_bytes.push_back(((current_cylinder&0x300)>>3)|current_sector);
                        output_bytes.push_back(current_cylinder&0xFF);
                        r1_iomode = IO_B;
                        errorcode = NO_ERROR;
                    }
                    else if (data_in[0] == INITIALIZE_DRIVE_CHARACTERISTICS)
                    {
                        //do nothing for now
                        do_drive_characteristics = true;
                        dc_index = 0;
                        r1_req = false;
                    }
                    else if (data_in[0] == WRITE_DATA_TO_SECTOR_BUFFER)
                    {
                        //dma.chans[3].device_data = sector_buffer;
                        if (dma.chans[3].transfer_count != 0x1FF)
                        {
                            cout << "sector buf write size not 512" << endl;
                            std::abort();
                        }
                        dma.print_params(3);
                        dma.transfer(3, &sector_buffer, 0);
                        r1_req = false;
                        dma_in_progress = true;
                    }
                    else if (data_in[0] == SEEK)
                    {
                        //do nothing(?)
                        interrupttime = 0x300;
                        r1_req = false;
                    }
                    else if (data_in[0] == READY_VERIFY)
                    {
                        //do nothing(?)
                        interrupttime = 0x300;
                        r1_req = false;
                    }
                    else if (data_in[0] == TEST_DRIVE_READY)
                    {
                        //do nothing(?)
                        interrupttime = 0x300;
                        r1_req = false;
                    }
                    else if (data_in[0] == RECALIBRATE)
                    {
                        //do nothing(?)
                        interrupttime = 0x300;
                        r1_req = false;
                    }
                    else if (data_in[0] == RAM_DIAGNOSTIC)
                    {
                        //do nothing(?)
                        interrupttime = 0x300;
                        r1_req = false;
                    }
                    else if (data_in[0] == CONTROLLER_INTERNAL_DIAGNOSTICS)
                    {
                        //do nothing(?)
                        interrupttime = 0x300;
                        r1_req = false;
                    }
                    else
                    {
                        cout << "idk command " << u32(data_in[0]) << endl;
                        std::abort();
                    }

                    current_data_in_index = 0;
                }
            }
        }
        else if (port == 1) //controller reset
        {
            //cout << "HD WRITE: reset controller" << endl;
            //startprinting = true;
            error=false;
            r1_busy = false;
            r1_int_occurred = false;
            r1_iomode = IO_A;
            current_data_in_index = 0;
        }
        else if (port == 2) //generate controller-select pulse (?)
        {
            //cout << "HD WRITE: controller select pulse! unn tss unn tss" << endl;
            //idk
        }
        else if (port == 3)
        {
            dma_enabled = (data&0x01);
            irq_enabled = (data&0x02);
            //cout << "HD WRITE: dma=" << (dma_enabled?"enabled":"disabled") << " irq=" << (irq_enabled?"enabled":"disabled") << endl;
            r1_iomode = IO_A;
            r1_busy = true;
            r1_bus = true;
            r1_req = true;
            current_data_in_index = 0;
        }
        else
        {
            cout << "HD WRITE: unknown port " << u32(port) << " w/data " << u32(data) << endl;
            std::abort();
        }
    }
    u8 read(u8 port) //port from 0 to 3! inclusive
    {
        u8 data{};
        if (port == 0)
        {
            if (output_bytes.empty())
            {
                data = (error<<1) | (logical_unit_number<<5);
                r1_busy = 0;
                r1_int_occurred = 0;
                r1_iomode = IO_A;
            }
            else
            {
                data = output_bytes.front();
                output_bytes.pop_front();
            }
        }
        else if (port == 1) //controller hardware status
        {
            //data |= (error << 1); //error bit but is wrong?
            //data |= (logical_unit_number << 5); //these are for some other status byte

            data |= (r1_busy << 3);
            data |= (r1_bus << 2);
            data |= (r1_iomode << 1);
            data |= (r1_req << 0);
            data |= (r1_int_occurred << 5);
            //cout << "HD READ hw status: " << u32(data) << endl;
        }
        else if (port == 2) //switch settings
        {
            data = 0b0101; //both drives type 2 (note inverted logic)
            //cout << "HD READ switch: " << u32(data) << endl;
            r1_req = true;
        }
        else
        {
            cout << "HD READ: unknown port " << u32(port) << endl;
            std::abort();
        }
        //cout << "HD READ total=" << u32(data) << endl;
        return data;
    }

    void cycle()
    {
        if (dma_in_progress)
        {
            if (dma.chans[3].is_complete_and_reset())
            {
                dma_in_progress = false;
                pic.request_interrupt(5);
                //cout << "HD IRQ AFTER DMA!!!" << endl;
                r1_int_occurred = true;
                current_sector += dma.chans[3].transfer_count/512; //this is correct. the count is -1, but we want -1.
                set_current_params();
            }
        }
        else if (interrupttime > 0)
        {
            //cout << "hd irq in " << interrupttime << endl;
            --interrupttime;
            if (interrupttime == 0)
            {
                if (irq_enabled)
                {
                    //cout << "HD IRQ!!!" << endl;
                    pic.request_interrupt(5);
                    r1_int_occurred = true;
                }
            }
        }
    }


} harddisk;

struct DISKETTECONTROLLER
{
/*
3F0-3F7  Floppy disk controller (except PCjr)
	3F0 Diskette controller status A
	3F1 Diskette controller status B
	3F2 controller control port
	3F4 controller status register
	3F5 data register (write 1-9 byte command, see INT 13)
	3F6 Diskette controller data
	3F7 Diskette digital input

STATUS_REGISTER_A                = 0x3F0, // read-only
STATUS_REGISTER_B                = 0x3F1, // read-only
DIGITAL_OUTPUT_REGISTER          = 0x3F2,
TAPE_DRIVE_REGISTER              = 0x3F3,
MAIN_STATUS_REGISTER             = 0x3F4, // read-only
DATARATE_SELECT_REGISTER         = 0x3F4, // write-only
DATA_FIFO                        = 0x3F5,
DIGITAL_INPUT_REGISTER           = 0x3F7, // read-only
CONFIGURATION_CONTROL_REGISTER   = 0x3F7  // write-only
*/

    static const u16 RESET_CYCLES = 256;
    u16 reset_state{};
    u32 interrupt_timer{};

    std::deque<u8> out_buffer;

    struct Drive
    {
        struct DISKETTE
        {
            u32 cylinders{};
            u32 heads{};
            u32 sectors{};
            u32 bytes_per_sector{512};
            vector<u8> data;

            void eject()
            {
                data.clear();
            }

            u32 get_byte_offset(u32 cylinder, u32 head, u32 sector)
            {
                return ((cylinder*heads+head)*sectors+(sector-1))*bytes_per_sector;
            }

            bool is_ready()
            {
                return !data.empty();
            }

            DISKETTE()
            {
                cylinders = 40;
                heads = 1;
                sectors = 9;
                //data.assign(184320,0);
            }

            DISKETTE(std::string filename)
            {
                FILE* filu = fopen(filename.c_str(), "rb");
                fseek(filu,0,SEEK_END);
                u32 size = ftell(filu);
                fseek(filu,0,SEEK_SET);

                if (size == 163840) //160k disk
                {
                    cylinders = 40;
                    heads = 1;
                    sectors = 8;
                }
                else if (size == 184320) //180k disk
                {
                    cylinders = 40;
                    heads = 1;
                    sectors = 9;
                }
                else if (size == 327680) //320k disk :-)
                {
                    cylinders = 40;
                    heads = 2;
                    sectors = 8;
                }
                else if (size == 368640) //360k disk :o
                {
                    cylinders = 40;
                    heads = 2;
                    sectors = 9;
                }
                else if (size == 1228800) //1.2M disk :O
                {
                    cylinders = 80;
                    heads = 2;
                    sectors = 15;
                }
                else if (size == 737280) //720k disk
                {
                    cylinders = 80;
                    heads = 2;
                    sectors = 9;
                }
                else if (size == 1474560) //1.44M disk
                {
                    cylinders = 80;
                    heads = 2;
                    sectors = 18;
                }
                else if (size == 2949120) //2.88M disk
                {
                    cylinders = 80;
                    heads = 2;
                    sectors = 36;
                }
                else
                {
                    cout << "Unknown floppy size in bytes: " << size << " and in sectors: " << size/512 << endl;
                    std::abort();
                }
                //u32 size = cylinders*heads*sectors*bytes_per_sector;
                data.assign(size,0);
                fread(data.data(), size, 1, filu);
                fclose(filu);
            }
        };
        DISKETTE diskette;

        bool is_write_protected()
        {
            return false;
        }
        bool is_double_sided()
        {
            return (diskette.heads > 1);
        }
        bool is_ready()
        {
            return diskette.is_ready();
        }

        u8 current_cylinder{};
        bool motor{};
    } drives[4];

    u8 selected_drive{};
    u8 main_status{0x80}; // RQM DIO NDM CB D3B D2B D1B D0B
    u8 st0{};
    u8 st1{};
    u8 st2{};

    u8 registers[8] = {}; // not all registers are used, but we'll do it this way to be simple

    u8 is_selected_and_on(u8 drive)
    {
        bool on = registers[2]&(0x10<<drive);
        bool selected = (registers[2]&0x3) == drive;
        return on && selected;
    }

    u8 read(u8 port) //port from 0 to 7! inclusive
    {
        /*if (FLOPPY_DEBUG)
            cout << "/-------------------------------------------\\" << endl;
        if (FLOPPY_DEBUG)
            cout << "FLOPPY CONTROLLER READ: " << u32(port) << endl;*/
        //PrintCSIP();
        u8 readdata{};
        if(false);
        else if (port == 0) // STATUS_REGISTER_A
        {
            return 0;
        }
        else if (port == 1) // STATUS_REGISTER_B
        {
            return 0;
        }
        else if (port == 4)
        {
            if (FLOPPY_DEBUG)
                cout << "READ MAIN STATUS REGISTER: ";
            readdata = main_status;
        }
        else if (port == 5) // FIFO
        {
            if (FLOPPY_DEBUG)
                cout << "READ FIFO" << endl;
            if (!out_buffer.empty())
            {
                readdata = out_buffer.front();
                out_buffer.pop_front();
                if (FLOPPY_DEBUG)
                    cout << "After the read, buffer still has " << out_buffer.size() << " bytes." << endl;
            }
            else
            {
                cout << "FIFO is empty :(" << endl;
                std::abort();
            }
        }
        else // 6 isnt used
        {
            std::cout << "Unsupported floppy port " << u32(port) << endl;
            std::abort();
        }
        if (FLOPPY_DEBUG)
            cout << "DATA READ = " << u32(readdata) << endl;
        /*if (FLOPPY_DEBUG)
            cout << "\\-------------------------------------------/" << endl;*/

        if (out_buffer.empty())
        {
            main_status &= ~0x50;
            main_status |= 0x80;
        }
        return readdata;
    }


    enum FloppyCommands // https://wiki.osdev.org/Floppy_Disk_Controller
    {
       READ_TRACK =                 2,	// generates IRQ6
       SPECIFY =                    3,      // * set drive parameters
       SENSE_DRIVE_STATUS =         4,
       WRITE_DATA =                 5,      // * write to the disk
       READ_DATA =                  6,      // * read from the disk
       RECALIBRATE =                7,      // * seek to cylinder 0
       SENSE_INTERRUPT =            8,      // * ack IRQ6, get status of last command
       WRITE_DELETED_DATA =         9,
       READ_ID =                    10,	// generates IRQ6
       READ_DELETED_DATA =          12,
       FORMAT_TRACK =               13,     // *
       DUMPREG =                    14,
       SEEK =                       15,     // * seek both heads to cylinder X
       VERSION =                    16,	// * used during initialization, once
       SCAN_EQUAL =                 17,
       PERPENDICULAR_MODE =         18,	// * used during initialization, once, maybe
       CONFIGURE =                  19,     // * set controller parameters
       LOCK =                       20,     // * protect controller params from a reset
       VERIFY =                     22,
       SCAN_LOW_OR_EQUAL =          25,
       SCAN_HIGH_OR_EQUAL =         29
    };
    static constexpr const char* commandnames[256] =
    {
        nullptr,
        nullptr,
        "read track",
        "specify",
        "sense drive",
        "write data",
        "read data",
        "recalibrate",
        "sense interrupt",
        "write deleted data",
        "read id",
        nullptr,
        "read deleted data",
        "format track",
        "dump registers",
        "seek",
        "version",
        "scan equal",
        "perpendicular mode",
        "configure",
        "lock",
        nullptr,
        "verify",
        nullptr,
        nullptr,
        "scan low or equal",
        nullptr,
        nullptr,
        nullptr,
        "scan high or equal"
    };

    u8 current_command{};
    u8 current_full_command{};
    u8 fifo_input_bytes_left{};

    void write(u8 port, u8 data) //port from 0 to 7! inclusive.
    {
        /*if (FLOPPY_DEBUG)
            cout << "/===========================================\\" << endl;
        if (FLOPPY_DEBUG)
            cout << "FLOPPY CONTROLLER WRITE: " << u32(port) << " data=" << u32(data) << endl;*/
        //PrintCSIP();
        if (port == 5) // DATA_FIFO
        {
            if (fifo_input_bytes_left > 0) // FIFO has input bytes, collect them
            {
                out_buffer.push_back(data);
                --fifo_input_bytes_left;
                if (fifo_input_bytes_left == 0) //all bytes collected! execute command
                {
                    if (FLOPPY_DEBUG)
                    {
                        cout << u32(current_command) << " finished!" << endl;
                        cout << "Command data: " << endl;
                        for(u64 i=0; i<out_buffer.size(); ++i)
                            cout << u32(out_buffer[i]) << " ";
                        cout << endl;
                    }
                    if (current_command == 0x03) //specify
                    {
                        out_buffer.clear();
                        main_status &= ~0x50; //no output bytes
                        current_command = 0;
                    }
                    else if (current_command == 0x04) // sense drive status
                    {
                        u8 databyte = out_buffer.back();
                        u8 drive_n = databyte&0x03;
                        out_buffer.clear();
                        main_status &= ~0xC0;
                        u8 st3 = 0;
                        //bit7 is fault, no fault
                        //bit6 is writeprotect
                        st3 |= drives[drive_n].is_write_protected()<<6;
                        st3 |= (drives[drive_n].is_ready()<<5); //ready
                        st3 |= (drives[drive_n].is_ready()<<4); //track 0 signal
                        st3 |= drives[drive_n].is_double_sided()<<3;
                        st3 |= databyte&0x07; //the rest are the same
                        out_buffer.push_back(st3);
                    }
                    else if (current_command == 0x05 || current_command == 0x06) //write | read
                    {
                        u32 drive = out_buffer[0]&0x03;
                        u32 head_A = out_buffer[0]>>2;
                        u32 cylinder = out_buffer[1];
                        u32 head_B = out_buffer[2];
                        if (head_A != head_B)
                        {
                            std::cout << "Head numbers don't match in WRITE/READ Command" << endl;
                        }
                        u32 sector = out_buffer[3];
                        if (out_buffer[4] != 0x02)
                        {
                            std::cout << "weird out_buffer[4] = " << u32(out_buffer[4]) << ", should be 0x02" << endl;
                            std::abort();
                        }
                        u32 end_of_track = out_buffer[5]; //number of sectors in a track
                        if (out_buffer[7] != 0xFF)
                        {
                            std::cout << "weird out_buffer[7] = " << u32(out_buffer[7]) << ", should be 0xFF" << endl;
                            std::abort();
                        }
                        u32 byte_offset = drives[drive].diskette.get_byte_offset(cylinder,head_A,sector);
                        std::cout << (current_command == 0x05?"WRITE":"READ") << ":";
                        cout << " drive=" << drive;
                        cout << " head=" << head_A;
                        cout << " cylinder=" << cylinder;
                        cout << " sector=" << sector;
                        cout << " end_of_track=" << end_of_track;
                        cout << " -> byte offset=" << byte_offset << endl;

                        if (drives[drive].diskette.is_ready())
                        {
                            dma.transfer(2, &drives[drive].diskette.data, byte_offset);
                        }
                        else
                        {
                            cout << "Drive not ready. (no diskette?)" << endl;
                        }
                        main_status &= ~0xC0;
                    }
                    else if (current_command == 0x07) //recalibrate
                    {
                        drives[data&0x03].current_cylinder = 0;
                        out_buffer.clear();
                        pic.request_interrupt(6);
                        main_status &= ~0x50; //no output bytes
                        st0 = selected_drive;
                        current_command = 0;
                    }
                    else if (current_command == 0x0F) //seek
                    {
                        //TODO: verify that this drive is selected
                        u8 drive_number = out_buffer[0]&0x03;
                        if (drives[drive_number].motor)
                        {
                            drives[drive_number].current_cylinder = out_buffer[1];
                        }
                        else
                        {
                            cout << "Tried to seek on drive #" << u32(drive_number) << " but motor is not on." << endl;
                        }
                        st0 = selected_drive;
                        main_status |= (1<<st0);
                        interrupt_timer = 4096;
                    }
                    else
                    {
                        cout << "Weird command " << u32(current_command) << " while starting operation." << endl;
                        std::abort();
                    }
                }
            }
            else //FIFO not active, start a new command
            {
                // Handle FDC commands here
                if (FLOPPY_DEBUG)
                    cout << "PORT 5 means COMMAND! ";

                u8 command = data&0x1F;

                if (FLOPPY_DEBUG)
                    if (commandnames[command])
                        cout << commandnames[command] << " - ";

                if (FLOPPY_DEBUG)
                    cout << "number=" << u32(command) << endl;
                switch (data&0x1F)
                {
                    case 0x03: // Specify
                        fifo_input_bytes_left = 2;
                        current_command = (data&0x1F);
                        main_status |= 0x10;
                        out_buffer.clear();
                        break;
                    case 0x04: // sense drive status
                        fifo_input_bytes_left = 1;
                        current_command = (data&0x1F);
                        main_status |= 0x10;
                        out_buffer.clear();
                        break;
                    case 0x05: // Write Data
                        fifo_input_bytes_left = 8;
                        current_command = (data&0x1F);
                        current_full_command = data;
                        main_status |= 0x10;
                        out_buffer.clear();
                        break;
                    case 0x06: // Read Data
                        fifo_input_bytes_left = 8;
                        current_command = (data&0x1F);
                        current_full_command = data;
                        main_status |= 0x10;
                        out_buffer.clear();
                        break;
                    case 0x07: // Recalibrate (seek to cyl 0)
                        fifo_input_bytes_left = 1;
                        current_command = (data&0x1F);
                        main_status |= 0x10;
                        out_buffer.clear();
                        break;
                    case 0x08: // Sense interrupt
                        out_buffer.push_back(st0);
                        out_buffer.push_back(drives[0].current_cylinder);
                        main_status |= 0x50;
                        break;
                    case 0x0F: // Seek
                        fifo_input_bytes_left = 2;
                        current_command = (data&0x1F);
                        main_status |= 0x10;
                        out_buffer.clear();
                        break;
                    default:
                        std::cout << "Unsupported FDC command " << u32(data) << "/" << u32(data&0x1F) << endl;
                        std::abort();
                }
                if (FLOPPY_DEBUG)
                    cout << "expecting " << u32(fifo_input_bytes_left) << " more bytes." << endl;
            }

        }
        else if (port == 2)
        {
            if (FLOPPY_DEBUG)
            {
                cout << "DOR byte!" << endl;
                cout << "Select drive #" << (data&0x03) << endl;

                if (data&0x04)
                    cout << "No reset mode." << endl;
                else
                {
                    cout << "Enter reset mode." << endl;
                }

                if (data&0x08)
                    cout << "Enable IRQ & DMA." << endl;
                else
                    cout << "Disable IRQ & DMA." << endl;
            }

            selected_drive = (data&0x03);
            if (!(data&0x04))
                reset_state = RESET_CYCLES;

            for(int i=0; i<4; ++i)
            {
                if (FLOPPY_DEBUG)
                    cout << "Drive #" << i << " motor " << ((data&(0x10<<i))?"ON":"OFF") << endl;
                drives[i].motor = (data&(0x10<<i));
            }
            registers[port] = data;
        }
        else
        {
            std::cout << "Unsupported floppy port " << u32(port) << endl;
            std::abort();
            registers[port] = data;
        }
        /*if (FLOPPY_DEBUG)
            cout << "\\===========================================/" << endl;*/
    }

    void cycle()
    {
        if (reset_state > 0)
        {
            --reset_state;
            if (reset_state == 0) //RESET DONE!
            {
                out_buffer.clear();
                cout << "FLOPPY Reset is now done!" << endl;
                main_status = 0x80;
                st0 = 0xC0;
                st1 = 0;
                st2 = 0;
                pic.request_interrupt(6);
            }
        }
        if (interrupt_timer > 0)
        {
            --interrupt_timer;
            if (interrupt_timer == 0)
            {
                st0 &= 0xF0; //clear the "seek" bits
                pic.request_interrupt(6);
            }
        }

        if (dma.chans[2].is_complete_and_reset())
        {
            pic.request_interrupt(6);
            main_status = 0xC0 | 0x10;

            st0 = 0x00;

            u32 cylinder = out_buffer[1];
            u32 sector = out_buffer[3];

            out_buffer.clear();
            out_buffer.push_back(st0);
            out_buffer.push_back(st1);
            out_buffer.push_back(st2);
            out_buffer.push_back(cylinder);
            out_buffer.push_back(0);
            out_buffer.push_back(sector + (dma.chans[2].transfer_count+1)/512);
            out_buffer.push_back(2);
            cout << "DMA COMPLETE lol. interrupt 6. did " << dma.chans[2].transfer_count << " bytes aka " << (dma.chans[2].transfer_count)/512+1 << " sectors" << endl;
            dma.print_params(2);
        }
    }
} diskettecontroller;

struct IO
{
    static void out(u16 port, u16 data)
    {
        if (startprinting)
            cout << "Write Port 0x" << u32(port) << " ----> 0x" << u32(data) << endl;
        if (false);
        else if (port == 0xA0)
        {
            cout << "NMI interrupt setting: " << data << endl;
        }
        else if (port >= 0x40 && port <= 0x43)
        {
            pit.write(port-0x40, data&0xFF);
        }
        else if (port >= 0x20 && port <= 0x21)
        {
            pic.write(port-0x20, data&0xFF);
        }
        else if ((port >= 0x00 && port <= 0x0F)
                 || (port == 0x87)
                 || (port == 0x83)
                 || (port == 0x81)
                 || (port == 0x82)
                 )
        {
            dma.write(port-0x00, data&0xFF);
        }
        else if (port >= 0x60 && port <= 0x63)
        {
            kbd.write(port-0x60, data&0xFF);
        }
        else if (port >= 0x3D0 && port <= 0x3DF)
        {
            cga.write(port-0x3D0, data&0xFF);
        }
        else if (port >= 0x3F0 && port <= 0x3F7)
        {
            diskettecontroller.write(port-0x3F0, data&0xFF);
        }
        else if (port >= 0x388 && port <= 0x389)
        {
            ym3812.write(port-0x388, data&0xFF);
        }
        else if (port >= 0x320 && port <= 0x323)
        {
            harddisk.write(port-0x320, data&0xFF);
        }
        else if (port == 0x201)
        {
            gameport.write(port-0x201, data&0xFF);
        }
        else if (port >= 0x260 && port <= 0x263)
        {
            ltems.write(port-0x260, data&0xFF);
        }
        else
        {
            //if constexpr(DEBUG_LEVEL > 0)
                //cout << "Writing Unknown port " << u32(port) << " data=" << u32(data&0xFF) << endl;
            //std::abort();
        }
    }
    static u16 in(u16 port)
    {
        u16 data = 0;
        if (false);
        else if (port >= 0x40 && port <= 0x43)
        {
            data = pit.read(port-0x40);
        }
        else if (port >= 0x20 && port <= 0x21)
        {
            data = pic.read(port-0x20);
        }
        else if (port >= 0x00 && port <= 0x0F)
        {
            data = dma.read(port-0x00);
        }
        else if (port >= 0x60 && port <= 0x63)
        {
            data = kbd.read(port-0x60);
        }
        else if (port >= 0x3D0 && port <= 0x3DF)
        {
            data = cga.read(port-0x3D0);
        }
        else if (port >= 0x3F0 && port <= 0x3F7)
        {
            data = diskettecontroller.read(port-0x3F0);
        }
        else if (port >= 0x388 && port <= 0x389)
        {
            data = ym3812.read(port-0x388);
        }
        else if (port >= 0x320 && port <= 0x323)
        {
            data = harddisk.read(port-0x320);
        }
        else if (port == 0x201)
        {
            data = gameport.read(port-0x201);
        }
        else if (port >= 0x260 && port <= 0x263)
        {
            data = ltems.read(port-0x260);
        }
        else
        {
            //if constexpr(DEBUG_LEVEL > 0)
                //cout << "Reading unknown port " << u32(port) << endl;
            //std::abort();
        }
        if (startprinting)
            cout << "Read  Port 0x" << u32(port) << " <---- 0x" << u32(data) << endl;
        return data;
    }
};

auto divcord_byte(u16 ax, u8 m, u16 startflags)
{
    const u8 bitwidth{8};
    u16 tmpa{}, tmpb{}, tmpc{}, counter{}, flags{startflags}, aluflags{startflags};
    u16 sigma{};
    bool alucarry{};

    u8 al = (ax&0xFF);
    u8 ah = (ax>>8);

    auto setflag = [&](auto flag, bool value)
    {
        aluflags = (aluflags&~flag) | (value?flag:0);
    };

    auto printstate = [&](const char* point)
    {
        //cout << point << " a=" << tmpa << " b=" << tmpb << " c=" << tmpc << " s=" << sigma << " ctr=" << counter << " f=" << flags << " af=" << aluflags << endl;
    };

    enum OPER
    {
        SUBT,
        LRCY,
        COM1
    };
    enum FLAG
    {
        CARRY=(1<<0),
        PARITY=(1<<2),
        AUX_CARRY=(1<<4),
        ZERO=(1<<6),
        SIGN=(1<<7),
        OVERFLOW=(1<<11)
    };

    auto alu = [&](OPER oper, u16 reg1)
    {
        if (oper == COM1)
        {
            alucarry = (reg1&0x80);
            sigma = ~reg1;
        }
        else if (oper == SUBT)
        {
            sigma = reg1-tmpb;
            alucarry = (reg1<tmpb);

            setflag(CARRY,reg1<tmpb);
            setflag(PARITY,byte_parity[sigma&0xFF]);
            setflag(AUX_CARRY, (reg1 ^ tmpb ^ sigma) & 0x10);
            setflag(ZERO,sigma==0);
            setflag(SIGN,sigma&0x80);
            setflag(OVERFLOW,((reg1 ^ tmpb) & (reg1 ^ sigma))&0x80);
        }
        else if (oper == LRCY)
        {
            sigma = reg1 << 1;
            sigma |= (alucarry?1:0);
            alucarry = (reg1&0x80);
        }
    };

    //DIV 0
    printstate("DIV 0");
    tmpa = ah;

    //DIV 1
    printstate("DIV 1");
    tmpc = al;
    alu(LRCY, tmpa);

    //DIV 2
    printstate("DIV 2");
    tmpb = m;

    //DIV 3
    goto CORD_0;

CORD_RTN:

    //DIV 4
    printstate("DIV 4");
    alu(COM1, tmpc);

    //DIV 5
    printstate("DIV 5");
    tmpb = ah;

    //DIV 6
    printstate("DIV 6");
    al = sigma;
    flags = (flags&~CARRY) | (alucarry?CARRY:0);

    //DIV 7
    printstate("DIV 7");
    ah = tmpa;
    return make_tuple(ah, al, flags, false);

    //CORD
    //188
CORD_0:
    printstate("CORD 0");
    alu(SUBT, tmpa);

    //189
    printstate("CORD 1");
    flags = aluflags;
    counter=7; //only byte for now

    //18a
    printstate("CORD 2");
    if (!(flags&CARRY))
    {
        return make_tuple(u8(0),u8(0),flags,true); //simulate int0
    }

    //18b
CORD_3:
    printstate("CORD 3");
    alu(LRCY, tmpc);

    //18c
    printstate("CORD 4");
    tmpc = sigma;
    flags = (flags&~CARRY) | (alucarry?CARRY:0);
    alu(LRCY, tmpa);

    //18d
    printstate("CORD 5");
    tmpa = sigma;
    flags = (flags&~CARRY) | (alucarry?CARRY:0);
    alu(SUBT, tmpa);

    //18e
    printstate("CORD 6");
    if (flags&CARRY)
        goto CORD_13;

    //18f
    printstate("CORD 7");
    sigma; //no destination. here to indicate we're using sigma so we must update carry!
    flags = aluflags;

    //190
    printstate("CORD 8");
    if (!(flags&CARRY))
        goto CORD_14;

    //191
    printstate("CORD 9");
    if (counter-- != 0)
        goto CORD_3;

    //192
CORD_10: //from CORD_15

    printstate("CORD 10");
    alu(LRCY, tmpc);

    //193
    printstate("CORD 11");
    flags = (flags&~CARRY) | (alucarry?CARRY:0);
    tmpc = sigma;

    //194
    printstate("CORD 12");
    flags = (flags&~CARRY) | (alucarry?CARRY:0);
    goto CORD_RTN;

    //195
CORD_13:
    printstate("CORD 13");
    flags = (flags&~CARRY);

    //196
CORD_14:
    printstate("CORD 14");
    flags = (flags&~CARRY) | (alucarry?CARRY:0);
    tmpa = sigma;
    if (counter-- != 0)
        goto CORD_3;

    //197
    printstate("CORD 15");
    goto CORD_10;
}

#include "808x.h"
//#include "80286.h"

unsigned char key_lookup[GLFW_KEY_LAST+1] = {};
void initialize_key_lookup()
{
    key_lookup[GLFW_KEY_ESCAPE] = 0x01;
    key_lookup[GLFW_KEY_1] = 0x02;
    key_lookup[GLFW_KEY_2] = 0x03;
    key_lookup[GLFW_KEY_3] = 0x04;
    key_lookup[GLFW_KEY_4] = 0x05;
    key_lookup[GLFW_KEY_5] = 0x06;
    key_lookup[GLFW_KEY_6] = 0x07;
    key_lookup[GLFW_KEY_7] = 0x08;
    key_lookup[GLFW_KEY_8] = 0x09;
    key_lookup[GLFW_KEY_9] = 0x0A;
    key_lookup[GLFW_KEY_0] = 0x0B;
    key_lookup[GLFW_KEY_MINUS] = 0x0C;
    key_lookup[GLFW_KEY_EQUAL] = 0x0D;
    key_lookup[GLFW_KEY_BACKSPACE] = 0x0E;
    key_lookup[GLFW_KEY_TAB] = 0x0F;
    key_lookup[GLFW_KEY_Q] = 0x10;
    key_lookup[GLFW_KEY_W] = 0x11;
    key_lookup[GLFW_KEY_E] = 0x12;
    key_lookup[GLFW_KEY_R] = 0x13;
    key_lookup[GLFW_KEY_T] = 0x14;
    key_lookup[GLFW_KEY_Y] = 0x15;
    key_lookup[GLFW_KEY_U] = 0x16;
    key_lookup[GLFW_KEY_I] = 0x17;
    key_lookup[GLFW_KEY_O] = 0x18;
    key_lookup[GLFW_KEY_P] = 0x19;
    key_lookup[GLFW_KEY_LEFT_BRACKET] = 0x1A;
    key_lookup[GLFW_KEY_RIGHT_BRACKET] = 0x1B;
    key_lookup[GLFW_KEY_ENTER] = 0x1C;
    key_lookup[GLFW_KEY_LEFT_CONTROL] = 0x1D;
    key_lookup[GLFW_KEY_A] = 0x1E;
    key_lookup[GLFW_KEY_S] = 0x1F;
    key_lookup[GLFW_KEY_D] = 0x20;
    key_lookup[GLFW_KEY_F] = 0x21;
    key_lookup[GLFW_KEY_G] = 0x22;
    key_lookup[GLFW_KEY_H] = 0x23;
    key_lookup[GLFW_KEY_J] = 0x24;
    key_lookup[GLFW_KEY_K] = 0x25;
    key_lookup[GLFW_KEY_L] = 0x26;
    key_lookup[GLFW_KEY_SEMICOLON] = 0x27;
    key_lookup[GLFW_KEY_APOSTROPHE] = 0x28;
    key_lookup[GLFW_KEY_GRAVE_ACCENT] = 0x29;
    key_lookup[GLFW_KEY_LEFT_SHIFT] = 0x2A;
    key_lookup[GLFW_KEY_BACKSLASH] = 0x2B;
    key_lookup[GLFW_KEY_Z] = 0x2C;
    key_lookup[GLFW_KEY_X] = 0x2D;
    key_lookup[GLFW_KEY_C] = 0x2E;
    key_lookup[GLFW_KEY_V] = 0x2F;
    key_lookup[GLFW_KEY_B] = 0x30;
    key_lookup[GLFW_KEY_N] = 0x31;
    key_lookup[GLFW_KEY_M] = 0x32;
    key_lookup[GLFW_KEY_COMMA] = 0x33;
    key_lookup[GLFW_KEY_PERIOD] = 0x34;
    key_lookup[GLFW_KEY_SLASH] = 0x35;
    key_lookup[GLFW_KEY_RIGHT_SHIFT] = 0x36;
    key_lookup[GLFW_KEY_PRINT_SCREEN] = 0x37;
    key_lookup[GLFW_KEY_LEFT_ALT] = 0x38;
    key_lookup[GLFW_KEY_SPACE] = 0x39;
    key_lookup[GLFW_KEY_CAPS_LOCK] = 0x3A;
    key_lookup[GLFW_KEY_F1] = 0x3B;
    key_lookup[GLFW_KEY_F2] = 0x3C;
    key_lookup[GLFW_KEY_F3] = 0x3D;
    key_lookup[GLFW_KEY_F4] = 0x3E;
    key_lookup[GLFW_KEY_F5] = 0x3F;
    key_lookup[GLFW_KEY_F6] = 0x40;
    key_lookup[GLFW_KEY_F7] = 0x41;
    key_lookup[GLFW_KEY_F8] = 0x42;
    key_lookup[GLFW_KEY_F9] = 0x43;
    key_lookup[GLFW_KEY_F10] = 0x44;
    key_lookup[GLFW_KEY_NUM_LOCK] = 0x45;
    key_lookup[GLFW_KEY_SCROLL_LOCK] = 0x46;
    key_lookup[GLFW_KEY_KP_7] = 0x47;
    key_lookup[GLFW_KEY_KP_8] = 0x48;
    key_lookup[GLFW_KEY_KP_9] = 0x49;
    key_lookup[GLFW_KEY_KP_SUBTRACT] = 0x4A;
    key_lookup[GLFW_KEY_KP_4] = 0x4B;
    key_lookup[GLFW_KEY_KP_5] = 0x4C;
    key_lookup[GLFW_KEY_KP_6] = 0x4D;
    key_lookup[GLFW_KEY_KP_ADD] = 0x4E;
    key_lookup[GLFW_KEY_KP_1] = 0x4F;
    key_lookup[GLFW_KEY_KP_2] = 0x50;
    key_lookup[GLFW_KEY_KP_3] = 0x51;
    key_lookup[GLFW_KEY_KP_0] = 0x52;
    key_lookup[GLFW_KEY_KP_DECIMAL] = 0x53;
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

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_1)
        gameport.set_button_state(0, action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_2)
        gameport.set_button_state(1, action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_3)
        gameport.set_button_state(2, action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_4)
        gameport.set_button_state(3, action == GLFW_PRESS);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS && action != GLFW_RELEASE)
        return;
    if (globalsettings.functionkeypress)
    {
        if (action == GLFW_PRESS)
        {
            if (key == GLFW_KEY_S)
            {
                globalsettings.sound_on = !globalsettings.sound_on;
                cout << "Sound " << (globalsettings.sound_on?"on":"off") << endl;
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
                cpu.reset();
            }
            else if (key == GLFW_KEY_G)
            {
                if (cpu.halt)
                {
                    cout << "Force CPU out of halt." << endl;
                    cpu.halt = false;
                }
                else
                    cout << "Not in halt." << endl;
            }
            else if (key == GLFW_KEY_F)
            {
                cout << "Flushing disks." << endl;
                harddisk.disks[0].flush();
                harddisk.disks[1].flush();
                cout << "Disks flushed." << endl;
            }
            else if (key == GLFW_KEY_G)
            {
                cout << "FLAGS: " << endl;
                for(int i=0; i<16; ++i)
                    cout << bool(cpu.registers[cpu.FLAGS]&(1<<i)) << (i%4==3?"  ":" ");
                cout << endl;
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
            else if (key == GLFW_KEY_V)
            {
                if (cga.output == cga.OUTPUT::RGB)
                    cga.output = cga.OUTPUT::COMPOSITE;
                else
                    cga.output = cga.OUTPUT::RGB;
                cout << "Cga output changed to " << (cga.output==cga.OUTPUT::RGB?"RGB.":"composite.") << endl;
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
                    diskettecontroller.drives[chosen_drive].diskette = DISKETTECONTROLLER::Drive::DISKETTE(files[chosen_id]);
                }
                else
                {
                    cout << "Ejecting " << char('A'+chosen_drive) << ":" << endl;
                    diskettecontroller.drives[chosen_drive].diskette.eject();
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
            mem.dump_memory("memory.raw");
        }
        u8 pc_scancode = key_lookup[key];
        if (pc_scancode != 0)
        {
            kbd.press(pc_scancode | (action == GLFW_RELEASE ? 0x80 : 0));
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

void configline(std::string line)
{
    if (line.empty())
        return;
    if (line[0] == '#')
        return;

    std::istringstream iss(line);
    std::string command;
    iss >> command;

    if (command == "rom")
    {
        std::string address_str, filename;
        iss >> address_str >> filename;

        // Convert hex address to integer
        unsigned long address = std::stoul(address_str, nullptr, 16);

        // Read the ROM file
        std::ifstream file(filename, std::ios::binary);
        if (!file)
        {
            std::cerr << "Error: Unable to open ROM file: " << filename << std::endl;
            return;
        }

        // Get file size
        file.seekg(0, std::ios::end);
        std::streampos fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // Read file contents into memory
        if (address + fileSize <= 0x100000)
        {
            file.read(reinterpret_cast<char*>(&mem.memory_bytes[address]), fileSize);
        }
        else
        {
            std::cerr << "Error: ROM file too large or invalid address" << std::endl;
        }
    }
    else if (command == "load")
    {
        char drive_letter;
        std::string image_filename;
        iss >> drive_letter >> image_filename;

        int drive_number = tolower(drive_letter) - 'a';
        if (drive_number >= 0 && drive_number < 2)
        {
            diskettecontroller.drives[drive_number].diskette = DISKETTECONTROLLER::Drive::DISKETTE(image_filename);
        }
        else if (drive_number >= 2 && drive_number < 4)
        {
            cout << "Loading hard disk from " << image_filename << endl;
            harddisk.disks[drive_number-2] = HARDDISK::DISK(image_filename);
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
        else
        {
            cout << "Unknown machine type. Supported machine types: pc,xt" << endl;
            std::abort();
        }
    }
    else if (command == "test")
    {
        readonly_start = 0xFFFF0000;
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
        while(ptr < filedata.size())
        {
            bool test_passed = true;
            CPU8088 testcpu;
            testcpu.reset();
            memset(mem.memory_bytes, 0, 1<<20);

            u16 start_regs[14] = {};
            u16 final_regs[14] = {};

            for(int i=0; i<14; ++i)
            {
                start_regs[i] = data16();
                testcpu.registers[i] = start_regs[i];
            }

            u32 initial_ram_n = data32();
            for(u32 i=0; i<initial_ram_n; ++i)
            {
                u32 address = data32();
                u32 value = data32();
                mem.memory_bytes[address] = value;
            }

            do
            {
                testcpu.cycle();
            } while(testcpu.is_inside_multi_part_instruction);

            for(int i=0; i<14; ++i)
            {
                final_regs[i] = data16();
            }
            for(int i=0; i<14; ++i)
            {
                u16 test_reg = testcpu.registers[i];
                if (final_regs[i] != test_reg)
                {
                    test_passed = false;
                    if (i==testcpu.FLAGS)
                    {
                        for(int flag=0; flag<16; ++flag)
                        {
                            flags_failed[flag] += bool(final_regs[i]&(1<<flag)) ^ bool(test_reg&(1<<flag));
                        }
                    }
                    regs_failed[i] += 1;
                }
                if (i==12 && (test_reg^final_regs[i]))
                {
                    cout << test_filename << "#" << test_id <<std::hex <<  ": AX=" <<final_regs[0]  << " " << r_fullnames[u32(i)] << ": " << start_regs[i] << "->" << final_regs[i] << " cpu gave " << test_reg << " , diff=" << (test_reg^final_regs[i]) << std::dec << endl;
                }
            }

            u32 final_ram_n = data32();
            for(u32 i=0; i<final_ram_n; ++i)
            {
                u32 address = data32();
                u32 value = data32();

                if (mem.memory_bytes[address] != value)
                {
                    test_passed = false;
                    //cout << test_filename << "#" << test_id << ": Memory bytes at " << address << " not correct: " << std::hex << u32(mem.memory_bytes[address]) << "!=" << u32(value) << std::dec << endl;
                }
            }

            if (!test_passed)
                tests_failed += 1, ++tests_totalfailed;
            ++test_id;
            ++tests_totaldone;
        }

        if (tests_failed > 0)
        {   cout << test_filename << ": " << tests_failed << " TESTS FAILED!" << endl;
            cout << "Reg failures:   ";
            for(int i=0; i<14; ++i)
                cout << regs_failed[i] << (i%4==3?"  ":" ");
            cout << endl;
            cout << "Flag failures:   ";
            for(int i=0; i<16; ++i)
                cout << flags_failed[i] << (i%4==3?"  ":" ");
            cout << endl;
        }
        readonly_start = 0xF0000;
    }
    else if (command == "end_tests")
    {
        cout << "tests failed: " << tests_totalfailed << "/" << tests_totaldone << endl;
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

void gamepadbuttonfun(int joy_id, int key_id, InputEventSTATE action)
{
    if (key_id >= 0 && key_id <= 3)
    {
        gameport.set_button_state(key_id, action == InputEventSTATE::DOWN);
    }
}
void gamepadaxisfun([[maybe_unused]] int joy_id, int axis_id, int amount)
{
    if (axis_id >= 0 && axis_id <= 3)
    {
        gameport.axes[axis_id] = amount;
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

int main(int argc, char* argv[])
{
    //glfwInit();
    initialize_key_lookup();

    std::string configFilename = "config.txt";
    if (argc > 1)
    {
        configFilename = argv[1];
    }

    readConfigFile(configFilename);
    screen.SCREEN_start();

    Opl2::Init();

    startTime = glfwGetTime();

    std::cout << std::uppercase << std::hex;

    glfwSetKeyCallback(screen.window, key_callback);
    glfwSetMouseButtonCallback(screen.window, mouse_button_callback);
    for(int jid=GLFW_JOYSTICK_1; jid<GLFW_JOYSTICK_LAST; ++jid)
    {
        if (glfwJoystickPresent(jid))
        {
            joystickfun(jid, GLFW_CONNECTED);
        }
    }
    glfwSetJoystickCallback(joystickfun);
    cpu.reset();

    double previousTime=0.0;

    u64 loop_counter=0, clockgen_fast=0, clockgen_real=0;
    while(true)
    {
        //the loop is ca. ~14.31818 MHz
        ++loop_counter;

        if (!lockstep || turbo)
        {
            ++clockgen_fast;

            if (clockgen_fast%3 == 0)
            {
                cpu.cycle();
                ++cpu.cpu_steps;
                pic.cycle();
                for(u8 irq=0; irq<8; ++irq)
                {
                    if (pic.isr&(1<<irq))
                    {
                        if constexpr(DEBUG_LEVEL > 0)
                            cout << "IRQ: ATTEMPT TO CPU " << u32(irq) << " int flag=" << u32(cpu.flag(cpu.F_INTERRUPT)) << endl;
                        if (cpu.accepts_interrupts())
                        {
                            if constexpr(DEBUG_LEVEL > 0)
                                cout << "IRQ: SENT TO CPU " << u32(irq) << endl;
                            cpu.irq(irq);
                            break;
                        }
                    }
                }
                kbd.cycle();
                diskettecontroller.cycle();
                harddisk.cycle();
                if (clockgen_fast%4 == 0) //we're already inside %3 so this makes for %12
                    dma.cycle();
            }
            if (clockgen_fast%288)
            {
                ym3812.cycle_timers();
            }

            //realtime stuff
            if (lockstep) //implies turbo==true
            {
                if (clockgen_fast%8 == 0)
                    cga.cycle();
                if (clockgen_fast%12 == 0)
                    pit.cycle();
                if (clockgen_fast%298 == 0) //ca. 48kHz. handles sound output in general
                    beeper.cycle();
                if (clockgen_fast%288 == 0)
                    ym3812.cycle();
                if (clockgen_fast%GAMEPORT_CYCLE == 0)
                    gameport.cycle();
            }
        }

        //do realtime stuff
        if (!turbo && (loop_counter&0x1F) == 0) //calculate how many cycles we need to do
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
                    if (clockgen_real%3 == 0)
                    {
                        cpu.cycle();
                        ++cpu.cpu_steps;
                        pic.cycle();
                        for(u8 irq=0; irq<8; ++irq)
                        {
                            if (pic.isr&(1<<irq))
                            {
                                if constexpr(DEBUG_LEVEL > 0)
                                    cout << "IRQ: ATTEMPT TO CPU " << u32(irq) << " int flag=" << u32(cpu.flag(cpu.F_INTERRUPT)) << endl;
                                if (cpu.accepts_interrupts())
                                {
                                    if constexpr(DEBUG_LEVEL > 0)
                                        cout << "IRQ: SENT TO CPU " << u32(irq) << endl;
                                    cpu.irq(irq);
                                    break;
                                }
                            }
                        }
                        kbd.cycle();
                        diskettecontroller.cycle();
                        harddisk.cycle();
                        if (clockgen_real%4 == 0) //we're already inside %3 so this makes for %12
                            dma.cycle();
                    }
                }
                //realtime stuff
                if (clockgen_real%8 == 0)
                    cga.cycle();
                if (clockgen_real%12 == 0)
                    pit.cycle();
                if (clockgen_real%298 == 0) //ca. 48kHz. handles sound output in general
                    beeper.cycle();
                if (clockgen_real%288 == 0)
                {
                    ym3812.cycle();
                    ym3812.cycle_timers();
                }
                if (clockgen_real%GAMEPORT_CYCLE == 0)
                    gameport.cycle();
            }
            previousTime += double(cycles_done)/14318180.0;
        }
        if ((loop_counter&0xFFFF) == 0)
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
                cout << cpu.cpu_steps*3/14318180.0 << "x realtime ";
                cout << std::dec << cpu.cpu_steps/1000000.0 << std::hex << " MHz ";
                //cout << std::dec << totalframes << " Hz audio " << std::hex;
                cout << std::dec << cga.totalvsync << " Hz vsync, " << std::hex;
                cout << std::hex << "flags=" << cpu.registers[cpu.FLAGS] << " " << std::hex;
                cout << "halt=" << cpu.halt << " ";

                cout << endl;

                harddisk.disks[0].flush();
                harddisk.disks[1].flush();
                cpu.cpu_steps = 0;
                totalframes = 0;
                cga.totalvsync = 0;
                pit.int0_count = 0;

                /*for(int i=0; i<256; ++i)
                {
                    if (cpu.interrupt_table[i] != 0)
                    {
                        cout << std::hex << "int" << i << "=" << std::dec << cpu.interrupt_table[i] << std::hex << "Hz ";
                        cpu.interrupt_table[i] = 0;
                    }
                }
                cout << endl;*/
            }
        }

    }
    Opl2::Quit();
    glfwTerminate();
}
