#pragma once

//VGA implementation based on standard VGA hardware
//supports all standard VGA modes including 256-color modes

/* REGISTER enum defines OVERFLOW, but it is a
    definition in Windows headers */
#ifdef _WIN32
    #undef OVERFLOW
#endif

template<bool GD5428>
struct VGACARD
{
    static constexpr u32 BYTELOOKUP[16] =
    {
        0x00000000,
        0x000000FF,
        0x0000FF00,
        0x0000FFFF,
        0x00FF0000,
        0x00FF00FF,
        0x00FFFF00,
        0x00FFFFFF,
        0xFF000000,
        0xFF0000FF,
        0xFF00FF00,
        0xFF00FFFF,
        0xFFFF0000,
        0xFFFF00FF,
        0xFFFFFF00,
        0xFFFFFFFF,
    };

    //TODO: GD5428
    u32 clock_numer() const
    {
        return ((misc&0x04)?99693:88616)*((seq_regs[CLOCKING_MODE]&0x08)?1:2);

        //GD5428:
        /*
        EDCLK(what is this?) == 1
        misc3:2 00 25.180 MHz
        misc3:2 01 28.325 MHz
        misc3:2 10 41.165 MHz
        misc3:2 11 36.082 MHz

        EDCLK == 0
        misc3 1 DCLK pin (DAC and CRTC counters)
        misc3 0 DCLK pin (DAC only)

        appendix B8 for more VCLK frequencies..
        */
    }

    //TODO: GD5428
    u32 clock_denom() const
    {
        //also do SR1&0x01 ? the 8/9 one. SR1:0 == 0 means 9, SR1:0 == 1 means 8. so basically
        //could do it here, 50400 vs. 56700
        return (seq_regs[CLOCKING_MODE]&0x08?50400*2:56700*2);
    }

    bool debugprint{};

    static constexpr u64 MEMSIZE = GD5428?0x40000:0x10000; //1 MB vs. 256kB
    //TODO: whatabout GD5428 2 MB support?

    u32 mem[MEMSIZE] = {}; //0x10000 per plane (64KB each)

    u32 latch{};

    // DAC registers for 256-color modes
    u8 dac_mask{0xFF};
    u8 dac_read_index{};
    u8 dac_write_index{};
    u8 dac_state{}; // 0=read index, 1=read r, 2=read g, 3=read b (similar for write)
    u8 dac_palette[256*3] = {}; // RGB values (6-bit each)
    u8 dac_state_register{}; //0 = read, 3 = write

    u32 get_write_mask()
    {
        return BYTELOOKUP[seq_regs[MAP_MASK]&0x0F];
    }

    bool chain4()
    {
        return seq_regs[MEMORY_MODE]&0x08;
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
        return !(seq_regs[MEMORY_MODE]&0x04);
    }

    bool map_active[4] = {true,true,true,true};
    u32 map_address_mask = 0x1FFFF;


    void update_memory_map()
    {
        u8 memory_map = ((gfx_regs[MISCELLANEOUS]>>2)&0x03);

        switch(memory_map)
        {
        case 0:
            map_address_mask = 0x1FFFF;
            map_active[0] = true;
            map_active[1] = true;
            map_active[2] = true;
            map_active[3] = true;
            break;
        case 1:
            map_address_mask = 0xFFFF;
            map_active[0] = true;
            map_active[1] = true;
            map_active[2] = false;
            map_active[3] = false;
            break;
        case 2:
            map_address_mask = 0x7FFF;
            map_active[0] = false;
            map_active[1] = false;
            map_active[2] = true;
            map_active[3] = false;
            break;
        case 3:
            map_address_mask = 0x7FFF;
            map_active[0] = false;
            map_active[1] = false;
            map_active[2] = false;
            map_active[3] = true;
            break;
        }

    }

    bool adjust_address_for_memory_map(u32& address)
    {
        bool ret = map_active[(address>>15)&0x03];
        address &= map_address_mask;
        return ret;
    }

    //address from 00000 to 1FFFF
    void w8(u32 address, u8 data)
    {
        if (!adjust_address_for_memory_map(address))
        {
            //if (debugprint)
            //    std::cout << "vga W?!" << std::endl;
            return;
        }

        //if (debugprint)
        //    std::cout << "vga W=" << std::hex << address << " writemode=" << (gfx_regs[MODE_REGISTER]&0x03) << std::endl;

        u32 bmask = gfx_regs[BIT_MASK];
        bmask |= bmask<<8;
        bmask |= bmask<<16;
        u32 mask = BYTELOOKUP[seq_regs[MAP_MASK]&0x0F];

        if (chain4())
        {
            // Chain-4 mode: address bits 0-1 select plane
            u32 plane = address & 3;
            address >>= 2;
            mask = 0xFF << (plane * 8);
        }
        else if (chain() || w_oddeven())
        {
            if (address&1)
            {
                mask &= 0xFF00FF00;
            }
            else
            {
                mask &= 0x00FF00FF;
            }
            address >>= 1;
        }
        address &= 0xFFFF;

        u32 data32{};
        u8 write_mode = gfx_regs[MODE_REGISTER]&0x03;

        if (write_mode == 3)
        {
            // Write mode 3
            u8 rotate_amount = (gfx_regs[DATA_ROTATE]&0x07);
            u32 rotated_data = data;
            if (rotate_amount)
            {
                rotated_data = ((data >> rotate_amount) | (data << (8 - rotate_amount))) & 0xFF;
            }

            u32 bit_mask = rotated_data;
            bit_mask |= bit_mask << 8;
            bit_mask |= bit_mask << 16;

            u32 set_reset = BYTELOOKUP[gfx_regs[SET_RESET]&0x0F];

            data32 = (set_reset & bit_mask) | (latch & ~bit_mask);
        }
        else if (write_mode == 2)
        {
            u8 function_select = (gfx_regs[DATA_ROTATE]>>3)&0x03;
            data32 = BYTELOOKUP[data&0x0F];

            u8 rotate_amount = (gfx_regs[DATA_ROTATE]&0x07);
            if (rotate_amount)
            {
                u32 lomask = 0xFF>>rotate_amount;
                lomask |= lomask<<8;
                lomask |= lomask<<16;
                data32 = ((data32>>rotate_amount)&lomask) | ((data32<<(8-rotate_amount))&~lomask);
            }

            if (function_select == 1)
                data32 &= latch;
            else if(function_select == 2)
                data32 |= latch;
            else if(function_select == 3)
                data32 ^= latch;
        }
        else if (write_mode == 1)
        {
            data32 = latch;
        }
        else // write_mode == 0
        {
            data32 = data;
            data32 |= (data32<<8);
            data32 |= (data32<<16);

            u8 rotate_amount = (gfx_regs[DATA_ROTATE]&0x07);
            u8 function_select = (gfx_regs[DATA_ROTATE]>>3)&0x03;

            if (rotate_amount)
            {
                u32 lomask = 0xFF>>rotate_amount;
                lomask |= lomask<<8;
                lomask |= lomask<<16;
                data32 = ((data32>>rotate_amount)&lomask) | ((data32<<(8-rotate_amount))&~lomask);
            }

            u32 enable_set_reset = BYTELOOKUP[gfx_regs[ENABLE_SET_RESET]&0x0F];
            u32 set_reset = BYTELOOKUP[gfx_regs[SET_RESET]&0x0F];

            data32 = (data32&~enable_set_reset) | (set_reset&enable_set_reset);

            if (function_select == 1)
                data32 &= latch;
            else if(function_select == 2)
                data32 |= latch;
            else if(function_select == 3)
                data32 ^= latch;
        }

        mem[address] = (data32&mask&bmask) | (latch&mask&~bmask) | (mem[address]&~mask);
    }

    u8 r8(u32 address)
    {
        if (!adjust_address_for_memory_map(address))
        {
            //if (debugprint)
            //    std::cout << "vga R?!" << std::endl;
            latch = 0xFFFFFFFF;
            return 0xFF;
        }

        //if (debugprint)
        //    std::cout << "vga R=" << std::hex << address << ", readmode=" << ((gfx_regs[MODE_REGISTER]&0x08)?"cmp":"norm") << std::endl;

        u32 plane_id = (gfx_regs[READ_MAP_SELECT])&0x03;

        if (chain4())
        {
            plane_id = address & 3;
            address >>= 2;
        }
        else if (chain())
        {
            plane_id = (plane_id&0xFE) | (address & 1);
            address >>= 1;
        }
        else if (r_oddeven() || w_oddeven())
        {
            plane_id = (plane_id&0xFE) | (address & 1);
            address >>= 1;
        }

        address &= 0xFFFF;
        latch = mem[address];

        u8 ret{};
        if(gfx_regs[MODE_REGISTER]&0x08) // color compare
        {
            u32 color_compare = BYTELOOKUP[gfx_regs[COLOR_COMPARE]&0x0F];
            u32 color_dontcare = BYTELOOKUP[gfx_regs[COLOR_DONT_CARE]&0x0F];

            u32 ret32 = ((latch^color_compare)&color_dontcare);
            ret32 |= (ret32>>16);
            ret32 |= (ret32>>8);
            ret = ~ret32;
        }
        else //normal read
        {
            u64 mask = 0xFFULL<<(plane_id<<3);
            ret = (u64(latch&mask)>>(plane_id<<3));
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
        cout << std::hex;
        for(int i=0; i<CRTC_REG_COUNT; ++i)
            cout << u32(crtc_regs[i]) << (i%4==3?"  ":" ");
        cout << endl;
    }

    u8 switches{u8(~0b0110)};
    u8 feature_control{};
    u8 input_status_1{};

    enum ATTR_REGISTER //index, value = 3C0, with a flipflop. reading from 3DA resets flipflop
    {
        PALETTE, //0x0-0xF
        MODE_CONTROL_ATTR = 0x10,
        OVERSCAN_COLOR,
        COLOR_PLANE_ENABLE,
        HORIZONTAL_PEL_PANNING,
        COLOR_SELECT
    } attr_choice{};
    bool attr_flipflop{};
    static const u8 ATTR_REG_COUNT = 0x15;
    u8 attr_regs[ATTR_REG_COUNT] = {};

    enum SEQ_REGISTER //index=3C4, value=3C5
    {
        RESET,
        CLOCKING_MODE,
        MAP_MASK,
        CHAR_MAP_SELECT,
        MEMORY_MODE,
        GD5428_UNLOCK_ALL_EXTENSIONS=0x06,//0x06: load with xxx1x010 to enable. otherwise it's 00001111
        GD5428_EXTENDED_SEQ_MODE=0x07,//0x07: 7:4 memorysegment 3:0, 3 reserved, 2:1 select CRTC char clock divider, 0 select high-res 256 color
        GD5428_EEPROM_CONTROL=0x08, //0x08: doesn't exist on 486 local bus or VLB
        GD5428_SCRATCHPAD_0=0x09, //internal use only
        GD5428_SCRATCHPAD_1=0x0A, //internal use only
        GD5428_SCRATCHPAD_2=0x14, //internal use only (689)
        GD5428_SCRATCHPAD_3=0x15, //internal use only (689)

        GD5428_VCLK0_NUMERATOR=0x0B, //7 reserved, 6-0 value
        GD5428_VCLK1_NUMERATOR=0x0C, //7 reserved, 6-0 value
        GD5428_VCLK2_NUMERATOR=0x0D, //7 reserved, 6-0 value
        GD5428_VCLK3_NUMERATOR=0x0E, //7 reserved, 6-0 value

        GD5428_DRAM_CONTROL=0x0F, // 4:3 DRAM data bus width, 2:0 read-only CF11:9 others look at the doc
        GD5428_GFX_CURSOR_POS_X=0x10, //special, high 8 bits in value, low 3 bits in high 3 bits of index reg
        GD5428_GFX_CURSOR_POS_Y=0x11, //same as above
        GD5428_CURSOR_ATTRS=0x12, //7 overscan color protect, 2 cursor size select 1=64x64,0=32x32, 1 enable DAC extended color, 0 cursor enable
        GD5428_CURSOR_PATTERN=0x13, //5:0 select 32x32, or 5:2 select 64x64 cursor

        GD5428_PERFORMANCE_TUNING=0x16, //don't write to this!
        GD5428_CFG_READBACK_EXT_CTRL=0x17, //not 5420. has 5429 specific ones (6 2 1), 5:3 read system bus select, 0 shadow DAC writes on local bus
    } seq_choice{};
    static const u8 SEQ_REG_COUNT = 0x20;
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
    static const u8 GFX_REG_COUNT = 0x09;
    u8 gfx_regs[GFX_REG_COUNT] = {};

    enum CRTC_REGISTER //index=3D4, value=3D5
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
        MAX_SCAN_LINE,
        CURSOR_START,
        CURSOR_END,

        START_ADDRESS_H,
        START_ADDRESS_L,
        CURSOR_LOC_H,
        CURSOR_LOC_L,

        V_RETRACE_START,
        V_RETRACE_END,
        V_DISPLAY_END,
        OFFSET,

        UNDERLINE_LOCATION,
        V_BLANK_START,
        V_BLANK_END,
        MODE_CONTROL_CRTC,

        LINE_COMPARE,

        //these three were never documented by IBM
        READ_BACK_CRT_LATCH = 0x22,
        ATTRIBUTE_TOGGLE_READBACK = 0x24,
        ATTRIBUTE_INDEX_READBACK = 0x26,

        /* GD5428 extended 0x19 0x1A 0x1B 0x25 0x27 */
    } crtc_choice{};

    static const u8 CRTC_REG_COUNT = 0x40;
    u8 crtc_regs[CRTC_REG_COUNT] = {};

    u8 misc{}; //3C2 write, 3CC read

    u8 horizontal_retrace{};
    u8 vertical_retrace{};
    bool retrace{};
    u16 current_startaddress{};

    u32 totalvsync{};

    u8 read(u8 port) //VGA port reading
    {
        u8 port_add = (misc&0x01)?0x20:0x00;
        u8 data{};

        if (port == 0x1C) // 3CC - Miscellaneous Output Register (read)
        {
            data = misc;
        }
        else if (port == 0x11) // 3C1 - Attribute Controller Data Read
        {
            if (attr_choice < ATTR_REG_COUNT)
                data = attr_regs[attr_choice];
        }
        else if (port == 0x15) // 3C5 - Sequencer Data
        {
            if (seq_choice < SEQ_REG_COUNT)
                data = seq_regs[seq_choice];
        }
        else if (port == 0x16) // 3C6 - DAC Mask
        {
            data = dac_mask;
        }
        else if (port == 0x17) // 3C7 - DAC State (read)
        {
            data = dac_state_register;
        }
        else if (port == 0x18)
        {
            data = dac_write_index;
        }
        else if (port == 0x19) // 3C9 - DAC Data
        {
            if (dac_state >= 1 && dac_state <= 3)
            {
                data = dac_palette[dac_read_index*3+dac_state - 1];
                dac_state++;
                if (dac_state > 3)
                {
                    dac_state = 1;
                    dac_read_index++;
                }
            }
        }
        else if (port == 0x1F) // 3CF - Graphics Controller Data
        {
            if (gfx_choice < GFX_REG_COUNT)
                data = gfx_regs[gfx_choice];
        }
        else if (port == 0x05+port_add) // 3D5 - CRTC Data
        {
            if (crtc_choice < CRTC_REG_COUNT)
                data = crtc_regs[crtc_choice];
            else if (crtc_choice == 0x1E)
                //trident SVGA:
                //bits 76: 00=256k, 01=512k, 10=768k, 11=1024k vram
                data = 0b00'00'0000;
        }
        else if (port == 0x0A+port_add) // 3DA - Input Status Register 1
        {
            data |= retrace;
            data |= (vertical_retrace<<3);
            attr_flipflop = false;
        }
        else if (port == 0x12) // 3C2 - Input Status Register 0
        {
            data = ((switches>>((misc>>2)&3))&1)<<4;
        }

        if (debugprint)
            cout << "VGA READ! " << u32(port+0x3B0) << ":" << u32(data) << std::endl;
        return data;
    }

    void write(u8 port, u8 data)
    {
        u8 port_add = (misc&0x01)?0x20:0x00;

        if (port == 0x10) // 3C0 - Attribute Controller
        {
            if (!attr_flipflop)
            {
                attr_choice = ATTR_REGISTER(data&0x1F);
            }
            else if(attr_choice < ATTR_REG_COUNT)
            {
                attr_regs[attr_choice] = data;
                if(debugprint)
                    cout << "vga w attr " << u32(attr_choice) << ":" << u32(data) << std::endl;
            }
            attr_flipflop = !attr_flipflop;
        }
        else if (port == 0x12) // 3C2 - Miscellaneous Output Register
        {
            misc = data;
            if(debugprint)
                cout << "vga w misc " << u32(data) << std::endl;
        }
        else if (port == 0x14) // 3C4 - Sequencer Address
        {
            seq_choice = SEQ_REGISTER(data&0x1F);
        }
        else if (port == 0x15) // 3C5 - Sequencer Data
        {
            if (seq_choice < SEQ_REG_COUNT)
            {
                seq_regs[seq_choice] = data;
                if(debugprint)
                    cout << "vga w seq  " << u32(seq_choice) << ":" << u32(data) << std::endl;
            }
        }
        else if (port == 0x16) // 3C6 - DAC Mask
        {
            dac_mask = data;
        }
        else if (port == 0x17) // 3C7 - DAC Address Read Mode
        {
            dac_state_register = 0;
            dac_read_index = data;
            dac_state = 1;
        }
        else if (port == 0x18) // 3C8 - DAC Address Write Mode
        {
            dac_state_register = 3;
            dac_write_index = data;
            dac_state = 1;
        }
        else if (port == 0x19) // 3C9 - DAC Data
        {
            if (dac_state >= 1 && dac_state <= 3)
            {
                dac_palette[dac_write_index*3+dac_state - 1] = data & 0x3F;
                dac_state++;
                if (dac_state > 3)
                {
                    dac_state = 1;
                    dac_write_index++;
                }
            }
        }
        else if (port == 0x1E) // 3CE - Graphics Controller Address
        {
            gfx_choice = GFX_REGISTER(data&0x0F);
        }
        else if (port == 0x1F) // 3CF - Graphics Controller Data
        {
            if (gfx_choice < GFX_REG_COUNT)
            {
                gfx_regs[gfx_choice] = data;
                update_memory_map();
                if(debugprint)
                    cout << "vga w gfx  " << u32(gfx_choice) << ":" << u32(data) << std::endl;
            }
        }
        else if (port == 0x04+port_add) // 3D4 - CRTC Address
        {
            crtc_choice = CRTC_REGISTER(data&0x1F);
        }
        else if (port == 0x05+port_add) // 3D5 - CRTC Data
        {
            if (crtc_choice < CRTC_REG_COUNT)
            {
                // Check write protection for registers 0-7
                if (crtc_choice <= 7 && (crtc_regs[V_RETRACE_END] & 0x80))
                {
                    // Write protected
                    return;
                }

                crtc_regs[crtc_choice] = data;
                update_crtc_params();

                if(debugprint)
                    if (crtc_choice != 0x0e && crtc_choice != 0x0F)//not cursor position
                        cout << "vga w crtc " << u32(crtc_choice) << ":" << u32(data) << std::endl;
            }
        }
        else if (port == 0x08+port_add || port == 0x09+port_add); // Legacy CGA ports
        else if (port == 0x0A+port_add)
        {
            feature_control = data;
        }
        else if (port == 0x0F || port == 0x2F){} // Legacy ports
    }

    // Monitor timing and display generation
    struct MegaCounter
    {
        u32 length[2] = {};
        u32 count = {};
        bool prev = {};

        bool get_prev() const
        {
            return prev^get_polarity();
        }

        bool get_polarity() const
        {
            return (length[0]>length[1]);
        }

        bool cycle(bool data)
        {
            bool change{prev != data};
            if (change)
            {
                length[data] = count;
                count = 0;
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

    void monitor_cycle(u8 r, u8 g, u8 b, u8 vsync_pin, u8 hsync_pin)
    {
        bool vc = vsync_ctr.cycle(vsync_pin);
        bool hc = hsync_ctr.cycle(hsync_pin);

        if ((vc && !vsync_ctr.get_prev()))
        {
            prev_line_amount = linepos;
            if (screen.screenSizeY != prev_line_amount)
            {
                std::cout << "APERTURE " << std::dec << screen.screenSizeX << "x" << screen.screenSizeY << std::endl << std::hex;
            }
            screen.screenSizeY = prev_line_amount;
            linepos = 0;
        }
        if (hc && !hsync_ctr.get_prev() && !vsync_ctr.get_prev())
        {
            prev_col_times = (prev_col_amount==colpos)?prev_col_times+1:0;
            prev_col_amount = colpos;
            if (prev_col_times >= 4)
            {
                if (screen.screenSizeX != prev_col_amount)
                {
                    std::cout << "APERTURE " << std::dec << screen.screenSizeX << "x" << screen.screenSizeY << std::endl << std::hex;
                }
                screen.screenSizeX = prev_col_amount;
            }
            colpos = 0;
            ++linepos;
            ++hframes;
        }
        if (!hsync_ctr.get_prev())
        {
            ++colpos;
        }

        u32 color = (b << 16) | (g << 8) | r;
        u32 renderline = linepos;
        u32 rendercol = colpos;

        if (renderline < screen.Y && rendercol < screen.X)
        {
            screen.pixels[renderline * screen.X + rendercol] = color;
        }
    }

    u32 column{};
    u32 logical_line{};
    u32 scan_line{};
    u32 line_inside_character{};
    u32 vsyncadjust{};
    bool hsync{}, vsync{};
    u32 hsync_monitor_ctr{};
    u32 vsync_monitor_ctr{};
    u32 frames{};
    u32 hframes{};

    u16 v_total{};
    u16 v_display_end{};
    u16 v_retrace_start{};
    u16 vblank_start{};
    u16 vblank_end{};
    u16 h_blank_end{};
    u8 scan_doubling{};
    u16 crtc_offset{};

    void update_crtc_params()
    {
        // Extract timing values with overflow bits
        //
        h_blank_end = (crtc_regs[H_BLANK_END]&0x1F) + ((crtc_regs[H_RETRACE_END]&0x80)?0x20:0x00);
        if constexpr (GD5428)
        {
            h_blank_end |= (crtc_regs[0x1A]<<2)&0xC0;
        }
        //0x06
        v_total = crtc_regs[V_TOTAL] + ((crtc_regs[OVERFLOW]&0x01)?0x100:0x000) + ((crtc_regs[OVERFLOW]&0x20)?0x200:0x000);
        //0x12
        v_display_end = crtc_regs[V_DISPLAY_END] + ((crtc_regs[OVERFLOW]&0x02)?0x100:0x000) + ((crtc_regs[OVERFLOW]&0x40)?0x200:0x000);
        //0x10
        v_retrace_start = crtc_regs[V_RETRACE_START] + ((crtc_regs[OVERFLOW]&0x04)?0x100:0x000) + ((crtc_regs[OVERFLOW]&0x80)?0x200:0x000);
        //0x15
        vblank_start = crtc_regs[V_BLANK_START] + ((crtc_regs[OVERFLOW]&0x08)?0x100:0x000) + ((crtc_regs[MAX_SCAN_LINE]&0x20)?0x200:0x000);;
        //0x16
        vblank_end = crtc_regs[V_BLANK_END];
        if constexpr (GD5428)
        {
            vblank_end |= (crtc_regs[0x1A]<<2)&0x300;
        }
        crtc_offset = crtc_regs[OFFSET];
        if constexpr (GD5428)
        {
            crtc_offset |= (crtc_regs[0x1B]&0x10)?0x100:0x000;
        }

        //TODO: line compare register


        scan_doubling = (crtc_regs[MAX_SCAN_LINE]&0x80)>>7;
    }
    void cycle() // 8 pixels per cycle
    {
        bool is_graphics_mode = !(seq_regs[MEMORY_MODE]&0x01);


        column += 1;
        column = (column>=(crtc_regs[H_TOTAL]+5)?0:column);

        if (column == 0) // new line
        {
            ++scan_line;
            ++line_inside_character;
            if ((line_inside_character>>scan_doubling) > (crtc_regs[MAX_SCAN_LINE]&0x1F))
            {
                line_inside_character = 0;
                ++logical_line;
            }

            bool all_lines_drawn = (scan_line > v_total);
            if (all_lines_drawn)
            {
                logical_line = 0;
                scan_line = 0;
                line_inside_character = 0;
            }
        }

        vsync = (scan_line >= vblank_start && scan_line < vblank_start + 16);

        if (!vsync)
            vsync_monitor_ctr = 0;
        else
            ++vsync_monitor_ctr;

        bool do_print = false;
        if (vsync_monitor_ctr==1)
        {
            ++frames;
            current_startaddress = ((crtc_regs[START_ADDRESS_H]<<8) | crtc_regs[START_ADDRESS_L]);
            do_print = true;
        }

        bool monitor_vsync = vsync ^ bool(misc&0x80);

        u16 hsync_start = (crtc_regs[H_RETRACE_START]);
        u16 hsync_end = hsync_start + ((crtc_regs[H_RETRACE_END]&0x1F));
        hsync = (column >= hsync_start && column <= hsync_end);
        bool monitor_hsync = hsync ^ bool(misc&0x40);

        vertical_retrace = (scan_line >= v_retrace_start && scan_line <= v_retrace_start + (crtc_regs[V_RETRACE_END]&0x0F));
        retrace = (vertical_retrace || hsync);

        bool display_enable = !((column > crtc_regs[H_DISPLAY_END]) || (scan_line > v_display_end));

        if (is_graphics_mode)
        {
            bool cms0 = crtc_regs[MODE_CONTROL_CRTC]&0x01;

            u32 offset=0;
            if (!cms0)
            {
                if (line_inside_character>>scan_doubling)
                {
                    //12 for lores cga, 13 for hires cga
                    if (w_oddeven())
                        offset += (1<<12);
                    else
                        offset += (1<<13);

                }

            }

            offset += current_startaddress + logical_line*crtc_offset*2 + column;

            if (attr_regs[MODE_CONTROL_ATTR] & 0x40) //256 color mode
            {
                u32 colors{};
                if (!display_enable)
                {
                    colors = u32(attr_regs[OVERSCAN_COLOR]) * 0x01010101U;
                }
                else
                {
                    colors = r32(offset);
                }

                for(int i=0; i<4; ++i)
                {
                    u8 color_index = colors&dac_mask; //dac_mask is u8, it's fine
                    u8 r = (dac_palette[color_index*3+0] & 0x3F) << 2;
                    u8 g = (dac_palette[color_index*3+1] & 0x3F) << 2;
                    u8 b = (dac_palette[color_index*3+2] & 0x3F) << 2;
                    monitor_cycle(r, g, b, monitor_vsync, monitor_hsync);
                    colors >>= 8;
                }
            }
            else // 16-color planar mode
            {
                for(int i=0; i<8; ++i)
                {
                    int pel_panned_i = i + (attr_regs[HORIZONTAL_PEL_PANNING]&0x07);
                    const u32 mask = 0x80808080;
                    u32 color = attr_regs[OVERSCAN_COLOR];
                    if (display_enable)
                    {
                        u32 data = r32(offset + (pel_panned_i>>3));

                        if(gfx_regs[MODE_REGISTER]&0x20) //shift register
                        {
                            color = ((data<<((i^0x04)*2))&0xC000C000)>>14;
                            //0b00000000'000000ab'00000000'000000cd
                        }
                        else
                        {
                            color = ((data<<(pel_panned_i&7))&mask)>>7; //pixel from 0 to 7
                            //0b0000000a'0000000b'0000000c'0000000d
                            color |= color >> 7;
                            //0b0000000a'000000ab'000000bc'000000cd
                        }
                        color |= color >> 14;
                        //0b0000000a'000000ab'00000abc'0000abcd
                        color &= attr_regs[COLOR_PLANE_ENABLE]&0x0F;
                        //0b00000000'00000000'00000000'0000abcd
                    }
                    color = attr_regs[color];
                    u8 r = (dac_palette[color*3+0] & 0x3F) << 2;
                    u8 g = (dac_palette[color*3+1] & 0x3F) << 2;
                    u8 b = (dac_palette[color*3+2] & 0x3F) << 2;

                    monitor_cycle(r, g, b, monitor_vsync, monitor_hsync);
                }
            }
        }
        else // Text mode
        {
            int x = column;
            u32 offset = current_startaddress + logical_line*crtc_offset*2 + x;
            u8 char_code = r32(offset) & 0xFF;

            u8 attribute = (r32(offset) >> 8) & 0xFF;
            u8 fg_color = attribute & 0x0F;
            u8 bg_color = (attribute >> 4) & 0x0F;
            u8 char_row = (r32((char_code<<5)+(line_inside_character>>scan_doubling)) >> 16) & 0xFF;

            for (u32 x_off = 0; x_off < 8; x_off++)
            {
                u8 mask = 1 << (7 - (x_off % 8));
                u8 color = (char_row & mask) ? fg_color : bg_color;
                if (!display_enable)
                    color = attr_regs[OVERSCAN_COLOR];
                color = attr_regs[color & 0x0F];
                u8 r = (dac_palette[color*3+0] & 0x3F) << 2;
                u8 g = (dac_palette[color*3+1] & 0x3F) << 2;
                u8 b = (dac_palette[color*3+2] & 0x3F) << 2;
                monitor_cycle(r, g, b, monitor_vsync, monitor_hsync);
            }
            if (!(seq_regs[CLOCKING_MODE]&0x01))
            {
                u8 color{bg_color};
                if (char_code >= 0xC0 && char_code < 0xE0)
                {
                    color = (char_row & 1) ? fg_color : bg_color;
                }
                if (!display_enable)
                    color = attr_regs[OVERSCAN_COLOR];
                color = attr_regs[color & 0x0F];
                u8 r = (dac_palette[color*3+0] & 0x3F) << 2;
                u8 g = (dac_palette[color*3+1] & 0x3F) << 2;
                u8 b = (dac_palette[color*3+2] & 0x3F) << 2;
                monitor_cycle(r, g, b, monitor_vsync, monitor_hsync);
            }
        }
    }
};

using VGA = VGACARD<false>;
