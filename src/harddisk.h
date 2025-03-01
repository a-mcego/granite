#pragma once


struct HARDDISK
{
    CHIP8237& dma;
    CHIP8259& pic;

    HARDDISK(CHIP8237& dma_, CHIP8259& pic_) : dma(dma_), pic(pic_) {}

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
                //std::abort();
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
                        cout << "HD READ offset: " << offset << endl;
                        if (address_valid)
                        {
                            //dma.print_params(3);
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

                            //dma.print_params(3);
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
                            //std::abort();
                        }
                        //dma.print_params(3);
                        dma.transfer(3, &sector_buffer, 0);
                        r1_req = false;
                        dma_in_progress = true;
                    }
                    else if (data_in[0] == SEEK)
                    {
                        //todo: emulate seek behavior
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
                    else if (data_in[0] == READ_LONG)
                    {
                        set_current_params();
                        u32 offset = disks[current_drive].type.get_byte_offset(current_cylinder, current_head, current_sector);
                        cout << "HD READ LONG offset: " << offset << ", transfercount=" << dma.chans[3].transfer_count << endl;
                        if (address_valid)
                        {
                            //dma.print_params(3);
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
                    else if (data_in[0] == WRITE_LONG)
                    {
                        set_current_params();
                        u32 offset = disks[current_drive].type.get_byte_offset(current_cylinder, current_head, current_sector);
                        cout << "HD WRITE LONG offset: " << offset << ", transfercount=" << dma.chans[3].transfer_count << endl;
                        if (address_valid)
                        {
                            if (dma.chans[3].transfer_count != 0x1FF)
                            {
                                //cout << "----------------Transfer count: " << dma.chans[3].transfer_count << endl;
                            }

                            //dma.print_params(3);
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
                    else
                    {
                        cout << "idk command " << u32(data_in[0]) << endl;
                        //std::abort();
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


};

