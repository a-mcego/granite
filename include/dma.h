#pragma once

#include "dmapage.h"

struct CHIP8237 //DMA
{
    u8 type{}; //0 for primary, 1 for secondary
    CHIPLS612N& dmapage;
    MemoryManager286& mem;
    struct Channel
    {
        CHIPLS612N& dmapage;
        MemoryManager286& mem;
        u16 num{}; //which channel this is


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


        u8& page()
        {
            static const u16 addrs[8] = {0x7, 0x3, 0x1, 0x2, 0xF, 0xB, 0x9, 0xA};

            return dmapage.pages[addrs[num&0x07]];
        }
        void initiate_transfer()
        {
            if (!pending)
            {
                curr_addr = start_addr;
                curr_count = transfer_count;
                curr_vector_offset = device_vector_offset;
                pending = true;
                //is_complete = false;
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
                mem.w8((page()<<16U)+curr_addr, (*device_vector)[curr_vector_offset]);
            }
            else if (transfer_direction == DIR_FROM_MEMORY)
            {
                (*device_vector)[curr_vector_offset] = mem.r8((page()<<16U)+curr_addr);
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
            if (down)
                --curr_addr;
            else
                ++curr_addr;

            bool cross_seg_boundary = (down && curr_addr==0xFFFF) || (!down && curr_addr==0x0000);

            if (cross_seg_boundary)
            {
                //cout << "DMA " << num << ": seg boundary crossed. :(" << endl;
            }

            ++curr_vector_offset;
            if (curr_count == 0 || cross_seg_boundary)
            {
                pending = automatic;
                //is_complete = !automatic;
                is_complete = true;
                curr_addr = start_addr;
                curr_count = transfer_count;
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
    } chans[4] = { {dmapage, mem}, {dmapage, mem}, {dmapage, mem}, {dmapage, mem}};

    CHIP8237(u8 type_, CHIPLS612N& dmapage_, MemoryManager286& mem_):type(type_), dmapage(dmapage_), mem(mem_)
    {
        chans[0].num = 0 + (type?4:0);
        chans[1].num = 1 + (type?4:0);
        chans[2].num = 2 + (type?4:0);
        chans[3].num = 3 + (type?4:0);
    }

    void print_params(u8 channel)
    {
        Channel& c = chans[channel];
        cout << std::hex;
        cout << ">DMA port " << u32(channel) << "!< ";
        cout << "p+addr=" << c.page()*65536+c.start_addr << " ";
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

                chans[i].is_complete = false;
            }
        }
        else
        {
            std::cout << "Unsupported DMA read port " << u32(port) << endl;
            //std::abort();
            result = 0;
        }
        return result;
    }

    void write(u8 port, u8 data) //port from 0 to 15! inclusive
    {
        if (startprinting)
            cout << "DMA WRITE " << u32(port) << " <- " << u32(data) << endl;
        if (port >= 0x08 && port <= 0x0F)
        {
            if (false);
            else if (port == 0x08) //we're only interested in bit 2 as per osdev's article. hooray indeed
            {
                enabled = (data&0x04);
            }
            else if (port == 0x09)
            {
                //osdev says
                //"Request Registers 0x09 and 0xD2 (Write)"
                //"Used for memory to memory transfers and setting up priority rotation -- absolutely useless."
                //interesting.
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
                std::cout << "Unsupported DMA write port " << u32(port) << endl;
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
            cout << "p=" << u32(c.page()) << " ";
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

    /*void cycle()
    {
        if (chans[3].pending)
            chans[3].cycle_transfer();
        for(u64 i=0; i<4; ++i)
        {
            Channel& c = chans[i];
            if (c.pending && i != 1 && i!=2)
            {
                c.cycle_transfer();
            }
        }
    }*/
};

