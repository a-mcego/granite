#pragma once

#include <queue>
#include "dma.h"
#include "interrupt.h"

//soundblaster! :-)
//trying to get as many versions as possible, from 1.0 to awe64, all ISA cards
//currently sb pro, hardcoded to I/O 220, DMA 1, IRQ 7. remember to SET BLASTER!

struct SoundBlaster
{
    CHIP8237& dma;
    CHIP8259& pic;

    SoundBlaster(CHIP8237& dma_, CHIP8259& pic_) : dma(dma_), pic(pic_)
    {
        buffer.assign(256,0);
    }

    i16 sound_out_l{}, sound_out_r{};
    static constexpr u8 VERSION_MAJOR = 3;
    static constexpr u8 VERSION_MINOR = 1;

    enum PORTS
    {
        MIXER=0x04,
        MIXER_DATA=0x05,
        RESET=0x06,
        READ=0x0A,
        WRITE=0x0C,
        STATUS=0x0E,
        INT_ACK=0x0F //sb16+ only
    };
    enum COMMANDS
    {
        OUTPUT_8_SINGLE=0x10, //one more byte: the single data.
        OUTPUT_8_ONEBLOCK=0x14, //two bytes: low byte, high byte
        OUTPUT_8_AUTOINIT=0x1C,
        SET_TIME_CONSTANT=0x40, //requires one more write: the time constant
        SET_OUTPUT_SAMPLERATE=0x41, //two bytes, high byte and low byte DSP 4+
        SET_INPUT_SAMPLERATE=0x42, //two bytes, high byte and low byte DSP 4+
        SET_BLOCK_TRANSFER_SIZE=0x48, //two bytes. low byte and high byte.
        SET_16BIT_TRANSFER=0xB0, //not implemented yet
        SET_8BIT_TRANSFER=0xC0, //three bytes. mode, length low, length high
        PAUSE = 0xD0,
        SPEAKER_ON=0xD1,
        SPEAKER_OFF=0xD3,
        UNPAUSE = 0xD4,
        OUTPUT_8_AUTOINIT_STOP=0xDA,
        VERSION=0xE1, //returns 2 bytes: major version, minor version
    };

    struct Mixer
    {
        u8 regs[0x100] = {};
        u8 current_reg{};
    } mixer;

    bool play{};
    bool stop_after_current{};
    bool speaker{};
    bool reset{};
    u8 writestatus{};
    std::queue<u8> readdata;

    u8 bytes_left_to_write{};
    u8 current_command{};
    u16 block_transfer_size{};
    u16 current_block_transfer{};
    u8 io_command{};
    u8 io_mode{};

    u16 clocks_per_cycle{};
    u16 current_clock{};


    u8 time_constant{};

    u16 output_samplerate{};
    u16 input_samplerate{};

    u8 read(u8 port) // port from 0 to F inclusive
    {
        u8 data = 0;
        if (port == STATUS)
        {
            data = readdata.empty()?0:0x80;
        }
        else if (port == READ)
        {
            if (!readdata.empty())
            {
                data = readdata.front();
                readdata.pop();
            }
        }
        else if (port == MIXER_DATA)
        {
            data = mixer.regs[mixer.current_reg];
        }
        //std::cout << "SB read " << u32(port) << ":" << u32(data) << std::endl;
        return data;
    }

    void write(u8 port, u8 data) // port from 0 to F inclusive
    {
        //std::cout << "SB write " << u32(port) << ":" << u32(data) << std::endl;
        if (port == RESET)
        {
            if (reset && !data)
            {
                readdata.push(0xAA);
            }
            reset = bool(data);
        }
        else if (port == MIXER)
        {
            mixer.current_reg = data;
        }
        else if (port == MIXER_DATA)
        {
            mixer.regs[mixer.current_reg] = data;
        }
        else if (port == WRITE)
        {
            if (bytes_left_to_write)
            {
                --bytes_left_to_write;
                if (false);
                else if (current_command == SET_TIME_CONSTANT)
                {
                    //for example: 0xD2 is 22050 samples per second
                    //it's the high byte of: 65536 - (256'000'000 / (samplerate*channels))
                    time_constant = data;
                    clocks_per_cycle = 315*(256-time_constant)/22;
                    current_clock = 0;
                    std::cout << "Time constant set to " << u32(data) << std::endl;
                }
                else if (current_command == SET_OUTPUT_SAMPLERATE)
                {
                    if (bytes_left_to_write == 1)
                        output_samplerate = (data<<8);
                    else
                    {
                        output_samplerate |= data;
                        if (output_samplerate != 0)
                            clocks_per_cycle = 14318180/output_samplerate;
                        current_clock = 0;
                        std::cout << "Output sample rate set to " << u32(output_samplerate) << std::endl;
                    }
                }
                else if (current_command == SET_INPUT_SAMPLERATE) //input not used yet!
                {
                    if (bytes_left_to_write == 1)
                        input_samplerate = (data<<8);
                    else
                    {
                        input_samplerate |= data;
                        //TODO:
                        //if (input_samplerate != 0)
                        //    clocks_per_cycle = 14318180/output_samplerate;
                        //current_clock = 0;
                        std::cout << "Input sample rate set to " << u32(output_samplerate) << std::endl;
                    }
                }
                else if ((current_command&0xF0) == SET_8BIT_TRANSFER)
                {
                    if (false);
                    else if (bytes_left_to_write == 2)
                    {
                        io_command = current_command;
                        io_mode = data;
                    }
                    else if (bytes_left_to_write == 1)
                    {
                        block_transfer_size = (block_transfer_size&0xFF00)|data;
                    }
                    else
                    {
                        block_transfer_size = (block_transfer_size&0x00FF)|(data<<8);
                        current_block_transfer = block_transfer_size;
                        std::cout << "SB: " << u32(current_command) << " " << std::dec << block_transfer_size << std::endl;
                        play=true;
                    }
                }
                else if (current_command == OUTPUT_8_ONEBLOCK)
                {
                    if (bytes_left_to_write == 1)
                        block_transfer_size = (block_transfer_size&0xFF00)|data;
                    else
                    {
                        block_transfer_size = (block_transfer_size&0x00FF)|(data<<8);
                        current_block_transfer = block_transfer_size;
                        std::cout << "SB single-block size: " << std::dec << block_transfer_size << std::endl;
                        io_command = 0xC0;
                        io_mode = 0x00;
                        play=true;
                    }
                }
                else if (current_command == SET_BLOCK_TRANSFER_SIZE)
                {
                    if (bytes_left_to_write == 1)
                        block_transfer_size = (block_transfer_size&0xFF00)|data;
                    else
                    {
                        block_transfer_size = (block_transfer_size&0x00FF)|(data<<8);
                        current_block_transfer = block_transfer_size;
                        io_command = 0xC0;
                        io_mode = 0x00;
                        std::cout << "SB block transfer size: " << std::dec << block_transfer_size << std::endl;
                    }
                }
                else if (current_command == OUTPUT_8_SINGLE)
                {
                    sound_out_l = i16(i8(data^0x80))<<7;
                    sound_out_r = sound_out_l;
                }
            }
            else
            {
                current_command = data;
                if (data == VERSION)
                {
                    readdata.push(VERSION_MAJOR);
                    readdata.push(VERSION_MINOR);
                }
                else if (data == SET_TIME_CONSTANT)
                {
                    bytes_left_to_write=1;
                }
                else if (data == SET_BLOCK_TRANSFER_SIZE || data == SET_OUTPUT_SAMPLERATE || data == SET_INPUT_SAMPLERATE)
                    bytes_left_to_write=2;
                else if (data == SPEAKER_ON)
                    speaker = true;
                else if (data == SPEAKER_OFF)
                    speaker = false;
                else if (data == OUTPUT_8_SINGLE)
                {
                    bytes_left_to_write=1;
                }
                else if (data == OUTPUT_8_ONEBLOCK)
                {
                    bytes_left_to_write=2;
                }
                else if ((data&0xF0) == SET_8BIT_TRANSFER)
                {
                    bytes_left_to_write=3;
                }
                else if (data == OUTPUT_8_AUTOINIT)
                {
                    dma.chans[1].curr_addr = dma.chans[1].start_addr;
                    dma.chans[1].curr_count = dma.chans[1].transfer_count;
                    dma.chans[1].pending = true;
                    play=true;
                }
                else if (data == UNPAUSE)
                    play = true;
                else if (data == PAUSE)
                    play = false;
                else if (data == OUTPUT_8_AUTOINIT_STOP)
                    stop_after_current=play;
                else
                {
                    std::cout << "unknown SB command " << std::hex << u32(data) << std::endl;
                    std::abort();
                }
            }
        }
    }


    vector<u8> buffer;

    void cycle()
    {
        if (clocks_per_cycle == 0)
            return;
        ++current_clock;
        if (current_clock >= clocks_per_cycle)
        {
            current_clock -= clocks_per_cycle;
            if (play)
            {
                dma.chans[1].device_vector = &buffer;
                dma.chans[1].curr_vector_offset = 0;
                dma.chans[1].cycle_transfer();

                sound_out_l = i16(i8(buffer[0]^0x80))<<7;
                sound_out_r = sound_out_l;

                [[maybe_unused]] bool did_interrupt{}; //TODO
                if (current_block_transfer == 0)
                {
                    dma.chans[1].is_complete_and_reset();
                    if (stop_after_current)
                    {
                        play = false;
                        stop_after_current = false;
                    }
                    else
                    {
                        current_block_transfer = block_transfer_size;
                    }
                    pic.request_interrupt(7);
                    did_interrupt = true;
                }
                else
                {
                    --current_block_transfer;
                }
            }
        }

    }
};
