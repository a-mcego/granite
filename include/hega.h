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
    u32 mem[0x10000] = {}; //0x10000 per plane

    u32 latch{};

    u32 get_write_mask()
    {
        u8 val = seq_regs[MAP_MASK];

        u32 mask{};
        mask |= (val&0x01)?0xFF:0x00;
        mask |= (val&0x02)?0xFF00:0x00;
        mask |= (val&0x04)?0xFF0000:0x00;
        mask |= (val&0x08)?0xFF000000:0x00;
        return mask;
    }

    bool chain()
    {
        return gfx_regs[MISCELLANEOUS]&0x02;
    }
    bool r_oddeven()
    {
        return gfx_regs[MODE_REGISTER]&0x10;
    }
    bool w_oddeven()
    {
        return seq_regs[MEMORY_MODE]&0x04;
    }

    bool adjust_address_for_memory_map(u32& address)
    {
        u8 memory_map = ((gfx_regs[MISCELLANEOUS]>>2)&0x03);

        if (memory_map == 1 && address >= 0x10000)
            return false;
        if (memory_map == 2)
        {
            if (address < 0x10000 || address >= 0x18000)
                return false;
            address -= 0x10000;
            return true;
        }
        if (memory_map == 3)
        {
            if (address < 0x18000)
                return false;
            address -= 0x18000;
            return true;
        }
        return true;
    }

    //address from 00000 to 1FFFF
    void w8(u32 address, u8 data)
    {
        if (!adjust_address_for_memory_map(address))
            return;

        u32 mask = get_write_mask();

        u32 data32 = data;
        data32 |= (data32<<16);

        if (chain())
        {
            if (address&1)
            {
                data32 <<= 8;
                mask &= 0xFF00FF00;
            }
            else
            {
                mask &= 0x00FF00FF;
            }
            address >>= 1;
        }
        else
        {
            data32 |= (data32<<8);
        }
        address &= 0xFFFF;

        u32 bmask = gfx_regs[BIT_MASK];
        bmask |= bmask<<8;
        bmask |= bmask<<16;
        u8 function_select = (gfx_regs[DATA_ROTATE]>>3)&3;

        if ((gfx_regs[MODE_REGISTER]&0x03)==2)
        {
            data32 = 0;
            data32 |= (data&0x01)?0xFF:0x00;
            data32 |= (data&0x02)?0xFF00:0x00;
            data32 |= (data&0x04)?0xFF0000:0x00;
            data32 |= (data&0x08)?0xFF000000:0x00;
            data32 &= bmask;
            mask &= bmask;
        }
        else if ((gfx_regs[MODE_REGISTER]&0x03)==1)
        {
            data32 = latch;
        }
        else if ((gfx_regs[MODE_REGISTER]&0x03)==0)
        {
            u8 rotate_amount = (gfx_regs[DATA_ROTATE]&0x07);

            u32 lomask = 0xFF>>rotate_amount;
            lomask |= lomask<<8;
            lomask |= lomask<<16;

            data32 = ((data32>>rotate_amount)&lomask) | ((data32<<(8-rotate_amount))&~lomask);

            if (false);
            else if(function_select == 1)
                data32 &= latch&mask;
            else if(function_select == 2)
                data32 |= latch&mask;
            else if(function_select == 3)
                data32 ^= latch&mask;

        }
        mem[address] = (data32&mask) | (mem[address]&~mask);
    }

    u8 r8(u32 address)
    {
        if (!adjust_address_for_memory_map(address))
            return 0xFF;

        u32 plane_id = (gfx_regs[READ_MAP_SELECT])&0x03;
        if (r_oddeven() || chain())
        {
            plane_id = (gfx_regs[READ_MAP_SELECT])&0x02;
            plane_id |= (address & 1);
            address >>= 1;
        }

        latch = mem[address];

        u8 ret{};
        if(((gfx_regs[MODE_REGISTER]>>3)&0x01)==1) // color compare
        {
            u8 color_compare = gfx_regs[COLOR_COMPARE]&0x0F;
            u8 color_dontcare = gfx_regs[COLOR_DONT_CARE]&0x0F;
            ret = 0xFF;
            for(int i=0; i<4; ++i)
            {
                u8 databyte = latch>>(i<<3);
                u8 cmpbyte = (color_compare&(1<<i))?0xFF:0x00;
                u8 dontcare = (color_dontcare&(1<<i))?0xFF:0x00;
                ret &= (~databyte^cmpbyte)|dontcare;
            }
        }
        else //normal read
        {
            ret = (latch>>(plane_id<<3));
        }
        return ret;
    }

    //internal use
    u32 r32(u16 address)
    {
        return mem[address];
    }

    void print_regs()
    {
        for(int i=0; i<CRTC_REG_COUNT; ++i)
            cout << u32(crtc_regs[i]) << (i%4==3?"  ":" ");
        cout << endl;
    }

    u8 gfx1_pos{}; //3CC
    u8 gfx2_pos{}; //3CA

    u8 switches{u8(~0b0110)};
    u8 feature_control{}; //3_A, not needed for now
    //u8 input_status_0{0b0001'}; //3C2 read, already set to EGA monitor
    u8 input_status_1{}; //3_A

    enum ATTR_REGISTER //index, value = 3C0, with a flipflop. reading from 3_A resets flipflop
    {
        PALETTE, //0x0-0xF
        MODE_CONTROL_ATTR = 0x10,
        OVERSCAN_COLOR,
        COLOR_PLANE_ENABLE,
        HORIZONTAL_PEL_PANNING
    } attr_choice{};
    bool attr_flipflop{};
    static const u8 ATTR_REG_COUNT = 0x14;
    u8 attr_regs[ATTR_REG_COUNT] = {};

    enum SEQ_REGISTER //index=3C4, value=3C5
    {
        RESET,
        CLOCKING_MODE,
        MAP_MASK,
        CHAR_MAP_SELECT,
        MEMORY_MODE,
    } seq_choice{};
    static const u8 SEQ_REG_COUNT = 0x05;
    u8 seq_regs[SEQ_REG_COUNT] = {};

    enum GFX_REGISTER //index=3CE, value=3CF
    {
        SET_RESET,
        ENABLE_SET_RESET,
        COLOR_COMPARE,
        DATA_ROTATE,
        READ_MAP_SELECT,
        MODE_REGISTER,
        MISCELLANEOUS,
        COLOR_DONT_CARE,
        BIT_MASK
    } gfx_choice{};
    static const u8 GFX_REG_COUNT = 0x09; //3CF
    u8 gfx_regs[GFX_REG_COUNT] = {};

    enum CRTC_REGISTER //index=3_4, value=3_5
    {
        H_TOTAL,
        H_DISPLAY_END,
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
        CURSOR_LOC_H,
        CURSOR_LOC_L,

        LIGHT_PEN_H, V_RETRACE_START = LIGHT_PEN_H,
        LIGHT_PEN_L, V_RETRACE_END = LIGHT_PEN_L,
        V_DISPLAY_END,
        OFFSET,

        UNDERLINE_LOCATION,
        V_BLANK_START,
        V_BLANK_END,
        MODE_CONTROL_CRTC,

        LINE_COMPARE
    } crtc_choice{};

    static const u8 CRTC_REG_COUNT = 0x19; //3_5
    u8 crtc_regs[CRTC_REG_COUNT] = {};

    /*
    overflow definition
    bit 0: vertical total 0x06
    bit 1: vertical display enable end 0x12
    bit 2: vertical retrace start 0x10
    bit 3: vblank start 0x15
    bit 4: line compare 0x18
    bit 5: cursor location 0x0A
    */

    u8 misc{}; //3C2 write

    u8 horizontal_retrace{};
    u8 vertical_retrace{};
    bool retrace{};
    u16 current_startaddress{};
    u8 vcc{};

    u32 totalvsync{};

    u8 read(u8 port) //port from 0 to 47! inclusive. 0-F MDA, 10-1F EGA, 20-2F CGA
    {
        std::cout << std::hex;
        u8 port_add = (misc&0x01)?0x20:0x00;

        u8 data{};
        if (false);
        /*else if (port == 0x12)
        {
            //bit 0-3: switches 1-4 if CLKSEL is true
            //bit 4: switch sense, 1 if switchc can be read
            //bit 7: crt interrupt
        }*/
        else if (port == 0x05+port_add)
        {
            if (crtc_choice < CRTC_REG_COUNT)
            {
                data = crtc_regs[crtc_choice];
            }
            else
            {
                cout << "read 0x25, HEGA current register bad: 0x" << std::hex << u32(crtc_choice) << endl;
                std::abort();
            }
        }
        else if (port == 0x0A+port_add)
        {
            //bit 0 = we are in vert. or horiz. retrace
            data |= retrace;
            //bit 1 = light pen triggered (vs 0 = armed)
            //bit 2 = light pen switch open (vs 0 = closed)
            //bit 3 = vertical sync pulse!
            data |= (vertical_retrace<<3);
            attr_flipflop = false;
        }
        //else if (port == 0x0F || port == 0x2F); //nothing
        else if (port == 0x12)
        {
            data = ((switches>>((misc>>2)&3))&1)<<4;
        }
        else
        {
            //std::cout << "unknown" << std::endl;
            //std::abort();
        }
        //if (port+0x3B0 != 0x3DA)
        //    cout << globalsettings.current_IP << ": HEGA READ! " << u32(port+0x3B0) << ":" << u32(data) << std::endl;
        return data;
    }

    void write(u8 port, u8 data) //port from 0 to 47! inclusive.
    {
        //std::cout << std::hex;
        u8 port_add = (misc&0x01)?0x20:0x00;

        if (false);
        else if (port == 0x10) // choose attr, set attr
        {
            if (!attr_flipflop)
            {
                attr_choice = ATTR_REGISTER(data&0x1F);
            }
            else if(attr_choice < ATTR_REG_COUNT)
            {
                attr_regs[attr_choice] = data;
                //cout << globalsettings.current_IP << ": ega w attr " << u32(attr_choice) << ":" << u32(data) << std::endl;
            }
            attr_flipflop = !attr_flipflop;
        }
        else if (port == 0x12) // misc
        {
            misc = data;
            //cout << globalsettings.current_IP << ": ega w misc " << u32(data) << std::endl;
        }
        else if (port == 0x04+port_add) //choose crtc
        {
            crtc_choice = CRTC_REGISTER(data&0x1F);
        }
        else if (port == 0x05+port_add) //set crtc
        {
            if (crtc_choice < CRTC_REG_COUNT)
            {
                crtc_regs[crtc_choice] = data;
                //if (crtc_choice != 0x0e && crtc_choice != 0x0F)//not cursor position
                //    cout << globalsettings.current_IP << ": ega w crtc " << u32(crtc_choice) << ":" << u32(data) << std::endl;
            }
        }
        else if (port == 0x14) //choose seq
        {
            seq_choice = SEQ_REGISTER(data&0x1F);
        }
        else if (port == 0x15) //set seq
        {
            if (seq_choice < SEQ_REG_COUNT)
            {
                seq_regs[seq_choice] = data;
                //cout << globalsettings.current_IP << ": ega w seq  " << u32(seq_choice) << ":" << u32(data) << std::endl;
            }
        }
        else if (port == 0x1C)
        {
            gfx1_pos = data;
            //cout << globalsettings.current_IP << ": ega w g1pos " << u32(data) << std::endl;
        }
        else if (port == 0x1A)
        {
            gfx2_pos = data;
            //cout << globalsettings.current_IP << ": ega w g2pos " << u32(data) << std::endl;
        }
        else if (port == 0x1E) //choose gfx
        {
            gfx_choice = GFX_REGISTER(data&0x0F);
            //cout << globalsettings.current_IP << ": ega w gfx_choice " << u32(data) << std::endl;
        }
        else if (port == 0x1F) //set gfx
        {
            if (gfx_choice < GFX_REG_COUNT)
            {
                gfx_regs[gfx_choice] = data;
                //cout << globalsettings.current_IP << ": ega w gfx  " << u32(gfx_choice) << ":" << u32(data) << std::endl;
            }
        }
        else if (port == 0x08+port_add); //3B8/3D8 ? bios does writes to these.
        else if (port == 0x09+port_add); //3B9/3D9 ? bios does writes to these.
        else if (port == 0x0A+port_add)
        {
            data = feature_control;
            cout << globalsettings.current_IP << ": ega w feat " << u32(data) << std::endl;
        }
        else if (port == 0x0F || port == 0x2F); //3BF/3DF ? bios does writes to these.
        else
        {
            //std::cout << "unknown" << std::endl;
            //std::abort();
        }
    }

    u64 total_frames{};

    //pinout the same as ega but without the first ground:
    //bit 0: rred
    //bit 1: red
    //bit 2: green
    //bit 3: blue
    //bit 4: ggreen
    //bit 5: bblue
    //bit 6: hsync
    //bit 7: vsync

    enum struct MONITOR
    {
        RED2,
        RED,
        GREEN,
        BLUE,
        GREEN2,
        BLUE2,
        HSYNC,
        VSYNC
    };

    struct MegaCounter
    {
        u32 length[2] = {};
        u32 count = {};
        bool prev = {};

        bool get_prev()
        {
            return prev^(!get_polarity());
        }

        bool get_polarity() //true = inverted polarity
        {
            return (length[1]>length[0]);
        }



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

        if ((vc && !vsync_ctr.get_prev()))
        {
            prev_line_amount = linepos;
            linepos = 0;
        }
        if (hc && !hsync_ctr.get_prev() && !vsync_ctr.get_prev())
        {
            prev_col_amount = colpos;
            colpos = 0;
            ++linepos;
        }
        if (!hsync_ctr.get_prev())
        {
            ++colpos;
        }

        u32 color = pins&0x3F;
        if (hsync_ctr.get_prev() || vsync_ctr.get_prev())
        {
            color = 0;
        }


        //u32 renderline = screen.Y/2-prev_line_amount/2 + linepos;
        //u32 rendercol = screen.X/2-prev_col_amount/2 + colpos;

        u32 renderline = linepos;
        u32 rendercol = colpos;

        if (!vsync_ctr.get_polarity())
        {
            color = (color&0x07) | ((color&0x38)?0x38:0x00);
        }

        if (renderline < screen.Y && rendercol < screen.X)
        {
            screen.pixels[renderline * screen.X + rendercol] = getpalette_ega(color);
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
        //u8 textmode_40_80 = (mode_select>>0)&0x01;
        //u8 textmode_40_80 = true;
        //u8 is_graphics_mode = ((mode_select>>1)&0x01);
        u8 is_graphics_mode = !(seq_regs[MEMORY_MODE]&0x01);
        u16 hsync_mult = 8;

        /*
        CRTC overflow definition
        bit 0: vertical total 0x06
        bit 1: vertical display enable end 0x12
        bit 2: vertical retrace start 0x10
        bit 3: vblank start 0x15
        bit 4: line compare 0x18
        bit 5: cursor location 0x0A
        */
        u16 v_total = crtc_regs[V_TOTAL] + ((crtc_regs[OVERFLOW]&0x01)?0x100:0x000);
        u16 v_display_end = crtc_regs[V_DISPLAY_END] + ((crtc_regs[OVERFLOW]&0x02)?0x100:0x000);
        u16 v_retrace_start = crtc_regs[V_RETRACE_START] + ((crtc_regs[OVERFLOW]&0x04)?0x100:0x000);
        u16 vblank_start = crtc_regs[V_BLANK_START] + ((crtc_regs[OVERFLOW]&0x08)?0x100:0x000);
        u16 linecompare = crtc_regs[LINE_COMPARE] + ((crtc_regs[OVERFLOW]&0x10)?0x100:0x000);
        u16 cursor_loc_h = crtc_regs[CURSOR_LOC_H] + ((crtc_regs[OVERFLOW]&0x20)?0x100:0x000);

        u16 v_retrace_end = v_retrace_start;
        while((v_retrace_end&0x0F) != (crtc_regs[V_RETRACE_END]&0x0F))
            ++v_retrace_end;

        u16 h_retrace_end = crtc_regs[H_RETRACE_START];
        while ((h_retrace_end&0x0F) != (crtc_regs[H_RETRACE_END]&0x0F))
            ++h_retrace_end;

        column += 8;
        column = (column>=(crtc_regs[H_TOTAL]+2)*hsync_mult?0:column);

        if (column == 0) //new line
        {
            ++line_inside_character;
            ++scan_line;
            if (line_inside_character > crtc_regs[MAX_SCAN_LINE])
            {
                line_inside_character = 0;
                ++logical_line;
                //cout << std::dec << physical_line << "-" << logical_line <<std::hex << endl;
            }
            bool all_lines_drawn = (scan_line >= v_total);
            if (all_lines_drawn)
                ++vsyncadjust;
            else
                vsyncadjust = 0;

            if (all_lines_drawn)// && vsyncadjust > crtc_regs[V_TOTAL_ADJUST])
            {
                //if (v_total > 0 && logical_line > 1)
                //    std::cout << std::dec << logical_line << " out of " << v_total << " lines." << std::hex << std::endl;
                vsyncadjust = 0;
                logical_line = 0;
                scan_line = 0;
                line_inside_character = 0;
            }

            if (logical_line == 0 && line_inside_character == 0)
            {
                current_startaddress = ((crtc_regs[START_ADDRESS_H]<<8) | crtc_regs[START_ADDRESS_L]);
            }
        }

        vsync = (scan_line >= vblank_start && scan_line < vblank_start + 8);

        if (!vsync)
            vsync_monitor_ctr = 0;
        else
            ++vsync_monitor_ctr;

        bool monitor_vsync = vsync ^ bool(misc&0x80);

        u16 hsync_start = (crtc_regs[H_RETRACE_START])*hsync_mult;
        //u16 hsync_end = (crtc_regs[H_RETRACE_END])*hsync_mult;
        u16 hsync_end = (crtc_regs[H_RETRACE_START]+6)*hsync_mult;
        hsync = (column >= hsync_start && column < hsync_end);
        bool monitor_hsync = hsync ^ bool(misc&0x40);

        vertical_retrace = (scan_line >= v_retrace_start && scan_line <= v_retrace_end);
        horizontal_retrace = (column >= (crtc_regs[H_RETRACE_START])*hsync_mult && column <= (h_retrace_end)*hsync_mult);

        u8 resolution = true;
        bool output_enabled = !(misc&0x10);

        //const u8 add = ((color_select&0x10)?8:0) + ((color_select&0x20)?1:0);
        const u8 palette[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

        retrace = (vertical_retrace|horizontal_retrace);
        bool draw_bg = retrace|!output_enabled;

        if (is_graphics_mode)
        {
            int x = column>>3;
            u32 offset = current_startaddress + logical_line*crtc_regs[OFFSET]*2 + x;

            for(int i=0; i<8; ++i)
            {
                int pel_panned_i = i + (attr_regs[HORIZONTAL_PEL_PANNING]&0x07);
                const u32 mask = 0x80808080;
                u32 color = 0;
                if (!retrace)
                {
                    u32 data = r32(offset + (pel_panned_i>>3));
                    color = ((data<<(pel_panned_i&7))&mask)>>7; //pixel from 0 to 7
                    //0b0000000a'0000000b'0000000c'0000000d
                    color |= color >> 7;
                    //0b0000000a'000000ab'000000bc'000000cd
                    color |= color >> 14;
                    //0b0000000a'000000ab'00000abc'0000abcd
                    color &= 0x0F;
                    //0b00000000'00000000'00000000'0000abcd
                }
                color = attr_regs[color];
                monitor_cycle(color|(u8(monitor_hsync)<<6|(u8(monitor_vsync)<<7)));
            }
        }
        else
        {
            bool cms0 = crtc_regs[MODE_CONTROL_CRTC]&0x01;

            u32 chosen_line = logical_line;
            u32 startaddr = current_startaddress;
            if (!cms0)
            {
                chosen_line = logical_line / 2;
                startaddr += (1<<13);
            }

            int x = column>>3;
            u32 offset = current_startaddress + chosen_line*crtc_regs[OFFSET]*2 + x;
            u8 char_code = r32(offset);
            u8 attribute = (r32(offset)>>8);
            u8 fg_color = attribute & 0x0F;
            u8 bg_color = (attribute >> 4) & 0x0F;
            u8 char_row = r32((char_code<<5)+line_inside_character)>>16;

            for (u32 x_off = 0; x_off < 8; x_off++)
            {
                u8 mask = 1 << (7 - x_off);
                u8 color = (char_row & mask) ? fg_color : bg_color;
                if (draw_bg || is_graphics_mode)
                    color = palette[0];
                if (hsync|vsync)
                    color = 0;
                //else
                //    color = rand()&0x0F;
                color = attr_regs[color];
                monitor_cycle(color|(u8(monitor_hsync)<<6|(u8(monitor_vsync)<<7)));
            }
        }
    }
};

