#pragma once

#include <vector>
#include <cstdint>
#include <cstring> // For memset

// Forward declaration if needed by other headers, though likely not for this standalone VGA
// struct CPU80286;

struct VGA
{
    // Registers
    u8 crtc_address_register;
    u8 crtc_registers[25];       // Common: 0-18h. Max index used by some SVGA is 24 (18h). Let's use 25 for 0-24.
                                 // IBM VGA uses up to 0x18. Some sources say 0x3D (VGA) or 0x4A (XGA).
                                 // For standard VGA, 25 (0x00-0x18) is typical.

    u8 sequencer_address_register;
    u8 sequencer_registers[5];     // 0-4. (0x00-0x04)

    u8 graphics_controller_address_register;
    u8 graphics_controller_registers[9]; // 0-8. (0x00-0x08)

    u8 attribute_controller_address_register;
    u8 attribute_controller_registers[21]; // 0-14h (0-20). Index 0-15 are palette, 16-20 are mode control etc.
                                           // Actually 16 palette registers (0-15), then Mode Ctrl (16/10h),
                                           // Overscan Color (17/11h), Color Plane Enable (18/12h),
                                           // Horiz Pel Panning (19/13h), Color Select (20/14h).

    u8 misc_output_register;

    u8 dac_address_read_mode_register;  // PEL Address Read Mode (3C7)
    u8 dac_address_write_mode_register; // PEL Address Write Mode (3C8)
    u8 dac_data_register;             // PEL Data (3C9)
    u8 dac_pel_mask_register;         // PEL Mask (3C6)
    u8 dac_state; // 0: ready for index (read/write), 1: ready for R, 2: ready for G, 3: ready for B (for write)
                  // For read: 0: ready for index, 1: sent R, 2: sent G, 3: sent B (then back to 0)

    // VGA DAC stores 256 colors, each color is 3x6-bit values (R,G,B).
    // We can store them as raw bytes or packed. Let's use raw bytes for simplicity.
    u8 dac_palette[256 * 3]; // 256 entries, each R, G, B (6-bits each, stored in u8)

    std::vector<u8> video_memory;
    bool attribute_flip_flop; // For 3C0/3C1 access state

    // Font data (e.g., 8x16 font, 256 characters)
    std::vector<u8> character_font_data;

    // Pixel buffer for rendering output
    std::vector<u32> pixel_buffer;
    int screen_width;
    int screen_height;

    // Constructor
    VGA() : video_memory(256 * 1024), // 256KB VGA RAM
            screen_width(640), screen_height(400) // Default for 80x25 text mode (8*80 x 16*25)
    {
        pixel_buffer.resize(screen_width * screen_height);
        character_font_data.resize(256 * 16); // Allocate space for 256 chars, 16 bytes each
        reset_registers();
        load_embedded_font(); // Load an embedded font
    }

    void load_embedded_font() {
        // Embed a very small font for a few characters directly here.
        // This avoids needing a large static const array definition in the header for now.
        // Format: 16 bytes per character, 8 pixels wide.
        std::memset(character_font_data.data(), 0, 256 * 16); // Clear font data

        // Char 0 (Null) - already zero
        // Char ' ' (32)
        unsigned char space_font[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
        std::memcpy(&character_font_data[32 * 16], space_font, 16);

        // Char 'A' (65)
        unsigned char A_font[] = {0x00,0x00,0x18,0x24,0x42,0x42,0x7E,0x42,0x42,0x42,0x42,0x00,0x00,0x00,0x00,0x00};
        std::memcpy(&character_font_data[65 * 16], A_font, 16);

        // Char 'B' (66)
        unsigned char B_font[] = {0x00,0x00,0x7C,0x42,0x42,0x7C,0x42,0x42,0x42,0x7C,0x00,0x00,0x00,0x00,0x00,0x00};
        std::memcpy(&character_font_data[66 * 16], B_font, 16);

        // Char 'C' (67)
        unsigned char C_font[] = {0x00,0x00,0x3C,0x42,0x40,0x40,0x40,0x40,0x42,0x3C,0x00,0x00,0x00,0x00,0x00,0x00};
        std::memcpy(&character_font_data[67 * 16], C_font, 16);
    }

    void reset_registers()
    {
        crtc_address_register = 0;
        std::memset(crtc_registers, 0, sizeof(crtc_registers));
        // Some common non-zero defaults for CRTC if known, otherwise 0 is a start.
        // For example, CRTC Reg 0x00 (Horiz Total) might be ~98-100 for text modes.
        // CRTC Reg 0x11 (Vertical Blank End) might have bit 5 (Protect CR0-7) set.
        // For now, zeros are fine, specific init can be a later task.

        sequencer_address_register = 0;
        std::memset(sequencer_registers, 0, sizeof(sequencer_registers));
        sequencer_registers[0] = 0x03; // SR00: Reset (typically 01 for run, 03 for reset state)
        sequencer_registers[1] = 0x01; // SR01: Clocking Mode (e.g. 0x01 for 8-dot fonts, normal operation)
        sequencer_registers[2] = 0x0F; // SR02: Map Mask (Plane Mask) - all planes enabled
        sequencer_registers[3] = 0x00; // SR03: Character Map Select
        sequencer_registers[4] = 0x0E; // SR04: Memory Mode (e.g., 0x02 for text, 0x06 for graphics, 0x0E for chain4) - 0x0E enables extended memory

        graphics_controller_address_register = 0;
        std::memset(graphics_controller_registers, 0, sizeof(graphics_controller_registers));
        // GC05 (Mode Register): might need specific bits for write mode, read mode etc.
        // GC06 (Miscellaneous): Graphics mode / Alpha mode, Chain (0 for text, specific for graphics)

        attribute_controller_address_register = 0;
        std::memset(attribute_controller_registers, 0, sizeof(attribute_controller_registers));
        // Palette registers 0-15 default to a standard EGA/VGA palette.
        // For example, AR00-AR0F could map to default EGA colors.
        // AR10 (Mode Control Register)
        // AR11 (Overscan Color)
        // AR12 (Color Plane Enable)
        // AR13 (Horizontal PEL Panning)
        // AR14 (Color Select)

        misc_output_register = 0x63; // A common default: 10MHz clock, Enable RAM, IOAS=Color
                                     // Bit 0: IO Address Select (0=Color Emulation, 1=Mono Emulation)
                                     // Bit 1: Enable RAM
                                     // Bits 2-3: Clock Select (00=14MHz (from OSC), 01=16MHz (from OSC), 10=External, 11=External) - typically maps to 25/28MHz pixel clock select
                                     // Bit 4: Reserved
                                     // Bit 5: Page Select for high/low 64k region in some modes (0 for text mode normally)
                                     // Bit 6: Horizontal Sync Polarity (0=positive, 1=negative)
                                     // Bit 7: Vertical Sync Polarity (0=positive, 1=negative)
                                     // Common defaults: 0x63 (color), 0x67 (mono text?), 0xE3 (graphics)
                                     // Let's use a common color text mode default: 0x63 (25MHz, RAMena, Color IO, +hsync, +vsync)

        dac_address_read_mode_register = 0;
        dac_address_write_mode_register = 0;
        dac_data_register = 0;
        dac_pel_mask_register = 0xFF; // Default PEL Mask
        dac_state = 0; // Ready for index

        // Initialize DAC palette to a grayscale ramp or zeros
        for (int i = 0; i < 256; ++i) {
            dac_palette[i * 3 + 0] = i % 64; // R (6-bit, 0-63)
            dac_palette[i * 3 + 1] = i % 64; // G (6-bit, 0-63)
            dac_palette[i * 3 + 2] = i % 64; // B (6-bit, 0-63)
        }
        // Standard VGA text mode often uses the first 16 colors from the EGA palette.
        // This can be refined later.

        attribute_flip_flop = false;
    }

    // I/O Methods (stubs)
    void write_crtc_address(u8 data) { crtc_address_register = data & 0x1F; /* Mask to typical VGA range */ }
    void write_crtc_data(u8 data) { if (crtc_address_register < sizeof(crtc_registers)) crtc_registers[crtc_address_register] = data; }
    u8 read_crtc_data() { if (crtc_address_register < sizeof(crtc_registers)) return crtc_registers[crtc_address_register]; return 0xFF; }

    void write_sequencer_address(u8 data) { sequencer_address_register = data & 0x07; /* Mask to valid range */ }
    void write_sequencer_data(u8 data) { if (sequencer_address_register < sizeof(sequencer_registers)) sequencer_registers[sequencer_address_register] = data; }
    u8 read_sequencer_data() { if (sequencer_address_register < sizeof(sequencer_registers)) return sequencer_registers[sequencer_address_register]; return 0xFF; }

    void write_graphics_controller_address(u8 data) { graphics_controller_address_register = data & 0x0F; /* Mask to valid range */ }
    void write_graphics_controller_data(u8 data) { if (graphics_controller_address_register < sizeof(graphics_controller_registers)) graphics_controller_registers[graphics_controller_address_register] = data; }
    u8 read_graphics_controller_data() { if (graphics_controller_address_register < sizeof(graphics_controller_registers)) return graphics_controller_registers[graphics_controller_address_register]; return 0xFF; }

    // Attribute controller is special: address and data are same port (3C0)
    // A flip-flop determines if it's an address or data write.
    void write_attribute_address_or_data(u8 data) {
        if (attribute_flip_flop == false) { // Expecting Address
            attribute_controller_address_register = data /*& 0x1F*/; // Lower 5 bits for index, bit 5 for PAS (Palette Address Source)
            attribute_flip_flop = true; // Next write to 3C0 is data
        } else { // Expecting Data
            if ((attribute_controller_address_register & 0x1F) < sizeof(attribute_controller_registers)) {
                 attribute_controller_registers[attribute_controller_address_register & 0x1F] = data;
            }
            attribute_flip_flop = false; // Next write to 3C0 is address
        }
    }
    u8 read_attribute_controller_data() { // Reading from 3C1
        if ((attribute_controller_address_register & 0x1F) < sizeof(attribute_controller_registers)) {
            return attribute_controller_registers[attribute_controller_address_register & 0x1F];
        }
        return 0xFF;
    }
    void reset_attribute_flip_flop() { attribute_flip_flop = false; }


    void write_misc_output(u8 data) { misc_output_register = data; }
    u8 read_misc_output() { return misc_output_register; }

    // DAC I/O
    void write_dac_mask(u8 data) { dac_pel_mask_register = data; }
    u8 read_dac_mask() { return dac_pel_mask_register; }

    void write_dac_address_read_mode(u8 data) {
        dac_address_read_mode_register = data;
        dac_state = 1; // Start reading from this index: next read from 3C9 will be R
    }
    void write_dac_address_write_mode(u8 data) {
        dac_address_write_mode_register = data;
        dac_state = 1; // Start writing to this index: next write to 3C9 will be R
    }

    void write_dac_data(u8 data) {
        u8 index = dac_address_write_mode_register;
        if (dac_state == 1) { // Writing R
            dac_palette[index * 3 + 0] = data & 0x3F; // 6-bit color
            dac_state = 2;
        } else if (dac_state == 2) { // Writing G
            dac_palette[index * 3 + 1] = data & 0x3F;
            dac_state = 3;
        } else if (dac_state == 3) { // Writing B
            dac_palette[index * 3 + 2] = data & 0x3F;
            dac_state = 1; // Ready for next R (or next index)
            dac_address_write_mode_register++; // Auto-increment index
        }
        dac_data_register = data; // Store last written value
    }

    u8 read_dac_data() {
        u8 index = dac_address_read_mode_register;
        u8 val = 0;
        if (dac_state == 1) { // Reading R
            val = dac_palette[index * 3 + 0];
            dac_state = 2;
        } else if (dac_state == 2) { // Reading G
            val = dac_palette[index * 3 + 1];
            dac_state = 3;
        } else if (dac_state == 3) { // Reading B
            val = dac_palette[index * 3 + 2];
            dac_state = 1; // Ready for next R (or next index)
            dac_address_read_mode_register++; // Auto-increment index
        }
        return val & 0x3F; // Return 6-bit color value
    }

    u8 read_dac_state_register() { // Port 3C7
        // Bit 0,1: 00 = DAC ready for read/write (index can be set)
        //         11 = DAC busy with read/write (index cannot be set)
        // This is a simplification. DAC is 'busy' if not in state 0 for setting index.
        return (dac_state == 0) ? 0x00 : 0x03;
    }


    // Input Status Registers
    u8 read_input_status_0() { // Port 3C2 (Read) - This is also mirror of Misc Output Register on some VGAs or Feature Control Register
        // Bit 0: Switch sense (often 0 for VGA)
        // Bit 1-3: Reserved
        // Bit 4: Display Enable from CRTC (0 during retrace, 1 when displaying) - needs to be dynamic
        // Bit 5-6: Reserved (sometimes sense bits on older cards)
        // Bit 7: VSYNC state for MDA/CGA compatibility (not usually primary VGA VSYNC)
        // For now, a static value, should reflect actual display status.
        // This port is also used for writing to Feature Control Register on some VGA clones.
        // For reading, it's Input Status 0.
        // A common behavior is that reading 3C2 clears the attribute controller flip-flop to expect address next.
        // However, standard IBM VGA documentation states reading Input Stat 1 (3BA/3DA) resets the flip-flop.
        // Let's stick to INSTAT1 resetting it for now.
        return 0x10; // Simulate display enabled, other bits 0.
    }

    u8 read_input_status_1_color() { // Port 3DA (Color mode)
        reset_attribute_flip_flop(); // Reading INSTAT1 resets the Attribute Controller flip-flop
        // Bit 0: Display Enable (DE) - 0 during active display retrace periods, 1 when displaying.
        // Bit 1: Vertical Retrace (VR) - 1 during vertical retrace. (Not Vertical Blanking)
        // Bit 2: Light Pen Switch status (0=off, 1=on) - Not emulated
        // Bit 3: Vertical Blanking (VB) - 1 during vertical blanking interval. (VGA specific, not HR like CGA)
        // Bit 4-5: Diagnostic bits. (Can be read/write through Attribute reg 1Bh index 0,1)
        // Bit 6-7: Reserved (0)
        // Needs to be dynamic based on timing.
        static u8 counter = 0; counter++; // very simple simulation
        u8 status = 0;
        bool vsync_active = (counter % 120) < 2;  // Simulate short VSYNC pulse
        bool vblank_active = (counter % 120) < 10; // Simulate longer VBLANK period

        if (!vblank_active && !vsync_active) status |= 0x01; // Display Enabled (not in VBLANK)
        if (vsync_active) status |= 0x08; // Bit 3 is Vertical Sync Pulse on VGA (was VRetrace for CGA)
                                          // Some docs say bit 3 is "Video" (1 if in H/V retrace).
                                          // Let's use bit 3 for VSYNC active (matches some references for 3DAh)
                                          // Bit 1 for vertical retrace is sometimes mentioned for older cards.
                                          // For VGA, bit 3 is more consistently "Vertical Syncing"
        // A common interpretation for VGA's 3DAh:
        // Bit 0 (DE): 1 if display is enabled by CRTC (not in H/V blank)
        // Bit 1 (LPEN): Light pen status (0=triggered, 1=not triggered)
        // Bit 2 (LPENSW): Light pen switch (0=closed, 1=open)
        // Bit 3 (VSYNC): Vertical Sync pulse active (1 during pulse)
        // Let's use a simplified model for now:
        // status |= 0x01; // Display time
        // if (vblank_active) status |= 0x08; // VSync/VBlank indicator

        return status; // Placeholder, needs proper timing simulation
    }

    u8 read_input_status_1_mono() {  // Port 3BA (Mono mode)
        reset_attribute_flip_flop(); // Reading INSTAT1 resets the Attribute Controller flip-flop
        // Bit 0: Display Enable.
        // Bit 1-2: Reserved.
        // Bit 3: Horizontal Retrace. (For VGA, this bit is often stated as 'Video' like 3DA bit 3)
        //        Or simply Vertical Sync for mono modes as well.
        // Bit 4-7: Reserved.
        static u8 counter = 0; counter++;
        u8 status = 0;
        // if (!((counter % 120) < 10)) status |= 0x01; // Display Enabled
        // if ((counter % 120) < 10) status |= 0x08;    // VSync
        return status; // Placeholder
    }

    // Memory Access Methods
    u8 read_memory(u32 address) {
        u32 vga_base_addr = 0xA0000;
        u8 memory_map_mode = (graphics_controller_registers[6] >> 2) & 0x03; // GR6 bits 2-3: Memory Map Select
        // 00: A0000-BFFFF (128K)
        // 01: A0000-AFFFF (64K)
        // 10: B0000-B7FFF (32K)
        // 11: B8000-BFFFF (32K)

        // Adjust mapping for text modes if graphics mode is not set
        // Attribute Mode Control (AR10) bit 0: Graphics Enable (1=Graphics, 0=Text)
        bool graphics_enabled = (attribute_controller_registers[0x10] & 0x01) != 0;

        if (!graphics_enabled) { // Text Mode
            if ((misc_output_register & 0x01) == 1 && address >= 0xB8000 && address <= 0xBFFFF) { // Color Text B8000-BFFFF
                vga_base_addr = 0xB8000;
                 // For text mode, reads are typically direct from plane 0 (char) and plane 1 (attr)
                 // This simplified model reads linearly. A full model would interleave plane0/1.
                u32 offset = address - vga_base_addr;
                if (offset < video_memory.size()) return video_memory[offset]; // Simplified: access plane 0/char
                return 0xFF;
            } else if ((misc_output_register & 0x01) == 0 && address >= 0xB0000 && address <= 0xB7FFF) { // Mono Text B0000-B7FFF
                vga_base_addr = 0xB0000;
                u32 offset = address - vga_base_addr;
                 // Similar simplification for mono text (plane 0 char, plane 2 attr in some hw)
                if (offset < video_memory.size()) return video_memory[offset];
                return 0xFF;
            }
            // If not in a recognized text mode range, or graphics enabled, proceed to graphics logic.
        }

        // Graphics Mode or non-text range access
        // Determine actual vga_base_addr based on memory_map_mode for graphics
        if (memory_map_mode == 0x01) { // A0000-AFFFF (64K)
            if (address >= 0xB0000) return 0xFF; // Out of 64K range
        } else if (memory_map_mode == 0x02) { // B0000-B7FFF (32K)
            vga_base_addr = 0xB0000;
            if (address >= 0xB8000) return 0xFF;
        } else if (memory_map_mode == 0x03) { // B8000-BFFFF (32K)
            vga_base_addr = 0xB8000;
        }
        // else memory_map_mode == 0x00 (A0000-BFFFF, 128K default), vga_base_addr = 0xA0000

        u32 vga_offset = address - vga_base_addr;

        // Read Mode (GR05 bit 3)
        if (graphics_controller_registers[5] & 0x08) { // Read Mode 1 (Color Compare)
            // TODO: Implement Color Compare logic
            return 0x00; // Placeholder
        } else { // Read Mode 0
            u8 plane_select = graphics_controller_registers[4] & 0x03; // GR4 bits 0-1: Read Map Select
            u32 plane_offset = (u32)plane_select * 0x10000; // Each plane is 64KB in typical planar modes
            // Note: Some modes like Chain4 might pack differently. This assumes standard planar.
            // Max VGA memory is 256KB, so 4 planes of 64KB.
            // Max offset within a plane is 0xFFFF.
            if (vga_offset >= 0x10000 && (sequencer_registers[4] & 0x08)) {} // Chained mode allows larger offset
            else vga_offset %= 0x10000;


            u32 final_address = plane_offset + vga_offset;
            if (final_address < video_memory.size()) {
                return video_memory[final_address];
            }
        }
        return 0xFF; // Should not happen if memory map is correct
    }

    void write_memory(u32 address, u8 data) {
        u32 vga_base_addr = 0xA0000;
        u8 memory_map_mode = (graphics_controller_registers[6] >> 2) & 0x03; // GR6 bits 2-3
        bool graphics_enabled = (attribute_controller_registers[0x10] & 0x01) != 0;

        if (!graphics_enabled) { // Text Mode
            if ((misc_output_register & 0x01) == 1 && address >= 0xB8000 && address <= 0xBFFFF) { // Color Text B8000-BFFFF
                vga_base_addr = 0xB8000;
                u32 offset = address - vga_base_addr;
                // Simplified: Write to plane 0 (char) or plane 1 (attr) based on address even/odd
                // For now, linear write.
                if (offset < video_memory.size()) video_memory[offset] = data;
                return;
            } else if ((misc_output_register & 0x01) == 0 && address >= 0xB0000 && address <= 0xB7FFF) { // Mono Text B0000-B7FFF
                vga_base_addr = 0xB0000;
                u32 offset = address - vga_base_addr;
                if (offset < video_memory.size()) video_memory[offset] = data;
                return;
            }
        }

        // Graphics Mode or non-text range access
        if (memory_map_mode == 0x01) { // A0000-AFFFF (64K)
            if (address >= 0xB0000) return;
        } else if (memory_map_mode == 0x02) { // B0000-B7FFF (32K)
            vga_base_addr = 0xB0000;
            if (address >= 0xB8000) return;
        } else if (memory_map_mode == 0x03) { // B8000-BFFFF (32K)
            vga_base_addr = 0xB8000;
        }

        u32 vga_offset = address - vga_base_addr;
        // Ensure offset is within a 64k plane boundary for standard planar modes, unless chain4 etc.
        if (vga_offset >= 0x10000 && !(sequencer_registers[4] & 0x08) ) { // Not chain4 mode and offset > 64k
             vga_offset %= 0x10000; // Wrap around within the plane
        }


        u8 write_mode = graphics_controller_registers[5] & 0x03; // GR5 bits 0-1
        u8 plane_mask = sequencer_registers[2]; // SR2: Map Mask (Plane Mask)

        // TODO: Data Rotate (GR3), Set/Reset (GR0, GR1), Bit Mask (GR8), Logical Ops (GR3)

        for (int plane = 0; plane < 4; ++plane) {
            if ((plane_mask >> plane) & 1) { // If this plane is enabled for writes
                u32 plane_offset = (u32)plane * 0x10000;
                u32 final_address = plane_offset + vga_offset;

                if (final_address < video_memory.size()) {
                    switch (write_mode) {
                        case 0: // Write Mode 0: Data is written after modification by rotate, set/reset, bitmask
                            // Simplified: direct data write for now
                            video_memory[final_address] = data;
                            break;
                        case 1: // Write Mode 1: Latched data is written
                            // TODO: Implement latch handling. For now, acts like Mode 0 or NOP.
                            // video_memory[final_address] = latches[plane];
                            break;
                        case 2: // Write Mode 2: Data bits 0-3 correspond to planes 0-3.
                                // Value 0xFF if bit is set, 0x00 if clear.
                            video_memory[final_address] = (data & (1 << plane)) ? 0xFF : 0x00;
                            break;
                        case 3: // Write Mode 3: Data is rotated by GR3, then masked by GR8 (Bit Mask)
                            // Simplified: direct data write for now (same as mode 0 for this stub)
                            // u8 rotated_data = ... (data >> (GR03_DataRotate & 7)) | (data << (8 - (GR03_DataRotate & 7)));
                            // u8 masked_data = rotated_data & graphics_controller_registers[8]; (GR8 is Bit Mask)
                            // video_memory[final_address] = masked_data;
                            video_memory[final_address] = data; // Placeholder
                            break;
                    }
                }
            }
        }
    }

    // Cycle method
    void cycle() {
        // Update display status (VSync, HSync for Input Status Regs) - TODO

        // Basic rendering logic
        bool graphics_mode = (attribute_controller_registers[0x10] & 0x01) != 0;
        if (!graphics_mode) {
            render_text_mode();
        } else {
            // render_graphics_mode(); // TODO
        }
    }

    u32 get_dac_color(u8 index) {
        if (index >= 256) return 0xFF000000; // Should not happen with valid palette indices (0-15 for attr)

        // DAC values are 6-bit. Scale to 8-bit for display. (val << 2) | (val >> 4) is a common way.
        u8 r = (dac_palette[index * 3 + 0] & 0x3F);
        u8 g = (dac_palette[index * 3 + 1] & 0x3F);
        u8 b = (dac_palette[index * 3 + 2] & 0x3F);

        u32 r8 = (r << 2) | (r >> 4);
        u32 g8 = (g << 2) | (g >> 4);
        u32 b8 = (b << 2) | (b >> 4);

        return 0xFF000000 | (r8 << 16) | (g8 << 8) | b8; // ARGB format (Alpha FF)
    }

    void render_text_mode() {
        // Determine active display parameters (simplified for Mode 3: 80x25, 16 color)
        // CR01: Horizontal Display End (number of characters - 1)
        // CR12: Vertical Display End (number of scanlines - 1)
        // CR09: Max Scan Line (character height - 1)

        int num_cols = (crtc_registers[1] + 1); // Horizontal Display End + 1 gives columns
        if (num_cols == 0 || num_cols > 80) num_cols = 80; // Default/clamp for typical mode 3

        int char_height = (crtc_registers[9] & 0x1F) + 1; // Max Scan Line + 1
        if (char_height == 0 || char_height > 32) char_height = 16; // Default/clamp

        // Vertical Display End (CR12) is total scanlines. Rows = TotalScanlines / CharHeight
        // Simplified: Assume 25 rows for now for standard 80x25 mode.
        // u16 vertical_display_end = ((crtc_registers[0x07] & 0x02) << 7) | crtc_registers[0x12];
        // int num_rows = (vertical_display_end + 1) / char_height;
        // if (num_rows == 0 || num_rows > 25) num_rows = 25;
        int num_rows = 25;


        // Adjust screen dimensions if CRTC changes them significantly, for now fixed to constructor values
        // screen_width = num_cols * 8; (assuming 8 pixel wide chars)
        // screen_height = num_rows * char_height;
        // pixel_buffer.resize(screen_width * screen_height); // Careful with frequent reallocs

        u32 video_ram_start_addr = 0xB8000; // For color text mode
        if ((misc_output_register & 0x01) == 0) { // Mono text mode
            video_ram_start_addr = 0xB0000;
        }

        if (character_font_data.empty()) return; // No font loaded

        for (int y = 0; y < num_rows; ++y) {
            for (int x = 0; x < num_cols; ++x) {
                u32 mem_offset = video_ram_start_addr + (y * num_cols + x) * 2;

                // Use the VGA's own read_memory which should handle memory mapping correctly
                // For text mode, this is simplified currently.
                u8 char_code = read_memory(mem_offset);
                u8 attr_byte = read_memory(mem_offset + 1);

                u8 fg_color_index = attr_byte & 0x0F;
                u8 bg_color_index = (attr_byte >> 4) & 0x0F; // Blink bit (bit 7) can modify this

                bool blink_enabled = (attribute_controller_registers[0x10] & 0x08) != 0; // AR10 bit 3: Enable Blink
                // Proper blinking requires timing. For now, if blink is enabled and attribute bit 7 is set,
                // use background color for foreground (effectively making it invisible periodically).
                // Or, map high intensity background if blink bit is set on attribute.
                // Standard: if blink enabled & attr bit 7 set, FG is itself, BG is also itself (text invisible)
                // OR: if blink bit set, use colors from palette entries 8-15 for FG.
                // Let's go with: if blink bit is set on attribute, FG color index gets bit 3 set (high intensity).
                if (attr_byte & 0x80 && !blink_enabled) { // Bit 7 is Intensity for FG if blink disabled
                    fg_color_index |= 0x08;
                }
                // If blink enabled and attr bit 7 set, this is blinking state. For non-blinking render:
                // treat as normal background. Or, if you want to show the "blink on" phase, use FG color.
                // For now, ignore true blinking, just handle intensity/background from attr_byte>>4.
                bg_color_index &= 0x07; // Standard VGA, background is only from low 3 bits of high nibble. Bit 7 is blink.


                u32 fg_rgb = get_dac_color(attribute_controller_registers[fg_color_index]); // Palette registers are in Attribute Controller
                u32 bg_rgb = get_dac_color(attribute_controller_registers[bg_color_index]);


                for (int char_scanline = 0; char_scanline < char_height; ++char_scanline) {
                    if ((char_code * 16 + char_scanline) >= character_font_data.size()) continue; // Font data bounds check

                    u8 font_byte = character_font_data[(char_code * 16) + char_scanline]; // Assuming 16 bytes per char for lookup

                    for (int char_pixel_x = 0; char_pixel_x < 8; ++char_pixel_x) { // Assuming 8-pixel wide chars
                        u32 target_pixel_color = bg_rgb;
                        if ((font_byte >> (7 - char_pixel_x)) & 0x01) {
                            target_pixel_color = fg_rgb;
                        }

                        int screen_x = x * 8 + char_pixel_x;
                        int screen_y = y * char_height + char_scanline;

                        if (screen_x < screen_width && screen_y < screen_height) {
                            pixel_buffer[screen_y * screen_width + screen_x] = target_pixel_color;
                        }
                    }
                }
            }
        }
    }

    const std::vector<u32>& get_pixel_buffer() const { return pixel_buffer; }
    int get_screen_width() const { return screen_width; }
    int get_screen_height() const { return screen_height; }

};

// Define the static font data (example: first few chars of a typical 8x16 font)
// Removed the problematic static const definition here.
// Font data is now directly embedded in load_embedded_font().
