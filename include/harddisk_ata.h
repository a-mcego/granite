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

    // Diagnostic error register values
    static const u8 DIAG_ERROR_NONE              = 0x01;
    static const u8 DIAG_ERROR_CONTROLLER        = 0x02;
    static const u8 DIAG_ERROR_SECTOR_BUFFER     = 0x03;
    static const u8 DIAG_ERROR_ECC_DEVICE        = 0x04;
    static const u8 DIAG_ERROR_CONTROL_PROCESSOR = 0x05;

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
        if (startprinting)
            std::cout << globalsettings.current_IP << " ATA write " << u32(port) << ":" << u32(data) << std::endl;
        switch(port)
        {
            case 0: // Data Register
                data_register = data;
                if (data_left > 0)
                {
                    auto& disk_data = disks.disk[current_disk()].data;
                    if (current_offset < disk_data.size())
                        disk_data[current_offset] = data&0xFF, disk_data[current_offset+1] = data>>8;
                    current_offset += 2;
                    data_left -= 2;
                    if (data_left%SECTOR_SIZE == 0)
                    {
                        send_irq();
                    }
                    if (data_left == 0)
                    {
                        status_register &= ~STATUS_DRQ;
                        status_register &= ~STATUS_BUSY;
                        status_register |= STATUS_READY;
                    }
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
                if (!(data&0x04) && (fixed_disk_register&0x04))
                {
                    //reset puts us into diagnostic mode
                    error_register = DIAG_ERROR_NONE;
                }
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
                if (data_left > 0)
                {
                    auto& disk_data = disks.disk[current_disk()].data;
                    if (current_offset < disk_data.size())
                        data_register = disk_data[current_offset] + (disk_data[current_offset+1]<<8);
                    current_offset += 2;
                    data_left -= 2;
                    if (data_left%SECTOR_SIZE == 0 && data_left != 0)
                    {
                        send_irq();
                    }
                    if (data_left == 0)
                    {
                        status_register &= ~STATUS_DRQ;
                        status_register &= ~STATUS_BUSY;
                        status_register |= STATUS_READY;
                    }
                }
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
        if (startprinting)
            std::cout << std::hex << globalsettings.current_IP << " ATA read " << u32(port) << ":" << u32(data) << std::endl;
        return data;
    }

    void execute_command(u8 cmd)
    {
        //std::cout << std::hex << globalsettings.current_IP << " ATA command: " << u32(cmd) <<  std::endl;

        if (false);
        else if ((cmd&0xF0) == 0x10) //restore
        {
            send_irq_delay = SEND_IRQ_DELAY;
            status_register |= STATUS_BUSY;
            status_register &= ~STATUS_READY;
        }
        else if ((cmd&0xF0) == 0x70) //seek
        {
            send_irq_delay = SEND_IRQ_DELAY;
            status_register |= STATUS_BUSY;
            status_register &= ~STATUS_READY;
        }
        else if (cmd == 0x50) //identify
        {
            u16 cylinder = (cylinder_high << 8) | cylinder_low;
            u16 head = drive_head & 0x0F;

            if (!disks.disk[current_disk()].type.size_is(cylinder,head,sector_count))
            {
                std::cout << "Disk size mismatch between bios and actual disk." << std::endl;
                std::cout << "Actual: ";
                std::cout << disks.disk[current_disk()].type.cylinders << "/";
                std::cout << disks.disk[current_disk()].type.heads << "/";
                std::cout << disks.disk[current_disk()].type.sectors << std::endl;

                std::cout << "Bios:  ";
                std::cout << cylinder << "/";
                std::cout << head << "/";
                std::cout << sector_count << std::endl;

                std::abort();
            }
        }
        else if ((cmd&0xFE) == 0x20) //read, todo: long read
        {
            prepare_rw_sector();
            error_register = 0;
            status_register |= STATUS_BUSY;
            status_register &= ~STATUS_READY;
        }
        else if ((cmd&0xFE) == 0x30) //write, todo: long write
        {
            prepare_rw_sector();
            error_register = 0;
            status_register |= STATUS_BUSY;
            status_register &= ~STATUS_READY;
        }
        else if ((cmd&0xFE) == 0x40) //read verify
        {
            send_irq_delay = SEND_IRQ_DELAY;
            status_register |= STATUS_BUSY;
        }
        else if (cmd==0x90) //diagnose
        {
            //diagnose
            status_register = STATUS_READY | STATUS_DSC;
            error_register = DIAG_ERROR_NONE;
            send_irq_delay = SEND_IRQ_DELAY;
        }
        else if (cmd==0x91) //set parameters
        {
            send_irq_delay = SEND_IRQ_DELAY;
            status_register |= STATUS_BUSY;
        }
        else
        {
            std::cout << "Unknown ATA command " << std::hex << u32(cmd) << std::endl;
            //std::abort();
            error_register = 0x04; // Abort
            status_register = STATUS_READY | STATUS_ERR;
        }
    }

    void prepare_rw_sector()
    {
        u16 cylinder = (cylinder_high << 8) | cylinder_low;
        u8 head = drive_head & 0x0F;
        size_t lba = disks.disk[current_disk()].type.get_byte_offset(cylinder,head,sector_number-1);
        current_offset = lba;
        data_left = (u32(u8(sector_count-1))+1) * SECTOR_SIZE;

        std::cout << ((command_register&0x10)?"Write ":"Read ") << u32(sector_count) << " sectors at " << u32(cylinder) << "/" << u32(head) << "/" << u32(sector_number) << std::endl;
        send_irq();
        status_register |= STATUS_DRQ;
        status_register |= STATUS_BUSY;
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

