#pragma once

struct CHIP8259 //PIC
{
    u8 init_state{}; // are we initializing?
    u8 irr{}; // Interrupt Request Register
    u8 imr{}; // Interrupt Mask Register
    u8 isr{}; // In-Service Register
    u8 icw[5] = {}; // Initialization Command Words
    u8 ocw[4] = {}; // Operation Command Words
    int irq_to_cpu = -1;
    bool is_initialized = false;
    CHIP8259* main_pic{nullptr};

    void reset()
    {
        init_state = 0;
        irr = 0;
        imr = 0;
        isr = 0;
        icw[0] = 0;
        icw[1] = 0;
        icw[2] = 0;
        icw[3] = 0;
        icw[4] = 0;
        ocw[0] = 0;
        ocw[1] = 0;
        ocw[2] = 0;
        ocw[3] = 0;
        is_initialized = false;
    }

    bool is_cascade()
    {
        return !(icw[1]&0x02);
    }
    bool icw4_present()
    {
        return icw[1]&0x01;
    }

    u8 read(u8 port) // port from 0 to 1 inclusive
    {
        u8 data{};
        if (port == 0)
        {
            if (!(ocw[3]&1))
            {
                if constexpr (DEBUG_LEVEL > 0)
                    cout << PRETTY_FUNCTION << ":" << std::dec << __LINE__ << std::hex << endl;
                data = irr;
            }
            else
            {
                if constexpr (DEBUG_LEVEL > 0)
                    cout << PRETTY_FUNCTION << ":" << std::dec << __LINE__ << std::hex << endl;
                data = isr;
            }
        }
        else if (port == 1)
        {
            if constexpr (DEBUG_LEVEL > 0)
                cout << PRETTY_FUNCTION << ":" << std::dec << __LINE__ << std::hex << endl;
            data = imr;
        }
        return data;
    }

    void write(u8 port, u8 data) // port from 0 to 1 inclusive
    {
        if (port == 0)
        {
            if (data & 0x10) // ICW1
            {
                init_state = 1;
                icw[1] = data;
                is_initialized = false;
                if constexpr (DEBUG_LEVEL > 0)
                    cout << PRETTY_FUNCTION << ":" << std::dec << __LINE__ << std::hex << endl;
            }
            else
            {
                // OCW2 or OCW3
                if (data & 0x18) // OCW3
                {
                    ocw[3] = data;
                }
                else // OCW2
                {
                    ocw[2] = data;
                    if (data & 0x20) // End of Interrupt (EOI)
                    {
                        if ((data&0x07) != 0)
                            cout << "EOI isr " << u32(data & 0x07) << endl;
                        //isr &= ~(1 << (data & 0x07));
                        cpu_ack_irq(data&0x07);
                    }
                }
            }
        }
        else if (port == 1)
        {
            if (!is_cascade() && init_state == 2)
            {
                init_state = 3;
            }
            if (!icw4_present() && init_state == 3)
            {
                init_state = 0;
                is_initialized = true;
            }
            if (init_state == 1) // ICW2
            {
                icw[2] = data;
                init_state = 2;
            }
            else if (init_state == 2) // ICW3
            {
                icw[3] = data;
                init_state = 3;
            }
            else if (init_state == 3) // ICW4
            {
                icw[4] = data;
                init_state = 0;
                is_initialized = true;
            }
            else
            {
                // Write to Interrupt Mask Register (IMR)
                imr = data;
                update();
                if constexpr (DEBUG_LEVEL > 0)
                {
                    cout << "new interrupt mask: " << u32(imr) << endl;
                }
            }
        }
    }

    void cpu_ack_irq(u8 irq)
    {
        isr &= ~(1 << irq);
        irr &= ~(1 << irq);
        update();
    }
    void cpu_ack_irq()
    {
        isr &= ~(1 << irq_to_cpu);
        irr &= ~(1 << irq_to_cpu);
        update();
    }

    u8 vector_pos()
    {
        return icw[2]&0xF8;
    }

    void update()
    {
        if (!is_initialized)
        {
            return;
        }

        // Check for any pending interrupts
        for (int i = 0; i < 8; ++i)
        {
            if (!masked(i) && pending(i) && !serviced(i))
            {
                if (startprinting)
                    cout << "IRQ: SERVICE " << u32(i) << endl;
                isr |= (1 << i);
                irr &= ~(1 << i);
            }
        }

        //update current IRQ value
        irq_to_cpu = -1;
        for(u8 irq=0; irq<8; ++irq)
        {
            if (!masked(irq) && serviced(irq))
            {
                irq_to_cpu = irq;
                break;
            }
        }
        if (main_pic != nullptr)
        {
            if (irq_to_cpu == -1)
                main_pic->cpu_ack_irq(2);
            else
                main_pic->request_interrupt(2);
        }

        if (startprinting)
            std::cout << "irq 2 cpu now: " << u32(irq_to_cpu) << ", isr=" << u32(isr) << ", imr=" << u32(imr) << " irr=" << u32(irr) << std::endl;
    }

    bool masked(u8 irq)
    {
        return (imr&(1<<irq));
    }
    bool pending(u8 irq)
    {
        return (irr&(1<<irq));
    }
    bool serviced(u8 irq)
    {
        return (isr&(1<<irq));
    }

    bool request_interrupt(u8 irq)
    {
        if (!is_initialized || irq >= 8)
            return false;

        if (irr&(1<<irq))
            return false;
        if (isr&(1<<irq))
            return false;

        irr |= (1<<irq);
        update();
        return true;
    }
};

