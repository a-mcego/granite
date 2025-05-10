#pragma once

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
                cout << "read 0x05, CGA current register bad: 0x" << std::hex << u32(current_register) << endl;
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
        //std::cout << "r" << u32(port) << " d" << u32(readdata) << " " << std::endl;
        return readdata;
    }

    void write(u8 port, u8 data) //port from 0 to 15! inclusive.
    {
        //std::cout << "w" << u32(port) << " d" << u32(data) << " " << std::endl;
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
    u32 prev_col_times{};
    void monitor_cycle(u8 pins)
    {
        bool vc = vsync_ctr.cycle(pins & (1 << int(MONITOR::VSYNC)));
        bool hc = hsync_ctr.cycle(pins & (1 << int(MONITOR::HSYNC)));

        if ((vc && !vsync_ctr.prev))
        {
            if (linepos >= 100)
            {
                prev_line_amount = linepos;
                screen.screenSizeY = prev_line_amount;
                linepos = 0;
            }
        }
        if (hc && !hsync_ctr.prev && !vsync_ctr.prev)
        {
            if (colpos >= 400)
            {
                prev_col_times = (prev_col_amount==colpos)?prev_col_times+1:0;
                prev_col_amount = colpos;
                if (prev_col_times >= 4)
                    screen.screenSizeX = prev_col_amount;
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

        if (linepos < screen.Y && colpos < screen.X)
        {
            screen.pixels[linepos * screen.X + colpos] = getpalette(color);
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
        column = (column>=(registers[H_TOTAL])*hsync_mult?0:column);

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

        vsync = (logical_line >= registers[V_SYNC_POS]+1 && logical_line <= registers[V_SYNC_POS]+1);

        if (!vsync)
            vsync_monitor_ctr = 0;
        else
            ++vsync_monitor_ctr;

        bool monitor_vsync = (vsync_monitor_ctr > 0 && vsync_monitor_ctr <= 912*3/8);
        vsync = (vsync_monitor_ctr > 0 && vsync_monitor_ctr <= 912*16/8);

        u16 hsync_start = (registers[H_SYNC_POS]-1)*hsync_mult;
        u16 hsync_end = (registers[H_SYNC_POS]-1+registers[H_SYNC_WIDTH])*hsync_mult;
        hsync = (column >= hsync_start && column < hsync_end);
        if (!hsync)
            hsync_monitor_ctr = 0;
        else
            ++hsync_monitor_ctr;

        bool monitor_hsync = (hsync_monitor_ctr >= 5 && hsync_monitor_ctr < 13);

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
                u32 offset = current_startaddress + (line_inside_character&1?0x2000:0) + logical_line*registers[H_DISPLAYED]*2+x;
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
                u32 offset = current_startaddress + logical_line*registers[H_DISPLAYED]*2 + x*2;
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
                    //todo: make composite_cycle()
                    //screen.pixels[scan_line*screen.X + scan_column + i] = compositecolor.Get((i)&0x03, p1);
                    //screen.pixels[scan_line*screen.X + scan_column + i+1] = compositecolor.Get((i+1)&0x03, p2);
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
                u8 char_row = CGABIOS[((char_code<<3)+line_inside_character)|0x800];

                for (u32 x_off = 0; x_off < 8; x_off++)
                {
                    u8 mask = (1 << ((half?3:7) - (x_off>>(textmode_40_80?0:1))));
                    u8 color = (char_row & mask) ? fg_color : bg_color;
                    if (draw_bg || is_graphics_mode)
                        color = palette[0];
                    if (vsync|hsync)
                        color = 0;

                    //todo: make composite_cycle()
                    //screen.pixels[scan_line * screen.X + scan_column + x_off] = compositecolor.Get(x_off&0x03, color);
                }
            }
        }

        snow = false;
    }
};
