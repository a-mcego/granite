#pragma once

#include "interrupt.h"
#include "beeper.h"

struct CHIP8253 //PIT
{
    CHIP8259& pic;
    BEEPER& beeper;
    CHIP8253(CHIP8259& pic_, BEEPER& beeper_):pic(pic_), beeper(beeper_) {}


    static constexpr const u32 N_CHANNELS = 4;

    struct Channel
    {
        bool write_wait_for_second_byte{};
        u8 operating_mode{}; //0-5 inclusive
        u8 access_mode{}; //0-3 inclusive
        u16 reload{};
        u16 reload_loader{};
        u16 current{};
        bool stopped{};
        u8 output{};

        void Print()
        {
            #define paska(x) cout << #x ": " << u32(x) << endl;
            paska(write_wait_for_second_byte);
            paska(operating_mode);
            paska(access_mode);
            paska(reload);
            paska(current);
            #undef paska
        }

        /*u8 get_current()
        {
            if (access_mode == 1)
                return current&0xFF;
            if (access_mode == 2)
                return current>>8;
            cout << "Tried to get current but mode is " << u32(access_mode) << endl;
            std::abort();
        }*/

        u8 normal_data_state{};

        u8 latch_data_state{}; //0=no data, 1=has data, 2=access mode III, upper bytes
        u16 latch_data{};

    } channels[N_CHANNELS];

    u8 read(u8 port) //port from 0 to 3! inclusive
    {
        //std::cout << std::hex << "PIT READ: " << u32(port) << std::endl;
        if (port >= 3) //can't read port 3
        {
            return 0;
        }
        //TODO: do better
        Channel& c = channels[port];

        if (c.latch_data_state > 0)
        {
            if (c.access_mode == 1)
            {
                c.latch_data_state = 0;
                return c.latch_data;
            }
            else if (c.access_mode == 2)
            {
                c.latch_data_state = 0;
                return c.latch_data>>8;
            }
            else if (c.access_mode == 3)
            {
                if (c.latch_data_state == 1)
                {
                    c.latch_data_state = 2;
                    return c.latch_data;
                }
                else if (c.latch_data_state == 2)
                {
                    c.latch_data_state = 0;
                    return c.latch_data>>8;
                }
            }

        }


        if (c.access_mode == 1)
        {
            return c.current;
        }
        else if (c.access_mode == 2)
        {
            return c.current>>8;
        }
        else if (c.access_mode == 3)
        {
            if (c.normal_data_state == 0)
            {
                c.normal_data_state = 1;
                return c.current;
            }
            else if (c.normal_data_state == 1)
            {
                c.normal_data_state = 0;
                return c.current>>8;
            }
        }

        if (c.access_mode == 0)
        {
            return 0;
        }

        cout << "PIT WTF. reading port: " << u32(port) << endl;
        cout << u32(c.access_mode) << endl;
        std::abort();
    }

    void write(u8 port, u8 data) //port from 0 to 3! inclusive.
    {
        //std::cout << std::hex << "PIT WRITE: " << u32(port) << ":" << u32(data) << std::endl;
        if (port == 3)
        {
            bool is_bcd = (data&0x01);
            if (is_bcd)
            {
                cout << "PIT doesn't support BCD mode yet!" << endl;
                std::abort();
            }
            u8 channel_n = (data>>6);
            if (channel_n == 3 && globalsettings.machine != GlobalSettings::MACHINE_AT)
            {
                cout << "Channel 3 non-existent on PIT! (trying to run AT code? this is an PC emulator.)" << endl;
                std::abort();
            }
            Channel& c = channels[channel_n];
            if (((data>>4)&0x03) == 0)
            {
                c.latch_data_state = 1;
                c.latch_data = c.current;
                return;
            }
            c.access_mode = ((data>>4)&0x03);

            c.operating_mode = ((data>>1)&0x07);
            c.operating_mode = (c.operating_mode>=6?c.operating_mode^4:c.operating_mode);
            c.write_wait_for_second_byte = false;
            c.latch_data_state = 0;
            c.normal_data_state = 0;

            if (c.access_mode == 0)
            {
                c.output = false;
            }

            //if constexpr (DEBUG_LEVEL > 0)
            {
                //cout << "-----PIT Channel #" << u32(channel_n) << ":" << endl;
                //c.Print();
            }
        }
        else
        {
            Channel& c = channels[port];

            if (c.access_mode == 1) //lobyte only
            {
                c.reload = data;
                c.stopped = false;
                if constexpr (DEBUG_LEVEL > 0)
                {
                    cout << "port " << u32(port) << " ACCESS MODE " << u32(c.access_mode) << ": new data " << c.reload << endl;                c.write_wait_for_second_byte = false;
                }
            }
            else if (c.access_mode == 2) //hibyte only
            {
                c.reload = (data<<8);
                c.stopped = false;
                if constexpr (DEBUG_LEVEL > 0)
                    cout << "port " << u32(port) << " ACCESS MODE " << u32(c.access_mode) << ": new data " << c.reload << endl;
            }
            else if (c.access_mode == 3) //lo, unless latch is, then hi
            {
                if (c.write_wait_for_second_byte)
                {
                    c.reload_loader = (c.reload_loader| (data<<8));
                    c.stopped = false;
                    c.reload = c.reload_loader;
                }
                else
                {
                    c.reload_loader = data;
                    c.stopped = false;
                }
                if constexpr (DEBUG_LEVEL > 0)
                    cout << "port " << u32(port) << " ACCESS MODE " << u32(c.access_mode) << ": new data " << c.reload << " & " << c.current << endl;
                c.write_wait_for_second_byte = !c.write_wait_for_second_byte;
            }
            if constexpr (DEBUG_LEVEL > 0)
                cout << "--- operating mode " << u32(c.operating_mode) << endl;

            if (c.operating_mode == 0 || c.operating_mode == 1 || c.operating_mode == 2)
            {
                c.current = c.reload;
                c.output = false;
                c.stopped = false;
            }
        }
    }

    u64 int0_count{};
    void cycle()
    {
        for(u32 i=0; i<3; ++i)
        {
            Channel& c = channels[i];

            if (i==2 && !(global_port0x61&0x01))
            {
                continue;
            }

            if (c.operating_mode == 0)
            {
                c.current -= 1;
                if (c.current == 0)
                {
                    if (c.output == false && i==0)
                    {
                        if constexpr (DEBUG_LEVEL > 0)
                        {
                            cout << std::dec;
                            cout << PRETTY_FUNCTION << ":" << __LINE__ << ": " << u32(c.reload) << " "  << u32(c.current) << " " << u32(c.operating_mode) << endl;
                            cout << std::hex;
                        }
                        ++int0_count;
                        pic.request_interrupt(0);
                    }
                    c.output = true;
                }
            }
            else if (c.operating_mode == 1)
            {
                if (c.current == 0)
                {
                    c.output = true;
                }
                else
                {
                    c.current -= 1;
                }
            }
            else if (c.operating_mode == 2)
            {
                //cout << "PIT #" << i << " opmode 2, curr " << u32(c.current) << endl;
                c.current -= 1;
                if (c.current <= 1)
                {
                    c.output = 0;
                    c.current = c.reload;
                    if (i==0)
                    {
                        if constexpr (DEBUG_LEVEL > 0)
                        {
                            cout << std::dec;
                            cout << PRETTY_FUNCTION << ":" << __LINE__ << ": " << u32(c.reload) << " "  << u32(c.current) << " " << u32(c.operating_mode) << endl;
                            cout << std::hex;
                        }
                        ++int0_count;
                        pic.request_interrupt(0);
                    }
                    else if (i==1 && globalsettings.machine == globalsettings.MACHINE_XT)
                    {
                        //cout << "INITIATE TRANSFER XT 1" << endl;
                        //dma.chans[0].initiate_transfer();
                        //dma.chans[0].start_addr += 1;
                    }
                }
                else if (c.current == u32(c.reload-1))
                {
                    c.output = 1;
                }
                //cout << "PIT #" << i << " opmode 2, new  " << u32(c.current) << endl;
            }
            else if (c.operating_mode == 3)
            {
                if (c.current&1)
                {
                    c.current -= c.output?1:3;
                }
                else
                {
                    c.current -= 2;
                }
                if (c.current < 2)
                {
                    c.output = !c.output;
                    c.current = c.reload;
                    if (i==0)
                    {
                        if constexpr (DEBUG_LEVEL > 0)
                        {
                            cout << std::dec;
                            cout << PRETTY_FUNCTION << ":" << __LINE__ << ": " << u32(c.reload) << " "  << u32(c.current) << " " << u32(c.operating_mode) << endl;
                            cout << std::hex;
                        }
                        if (c.output)
                        {
                            ++int0_count;
                            pic.request_interrupt(0);
                        }
                    }
                    else if (i==1 && globalsettings.machine == globalsettings.MACHINE_XT)
                    {
                        cout << "INITIATE TRANSFER XT 2" << endl;
                        //dma.chans[0].initiate_transfer();
                    }
                }
            }
            else
            {
                cout << "Unknown PIT operating mode " << u32(c.operating_mode) << endl;
                std::abort();
            }
            //TODO
        }
        beeper.set_output_from_pit(!channels[2].output);
    }
};
