#pragma once

//gameblaster! :-)
//missing:
//-stereo
//-envelope
//-noise channels

struct GameBlaster
{
    i16 sound_out_l, sound_out_r{};
    struct SAA1099
    {
        i16 sound_out_l, sound_out_r{}; //current output sample

        u8 reg[0x20] = {};
        u8 current_reg = {};

        void command(u8 data)
        {
            current_reg = data&0x1F;
        }
        void control(u8 data)
        {
            reg[current_reg] = data;
        }

        u32 osc_state[6] = {};

        void cycle() //calling frequency is real chip / 256, to save on processing
        {
            u16 out_sample_l{};
            u16 out_sample_r{};
            for(u8 channel=0; channel<6; ++channel) //6 melody channels
            {
                u8 amp_l = (reg[channel]&0x0F);
                u8 amp_r = (reg[channel]>>4)&0x0F;
                u8 freq = reg[channel|0x08];
                u8 octave = (reg[0x10 + (channel>>1)]>>(channel&1?4:0))&0x0F;
                u32 divisor = (0x1FF^freq) << (9-octave);
                osc_state[channel] += 256;
                if (osc_state[channel] >= 2*divisor)
                    osc_state[channel] -= 2*divisor;
                u16 sample = (osc_state[channel]>=divisor)?0x180:0x00;

                bool enable = (reg[0x14]>>channel)&0x01;
                out_sample_l += amp_l*(enable?sample:0);
                out_sample_r += amp_r*(enable?sample:0);
            }
            //TODO: noise channels, envelope
            bool sound_enabled = reg[0x1C]&0x01; //all channels
            sound_out_l = (sound_enabled?out_sample_l:0);
            sound_out_r = (sound_enabled?out_sample_r:0);
        }
    };

    SAA1099 low, high;
    u8 latchA{}, latchB{}; //for detecting C/MS

    u8 read(u8 port) // port from 0 to F inclusive
    {
        u8 ret{};
        if (port == 0x4)
            ret = 0x7F;
        if (port == 0xA)
            ret = latchA;
        if (port == 0xB)
            ret = latchB;

        //std::cout << "C/MS read " << u32(port) << ":" << u32(ret) << std::endl;
        return ret;
    }

    void write(u8 port, u8 data) // port from 0 to F inclusive
    {
        //std::cout << "C/MS write " << u32(port) << ":" << u32(data) << std::endl;
        if (port == 0x0)
            low.control(data);
        if (port == 0x1)
            low.command(data);
        if (port == 0x2)
            high.control(data);
        if (port == 0x3)
            high.command(data);
        if (port == 0x6)
            latchA = data;
        if (port == 0x7)
            latchB = data;
    }

    void cycle() //calling frequency is real chip / 256, to save on processing
    {
        low.cycle();
        high.cycle();

        sound_out_l = (low.sound_out_l>>1) + (high.sound_out_l>>1);
        sound_out_r = (low.sound_out_r>>1) + (high.sound_out_r>>1);
    }
};
