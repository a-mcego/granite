#pragma once

struct YM3812
{
    Opl2 opl2;

    u8 current_register{};
    u8 status{};

    u8 timer1{};
    u8 timer2{};

    u8 counter{};

    i16 sample{};
    i16 previous_sample{};

    void write(u8 port, u8 data) //port from 0 to 1! inclusive.
    {
        //cout << "YM3812 WRITE! " << u32(port) << ":" << u32(data) << endl;
        if (port == 0x00)
        {
            current_register = data;
        }
        else if (port == 0x01)
        {
            if (current_register == 0x02)
            {
                timer1 = data;
            }
            else if (current_register == 0x03)
            {
                timer2 = data;
            }
            else if (current_register == 0x04) //reset IRQ
            {
                if (data&0x80)
                {
                    status = 0;
                }
            }
            else
            {
                opl2.write(current_register,data);
            }
        }
    }

    u8 read(u8 port) //port from 0 to 1! inclusive.
    {
        u8 ret{};
        if (port == 0)
        {
            ret = status;
        }
        //cout << "YM3812 READ!" << u32(port) << ":" << u32(ret) << endl;
        return ret;
    }

    void cycle_timers() //at circa 49715 hz
    {
        ++counter;
        if(timer1 != 0  && (counter&0x03) == 0) //fast timer
        {
            ++timer1;
            if (timer1==0)
            {
                status |= 0xC0;
                cout << "YM3812: TIMER 1 expired!" << endl;
            }
        }
        if(timer2 != 0  && (counter&0x0F) == 0) //slow timer
        {
            ++timer2;
            if (timer2==0)
            {
                status |= 0xA0;
                cout << "YM3812: TIMER 2 expired!" << endl;
            }
        }
    }
    void cycle() //at circa 49715 hz in real time!
    {
        previous_sample = sample;
        sample = opl2.update();
    }
} ym3812;

