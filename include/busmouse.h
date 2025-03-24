#pragma once

struct BusMouse
{
    CHIP8259& pic;
    BusMouse(CHIP8259& pic_):pic(pic_){}
    int irq{3}; //2, 3, 4, or 5 on the original board

    void update_pos(i16 dataX, i16 dataY)
    {
        if (inited)
        {
            dX += dataX;
            dY += dataY;

            do_irq();
        }
    }

    void update_button(u8 button_id, bool state) //comes from GLFW
    {
        if (inited)
        {
            u8 on_mask = (0b001<<(state?0:3));
            u8 off_mask = 0b1001;
            on_mask = (on_mask<<button_id);
            off_mask = (off_mask<<button_id);
            std::cout << "buttons: " << u16(buttons);
            buttons = (buttons&~off_mask) | (state?on_mask:0);
            std::cout << " -> " << u32(buttons) << std::endl;
            do_irq();
        }
    }

    //state here
    i16 dX{}, dY{};
    u8 buttons{}; //lowest 3 bits
    u8 status{};
    bool interrupt_enabled{};
    bool inited{};

    enum COMMAND_MODE
    {
        READ_BUTTON_STATE=0,
        READ_X=1,
        READ_Y=2,
        CONTROL=7,
    };
    COMMAND_MODE command_mode{CONTROL};

    i8 clamp_i8(i16 val)
    {
        if (val >= 0x3F)
            return 0x3F;
        if (val < -0x40)
            return -0x40;
        return val;
    }

    u8 signature_byte{};

    void do_irq()
    {
        //if (!(status&0x08))
        if (inited)
            pic.request_interrupt(irq);
    }

    u8 read(u8 port) // port from 0 to 3 inclusive
    {
        u8 data{};
        if (false);
        else if (port == 0) // control/interrupt
        {
            data = status;
        }
        else if (port == 1) // data
        {
            if (false);
            else if (command_mode == READ_X)
            {
                i8 add = clamp_i8(dX);
                dX -= add;
                data = add;
            }
            else if (command_mode == READ_Y)
            {
                i8 add = clamp_i8(dY);
                dY -= add;
                data = add;
            }
            else if (command_mode == READ_BUTTON_STATE)
            {
                data = 0xF0|buttons;
            }
            else if (command_mode == CONTROL)
            {
                data = 0xF0|buttons;
            }
            else
            {
                std::cout << "weird mouse command: " << std::hex << u16(command_mode) << std::endl;
                std::abort();
            }
        }
        else if (port == 2) // signature
        {
            if (signature_byte)
                data = 0x02;
            else
                data = 0xDE;
            signature_byte = 1-signature_byte;
            inited = true;
        }
        else if (port == 3) // configuration
        {

        }
        //std::cout << "mouse read:  " << std::hex << u16(port) << ":" << u16(val) << std::endl;
        return data;
    }
    void write(u8 port, u8 data) // port from 0 to 3 inclusive
    {
        //std::cout << "mouse write: " << std::hex << u16(port) << ":" << u16(data) << std::endl;
        if (false);
        else if (port == 0) // control/interrupt
        {
            status = data;
            if (false);
            /*else if (data == 0x11)
                interrupt_enabled = true;
            else if (data == 0x10)
                interrupt_enabled = false;*/
            else if (data < 0x08)
                command_mode = COMMAND_MODE(data&0x07);
            else if (data == 0x80)
            {
            }
            else
            {
                std::cout << "weird mouse control: " << std::hex << u16(data) << std::endl;
                std::abort();
            }
        }
        else if (port == 1) // data
        {
            if (data == 0x10)
            {
                //int_wait = 100;
                pic.cpu_ack_irq(irq);
            }
            if (data == 0x16)
            {
                pic.request_interrupt(irq);
            }

            if (command_mode == CONTROL)
            {
                status = data;
            }
        }
        else if (port == 2) // signature
        {

        }
        else if (port == 3) // configuration
        {

        }
    }

    u32 int_wait{};
    void cycle()
    {
        /*if (int_wait > 0)
        {
            --int_wait;
            if (int_wait == 0)
            {
                std::cout << "mouse irq!" << std::endl;
                status |= 0x08;
                pic.request_interrupt(irq);
            }
        }*/
    }
};
