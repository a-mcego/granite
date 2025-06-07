#pragma once


struct DISKETTECONTROLLER
{
    CHIP8237& dma;
    CHIP8259& pic;

    DISKETTECONTROLLER(CHIP8237& dma_, CHIP8259& pic_) : dma(dma_), pic(pic_) {}
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

    static const u16 RESET_CYCLES = 4;//256
    static const u16 SEEK_ONE_TRACK = 144*2;//(12*1024);
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
            u32 rpm{300};
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
                rpm = 300;
                //data.assign(184320,0);
            }

            DISKETTE(std::string filename)
            {
                FILE* filu = fopen(filename.c_str(), "rb");
                if (filu == NULL)
                {
                    std::cout << "file \"" << filename << "\" not found." << std::endl;
                    std::abort();
                }
                fseek(filu,0,SEEK_END);
                u32 size = ftell(filu);
                fseek(filu,0,SEEK_SET);

                rpm = 300;
                if (size == 163840) //160k disk =)
                {
                    cylinders = 40;
                    heads = 1;
                    sectors = 8;
                }
                else if (size == 184320) //180k disk =P
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
                    rpm = 360;
                }
                else if (size == 737280) //720k disk :D
                {
                    cylinders = 80;
                    heads = 2;
                    sectors = 9;
                }
                else if (size == 1474560) //1.44M disk :-D
                {
                    cylinders = 80;
                    heads = 2;
                    sectors = 18;
                }
                else if (size == 2949120) //2.88M disk :--D
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
        u8 target_cylinder{};
        bool motor{};
    } drives[4];

    u8 selected_drive{};
    u8 main_status{0x80}; // RQM DIO NDM CB D3B D2B D1B D0B
    u8 st0{};
    u8 st1{};
    u8 st2{};

    //0 = 500kbps
    //1 = 300kbps
    //2 = 250kbps
    //3 = 1000kbps
    u8 datarate{};

    u8 registers[8] = {}; // not all registers are used, but we'll do it this way to be simple

    u8 is_selected_and_on(u8 drive)
    {
        bool on = registers[2]&(0x10<<drive);
        bool selected = (registers[2]&0x3) == drive;
        return on && selected;
    }

    u8 read(u8 port) //port from 0 to 7! inclusive
    {
        if (FLOPPY_DEBUG)
            cout << "/-------------------------------------------\\" << endl;
        if (FLOPPY_DEBUG)
            cout << "FLOPPY CONTROLLER READ: " << u32(port) << endl;
        //  PrintCSIP();
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
            //std::abort();
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
        if (FLOPPY_DEBUG)
            cout << "/===========================================\\" << endl;
        if (FLOPPY_DEBUG)
            cout << "FLOPPY CONTROLLER WRITE: " << u32(port) << " data=" << u32(data) << endl;
        //PrintCSIP();
        if (false);
        else if (port == 4 || port == 7) //datarate select, configuration control
        {
            datarate = data;
        }
        else if (port == 5) // DATA_FIFO
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
                        //u32 end_of_track = out_buffer[5]; //number of sectors in a track
                        if (out_buffer[7] != 0xFF)
                        {
                            std::cout << "weird out_buffer[7] = " << u32(out_buffer[7]) << ", should be 0xFF" << endl;
                            std::abort();
                        }
                        u32 byte_offset = drives[drive].diskette.get_byte_offset(cylinder,head_A,sector);
                        /*std::cout << (current_command == 0x05?"WRITE":"READ") << ":";
                        cout << " drive=" << drive;
                        cout << " head=" << head_A;
                        cout << " cylinder=" << cylinder;
                        cout << " sector=" << sector;
                        cout << " end_of_track=" << end_of_track;
                        cout << " -> byte offset=" << byte_offset << endl;*/

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
                        drives[data&0x03].target_cylinder = 0;
                        out_buffer.clear();
                        //pic.request_interrupt(6);
                        main_status &= ~0x50; //no output bytes
                        st0 = selected_drive;
                        current_command = 0;
                        interrupt_timer = SEEK_ONE_TRACK;
                    }
                    else if (current_command == 0x0F) //seek
                    {
                        //TODO: verify that this drive is selected
                        u8 drive_number = out_buffer[0]&0x03;
                        if (drives[drive_number].motor)
                        {
                            u8 target = out_buffer[1];
                            //std::cout << "seek target: " << u32(target) << "/" << u32(drives[drive_number].diskette.cylinders) << std::endl;
                            if (target >= drives[drive_number].diskette.cylinders && drives[drive_number].diskette.cylinders != 0)
                                target = drives[drive_number].diskette.cylinders-1;
                            drives[drive_number].target_cylinder = target;

                        }
                        else
                        {
                            cout << "Tried to seek on drive #" << u32(drive_number) << " but motor is not on." << endl;
                        }
                        st0 = selected_drive;
                        main_status |= (1<<st0);
                        interrupt_timer = SEEK_ONE_TRACK;
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
            if (!(registers[port]&0x04) && (data&0x04))
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
            std::cout << "Write: Unsupported floppy port " << u32(port) << " with data: " << u32(data) << endl;
            //registers[port] = data;
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
                u8& cur = drives[selected_drive].current_cylinder;
                u8& target = drives[selected_drive].target_cylinder;
                if (cur == target)
                {
                    st0 &= 0xF0; //clear the "seek" bits
                    pic.request_interrupt(6);
                }
                else
                {
                    std::cout << "seek " << u32(cur) << "->";
                    cur += (cur<target)?1:-1;
                    sampleplayer.play(0);
                    std::cout << u32(cur) << std::endl;
                    interrupt_timer = SEEK_ONE_TRACK;
                }
            }
        }
        if (dma.chans[2].pending)
        {
            dma.chans[2].cycle_transfer();
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
            //cout << "DMA COMPLETE lol. interrupt 6. did " << dma.chans[2].transfer_count << " bytes aka " << (dma.chans[2].transfer_count)/512+1 << " sectors" << endl;
            //dma.print_params(2);
        }
    }
};

