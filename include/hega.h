#pragma once

//emulation for Twinhead CT8190S HEGA
//use with the bios
//this supports cga, mda, hercules and ega

/* REGISTER enum defines OVERFLOW, but it is a
    definition in Windows headers */
#ifdef _WIN32
    #undef OVERFLOW
#endif

struct HEGA
{
    static const u8 CRTC_REG_COUNT = 0x19; //3B5 / 3D5
    static const u8 ATTRIB_REG_COUNT = 0x14; //3C0
    static const u8 GSI_REG_COUNT = 0x09; //3CF
    u8 crtc_regs[CRTC_REG_COUNT] = {};
    u8 attrib_regs[ATTRIB_REG_COUNT] = {};
    u8 gsi_regs[GSI_REG_COUNT] = {};


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
    u8 memory8_internal(u16 address)
    {
        return mem[address&0x3FFF];
    }
    u16 memory16_internal(u16 address)
    {
        return *(u16*)(void*)(mem+(address&0x3FFF));
    }
    void print_regs()
    {
        for(int i=0; i<CRTC_REG_COUNT; ++i)
            cout << u32(crtc_regs[i]) << (i%4==3?"  ":" ");
        cout << endl;
    }

    enum REGISTER
    {
        H_TOTAL,
        H_DISPLAYED,
        H_BLANK_START,
        H_BLANK_END,

        H_RETRACE_START,
        H_RETRACE_END,
        V_TOTAL,
        OVERFLOW,

        PRESET_ROW_SCAN,
        MAX_SCAN_LINE, //not "scanline" but "scan line" as per ibm's manual :-)
        CURSOR_START,
        CURSOR_END,

        START_ADDRESS_H,
        START_ADDRESS_L,
        LIGHT_PEN_H, V_RETRACE_START = LIGHT_PEN_H,
        LIGHT_PEN_L, V_RETRACE_END = LIGHT_PEN_L,

        OFFSET,
        UNDERLINE_LOCATION,
        V_BLANK_START,
        V_BLANK_END,
        MODE_CONTROL,
        LINE_COMPARE
    };

    /*
    overflow definition
    bit 0: vertical total 0x06
    bit 1: vertical display enable end 0x12
    bit 2: vertical retrace start 0x10
    bit 3: vblank start 0x15
    bit 4: line compare 0x18
    bit 5: cursor location 0x0A
    */

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

    u8 read(u8 port) //port from 0 to 47! inclusive
    {
        cout << "HEGA READ! " << u32(port+0x3B0) << endl;
        u8 readdata{};
        if (port == 0x25)
        {
            if (current_register < CRTC_REG_COUNT)
            {
                readdata = crtc_regs[current_register];
            }
            else
            {
                cout << "read 0x25, HEGA current register bad: 0x" << std::hex << u32(current_register) << endl;
                std::abort();
            }
        }
        else if (port == 0x2A)
        {
            //bit 0 = we are in vert. or horiz. retrace
            readdata |= retrace;
            //bit 1 = light pen triggered (vs 0 = armed)
            //bit 2 = light pen switch open (vs 0 = closed)
            //bit 3 = vertical sync pulse!
            readdata |= (vertical_retrace<<3);
        }
        //std::cout << "r" << u32(port) << " d" << u32(readdata) << " " << std::endl;
        return readdata;
    }

    void write(u8 port, u8 data) //port from 0 to 47! inclusive.
    {
        std::cout << "HEGA WRITE " << u32(port+0x3B0) << ":" << u32(data) << " " << std::endl;
        if (port == 0x24)
        {
            current_register = data;
        }
        else if (port == 0x25)
        {
            if (current_register < 0x10)
            {
                crtc_regs[current_register] = data;
                //if (current_register != 0x0E && current_register != 0x0F)
                //      cout << "CGA " << u32(current_register) << "=" << u32(data) << " " << std::dec << column << ":" << line << "(" << logical_line << ")" << std::hex << "  ", print_regs();
            }
            else
            {
                cout << "write 0x05, CGA current register bad: " << u32(current_register) << endl;
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
                cout << i << "=" << u32(crtc_regs[i]) << ", ";
            cout << endl;
        }
    }

    u64 total_frames{};

    //pinout the same as cga but without the first ground:
    //bit 0: NC
    //bit 1: red
    //bit 2: green
    //bit 3: blue
    //bit 4: intensity
    //bit 5: NC
    //bit 6: hsync
    //bit 7: vsync

    enum struct MONITOR
    {
        NC1,
        RED,
        GREEN,
        BLUE,
        INTENSITY,
        NC2,
        HSYNC,
        VSYNC
    };

    struct MegaCounter
    {
        u32 length[2] = {};
        u32 count = {};
        bool prev = {};

        bool cycle(bool data)
        {
            bool change{};
            if (prev != data)
            {
                length[data] = count;
                count = 0;
                change = true;
            }
            ++count;
            prev = data;
            return change;
        }
    };

    MegaCounter vsync_ctr, hsync_ctr;
    u32 linepos{};
    u32 colpos{};
    u32 prev_line_amount{};
    u32 prev_col_amount{};
    void monitor_cycle(u8 pins)
    {
        bool vc = vsync_ctr.cycle(pins & (1 << int(MONITOR::VSYNC)));
        bool hc = hsync_ctr.cycle(pins & (1 << int(MONITOR::HSYNC)));

        if ((vc && !vsync_ctr.prev))
        {
            if (linepos >= 100)
            {
                prev_line_amount = linepos;
                linepos = 0;
                render();
            }
        }
        if (hc && !hsync_ctr.prev && !vsync_ctr.prev)
        {
            if (colpos >= 400)
            {
                prev_col_amount = colpos;
                colpos = 0;
                ++linepos;
            }
        }
        if (!hsync_ctr.prev)
        {
            ++colpos;
        }

        u32 color = (pins>>1)&0x0F;
        if (vc || hc)
        {
            color = 0;
        }


        u32 renderline = screen.Y/2-prev_line_amount/2 + linepos;
        u32 rendercol = screen.X/2-prev_col_amount/2 + colpos;
        if (renderline < screen.Y && rendercol < screen.X)
        {
            screen.pixels[renderline * screen.X + rendercol] = getpalette(color);
        }
    }

    u32 column{};
    u32 logical_line{};
    u32 scan_line{};
    u32 scan_column{};
    u32 line_inside_character{};
    u32 vsyncadjust{};
    bool hsync{}, vsync{};
    u32 hsync_monitor_ctr{}; //32-96 hdots
    u32 vsync_monitor_ctr{};
    void cycle() //8 hdots per cycle
    {
        u8 textmode_40_80 = (mode_select>>0)&0x01;
        u8 is_graphics_mode = ((mode_select>>1)&0x01);
        u16 hsync_mult = 16;

        if (textmode_40_80)
        {
            hsync_mult = 8;
        }

        column += 8;
        column = (column>=(crtc_regs[H_TOTAL])*hsync_mult?0:column);

        if (column == 0) //new line
        {
            ++line_inside_character;
            if (line_inside_character > crtc_regs[MAX_SCAN_LINE])
            {
                line_inside_character = 0;
                ++logical_line;
                //cout << std::dec << physical_line << "-" << logical_line <<std::hex << endl;
            }
            bool all_lines_drawn = (logical_line > crtc_regs[V_TOTAL]);
            if (all_lines_drawn)
                ++vsyncadjust;
            else
                vsyncadjust = 0;

            if (all_lines_drawn)// && vsyncadjust > crtc_regs[V_TOTAL_ADJUST])
            {
                vsyncadjust = 0;
                logical_line = 0;
                line_inside_character = 0;
            }

            if (logical_line == 0 && line_inside_character == 0)
            {
                current_startaddress = ((crtc_regs[START_ADDRESS_H]<<8) | crtc_regs[START_ADDRESS_L])*2;
            }
        }

        //vsync = (logical_line >= crtc_regs[V_SYNC_POS]+1 && logical_line <= crtc_regs[V_SYNC_POS]+1);

        if (!vsync)
            vsync_monitor_ctr = 0;
        else
            ++vsync_monitor_ctr;

        bool monitor_vsync = (vsync_monitor_ctr > 0 && vsync_monitor_ctr <= 912*3/8);
        vsync = (vsync_monitor_ctr > 0 && vsync_monitor_ctr <= 912*16/8);

        u16 hsync_start = 0;//(crtc_regs[H_SYNC_POS]-1)*hsync_mult;
        u16 hsync_end = 0;//(crtc_regs[H_SYNC_POS]-1+crtc_regs[H_SYNC_WIDTH])*hsync_mult;
        hsync = (column >= hsync_start && column < hsync_end);
        if (!hsync)
            hsync_monitor_ctr = 0;
        else
            ++hsync_monitor_ctr;

        bool monitor_hsync = (hsync_monitor_ctr >= 5 && hsync_monitor_ctr < 13);

        //vertical_retrace = (logical_line >= crtc_regs[V_DISPLAYED]);
        horizontal_retrace = (column >= (crtc_regs[H_DISPLAYED])*hsync_mult);

        u8 no_colorburst = (mode_select>>2)&0x01;
        u8 resolution = (mode_select>>4)&0x01;
        bool output_enabled = (mode_select&0x08);

        const u8 add = ((color_select&0x10)?8:0) + ((color_select&0x20)?1:0);
        const u8 palette[4] = {u8(color_select&0x0F), u8(2+add), u8(4+(no_colorburst?0:add)), u8(6+add)};

        retrace = (vertical_retrace|horizontal_retrace);
        bool draw_bg = retrace|!output_enabled;

        if (is_graphics_mode && !textmode_40_80)
        {
            int x = column>>3;
            u32 offset = current_startaddress + (line_inside_character&1?0x2000:0) + logical_line*crtc_regs[H_DISPLAYED]*2+x;
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
                if (hsync|vsync)
                    p1 = 0, p2 = 0;

                monitor_cycle(((p1&0x0F)<<1)|(u8(monitor_hsync)<<6|(u8(monitor_vsync)<<7)));
                monitor_cycle(((p2&0x0F)<<1)|(u8(monitor_hsync)<<6|(u8(monitor_vsync)<<7)));
                gfx_byte <<= 2;
            }
        }
        else
        {
            int x = column>>(textmode_40_80?3:4);
            bool half = (textmode_40_80?0:(column&8));
            u32 offset = current_startaddress + logical_line*crtc_regs[H_DISPLAYED]*2 + x*2;
            u8 char_code = memory8_internal(offset);
            u8 attribute = memory8_internal(offset+1);
            u8 fg_color = attribute & 0x0F;
            u8 bg_color = (attribute >> 4) & 0x0F;
            u8 char_row = CGABIOS[((char_code<<3)+line_inside_character)|0x800];

            for (u32 x_off = 0; x_off < 8; x_off++)
            {
                u8 mask = (1 << ((half?3:7) - (x_off>>(textmode_40_80?0:1))));
                u8 color = (char_row & mask) ? fg_color : bg_color;
                if (draw_bg || is_graphics_mode)
                    color = palette[0];
                if (hsync|vsync)
                    color = 0;

                monitor_cycle(((color&0x0F)<<1)|(u8(monitor_hsync)<<6|(u8(monitor_vsync)<<7)));
            }
        }
    }
};

