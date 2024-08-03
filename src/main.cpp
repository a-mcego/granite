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

bool realtime_timing = false;
bool lockstep = false;

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

u8 memory_bytes[(1<<20)+65536+1] = {};
void dump_memory(const char* filename)
{
    FILE* filu = fopen(filename, "wb");
    fwrite(memory_bytes, 0x100000, 1, filu);
    fclose(filu);
}

u16 extrawords[256] = {};
u8 extraword{};
u8 extrabytes[256] = {};
u8 extrabyte{};

u32 readonly_start = 0xC0000;

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
    u8 oplcycles{}; //for synchronizing sound

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
            else if (current_register == 0x04)
            {
                if (data&0x80)
                {
                    status &= 0x80;
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

struct CGA
{
    static const u8 REGISTER_COUNT = 18;
    u8 registers[REGISTER_COUNT] = {};
    u8 current_register{};
    u8 mode_select{};
    u8 color_select{};

    u8 mem[0x4000 + 1] = {};
    u8& memory8(u16 address)
    {
       return mem[address&0x3FFF];
    }
    u16& memory16(u16 address)
    {
       return *(u16*)(void*)(mem+(address&0x3FFF));
    }
    void print_regs()
    {
        for(int i=0; i<16; ++i)
            cout << u32(registers[i]) << " ";
        cout << endl;
    }

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
    u16 current_startaddress{};
    u8 current_modeselect{};
    u8 vcc{};

    void render()
    {
        screen.render();
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
                //readdata = registers[(mode_id()<<4)+current_register];
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
            readdata |= horizontal_retrace|vertical_retrace;
            //bit 1 = light pen triggered (vs 0 = armed)
            //bit 2 = light pen switch open (vs 0 = closed)
            //bit 3 = vertical sync pulse!
            readdata |= (vertical_retrace<<3);
        }

        return readdata;
    }

    void write(u8 port, u8 data) //port from 0 to 15! inclusive.
    {
        if constexpr (DEBUG_LEVEL > 0)
        {
            cout << "CGA WRITE! " << u32(port) << " <- " << u32(data) << endl;
        }
        if (port == 0x04)
        {
            current_register = data;
        }
        else if (port == 0x05)
        {
            if (current_register < 0x10)
            {
                registers[current_register] = data;
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

    u32 cycle_n{};
    void cycle()
    {
        cycle_n += 8;
        cycle_n -= (cycle_n>=912*262?912*262:0);
        if (cycle_n == 262*640)
        {
            ++total_frames;
            render();
        }

        u32 line = cycle_n/912;
        u32 column = cycle_n%912;

        u32 columnbyte = column/8;

        if (line < 200)
        {
            if (column == 0 && line==0)
            {
                current_startaddress = ((registers[START_ADDRESS_H]<<8) | registers[START_ADDRESS_L])*2;
                current_modeselect = mode_select;
            }
            //cout << line << ": "; print_regs();
            u8 textmode_40_80 = (current_modeselect>>0)&0x01;
            u8 is_graphics_mode = (current_modeselect>>1)&0x01;
            u8 no_colorburst = (current_modeselect>>2)&0x01;
            u8 resolution = (current_modeselect>>4)&0x01;
            u16 base_offset = current_startaddress;
            u8 max_scanline = registers[MAX_SCAN_LINE]+1;

            const u8 add = ((color_select&0x10)?8:0) + ((color_select&0x20)?1:0);
            const u8 palette[4] = {u8(color_select&0x0F), u8(2+add), u8(4+add), u8(6+add)};

            int y = line;
            if (is_graphics_mode)
            {
                if (resolution && !no_colorburst)
                {
                    int x = columnbyte;
                    if (x < registers[H_DISPLAYED]*2)
                    {
                        u8 gfx_byte = memory8((base_offset+(y&1?0x2000:0))+(y>>1)*registers[H_DISPLAYED]*2+x);
                        const u8 colorburst[16] =
                        {
                            0, 120, 1, 3, 108, 24, 35, 80,
                            188, 47, 24, 72, 41, 43, 60, 15
                        };

                        for(int i=0; i<2; ++i)
                        {
                            u8 p1 = colorburst[(gfx_byte&0xF0)>>4];
                            screen.pixels[y*screen.X + x*8 + 4*i] = p1;
                            screen.pixels[y*screen.X + x*8 + 4*i+1] = p1;
                            screen.pixels[y*screen.X + x*8 + 4*i+2] = p1;
                            screen.pixels[y*screen.X + x*8 + 4*i+3] = p1;
                            gfx_byte <<= 4;
                        }
                    }

                }
                else
                {
                    int x = columnbyte;
                    if (x < registers[H_DISPLAYED]*2)
                    {
                        u8 gfx_byte = memory8((base_offset+(y&1?0x2000:0))+(y>>1)*registers[H_DISPLAYED]*2+x);
                        for(int i=0; i<4; ++i)
                        {
                            u8 p1 = (resolution?((gfx_byte&0x80)?15:0):palette[(gfx_byte&0xC0)>>6]);
                            u8 p2 = (resolution?((gfx_byte&0x40)?15:0):p1);
                            screen.pixels[y*screen.X + x*8 + 2*i] = p1;
                            screen.pixels[y*screen.X + x*8 + 2*i+1] = p2;
                            gfx_byte <<= 2;
                        }
                    }
                }
            }
            else
            {
                int x = columnbyte;
                {
                    if (x < registers[H_DISPLAYED])
                    {
                        u32 screen_row = y/max_scanline;
                        u32 current_line = y%max_scanline;

                        u32 offset = base_offset + (screen_row * registers[H_DISPLAYED] + x) * 2;
                        u8 char_code = memory8(offset);
                        u8 attribute = memory8(offset+1);
                        u8 fg_color = attribute & 0x0F;
                        u8 bg_color = (attribute >> 4) & 0x0F;
                        u8 char_row = CGABIOS[char_code*8+current_line];

                        for (u32 x_off = 0; x_off < 8; x_off++)
                        {
                            u32 pixel_x = x * 8 + x_off;
                            u8 color = (char_row & (1 << (7 - x_off))) ? fg_color : bg_color;
                            if (textmode_40_80)
                            {
                                screen.pixels[y * screen.X + pixel_x] = color; //screen.pixels is one byte per pixel
                            }
                            else
                            {
                                screen.pixels[y * screen.X + pixel_x*2] = color;
                                screen.pixels[y * screen.X + pixel_x*2+1] = color;
                            }
                        }
                    }
                }
            }
        }

        horizontal_retrace = (cycle_n%912 >= 640);
        vertical_retrace = (cycle_n >= 912*200);
    }
} cga;

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
        sampleA = (value&pb1)?60*256:0;
        sampleB = ((sampleB<<5)-sampleB+sampleA)>>5; //crude lowpass
        sampleC = ((sampleC<<5)-sampleC+sampleB)>>5; //crude lowpass
    }

    void cycle()
    {

        //i16 state = ((pb1&timer)?60*256:0)*(pb0?-1:1) * (divisor>=64?1:0);
        i16 state = sampleC*(pb0?-1:1);// * (divisor>=64?1:0);

        float prev_weight = float(ym3812.oplcycles)*(1.0f/24.0f);

        i32 data = state+i16(prev_weight*ym3812.previous_sample)+i16((1.0f-prev_weight)*ym3812.sample);
        data = (data<-32768?-32768:data);
        data = (data>32767?32767:data);
        buffer[write_offset] = globalsettings.sound_on?i16(data):i16(0);
        ++write_offset;
    }
} beeper;

void miniaudio_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    u16 offset_end = beeper.write_offset;
    u16 offset_start = beeper.read_offset;
    u32 done_count = offset_end-offset_start;
    if (frameCount == 0 || done_count == 0)
        return;

    //METHOD 1: always take the N last frames. might lead to misses or repetition, but has stable pitch!
    offset_start = offset_end-frameCount;
    i16* pi16Output = (i16*)pOutput;
    for(u32 done_frames=0; done_frames<frameCount; ++done_frames)
    {
        i16 data = beeper.buffer[u16(offset_start+done_frames)];
        *pi16Output = data;
        ++pi16Output;
    }

    //METHOD 2: resample the M last frames to fit into frameCount. always uses every sample but has unstable pitch!
    /*u64 frame_counter=0;
    i16* pi16Output = (i16*)pOutput;
    for(u32 done_frames=0; done_frames<frameCount; ++done_frames)
    {
        i16 data = beeper.buffer[u16(offset_start+frame_counter/frameCount)];
        *pi16Output = data;
        ++pi16Output;
        frame_counter += done_count;
    }
    beeper.read_offset += frame_counter/frameCount;*/
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
        deviceConfig.sampleRate        = 48000;
        deviceConfig.dataCallback      = miniaudio_data_callback;

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
                    if constexpr (DEBUG_LEVEL > 0)
                        cout << __PRETTY_FUNCTION__ << ":" << std::dec << __LINE__ << std::hex << endl;
                }
                else // OCW2
                {
                    ocw[2] = data;
                    if (data & 0x20) // End of Interrupt (EOI)
                    {
                        isr &= ~(1 << (data & 0x07));
                        if constexpr (DEBUG_LEVEL > 0)
                            cout << __PRETTY_FUNCTION__ << ":" << std::dec << __LINE__ << std::hex << endl;
                    }
                    if constexpr (DEBUG_LEVEL > 0)
                        cout << __PRETTY_FUNCTION__ << ":" << std::dec << __LINE__ << std::hex << endl;
                }
            }
        }
        else if (port == 1)
        {
            if (init_state == 1) // ICW2
            {
                icw[2] = data;
                init_state = 3; //TODO: support multiple DMA chips. we skip ICW3 when there's only one
                if constexpr (DEBUG_LEVEL > 0)
                    cout << __PRETTY_FUNCTION__ << ":" << std::dec << __LINE__ << std::hex << endl;
            }
            else if (init_state == 2) // ICW3
            {
                icw[3] = data;
                init_state = 3;
                if constexpr (DEBUG_LEVEL > 0)
                    cout << __PRETTY_FUNCTION__ << ":" << std::dec << __LINE__ << std::hex << endl;
            }
            else if (init_state == 3) // ICW4
            {
                icw[4] = data;
                init_state = 0;
                is_initialized = true;
                if constexpr (DEBUG_LEVEL > 0)
                    cout << __PRETTY_FUNCTION__ << ":" << std::dec << __LINE__ << std::hex << endl;
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
            if (!masked(i) && pending(i))
            {
                if constexpr(DEBUG_LEVEL > 0)
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

    void request_interrupt(u8 irq)
    {
        if (!is_initialized || irq >= 8)
            return;

        if constexpr(DEBUG_LEVEL > 0)
            cout << "IRQ: REQUEST " << u32(irq) << endl;
        irr |= (1<<irq);
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

    void press(u8 scancode)
    {
        if (is_initialized)
        {
            current_scancode = scancode;
            pic.request_interrupt(1);
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
                    current_scancode = 0;
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
        if (port == 1 && (data&0x40) && (!(regs[port]&0x40)))
        {
            cout << "Setting keyboard self test." << endl;
            if (keyboard_self_test == 0)
            {
                keyboard_self_test = KEYBOARD_SELF_TEST_LENGTH;
                is_initialized = false;
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
    }
} kbd;

struct CHIP8237 //DMA
{
    struct Channel
    {
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
                memory_bytes[((page<<16)+curr_addr)&0xFFFFF] = (*device_vector)[curr_vector_offset];
            }
            else if (transfer_direction == DIR_FROM_MEMORY)
            {
                (*device_vector)[curr_vector_offset] = memory_bytes[((page<<16)+curr_addr)&0xFFFFF];
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
            ++curr_addr;
            ++curr_vector_offset;
            if (curr_count == 0)
            {
                pending = false;
                is_complete = true;
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

    void print_params(u8 channel)
    {
        Channel& c = chans[channel];
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
            }
        }
        else
        {
            std::cout << "Unsupported DMA port " << u32(port) << endl;
            std::abort();
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
                std::cout << "Unsupported DMA port " << u32(port) << endl;
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
        if constexpr (DEBUG_LEVEL > 1)
            cout << "READ PIT---- " << u32(port) << endl;
        if (port == 3) //can't read port 3
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

        cout << __PRETTY_FUNCTION__ << " WTF. reading port: " << u32(port) << endl;
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
                    cout << "port " << u32(port) << " ACCESS MODE " << u32(c.access_mode) << ": new data " << c.reload << endl;
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
                    c.reload |= (data<<8);
                    c.stopped = false;
                }
                else
                {
                    c.reload = data;
                    c.stopped = true;
                }
                if constexpr (DEBUG_LEVEL > 0)
                    cout << "port " << u32(port) << " ACCESS MODE " << u32(c.access_mode) << ": new data " << c.reload << " & " << c.current << endl;
                c.write_wait_for_second_byte = !c.write_wait_for_second_byte;
            }
            if constexpr (DEBUG_LEVEL > 0)
                cout << "--- operating mode " << u32(c.operating_mode) << endl;

            if (c.operating_mode == 0 || c.operating_mode == 2)
            {
                c.current = c.reload;
                c.output = false;
                c.stopped = false;
            }
        }
    }


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
                        pic.request_interrupt(0);
                    }
                    c.output = true;
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
                        pic.request_interrupt(0);
                    }
                    else if (i==1 && globalsettings.machine == globalsettings.MACHINE_XT)
                    {
                        //cout << "INITIATE TRANSFER XT 1" << endl;
                        dma.chans[0].initiate_transfer();
                        dma.chans[0].start_addr += 1;
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
                c.current -= 2;
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
                            pic.request_interrupt(0);
                    }
                    else if (i==1 && globalsettings.machine == globalsettings.MACHINE_XT)
                    {
                        cout << "INITIATE TRANSFER XT 2" << endl;
                        dma.chans[0].initiate_transfer();
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


void PrintCSIP();

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
        vector<u8> data, dirty;
        std::string filename = "cdisk.img";

        void flush_complete()
        {
            FILE* filu = fopen(filename.c_str(), "wb");
            fwrite(data.data(), data.size(), 1, filu);
            fclose(filu);
        }

        void flush()
        {
            FILE* filu = fopen(filename.c_str(), "rb+");

            if (filu == nullptr)
            {
                flush_complete();
                return;
            }

            for(int i=0; i<i32(dirty.size()); ++i)
            {
                if (dirty[i])
                {
                    fseek(filu,i*DiskType::BYTES_PER_SECTOR,SEEK_SET);
                    fwrite(data.data()+i*512,512,1,filu);
                    dirty[i] = 0;
                }
            }
            fclose(filu);
        }

        DISK()
        {
            data.assign(type.totalsize(),0);
            dirty.assign(type.totalsize()/512, 0);
            cout << "HD: " << data.size() << " bytes." << endl;
        }

        DISK(const std::string& filename_):filename(filename_)
        {
            data.assign(type.totalsize(),0);
            dirty.assign(type.totalsize()/512, 0);

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
    } disk;

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

        address_valid = disk.type.is_valid(current_cylinder,current_head,current_sector);
    }

    void write(u8 port, u8 data) //port from 0 to 3! inclusive.
    {
        if (port == 0) // data port
        {
            if (do_drive_characteristics)
            {
                r1_iomode = IO_B;
                r1_req = true;
                cout << "HD: doing more drive characteristics! c[" << u32(dc_index) << "] = " << u32(data) << endl;
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
                    cout << "HD: All data collected! ";
                    for(int i=0; i<6; ++i)
                        cout << u32(data_in[i]) << ' ';
                    cout << endl;
                    print_data_in();

                    if (false);
                    else if (data_in[0] == READ)
                    {
                        set_current_params();
                        u32 offset = disk.type.get_byte_offset(current_cylinder, current_head, current_sector);
                        cout << "HD READ offset: " << offset << endl;
                        if (address_valid)
                        {
                            dma.print_params(3);
                            dma.transfer(3, &disk.data, offset);
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
                        u32 offset = disk.type.get_byte_offset(current_cylinder, current_head, current_sector);
                        cout << "HD WRITE offset: " << offset << endl;
                        if (address_valid)
                        {
                            if (dma.chans[3].transfer_count != 0x1FF)
                            {
                                cout << "----------------Transfer count: " << dma.chans[3].transfer_count << endl;
                            }

                            dma.print_params(3);
                            dma.transfer(3, &disk.data, offset);
                            for(int i=0; i<dma.chans[3].transfer_count/512+1; ++i) //TODO: move this where the dma is actually finished
                            {
                                disk.dirty[offset/512+i] = 1;
                            }
                            dma_in_progress = true;
                        }
                        else
                        {
                            interrupttime = 0x300;
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
            cout << "HD WRITE: reset controller" << endl;
            //startprinting = true;
            error=false;
            r1_busy = false;
            r1_int_occurred = false;
            r1_iomode = IO_A;
            current_data_in_index = 0;
        }
        else if (port == 2) //generate controller-select pulse (?)
        {
            cout << "HD WRITE: controller select pulse! unn tss unn tss" << endl;
            //idk
        }
        else if (port == 3)
        {
            dma_enabled = (data&0x01);
            irq_enabled = (data&0x02);
            cout << "HD WRITE: dma=" << (dma_enabled?"enabled":"disabled") << " irq=" << (irq_enabled?"enabled":"disabled") << endl;
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
            cout << "HD READ hw status: " << u32(data) << endl;
        }
        else if (port == 2) //switch settings
        {
            data = 0b0101; //both drives type 2 (note inverted logic)
            cout << "HD READ switch: " << u32(data) << endl;
            r1_req = true;
        }
        else
        {
            cout << "HD READ: unknown port " << u32(port) << endl;
            std::abort();
        }
        cout << "HD READ total=" << u32(data) << endl;
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
                cout << "HD IRQ AFTER DMA!!!" << endl;
                r1_int_occurred = true;
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
                    cout << "HD IRQ!!!" << endl;
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
            out_buffer.push_back(sector + (dma.chans[2].transfer_count)/512);
            out_buffer.push_back(2);
            cout << "DMA COMPLETE lol. interrupt 6. did " << dma.chans[2].transfer_count << " bytes aka " << (dma.chans[2].transfer_count)/512+1 << " sectors" << endl;
        }
    }
} diskettecontroller;

struct IO
{
    static void out(u16 port, u16 data)
    {
        if constexpr (DEBUG_LEVEL > 1)
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
        else
        {
            if constexpr(DEBUG_LEVEL > 0)
                cout << "Unknown port " << u32(port) << endl;
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
        else
        {
            if constexpr(DEBUG_LEVEL > 0)
                cout << "Unknown port " << u32(port) << endl;
            //std::abort();
        }
        if constexpr (DEBUG_LEVEL > 1)
            cout << "Read  Port 0x" << u32(port) << " <---- 0x" << u32(data) << endl;
        return data;
    }
};

std::vector<u32> opcodes_used(256,0);

u32 current_delay = 9;

struct CPU8088
{
    u16 registers[16] = {};

    u8 segment_override{};
    u8 string_prefix{};
    u8 lock{};
    u8 delay{}; //HACK: to make the cpu slow down a bit so it passes POST lol.
    enum SP_VALUES
    {
        SP_REPNZ = 1,
        SP_REPZ = 2 //also REP
    };

    enum REG
    {
        AX,CX,DX,BX, SP,BP,SI,DI, //normal registers
        ES,CS,SS,DS,              //segment registers
        FLAGS,                    //flags, duh
        IP                        //instruction pointer
    };

    enum FLAG
    {
        F_CARRY=0,
        F_PARITY=2,
        F_AUX_CARRY=4,
        F_ZERO=6,
        F_SIGN=7,
        F_TRAP=8,
        F_INTERRUPT=9,
        F_DIRECTIONAL=10,
        F_OVERFLOW=11
    };

    void print_flags()
    {
#define pflag(x) std::cout << #x << ": " << bool(registers[FLAGS]&u32(1<<x)) << std::endl;
        pflag(F_CARRY)
        pflag(F_PARITY)
        pflag(F_AUX_CARRY)
        pflag(F_ZERO)
        pflag(F_SIGN)
        pflag(F_OVERFLOW)
#undef pflag
    }

    void set_flag(FLAG f_n, bool value)   { registers[FLAGS] = (registers[FLAGS]&~(1<<f_n))|(value?1<<f_n:0); }

    bool flag(FLAG f_n)
    {
        return registers[FLAGS]&(1<<f_n);
    }

    void reset()
    {
        clear_prefix();
        for(u32 i=0; i<16; ++i)
            registers[i] = 0x0000;
        registers[CS] = ~registers[CS]; //set code segment to 0xFFFF for reset
    }

    u8& memory8(u16 segment, u16 index)
    {
        u32 total_address = (((segment<<4)+index)&0xFFFFF);
        if (total_address >= readonly_start)
        {
            ++extrabyte;
            extrabytes[extrabyte] = memory_bytes[total_address];
            return extrabytes[extrabyte];
        }

        if (readonly_start < 0x100000 && (total_address&0xF8000) == 0xB8000)
            return cga.memory8(total_address&0x7FFF);
       return memory_bytes[total_address];
    }
    u16& memory16(u16 segment, u16 index)
    {
        u32 total_address = (((segment<<4)+index)&0xFFFFF);
        if (total_address >= readonly_start)
        {
            ++extraword;
            extrawords[extraword] = *(u16*)(void*)(memory_bytes+total_address);
            return extrawords[extraword];
        }
        if (readonly_start < 0x100000 && (total_address&0xF8000) == 0xB8000)
            return cga.memory16(total_address&0x7FFF);
        return *(u16*)(void*)(memory_bytes+total_address);
    }

    static const u32 PREFETCH_QUEUE_SIZE = 8;
    u8 prefetch_queue[PREFETCH_QUEUE_SIZE] = {};
    u32 prefetch_address{};

    template<typename T>
    T read_inst() requires integral<T>
    {
        if constexpr(PREFETCH_QUEUE_SIZE == 0)
        {
            u32 position = ((registers[CS]<<4) + registers[IP])&0xFFFFF;
            T data = *(T*)(memory_bytes+position);
            registers[IP] += sizeof(T);
            return data;
        }
        T result{};
        u32 position = ((registers[CS]<<4) + registers[IP])&0xFFFFF;
        if (prefetch_address != position)
        {
            prefetch_address = position;
            for(u32 i=0; i<PREFETCH_QUEUE_SIZE; ++i)
            {
                prefetch_queue[i] = memory8(registers[CS], registers[IP]+i);
            }
        }

        result = *(T*)(prefetch_queue);
        prefetch_address += sizeof(T);
        registers[IP] += sizeof(T);

        for(u32 i=0; i<PREFETCH_QUEUE_SIZE-sizeof(T); ++i)
        {
            prefetch_queue[i] = prefetch_queue[i+sizeof(T)];
        }
        for(u32 i=PREFETCH_QUEUE_SIZE-sizeof(T); i<PREFETCH_QUEUE_SIZE; ++i)
        {
            prefetch_queue[i] = memory8(registers[CS], registers[IP]+i);
        }
        if (startprinting)
        {
            //cout << (sizeof(T)==2?"w":"b") << u32(result) << " ";
        }
        return result;
    }

    template<typename T>
    void commonflags(T a, T b, T result) requires std::same_as<T,u8> || std::same_as<T,u16>
    {
        set_flag(F_ZERO, result == 0);
        set_flag(F_SIGN, result >> (sizeof(T)*8-1));
        set_flag(F_PARITY, byte_parity[result & 0xFF]);
        set_flag(F_AUX_CARRY, ((a ^ b ^ result) & 0x10) != 0);
    }
    template<typename T>
    void cmp_flags(T a, T b, T result) requires std::same_as<T,u8> || std::same_as<T,u16>
    {
        commonflags(a,b,result);
        set_flag(F_OVERFLOW, ((a ^ b) & (a ^ result)) >> (sizeof(T)*8-1));
        set_flag(F_CARRY, a < b);
    }
    template<typename T>
    void test_flags(T result) requires std::same_as<T,u8> || std::same_as<T,u16>
    {
        commonflags(T(0),T(0),result);
        set_flag(F_OVERFLOW, false);
        //AUX_CARRY left undefined - so we set it in commonflags
        set_flag(F_AUX_CARRY, false); //set it false here to pass 0x0A test
        set_flag(F_CARRY, false);
    }
    template<typename T>
    void add_flags(T a, T b, T result) requires std::same_as<T,u8> || std::same_as<T,u16>
    {
        commonflags(a,b,result);
        set_flag(F_OVERFLOW, ((a ^ result) & (b ^ result)) >> (sizeof(T)*8-1));
        set_flag(F_CARRY, result < a);
    }

    void decode_modrm(u8 mod, u8 rm, u16& segment, u16& offset)
    {
        offset = 0;
        segment = DS;

		if (mod == 0x1)
			offset = i16(read_inst<i8>());
		else if (mod == 0x2)
			offset = read_inst<u16>();

		if (mod == 0x00 && rm == 0x06)
			offset += read_inst<u16>();
		else
		{
			if (rm < 0x06)
				offset += registers[SI+(rm&0x01)]; //DI is after SI
			if (((rm+1)&0x07) <= 2)
				offset += registers[BX];
			if ((rm&0x02) && rm != 7)
				offset += registers[BP], segment = SS;
		}
        segment = registers[get_segment(segment)];
    }

    u8& decode_modrm_u8(u8 modrm)
    {
        u8 mod = (modrm >> 6) & 0x03;
        u8 rm = modrm & 0x07;
		if (mod == 0x03)
			return get_r8(rm);
        u16 offset{}, segment{};
        decode_modrm(mod,rm,segment,offset);
        if constexpr (DEBUG_LEVEL > 1)
            cout << "MEM8! " << segment << ":" << offset << " has " << u32(memory8(segment, offset)) << " prm=" << u32(mod) << "," << u32(rm) << endl;
        return memory8(segment, offset);
    }

    u16& decode_modrm_u16(u8 modrm)
    {
        u8 mod = (modrm >> 6) & 0x03;
        u8 rm = modrm & 0x07;
		if (mod == 0x03)
			return get_r16(rm);
        u16 offset{}, segment{};
        decode_modrm(mod,rm,segment,offset);
        if constexpr (DEBUG_LEVEL > 1)
            cout << "MEM16! " << segment << ":" << offset << " has " << memory16(segment, offset) << " prm=" << u32(mod) << "," << u32(rm) << endl;
        return memory16(segment, offset);
    }

    u16 effective_address(u8 modrm)
    {
        u8 mod = (modrm >> 6) & 0x03;
        u8 rm = modrm & 0x07;
		if (mod == 0x03)
        {
            cout << "Loading effective address of a register? are you gone mad?" << endl;
            std::abort();
        }
        u16 offset{}, segment{};
        decode_modrm(mod,rm,segment,offset);
        if constexpr (DEBUG_LEVEL > 1)
            cout << "LEA! " << segment << ":" << offset << " has " << memory16(segment, offset) << " prm=" << u32(mod) << "," << u32(rm) << endl;
        return offset;
    }


    u8* reg8() { return (u8*)(void*)registers; }

    u16& get_r16(u8 value)
    {
        return registers[value&0x7];
    }
    u8& get_r8(u8 value)
    {
        return reg8()[((value&0x3)<<1)+((value&0x4)>>2)];
    }

    u16& get_segment_r16(u8 value)
    {
        return registers[(value&0x03)+8]; //&0x03 for 8086 compatibility
    }

    void print_regs()
    {
        for(int i=0; i<8; ++i)
            std::cout << " " << r16_names[i] << "=" << std::setw(4) << std::setfill('0') << registers[i];
        std::cout << endl;
        cout << "                    ";
        for(int i=0; i<4; ++i)
            std::cout << " " << seg_names[i] << "=" << std::setw(4) << std::setfill('0') << registers[i+8];
        std::cout << " FL=" << std::setw(4) << std::setfill('0') << registers[FLAGS] << " IP=" << std::setw(4) << std::setfill('0') << registers[IP]-1;

        std::cout << " S ";
        std::cout << std::setw(4) << std::setfill('0') << memory16(registers[SS],registers[SP]) << ' ';
        std::cout << std::setw(4) << std::setfill('0') << memory16(registers[SS],registers[SP]+2) << ' ';
        std::cout << std::setw(4) << std::setfill('0') << memory16(registers[SS],registers[SP]+4) << ' ';
        std::cout << std::setw(4) << std::setfill('0') << memory16(registers[SS],registers[SP]+6) << ' ';
        std::cout << std::endl;
    }

    template<typename T>
    T run_arith(T p1, T p2, u8 instr_choice)
    {
        T out{};

        switch(instr_choice)
        {
            case 0x00: //ADD
                out = p1+p2;
                add_flags(p1,p2,out);
                break;
            case 0x02: //ADC
                out = p1+p2+flag(F_CARRY);
                commonflags(p1,p2,out);
                set_flag(F_AUX_CARRY, (p1&0xF)+(p2&0xF)+flag(F_CARRY) >= 0x10);
                set_flag(F_OVERFLOW, ((p1 ^ out) & (p2 ^ out)) >> (sizeof(T)*8-1));
                set_flag(F_CARRY, ((p1+p2+flag(F_CARRY))>>(sizeof(T)*8)) > 0);
                break;
            case 0x01: //OR
                out = p1|p2;
                break;
            case 0x04: //AND
                out = p1&p2;
                break;
            case 0x03: //SBB
                out = p1-(p2+flag(F_CARRY));
                commonflags(p1,p2,out);
                set_flag(F_AUX_CARRY, (p1&0xF)-((p2&0xF)+flag(F_CARRY)) < 0x00);
                set_flag(F_OVERFLOW, ((p1 ^ p2) & (p1 ^ out)) >> (sizeof(T)*8-1));
                set_flag(F_CARRY, ((p1-(p2+flag(F_CARRY)))>>(sizeof(T)*8)) < 0);
                break;
            case 0x05: //SUB
            case 0x07: //CMP
                out = p1-p2;
                cmp_flags(p1,p2,out);
                break;
            case 0x06://XOR
                out = p1^p2;
                break;
        }

        if (instr_choice == 0x01 || instr_choice == 0x04 || instr_choice == 0x06) //or and xor
            test_flags(out);

        if (instr_choice == 0x07) //cmp
            out = p1;
        return out;
    }

    u8 get_segment(u8 default_segment)
    {
        return segment_override?segment_override:default_segment;
    }

    void clear_prefix()
    {
        segment_override = 0;
        string_prefix = 0;
        lock = 0;
    }
    bool set_prefix(u8 instruction)
    {
        if ((instruction&0xE7) == 0x26)
        {
            segment_override = ((instruction>>3)&0x3)|0x8;
            return true;
        }
        if ((instruction&0xFE) == 0xF2)
        {
            string_prefix = 1+(instruction&0x01); //REPNZ REPZ
            return true;
        }
        if (instruction == 0xF0)
        {
            lock = true;
            return true;
        }
        return false;
    }

    u32 interrupt_true_cycles{};
    bool accepts_interrupts()
    {
        return interrupt_true_cycles >= 2;
    }

    void interrupt(u8 n, bool forced=false)
    {
        if (flag(F_INTERRUPT) || forced)
        {
            halt = false;

            push(registers[FLAGS]);
            push(registers[CS]);
            push(registers[IP]);

            registers[IP] = memory16(0, n*4);
            registers[CS] = memory16(0, n*4+2);
            set_flag(F_INTERRUPT,false);
            if (!forced)
            {
                if constexpr(DEBUG_LEVEL > 0)
                    cout << "IRQ: CPU ACK " << u32(n-8) << endl;
                pic.cpu_ack_irq(n-8);
            }
        }
    }

    void irq(u8 n)
    {
        interrupt(n+8, false);
    }

    void push(u16 data)
    {
        registers[SP] -= 2;
        memory16(registers[SS], registers[SP]) = data;
    }
    u16 pop()
    {
        u16 data = memory16(registers[SS], registers[SP]);
        registers[SP] += 2;
        return data;
    }

    bool halt{false};

    void cycle()
    {
        ++cycles;

        if (delay)
        {
            --delay;
            return;
        }
        if (halt)
        {
            return;
        }

        PrintCSIP();
        if (registers[CS] == 0 && registers[IP] == 0)
        {
            cout << "Trying to run code at CS:IP 0:0" << endl;
            std::abort();
        }

        u8 instruction = read_inst<u8>();
        if (startprinting)
        {
            std::cout << "#" << std::dec << cycles << std::hex << ": " << u32(instruction) << " @ " << registers[CS]*16+registers[IP]-1;
            print_regs();
        }

        opcodes_used[instruction] = true;
        while (set_prefix(instruction))
        {
            instruction = read_inst<u8>();
            opcodes_used[instruction] = true;
            if (startprinting || DEBUG_LEVEL > 1)
                std::cout << "prefix read. #" << std::dec << cycles << std::hex << ": " << "Executing 0x" << u32(instruction) << " at CS:IP = " << registers[CS] << ":" << registers[IP]-1 << " = " << registers[CS]*16+registers[IP]-1 << std::endl;
        }

        if (false);
        else if (instruction < 0x40 && (instruction&0x07) < 6)
        {
            u8 instr_choice = (instruction&0x38)>>3;
            if (instruction&0x04)
            {
                if (instruction&0x01)//16bit
                {
                    u16& r = registers[AX];
                    u16 imm = read_inst<u16>();
                    r = run_arith(r, imm, instr_choice);
                }
                else //8bit
                {
                    u8& r = get_r8(0);
                    u8 imm = read_inst<u8>();
                    r = run_arith(r, imm, instr_choice);
                }
            }
            else
            {
                u8 modrm = read_inst<u8>();
                if (instruction&0x01)//16bit
                {
                    u16& rm = decode_modrm_u16(modrm);
                    u16& r = get_r16((modrm>>3)&0x07);
                    u16& rout = (instruction&0x02?r:rm);
                    u16& rin = (instruction&0x02?rm:r);
                    rout = run_arith(rout, rin, instr_choice);
                }
                else//8bit
                {
                    u8& rm = decode_modrm_u8(modrm);
                    u8& r = get_r8((modrm>>3)&0x07);
                    u8& rout = (instruction&0x02?r:rm);
                    u8& rin = (instruction&0x02?rm:r);
                    rout = run_arith(rout, rin, instr_choice);
                }
            }
        }
        else if ((instruction&0xE6) == 0x06)
        {
            if (instruction == 0x0F)
            {
                cout << "POP CS?? ASDFGH" << endl;
            }
            u16& reg = get_segment_r16((instruction>>3)&0x03);
            if (instruction&0x01)
                reg = pop();
            else
                push(reg);
        }
        else if (instruction == 0x27) // DAA
        {
            u8 old_AL = registers[AX]&0xFF;
            bool weird_special_case = (!flag(F_CARRY)) && flag(F_AUX_CARRY);

            u8 added{};

            set_flag(F_AUX_CARRY, (registers[AX] & 0x0F) > 9 || flag(F_AUX_CARRY));
            if (flag(F_AUX_CARRY))
                added += 0x06;

            set_flag(F_CARRY, old_AL > 0x99+(weird_special_case?6:0) || flag(F_CARRY));
            if (flag(F_CARRY))
                added += 0x60;

            get_r8(0) += added;

            set_flag(F_ZERO, (registers[AX]&0xFF) == 0);
            set_flag(F_SIGN, (registers[AX] & 0x80));
            set_flag(F_PARITY, byte_parity[registers[AX]&0xFF]);
            set_flag(F_OVERFLOW, (old_AL ^ registers[AX]) & (added ^ registers[AX])&0x80);
        }
        else if (instruction == 0x37) // AAA
        {
            u16 old_AX = registers[AX];
            bool add_ax = (registers[AX] & 0x0F) > 9 || flag(F_AUX_CARRY);
            if (add_ax)
            {
                get_r8(0) += 0x06; //AL
                get_r8(4) += 0x01; //AH
            }
            u16 added = registers[AX]-old_AX;

            set_flag(F_AUX_CARRY, add_ax);
            set_flag(F_CARRY, add_ax);
            set_flag(F_PARITY, byte_parity[registers[AX]&0xFF]);
            set_flag(F_ZERO, (registers[AX]&0xFF) == 0);
            set_flag(F_SIGN, (registers[AX] & 0x80));
            set_flag(F_OVERFLOW, (old_AX ^ registers[AX]) & (added ^ registers[AX])&0x80);

            get_r8(0) &= 0x0F;
        }
        else if (instruction == 0x2F) // DAS
        {
            u8 old_AL = registers[AX] & 0xFF;
            bool weird_special_case = (!flag(F_CARRY)) && flag(F_AUX_CARRY);

            u8 subtracted{};

            bool sub_al = ((registers[AX] & 0x0F) > 9 || flag(F_AUX_CARRY));
            if (sub_al)
                subtracted += 0x06;

            set_flag(F_AUX_CARRY, sub_al);
            bool sub_al2 = (old_AL > (0x99+(weird_special_case?6:0)) || flag(F_CARRY));
            if (sub_al2)
                subtracted += 0x60;

            get_r8(0) -= subtracted;
            set_flag(F_CARRY, sub_al2);
            set_flag(F_ZERO, (registers[AX] & 0xFF) == 0);
            set_flag(F_SIGN, (registers[AX] & 0x80));
            set_flag(F_PARITY, byte_parity[registers[AX] & 0xFF]);
            set_flag(F_OVERFLOW, ((old_AL ^ subtracted) & (old_AL ^ registers[AX]))&0x80);
        }
        else if (instruction == 0x3F) // AAS
        {
            u16 old_AX = registers[AX];
            bool sub_ax = (registers[AX] & 0x0F) > 9 || flag(F_AUX_CARRY);
            if (sub_ax)
            {
                get_r8(4) -= 1;
                get_r8(0) -= 6;
            }
            u16 subtracted = old_AX-registers[AX];
            set_flag(F_AUX_CARRY, sub_ax);
            set_flag(F_CARRY, sub_ax);
            set_flag(F_ZERO, (registers[AX] & 0xFF) == 0);
            set_flag(F_SIGN, (registers[AX] & 0x80));
            set_flag(F_PARITY, byte_parity[registers[AX] & 0xFF]);
            set_flag(F_OVERFLOW, (old_AX ^ subtracted) & (old_AX ^ registers[AX])&0x80);

            get_r8(0) &= 0x0F;
        }
        else if ((instruction&0xF0) == 0x40) //INC/DEC register
        {
            u16 result = registers[instruction&0x07]+(1-((instruction&0x08)?2:0));
            set_flag(F_SIGN, result&0x8000);
            set_flag(F_ZERO, result==0);
            set_flag(F_AUX_CARRY, (result&0x0F) == ((instruction&0x08)?0x0F:0x00));
            set_flag(F_PARITY, byte_parity[result&0xFF]);
            set_flag(F_OVERFLOW, result==0x8000-((instruction&0x08)?1:0));
            registers[instruction&0x07] = result;
        }
        else if ((instruction&0xF8) == 0x50) // push reg
        {
            push(registers[instruction&0x07]);
        }
        else if ((instruction&0xF8) == 0x58) //pop reg
        {
            registers[instruction&0x07] = pop();
        }
        //else if ((instruction&0xF0) == 0x60) // on 8086 these are synonymous to 0x7*
        //{
        //    cout << "*" << u32(instruction);
        //}
        else if ((instruction&0xE0) == 0x60) //various short jumps
        {
            if ((instruction&0xF0) == 0x60)
            {
                cout << "*" << u32(instruction);
            }
            u8 type = (instruction&0x0F)>>1;
            u16 f = (registers[FLAGS]&0xFFFD) | (flag(F_SIGN) != flag(F_OVERFLOW) ? 0x2:0x0);
            const u16 masks[8] =
            {
                0x800,0x001,0x040,0x041,0x080,0x004,0x002,0x042
            };
            i8 offset = read_inst<i8>();
            if (bool(f&masks[type])^(instruction&0x01))
            {
                registers[IP] += offset;
            }
        }
        else if (instruction == 0x80 || instruction == 0x82)
        {
            u8 modrm = read_inst<u8>();
            u8& rm = decode_modrm_u8(modrm);
            u8 imm = read_inst<u8>();
            rm = run_arith(rm, imm, (modrm>>3)&0x07);
        }
        else if (instruction == 0x81)
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            u16 imm = read_inst<u16>();
            rm = run_arith(rm, imm, (modrm>>3)&0x07);
        }
        else if (instruction == 0x83)
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            u16 imm = i16(read_inst<i8>());
            rm = run_arith(rm, imm, (modrm>>3)&0x07);
        }
        else if (instruction == 0x84)
        {
            u8 modrm = read_inst<u8>();
            u8& rm = decode_modrm_u8(modrm);
            u8& r = get_r8((modrm>>3)&0x07);
            test_flags(u8(rm&r));
        }
        else if (instruction == 0x85)
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            u16& r = get_r16((modrm>>3)&0x07);
            test_flags(u16(rm&r));
        }
        else if (instruction == 0x86)
        {
            u8 modrm = read_inst<u8>();
            u8& rm = decode_modrm_u8(modrm);
            u8& r = get_r8((modrm>>3)&0x07);
            u8 temp = rm;
            rm = r;
            r = temp;
        }
        else if (instruction == 0x87)
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            u16& r = get_r16((modrm>>3)&0x07);
            u16 temp = rm;
            rm = r;
            r = temp;
        }
        else if ((instruction&0xFC) == 0x88)
        {
            u8 modrm = read_inst<u8>();
            if (instruction&0x01)//16bit
            {
                u16& rm = decode_modrm_u16(modrm);
                u16& r = get_r16((modrm>>3)&0x07);
                u16& rout = (instruction&0x02?r:rm);
                u16& rin = (instruction&0x02?rm:r);
                rout = rin;
            }
            else//8bit
            {
                u8& rm = decode_modrm_u8(modrm);
                u8& r = get_r8((modrm>>3)&0x07);
                u8& rout = (instruction&0x02?r:rm);
                u8& rin = (instruction&0x02?rm:r);
                rout = rin;
            }
        }
        else if (instruction == 0x8C) // MOV Ew Sw
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            u16& r = get_segment_r16((modrm>>3)&0x07);
            rm = r;
        }
        else if (instruction == 0x8D) // LEA Gv M
        {
            u8 modrm = read_inst<u8>();
            u16& r = get_r16((modrm>>3)&0x07);
            r = effective_address(modrm);
        }
        else if (instruction == 0x8E) // MOV Sw Ew
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            u16& r = get_segment_r16((modrm>>3)&0x07);
            r = rm;
        }
        else if (instruction == 0x8F) //POP modrm
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            rm = pop();
        }
        else if ((instruction&0xF8) == 0x90) // XCHG AX, r16 - note how 0x90 is effectively NOP :-)
        {
            u8 reg_id = instruction&0x7;
            u16 tmp = registers[AX];
            registers[AX] = registers[reg_id];
            registers[reg_id] = tmp;
        }
        else if (instruction == 0x98) //CBW
        {
            u16 r = (registers[AX])&0xFF;
            r |= (r&0x80)?0xFF00:0x0000;
            registers[AX] = r;
        }
        else if (instruction == 0x99) //CWD
        {
            registers[DX] = (registers[AX]&0x8000)?0xFFFF:0x0000;
        }
        else if (instruction == 0x9A) //call Ap
        {
            u16 pointer = read_inst<u16>();
            u16 segment = read_inst<u16>();
            push(registers[CS]);
            push(registers[IP]);
            registers[CS] = segment;
            registers[IP] = pointer;
        }
        else if (instruction == 0x9B) // WAIT/FWAIT
        {
            // waits for floating point exceptions.
            // basically a NOP because I don't have a FPU yet
        }
        else if (instruction == 0x9C) //pushf
        {
            push(registers[FLAGS]);
        }
        else if (instruction == 0x9D) //popf
        {
            u16 newflags = pop();
            const u16 FLAG_MASK = 0b0000'1111'1101'0101;
            registers[FLAGS] = (registers[FLAGS]&~FLAG_MASK) | (newflags&FLAG_MASK);
        }
        else if (instruction == 0x9E) //sahf
        {
            reg8()[FLAGS*2] = (get_r8(4)&0xD5) | 0x02;
        }
        else if (instruction == 0x9F) //lahf
        {
            get_r8(4) = reg8()[FLAGS*2];
        }
        else if (instruction >= 0xA0 && instruction <= 0xAF) //AL/X=MEM  MEM=AL/X MOVSB/W CMPSB/W TEST AL/X,imm8/16 STOSB/W LODSB/W SCASB/W
        {
            bool big = (instruction&0x01); //word-sized?
            i8 size = i8(big)+1;
            i8 direction = flag(F_DIRECTIONAL)?-size:size;
            const u16 programs[8] = // lol microcode
            {
                0x0041, 0x0014, 0x4021, 0x8103,
                0x020C, 0x4024, 0x4041, 0x8106,
            };

            u16 source_segment = registers[get_segment(DS)];
            u16 source_offset = registers[SI];
            if ((instruction&0x0F) < 0x04)
                source_offset = read_inst<u16>();

            u16 program = programs[(instruction>>1)&0x07];
            while (registers[CX] != 0 || string_prefix == 0)
            {
                u16 value1{}, value2{};
                if (big)
                {
                    if (program&0x01)
                        value1 = memory16(source_segment, source_offset);
                    if (program&0x02)
                        value2 = memory16(registers[ES], registers[DI]);
                    if (program&0x04)
                        value1 = registers[AX];
                    if (program&0x08)
                        value2 = read_inst<u16>();
                    if (program&0x10)
                        memory16(source_segment, source_offset) = value1;
                    if (program&0x20)
                        memory16(registers[ES],registers[DI]) = value1;
                    if (program&0x40)
                        registers[AX] = value1;
                    if (program&0x100)
                        cmp_flags<u16>(value1,value2,u16(value1-value2));
                    if (program&0x200)
                        test_flags<u16>(u16(value1&value2));
                }
                else
                {
                    if (program&0x01)
                        value1 = memory8(source_segment, source_offset);
                    if (program&0x02)
                        value2 = memory8(registers[ES], registers[DI]);
                    if (program&0x04)
                        value1 = get_r8(0);
                    if (program&0x08)
                        value2 = read_inst<u8>();
                    if (program&0x10)
                        memory8(source_segment, source_offset) = value1;
                    if (program&0x20)
                        memory8(registers[ES],registers[DI]) = value1;
                    if (program&0x40)
                        get_r8(0) = value1;
                    if (program&0x100)
                        cmp_flags<u8>(value1,value2,u8(value1-value2));
                    if (program&0x200)
                        test_flags<u8>(u8(value1&value2));
                }
                if ((program&0xC000)==0x0000)
                    break;
                if (program&0x11) //uses DS:SI
                    registers[SI] += direction, source_offset += direction;
                if (program&0x22) //uses ES:DI
                    registers[DI] += direction;

                if (string_prefix == 0)
                    break;
                registers[CX] -= 1;
                if ((program&0xC000)==0x8000)
                {
                    if (string_prefix == SP_REPNZ && flag(F_ZERO))
                        break;
                    if (string_prefix == SP_REPZ && !flag(F_ZERO))
                        break;
                }
            }
        }
        else if ((instruction&0xF8) == 0xB0) //mov reg8, Ib
        {
            get_r8(instruction&0x07) = read_inst<u8>();
        }
        else if ((instruction&0xF8) == 0xB8) //mov reg16, Iv
        {
            get_r16(instruction&0x07) = read_inst<u16>();
        }
        else if (instruction == 0xC0 || instruction == 0xC2) // near return w/imm
        {
            u16 imm = read_inst<u16>();
            registers[IP] = pop();
            registers[SP] += imm;
        }
        else if (instruction == 0xC1 || instruction == 0xC3) // near return
        {
            registers[IP] = pop();
        }
        else if (instruction == 0xC8 || instruction == 0xCA) // far return w/imm
        {
            u16 imm = read_inst<u16>();
            registers[IP] = pop();
            registers[CS] = pop();
            registers[SP] += imm;
        }
        else if (instruction == 0xC9 || instruction == 0xCB) // far return
        {
            registers[IP] = pop();
            registers[CS] = pop();
        }
        else if ((instruction&0xFE) == 0xC4) // LES LDS
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            u16& r = get_r16((modrm>>3)&0x07);
            r = rm;
            registers[(instruction&1)?DS:ES] = *((&rm)+1); //ES or DS, based on the opcode
        }
        else if (instruction == 0xC6)
        {
            u8 modrm = read_inst<u8>();
            u8& rm = decode_modrm_u8(modrm);
            rm = read_inst<u8>();
        }
        else if (instruction == 0xC7)
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            rm = read_inst<u16>();
        }
        else if (instruction == 0xCC) // INT 3
        {
            if constexpr (DEBUG_LEVEL > 0)
                cout << "Calling interrupt 3... AX=" << registers[AX] << endl;
            interrupt(3, true);
        }
        else if (instruction == 0xCD) // INT imm8
        {
            u8 int_num = read_inst<u8>();
            if constexpr (DEBUG_LEVEL > 0)
                cout << "Calling interrupt... " << u32(int_num) << " AX=" << registers[AX] << endl;
            interrupt(int_num, true);
        }
        else if (instruction == 0xCE) // INTO
        {
            if (flag(F_OVERFLOW))
            {
                if constexpr (DEBUG_LEVEL > 0)
                    cout << "Calling int 4... AX=" << registers[AX] << endl;
                interrupt(4, true);
            }
        }
        else if (instruction == 0xCF) // IRET!
        {
            registers[IP] = pop();
            registers[CS] = pop();
            u16 newflags = pop();
            const u16 FLAG_MASK = 0b0000'1111'1101'0101;
            registers[FLAGS] = (registers[FLAGS]&~FLAG_MASK) | (newflags&FLAG_MASK);

            if (startprinting)
                cout << "RETURN FROM INTERRUPT to " << registers[IP]<< ":" << registers[CS] << "|" << newflags << endl;
        }
        else if (instruction == 0xD0 || instruction == 0xD2)
        {
            bool single_shift = ((instruction&0x02) == 0) || ((registers[CX]&0xFF) == 1);
            u8 modrm = read_inst<u8>();
            u8& rm = decode_modrm_u8(modrm);

            u8 inst_type = (modrm>>3)&0x07;
            u8 amount = (single_shift)?1:(registers[CX]&0xFF);

            for(u32 i=0; i<amount; ++i)
            {
                //F_OVERFLOW, F_SIGN, F_ZERO, F_AUX_CARRY, F_PARITY, F_CARRY
                u8 original=rm;
                u8 result=0;
                if (false);
                //ROL ROR RCL RCR SHL SHR SAL SAR
                else if ((inst_type&1) == 0) //ROL RCL SHL SAL
                {
                    result = (original << 1) | ((inst_type&0x04)?0:((inst_type&0x02) ? flag(F_CARRY) : original>>7));
                }
                else if (inst_type == 1 || inst_type == 3) //ROR RCR
                {
                    result = (original >> 1) | ((inst_type&0x02) ? flag(F_CARRY)<<7 : original<<7);
                }
                else if (inst_type == 5 || inst_type == 7) //SHR SAR
                {
                    result = (original >> 1) | ((inst_type&0x02) ? original&0x80 : 0);
                }
                set_flag(F_CARRY, original&((inst_type&1)?0x01:0x80));
                u8 flag_value = (inst_type&1)?result:original;
                set_flag(F_OVERFLOW, (bool(flag_value&0x80) != bool(flag_value&0x40)));
                if (inst_type&0x04)
                {
                    set_flag(F_SIGN, result&0x80);
                    set_flag(F_ZERO, result==0);
                    set_flag(F_AUX_CARRY, (inst_type==4?result&0x10:false));
                    set_flag(F_PARITY, byte_parity[result&0xFF]);
                }
                rm = result;
            }
        }
        else if (instruction == 0xD1 || instruction == 0xD3)
        {
            bool single_shift = ((instruction&0x02) == 0) || ((registers[CX]&0xFF) == 1);
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);

            u8 inst_type = (modrm>>3)&0x07;
            u8 amount = (single_shift)?1:(registers[CX]&0xFF);

            for(u32 i=0; i<amount; ++i)
            {
                //F_OVERFLOW, F_SIGN, F_ZERO, F_AUX_CARRY, F_PARITY, F_CARRY
                u16 original=rm;
                u16 result=0;
                if (false);
                //ROL ROR RCL RCR SHL SHR SAL SAR
                else if ((inst_type&1) == 0) //ROL RCL SHL SAL
                {
                    result = (original << 1) | ((inst_type&0x04)?0:((inst_type&0x02) ? flag(F_CARRY) : original>>15));
                }
                else if (inst_type == 1 || inst_type == 3) //ROR RCR
                {
                    result = (original >> 1) | ((inst_type&0x02) ? flag(F_CARRY)<<15 : original<<15);
                }
                else if (inst_type == 5 || inst_type == 7) //SHR SAR
                {
                    result = (original >> 1) | ((inst_type&0x02) ? original&0x8000 : 0);
                }
                set_flag(F_CARRY, original&((inst_type&1)?0x01:0x8000));
                u16 flag_value = (inst_type&1)?result:original;
                set_flag(F_OVERFLOW, (bool(flag_value&0x8000) != bool(flag_value&0x4000)));
                if (inst_type&0x04)
                {
                    set_flag(F_SIGN, result&0x8000);
                    set_flag(F_ZERO, result==0);
                    set_flag(F_AUX_CARRY, (inst_type==4?result&0x10:false));
                    set_flag(F_PARITY, byte_parity[result&0xFF]);
                }
                rm = result;
            }
        }
        else if (instruction == 0xD4) // AAM
        {
            u8 imm = read_inst<u8>();
            if (imm != 0)
            {
                u8 tempAL = (registers[AX]&0xFF);
                u8 tempAH = tempAL/imm;
                tempAL = tempAL%imm;
                registers[AX] = (tempAH<<8)|tempAL;

                set_flag(F_SIGN, tempAL&0x80);
                set_flag(F_ZERO, tempAL==0);
                set_flag(F_PARITY, byte_parity[tempAL]);
                set_flag(F_OVERFLOW,false);
                set_flag(F_AUX_CARRY,false);
                set_flag(F_CARRY,false);

            }
            else
            {
                set_flag(F_SIGN, false);
                set_flag(F_ZERO, true);
                set_flag(F_PARITY, true);
                set_flag(F_OVERFLOW,false);
                set_flag(F_AUX_CARRY,false);
                set_flag(F_CARRY,false);
                interrupt(0, true);
            }
        }
        else if (instruction == 0xD5) // AAD TODO: neaten this code up, also still F_ZERO is wrong sometimes ?!
        {
            u8 imm = read_inst<u8>();
            u16 orig16 = registers[AX];
            u16 temp16 = (registers[AX]&0xFF) + (registers[AX]>>8)*imm;
            registers[AX] = (temp16&0xFF);

            set_flag(F_SIGN,temp16&0x80);
            set_flag(F_ZERO, temp16==0);
            set_flag(F_PARITY, byte_parity[temp16&0xFF]);

            u8 a = orig16;
            u8 b = (orig16>>8)*imm;
            u8 result = a+b;

            set_flag(F_CARRY,result < a); //this is now correct

            bool of = ((a ^ result) & (b ^ result)) & 0x80;
            bool af = ((a ^ b ^ result) & 0x10);
            set_flag(F_OVERFLOW,of);
            set_flag(F_AUX_CARRY,af);
        }
        else if (instruction == 0xD6) // SALC (undocumented!)
        {
            get_r8(0) = flag(F_CARRY)?0xFF:0x00;
        }
        else if (instruction == 0xD7) // XLAT
        {
            u8 result = memory8(registers[get_segment(DS)],registers[BX]+(registers[AX]&0xFF));
            get_r8(0) = result;
        }
        else if (instruction >= 0xD8 && instruction <= 0xDF)
        {
            //cout << "Trying to run floating point instruction! :(" << endl;
            u8 modrm = read_inst<u8>(); //read modrm data anyway to sync up
            decode_modrm_u8(modrm);
            //FLOATING POINT INSTRUCTIONS! 8087! we don't have this. yet?
        }
        else if ((instruction & 0xFC) == 0xE0) // LOOPNZ LOOPZ LOOP JCXZ
        {
            i8 offset = read_inst<i8>();
            registers[CX] -= ((instruction&0x03)!=3);
            if ((registers[CX] != 0) == ((instruction&0x03) != 3) && (instruction&0x02 ? true:(flag(F_ZERO) == (instruction&0x01))))
            {
                registers[IP] = i16(registers[IP]) + offset;
            }
        }
        else if (instruction == 0xE4)
        {
            get_r8(0) = IO::in(read_inst<u8>());
        }
        else if (instruction == 0xE5)
        {
            u8 port = read_inst<u8>();
            u8 low = IO::in(port);
            u8 high = IO::in(port+1);
            registers[AX] = (high<<8)|low;
        }
        else if (instruction == 0xE6)
        {
            IO::out(read_inst<u8>(), registers[AX]&0xFF);
        }
        else if (instruction == 0xE7)
        {
            u8 port = read_inst<u8>();
            IO::out(port, registers[AX]&0xFF);
            IO::out(port+1, registers[AX]>>8);
        }
        else if (instruction == 0xE8)
        {
            i16 ip_offset = read_inst<i16>();
            push(registers[IP]);
            registers[IP] = i16(registers[IP])+ip_offset;
        }
        else if (instruction == 0xE9)
        {
            i16 ip_offset = read_inst<i16>();
            registers[IP] = i16(registers[IP])+ip_offset;
        }
        else if (instruction == 0xEA) //far jump
        {
            u16 new_ip = read_inst<u16>();
            u16 new_cs = read_inst<u16>();

            registers[IP] = new_ip;
            registers[CS] = new_cs;
        }
        else if (instruction == 0xEB)
        {
            i8 ip_offset = read_inst<i8>();
            registers[IP] = i16(registers[IP])+ip_offset;
        }
        else if (instruction == 0xEC)
        {
            get_r8(0) = IO::in(registers[DX]);
        }
        else if (instruction == 0xED)
        {
            u16 port = registers[DX];
            u8 low = IO::in(port);
            u8 high = IO::in(port+1);
            registers[AX] = (high<<8)|low;
        }
        else if (instruction == 0xEE)
        {
            IO::out(registers[DX], registers[AX]&0xFF);
        }
        else if (instruction == 0xEF)
        {
            IO::out(registers[DX], registers[AX]&0xFF);
            IO::out(registers[DX]+1, registers[AX]>>8);
        }
        else if (instruction == 0xF1)
        {
            // THIS ISN'T VALID ON 8088 / 8086
        }
        else if (instruction == 0xF4) // HALT / HLT
        {
            halt = true;
            //registers[IP] -= 1;
        }
        else if (instruction == 0xF5) // cmc
        {
            set_flag(F_CARRY, !flag(F_CARRY));
        }
        else if(instruction == 0xF6)
        {
            u8 modrm = read_inst<u8>();
            u8& rm = decode_modrm_u8(modrm);
            u8 op = ((modrm>>3)&0x07);
            if (op == 0) // TEST
            {
                u8 imm = read_inst<u8>();
                test_flags(u8(rm&imm));
            }
            else if (op == 1) //invalid!!!
            {
                [[maybe_unused]] u8 imm = read_inst<u8>();
                cout << "$";
            }
            else if (op==2) // NOT
            {
                rm = ~rm;
            }
            else if (op == 3) // NEG
            {
                cmp_flags(u8(0),rm,u8(-rm));
                set_flag(F_CARRY,rm!=0);
                rm = -rm;
            }
            else if (op==4) // MUL
            {
                u8 op2 = registers[AX]&0xFF;
                u16 result = rm*op2;
                set_flag(F_SIGN,result&0x8000);
                set_flag(F_PARITY,byte_parity[result>>8]);
                set_flag(F_OVERFLOW,result&0xFF00);
                set_flag(F_CARRY,result&0xFF00);
                set_flag(F_AUX_CARRY,false);
                set_flag(F_ZERO,(result&0xFF00)==0);
                registers[AX] = result;
            }
            else if (op==5) // IMUL
            {
                u8 op2 = registers[AX]&0xFF;
                i16 result = i16(i8(rm))*i16(i8(op2));

                set_flag(F_SIGN,result&0x8000);
                set_flag(F_PARITY,byte_parity[u16(result)>>8]);
                set_flag(F_OVERFLOW,result>=0x80 || result < -0x80);
                set_flag(F_CARRY,result>=0x80 || result < -0x80);
                set_flag(F_AUX_CARRY,false);
                set_flag(F_ZERO,(result&0xFFFF)==0);
                registers[AX] = u16(result);
            }
            else if (op==6 || op == 7) //DIV IDIV
            {
                if (rm == 0)
                {
                    interrupt(0, true);
                }
                else if (op == 6)
                {
                    u8 denominator = rm;
                    u16 result = registers[AX]/denominator;
                    if (result >= 0x100)
                    {
                        set_flag(F_PARITY,false);
                        set_flag(F_SIGN, registers[AX]&0x8000);
                        interrupt(0, true);
                    }
                    else
                    {
                        u8 quotient = result;
                        u8 remainder = registers[AX] % denominator;
                        registers[AX] = (remainder<<8)|quotient;
                        set_flag(F_PARITY,false);
                        set_flag(F_SIGN, remainder^0x80);
                    }
                }
                else if (op == 7)
                {
                    i8 denominator = i8(rm);
                    i16 result = i16(registers[AX]) / denominator;
                    if (result <= -0x80 || result >= 0x80) // 8088/6 doesn't accept -0x80
                    {
                        interrupt(0, true);
                    }
                    else
                    {
                        i8 quotient = result&0xFF;
                        if (string_prefix != 0)
                            quotient = -quotient;
                        i8 remainder = i16(registers[AX]) % denominator;
                        registers[AX] = (remainder<<8)|u8(quotient);
                        set_flag(F_PARITY,false);
                    }
                }
            }
            else
            {
                std::cout << "0xF6 op " << u32(op) << " is invalid!" << std::endl;
                std::abort();
            }
        }
        else if(instruction == 0xF7)
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            u8 op = ((modrm>>3)&0x07);
            if (op == 0) // TEST
            {
                u16 imm = read_inst<u16>();
                test_flags(u16(rm&imm));
            }
            else if (op == 1) //invalid!!!
            {

            }
            else if (op==2) // NOT
            {
                rm = ~rm;
            }
            else if (op == 3) // NEG
            {
                cmp_flags(u16(0),rm,u16(-rm));
                set_flag(F_CARRY,rm!=0);
                rm = -rm;
            }
            else if (op==4) // MUL
            {
                u16 op2 = registers[AX];
                u32 result = u32(rm)*u32(op2);
                set_flag(F_SIGN,result&0x80000000);
                set_flag(F_PARITY,byte_parity[(result>>16)&0xFF]);
                set_flag(F_OVERFLOW,result&0xFFFF0000);
                set_flag(F_CARRY,result&0xFFFF0000);
                set_flag(F_AUX_CARRY,false);
                set_flag(F_ZERO,(result>>16)==0);

                registers[AX] = result&0xFFFF;
                registers[DX] = result >> 16;
            }
            else if (op==5) //IMUL
            {
                u16 op2 = registers[AX];
                i32 result = i32(i16(rm))*i32(i16(op2));
                set_flag(F_SIGN,result&0x80000000);
                set_flag(F_PARITY,byte_parity[(result>>16)&0xFF]);
                set_flag(F_OVERFLOW,result>=0x8000 || result < -0x8000);
                set_flag(F_CARRY,result>=0x8000 || result < -0x8000);
                set_flag(F_AUX_CARRY,false);
                set_flag(F_ZERO,(result)==0);

                registers[AX] = result&0xFFFF;
                registers[DX] = result >> 16;
            }
            else if (op == 6 || op == 7) //DIV IDIV
            {
                if (rm == 0)
                {
                    interrupt(0, true); //division by zero
                }
                else if (op == 6)
                {
                    u32 numerator = (registers[DX]<<16)|registers[AX];
                    u16 denominator = rm;
                    u32 result = numerator / denominator;
                    if (result >= 0x10000)
                    {
                        interrupt(0,true);
                    }
                    else
                    {
                        registers[AX] = result;
                        registers[DX] = numerator % denominator;
                    }
                }
                else if (op == 7)
                {
                    i32 numerator = i32((registers[DX]<<16)|registers[AX]);
                    i16 denominator = i16(rm);
                    i32 result = numerator / denominator;
                    if (result <= -0x8000 || result >= 0x8000)
                    {
                        interrupt(0,true);
                    }
                    else
                    {
                        registers[AX] = numerator / denominator;
                        registers[DX] = numerator % denominator;
                        if (string_prefix != 0)
                            registers[AX] = -registers[AX];
                    }
                }
            }
            else
            {
                std::cout << "0xF7 op " << u32(op) << " is invalid!" << std::endl;
                std::abort();
            }
        }
        else if ((instruction&0xFE) == 0xF8) //carry flag bit 0
        {
            set_flag(F_CARRY, instruction&0x01);
        }
        else if ((instruction&0xFE) == 0xFA) //interrupt flag bit 9
        {
            set_flag(F_INTERRUPT, instruction&0x01);
        }
        else if ((instruction&0xFE) == 0xFC) //direction flag bit 10
        {
            set_flag(F_DIRECTIONAL, instruction&0x01);
        }
        else if (instruction == 0xFE)
        {
            u8 modrm = read_inst<u8>();
            u8& reg = decode_modrm_u8(modrm);
            u8 op = (modrm>>3)&0x07;
            u8 result = reg+1-(op<<1);
            if (op >= 2)
            {
                cout << "*" << u32(instruction) << "-" << u32(modrm);
                //std::cout << "Invalid opcode combo: 0x" << u32(instruction) << " 0x" << u32(modrm) << std::endl;
                //std::abort();
            }
            else
            {
                set_flag(F_OVERFLOW,result==0x80-op);
                set_flag(F_AUX_CARRY,(result&0x0F) == ((op&0x01)?0x0F:0x00));
                set_flag(F_ZERO,result==0);
                set_flag(F_SIGN,result&0x80);
                set_flag(F_PARITY,byte_parity[result&0xFF]);
                //no carry!
                reg = result;
            }
        }
        else if (instruction == 0xFF)
        {
            u8 modrm = read_inst<u8>();
            u16& reg = decode_modrm_u16(modrm);
            u8 op = (modrm>>3)&0x07;
            if (op == 0 || op == 1)
            {
                u16 result = reg+1-(op<<1);
                set_flag(F_OVERFLOW,result==(0x8000-op));
                set_flag(F_AUX_CARRY,(result&0x0F) == ((op&0x01)?0x0F:0x00));
                set_flag(F_ZERO,result==0);
                set_flag(F_SIGN,result&0x8000);
                set_flag(F_PARITY,byte_parity[result&0xFF]);
                reg = result;
            }
            else if (op == 2) //call near
            {
                u16 address = reg;
                push(registers[IP]);
                registers[IP] = address; //have to do this because reg could be SP :')
            }
            else if (op == 3) //call far
            {
                u16 segment = *((&reg)+1);
                u16 address = reg;
                push(registers[CS]);
                push(registers[IP]);
                registers[CS] = segment;
                registers[IP] = address;
            }
            else if (op == 4) //jmp near
            {
                registers[IP] = reg;
            }
            else if (op == 5) //jmp far
            {
                registers[IP] = reg;
                registers[CS] = *((&reg)+1);
            }
            else if (op == 6 || op == 7)
            {
                push(reg);
            }
            else
            {
                std::cout << "#" << std::dec << cycles << std::hex << ": " << "Executing 0x" << u32(instruction) << " at CS:IP = " << registers[CS] << ":" << registers[IP]-1 << " = " << registers[CS]*16+registers[IP]-1 << std::endl;
                std::cout << "Unimplemented opcode combo: 0x" << u32(instruction) << " 0x" << u32(modrm) << std::endl;
                std::abort();
            }
        }
        else
        {
            std::cout << "#" << std::dec << cycles << std::hex << ": " << "Executing 0x" << u32(instruction) << " at CS:IP = " << registers[CS] << ":" << registers[IP]-1 << " = " << registers[CS]*16+registers[IP]-1 << std::endl;
            std::cout << "# " << std::dec << cycles << std::hex << ", Unknown opcode: 0x" << u32(instruction) << std::endl;
            std::abort();
        }

        clear_prefix();

        if (flag(F_INTERRUPT))
        {
            if (interrupt_true_cycles < 2)
                ++interrupt_true_cycles;
        }
        else
        {
            interrupt_true_cycles = 0;
        }
        delay += 4;
    }
} cpu;

u32 previous_csip{};
u32 csip_counter{};
void PrintCSIP()
{
    //if (cpu.registers[cpu.CS] == 0xEA && cpu.registers[cpu.IP] == 0x2A0A)
    if (cpu.registers[cpu.CS] == 0x00 && cpu.registers[cpu.IP] == 0x7C00)
    {
        realtime_timing = true; //when we start booting up the OS, switch the CPU to super mode
    }
}

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
                globalsettings.entertrace = true;
                cout << "Tracer armed. Press enter to start trace." << endl;
            }
            else if (key == GLFW_KEY_R) //reset
            {
                cpu.registers[cpu.CS] = 0xFFFF;
                cpu.registers[cpu.IP] = 0x0000;
                cout << "Soft reset!" << endl;
            }
            else if (key == GLFW_KEY_F)
            {
                cout << "Flushing disk." << endl;
                harddisk.disk.flush_complete();
                cout << "Disk flushed." << endl;
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
            else if (key == GLFW_KEY_9)
            {
                current_delay -= 1;
                cout << "Delay set to " << std::dec << current_delay << std::hex << endl;
            }
            else if (key == GLFW_KEY_0)
            {
                current_delay += 1;
                cout << "Delay set to " << std::dec << current_delay << std::hex << endl;
            }
            else if (key == GLFW_KEY_L)
            {
                cout << "Load floppy to A:" << endl;
                std::vector<std::string> files = list_all_files("disk/");
                int start_id=0;
                int chosen_id=-1;
                while(true)
                {
                    for(int i=0; i<10; ++i)
                    {
                        if (start_id+i >= int(files.size()))
                            break;
                        cout << "[" << i << "] " << files[start_id+i] << endl;
                    }
                    cout << "[,] previous" << endl;
                    cout << "[.] next" << endl;
                    cout << "[-] eject" << endl;
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
                }
                if (chosen_id != -1)
                {
                    cout << "Loading " << files[chosen_id] << " to A:" << endl;
                    diskettecontroller.drives[0].diskette = DISKETTECONTROLLER::Drive::DISKETTE(files[chosen_id]);
                }
                else
                {
                    cout << "Ejecting A:" << endl;
                    diskettecontroller.drives[0].diskette.eject();
                }
            }
        }
    }
    else
    {
        if (globalsettings.entertrace && key == GLFW_KEY_ENTER)
        {
            startprinting = true;
            realtime_timing = false;
            globalsettings.entertrace = false;
        }
        u8 pc_scancode = key_lookup[key];
        if (pc_scancode != 0)
        {
            kbd.press(pc_scancode | (action == GLFW_RELEASE ? 0x80 : 0));
        }
    }


    if (key == GLFW_KEY_F11)
    {
        realtime_timing = (action != GLFW_PRESS);
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
            file.read(reinterpret_cast<char*>(&memory_bytes[address]), fileSize);
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
        else if (drive_number >= 2 && drive_number < 3)
        {
            cout << "Loading hard disk from " << image_filename << endl;
            harddisk.disk = HARDDISK::DISK(image_filename);
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
        while(ptr < filedata.size())
        {
            bool test_passed = true;
            CPU8088 testcpu;
            testcpu.reset();
            memset(memory_bytes, 0, 1<<20);

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
                memory_bytes[address] = value;
            }

            testcpu.cycle();

            for(int i=0; i<14; ++i)
            {
                final_regs[i] = data16();
            }
            for(int i=0; i<14; ++i)
            {
                u16 test_reg = testcpu.registers[i];

                if (i!=testcpu.FLAGS)
                {
                    /*{
                        if ((start_regs[0]&0xFF) == 0)
                        {
                            cout << std::hex << "AX: " << std::dec << (start_regs[0]>>8) << "," << (start_regs[0]&0xFF) << "->" << final_regs[0] << std::hex << " - ";
                            cout << std::setw(4) << std::setfill('0') << (final_regs[i]) << "^" << (testcpu.registers[i]) << "=" << (final_regs[i]^testcpu.registers[i]) << std::dec << endl;
                        }
                    }*/
                    //continue;
                }
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


                    //ÅÄÖ

                    //if (i==0)
                    //    cout << test_filename << "#" << test_id << ": "  << hex << (start_regs[0]) << " should provide " << (final_reg) << " but CPU said " << (test_reg) << dec << endl;

                    //cout << test_filename << "#" << test_id << ": " << r_fullnames[u32(i)] << " not correct: " << std::hex << start_regs[i] << "->" << final_regs[i] << " cpu gave " << test_reg << " , diff=" << (test_reg^final_regs[i]) << std::dec << endl;
                }
            }

            u32 final_ram_n = data32();
            for(u32 i=0; i<final_ram_n; ++i)
            {
                u32 address = data32();
                u32 value = data32();

                if (memory_bytes[address] != value)
                {
                    test_passed = false;
                    //cout << test_filename << "#" << test_id << ": Memory bytes at " << address << " not correct: " << std::hex << u32(memory_bytes[address]) << "!=" << u32(value) << std::dec << endl;
                }
            }

            if (!test_passed)
                tests_failed += 1, ++tests_totalfailed;
            ++test_id;
            ++tests_totaldone;
        }

        if (tests_failed > 0)
        {   cout << test_filename << ": " << tests_failed << " TESTS FAILED!" << endl;
            cout << "Flag failures:   ";
            for(int i=0; i<16; ++i)
                cout << flags_failed[i] << (i%4==3?"  ":" ");
            cout << endl;
        }
        readonly_start = 0xC0000;
    }
    else if (command == "end_tests")
    {
        cout << "tests failed: " << tests_totalfailed << "/" << tests_totaldone << endl;
        std::abort();
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
    cpu.reset();

    double previousTime=0.0;

    u64 clockgen=0, clockgen_counter=0;
    u8 pitcycles=0; //for synchronising beeper/PIT
    u8 cgacycle{};
    u32 lockstep_cpu{};
    while(true)
    {
        //the loop is ca. ~14.31818 MHz
        ++clockgen;
        ++clockgen_counter;
        if (clockgen%3 == 0 && !lockstep)
        {
            cpu.cycle();
            ++lockstep_cpu;
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
            dma.cycle();
        }
        bool do_opl = clockgen%288;
        if (!realtime_timing && !lockstep && clockgen%12 == 0)
        {
            if (clockgen%8 == 0)
            {
                cga.cycle();
            }
            if (clockgen%12 == 0)
            {
                pit.cycle();
            }
            if (clockgen%298 == 0) //ca. 48kHz. handles sound output in general
                beeper.cycle();
            if (do_opl)
            {
                ym3812.cycle();
            }
        }

        if (do_opl)
        {
            ym3812.cycle_timers();
        }


        if ((clockgen&0x1F) == 0)
        {
            double newTime = glfwGetTime();
            if (realtime_timing | lockstep)
            {
                u64 cycles_done = (newTime-previousTime)*(14318180.0/3.0);

                //if there's a slowdown, prevent it from going "faster than realtime" in the emulation
                //u64 actual_cycles_done = cycles_done>=0x100?0x100:cycles_done;
                //cout << cycles_done<<' ';
                u64 actual_cycles_done = cycles_done;
                for(u64 i=0; i<actual_cycles_done; ++i) //3 divisor (4.77 MHz)
                {
                    if (lockstep)
                    {
                        cpu.cycle();
                        ++lockstep_cpu;
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
                        dma.cycle();
                    }

                    static u8 realtime_clock{};
                    ++realtime_clock;

                    if (realtime_timing | lockstep)
                    {
                        cgacycle += 3; //convert /3 divisor to /8
                        while (cgacycle >= 8)
                        {
                            cga.cycle();
                            cgacycle -= 8;
                        }
                    }

                    if ((realtime_clock&0x03) == 0 && (realtime_timing | lockstep))// 12 divisor
                    {
                        pit.cycle();
                        pitcycles += 7;
                        if (pitcycles >= 174) //ca. 48kHz. handles sound output in general
                        {
                            beeper.cycle();
                            pitcycles -= 174;
                        }
                        ++ym3812.oplcycles;
                        if (ym3812.oplcycles >= 24)
                        {
                            ym3812.cycle();
                            ym3812.oplcycles -= 24;
                        }
                    }
                }
                //update the time with the original cycles done
                //so when the slowdown ends, we don't try to "catch up"
                previousTime += double(cycles_done)/(14318180.0/3.0);
            }
            else
                previousTime = newTime;

            if ((clockgen&0xFFFF) == 0)
            {
                //check_bios(__LINE__);
                if (newTime-startTime >= 1.0)
                {
                    startTime += 1.0;
                    cout << clockgen_counter/14318180.0 << "x realtime ";
                    cout << std::dec << lockstep_cpu << std::hex << " inst/s";
                    //cout << u32(memory_bytes[0x410]) << " " << u32(memory_bytes[0x411]);
                    cout << endl;

                    harddisk.disk.flush();

                    //cga.print_regs();
                    lockstep_cpu = 0;
                    clockgen_counter = 0;
                }
            }
        }
    }
    Opl2::Quit();
    glfwTerminate();
}
