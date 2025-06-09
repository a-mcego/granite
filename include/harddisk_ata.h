#pragma once
#include <vector>
#include <cstdint>

#include "harddisk.h"

struct HARDDISK_ATA
{
    DISKS& disks;

    CHIP8237& dma;
    CHIP8259& pic;
    CHIP8259& pic2;
    static const int SECTOR_SIZE = 512;

    // Status register bits
    static const u8 STATUS_ERR  = 0x01; // Error
    static const u8 STATUS_IDX  = 0x02; // Index
    static const u8 STATUS_CORR = 0x04; // Corrected data
    static const u8 STATUS_DRQ  = 0x08; // Data request
    static const u8 STATUS_DSC  = 0x10; // Drive seek complete
    static const u8 STATUS_DWF  = 0x20; // Drive write fault
    static const u8 STATUS_READY= 0x40; // Drive ready
    static const u8 STATUS_BUSY = 0x80; // Controller busy

    static const u32 SEND_IRQ_DELAY = 8;

    // Error register bits
    static const u8 ERROR_BBD  = 0x80; // Bad Block Detect
    static const u8 ERROR_ECC  = 0x40; // Data ECC Error
    static const u8 ERROR_ID   = 0x10; // ID Not Found
    static const u8 ERROR_ABRT = 0x04; // Aborted Command
    static const u8 ERROR_TR0  = 0x02; // Track 000 Error
    static const u8 ERROR_DAM  = 0x01; // Data Address Mark Not Found

    // Registers
    u8 error_register{};
    u8 write_precomp{};
    u8 sector_count{};
    u8 sector_number{1};
    u8 cylinder_low{};
    u8 cylinder_high{};
    u8 drive_head{};
    u8 status_register{STATUS_READY | STATUS_DSC};
    u8 command_register{};
    u16 data_register{};
    u8 fixed_disk_register{};

    // Buffer for IDENTIFY DRIVE data
    u16 identify_data_buffer[256];
    bool identify_command_active{false};
    bool is_writing{false}; // To distinguish between READ and WRITE commands for data port logic

    u8 current_disk()
    {
        return (drive_head&0x10) ? 1 : 0;
    }

    // Drive characteristics
    //u16 cylinders;
    //u8 heads;
    //u8 sectors_per_track;

    // Data buffer
    //std::vector<u8> disk_data;
    size_t current_offset{};

    HARDDISK_ATA(DISKS& disks_, CHIP8237& dma_, CHIP8259& pic_, CHIP8259& pic2_) : disks(disks_), dma(dma_), pic(pic_), pic2(pic2_)
    {
        // Initialize a basic hard disk (20MB)
        //cylinders = 615;
        //heads = 4;
        //sectors_per_track = 17;

        // Initialize disk data
        //size_t disk_size = (size_t)cylinders * heads * sectors_per_track * SECTOR_SIZE;
        //disk_data.resize(disk_size, 0);
    }

    void write(u8 port, u16 data)
    {
        //std::cout << globalsettings.current_IP << " ATA write " << u32(port) << ":" << u32(data) << std::endl;
        switch(port)
        {
            case 0: // Data Register
                data_register = data; // Store last written value
                if (is_writing && data_left > 0 && (status_register & STATUS_DRQ))
                {
                    auto& disk_data_vec = disks.disk[current_disk()].data;
                    if (current_offset < disk_data_vec.size())
                        disk_data_vec[current_offset] = data_register & 0xFF;
                    if (current_offset + 1 < disk_data_vec.size())
                        disk_data_vec[current_offset + 1] = (data_register >> 8) & 0xFF;

                    current_offset += 2;
                    data_left -= 2;

                    if (data_left == 0)
                    {
                        status_register &= ~(STATUS_BUSY | STATUS_DRQ);
                        status_register |= STATUS_READY | STATUS_DSC;
                        // error_register should be clear if no error occurred during write
                        send_irq_delay = SEND_IRQ_DELAY; // Signal command completion
                    }
                    // else, if more data is expected for the current sector or more sectors,
                    // DRQ remains high, BUSY might briefly toggle or stay high.
                    // For simplicity, DRQ stays high, BUSY stays high until all data_left is processed.
                }
                break;

            case 1: // Write precomp
                write_precomp = data;
                break;

            case 2: // Sector Count
                sector_count = data;
                break;

            case 3: // Sector Number
                sector_number = data;
                break;

            case 4: // Cylinder Low
                cylinder_low = data;
                break;

            case 5: // Cylinder High
                cylinder_high = data;
                break;

            case 6: // Drive/Head
                drive_head = data;
                break;

            case 7: // Command Register
                command_register = data;
                execute_command(data);
                break;

            case 0x0E: // 3F6
                fixed_disk_register = data;
                break;
        }
    }

    void send_irq()
    {
        if (!(fixed_disk_register&2))
        {
            pic2.request_interrupt(14-8);
        }
    }

    u32 data_left{};

    u16 read(u8 port)
    {
        u16 data{};
        switch(port)
        {
            case 0: // Data Register
                if (identify_command_active && data_left > 0 && (status_register & STATUS_DRQ))
                {
                    data_register = identify_data_buffer[current_offset / 2];
                    current_offset += 2;
                    data_left -= 2;
                    if (data_left == 0)
                    {
                        identify_command_active = false;
                        status_register &= ~(STATUS_DRQ | STATUS_BUSY); // BUSY should be clear already
                        status_register |= STATUS_READY | STATUS_DSC;
                        // No IRQ typically for IDENTIFY DRIVE completion itself.
                    }
                }
                else if (!is_writing && data_left > 0 && (status_register & STATUS_DRQ)) // Read sector data
                {
                    auto& disk_data_vec = disks.disk[current_disk()].data;
                    u8 byte_low = 0, byte_high = 0;
                    if (current_offset < disk_data_vec.size())
                        byte_low = disk_data_vec[current_offset];
                    if (current_offset + 1 < disk_data_vec.size())
                        byte_high = disk_data_vec[current_offset + 1];
                    data_register = byte_low | (byte_high << 8);

                    current_offset += 2;
                    data_left -= 2;

                    if (data_left == 0)
                    {
                        status_register &= ~(STATUS_BUSY | STATUS_DRQ);
                        status_register |= STATUS_READY | STATUS_DSC;
                        // error_register should be clear if no error occurred
                        send_irq_delay = SEND_IRQ_DELAY; // Signal command completion
                    }
                    // else, DRQ remains high, BUSY might be set by controller logic to fetch next sector internally
                }
                // If DRQ is not set, or data_left is 0, typically returns last latched value or garbage.
                // For simplicity, return last data_register value if no active DRQ transfer.
                data = data_register;
                break;

            case 1: // Error Register
                data = error_register;
                break;

            case 2: // Sector Count
                data = sector_count;
                break;

            case 3: // Sector Number
                data = sector_number;
                break;

            case 4: // Cylinder Low
                data = cylinder_low;
                break;

            case 5: // Cylinder High
                data = cylinder_high;
                break;

            case 6: // Drive/Head
                data = drive_head;
                break;

            case 7: // Status Register
                data = status_register;
                break;

            case 0x0F: // 3F7
                u8 result = 0;
                result |= (drive_head&0x10)?1:2;
                result |= (drive_head&0x0F)<<2;
                data = result;
                break;
        }
        //std::cout << globalsettings.current_IP << " ATA read " << u32(port) << ":" << u32(data) << std::endl;
        return data;
    }

    void execute_command(u8 cmd)
    {
        std::cout << globalsettings.current_IP << " ATA command: " << u32(cmd) <<  std::endl;

        if (false);
        else if (cmd == 0xEC) // IDENTIFY DRIVE
        {
            handle_identify_drive();
        }
        else if ((cmd&0xF0) == 0x10) //restore (Recalibrate)
        {
            send_irq_delay = SEND_IRQ_DELAY;
            status_register |= STATUS_BUSY;
            status_register &= ~STATUS_READY;
            error_register = 0; // Clear errors
            // On completion: set ready, dsc. No error implies success.
        }
        else if ((cmd&0xF0) == 0x70) //seek
        {
            send_irq_delay = SEND_IRQ_DELAY; // Seek complete will be signaled by IRQ
            status_register |= STATUS_BUSY;
            status_register &= ~STATUS_READY;
            error_register = 0;
            // On completion: set ready, dsc.
        }
        else if (cmd == 0x50) // FORMAT BAD TRACK (XT-IDE, not standard ATA for IDENTIFY)
        {
            // This is not IDENTIFY DRIVE. For now, let's make it a NOP that successfully completes.
            // Or, treat as an invalid command for ATA. Given it's 0x50, it's likely an older command.
            // For now, let's assume it completes successfully without error for compatibility.
            std::cout << "ATA Command 0x50 (FORMAT BAD TRACK) called. Completing successfully as NOP." << std::endl;
            status_register = STATUS_READY | STATUS_DSC;
            error_register = 0;
            send_irq_delay = SEND_IRQ_DELAY; // Signal completion via IRQ
        }
        else if (cmd == 0x20 || cmd == 0x21) // READ SECTORS (0x20 with retry, 0x21 without)
        {
            is_writing = false;
            prepare_rw_sector();
            error_register = 0;
            // BUSY will be cleared and DRQ set by prepare_rw_sector if successful
        }
        else if (cmd == 0x30 || cmd == 0x31) // WRITE SECTORS (0x30 with retry, 0x31 without)
        {
            is_writing = true;
            prepare_rw_sector();
            error_register = 0;
            // BUSY will be cleared and DRQ set by prepare_rw_sector if successful
        }
        else if ((cmd&0xFE) == 0x40) //read verify
        {
            send_irq_delay = SEND_IRQ_DELAY;
            status_register |= STATUS_BUSY; // No data transfer, just verify
            status_register &= ~STATUS_READY;
            error_register = 0;
            send_irq_delay = SEND_IRQ_DELAY; // Signal completion
        }
        else if (cmd==0x90) // EXECUTE DRIVE DIAGNOSTIC
        {
            // Master passes, slave not implemented or passes.
            status_register = STATUS_READY | STATUS_DSC; // Clear BUSY
            error_register = (current_disk() == 0) ? 0x01 : 0x01; // All HDDs passed. Bit 0 for drive 0, Bit 1 for drive 1 etc. Usually 0x01 means drive 0 ok.
            send_irq_delay = SEND_IRQ_DELAY;
        }
        else if (cmd==0x91) // INITIALIZE DRIVE PARAMETERS (Set Geometry)
        {
            // This command usually uses sector_count for sectors per track and drive_head for heads.
            // For 286 era, this is important.
            // The new geometry is ((drive_head & 0x0F) + 1) heads and sector_count sectors.
            // Cylinders are not set by this command directly, they are assumed from IDENTIFY or previous state.
            // Let's simulate this by updating our "type" if it's a dynamic disk or just acknowledging.
            // For now, just acknowledge. Some BIOSes send this.
            std::cout << "ATA Command 0x91 (INITIALIZE DRIVE PARAMETERS) called. H:" << u32((drive_head & 0x0F)+1) << " S:" << u32(sector_count) << std::endl;
            disks.disk[current_disk()].type.heads = (drive_head & 0x0F) + 1;
            disks.disk[current_disk()].type.sectors = sector_count;
            // Mark disk as "configured" by this command if needed, or just accept.
            status_register = STATUS_READY | STATUS_DSC; // Clear BUSY
            error_register = 0;
            send_irq_delay = SEND_IRQ_DELAY;
        }
        else
        {
            std::cout << "Unknown ATA command " << u32(cmd) << std::endl;
            error_register = ERROR_ABRT; // Abort
            status_register = STATUS_READY | STATUS_ERR | STATUS_DSC; // Clear BUSY
            send_irq_delay = SEND_IRQ_DELAY; // Signal error completion
            //std::abort(); // Keep it running for now
        }
    }

    void handle_identify_drive()
    {
        identify_command_active = true;
        is_writing = false; // It's a read from the host perspective
        current_offset = 0;
        data_left = 512;

        // Zero out the buffer first
        for(int i=0; i<256; ++i) identify_data_buffer[i] = 0;

        DISKTYPE& t = disks.disk[current_disk()].type;

        identify_data_buffer[0] = 0x0040; // General configuration: Non-removable, hard-sectored (bit 6 = 1 means not MFM, common for IDE)
                                          // Bit 15 = 0 for ATA device. Bit 7 = 1 (TAPE). Let's use a typical value.
                                          // 0x0400 (incomplete response) or 0x848A are other examples.
                                          // For XT-IDE Universal BIOS, 0x0040 seems fine for a generic fixed disk.
        identify_data_buffer[1] = t.cylinders;
        identify_data_buffer[2] = 0; // Reserved
        identify_data_buffer[3] = t.heads;
        identify_data_buffer[4] = t.sectors * SECTOR_SIZE; // Bytes per track (unformatted)
        identify_data_buffer[5] = SECTOR_SIZE; // Bytes per sector (unformatted)
        identify_data_buffer[6] = t.sectors;
        identify_data_buffer[7] = 0; // Reserved for vendor (bytes 14-19)
        identify_data_buffer[8] = 0; // Reserved
        identify_data_buffer[9] = 0; // Reserved

        // Words 10-19: Serial number (20 bytes ASCII)
        char serial_num[21] = "SN0123456789ABCDEFGH"; // Max 20 chars
        for(int i=0; i<10; ++i) {
            identify_data_buffer[10+i] = (serial_num[i*2] << 8) | serial_num[i*2+1];
        }

        identify_data_buffer[20] = 0; // Buffer type (obsolete) -> 0x0003 for dual port, multi-sector with read/write caching
        identify_data_buffer[21] = 512 / 2; // Buffer size in 512-byte increments (e.g. 1 for 512 byte buffer, common for old drives)
        identify_data_buffer[22] = 4; // Number of ECC bytes passed on R/W Long (usually 4)

        // Words 23-26: Firmware revision (8 bytes ASCII)
        char fw_rev[9] = "1.0     ";
        for(int i=0; i<4; ++i) {
            identify_data_buffer[23+i] = (fw_rev[i*2] << 8) | fw_rev[i*2+1];
        }

        // Words 27-46: Model number (40 bytes ASCII)
        char model_num[41] = "GENERIC IDE HARD DISK                   ";
        for(int i=0; i<20; ++i) {
            identify_data_buffer[27+i] = (model_num[i*2] << 8) | model_num[i*2+1];
        }

        identify_data_buffer[47] = 1; // Maximum number of sectors on R/W MULTIPLE (0 or 1 for no support)
                                      // Bit 8 must be 1 to indicate value is valid. So 0x0101 or just 1.
        identify_data_buffer[48] = 0; // Reserved (was DoubleWordIO possible: 0 for no, 1 for yes)
        identify_data_buffer[49] = 0x0200; // Capabilities: LBA supported (bit 9), DMA not supported (bit 8=0)
                                           // 0x0200 means LBA supported. IORDY not supported.
        identify_data_buffer[50] = 0; // Reserved
        identify_data_buffer[51] = 0; // PIO data transfer cycle timing mode (0 for mode 0,1,2)
        identify_data_buffer[52] = 0; // DMA data transfer cycle timing mode
        identify_data_buffer[53] = 0x0001; // Translation parameters are valid (words 54-58, 64-70)

        // Current CHS translation parameters (may be same as logical if no translation active)
        identify_data_buffer[54] = t.cylinders;
        identify_data_buffer[55] = t.heads;
        identify_data_buffer[56] = t.sectors;
        u32 total_sectors = (u32)t.cylinders * t.heads * t.sectors;
        identify_data_buffer[57] = total_sectors & 0xFFFF; // Current capacity in sectors (LWord)
        identify_data_buffer[58] = total_sectors >> 16;   // Current capacity in sectors (HWord)

        identify_data_buffer[59] = 0; // Multiple sector setting (lower byte has current count, bit 8 must be 1 if valid)

        // LBA total capacity
        identify_data_buffer[60] = total_sectors & 0xFFFF; // Total addressable sectors in LBA mode (LWord)
        identify_data_buffer[61] = total_sectors >> 16;   // Total addressable sectors in LBA mode (HWord)

        // Word 80: Major version number (bit 4 for ATA-1, bit 5 for ATA-2 etc.) -> 0x001E (ATA-1 to ATA-4)
        // Word 82: Commands supported (e.g. NOP, Read Buffer, Write Buffer, IDENTIFY, READ/WRITE DMA etc.)
        // Word 83: Commands supported (LBA, DMA, etc.)
        // Word 84: Features enabled (e.g. Write cache, Read lookahead)

        status_register &= ~STATUS_BUSY; // Clear BUSY
        status_register |= STATUS_DRQ;   // Set DRQ
        status_register |= STATUS_READY; // Should be ready
        error_register = 0; // Clear any previous errors
    }

    void prepare_rw_sector()
    {
        // Check if drive is ready
        if (status_register & STATUS_BUSY) {
            error_register = ERROR_ABRT; // Or some other error like drive not ready
            status_register |= STATUS_ERR;
            status_register &= ~STATUS_BUSY; // Clear busy after error
            send_irq_delay = SEND_IRQ_DELAY; // Signal error
            return;
        }

        u16 cylinder = (cylinder_high << 8) | cylinder_low;
        u8 head = drive_head & 0x0F;
        // LBA calculation based on CHS from registers
        // Note: sector_number is 1-based from register, but 0-based for calculation
        size_t lba_offset = disks.disk[current_disk()].type.get_byte_offset(cylinder, head, sector_number -1 );

        // TODO: Check if LBA is out of bounds for the disk
        if (lba_offset >= disks.disk[current_disk()].data.size() || !disks.disk[current_disk()].type.is_valid_chs(cylinder, head, sector_number)) {
            error_register = ERROR_ID; // ID Not Found (or Address Mark Not Found)
            status_register |= STATUS_ERR;
            status_register &= ~STATUS_BUSY;
            send_irq_delay = SEND_IRQ_DELAY;
            return;
        }

        current_offset = lba_offset;
        data_left = (sector_count == 0 ? 256 : sector_count) * SECTOR_SIZE; // Sector count 0 means 256 sectors

        std::cout << (is_writing ? "Write " : "Read ") << (sector_count == 0 ? 256 : u32(sector_count)) << " sectors at C:" << u32(cylinder) << "/H:" << u32(head) << "/S:" << u32(sector_number) << " (LBA offset: " << current_offset << ")" << std::endl;

        status_register |= STATUS_BUSY;  // Set BUSY before DRQ for write, or before data transfer for read
        status_register &= ~STATUS_READY;
        status_register |= STATUS_DRQ;   // Data is ready to be transferred (for write from host, for read to host)

        // For reads, DRQ means data is ready from disk. For writes, DRQ means disk is ready for data from host.
        // IRQ is usually generated *after* data transfer for a sector/block or command completion.
        // The current logic of send_irq() in read/write data port might be too soon.
        // Let's defer IRQ until command completion or explicit points.
        // For PIO, an IRQ is typically generated when DRQ is set for the first block of data.
        // For now, let prepare_rw_sector set DRQ. The actual IRQ generation will be handled at data port or command end.
        // send_irq(); // Let's not send IRQ here, but when data is actually ready or command completes.
    }

    u32 send_irq_delay{};

    u32 idx_counter{};
    void cycle()
    {
        ++idx_counter;
        if (idx_counter == 1065)
            idx_counter = 0;

        if (idx_counter & 1024)
        {
            status_register |= STATUS_IDX;
        }
        else
        {
            status_register &= ~STATUS_IDX;
        }

        if (send_irq_delay > 0)
        {
            --send_irq_delay;
            if (send_irq_delay == 0)
            {
                send_irq();
                status_register &= ~STATUS_BUSY;
                status_register |= STATUS_READY;
            }
        }
    }
};

