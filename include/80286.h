#pragma once

#include "interrupt.h"
#include "mem286.h"

struct CPU80286
{
    MemoryManager286& mem;
    CHIP8259& pic;
    CHIP8259& pic2;
    IOSystem& iosystem;
    CPU80286(MemoryManager286& mem_, CHIP8259& pic_,  CHIP8259& pic2_, IOSystem& iosystem_) : mem(mem_), pic(pic_), pic2(pic2_), iosystem(iosystem_) {}


    static const u16 FLAG_MASK = 0b0000'1111'1101'0101;
    static const u16 FLAG_ON =   0b0000'0000'0000'0010;

    u16 registers[16] = {};

    u8 string_prefix{};
    u8 lock{};
    u32 delay{}; //HACK: to make the cpu slow down a bit so it passes POST lol.
    u32 cpu_steps{};

    enum SP_VALUES
    {
        SP_REPNZ = 1,
        SP_REPZ = 2 //also REP
    };

    enum REG
    {
        AX,CX,DX,BX, SP,BP,SI,DI,  //normal registers
        FLAGS,                     //flags, duh
        IP,                        //instruction pointer
        ESTMP, CSTMP, SSTMP, DSTMP,//for tests
        Z0                         //always zero
    };

    enum struct SEG
    {
        ES,CS,SS,DS,              //segment registers
        NO_OVERRIDE, //marker
    };
    SEG segment_override{SEG::NO_OVERRIDE};

    static constexpr u16 registermap[14] = //i wish we didnt need this
    {
        AX,CX,DX,BX, SP,BP,SI,DI, ESTMP, CSTMP, SSTMP, DSTMP, FLAGS, IP,
    };

    u16 msw{};

    u16 cpl{};

    struct Descriptor
    {
        u16 data{};
        u16 limit{};
        u32 base{}; //up to 0x00FFFFFF, really 24 bits.
        u8 flags{};

        u8 dpl()
        {
            return (flags>>5)&0x03;
        }
    };
    struct DescriptorTable
    {
        u32 base{};
        u16 n_entries{};
    };
    DescriptorTable gdtr{}, ldtr{}, idtr{}; //global, local and interrupt tables!
    Descriptor descriptor_cache[4]; //cache, one for each segment
    Descriptor task;

    void load_segment(SEG segment_number, u16 segment_data)
    {
        if (msw&1) // protected mode
        {
            if(startprinting)
                std::cout << "Load segment from data: " << segment_data << std::endl;
            // Extract fields from segment_data
            u16 index = segment_data >> 3;        // Bits 15-3
            bool is_ldt = segment_data & 0x4;     // Bit 2 (TI)
            [[maybe_unused]] u8 rpl = segment_data & 0x3;          // Bits 1-0

            // Choose GDT or LDT
            DescriptorTable& table = is_ldt ? ldtr : gdtr; //TODO: how to do idtr here?

            // Check if index is within table limits
            if(index*8 > table.n_entries)
            {
                // Should generate exception
                std::cout << std::dec << "load segment " << int(segment_number) << " from index " << index << " (sdata=" << segment_data <<  ") but table only has " << table.n_entries << " bytes." << std::hex << std::endl;
                protection_fault(original_ip, 0);
                //throw std::runtime_error("Segment index out of bounds");
            }
            else
            {
                // Calculate descriptor address in physical memory
                u32 descriptor_addr = table.base + (index * 8);

                // Read 8 bytes from physical memory
                u16 word1 = mem.r16(descriptor_addr);
                u16 word2 = mem.r16(descriptor_addr+2);
                u16 word3 = mem.r16(descriptor_addr+4);

                // Fill descriptor fields
                descriptor_cache[(int)segment_number].base = ((word3 & 0xFF) << 16) | word2;
                descriptor_cache[(int)segment_number].limit = word1;
                descriptor_cache[(int)segment_number].flags = (word3 >> 8) & 0xFF;
                descriptor_cache[(int)segment_number].data = segment_data;
                const char* names[4] = {"ES", "CS", "SS", "DS"};
                if (startprinting)
                {
                    std::cout << names[(int)segment_number] << ": Protected base=" << descriptor_cache[(int)segment_number].base << std::endl;
                    std::cout << "words: " << word1 << " " << word2 << " " << word3 << std::endl;
                    std::cout << "table base: " << table.base << std::endl;

                    std::cout << "first 16 entries: " << std::endl;
                    for(int i=0; i<16; ++i)
                    {
                        u16 entry_word1 = mem.r16(table.base+(i*8));
                        u16 entry_word2 = mem.r16(table.base+(i*8+2));
                        u16 entry_word3 = mem.r16(table.base+(i*8+4));
                        std::cout << "entry " << i << " words: " << entry_word1 << " " << entry_word2 << " " << entry_word3 << std::endl;
                    }
                }
            }

        }
        else // real mode
        {
            descriptor_cache[(int)segment_number].base = (segment_data<<4);
            descriptor_cache[(int)segment_number].limit = 0xFFFF;
            descriptor_cache[(int)segment_number].flags = 0;
            descriptor_cache[(int)segment_number].data = segment_data;

            const char* names[4] = {"ES", "CS", "SS", "DS"};
            if (startprinting)
                std::cout << names[(int)segment_number] << ":R" << descriptor_cache[(int)segment_number].base << std::endl;
        }
    }
    void load_tmp_segs_for_test()
    {
        load_segment(SEG::ES, registers[ESTMP]);
        load_segment(SEG::CS, registers[CSTMP]);
        load_segment(SEG::SS, registers[SSTMP]);
        load_segment(SEG::DS, registers[DSTMP]);
    }
    void store_tmp_segs_for_test()
    {
        registers[ESTMP] = descriptor_cache[0].base>>4;
        registers[CSTMP] = descriptor_cache[1].base>>4;
        registers[SSTMP] = descriptor_cache[2].base>>4;
        registers[DSTMP] = descriptor_cache[3].base>>4;
    }


    u32 get_offset(SEG segment_name)
    {
        auto& cache = descriptor_cache[(int)segment_name];
        u32 base = cache.base;
        return base;
    }

    enum FLAG
    {
        F_CARRY=0,
        F_PARITY=2,
        F_AUX_CARRY=4,
        F_ZERO=6,
        F_SIGN=7,
        F_TRAP=8,
        F_INTERRUPT=9,
        F_DIRECTIONAL=10,
        F_OVERFLOW=11
    };

    void print_flags()
    {
#define pflag(x) std::cout << #x << ": " << bool(registers[FLAGS]&u32(1<<x)) << std::endl;
        pflag(F_CARRY)
        pflag(F_PARITY)
        pflag(F_AUX_CARRY)
        pflag(F_ZERO)
        pflag(F_SIGN)
        pflag(F_OVERFLOW)
#undef pflag
    }

    void set_flag(FLAG f_n, bool value)
    {
        registers[FLAGS] = (registers[FLAGS]&~(1<<f_n))|(value?1<<f_n:0);
        registers[FLAGS] = (registers[FLAGS] & FLAG_MASK) | FLAG_ON;
    }

    bool flag(FLAG f_n)
    {
        return registers[FLAGS]&(1<<f_n);
    }

    void reset()
    {
        //std::cout << "CPU reset 80286" << std::endl;
        halt = false;
        clear_prefix();
        for(u32 i=0; i<16; ++i)
            registers[i] = 0x0000;

        registers[IP] = 0xFFF0;
        msw = 0;
        cpl = 0;
        registers[FLAGS] = FLAG_ON;

        load_segment(SEG::ES, 0x0000);
        load_segment(SEG::CS, 0xF000);
        load_segment(SEG::SS, 0x0000);
        load_segment(SEG::DS, 0x0000);

        mem.reset();
    }

    static const u32 PREFETCH_QUEUE_SIZE = 8;
    u8 prefetch_queue[PREFETCH_QUEUE_SIZE] = {};
    u32 prefetch_address{};

    template<typename T>
    T read_inst() requires integral<T>
    {
        T result{};
        u32 offset = get_offset(SEG::CS);
        if constexpr(sizeof(T)==1)
        {
            result = mem.r8(offset+registers[IP]);
        }
        else if constexpr(sizeof(T) == 2)
        {
            result = mem.r16(offset+registers[IP]);
        }
        registers[IP] += sizeof(T);
        return result;
    }
    template<typename T>
    T peek_inst() requires integral<T>
    {
        T result{};
        u32 offset = get_offset(SEG::CS);
        if constexpr(sizeof(T)==1)
        {
            result = mem.r8(offset+registers[IP]);
        }
        else if constexpr(sizeof(T) == 2)
        {
            result = mem.r16(offset+registers[IP]);
        }
        return result;
    }

    template<typename T>
    void commonflags(T a, T b, T result) requires std::same_as<T,u8> || std::same_as<T,u16>
    {
        set_flag(F_ZERO, result == 0);
        set_flag(F_SIGN, result >> (sizeof(T)*8-1));
        set_flag(F_PARITY, byte_parity[result & 0xFF]);
        set_flag(F_AUX_CARRY, ((a ^ b ^ result) & 0x10) != 0);
    }
    template<typename T>
    void cmp_flags(T a, T b, T result) requires std::same_as<T,u8> || std::same_as<T,u16>
    {
        commonflags(a,b,result);
        set_flag(F_OVERFLOW, ((a ^ b) & (a ^ result)) >> (sizeof(T)*8-1));
        set_flag(F_CARRY, a < b);
    }
    template<typename T>
    void test_flags(T result) requires std::same_as<T,u8> || std::same_as<T,u16>
    {
        commonflags(T(0),T(0),result);
        set_flag(F_OVERFLOW, false);
        //AUX_CARRY left undefined - so we set it in commonflags
        set_flag(F_AUX_CARRY, false); //set it false here to pass 0x0A test
        set_flag(F_CARRY, false);
    }
    template<typename T>
    void add_flags(T a, T b, T result) requires std::same_as<T,u8> || std::same_as<T,u16>
    {
        commonflags(a,b,result);
        set_flag(F_OVERFLOW, ((a ^ result) & (b ^ result)) >> (sizeof(T)*8-1));
        set_flag(F_CARRY, result < a);
    }

    static constexpr u8 effective_address_cycles[32] = //verify this still!
    {
         0, 0, 0, 0, 0, 0, 0, 0,
         1, 1, 1, 1, 0, 0, 0, 0,
         1, 1, 1, 1, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, //reg
    };
    bool modrm_is_register{};
    u32 modrm_seg{};
    u16 modrm_offset{};
    u8 modrm_reg{}; //register from modRM part
    u8 modrm_r{}; //register from R part

    void writeM8(u16 data)
    {
        if (modrm_is_register)
        {
            get_r8(modrm_reg) = data;
        }
        else
        {
            mem.w8(modrm_seg+modrm_offset, (data&0xFF));
        }
    }
    u16 readM8()
    {
        u16 ret{};
        if (modrm_is_register)
        {
            ret = get_r8(modrm_reg);
        }
        else
        {
            ret = mem.r8(modrm_seg+modrm_offset);
        }
        return ret;
    }
    void writeM16(u16 data)
    {
        if (modrm_is_register)
        {
            get_r16(modrm_reg) = data;
        }
        else
        {
            mem.w16(modrm_seg+modrm_offset, data);
        }
    }
    u16 readM16()
    {
        u16 ret{};
        if (modrm_is_register)
        {
            ret = get_r16(modrm_reg);
        }
        else
        {
            ret = mem.r16(modrm_seg+modrm_offset);
        }
        return ret;
    }

    static constexpr REG regchoice[32]=
    {
        BX,BX,BP,BP,Z0,Z0,Z0,BX,
        BX,BX,BP,BP,Z0,Z0,BP,BX,
        BX,BX,BP,BP,Z0,Z0,BP,BX,
        Z0,Z0,Z0,Z0,Z0,Z0,Z0,Z0,
    };
    static constexpr REG regchoice2[32]=
    {
        SI,DI,SI,DI,SI,DI,Z0,Z0,
    };
    static constexpr u16 MODRM_ADVANCE_IP[32] =
    {
        0,0,0,0,0,0,2,0,
        1,1,1,1,1,1,1,1,
        2,2,2,2,2,2,2,2,
        0,0,0,0,0,0,0,0,
    };

    void decode_modrm(u8 modrm)
    {
        u8 mod = (modrm >> 6) & 0x03;
        modrm_r = (modrm>>3)&0x07;
        modrm_reg = modrm & 0x07;
        modrm_is_register = (mod==0x03);
        u8 mod_table_index = modrm_reg | (mod<<3);
		if (mod != 0x03)
        {
            modrm_offset = 0;
            SEG segname = SEG::DS;

            switch(MODRM_ADVANCE_IP[mod_table_index])
            {
            case 1:
                modrm_offset = i16(peek_inst<i8>());
                break;
            case 2:
                modrm_offset = peek_inst<u16>();
                break;
            default:
                break;
            }

            modrm_offset += registers[regchoice[mod_table_index]]+registers[regchoice2[modrm_reg]];
            segname = (regchoice[mod_table_index]==BP)?SEG::SS:SEG::DS;
            registers[IP] += MODRM_ADVANCE_IP[mod_table_index];
            modrm_seg = get_offset(get_segment(segname));
        }
    }

    u8* reg8() { return (u8*)(void*)registers; }

    u16& get_r16(u8 value)
    {
        return registers[value&0x7];
    }
    u8& get_r8(u8 value)
    {
        return reg8()[((value&0x3)<<1)+((value&0x4)>>2)];
    }

    void print_regs()
    {
        for(int i=0; i<8; ++i)
            std::cout << " " << r16_names[i] << "=" << std::setw(4) << std::setfill('0') << registers[i];
        //for(int i=0; i<4; ++i)
        //    std::cout << " " << seg_names[i] << "=" << std::setw(4) << std::setfill('0') << registers[i+8];
        std::cout << " FL=" << std::setw(4) << std::setfill('0') << registers[FLAGS] << " IP=" << std::setw(4) << std::setfill('0') << registers[IP]-1;

        std::cout << " ES=" << descriptor_cache[0].base << "=" << descriptor_cache[0].data;
        std::cout << " CS=" << descriptor_cache[1].base << "=" << descriptor_cache[1].data;
        std::cout << " SS=" << descriptor_cache[2].base << "=" << descriptor_cache[2].data;
        std::cout << " DS=" << descriptor_cache[3].base << "=" << descriptor_cache[3].data;
        std::cout << " A20=" << globalsettings.A20;

        /*std::cout << " S ";
        std::cout << std::setw(4) << std::setfill('0') << mem._16(registers[SS],registers[SP]) << ' ';
        std::cout << std::setw(4) << std::setfill('0') << mem._16(registers[SS],registers[SP]+2) << ' ';
        std::cout << std::setw(4) << std::setfill('0') << mem._16(registers[SS],registers[SP]+4) << ' ';
        std::cout << std::setw(4) << std::setfill('0') << mem._16(registers[SS],registers[SP]+6) << ' ';*/
        std::cout << std::endl;
    }

    template<typename T>
    T run_arith(T p1, T p2, u8 instr_choice)
    {
        T out{};

        switch(instr_choice)
        {
            case 0x00: //ADD
                out = p1+p2;
                add_flags(p1,p2,out);
                break;
            case 0x01: //OR
                out = p1|p2;
                test_flags(out);
                break;
            case 0x02: //ADC
                out = p1+p2+flag(F_CARRY);
                commonflags(p1,p2,out);
                set_flag(F_AUX_CARRY, (p1&0xF)+(p2&0xF)+flag(F_CARRY) >= 0x10);
                set_flag(F_OVERFLOW, ((p1 ^ out) & (p2 ^ out)) >> (sizeof(T)*8-1));
                set_flag(F_CARRY, ((p1+p2+flag(F_CARRY))>>(sizeof(T)*8)) > 0);
                break;
            case 0x03: //SBB
                out = p1-(p2+flag(F_CARRY));
                commonflags(p1,p2,out);
                set_flag(F_AUX_CARRY, (p1&0xF)-((p2&0xF)+flag(F_CARRY)) < 0x00);
                set_flag(F_OVERFLOW, ((p1 ^ p2) & (p1 ^ out)) >> (sizeof(T)*8-1));
                set_flag(F_CARRY, ((p1-(p2+flag(F_CARRY)))>>(sizeof(T)*8)) < 0);
                break;
            case 0x04: //AND
                out = p1&p2;
                test_flags(out);
                break;
            case 0x05: //SUB
                out = p1-p2;
                cmp_flags(p1,p2,out);
                break;
            case 0x06://XOR
                out = p1^p2;
                test_flags(out);
                break;
            case 0x07: //CMP
                out = p1;
                cmp_flags(p1,p2,T(p1-p2));
                break;
        }
        return out;
    }

    SEG get_segment(SEG default_segment)
    {
        return segment_override!=SEG::NO_OVERRIDE?segment_override:default_segment;
    }

    void clear_prefix()
    {
        segment_override = SEG::NO_OVERRIDE;
        string_prefix = 0;
        lock = 0;
    }

    static constexpr u8 IS_PREFIX[0x100] =
    {
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //00
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //10
        0,0,0,0, 0,0,1,0, 0,0,0,0, 0,0,1,0, //20
        0,0,0,0, 0,0,1,0, 0,0,0,0, 0,0,1,0, //30

        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //40
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //50
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //60
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //70

        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //80
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //90
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //A0
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //B0

        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //C0
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //D0
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //E0
        4,0,2,3, 0,0,0,0, 0,0,0,0, 0,0,0,0, //F0
    };

    bool set_prefix(u8 instruction)
    {
        switch(IS_PREFIX[instruction])
        {
        case 1: //segment override
            segment_override = SEG((instruction>>3)&0x3);
            break;
        case 2: //REPNZ
            string_prefix = 1; //REPNZ
            break;
        case 3: //REPZ
            string_prefix = 2; //REPZ
            break;
        case 4: //LOCK
            lock = 1;
            break;
        }
        return bool(IS_PREFIX[instruction]);
    }

    u32 interrupt_true_cycles{};
    bool inhibit_ss{};
    u32 interrupt_table[256] = {};
    u8 interrupt_stack{};


    bool accepts_interrupts()
    {
        return interrupt_true_cycles >= 2 && !inhibit_ss && delay==0;
    }

    bool interrupt(u8 n, bool forced=false, bool has_code=false, u16 code=0)
    {
        if (inhibit_ss)
            return false;
        if (delay > 0)
            return false;
        if (accepts_interrupts() || forced)
        {
            if (startprinting)
                cout << "INTERRUPT " << u32(n) << " start! orig ip=" << registers[IP] << " protmode=" << (msw&1) << endl;
            halt = false;
            cycles_used += 23; //286

            ++interrupt_table[n];

            push(registers[FLAGS]);
            push(descriptor_cache[(int)SEG::CS].data);
            push(registers[IP]);
            if (has_code && (msw&1))
            {
                push(code);
            }

            u32 idtr_base = 0;
            u32 mult = 4;
            if (msw&1)
            {
                idtr_base = idtr.base;
                mult = 8;
            }
            registers[IP] = mem.r16(idtr_base + n*mult); //TODO: check n in protected mode
            load_segment(SEG::CS, mem.r16(idtr_base + n*mult+2));
            set_flag(F_INTERRUPT,false);
            set_flag(F_TRAP, false);
            if (!forced)
            {
                if (startprinting)
                    cout << "IRQ: CPU ACK " << u32(n-8) << endl;
                //pic.cpu_ack_irq(n-pic.vector_pos());
                return true;
            }
            return false;
        }
        return false;
    }

    u32 full_ip()
    {
        return u32(descriptor_cache[(int)SEG::CS].base) + u32(registers[IP]);
    }

    void divide_by_zero(u16 original_ip_value)
    {
        registers[IP] = original_ip_value;
        interrupt(0, true);
    }
    void invalid_instruction(u16 original_ip_value)
    {
        //cout << "Invalid instruction." << endl;
        registers[IP] = original_ip_value;
        interrupt(6, true);
    }
    void outside_bound(u16 original_ip_value)
    {
        //cout << "Bounds violation." << endl;
        registers[IP] = original_ip_value;
        interrupt(5, true);
    }
    void protection_fault(u16 original_ip_value, u16 error_code)
    {
        registers[IP] = original_ip_value;
        interrupt(13, true, true, error_code);
        std::cout << "-------------------PROTECTION FAULT----------------- at " << full_ip() << std::endl;
        print_regs();
        //std::abort();
    }

    void irq(u8 n)
    {
        if (n >= 8)
        {
            if (interrupt(n-8+pic2.vector_pos(), false))
            {
                pic2.cpu_ack_irq(n-8);
                pic.cpu_ack_irq(2);
            }
        }
        else
        {
            if (interrupt(n+pic.vector_pos(), false))
            {
                pic.cpu_ack_irq(n);
            }
        }
    }

    void push(u16 data)
    {
        registers[SP] -= 2;
        mem.w16(get_offset(SEG::SS) + registers[SP], data);
    }
    u16 pop()
    {
        u16 data = mem.r16(get_offset(SEG::SS) + registers[SP]);
        registers[SP] += 2;
        return data;
    }

    bool halt{false};
    u64 cycles_used{};
    bool is_inside_multi_part_instruction{};

    u16 original_ip{};

    void cycle()
    {
        bool previous_trap = flag(F_TRAP);
        ++cycles;

        if (delay)
        {
            --delay;
            return;
        }

        if (pic.irq_to_cpu != -1)
        {
            if (pic.irq_to_cpu == 2 && pic2.irq_to_cpu != -1)
                irq(pic2.irq_to_cpu+8);
            else
                irq(pic.irq_to_cpu);
        }

        if (halt)
        {
            return;
        }

        inhibit_ss = false;

        if (get_offset(SEG::CS) == 0 && registers[IP] == 0)
        {
            //cout << "Trying to run code at CS:IP 0:0... resetting." << endl;
            reset();
            //std::abort();
        }
        original_ip = registers[IP];

        is_inside_multi_part_instruction = false;

        globalsettings.current_IP = get_offset(SEG::CS)+registers[IP];
        if (globalsettings.current_IP == 0x7C00)
        {
            std::cout << "7C00 gotten!" << std::endl;
            //startprinting = true;
        }

        u8 instruction = read_inst<u8>();

        if (startprinting && (get_offset(SEG::CS)+registers[IP]-1) < 0xF0000)
        {
            std::cout << (msw&1?"&":"#") << std::dec << cycles << std::hex << ": " << u32(instruction) << " @ " << get_offset(SEG::CS)+registers[IP]-1;
            print_regs();
        }

        u32 prefix_byte_n = 0;
        while (set_prefix(instruction))
        {
            ++prefix_byte_n;
            instruction = read_inst<u8>();
            if ((startprinting || DEBUG_LEVEL > 1) && (get_offset(SEG::CS)+registers[IP]-1) < 0xF0000)
                std::cout << "prefix read. " << (msw&1?"&":"#") << std::dec << cycles << std::hex << ": " << "Executing 0x" << u32(instruction) << " at CS+IP = " << get_offset(SEG::CS) << ":" << registers[IP]-1 << " = " << get_offset(SEG::CS)+registers[IP]-1 << std::endl;
        }
        if (false);
        else if (instruction == 0x0F) // pop cs :-)
        {
            u8 secondbyte = read_inst<u8>();
            u8 modrm = read_inst<u8>();
            u8 op = u32(modrm>>3)%0x08;

            if (false);
            else if (secondbyte == 0x00 && op == 0x00) // SLDT
            {
                decode_modrm(modrm);
                u32 addr = modrm_seg + modrm_offset;
                mem.w16(addr, ldtr.n_entries);
                mem.w16(addr+2, (ldtr.base)&0xFFFF);
                mem.w16(addr+4, (ldtr.base>>16)&0xFFFF);
                std::cout << "store ldtr" << std::endl;
            }
            else if (secondbyte == 0x00 && op == 0x02) // LLDT
            {
                //TODO: check cpl
                decode_modrm(modrm);
                u32 addr = modrm_seg + modrm_offset;
                ldtr.n_entries = mem.r16(addr);
                ldtr.base = (mem.r16(addr+2) | (u32(mem.r16(addr+4))<<16))&0x00FFFFFF; //mask to 24bit max

                std::cout << "Loaded ldtr with n_entries=0x" << std::hex << ldtr.n_entries << " and base=0x" << ldtr.base << std::endl;
                //startprinting = true;
            }
            else if (secondbyte == 0x00 && op == 0x01) // STR
            {
                decode_modrm(modrm);
                u32 addr = modrm_seg + modrm_offset;
                mem.w16(addr, task.data);
                std::cout << "store task" << std::endl;
            }
            else if (secondbyte == 0x00 && op == 0x03) // LTR
            {
                //TODO: check cpl
                decode_modrm(modrm);
                u32 addr = modrm_seg + modrm_offset;
                task.data = mem.r16(addr);
                std::cout << "load task" << std::endl;
            }
            else if (secondbyte == 0x01 && op == 0x00) // SGDT
            {
                decode_modrm(modrm);
                u32 addr = modrm_seg + modrm_offset;
                mem.w16(addr, gdtr.n_entries);
                mem.w16(addr+2, (gdtr.base)&0xFFFF);
                mem.w16(addr+4, (gdtr.base>>16)&0xFFFF);
                //std::cout << "store gdtr" << std::endl;
                //startprinting = true;
            }
            else if (secondbyte == 0x01 && op == 0x02) // LGDT
            {
                //TODO: check cpl
                decode_modrm(modrm);
                u32 addr = modrm_seg + modrm_offset;
                gdtr.n_entries = mem.r16(addr);
                gdtr.base = (mem.r16(addr+2) | (u32(mem.r16(addr+4))<<16))&0x00FFFFFF; //mask to 24bit max

                //std::cout << "Loaded gdtr with n_entries=0x" << std::hex << gdtr.n_entries << " and base=0x" << gdtr.base << std::endl;
                //startprinting = true;
            }
            else if (secondbyte == 0x01 && op == 0x01) // SIDT
            {
                //TODO: check cpl
                decode_modrm(modrm);
                u32 addr = modrm_seg + modrm_offset;
                mem.w16(addr, idtr.n_entries);
                mem.w16(addr+2, (idtr.base)&0xFFFF);
                mem.w16(addr+4, (idtr.base>>16)&0xFFFF);
                //std::cout << "store idtr" << std::endl;
            }
            else if (secondbyte == 0x01 && op == 0x03) // LIDT
            {
                //TODO: check cpl
                decode_modrm(modrm);
                u32 addr = modrm_seg + modrm_offset;
                idtr.n_entries = mem.r16(addr);
                idtr.base = (mem.r16(addr+2) | (u32(mem.r16(addr+4))<<16))&0x00FFFFFF; //mask to 24bit max

                //std::cout << "Loaded idtr from fulladdr=" << addr << " with n_entries=0x" << std::hex << idtr.n_entries << " and base=0x" << idtr.base << std::endl;
            }
            else if (secondbyte == 0x01 && op == 0x06) // LMSW
            {
                decode_modrm(modrm);
                msw = (msw & 0xFFF1) | (readM16() & 0x000F);
                //std::cout << "New msw: " << msw << std::endl;

                /*std::cout << std::dec << gdtr.n_entries << " global entries." << std::endl;
                std::cout << std::hex;
                for(int i=0; i<gdtr.n_entries; ++i)
                {
                    std::cout << u16(mem.direct8(gdtr.base + i*8)) << " ";
                    std::cout << u16(mem.direct8(gdtr.base + i*8 +1)) << "       ";
                    std::cout << u16(mem.direct8(gdtr.base + i*8 +2)) << " ";
                    std::cout << u16(mem.direct8(gdtr.base + i*8 +3)) << " ";
                    std::cout << u16(mem.direct8(gdtr.base + i*8 +4)) << "       ";
                    std::cout << u16(mem.direct8(gdtr.base + i*8 +5)) << " ";
                    std::cout << std::endl;
                }*/
            }
            else if (secondbyte == 0x01 && op == 0x04) // SMSW
            {
                decode_modrm(modrm);
                writeM16(msw);
            }
            else
            {
                //std::cout << "popcs - " << u32(secondbyte) << " op=" << u32(modrm>>3)%0x08 << std::endl;
                //std::cout << "with address: " << effective_address(modrm) << std::endl;
                invalid_instruction(original_ip);
            }
            cycles_used += 10; //TODO: right values
            //invalid_instruction(original_ip); //TODO: v20, 286
        }
        else if ((instruction&0xF0) == 0x60) //PUSHA/POPA
        {
            //cout << "BLAH: " << u32(instruction) << endl; std::abort();
            if (instruction == 0x60)
            {
                //0x60 pusha //pushes all 8 regs
                u16 temp = registers[SP];
                push(registers[AX]);
                push(registers[CX]);
                push(registers[DX]);
                push(registers[BX]);
                push(temp);
                push(registers[BP]);
                push(registers[SI]);
                push(registers[DI]);
                cycles_used += 17; //286
            }
            else if (instruction == 0x61)
            {
                //0x61 popa //pops all 8 regs
                registers[DI] = pop();
                registers[SI] = pop();
                registers[BP] = pop();
                pop(); //ignore SP
                registers[BX] = pop();
                registers[DX] = pop();
                registers[CX] = pop();
                registers[AX] = pop();
                cycles_used += 19; //286
            }
            else if (instruction == 0x62) // BOUND
            {
                u8 modrm = read_inst<u8>();
                decode_modrm(modrm);
                u16 rm = readM16();
                u16 r = get_r16((modrm>>3)&0x07);
                u16 lower_bound = mem.r16(get_offset(get_segment(SEG::DS)) + rm);
                u16 upper_bound = mem.r16(get_offset(get_segment(SEG::DS)) + rm + 2);
                if (r < lower_bound || r > upper_bound)
                {
                    outside_bound(original_ip);
                }
                cycles_used += 13; //286, TODO: check!
            }
            else if (instruction == 0x68)
            {
                //0x68 push immed word
                u16 imm = read_inst<u16>();
                push(imm);
                cycles_used += 3; //286
            }
            else if (instruction == 0x69)
            {
                //std::abort();
                //0x69 mul modrm, immed word
                u8 modrm = read_inst<u8>();
                decode_modrm(modrm);
                u16 rm = readM16();
                u16 op2 = read_inst<u16>();
                i16 result = i32(i16(rm))*i32(i16(op2));
                set_flag(F_SIGN,result&0x8000);
                set_flag(F_PARITY,byte_parity[(result>>16)&0xFF]);
                set_flag(F_OVERFLOW,result>=0x80 || result < -0x80);
                set_flag(F_CARRY,result>=0x80 || result < -0x80);
                set_flag(F_AUX_CARRY,false);
                set_flag(F_ZERO,(result)==0);

                //registers[AX] = result&0xFFFF;
                //registers[DX] = result >> 16;

                get_r16(modrm_r) = result;

                cycles_used += (modrm_is_register?21:24); //286
            }
            else if (instruction == 0x6A)
            {
                //0x6A push immed byte, sign extension!
                i8 imm = read_inst<u8>();
                push(i16(imm));
                cycles_used += 3; //286
            }
            else if (instruction == 0x6B)
            {
                //std::abort();
                //0x6B mul modrm, immed byte
                u8 modrm = read_inst<u8>();
                decode_modrm(modrm);
                u16 rm = readM16();
                u16 op2 = i16(read_inst<i8>());
                i16 result = i16(rm)*i16(op2);

                set_flag(F_SIGN,result&0x8000);
                set_flag(F_PARITY,byte_parity[u16(result)>>8]);
                set_flag(F_OVERFLOW,result>=0x80 || result < -0x80);
                set_flag(F_CARRY,result>=0x80 || result < -0x80);
                set_flag(F_AUX_CARRY,false);
                set_flag(F_ZERO,(result&0xFFFF)==0);
                //registers[AX] = u16(result);

                get_r16(modrm_r) = result;

                cycles_used += (modrm_is_register?21:24); //286
            }
            else if (instruction == 0x6C) // INS, byte from DX port
            {
                u16 port = registers[DX];
                if (string_prefix == 0)
                {
                    mem.w8(get_offset(SEG::ES) + registers[DI], iosystem.io_in<u8>(port));
                    registers[DI] += flag(F_DIRECTIONAL) ? -1 : 1;
                    cycles_used += 5; //286
                }
                else
                {
                    while (registers[CX] != 0)
                    {
                        mem.w8(get_offset(SEG::ES) + registers[DI], iosystem.io_in<u8>(port));
                        registers[DI] += flag(F_DIRECTIONAL) ? -1 : 1;
                        registers[CX] -= 1;
                        cycles_used += 5; //286
                    }
                }
            }
            else if (instruction == 0x6D) // INS, word from DX port
            {
                u16 port = registers[DX];
                if (string_prefix == 0)
                {
                    u16 data = iosystem.io_in<u16>(port);
                    mem.w16(get_offset(SEG::ES) + registers[DI], data);
                    registers[DI] += flag(F_DIRECTIONAL) ? -2 : 2;
                    cycles_used += 5; //286
                }
                else
                {
                    while (registers[CX] != 0)
                    {
                        u16 data = iosystem.io_in<u16>(port);
                        mem.w16(get_offset(SEG::ES) + registers[DI], data);
                        registers[DI] += flag(F_DIRECTIONAL) ? -2 : 2;
                        registers[CX] -= 1;
                        cycles_used += 5; //286
                    }
                }
            }
            else if (instruction == 0x6E) // OUTS, byte to DX port
            {
                u16 port = registers[DX];
                if (string_prefix == 0)
                {
                    iosystem.io_out<u8>(port, mem.r8(get_offset(get_segment(SEG::DS)) + registers[SI]));
                    registers[SI] += flag(F_DIRECTIONAL) ? -1 : 1;
                    cycles_used += 5; //286
                }
                else
                {
                    while (registers[CX] != 0)
                    {
                        iosystem.io_out<u8>(port, mem.r8(get_offset(get_segment(SEG::DS)) + registers[SI]));
                        registers[SI] += flag(F_DIRECTIONAL) ? -1 : 1;
                        registers[CX] -= 1;
                        cycles_used += 5; //286
                    }
                }
            }
            else if (instruction == 0x6F) // OUTS, word to DX port
            {
                u16 port = registers[DX];
                if (string_prefix == 0)
                {
                    u16 value = mem.r16(get_offset(get_segment(SEG::DS)) + registers[SI]);
                    iosystem.io_out<u16>(port, value);
                    registers[SI] += flag(F_DIRECTIONAL) ? -2 : 2;
                    cycles_used += 5; //286
                }
                else
                {
                    while (registers[CX] != 0)
                    {
                        u16 value = mem.r16(get_offset(get_segment(SEG::DS)) + registers[SI]);
                        iosystem.io_out<u16>(port, value);
                        registers[SI] += flag(F_DIRECTIONAL) ? -2 : 2;
                        registers[CX] -= 1;
                        cycles_used += 5; //286
                    }
                }
            }
            else
            {
                invalid_instruction(original_ip);
            }
        }
        else if (instruction == 0xC0)
        {
            //cout << "BLAH: " << u32(instruction) << endl; std::abort();
            //0xC0 shift/rotate imm8 (take op from modrm as in the other rotate instructions)
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            u8 rm = readM8();
            u8 amount = read_inst<u8>() & 0x1F; // Only the lower 5 bits are used for the shift count

            u8 inst_type = (modrm >> 3) & 0x07;
            cycles_used += (modrm_is_register ? 5 : 8); //286

            for (u32 i = 0; i < amount; ++i)
            {
                cycles_used += 1; //286
                u8 original = rm;
                u8 result = 0;

                if (inst_type == 6)
                {
                    invalid_instruction(original_ip);
                    break;
                }
                // ROL ROR RCL RCR SHL SHR SAL SAR
                else if ((inst_type & 1) == 0) // ROL RCL SHL SAL
                {
                    result = (original << 1) | ((inst_type & 0x04) ? 0 : ((inst_type & 0x02) ? flag(F_CARRY) : original >> 7));
                }
                else if (inst_type == 1 || inst_type == 3) // ROR RCR
                {
                    result = (original >> 1) | ((inst_type & 0x02) ? flag(F_CARRY) << 7 : original << 7);
                }
                else if (inst_type == 5 || inst_type == 7) // SHR SAR
                {
                    result = (original >> 1) | ((inst_type & 0x02) ? original & 0x80 : 0);
                }
                set_flag(F_CARRY, original & ((inst_type & 1) ? 0x01 : 0x80));
                u8 flag_value = (inst_type & 1) ? result : original;
                set_flag(F_OVERFLOW, (bool(flag_value & 0x80) != bool(flag_value & 0x40)));
                if (inst_type & 0x04)
                {
                    set_flag(F_SIGN, result & 0x80);
                    set_flag(F_ZERO, result == 0);
                    set_flag(F_AUX_CARRY, (inst_type == 4 ? result & 0x10 : false));
                    set_flag(F_PARITY, byte_parity[result & 0xFF]);
                }
                rm = result;
            }
            writeM8(rm);
        }
        else if (instruction == 0xC1)
        {
            //cout << "BLAH: " << u32(instruction) << endl; std::abort();
            //0xC0 shift/rotate imm8 (take op from modrm as in the other rotate instructions)
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            u16 rm = readM16();
            u8 amount = read_inst<u8>() & 0x1F; // Only the lower 5 bits are used for the shift count

            u8 inst_type = (modrm >> 3) & 0x07;
            cycles_used += (modrm_is_register ? 5 : 8); //286

            for (u32 i = 0; i < amount; ++i)
            {
                cycles_used += 1;
                u16 original = rm;
                u16 result = 0;

                if (inst_type == 6)
                {
                    invalid_instruction(original_ip);
                    break;
                }
                // ROL ROR RCL RCR SHL SHR SAL SAR
                else if ((inst_type & 1) == 0) // ROL RCL SHL SAL
                {
                    result = (original << 1) | ((inst_type & 0x04) ? 0 : ((inst_type & 0x02) ? flag(F_CARRY) : original >> 15));
                }
                else if (inst_type == 1 || inst_type == 3) // ROR RCR
                {
                    result = (original >> 1) | ((inst_type & 0x02) ? flag(F_CARRY) << 15 : original << 15);
                }
                else if (inst_type == 5 || inst_type == 7) // SHR SAR
                {
                    result = (original >> 1) | ((inst_type & 0x02) ? original & 0x8000 : 0);
                }
                set_flag(F_CARRY, original & ((inst_type & 1) ? 0x01 : 0x8000));
                u8 flag_value = (inst_type & 1) ? result : original;
                set_flag(F_OVERFLOW, (bool(flag_value & 0x8000) != bool(flag_value & 0x4000)));
                if (inst_type & 0x04)
                {
                    set_flag(F_SIGN, result & 0x8000);
                    set_flag(F_ZERO, result == 0);
                    set_flag(F_AUX_CARRY, (inst_type == 4 ? result & 0x10 : false));
                    set_flag(F_PARITY, byte_parity[result & 0xFF]);
                }
                rm = result;
            }
            writeM16(rm);
        }
        else if (instruction == 0xC8)
        {
            //0xC8 ENTER data16, imm8
            u16 frame_size = read_inst<u16>();
            u8 nesting_level = read_inst<u8>();

            push(registers[BP]);
            u16 frame_temp = registers[SP];
            if (nesting_level > 0)
            {
                for (u8 i = 1; i < nesting_level; ++i)
                {
                    registers[BP] -= 2;
                    push(mem.r16(get_offset(SEG::SS) + registers[BP]));
                }
                push(frame_temp);
            }
            registers[BP] = frame_temp;
            registers[SP] -= frame_size;
            cycles_used += 11 + 4 * nesting_level + (nesting_level>1?1:0); //286
        }
        else if (instruction == 0xC9)
        {
            //0xC9 LEAVE
            registers[SP] = registers[BP];
            registers[BP] = pop();
            cycles_used += 5; //286
        }
        else if (instruction < 0x40 && (instruction&0x07) < 6)
        {
            u8 instr_choice = (instruction&0x38)>>3;
            if (instruction&0x04) //AX,imm
            {
                if (instruction&0x01)//16bit
                {
                    u16& r = registers[AX];
                    u16 imm = read_inst<u16>();
                    r = run_arith(r, imm, instr_choice);
                }
                else //8bit
                {
                    u8& r = get_r8(0);
                    u8 imm = read_inst<u8>();
                    r = run_arith(r, imm, instr_choice);
                }
                cycles_used += 3; //286
            }
            else //reg, r/m
            {
                u8 modrm = read_inst<u8>();
                decode_modrm(modrm);
                if (instruction&0x01)//16bit
                {
                    u16 rm = readM16();
                    u16& r = get_r16((modrm>>3)&0x07);
                    u16& rout = (instruction&0x02?r:rm);
                    u16& rin = (instruction&0x02?rm:r);
                    rout = run_arith(rout, rin, instr_choice);

                    if (!(instruction&0x02))
                    {
                        writeM16(rout);
                    }
                }
                else//8bit
                {
                    u8 rm = readM8();
                    u8& r = get_r8((modrm>>3)&0x07);
                    u8& rout = (instruction&0x02?r:rm);
                    u8& rin = (instruction&0x02?rm:r);
                    rout = run_arith(rout, rin, instr_choice);

                    if (!(instruction&0x02))
                    {
                        writeM8(rout);
                    }
                }
                cycles_used += (modrm_is_register?2:7); //286
            }
        }
        else if ((instruction&0xE6) == 0x06) // push/pop SEG
        {
            u8 seg_n = (instruction>>3)&0x03;
            if (instruction&0x01)
            {
                cycles_used += ((msw&1)?20:3); //286
                load_segment(SEG(seg_n), pop());
                inhibit_ss = true;
            }
            else
            {
                cycles_used += 3; //286
                if (startprinting)
                    std::cout << "push seg " << u32(seg_n) << " = " << descriptor_cache[seg_n].data << std::endl;
                push(descriptor_cache[seg_n].data);
            }
        }
        else if (instruction == 0x27) // DAA
        {
            u8 old_AL = registers[AX]&0xFF;
            bool weird_special_case = (!flag(F_CARRY)) && flag(F_AUX_CARRY);

            u8 added{};

            set_flag(F_AUX_CARRY, (registers[AX] & 0x0F) > 9 || flag(F_AUX_CARRY));
            if (flag(F_AUX_CARRY))
                added += 0x06;

            set_flag(F_CARRY, old_AL > 0x99+(weird_special_case?6:0) || flag(F_CARRY));
            if (flag(F_CARRY))
                added += 0x60;

            get_r8(0) += added;

            set_flag(F_ZERO, (registers[AX]&0xFF) == 0);
            set_flag(F_SIGN, (registers[AX] & 0x80));
            set_flag(F_PARITY, byte_parity[registers[AX]&0xFF]);
            set_flag(F_OVERFLOW, (old_AL ^ registers[AX]) & (added ^ registers[AX])&0x80);
            cycles_used += 3; //286
        }
        else if (instruction == 0x37) // AAA
        {
            u16 old_AX = registers[AX];
            bool add_ax = (registers[AX] & 0x0F) > 9 || flag(F_AUX_CARRY);
            if (add_ax)
            {
                get_r8(0) += 0x06; //AL
                get_r8(4) += 0x01; //AH
            }
            u16 added = registers[AX]-old_AX;

            set_flag(F_AUX_CARRY, add_ax);
            set_flag(F_CARRY, add_ax);
            set_flag(F_PARITY, byte_parity[registers[AX]&0xFF]);
            set_flag(F_ZERO, (registers[AX]&0xFF) == 0);
            set_flag(F_SIGN, (registers[AX] & 0x80));
            set_flag(F_OVERFLOW, (old_AX ^ registers[AX]) & (added ^ registers[AX])&0x80);

            get_r8(0) &= 0x0F;
            cycles_used += 3; //286
        }
        else if (instruction == 0x2F) // DAS
        {
            u8 old_AL = registers[AX] & 0xFF;
            bool weird_special_case = (!flag(F_CARRY)) && flag(F_AUX_CARRY);

            u8 subtracted{};

            bool sub_al = ((registers[AX] & 0x0F) > 9 || flag(F_AUX_CARRY));
            if (sub_al)
                subtracted += 0x06;

            set_flag(F_AUX_CARRY, sub_al);
            bool sub_al2 = (old_AL > (0x99+(weird_special_case?6:0)) || flag(F_CARRY));
            if (sub_al2)
                subtracted += 0x60;

            get_r8(0) -= subtracted;
            set_flag(F_CARRY, sub_al2);
            set_flag(F_ZERO, (registers[AX] & 0xFF) == 0);
            set_flag(F_SIGN, (registers[AX] & 0x80));
            set_flag(F_PARITY, byte_parity[registers[AX] & 0xFF]);
            set_flag(F_OVERFLOW, ((old_AL ^ subtracted) & (old_AL ^ registers[AX]))&0x80);
            cycles_used += 3; //286
        }
        else if (instruction == 0x3F) // AAS
        {
            u16 old_AX = registers[AX];
            bool sub_ax = (registers[AX] & 0x0F) > 9 || flag(F_AUX_CARRY);
            if (sub_ax)
            {
                get_r8(4) -= 1;
                get_r8(0) -= 6;
            }
            u16 subtracted = old_AX-registers[AX];
            set_flag(F_AUX_CARRY, sub_ax);
            set_flag(F_CARRY, sub_ax);
            set_flag(F_ZERO, (registers[AX] & 0xFF) == 0);
            set_flag(F_SIGN, (registers[AX] & 0x80));
            set_flag(F_PARITY, byte_parity[registers[AX] & 0xFF]);
            set_flag(F_OVERFLOW, (old_AX ^ subtracted) & (old_AX ^ registers[AX])&0x80);

            get_r8(0) &= 0x0F;
            cycles_used += 3; //286
        }
        else if ((instruction&0xF0) == 0x40) //INC/DEC register
        {
            u16 result = registers[instruction&0x07]+(1-((instruction&0x08)?2:0));
            set_flag(F_SIGN, result&0x8000);
            set_flag(F_ZERO, result==0);
            set_flag(F_AUX_CARRY, (result&0x0F) == ((instruction&0x08)?0x0F:0x00));
            set_flag(F_PARITY, byte_parity[result&0xFF]);
            set_flag(F_OVERFLOW, result==0x8000-((instruction&0x08)?1:0));
            registers[instruction&0x07] = result;
            cycles_used += 2; //286
        }
        else if ((instruction&0xF8) == 0x50) // push reg
        {
            push(registers[instruction&0x07]);
            cycles_used += 3; //286
        }
        else if ((instruction&0xF8) == 0x58) //pop reg
        {
            registers[instruction&0x07] = pop();
            cycles_used += 5; //286
        }
        else if ((instruction&0xF0) == 0x70) //various short jumps
        {
            u8 type = (instruction&0x0F)>>1;
            u16 f = (registers[FLAGS]&0xFFFD) | (flag(F_SIGN) != flag(F_OVERFLOW) ? 0x2:0x0);
            const u16 masks[8] =
            {
                0x800,0x001,0x040,0x041,0x080,0x004,0x002,0x042
            };
            i8 offset = read_inst<i8>();
            cycles_used += 3; //286
            if (bool(f&masks[type])^(instruction&0x01))
            {
                registers[IP] += offset;
                cycles_used += 4; //286
            }
        }
        else if (instruction == 0x80 || instruction == 0x82)
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            u8 rm = readM8();
            u8 imm = read_inst<u8>();
            rm = run_arith(rm, imm, (modrm>>3)&0x07);
            writeM8(rm);
            cycles_used += (modrm_is_register?3:7); //286
        }
        else if (instruction == 0x81)
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            u16 rm = readM16();
            u16 imm = read_inst<u16>();
            rm = run_arith(rm, imm, (modrm>>3)&0x07);
            writeM16(rm);
            cycles_used += (modrm_is_register?3:7); //286
        }
        else if (instruction == 0x83)
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            u16 rm = readM16();
            u16 imm = i16(read_inst<i8>());
            rm = run_arith(rm, imm, (modrm>>3)&0x07);
            writeM16(rm);
            cycles_used += (modrm_is_register?3:7); //286
        }
        else if (instruction == 0x84) //TEST
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            u8 rm = readM8();
            u8 r = get_r8((modrm>>3)&0x07);
            test_flags(u8(rm&r));
            cycles_used += (modrm_is_register?2:6); //286
        }
        else if (instruction == 0x85) //TEST
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            u16 rm = readM16();
            u16 r = get_r16((modrm>>3)&0x07);
            test_flags(u16(rm&r));
            cycles_used += (modrm_is_register?2:6); //286
        }
        else if (instruction == 0x86) //XCHG
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            u8 rm = readM8();
            u8& r = get_r8((modrm>>3)&0x07);
            u8 temp = rm;
            rm = r;
            r = temp;
            writeM8(rm);
            cycles_used += (modrm_is_register?3:5); //286
        }
        else if (instruction == 0x87) //XCHG
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            u16 rm = readM16();
            u16& r = get_r16((modrm>>3)&0x07);
            u16 temp = rm;
            rm = r;
            r = temp;
            writeM16(rm);
            cycles_used += (modrm_is_register?3:5); //286
        }
        else if ((instruction&0xFC) == 0x88) // MOV EbGb, EvGv, GbEb, GvEv
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            if (instruction&0x01)//16bit
            {
                u16& r = get_r16((modrm>>3)&0x07);
                if (instruction&0x02) // towards general register
                    r = readM16();
                else
                    writeM16(r);
            }
            else//8bit
            {
                u8& r = get_r8((modrm>>3)&0x07);
                if (instruction&0x02) // towards general register
                    r = readM8();
                else
                    writeM8(r);
            }

            if (modrm_is_register)
            {
                cycles_used += 2; //286
            }
            else
            {
                if (instruction&0x02) // towards general register
                {
                    cycles_used += 5; //286
                }
                else //towards modrm
                {
                    cycles_used += 3; //286
                }
            }
        }
        else if (instruction == 0x8C) // MOV EwSw
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            if (((modrm>>3)&0x07) < 4)
            {
                writeM16(descriptor_cache[(modrm>>3)&0x07].data);
            }
            else
            {
                invalid_instruction(original_ip);
            }
            cycles_used += (modrm_is_register?2:3); //286
        }
        else if (instruction == 0x8E) // MOV SwEw
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            u16 rm = readM16();
            u8 seg_n = (modrm>>3)&0x07;

            if(seg_n >= 4) //not valid segment registers, we only have four.
            {
                invalid_instruction(original_ip);
            }
            else if (msw&1) //protected mode
            {
                if (seg_n == 1) //CS
                {
                    protection_fault(original_ip, 0);
                }
                else
                {
                    u16 selector = rm;

                    // Get descriptor
                    u16 index = selector >> 3;
                    if (index * 8 >= gdtr.n_entries)
                    {
                        protection_fault(original_ip,0);
                    }
                    else
                    {
                        load_segment(SEG(seg_n), selector);
                        Descriptor& desc = descriptor_cache[seg_n];

                        // Check descriptor privileges
                        if (desc.dpl() < cpl || desc.dpl() < (selector & 3))
                        {
                            protection_fault(original_ip,0);
                        }
                    }

                }

            }
            else
            {
                load_segment(SEG(seg_n), rm);
            }
            if (msw&1)
                cycles_used += (modrm_is_register?17:19); //286
            else
                cycles_used += (modrm_is_register?2:5); //286

            inhibit_ss = true;
        }
        else if (instruction == 0x8D) // LEA Gv M
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            u16& r = get_r16((modrm>>3)&0x07);
            r = modrm_offset;
            cycles_used += 3; //286
        }
        else if (instruction == 0x8F) //POP modrm
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            writeM16(pop());
            cycles_used += 5; //286
        }
        else if ((instruction&0xF8) == 0x90) // XCHG AX, r16 - note how 0x90 is effectively NOP :-)
        {
            u8 reg_id = instruction&0x7;
            u16 tmp = registers[AX];
            registers[AX] = registers[reg_id];
            registers[reg_id] = tmp;
            cycles_used += 3; //286
        }
        else if (instruction == 0x98) //CBW
        {
            u16 r = (registers[AX])&0xFF;
            r |= (r&0x80)?0xFF00:0x0000;
            registers[AX] = r;
            cycles_used += 2; //286
        }
        else if (instruction == 0x99) //CWD
        {
            registers[DX] = (registers[AX]&0x8000)?0xFFFF:0x0000;
            cycles_used += 2; //286
        }
        else if (instruction == 0x9A) //call Ap
        {
            u16 pointer = read_inst<u16>();
            u16 segment = read_inst<u16>();
            push(descriptor_cache[(int)SEG::CS].data);
            push(registers[IP]);
            load_segment(SEG::CS, segment);
            registers[IP] = pointer;
            cycles_used += 13; //286, TODO: protected mode
        }
        else if (instruction == 0x9B) // WAIT/FWAIT
        {
            // waits for floating point exceptions.
            // basically a NOP because I don't have a FPU yet
            cycles_used += 3; //286
        }
        else if (instruction == 0x9C) //pushf
        {
            push(registers[FLAGS]);
            cycles_used += 3; //286
        }
        else if (instruction == 0x9D) //popf
        {
            u16 newflags = pop();
            registers[FLAGS] = (newflags&FLAG_MASK) | FLAG_ON;
            cycles_used += 5; //286
        }
        else if (instruction == 0x9E) //sahf
        {
            reg8()[FLAGS*2] = (get_r8(4)&0xD5) | 0x02;
            cycles_used += 2; //286
        }
        else if (instruction == 0x9F) //lahf
        {
            get_r8(4) = registers[FLAGS]&0xFF;
            cycles_used += 2; //286
        }
        else if (instruction >= 0xA0 && instruction <= 0xA3) //AL/X=MEM  MEM=AL/X
        {
            u32 source_segment = get_offset(get_segment(SEG::DS));
            u16 source_offset = read_inst<u16>();
            switch(instruction)
            {
                case 0xA0: get_r8(0) = mem.r8(source_segment + source_offset); break;
                case 0xA1: registers[AX] = mem.r16(source_segment + source_offset); break;
                case 0xA2: mem.w8(source_segment + source_offset, get_r8(0)); break;
                case 0xA3: mem.w16(source_segment + source_offset, registers[AX]); break;
            }
            cycles_used += (instruction&0x02)?3:5; //286
        }
        else if (instruction >= 0xA8 && instruction <= 0xA9) //TEST AL/X,imm8/16
        {
            if (instruction&1)
            {
                u16 value1 = read_inst<u16>();
                test_flags<u16>(u16(value1&registers[AX]));
            }
            else
            {
                u8 value1 = read_inst<u8>();
                test_flags<u8>(u8(value1&registers[AX]));
            }
            cycles_used += 3; //286
        }
        else if (instruction >= 0xA4 && instruction <= 0xAF) //MOVSB/W CMPSB/W --- STOSB/W LODSB/W SCASB/W
        {
            bool big = (instruction&0x01); //word-sized?
            i8 size = i8(big)+1;
            i8 direction = flag(F_DIRECTIONAL)?-size:size;
            const u16 programs[8] = // lol microcode
            {
                0x0000, 0x0000, 0x4021, 0x8103,
                0x0000, 0x4024, 0x4041, 0x8106,
            };

            const u8 cycles_single[16] =
            {
                 0,  0,  0,  0, 5, 5, 8, 8,
                 0,  0,  3,  3, 5, 5, 7, 7,
            }; //286
            const u8 cycles_rep_mult[16] =
            {
                 0,  0,  0,  0, 5, 5, 8, 8,
                 0,  0,  3,  3, 5, 5, 7, 7,
            }; //286 TODO: check!

            //u16 source_segment = registers[get_segment(DS)];

            u16 program = programs[(instruction>>1)&0x07];
            if (registers[CX] != 0 || string_prefix == 0) do
            {
                u16 value1{}, value2{};
                if (big)
                {
                    if (program&0x01)
                        value1 = mem.r16(get_offset(get_segment(SEG::DS)) +registers[SI]);
                    if (program&0x02)
                        value2 = mem.r16(get_offset(SEG::ES) +registers[DI]);
                    if (program&0x04)
                        value1 = registers[AX];
                    if (program&0x10)
                        mem.w16(get_offset(get_segment(SEG::DS)) + registers[SI], value1);
                    if (program&0x20)
                        mem.w16(get_offset(SEG::ES) + registers[DI], value1);
                    if (program&0x40)
                        registers[AX] = value1;
                    if (program&0x100)
                        cmp_flags<u16>(value1,value2,u16(value1-value2));
                }
                else
                {
                    if (program&0x01)
                        value1 = mem.r8(get_offset(get_segment(SEG::DS)) + registers[SI]);
                    if (program&0x02)
                        value2 = mem.r8(get_offset(SEG::ES) + registers[DI]);
                    if (program&0x04)
                        value1 = get_r8(0);
                    if (program&0x10)
                        mem.w8(get_offset(get_segment(SEG::DS)) + registers[SI], value1);
                    if (program&0x20)
                        mem.w8(get_offset(SEG::ES) + registers[DI], value1);
                    if (program&0x40)
                        get_r8(0) = value1;
                    if (program&0x100)
                        cmp_flags<u8>(value1,value2,u8(value1-value2));
                }
                if (program&0x11) //uses DS:SI
                    registers[SI] += direction;
                if (program&0x22) //uses ES:DI
                    registers[DI] += direction;

                if (string_prefix == 0)
                {
                    cycles_used += cycles_single[instruction&0x0F];
                    break;
                }
                cycles_used += cycles_rep_mult[instruction&0x0F];
                registers[CX] -= 1;
                if ((program&0xC000)==0x8000)
                {
                    if (string_prefix == SP_REPNZ && flag(F_ZERO))
                    {
                        cycles_used += 5; //286 todo: check this?
                        break;
                    }
                    if (string_prefix == SP_REPZ && !flag(F_ZERO))
                    {
                        cycles_used += 5; //286 todo: check this?
                        break;
                    }
                }
                if (registers[CX] == 0)
                {
                    cycles_used += 5; //286 todo: check this?
                    break;
                }
                registers[IP] = original_ip;
                is_inside_multi_part_instruction = true;
            } while(false);
            else
            {
                cycles_used += 6;
            }
        }
        else if ((instruction&0xF8) == 0xB0) //mov reg8, Ib
        {
            get_r8(instruction&0x07) = read_inst<u8>();
            cycles_used += 2; //286
        }
        else if ((instruction&0xF8) == 0xB8) //mov reg16, Iv
        {
            get_r16(instruction&0x07) = read_inst<u16>();
            cycles_used += 2; //286
        }
        else if (instruction == 0xC2) // near return w/imm
        {
            u16 imm = read_inst<u16>();
            registers[IP] = pop();
            registers[SP] += imm;
            cycles_used += 11; //286
        }
        else if (instruction == 0xC3) // near return
        {
            registers[IP] = pop();
            cycles_used += 11; //286
        }
        else if (instruction == 0xCA) // far return w/imm
        {
            u16 imm = read_inst<u16>();
            registers[IP] = pop();
            load_segment(SEG::CS, pop());
            registers[SP] += imm;
            cycles_used += (msw&1)?25:15; //286 TODO: 55 if lesser privilege
        }
        else if (instruction == 0xCB) // far return
        {
            registers[IP] = pop();
            load_segment(SEG::CS, pop());
            cycles_used += (msw&1)?25:15; //286 TODO: 55 if lesser privilege
        }
        else if ((instruction&0xFE) == 0xC4) // LES LDS
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            u32 addr = modrm_seg+modrm_offset;
            u16& r = get_r16((modrm>>3)&0x07);
            r = mem.r16(addr);
            load_segment((instruction&1)?SEG::DS:SEG::ES, mem.r16(addr+2)); //ES or DS, based on the opcode
            cycles_used += (msw&1)?21:7; //286
        }
        else if (instruction == 0xC6) //MOV
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            writeM8(read_inst<u8>());
            cycles_used += (modrm_is_register?2:3); //286
        }
        else if (instruction == 0xC7) //MOV
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            writeM16(read_inst<u16>());
            cycles_used += (modrm_is_register?2:3); //286
        }
        else if (instruction == 0xCC) // INT 3
        {
            if (startprinting)
                cout << "Calling interrupt 3... AX=" << registers[AX] << endl;
            interrupt(3, true);
        }
        else if (instruction == 0xCD) // INT imm8
        {
            u8 int_num = read_inst<u8>();
            if (startprinting)
                cout << "Calling interrupt... " << u32(int_num) << " AX=" << registers[AX] << endl;
            interrupt(int_num, true);
        }
        else if (instruction == 0xCE) // INTO
        {
            if (flag(F_OVERFLOW))
            {
                if (startprinting)
                    cout << "Calling int 4... AX=" << registers[AX] << endl;
                interrupt(4, true);
                cycles_used += 1; //286
            }
            else
            {
                cycles_used += 3; //286
            }
        }
        else if (instruction == 0xCF) // IRET!
        {
            registers[IP] = pop();
            u16 newcs = pop();
            u16 newflags = pop();
            registers[FLAGS] = (newflags&FLAG_MASK) | FLAG_ON;
            if (startprinting)
                cout << "RETURN FROM INTERRUPT to " << newcs << ":" << registers[IP] << "|" << newflags << endl;
            load_segment(SEG::CS, newcs);
            cycles_used += (msw&1)?31:17; //286, TODO: return to lesser privilege, return to different task
        }
        else if (instruction == 0xD0 || instruction == 0xD2)
        {
            bool single_shift = ((instruction&0x02) == 0) || ((registers[CX]&0x1F) == 1);
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            //u8& rm = decode_modrm_u8(modrm);
            u8 rm = readM8();

            u8 inst_type = (modrm>>3)&0x07;
            if (inst_type == 6)
            {
                invalid_instruction(original_ip);
            }
            else
            {

                u8 amount = (single_shift)?1:(registers[CX]&0x1F);

                if ((instruction&0x02) == 0)
                {
                    cycles_used += (modrm_is_register?2:7); //286
                }
                else
                {
                    cycles_used += (modrm_is_register?5:8) + (registers[CX]&0x1F); //286
                }

                for(u32 i=0; i<amount; ++i)
                {
                    //F_OVERFLOW, F_SIGN, F_ZERO, F_AUX_CARRY, F_PARITY, F_CARRY
                    u8 original=rm;
                    u8 result=0;
                    if(false);
                    //ROL ROR RCL RCR SHL SHR (SAL) SAR
                    else if ((inst_type&1) == 0) //ROL RCL SHL SAL
                    {
                        result = (original << 1) | ((inst_type&0x04)?0:((inst_type&0x02) ? flag(F_CARRY) : original>>7));
                    }
                    else if (inst_type == 1 || inst_type == 3) //ROR RCR
                    {
                        result = (original >> 1) | ((inst_type&0x02) ? flag(F_CARRY)<<7 : original<<7);
                    }
                    else if (inst_type == 5 || inst_type == 7) //SHR SAR
                    {
                        result = (original >> 1) | ((inst_type&0x02) ? original&0x80 : 0);
                    }
                    set_flag(F_CARRY, original&((inst_type&1)?0x01:0x80));
                    u8 flag_value = (inst_type&1)?result:original;
                    set_flag(F_OVERFLOW, (bool(flag_value&0x80) != bool(flag_value&0x40)));
                    if (inst_type&0x04)
                    {
                        set_flag(F_SIGN, result&0x80);
                        set_flag(F_ZERO, result==0);
                        set_flag(F_AUX_CARRY, (inst_type==4?result&0x10:false));
                        set_flag(F_PARITY, byte_parity[result&0xFF]);
                    }
                    rm = result;
                }
                writeM8(rm);
            }
        }
        else if (instruction == 0xD1 || instruction == 0xD3)
        {
            bool single_shift = ((instruction&0x02) == 0) || ((registers[CX]&0x1F) == 1);
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            u16 rm = readM16();

            u8 inst_type = (modrm>>3)&0x07;
            if (inst_type == 6)
            {
                invalid_instruction(original_ip);
            }
            else
            {
                u8 amount = (single_shift)?1:(registers[CX]&0x1F);

                if ((instruction&0x02) == 0)
                {
                    cycles_used += (modrm_is_register?2:7); //286
                }
                else
                {
                    cycles_used += (modrm_is_register?5:8) + (registers[CX]&0x1F); //286
                }

                for(u32 i=0; i<amount; ++i)
                {
                    //F_OVERFLOW, F_SIGN, F_ZERO, F_AUX_CARRY, F_PARITY, F_CARRY
                    u16 original=rm;
                    u16 result=0;
                    if (false);
                    //ROL ROR RCL RCR SHL SHR (SAL) SAR
                    else if ((inst_type&1) == 0) //ROL RCL SHL SAL
                    {
                        result = (original << 1) | ((inst_type&0x04)?0:((inst_type&0x02) ? flag(F_CARRY) : original>>15));
                    }
                    else if (inst_type == 1 || inst_type == 3) //ROR RCR
                    {
                        result = (original >> 1) | ((inst_type&0x02) ? flag(F_CARRY)<<15 : original<<15);
                    }
                    else if (inst_type == 5 || inst_type == 7) //SHR SAR
                    {
                        result = (original >> 1) | ((inst_type&0x02) ? original&0x8000 : 0);
                    }
                    set_flag(F_CARRY, original&((inst_type&1)?0x01:0x8000));
                    u16 flag_value = (inst_type&1)?result:original;
                    set_flag(F_OVERFLOW, (bool(flag_value&0x8000) != bool(flag_value&0x4000)));
                    if (inst_type&0x04)
                    {
                        set_flag(F_SIGN, result&0x8000);
                        set_flag(F_ZERO, result==0);
                        set_flag(F_AUX_CARRY, (inst_type==4?result&0x10:false));
                        set_flag(F_PARITY, byte_parity[result&0xFF]);
                    }
                    rm = result;
                }
                writeM16(rm);
            }
        }
        else if (instruction == 0xD4) // AAM
        {
            u8 imm = read_inst<u8>();
            if (imm != 0)
            {
                u8 tempAL = (registers[AX]&0xFF);
                u8 tempAH = tempAL/imm;
                tempAL = tempAL%imm;
                registers[AX] = (tempAH<<8)|tempAL;

                set_flag(F_SIGN, tempAL&0x80);
                set_flag(F_ZERO, tempAL==0);
                set_flag(F_PARITY, byte_parity[tempAL]);
                set_flag(F_OVERFLOW,false);
                set_flag(F_AUX_CARRY,false);
                set_flag(F_CARRY,false);
            }
            else
            {
                set_flag(F_SIGN, false);
                set_flag(F_ZERO, true);
                set_flag(F_PARITY, true);
                set_flag(F_OVERFLOW,false);
                set_flag(F_AUX_CARRY,false);
                set_flag(F_CARRY,false);
                divide_by_zero(original_ip);
            }
            cycles_used += 16; //286
        }
        else if (instruction == 0xD5) // AAD TODO: neaten this code up, also still F_ZERO is wrong sometimes ?!
        {
            u8 imm = read_inst<u8>();
            u16 orig16 = registers[AX];
            u16 temp16 = (registers[AX]&0xFF) + (registers[AX]>>8)*imm;
            registers[AX] = (temp16&0xFF);

            set_flag(F_SIGN,temp16&0x80);
            set_flag(F_ZERO, temp16==0);
            set_flag(F_PARITY, byte_parity[temp16&0xFF]);

            u8 a = orig16;
            u8 b = (orig16>>8)*imm;
            u8 result = a+b;

            set_flag(F_CARRY,result < a); //this is now correct

            bool of = ((a ^ result) & (b ^ result)) & 0x80;
            bool af = ((a ^ b ^ result) & 0x10);
            set_flag(F_OVERFLOW,of);
            set_flag(F_AUX_CARRY,af);
            cycles_used += 14; //286
        }
        else if (instruction == 0xD6) // SALC (undocumented!), doesnt exist on V20, is XLAT. TODO: does it exist on 286?
        {
            get_r8(0) = flag(F_CARRY)?0xFF:0x00;
            cycles_used += 4; //TODO: make sure this SALC instruction exists on 286!
        }
        else if (instruction == 0xD7) // XLAT
        {
            u8 result = mem.r8(get_offset(get_segment(SEG::DS))+registers[BX]+(registers[AX]&0xFF));
            get_r8(0) = result;
            cycles_used += 5; //286
        }
        else if (instruction >= 0xD8 && instruction <= 0xDF)
        {
            //cout << "Trying to run floating point instruction! :(" << endl;
            u8 modrm = read_inst<u8>(); //read modrm data anyway to sync up
            //decode_modrm_u8(modrm);
            decode_modrm(modrm);

            //FLOATING POINT INSTRUCTIONS! 80287! we don't have this. yet?
            cycles_used += 3; //TODO: check that this is right!
        }
        else if ((instruction & 0xFC) == 0xE0) // LOOPNZ LOOPZ LOOP JCXZ
        {
            i8 offset = read_inst<i8>();
            registers[CX] -= ((instruction&0x03)!=3);
            if ((registers[CX] != 0) == ((instruction&0x03) != 3) && (instruction&0x02 ? true:(flag(F_ZERO) == (instruction&0x01))))
            {
                registers[IP] = i16(registers[IP]) + offset;
                cycles_used += 4; //286
            }
            //not taken:  4  4  4  4
            //taken:      8  8  8  8 (delta: 4 4 4 4)
            cycles_used += 4; //286
        }
        else if (instruction == 0xE4) // IN
        {
            get_r8(0) = iosystem.io_in<u8>(read_inst<u8>());
            cycles_used += 5; //286
        }
        else if (instruction == 0xE5) // IN
        {
            u8 port = read_inst<u8>();
            registers[AX] = iosystem.io_in<u16>(port);
            cycles_used += 5; //286
        }
        else if (instruction == 0xE6) // OUT
        {
            iosystem.io_out<u8>(read_inst<u8>(), registers[AX]&0xFF);
            cycles_used += 3; //286
        }
        else if (instruction == 0xE7) // OUT
        {
            u8 port = read_inst<u8>();
            iosystem.io_out<u16>(port, registers[AX]);
            cycles_used += 3; //286
        }
        else if (instruction == 0xE8)
        {
            i16 ip_offset = read_inst<i16>();
            push(registers[IP]);
            registers[IP] = i16(registers[IP])+ip_offset;
            cycles_used += 7; //286
        }
        else if (instruction == 0xE9)
        {
            i16 ip_offset = read_inst<i16>();
            registers[IP] = i16(registers[IP])+ip_offset;
            cycles_used += 7; //286
        }
        else if (instruction == 0xEA) //far jump
        {
            u16 new_ip = read_inst<u16>();
            u16 new_cs = read_inst<u16>();

            registers[IP] = new_ip;
            load_segment(SEG::CS, new_cs);
            cycles_used += (msw&1)?23:11; //286, TODO: missing task stuff
        }
        else if (instruction == 0xEB)
        {
            i8 ip_offset = read_inst<i8>();
            registers[IP] = i16(registers[IP])+ip_offset;
            cycles_used += 7; //286
        }
        else if (instruction == 0xEC) // IN
        {
            get_r8(0) = iosystem.io_in<u8>(registers[DX]);
            cycles_used += 5; //286
        }
        else if (instruction == 0xED) // IN
        {
            u16 port = registers[DX];
            registers[AX] = iosystem.io_in<u16>(port);
            cycles_used += 5; //286
        }
        else if (instruction == 0xEE) // OUT
        {
            iosystem.io_out<u8>(registers[DX], registers[AX]&0xFF);
            cycles_used += 3; //286
        }
        else if (instruction == 0xEF) // OUT
        {
            iosystem.io_out<u16>(registers[DX], registers[AX]);
            cycles_used += 3; //286
        }
        else if (instruction == 0xF1)
        {
            invalid_instruction(original_ip);
        }
        else if (instruction == 0xF4) // HALT / HLT
        {
            halt = true;
            cycles_used += 2; //286
            set_flag(F_INTERRUPT, true);
        }
        else if (instruction == 0xF5) // cmc
        {
            set_flag(F_CARRY, !flag(F_CARRY));
            cycles_used += 2; //286
        }
        else if(instruction == 0xF6) //byte param
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            //u8& rm = decode_modrm_u8(modrm);
            u8 rm = readM8();
            u8 op = ((modrm>>3)&0x07);
            if (op == 0) // TEST
            {
                u8 imm = read_inst<u8>();
                test_flags(u8(rm&imm));
                cycles_used += (modrm_is_register?3:6); //286
            }
            else if (op == 1) //invalid!!!
            {
                invalid_instruction(original_ip);
            }
            else if (op==2) // NOT
            {
                writeM8(~rm);
                cycles_used += (modrm_is_register?2:7); //286
            }
            else if (op == 3) // NEG
            {
                cmp_flags(u8(0),rm,u8(-rm));
                set_flag(F_CARRY,rm!=0);
                writeM8(-rm);
                cycles_used += (modrm_is_register?2:7); //286
            }
            else if (op==4) // MUL
            {
                u8 op2 = registers[AX]&0xFF;
                u16 result = rm*op2;
                set_flag(F_SIGN,result&0x8000);
                set_flag(F_PARITY,byte_parity[result>>8]);
                set_flag(F_OVERFLOW,result&0xFF00);
                set_flag(F_CARRY,result&0xFF00);
                set_flag(F_AUX_CARRY,false);
                set_flag(F_ZERO,(result&0xFF00)==0);
                //set_flag(F_ZERO,true); //V20/V30!
                registers[AX] = result;
                cycles_used += (modrm_is_register?13:16); //286, TODO: accurate?
            }
            else if (op==5) // IMUL
            {
                u8 op2 = registers[AX]&0xFF;
                i16 result = i16(i8(rm))*i16(i8(op2));

                set_flag(F_SIGN,result&0x8000);
                set_flag(F_PARITY,byte_parity[u16(result)>>8]);
                set_flag(F_OVERFLOW,result>=0x80 || result < -0x80);
                set_flag(F_CARRY,result>=0x80 || result < -0x80);
                set_flag(F_AUX_CARRY,false);
                set_flag(F_ZERO,(result&0xFFFF)==0);
                registers[AX] = u16(result);
                cycles_used += (modrm_is_register?13:16); //286, TODO: accurate?
            }
            else if (op==6 || op == 7) //DIV IDIV
            {
                if (rm == 0)
                {
                    divide_by_zero(original_ip);
                }
                else if (op == 6) //DIV
                {
                    //std::cout << "DIVISIO PIQ 1" << std::endl;

                    u8 denominator = rm;
                    u16 result = registers[AX]/denominator;
                    if (result >= 0x100)
                    {
                        set_flag(F_PARITY,false);
                        set_flag(F_SIGN, registers[AX]&0x8000);
                        divide_by_zero(original_ip);
                    }
                    else
                    {
                        u8 quotient = result;
                        u8 remainder = registers[AX] % denominator;
                        registers[AX] = (remainder<<8)|quotient;
                        set_flag(F_PARITY,false);
                        set_flag(F_SIGN, remainder^0x80);
                    }
                    cycles_used += (modrm_is_register?14:17); //286
                }
                else if (op == 7) //IDIV
                {
                    //std::cout << "DIVISIO PIQ 2" << std::endl;

                    i8 denominator = i8(rm);
                    i16 result = i16(registers[AX]) / denominator;
                    if (result < -0x80 || result >= 0x80) //186+ accept -0x80
                    {
                        divide_by_zero(original_ip);
                    }
                    else
                    {
                        i8 quotient = result&0xFF;
                        i8 remainder = i16(registers[AX]) % denominator;
                        registers[AX] = (remainder<<8)|u8(quotient);
                        set_flag(F_PARITY,false);
                    }
                    cycles_used += (modrm_is_register?17:20); //286
                }
            }
        }
        else if(instruction == 0xF7) //word param
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            u16 rm = readM16();
            //u16& rm = decode_modrm_u16(modrm);
            u8 op = ((modrm>>3)&0x07);
            if (op == 0) // TEST
            {
                u16 imm = read_inst<u16>();
                test_flags(u16(rm&imm));
                cycles_used += (modrm_is_register?3:6); //286
            }
            else if (op == 1) //invalid!!!
            {
                invalid_instruction(original_ip);
            }
            else if (op==2) // NOT
            {
                //rm = ~rm;
                writeM16(~rm);
                cycles_used += (modrm_is_register?2:7); //286
            }
            else if (op == 3) // NEG
            {
                cmp_flags(u16(0),rm,u16(-rm));
                set_flag(F_CARRY,rm!=0);
                //rm = -rm;
                writeM16(-rm);
                cycles_used += (modrm_is_register?2:7); //286
            }
            else if (op==4) // MUL
            {
                u16 op2 = registers[AX];
                u32 result = u32(rm)*u32(op2);
                set_flag(F_SIGN,result&0x80000000);
                set_flag(F_PARITY,byte_parity[(result>>16)&0xFF]);
                set_flag(F_OVERFLOW,result&0xFFFF0000);
                set_flag(F_CARRY,result&0xFFFF0000);
                set_flag(F_AUX_CARRY,false);
                set_flag(F_ZERO,(result>>16)==0);

                registers[AX] = result&0xFFFF;
                registers[DX] = result >> 16;
                cycles_used += (modrm_is_register?21:24); //286 TODO: accurate?
            }
            else if (op==5) //IMUL
            {
                u16 op2 = registers[AX];
                i32 result = i32(i16(rm))*i32(i16(op2));
                set_flag(F_SIGN,result&0x80000000);
                set_flag(F_PARITY,byte_parity[(result>>16)&0xFF]);
                set_flag(F_OVERFLOW,result>=0x8000 || result < -0x8000);
                set_flag(F_CARRY,result>=0x8000 || result < -0x8000);
                set_flag(F_AUX_CARRY,false);
                set_flag(F_ZERO,(result)==0);

                registers[AX] = result&0xFFFF;
                registers[DX] = result >> 16;
                cycles_used += (modrm_is_register?22:25); //286 TODO: accurate?
            }
            else if (op == 6 || op == 7) //DIV IDIV
            {
                if (rm == 0)
                {
                    divide_by_zero(original_ip); //division by zero
                }
                else if (op == 6)
                {
                    //std::cout << "DIVISIO" << std::endl;
                    u32 numerator = (registers[DX]<<16)|registers[AX];
                    u16 denominator = rm;
                    u32 result = numerator / denominator;
                    if (result >= 0x10000)
                    {
                        divide_by_zero(original_ip);
                    }
                    else
                    {
                        registers[AX] = result;
                        registers[DX] = numerator % denominator;
                    }
                    cycles_used += (modrm_is_register?25:28); //286, TODO: accurate?
                }
                else if (op == 7)
                {
                    i32 numerator = i32((registers[DX]<<16)|registers[AX]);
                    i16 denominator = i16(rm);
                    i32 result = numerator / denominator;
                    if (result < -0x8000 || result >= 0x8000) //186+ accept -0x8000
                    {
                        divide_by_zero(original_ip);
                    }
                    else
                    {
                        registers[AX] = numerator / denominator;
                        registers[DX] = numerator % denominator;
                    }
                    cycles_used += (modrm_is_register?165:171); //TODO: 165-184, 171-190
                }
            }
        }
        else if ((instruction&0xFE) == 0xF8) //CLC STC carry flag bit 0
        {
            set_flag(F_CARRY, instruction&0x01);
            cycles_used += 2; //286
        }
        else if ((instruction&0xFE) == 0xFA) //CLI STI interrupt flag bit 9
        {
            set_flag(F_INTERRUPT, instruction&0x01);
            cycles_used += 3; //286
        }
        else if ((instruction&0xFE) == 0xFC) //CLD STD direction flag bit 10
        {
            set_flag(F_DIRECTIONAL, instruction&0x01);
            cycles_used += 2; //286
        }
        else if (instruction == 0xFE)
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            u8 reg = readM8();
            u8 op = (modrm>>3)&0x07;
            u8 result = reg+1-(op<<1);
            if (op >= 2)
            {
                invalid_instruction(original_ip);
                cycles_used += 4; //TODO: fix the amount???
            }
            else
            {
                set_flag(F_OVERFLOW,result==0x80-op);
                set_flag(F_AUX_CARRY,(result&0x0F) == ((op&0x01)?0x0F:0x00));
                set_flag(F_ZERO,result==0);
                set_flag(F_SIGN,result&0x80);
                set_flag(F_PARITY,byte_parity[result&0xFF]);
                //no carry!
                //reg = result;
                writeM8(result);
                cycles_used = (modrm_is_register?2:7); //286
            }
        }
        else if (instruction == 0xFF)
        {
            u8 modrm = read_inst<u8>();
            decode_modrm(modrm);
            u8 op = (modrm>>3)&0x07;
            if (op == 0 || op == 1)
            {
                u16 reg = readM16();
                u16 result = reg+1-(op<<1);
                set_flag(F_OVERFLOW,result==(0x8000-op));
                set_flag(F_AUX_CARRY,(result&0x0F) == ((op&0x01)?0x0F:0x00));
                set_flag(F_ZERO,result==0);
                set_flag(F_SIGN,result&0x8000);
                set_flag(F_PARITY,byte_parity[result&0xFF]);
                writeM16(result);
                cycles_used = (modrm_is_register?2:7); //286
            }
            else if (op == 2) //call near
            {
                u16 address = readM16();
                push(registers[IP]);
                registers[IP] = address; //have to do this because reg could be SP :')
                cycles_used += (modrm_is_register?7:11); //286
            }
            else if (op == 3) //call far
            {
                u32 addr = modrm_seg+modrm_offset;
                u16 address = mem.r16(addr);
                u16 segment = mem.r16(addr+2);
                push(descriptor_cache[(int)SEG::CS].data);
                push(registers[IP]);
                load_segment(SEG::CS, segment);
                registers[IP] = address;
                cycles_used += (modrm_is_register?16:29); //286, TODO: protected mode
            }
            else if (op == 4) //jmp near
            {
                registers[IP] = readM16();
                cycles_used += (modrm_is_register?7:11); //286
            }
            else if (op == 5) //jmp far
            {
                u32 addr = modrm_seg+modrm_offset;
                u16 address = mem.r16(addr);
                u16 segment = mem.r16(addr+2);
                registers[IP] = address;
                load_segment(SEG::CS, segment);
                cycles_used += (modrm_is_register?15:26); //286, TODO: protected mode
            }
            else if (op == 6)
            {
                u16 reg = readM16();
                push(reg);
                cycles_used += 5; //286
            }
            else
            {
                invalid_instruction(original_ip);
            }
        }
        else
        {
            //std::cout << "#" << std::dec << cycles << std::hex << ": " << "Executing 0x" << u32(instruction) << " at CS:IP = " << registers[CS] << ":" << registers[IP]-1 << " = " << registers[CS]*16+registers[IP]-1 << std::endl;
            std::cout << "# " << std::dec << cycles << std::hex << ", Unknown opcode: 0x" << u32(instruction) << std::endl;
            //std::abort();
        }

        clear_prefix();

        if (flag(F_INTERRUPT))
        {
            if (interrupt_true_cycles < 2)
                ++interrupt_true_cycles;
        }
        else
        {
            interrupt_true_cycles = 0;
        }

        if (flag(F_TRAP) && previous_trap)
        {
            interrupt(1, true);
        }

        if (cycles_used > 0)
        {
            if (lockstep)
                delay += cycles_used-1;
            else
            {
                cpu_steps += cycles_used -1;
            }
            cycles_used = 0;
        }
        else
        {
            cout << "Instruction without timing: " << u32(instruction) << endl;
            delay += 1;
        }

    }
};


