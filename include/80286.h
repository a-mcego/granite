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


    enum struct ACCESS
    {
        READ,
        WRITE,
        N
    };

    static const u16 FLAG_MASK = 0b0000'1111'1101'0101;
    static const u16 FLAG_ON =   0b0000'0000'0000'0010;

    u32 test_subtype{}; //only used in test mode
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

    enum REG : u8
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
    u16 should_flags{};

    std::string flagstostr(u16 f)
    {
        std::string ret{};
        /*ret += "CF="; */ ret += (f&0x01)?'1':'0'; ret += ' ';
        /*ret += "PF="; */ ret += (f&0x04)?'1':'0'; ret += ' ';
        /*ret += "AF="; */ ret += (f&0x10)?'1':'0'; ret += ' ';
        /*ret += "ZF="; */ ret += (f&0x40)?'1':'0'; ret += ' ';
        /*ret += "SF=";*/  ret += (f&0x80)?'1':'0'; ret += ' ';
        /*ret += "OF="; */ ret += (f&0x800)?'1':'0';
        return ret;
    }

    template<u8 size, ACCESS type>
    void check_segment_access(SEG seg_idx, u16 offset)
    {
        if (u64(seg_idx)>=4)
            throw 13;
        if (u32(offset)+u32(size-1) > u32(descriptor_cache[u64(seg_idx)].limit))
        {
            //std::cout << "offset:" << u32(offset) << "+" << u32(size) << " over limit: " << u32(descriptor_cache[u64(seg_idx)].limit) << std::endl;
            throw 13;
        }
    }


    u8 mem_r8(SEG seg_idx, u16 offset)
    {
        check_segment_access<1,ACCESS::READ>(seg_idx, offset);
        u8 ret = mem.r8(descriptor_cache[u64(seg_idx)].base + offset);
        //std::cout << std::hex << "read8: " << u32(seg_idx) << "=" << descriptor_cache[u64(seg_idx)].base << ":" << offset << ", val=" << (u32)ret << std::endl;
        return ret;
    }

    u16 mem_r16(SEG seg_idx, u16 offset)
    {
        check_segment_access<2,ACCESS::READ>(seg_idx, offset);
        u16 ret = mem.r16(descriptor_cache[u64(seg_idx)].base + offset);
        //std::cout << std::hex << "read16: " << u32(seg_idx) << "=" << descriptor_cache[u64(seg_idx)].base << ":" << offset << ", val=" << ret << std::endl;
        return ret;
    }

    void mem_w8(SEG seg_idx, u16 offset, u8 data)
    {
        check_segment_access<1,ACCESS::WRITE>(seg_idx, offset);
        mem.w8(descriptor_cache[u64(seg_idx)].base + offset, data);
        //std::cout << std::hex << "write8: " << u32(seg_idx) << "=" << descriptor_cache[u64(seg_idx)].base << ":" << offset << ", val=" << (u32)data << std::endl;
    }

    void mem_w16(SEG seg_idx, u16 offset, u16 data)
    {
        check_segment_access<2,ACCESS::WRITE>(seg_idx, offset);
        mem.w16(descriptor_cache[u64(seg_idx)].base + offset, data);
        //std::cout << std::hex << "write16: " << u32(seg_idx) << "=" << descriptor_cache[u64(seg_idx)].base << ":" << offset << ", val=" << data << std::endl;
    }

    void load_segment(SEG segment_number, u16 segment_data)
    {
        if (msw&1) // protected mode
        {
            //if(startprinting)
            //    std::cout << "Load segment from data: " << segment_data << std::endl;
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
                /*if (startprinting)
                {
                    std::cout << names[(int)segment_number] << ": Protected base=" << descriptor_cache[(int)segment_number].base << std::endl;
                    std::cout << "words: " << word1 << " " << word2 << " " << word3 << std::endl;
                    std::cout << "table base: " << table.base << std::endl;

                    std::cout << "first 16 entries: " << std::endl;
                    for(int i=0; i<16; ++i)
                    {
                        u16 entry_word1 = mem_r16(table.base+(i*8));
                        u16 entry_word2 = mem_r16(table.base+(i*8+2));
                        u16 entry_word3 = mem_r16(table.base+(i*8+4));
                        std::cout << "entry " << i << " words: " << entry_word1 << " " << entry_word2 << " " << entry_word3 << std::endl;
                    }
                }*/
            }

        }
        else // real mode
        {
            descriptor_cache[(int)segment_number].base = (segment_data<<4);
            descriptor_cache[(int)segment_number].limit = 0xFFFF;
            descriptor_cache[(int)segment_number].flags = 0;
            descriptor_cache[(int)segment_number].data = segment_data;

            const char* names[4] = {"ES", "CS", "SS", "DS"};
            //if (startprinting)
            //    std::cout << names[(int)segment_number] << ":R" << descriptor_cache[(int)segment_number].base << std::endl;
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
    }

    void finish_flags()
    {
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

    u8 read_bytes{};
    template<typename T>
    T read_inst() requires integral<T>
    {
        read_bytes += sizeof(T);
        if (read_bytes > 10)
        {
            read_bytes = 0;
            throw 13; //286 errata. 286s did 13 here, not 6 like the documentation says.
        }
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

    bool parity(u8 value)
    {
        return !__builtin_parity(value); //TODO: alternative if this builtins isnt available
    }

    template<typename T>
    void cmp_flags(T a, T b, T result) requires std::same_as<T,u8> || std::same_as<T,u16>
    {
        set_flag(F_ZERO, result == 0);
        set_flag(F_SIGN, result >> (sizeof(T)*8-1));
        set_flag(F_PARITY, parity(result));
        set_flag(F_AUX_CARRY, ((a ^ b ^ result) & 0x10) != 0);
        set_flag(F_OVERFLOW, ((a ^ b) & (a ^ result)) >> (sizeof(T)*8-1));
        set_flag(F_CARRY, a < b);
    }
    template<typename T>
    void test_flags(T result) requires std::same_as<T,u8> || std::same_as<T,u16>
    {
        set_flag(F_ZERO, result == 0);
        set_flag(F_SIGN, result >> (sizeof(T)*8-1));
        set_flag(F_PARITY, parity(result));
        set_flag(F_AUX_CARRY, false); //set it false here to pass 0x0A test
        set_flag(F_OVERFLOW, false);
        set_flag(F_CARRY, false);
    }
    template<typename T>
    void add_flags(T a, T b, T result) requires std::same_as<T,u8> || std::same_as<T,u16>
    {
        set_flag(F_ZERO, result == 0);
        set_flag(F_SIGN, result >> (sizeof(T)*8-1));
        set_flag(F_PARITY, parity(result));
        set_flag(F_AUX_CARRY, ((a ^ b ^ result) & 0x10) != 0);
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
    //u32 modrm_seg{};
    SEG modrm_seg_index{};
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
            mem_w8(modrm_seg_index, modrm_offset, (data&0xFF));
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
            ret = mem_r8(modrm_seg_index, modrm_offset);
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
            mem_w16(modrm_seg_index, modrm_offset, data);
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
            ret = mem_r16(modrm_seg_index, modrm_offset);
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
    static constexpr REG regchoice2[8]=
    {
        SI,DI,SI,DI,SI,DI,Z0,Z0,
    };
    static constexpr u8 MODRM_ADVANCE_IP[32] =
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
		if (mod != 0x03)
        {
            //*
            u8 mod_table_index = modrm_reg | (mod<<3);
            modrm_offset = registers[regchoice[mod_table_index]]+registers[regchoice2[modrm_reg]];

            switch(MODRM_ADVANCE_IP[mod_table_index])
            {
            case 1:
                modrm_offset += i16(read_inst<i8>());
                break;
            case 2:
                modrm_offset += read_inst<u16>();
                break;
            }

            SEG segname = (regchoice[mod_table_index]==BP)?SEG::SS:SEG::DS;
            /*/
            if (mod == 0x1)
            {
                modrm_offset = i16(read_inst<i8>());
            }
            else if (mod == 0x2)
            {
                modrm_offset = read_inst<u16>();
            }
            else
            {
                modrm_offset = 0;
            }

            SEG segname = SEG::DS;
            if (mod == 0x00 && modrm_reg == 0x06)
            {
                modrm_offset += read_inst<u16>();
            }
            else
            {
                if (modrm_reg < 0x06)
                {
                    modrm_offset += registers[SI+(modrm_reg&0x01)]; //DI is after SI
                }
                //0 1 7
                if (((modrm_reg+1)&0x07) <= 2)
                {
                    modrm_offset += registers[BX];
                }
                //2 3 6

                if ((modrm_reg&0x02) && modrm_reg != 7)
                {
                    modrm_offset += registers[BP], segname = SEG::SS;
                }
            }

            //*/
            modrm_seg_index = get_segment(segname);
            //modrm_seg = get_offset(get_segment(segname));
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
        std::cout << std::setw(4) << std::setfill('0') << mem__16(registers[SS],registers[SP]) << ' ';
        std::cout << std::setw(4) << std::setfill('0') << mem__16(registers[SS],registers[SP]+2) << ' ';
        std::cout << std::setw(4) << std::setfill('0') << mem__16(registers[SS],registers[SP]+4) << ' ';
        std::cout << std::setw(4) << std::setfill('0') << mem__16(registers[SS],registers[SP]+6) << ' ';*/
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
                set_flag(F_ZERO, out == 0);
                set_flag(F_SIGN, out >> (sizeof(T)*8-1));
                set_flag(F_PARITY, parity(out));
                set_flag(F_AUX_CARRY, (p1&0xF)+(p2&0xF)+flag(F_CARRY) >= 0x10);
                set_flag(F_OVERFLOW, ((p1 ^ out) & (p2 ^ out)) >> (sizeof(T)*8-1));
                set_flag(F_CARRY, ((p1+p2+flag(F_CARRY))>>(sizeof(T)*8)) > 0);
                break;
            case 0x03: //SBB
                out = p1-(p2+flag(F_CARRY));
                set_flag(F_ZERO, out == 0);
                set_flag(F_SIGN, out >> (sizeof(T)*8-1));
                set_flag(F_PARITY, parity(out));
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
            default:
                __builtin_unreachable(); //how to do this with visual studio?
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

    static constexpr bool HAS_MODRM[0x100] =
    {
        1,1,1,1, 0,0,0,0, 1,1,1,1, 0,0,0,0, //00
        1,1,1,1, 0,0,0,0, 1,1,1,1, 0,0,0,0, //10
        1,1,1,1, 0,0,0,0, 1,1,1,1, 0,0,0,0, //20
        1,1,1,1, 0,0,0,0, 1,1,1,1, 0,0,0,0, //30

        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //40
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //50
        0,0,1,0, 0,0,0,0, 0,1,0,1, 0,0,0,0, //60
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //70

        1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, //80
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //90
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //A0
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //B0

        1,1,0,0, 1,1,1,1, 0,0,0,0, 0,0,0,0, //C0
        1,1,1,1, 0,0,0,0, 1,1,1,1, 1,1,1,1, //D0
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, //E0
        0,0,0,0, 0,0,1,1, 0,0,0,0, 0,0,1,1, //F0
    };

    bool set_prefix(u8 instruction)
    {
        if ((instruction & 0b1110'0111) == 0b0010'0110)
        {
            segment_override = SEG((instruction>>3)&0x3);
            return true;
        }
        if ((instruction & 0b1111'1100) == 0b11110000)
        {
            switch(instruction&0x03)
            {
            case 0:
                lock=1; return true;
            case 2:
            case 3:
                string_prefix=(instruction&0x03)-1; return true;
            }
        }
        return false;

        /*switch(IS_PREFIX[instruction])
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
        return bool(IS_PREFIX[instruction]);*/
    }

    u32 interrupt_true_cycles{};
    bool inhibit_ss{};

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
            //if (startprinting)
            //    cout << "INTERRUPT " << u32(n) << " start! orig ip=" << registers[IP] << " protmode=" << (msw&1) << endl;
            halt = false;
            cycles_used += 23; //286

            //++interrupt_table[n];

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

            return !forced;
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
        //std::cout << "-------------------PROTECTION FAULT----------------- at " << full_ip() << std::endl;
        //print_regs();
        //std::abort();
    }

    /*void irq(u8 n)
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
    }*/


    void push(u16 data)
    {
        registers[SP] -= 2;
        mem_w16(SEG::SS, registers[SP], data);
    }
    u16 pop()
    {
        u16 data = mem_r16(SEG::SS, registers[SP]);
        registers[SP] += 2;
        return data;
    }
    void push_with(u16 data, u16& offset)
    {
        offset -= 2;
        mem_w16(SEG::SS, offset, data);
    }
    u16 pop_with(u16& offset)
    {
        u16 data = mem_r16(SEG::SS, offset);
        offset += 2;
        return data;
    }

    bool halt{false};
    //u64 cycles_used{};

    struct CUSTRUCT // temporary construct to disable cycle counting by making it do nothing, the optimizer optimizes it away
    {
        CUSTRUCT& operator+=(int n)
        {
            return *this;
        }

        CUSTRUCT& operator=(int n)
        {
            return *this;
        }

    } cycles_used;

    bool is_inside_multi_part_instruction{};

    u16 original_ip{};

    std::atomic<u64> cyclecounter{};

    void cycle()
    {
        try{
        bool previous_trap = flag(F_TRAP);
        read_bytes = 0;
        ++cyclecounter;

        /*if (delay)
        {
            --delay;
            return;
        }*/

        if (pic.irq_to_cpu != -1)
        {
            CHIP8259& chosen_pic = (pic.irq_to_cpu == 2 && pic2.irq_to_cpu != -1)?pic2:pic;
            if (interrupt(chosen_pic.irq_to_cpu+chosen_pic.vector_pos(), false))
            {
                chosen_pic.cpu_ack_irq();
            }
        }

        if (halt)
        {
            return;
        }

        inhibit_ss = false;

        /*if (get_offset(SEG::CS) == 0 && registers[IP] == 0)
        {
            //cout << "Trying to run code at CS:IP 0:0... resetting." << endl;
            reset();
            //std::abort();
        }*/
        original_ip = registers[IP];

        //is_inside_multi_part_instruction = false;

        globalsettings.current_IP = get_offset(SEG::CS)+registers[IP];
        /*if (globalsettings.current_IP == 0x7C00)
        {
            std::cout << "7C00 gotten!" << std::endl;
            //startprinting = true;
        }*/

        u8 instruction = read_inst<u8>();

        /*if (startprinting && (get_offset(SEG::CS)+registers[IP]-1) < 0xF0000)
        {
            std::cout << (msw&1?"&":"#") << std::dec << cycles << std::hex << ": " << u32(instruction) << " @ " << get_offset(SEG::CS)+registers[IP]-1;
            print_regs();
        }*/

        while (set_prefix(instruction))
        {
            instruction = read_inst<u8>();
            /*if ((startprinting || DEBUG_LEVEL > 1) && (get_offset(SEG::CS)+registers[IP]-1) < 0xF0000)
                std::cout << "prefix read. " << (msw&1?"&":"#") << std::dec << cycles << std::hex << ": " << "Executing 0x" << u32(instruction) << " at CS+IP = " << get_offset(SEG::CS) << ":" << registers[IP]-1 << " = " << get_offset(SEG::CS)+registers[IP]-1 << std::endl;
            */
        }

        if (HAS_MODRM[instruction])
        {
            decode_modrm(read_inst<u8>());
        }

        if (false);
        else if (instruction == 0x0F) // pop cs :-)
        {
            u8 secondbyte = read_inst<u8>();
            decode_modrm(read_inst<u8>());
            u8 op = modrm_r;

            if (false);
            else if (secondbyte == 0x00 && op == 0x00) // SLDT
            {
                mem_w16(modrm_seg_index, modrm_offset, ldtr.n_entries);
                mem_w16(modrm_seg_index, modrm_offset+2, (ldtr.base)&0xFFFF);
                mem_w16(modrm_seg_index, modrm_offset+4, (ldtr.base>>16)&0xFFFF);
                //std::cout << "store ldtr" << std::endl;
            }
            else if (secondbyte == 0x00 && op == 0x02) // LLDT
            {
                //TODO: check cpl
                ldtr.n_entries = mem_r16(modrm_seg_index, modrm_offset);
                ldtr.base = (mem_r16(modrm_seg_index, modrm_offset+2) | (u32(mem_r16(modrm_seg_index, modrm_offset+4))<<16))&0x00FFFFFF; //mask to 24bit max

                //std::cout << "Loaded ldtr with n_entries=0x" << std::hex << ldtr.n_entries << " and base=0x" << ldtr.base << std::endl;
                //startprinting = true;
            }
            else if (secondbyte == 0x00 && op == 0x01) // STR
            {
                mem_w16(modrm_seg_index, modrm_offset, task.data);
                //std::cout << "store task" << std::endl;
            }
            else if (secondbyte == 0x00 && op == 0x03) // LTR
            {
                //TODO: check cpl
                task.data = mem_r16(modrm_seg_index, modrm_offset);
                //std::cout << "load task" << std::endl;
            }
            else if (secondbyte == 0x01 && op == 0x00) // SGDT
            {
                mem_w16(modrm_seg_index, modrm_offset, gdtr.n_entries);
                mem_w16(modrm_seg_index, modrm_offset+2, (gdtr.base)&0xFFFF);
                mem_w16(modrm_seg_index, modrm_offset+4, (gdtr.base>>16)&0xFFFF);
                //std::cout << "store gdtr" << std::endl;
                //startprinting = true;
            }
            else if (secondbyte == 0x01 && op == 0x02) // LGDT
            {
                //TODO: check cpl
                gdtr.n_entries = mem_r16(modrm_seg_index, modrm_offset);
                gdtr.base = (mem_r16(modrm_seg_index, modrm_offset+2) | (u32(mem_r16(modrm_seg_index, modrm_offset+4))<<16))&0x00FFFFFF; //mask to 24bit max

                //std::cout << "Loaded gdtr with n_entries=0x" << std::hex << gdtr.n_entries << " and base=0x" << gdtr.base << std::endl;
                //startprinting = true;
            }
            else if (secondbyte == 0x01 && op == 0x01) // SIDT
            {
                //TODO: check cpl
                mem_w16(modrm_seg_index, modrm_offset, idtr.n_entries);
                mem_w16(modrm_seg_index, modrm_offset+2, (idtr.base)&0xFFFF);
                mem_w16(modrm_seg_index, modrm_offset+4, (idtr.base>>16)&0xFFFF);
                //std::cout << "store idtr" << std::endl;
            }
            else if (secondbyte == 0x01 && op == 0x03) // LIDT
            {
                //TODO: check cpl
                idtr.n_entries = mem_r16(modrm_seg_index, modrm_offset);
                u16 val1 = mem_r16(modrm_seg_index, modrm_offset+2);
                u16 val2 = mem_r16(modrm_seg_index, modrm_offset+4);
                idtr.base = (val1 | (u32(val2)<<16))&0x00FFFFFF; //mask to 24bit max

                //std::cout << "Loaded idtr from fulladdr=" << addr << " with n_entries=0x" << std::hex << idtr.n_entries << " and base=0x" << idtr.base << std::endl;
            }
            else if (secondbyte == 0x01 && op == 0x06) // LMSW
            {
                msw = (msw & 0xFFF1) | (readM16() & 0x000F);
                //std::cout << "New msw: " << msw << std::endl;

                /*std::cout << std::dec << gdtr.n_entries << " global entries." << std::endl;
                std::cout << std::hex;
                for(int i=0; i<gdtr.n_entries; ++i)
                {
                    std::cout << u16(mem_direct8(gdtr.base + i*8)) << " ";
                    std::cout << u16(mem_direct8(gdtr.base + i*8 +1)) << "       ";
                    std::cout << u16(mem_direct8(gdtr.base + i*8 +2)) << " ";
                    std::cout << u16(mem_direct8(gdtr.base + i*8 +3)) << " ";
                    std::cout << u16(mem_direct8(gdtr.base + i*8 +4)) << "       ";
                    std::cout << u16(mem_direct8(gdtr.base + i*8 +5)) << " ";
                    std::cout << std::endl;
                }*/
            }
            else if (secondbyte == 0x01 && op == 0x04) // SMSW
            {
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
                u16 tempSP = registers[SP];

                tempSP -= 2;
                mem_w16(SEG::SS, tempSP, registers[AX]);
                tempSP -= 2;
                mem_w16(SEG::SS, tempSP, registers[CX]);
                tempSP -= 2;
                mem_w16(SEG::SS, tempSP, registers[DX]);
                tempSP -= 2;
                mem_w16(SEG::SS, tempSP, registers[BX]);
                tempSP -= 2;
                mem_w16(SEG::SS, tempSP, registers[SP]);
                tempSP -= 2;
                mem_w16(SEG::SS, tempSP, registers[BP]);
                tempSP -= 2;
                mem_w16(SEG::SS, tempSP, registers[SI]);
                tempSP -= 2;
                mem_w16(SEG::SS, tempSP, registers[DI]);

                registers[SP] = tempSP;
                cycles_used += 17; //286
            }
            else if (instruction == 0x61)
            {
                //0x61 popa //pops all 8 regs
                u16 tempSP = registers[SP];
                check_segment_access<2,ACCESS::READ>(SEG::SS,tempSP+0x0);
                //check_segment_access<2,ACCESS::READ>(SEG::SS,tempSP+0x2);
                //check_segment_access<2,ACCESS::READ>(SEG::SS,tempSP+0x4);
                //check_segment_access<2,ACCESS::READ>(SEG::SS,tempSP+0x6);
                //check_segment_access<2,ACCESS::READ>(SEG::SS,tempSP+0x8);
                //check_segment_access<2,ACCESS::READ>(SEG::SS,tempSP+0xA);
                //check_segment_access<2,ACCESS::READ>(SEG::SS,tempSP+0xC);
                check_segment_access<2,ACCESS::READ>(SEG::SS,tempSP+0xE);


                registers[DI] = mem_r16(SEG::SS, tempSP);
                tempSP += 2;
                registers[SI] = mem_r16(SEG::SS, tempSP);
                tempSP += 2;
                registers[BP] = mem_r16(SEG::SS, tempSP);
                tempSP += 2;
                registers[SP] = mem_r16(SEG::SS, tempSP);
                tempSP += 2;
                registers[BX] = mem_r16(SEG::SS, tempSP);
                tempSP += 2;
                registers[DX] = mem_r16(SEG::SS, tempSP);
                tempSP += 2;
                registers[CX] = mem_r16(SEG::SS, tempSP);
                tempSP += 2;
                registers[AX] = mem_r16(SEG::SS, tempSP);
                tempSP += 2;

                registers[SP] = tempSP;

                cycles_used += 19; //286
            }
            else if (instruction == 0x62) // BOUND
            {
                //u8 modrm = read_inst<u8>();
                //decode_modrm(modrm);
                if (modrm_is_register)
                    throw 6;
                u16 r = get_r16(modrm_r);
                u16 rm = readM16();
                i16 lower_bound = rm;
                i16 upper_bound = mem_r16(modrm_seg_index, modrm_offset + 2);//mem_r16(modrm_seg_index, rm + 2);

                if (i16(r) < lower_bound || i16(r) > upper_bound)
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
                u16 rm = readM16();
                u16 op2 = read_inst<u16>();
                i32 result = i16(rm)*i16(op2);

                set_flag(F_CARRY,result>=0x8000 || result < -0x8000);
                set_flag(F_PARITY,parity(result>>16));
                set_flag(F_AUX_CARRY,true);
                set_flag(F_ZERO,(result&0xFFFF0000)==0);
                set_flag(F_SIGN,result&0x80000000);
                set_flag(F_OVERFLOW,result>=0x8000 || result < -0x8000);

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
                u16 rm = readM16();
                u16 op2 = i16(read_inst<i8>());
                i32 result = i16(rm)*i16(op2);

                set_flag(F_CARRY,result>=0x8000 || result < -0x8000);
                set_flag(F_PARITY,parity(result>>16));
                set_flag(F_AUX_CARRY,true);
                set_flag(F_ZERO,(result&0xFFFF0000)==0);
                set_flag(F_SIGN,result&0x80000000);
                set_flag(F_OVERFLOW,result>=0x8000 || result < -0x8000);

                get_r16(modrm_r) = result;

                cycles_used += (modrm_is_register?21:24); //286
            }
            else if (instruction == 0x6C) // INS, byte from DX port
            {
                u16 port = registers[DX];
                if (string_prefix == 0)
                {
                    mem_w8(SEG::ES, registers[DI], iosystem.io_in<u8>(port));
                    registers[DI] += flag(F_DIRECTIONAL) ? -1 : 1;
                    cycles_used += 5; //286
                }
                else
                {
                    while (registers[CX] != 0)
                    {
                        mem_w8(SEG::ES, registers[DI], iosystem.io_in<u8>(port));
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
                    u16 oldDI = registers[DI];
                    registers[DI] += flag(F_DIRECTIONAL) ? -2 : 2;
                    mem_w16(SEG::ES, oldDI, data);
                    cycles_used += 5; //286
                }
                else
                {
                    while (registers[CX] != 0)
                    {
                        u16 oldDI = registers[DI];
                        registers[DI] += flag(F_DIRECTIONAL) ? -2 : 2;
                        registers[CX] -= 1;
                        u16 data = iosystem.io_in<u16>(port);
                        registers[CX] -= 1;
                        mem_w16(SEG::ES, oldDI, data);
                        registers[CX] += 1;
                        cycles_used += 5; //286
                    }
                }
            }
            else if (instruction == 0x6E) // OUTS, byte to DX port
            {
                u16 port = registers[DX];
                if (string_prefix == 0)
                {
                    u8 data = mem_r8(get_segment(SEG::DS), registers[SI]);
                    iosystem.io_out<u8>(port, data);
                    registers[SI] += flag(F_DIRECTIONAL) ? -1 : 1;
                    cycles_used += 5; //286
                }
                else
                {
                    while (registers[CX] != 0)
                    {
                        u8 data = mem_r8(get_segment(SEG::DS), registers[SI]);
                        iosystem.io_out<u8>(port, data);
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
                    u16 oldSI = registers[SI];
                    registers[SI] += flag(F_DIRECTIONAL) ? -2 : 2;
                    u16 value = mem_r16(get_segment(SEG::DS), oldSI);
                    iosystem.io_out<u16>(port, value);
                    cycles_used += 5; //286
                }
                else
                {
                    while (registers[CX] != 0)
                    {
                        u16 oldSI = registers[SI];
                        registers[SI] += flag(F_DIRECTIONAL) ? -2 : 2;
                        registers[CX] -= 1;
                        u16 value = mem_r16(get_segment(SEG::DS), oldSI);
                        iosystem.io_out<u16>(port, value);
                        cycles_used += 5; //286
                    }
                }
            }
            else
            {
                invalid_instruction(original_ip);
            }
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
                if (instruction&0x01)//16bit
                {
                    u16 rm = readM16();
                    u16& r = get_r16(modrm_r);
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
                    u8& r = get_r8(modrm_r);
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
                //if (startprinting)
                //    std::cout << "push seg " << u32(seg_n) << " = " << descriptor_cache[seg_n].data << std::endl;
                push(descriptor_cache[seg_n].data);
            }
        }
        else if ((instruction&0xF0) == 0x40) //INC/DEC register, don't change carry!
        {
            u16 result = registers[instruction&0x07]+(1-((instruction&0x08)?2:0));
            set_flag(F_SIGN, result&0x8000);
            set_flag(F_ZERO, result==0);
            set_flag(F_AUX_CARRY, (result&0x0F) == ((instruction&0x08)?0x0F:0x00));
            set_flag(F_PARITY, parity(result));
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
        else if ((instruction&0xF0) == 0x80)
        {
            if (instruction == 0x80 || instruction == 0x82)
            {
                //u8 modrm = read_inst<u8>();
                //decode_modrm(modrm);
                u8 rm = readM8();
                u8 imm = read_inst<u8>();
                rm = run_arith(rm, imm, modrm_r);
                writeM8(rm);
                cycles_used += (modrm_is_register?3:7); //286
            }
            else if (instruction == 0x81)
            {
                //u8 modrm = read_inst<u8>();
                //decode_modrm(modrm);
                u16 rm = readM16();
                u16 imm = read_inst<u16>();
                rm = run_arith(rm, imm, modrm_r);
                writeM16(rm);
                cycles_used += (modrm_is_register?3:7); //286
            }
            else if (instruction == 0x83)
            {
                //u8 modrm = read_inst<u8>();
                //decode_modrm(modrm);
                u16 rm = readM16();
                u16 imm = i16(read_inst<i8>());
                rm = run_arith(rm, imm, modrm_r);
                writeM16(rm);
                cycles_used += (modrm_is_register?3:7); //286
            }
            else if (instruction == 0x84) //TEST
            {
                //u8 modrm = read_inst<u8>();
                //decode_modrm(modrm);
                u8 rm = readM8();
                u8 r = get_r8(modrm_r);
                test_flags(u8(rm&r));
                cycles_used += (modrm_is_register?2:6); //286
            }
            else if (instruction == 0x85) //TEST
            {
                //u8 modrm = read_inst<u8>();
                //decode_modrm(modrm);
                u16 rm = readM16();
                u16 r = get_r16(modrm_r);
                test_flags(u16(rm&r));
                cycles_used += (modrm_is_register?2:6); //286
            }
            else if (instruction == 0x86) //XCHG
            {
                //u8 modrm = read_inst<u8>();
                //decode_modrm(modrm);
                u8 rm = readM8();
                u8& r = get_r8(modrm_r);
                u8 temp = rm;
                rm = r;
                r = temp;
                writeM8(rm);
                cycles_used += (modrm_is_register?3:5); //286
            }
            else if (instruction == 0x87) //XCHG
            {
                //u8 modrm = read_inst<u8>();
                //decode_modrm(modrm);
                u16 rm = readM16();
                u16& r = get_r16(modrm_r);
                u16 temp = rm;
                rm = r;
                r = temp;
                writeM16(rm);
                cycles_used += (modrm_is_register?3:5); //286
            }
            else if ((instruction&0xFC) == 0x88) // MOV EbGb, EvGv, GbEb, GvEv
            {
                //u8 modrm = read_inst<u8>();
                //decode_modrm(modrm);
                if (instruction&0x01)//16bit
                {
                    u16& r = get_r16(modrm_r);
                    if (instruction&0x02) // towards general register
                        r = readM16();
                    else
                        writeM16(r);
                }
                else//8bit
                {
                    u8& r = get_r8(modrm_r);
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
                //u8 modrm = read_inst<u8>();
                //decode_modrm(modrm);
                if (modrm_r < 4)
                {
                    writeM16(descriptor_cache[modrm_r].data);
                }
                else
                {
                    invalid_instruction(original_ip);
                }
                cycles_used += (modrm_is_register?2:3); //286
            }
            else if (instruction == 0x8E) // MOV SwEw
            {
                //u8 modrm = read_inst<u8>();
                //decode_modrm(modrm);
                u8 seg_n = modrm_r;
                if(seg_n >= 4 || seg_n == 1) //valid segments are ES SS DS.
                {
                    throw 6;
                }

                u16 rm = readM16();
                if (msw&1) //protected mode
                {
                    if (seg_n == 1) //CS
                    {
                        throw 13;
                    }
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
                if (modrm_is_register)
                    throw 6;
                u16& r = get_r16(modrm_r);
                r = modrm_offset;
                cycles_used += 3; //286
            }
            else if (instruction == 0x8F) //POP modrm
            {
                if (modrm_r != 0) // !?! this makes the 286 tests pass but..
                    throw 6;
                writeM16(pop());
                cycles_used += 5; //286
            }
        }
        else if ((instruction&0xF8) == 0x90) // XCHG AX, r16 - note how 0x90 is effectively NOP :-)
        {
            u8 reg_id = instruction&0x7;
            u16 tmp = registers[AX];
            registers[AX] = registers[reg_id];
            registers[reg_id] = tmp;
            cycles_used += 3; //286
        }
        else if ((instruction&0xF8) == 0x98)
        {
            if (instruction == 0x98) //CBW
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
        }

        else if (instruction >= 0xA0 && instruction <= 0xA3) //AL/X=MEM  MEM=AL/X
        {
            SEG source_segment = get_segment(SEG::DS);
            u16 source_offset = read_inst<u16>();
            switch(instruction)
            {
                case 0xA0: get_r8(0) = mem_r8(source_segment, source_offset); break;
                case 0xA1: registers[AX] = mem_r16(source_segment, source_offset); break;
                case 0xA2: mem_w8(source_segment, source_offset, get_r8(0)); break;
                case 0xA3: mem_w16(source_segment, source_offset, registers[AX]); break;
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
        else if ((instruction&0xF0) == 0xA0) //MOVSB/W CMPSB/W --- STOSB/W LODSB/W SCASB/W
        {
            i16 size = (instruction&0x01)+1;
            i16 direction = flag(F_DIRECTIONAL)?-size:size;
            if (registers[CX] != 0 || string_prefix == 0) do
            {
                u16 value1{}, value2{};
                u16 oldSI = registers[SI];
                u16 oldDI = registers[DI];

                if (string_prefix != 0)
                    registers[CX] -= 1;

                switch(instruction)
                {
                case 0xA4: //MOVSB 0x4021
                    registers[SI] += direction;
                    value1 = mem_r8(get_segment(SEG::DS), oldSI);
                    registers[DI] += direction;
                    mem_w8(SEG::ES, oldDI, value1);
                    break;
                case 0xA5: //MOVSW 0x4021
                    registers[SI] += direction;
                    value1 = mem_r16(get_segment(SEG::DS), oldSI);

                    registers[DI] += direction;
                    if (string_prefix != 0)
                        registers[CX] -= 1;
                    mem_w16(SEG::ES, oldDI, value1);
                    if (string_prefix != 0)
                        registers[CX] += 1;
                    break;
                case 0xA6: //CMPSB 0x8103
                    registers[SI] += direction;
                    value1 = mem_r8(get_segment(SEG::DS), oldSI);
                    registers[DI] += direction;
                    value2 = mem_r8(SEG::ES, oldDI);
                    cmp_flags<u8>(value1,value2,u8(value1-value2));
                    break;
                case 0xA7: //CMPSW 0x8103
                    registers[DI] += direction;
                    if (string_prefix != 0)
                        registers[CX] += 1;
                    value2 = mem_r16(SEG::ES, oldDI);
                    if (string_prefix != 0)
                        registers[CX] -= 1;

                    registers[SI] += direction;
                    value1 = mem_r16(get_segment(SEG::DS), oldSI);
                    cmp_flags<u16>(value1,value2,u16(value1-value2));
                    break;
                case 0xAA: //STOSB 0x4024
                    value1 = get_r8(0);
                    registers[DI] += direction;
                    mem_w8(SEG::ES, oldDI, value1);
                    break;
                case 0xAB: //STOSW 0x4024
                    value1 = registers[AX];
                    registers[DI] += direction;
                    if (string_prefix != 0)
                        registers[CX] -= 1;
                    mem_w16(SEG::ES, oldDI, value1);
                    if (string_prefix != 0)
                        registers[CX] += 1;
                    break;
                case 0xAC: //LODSB 0x4041
                    registers[SI] += direction;
                    value1 = mem_r8(get_segment(SEG::DS), oldSI);
                    get_r8(0) = value1;
                    break;
                case 0xAD: //LODSW 0x4041
                    registers[SI] += direction;
                    value1 = mem_r16(get_segment(SEG::DS), oldSI);
                    registers[AX] = value1;
                    break;
                case 0xAE: //SCASB 0x8106
                    registers[DI] += direction;
                    value2 = mem_r8(SEG::ES, oldDI);
                    value1 = get_r8(0);
                    cmp_flags<u8>(value1,value2,u8(value1-value2));
                    break;
                case 0xAF: //SCASW 0x8106
                    registers[DI] += direction;
                    value2 = mem_r16(SEG::ES, oldDI);
                    value1 = registers[AX];
                    cmp_flags<u16>(value1,value2,u16(value1-value2));
                    break;
                }
                if (string_prefix == 0)
                {
                    cycles_used += 5;
                    break;
                }
                cycles_used += 5;
                if ((instruction&0x6) == 0x6)
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
        else if ((instruction&0xF0) == 0xC0)
        {
            if (instruction == 0xC0)
            {
                u16 rm = readM8(); //yes u16, we need room for carry in the RCx instructions
                u8 amount = read_inst<u8>() & 0x1F; // Only the lower 5 bits are used for the shift count
                bool zeroamount = amount==0;

                if (!zeroamount)
                {
                    u8 inst_type = modrm_r;
                    cycles_used += (modrm_is_register ? 5 : 8) + amount; //286

                    //std::cout << std::hex << u16(rm) << std::dec << " inst=" << u16(inst_type) << " amount=" << u16(amount) << std::endl;
                    switch(inst_type)
                    {
                    case 0: //ROL
                        amount &= 7;
                        rm = (rm<<amount)|(rm>>(8-amount));
                        set_flag(F_OVERFLOW, (bool(rm & 0x1) != bool(rm & 0x80)));
                        set_flag(F_CARRY, rm&0x1);
                        break;
                    case 1: //ROR
                        amount &= 7;
                        rm = (rm>>amount)|(rm<<(8-amount));
                        set_flag(F_OVERFLOW, (bool(rm & 0x40) != bool(rm & 0x80)));
                        set_flag(F_CARRY, rm&0x80);
                        break;
                    case 2: //RCL
                        amount %= 9;
                        rm |= flag(F_CARRY)?0x100:0x000;
                        rm = (rm<<amount)|(rm>>(9-amount));
                        set_flag(F_OVERFLOW, (bool(rm & 0x80) != bool(rm & 0x100)));
                        set_flag(F_CARRY, rm&0x100);
                        break;
                    case 3: //RCR
                        amount %= 9;
                        rm |= flag(F_CARRY)?0x100:0x000;
                        rm = (rm>>amount)|(rm<<(9-amount));
                        set_flag(F_OVERFLOW, (bool(rm & 0x80) != bool(rm & 0x40)));
                        set_flag(F_CARRY, rm&0x100);
                        break;
                    case 4: //SHL
                    case 6: //SAL
                        rm <<= amount;
                        set_flag(F_OVERFLOW, (bool(rm & 0x80) != bool(rm & 0x100)));
                        set_flag(F_CARRY, rm&0x100);
                        set_flag(F_AUX_CARRY, rm&0x10);
                        break;
                    case 5: //SHR
                        set_flag(F_CARRY, (rm>>(amount-1))&1);
                        set_flag(F_OVERFLOW, (rm>>amount)&0x40);
                        set_flag(F_AUX_CARRY, true);
                        rm >>= amount;
                        break;
                    case 7: //SAR
                        set_flag(F_CARRY, (i16(i8(rm))>>i8(amount-1))&1);
                        set_flag(F_OVERFLOW, false);
                        set_flag(F_AUX_CARRY, true);
                        rm = i8(rm)>>i8(amount);
                        break;
                    }

                    if (inst_type >= 4)
                    {
                        set_flag(F_SIGN, rm&0x80);
                        set_flag(F_ZERO, u8(rm)==0);
                        set_flag(F_PARITY, parity(rm));
                    }
                    writeM8(u8(rm));
                }
            }
            else if (instruction == 0xC1)
            {
                u32 rm = readM16(); //yes u32, we need space for carry in RCx instructions
                u8 amount = read_inst<u8>() & 0x1F; // Only the lower 5 bits are used for the shift count

                bool zeroamount = amount==0;

                if (!zeroamount)
                {
                    u8 inst_type = modrm_r;
                    cycles_used += (modrm_is_register ? 5 : 8) + amount; //286

                    //std::cout << std::hex << u16(rm) << std::dec << " inst=" << u16(inst_type) << " amount=" << u16(amount) << std::endl;
                    switch(inst_type)
                    {
                    case 0: //ROL
                        amount &= 15;
                        rm = (rm<<amount)|(rm>>(16-amount));
                        set_flag(F_OVERFLOW, (bool(rm & 0x1) != bool(rm & 0x8000)));
                        set_flag(F_CARRY, rm&0x1);
                        break;
                    case 1: //ROR
                        amount &= 15;
                        rm = (rm>>amount)|(rm<<(16-amount));
                        set_flag(F_OVERFLOW, (bool(rm & 0x4000) != bool(rm & 0x8000)));
                        set_flag(F_CARRY, rm&0x8000);
                        break;
                    case 2: //RCL
                        amount %= 17;
                        rm |= flag(F_CARRY)?0x10000:0x00000;
                        rm = (rm<<amount)|(rm>>(17-amount));
                        set_flag(F_OVERFLOW, (bool(rm & 0x8000) != bool(rm & 0x10000)));
                        set_flag(F_CARRY, rm&0x10000);
                        break;
                    case 3: //RCR
                        amount %= 17;
                        rm |= flag(F_CARRY)?0x10000:0x00000;
                        rm = (rm>>amount)|(rm<<(17-amount));
                        set_flag(F_OVERFLOW, (bool(rm & 0x8000) != bool(rm & 0x4000)));
                        set_flag(F_CARRY, rm&0x10000);
                        break;
                    case 4: //SHL
                    case 6: //SAL
                        rm <<= amount;
                        set_flag(F_OVERFLOW, (bool(rm & 0x8000) != bool(rm & 0x10000)));
                        set_flag(F_CARRY, rm&0x10000);
                        set_flag(F_AUX_CARRY, rm&0x10);
                        break;
                    case 5: //SHR
                        set_flag(F_CARRY, (rm>>(amount-1))&1);
                        set_flag(F_OVERFLOW, (rm>>amount)&0x4000);
                        set_flag(F_AUX_CARRY, true);
                        rm >>= amount;
                        break;
                    case 7: //SAR
                        set_flag(F_CARRY, (i32(i16(rm))>>i8(amount-1))&1);
                        set_flag(F_OVERFLOW, false);
                        set_flag(F_AUX_CARRY, true);
                        rm = i16(rm)>>i8(amount);
                        break;
                    }

                    if (inst_type >= 4)
                    {
                        set_flag(F_SIGN, rm&0x8000);
                        set_flag(F_ZERO, u16(rm)==0);
                        set_flag(F_PARITY, parity(rm));
                    }
                    writeM16(u16(rm));
                }
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
            else if ((instruction&0xFE) == 0xC4) // LES LDS
            {
                if (modrm_is_register)
                    throw 6;
                u16& r = get_r16(modrm_r);
                r = mem_r16(modrm_seg_index, modrm_offset);
                u16 segvalue = mem_r16(modrm_seg_index, modrm_offset+2);
                load_segment((instruction&1)?SEG::DS:SEG::ES, segvalue); //ES or DS, based on the opcode
                cycles_used += (msw&1)?21:7; //286
            }
            else if (instruction == 0xC6) //MOV
            {
                u8 data = read_inst<u8>();
                if (modrm_r != 0)
                    throw 6;
                writeM8(data);
                cycles_used += (modrm_is_register?2:3); //286
            }
            else if (instruction == 0xC7) //MOV
            {
                u16 data = read_inst<u16>();
                if (modrm_r != 0)
                    throw 6;
                writeM16(data);
                cycles_used += (modrm_is_register?2:3); //286
            }
            else if (instruction == 0xC8)
            {
                //0xC8 ENTER data16, imm8
                u16 frame_size = read_inst<u16>();
                u8 nesting_level = read_inst<u8>()&0x1F;

                u16 tempSP = registers[SP];
                push_with(registers[BP],tempSP);
                u16 tempBP = registers[BP];
                u16 frame_temp = tempSP;
                if (nesting_level > 0)
                {
                    u16 last_write=tempSP;
                    for (u8 i = 1; i < nesting_level; ++i)
                    {
                        tempBP -= 2;
                        u16 value = mem_r16(SEG::SS, tempBP);
                        if (last_write == tempBP) //pass MOO tests with this. weird thing tho. like a bus error. this might be wrong! or a CPU bug!
                            value &= 0xFF00;
                        push_with(value,tempSP);
                        last_write = tempSP;
                    }
                    push_with(frame_temp,tempSP);
                }
                registers[BP] = frame_temp;
                registers[SP] = tempSP-frame_size;
                cycles_used += 11 + 4 * nesting_level + (nesting_level>1?1:0); //286
            }
            else if (instruction == 0xC9)
            {
                //0xC9 LEAVE
                u16 newSP = registers[BP];
                registers[BP] = pop_with(newSP);
                registers[SP] = newSP;
                cycles_used += 5; //286
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
            else if (instruction == 0xCC) // INT 3
            {
                //if (startprinting)
                //    cout << "Calling interrupt 3... AX=" << registers[AX] << endl;
                interrupt(3, true);
            }
            else if (instruction == 0xCD) // INT imm8
            {
                u8 int_num = read_inst<u8>();
                //if (startprinting)
                //    cout << "Calling interrupt... " << u32(int_num) << " AX=" << registers[AX] << endl;
                interrupt(int_num, true);
            }
            else if (instruction == 0xCE) // INTO
            {
                if (flag(F_OVERFLOW))
                {
                    //if (startprinting)
                    //    cout << "Calling int 4... AX=" << registers[AX] << endl;
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
                //if (startprinting)
                //    cout << "RETURN FROM INTERRUPT to " << newcs << ":" << registers[IP] << "|" << newflags << endl;
                load_segment(SEG::CS, newcs);
                cycles_used += (msw&1)?31:17; //286, TODO: return to lesser privilege, return to different task
            }
        }
        else if ((instruction & 0xFD) == 0xD0)
        {
            bool single_shift = (instruction&0x02) == 0;
            u16 rm = readM8(); //space for rcl carry bit

            u8 inst_type = modrm_r;

            u8 amount = (single_shift)?1:(registers[CX]&0x1F);

            if (single_shift)
            {
                cycles_used += (modrm_is_register?2:7); //286
            }
            else
            {
                cycles_used += (modrm_is_register?5:8) + (registers[CX]&0x1F); //286
            }
            //*
            bool zeroamount = amount==0;
            if (!zeroamount)
            {
                cycles_used += (modrm_is_register ? 5 : 8) + amount; //286

                //std::cout << std::hex << u16(rm) << std::dec << " inst=" << u16(inst_type) << " amount=" << u16(amount) << std::endl;
                switch(inst_type)
                {
                case 0: //ROL
                    amount &= 7;
                    rm = (rm<<amount)|(rm>>(8-amount));
                    set_flag(F_OVERFLOW, (bool(rm & 0x1) != bool(rm & 0x80)));
                    set_flag(F_CARRY, rm&0x1);
                    break;
                case 1: //ROR
                    amount &= 7;
                    rm = (rm>>amount)|(rm<<(8-amount));
                    set_flag(F_OVERFLOW, (bool(rm & 0x40) != bool(rm & 0x80)));
                    set_flag(F_CARRY, rm&0x80);
                    break;
                case 2: //RCL
                    amount %= 9;
                    rm |= flag(F_CARRY)?0x100:0x000;
                    rm = (rm<<amount)|(rm>>(9-amount));
                    set_flag(F_OVERFLOW, (bool(rm & 0x80) != bool(rm & 0x100)));
                    set_flag(F_CARRY, rm&0x100);
                    break;
                case 3: //RCR
                    amount %= 9;
                    rm |= flag(F_CARRY)?0x100:0x000;
                    rm = (rm>>amount)|(rm<<(9-amount));
                    set_flag(F_OVERFLOW, (bool(rm & 0x80) != bool(rm & 0x40)));
                    set_flag(F_CARRY, rm&0x100);
                    break;
                case 4: //SHL
                case 6: //SAL
                    rm <<= amount;
                    set_flag(F_OVERFLOW, (bool(rm & 0x80) != bool(rm & 0x100)));
                    set_flag(F_CARRY, rm&0x100);
                    set_flag(F_AUX_CARRY, rm&0x10);
                    break;
                case 5: //SHR
                    set_flag(F_CARRY, (rm>>(amount-1))&1);
                    set_flag(F_OVERFLOW, (rm>>amount)&0x40);
                    set_flag(F_AUX_CARRY, true);
                    rm >>= amount;
                    break;
                case 7: //SAR
                    set_flag(F_CARRY, (i16(i8(rm))>>i8(amount-1))&1);
                    set_flag(F_OVERFLOW, false);
                    set_flag(F_AUX_CARRY, true);
                    rm = i8(rm)>>i8(amount);
                    break;
                }

                if (inst_type >= 4)
                {
                    set_flag(F_SIGN, rm&0x80);
                    set_flag(F_ZERO, u8(rm)==0);
                    set_flag(F_PARITY, parity(rm));
                }
                writeM8(u8(rm));
            }

            /*/

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
                    set_flag(F_PARITY, parity(result));
                }
                rm = result;
            }
            //*/
            writeM8(rm);
        }
        else if ((instruction & 0xFD) == 0xD1)
        {
            bool single_shift = ((instruction&0x02) == 0) || ((registers[CX]&0x1F) == 1);
            u32 rm = readM16(); //space for rcl carry bit

            u8 inst_type = modrm_r;
            u8 amount = (single_shift)?1:(registers[CX]&0x1F);

            if ((instruction&0x02) == 0)
            {
                cycles_used += (modrm_is_register?2:7); //286
            }
            else
            {
                cycles_used += (modrm_is_register?5:8) + (registers[CX]&0x1F); //286
            }
            bool zeroamount = amount==0;
            if (!zeroamount)
            {
                cycles_used += (modrm_is_register ? 5 : 8) + amount; //286

                //std::cout << std::hex << u16(rm) << std::dec << " inst=" << u16(inst_type) << " amount=" << u16(amount) << std::endl;
                switch(inst_type)
                {
                case 0: //ROL
                    amount &= 15;
                    rm = (rm<<amount)|(rm>>(16-amount));
                    set_flag(F_OVERFLOW, (bool(rm & 0x1) != bool(rm & 0x8000)));
                    set_flag(F_CARRY, rm&0x1);
                    break;
                case 1: //ROR
                    amount &= 15;
                    rm = (rm>>amount)|(rm<<(16-amount));
                    set_flag(F_OVERFLOW, (bool(rm & 0x4000) != bool(rm & 0x8000)));
                    set_flag(F_CARRY, rm&0x8000);
                    break;
                case 2: //RCL
                    amount %= 17;
                    rm |= flag(F_CARRY)?0x10000:0x00000;
                    rm = (rm<<amount)|(rm>>(17-amount));
                    set_flag(F_OVERFLOW, (bool(rm & 0x8000) != bool(rm & 0x10000)));
                    set_flag(F_CARRY, rm&0x10000);
                    break;
                case 3: //RCR
                    amount %= 17;
                    rm |= flag(F_CARRY)?0x10000:0x00000;
                    rm = (rm>>amount)|(rm<<(17-amount));
                    set_flag(F_OVERFLOW, (bool(rm & 0x8000) != bool(rm & 0x4000)));
                    set_flag(F_CARRY, rm&0x10000);
                    break;
                case 4: //SHL
                case 6: //SAL
                    rm <<= amount;
                    set_flag(F_OVERFLOW, (bool(rm & 0x8000) != bool(rm & 0x10000)));
                    set_flag(F_CARRY, rm&0x10000);
                    set_flag(F_AUX_CARRY, rm&0x10);
                    break;
                case 5: //SHR
                    set_flag(F_CARRY, (rm>>(amount-1))&1);
                    set_flag(F_OVERFLOW, (rm>>amount)&0x4000);
                    set_flag(F_AUX_CARRY, true);
                    rm >>= amount;
                    break;
                case 7: //SAR
                    set_flag(F_CARRY, (i32(i16(rm))>>i8(amount-1))&1);
                    set_flag(F_OVERFLOW, false);
                    set_flag(F_AUX_CARRY, true);
                    rm = i16(rm)>>i8(amount);
                    break;
                }

                if (inst_type >= 4)
                {
                    set_flag(F_SIGN, rm&0x8000);
                    set_flag(F_ZERO, u16(rm)==0);
                    set_flag(F_PARITY, parity(rm));
                }
                writeM16(u16(rm));
            }

            /*for(u32 i=0; i<amount; ++i)
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
                    set_flag(F_PARITY, parity(result));
                }
                rm = result;
            }*/
            writeM16(rm);
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
                set_flag(F_PARITY, parity(tempAL));
                set_flag(F_OVERFLOW,false);
                set_flag(F_AUX_CARRY,false);
                set_flag(F_CARRY,false);
            }
            else
            {
                set_flag(F_SIGN, false);
                set_flag(F_ZERO, false);
                set_flag(F_PARITY, !parity(registers[CX]>>8));// !CH is an experimental value, it's the only register that worked in cow tests
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
            u8 origAL = registers[AX];
            u8 mult_result = (registers[AX]>>8)*imm;
            u8 result = origAL + mult_result;
            registers[AX] = result;

            set_flag(F_SIGN, result&0x80);
            set_flag(F_ZERO, result==0);
            set_flag(F_PARITY, parity(result));
            set_flag(F_CARRY, result < origAL); //this is now correct
            set_flag(F_OVERFLOW, result < origAL);
            set_flag(F_AUX_CARRY, ((origAL ^ mult_result ^ result) & 0x10));
            cycles_used += 14; //286
        }
        else if (instruction == 0xD6) // SALC (undocumented!), doesnt exist on V20, is XLAT. does it exist on 286? yes!
        {
            get_r8(0) = flag(F_CARRY)?0xFF:0x00;
            cycles_used += 4; //TODO: make sure this SALC instruction exists on 286!
        }
        else if (instruction == 0xD7) // XLAT
        {
            u8 result = mem_r8(get_segment(SEG::DS), registers[BX]+(registers[AX]&0xFF));
            get_r8(0) = result;
            cycles_used += 5; //286
        }
        else if ((instruction&0xF8) == 0xD8)
        {
            check_segment_access<2,ACCESS::READ>(modrm_seg_index, modrm_offset);
            //cout << "Trying to run floating point instruction! :(" << endl;
            //u8 modrm = read_inst<u8>(); //read modrm data anyway to sync up
            //decode_modrm_u8(modrm);
            //decode_modrm(modrm);

            //FLOATING POINT INSTRUCTIONS! 80287! we don't have this. yet?
            cycles_used += 3; //TODO: check that this is right!
        }
        if ((instruction&0xF0) == 0xE0)
        {
            if ((instruction & 0xFC) == 0xE0) // LOOPNZ LOOPZ LOOP JCXZ
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
        }
        else if (instruction == 0xF1)
        {
            invalid_instruction(original_ip);
        }
        else if (instruction == 0xF4) // HALT / HLT
        {
            halt = true;
            cycles_used += 2; //286
            //set_flag(F_INTERRUPT, true);
        }
        else if (instruction == 0xF5) // cmc
        {
            set_flag(F_CARRY, !flag(F_CARRY));
            cycles_used += 2; //286
        }
        else if(instruction == 0xF6) //byte param
        {
            //u8 modrm = read_inst<u8>();
            //decode_modrm(modrm);
            //u8& rm = decode_modrm_u8(modrm);
            u8 rm = readM8();
            u8 op = modrm_r;
            if (op == 0 || op == 1) // TEST
            {
                u8 imm = read_inst<u8>();
                test_flags(u8(rm&imm));
                cycles_used += (modrm_is_register?3:6); //286
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
                set_flag(F_PARITY,parity(result>>8));
                set_flag(F_OVERFLOW,result&0xFF00);
                set_flag(F_CARRY,result&0xFF00);
                set_flag(F_AUX_CARRY,true);
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
                set_flag(F_PARITY,parity(result>>8));
                set_flag(F_OVERFLOW,result>=0x80 || result < -0x80);
                set_flag(F_CARRY,result>=0x80 || result < -0x80);
                set_flag(F_AUX_CARRY,true);
                set_flag(F_ZERO,(result&0xFF00)==0);
                registers[AX] = u16(result);
                cycles_used += (modrm_is_register?13:16); //286, TODO: accurate?
            }
            else if (op==6 || op == 7) //DIV IDIV
            {
                bool sign1 = false, sign2 = false;

                auto divN = [this, &sign1,&sign2]<size_t N>(u16 ax, u8 divisor, bool carry) -> void
                {
                    u16 div16 = divisor<<8;
                    bool q_overflow = (ax >= div16);

                    std::stringstream ss;
                    ss << "debug: ";
                    ss << "sgn=" <<sign1 <<sign2<< " ";

                    for(int x=0; x<N; ++x)
                    {
                        ss << std::hex << std::setw(4) << std::setfill('0') << ax << " ";
                        cmp_flags<u8>((ax>>8), divisor, (ax>>8)-divisor);
                        if (ax >= div16 || carry)
                        {
                            ax -= div16;
                            ax |= 1;
                        }
                        carry = ax&0x8000;
                        ax<<=1;
                    }
                    if (q_overflow)
                    {
                        //cmp_flags<u8>((ax>>8), divisor, (ax>>8)-divisor);

                        u8 al = registers[AX]&0xFF;
                        test_subtype = (divisor==0)?2:1;
                        //cmp_flags<u8>(al,divisor,al-divisor);
                        //set_flag(F_AUX_CARRY,true); //only for idiv

                        ss << std::setw(4) << std::setfill('0') << ax;
                        ss << " origAX=" << std::setw(4) << std::setfill('0') << registers[AX];
                        ss << " divisor=" << std::setw(2) << u16(divisor) << std::setw(4);
                        ss << " flags=" << (registers[FLAGS]&0x8D5) << " should=" << (should_flags&0x8D5);
                        //if (should_flags&0x40)
                        //    std::cout << ss.str() << std::endl;

                        throw 0;
                    }
                    //DIV5 microcode op sets these three flags?
                    //they seem illogical but pass tests.
                    set_flag(F_CARRY,     u8(ax>>8)<divisor);
                    set_flag(F_OVERFLOW,  u8(ax>>8)<divisor);
                    set_flag(F_AUX_CARRY, true);
                    if (ax >= div16 || carry)
                    {
                        ax -= div16;
                        ax |= 1;
                    }
                    set_flag(F_PARITY, parity(ax>>8));
                    set_flag(F_ZERO, u8(ax>>8) == 0);
                    set_flag(F_SIGN, ax&0x8000);
                    registers[AX] = ax;
                };
                auto idivN = [this]<size_t N>(u16 ax, u8 divisor, bool printthings) -> void
                {
                    bool sgn1 = false, sgn2 = false;
                    registers[AX] = ax;
                    i8 denominator = divisor;
                    i16 oldAX = ax;

                    //std::cout << i16(registers[AX]) << "/" << i16(i8(rm)) << " = ";
                    if (denominator<0)
                    {
                        denominator = -denominator-1;
                        sgn1=!sgn1;
                    }
                    i16 absax = oldAX;
                    if (absax<0)
                    {
                        absax = -absax-2+divisor;
                        sgn2 = true;
                        sgn1=!sgn1;
                    }
                    absax <<= 1;
                    //bool carry = (absax&0x8000);
                    bool carry = false;

                    u16 div16 = denominator<<8;
                    bool q_overflow = (u16(absax) >= div16);
                    ax = absax;

                    std::stringstream ss;
                    ss << "ax=" << std::hex << oldAX;
                    ss << " s=" <<sgn1 <<sgn2<< " ";

                    for(int x=0; x<7; ++x)
                    {
                        ss  << x << ":" << std::hex << std::setw(4) << std::setfill('0') << ax << " ";
                        cmp_flags<u8>((ax>>8), divisor, (ax>>8)-divisor);
                        if (ax >= div16 || carry)
                        {
                            ax -= div16;
                            ax += 1;
                        }
                        carry = ax&0x4000;
                        ax<<=1;
                    }
                    if (q_overflow)
                    {
                        ss << "X:" << std::hex << std::setw(4) << std::setfill('0') << ax << " ";
                        auto bx = ax;
                        if (ax >= div16 || carry)
                        {
                            bx -= div16;
                            bx += 1;
                            ss << "BX:" << std::hex << std::setw(4) << std::setfill('0') << bx << " ";
                        }
                        else
                        {
                            ss << "------- ";
                        }
                        //bx <<= 1;
                        u8 quotient = bx;
                        u8 remainder = bx>>8;
                        u8 cmpl = remainder;//-1-divisor;
                        u8 cmpr = divisor;
                        u8 cmplr = cmpl-cmpr;
                        cmp_flags<u8>(cmpl, cmpr, cmplr);
                        if (sgn1)
                        {
                            quotient = -quotient;
                            bx = (remainder<<8)|quotient;
                        }
                        if (sgn2)
                        {
                            quotient = bx;
                            remainder = bx>>8;

                            cmpl = remainder+1-divisor;
                            cmpr = divisor;
                            cmplr = cmpl-cmpr;
                            set_flag(F_AUX_CARRY, ((cmpl ^ cmpr ^ cmplr) & 0x10) != 0);
                            set_flag(F_OVERFLOW, ((cmpl ^ cmpr) & (cmpl ^ cmplr)) >> 7);
                            set_flag(F_SIGN, !((cmpl-cmpr)&0x80));
                            remainder = -remainder;
                            set_flag(F_PARITY, parity(remainder-divisor+1));
                            set_flag(F_ZERO, remainder == 0);
                        }
                        ax = (remainder<<8)|quotient;
                        registers[AX] = ax;

                        test_subtype = (divisor==0)?2:1;

                        ss << " flags=" << (registers[FLAGS]&0x8D5) << " should=" << (should_flags&0x8D5) << " err=" << ((should_flags&0x8D5)^(registers[FLAGS]&0x8D5));
                        if (printthings)
                            std::cout << ss.str() << std::endl;

                        return;
                    }
                    if (ax >= div16 || carry)
                    {
                        ax -= div16;
                        ax |= 1;
                    }
                    cmp_flags<u8>((ax>>8), divisor, (ax>>8)-divisor);

                    //DIV5 microcode op sets these three flags?
                    //they seem illogical but pass tests.
                    set_flag(F_CARRY,    sgn1==sgn2);
                    //set_flag(F_OVERFLOW, sgn1==sgn2);
                    set_flag(F_AUX_CARRY, true);

                    u8 quotient = ax;
                    u8 remainder = ax>>8;
                    if (sgn1)
                    {
                        quotient = -quotient;
                    }
                    if (sgn2)
                    {
                        remainder = -remainder;
                        set_flag(F_PARITY, parity(remainder));
                        set_flag(F_ZERO, remainder == 0);
                        set_flag(F_SIGN, remainder&0x80);
                    }

                    //cmp_flags<u8>((ax>>8), divisor, (ax>>8)-divisor);
                    //set_flag(F_CARRY,     u8(ax>>8)<divisor);
                    //set_flag(F_OVERFLOW,  u8(ax>>8)<divisor);
                    //set_flag(F_AUX_CARRY, true);
                    /*set_flag(F_PARITY, parity(ax>>8));
                    set_flag(F_ZERO, u8(ax>>8) == 0);
                    set_flag(F_SIGN, ax&0x8000);*/
                    registers[AX] = (remainder<<8)|quotient;
                    ss << std::setw(4) << std::setfill('0') << ax;
                    ss << " origAX=" << std::setw(4) << std::setfill('0') << oldAX;
                    ss << " divisor=" << std::setw(2) << u16(divisor) << std::setw(4);
                    ss << " AX=" << std::setw(4) << std::setfill('0') << ax;
                    ss << " flags=" << (registers[FLAGS]&0x8D5) << " should=" << (should_flags&0x8D5) << " err=" << ((should_flags&0x8D5)^(registers[FLAGS]&0x8D5));
                    if (printthings)
                        std::cout << ss.str() << std::endl;
                };

                if (op == 6) //DIV
                {
                    cycles_used += (modrm_is_register?14:17); //286
                    divN.operator()<8>(registers[AX], rm, false);
                }
                else if (op == 7) //IDIV
                {
                    int divider = 3; //------------------------------------------------------------------------------------
                    std::cin >> divider;

                    std::stringstream sstr;
                    sstr << "idiv" << divider << ".bin";

                    int errors = 0;
                    FILE* filu = fopen(sstr.str().c_str(),"rb");
                    for(int ax_value=0x0; ax_value<0x10000; ++ax_value)
                    {
                        //std::cout << "-------------------------------------" << std::endl;
                        //std::cout << std::setfill('0');
                        /*if (ax_value%0x10 == 0)
                            std::cout << std::endl;
                        if (ax_value%0x40 == 0)*/
                        should_flags = 0;
                        fseek(filu,ax_value*2,SEEK_SET);
                        fread(&should_flags,2,1,filu);
                        idivN.operator()<7>(ax_value, divider, false);
                        const u16 ARITH_FLAG_MASK = 0x8D5;
                        if ((registers[FLAGS]&ARITH_FLAG_MASK) != (should_flags&ARITH_FLAG_MASK))
                        {
                            if ((registers[FLAGS]&ARITH_FLAG_MASK) != (should_flags&ARITH_FLAG_MASK))
                            {
                                ++errors;
                            }
                            idivN.operator()<7>(ax_value, divider, true);
                            //std::cout << std::hex << "ax=" << std::setw(4) << std::setfill(' ') << u16(ax_value) << " ";
                            //std::cout << "ax>>1=" << std::setw(2) << std::setfill(' ') << u16(u8(currax>>1)) << " ";
                            //std::cout << std::hex << std::setw(3) << std::hex << (registers[FLAGS]&ARITH_FLAG_MASK);
                            if ((registers[FLAGS]&ARITH_FLAG_MASK) != (should_flags&ARITH_FLAG_MASK))
                            {
                                //std::cout << " shouldbe " << std::setw(3) << (should_flags&ARITH_FLAG_MASK);
                            }
                            //std::cout << std::endl;
                        }

                    }
                    std::cout << std::dec << errors << " errors."<< std::endl;
                    fclose(filu);
                    std::abort();

                    //std::cout << "DIVISIO PIQ 2" << std::endl;

                    /*i8 denominator = i8(rm);
                    i16 oldAX = registers[AX];

                    //std::cout << i16(registers[AX]) << "/" << i16(i8(rm)) << " = ";
                    if (denominator<0)
                    {
                        denominator = -denominator;
                        sign1=!sign1;
                    }
                    i16 absax = registers[AX];
                    if (absax<0)
                    {
                        absax = -absax;
                        sign2 = true;
                        sign1=!sign1;
                    }
                    bool carry = (absax&0x8000);
                    absax <<= 1;
                    divN.operator()<7>(absax, denominator, false);
                    test_subtype = 0;

                    u8 quotient = registers[AX];
                    u8 remainder = registers[AX]>>8;

                    set_flag(F_CARRY,    sign1==sign2);
                    set_flag(F_OVERFLOW, sign1==sign2);
                    set_flag(F_AUX_CARRY, true);

                    if (sign1)
                    {
                        quotient = -quotient;
                    }
                    if (sign2)
                    {
                        remainder = -remainder;
                        set_flag(F_PARITY, parity(remainder));
                        set_flag(F_ZERO, remainder == 0);
                        set_flag(F_SIGN, remainder&0x80);
                    }

                    registers[AX] = (remainder<<8)|quotient;
                    cycles_used += (modrm_is_register?17:20); //286*/
                }
            }
        }
        else if(instruction == 0xF7) //word param
        {
            //u8 modrm = read_inst<u8>();
            //decode_modrm(modrm);
            u16 rm = readM16();
            //u16& rm = decode_modrm_u16(modrm);
            u8 op = modrm_r;
            if (op == 0 || op == 1) // TEST
            {
                u16 imm = read_inst<u16>();
                test_flags(u16(rm&imm));
                cycles_used += (modrm_is_register?3:6); //286
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
                set_flag(F_PARITY,parity(result>>16));
                set_flag(F_OVERFLOW,result&0xFFFF0000);
                set_flag(F_CARRY,result&0xFFFF0000);
                set_flag(F_AUX_CARRY,true);
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
                set_flag(F_PARITY,parity(result>>16));
                set_flag(F_OVERFLOW,result>=0x8000 || result < -0x8000);
                set_flag(F_CARRY,result>=0x8000 || result < -0x8000);
                set_flag(F_AUX_CARRY,true);
                set_flag(F_ZERO,(result>>16)==0);

                registers[AX] = result&0xFFFF;
                registers[DX] = result >> 16;
                cycles_used += (modrm_is_register?22:25); //286 TODO: accurate?
            }
            else if (op == 6 || op == 7) //DIV IDIV
            {
                if (rm == 0)
                {
                    throw 0;
                }
                if (op == 6)
                {
                    //std::cout << "DIVISIO" << std::endl;
                    u32 numerator = (registers[DX]<<16)|registers[AX];
                    u16 denominator = rm;
                    u32 result = numerator / denominator;
                    if (result >= 0x10000)
                    {
                        throw 0;
                    }
                    registers[AX] = result;
                    registers[DX] = numerator % denominator;
                    cycles_used += (modrm_is_register?25:28); //286, TODO: accurate?
                }
                else if (op == 7)
                {
                    i32 numerator = i32((registers[DX]<<16)|registers[AX]);
                    i16 denominator = i16(rm);
                    i32 result = numerator / denominator;
                    if (result < -0x8000 || result >= 0x8000) //186+ accept -0x8000
                    {
                        throw 0;
                    }
                    registers[AX] = numerator / denominator;
                    registers[DX] = numerator % denominator;
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
            //u8 modrm = read_inst<u8>();
            //decode_modrm(modrm);
            u8 reg = readM8();
            u8 op = modrm_r;
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
                set_flag(F_PARITY,parity(result));
                //no carry!
                //reg = result;
                writeM8(result);
                cycles_used = (modrm_is_register?2:7); //286
            }
        }
        else if (instruction == 0xFF)
        {
            //u8 modrm = read_inst<u8>();
            //decode_modrm(modrm);
            u8 op = modrm_r;
            if (op == 0 || op == 1)
            {
                u16 reg = readM16();
                u16 result = reg+1-(op<<1);
                set_flag(F_OVERFLOW,result==(0x8000-op));
                set_flag(F_AUX_CARRY,(result&0x0F) == ((op&0x01)?0x0F:0x00));
                set_flag(F_ZERO,result==0);
                set_flag(F_SIGN,result&0x8000);
                set_flag(F_PARITY,parity(result));
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
                if (modrm_is_register)
                    throw 6;
                u16 address = mem_r16(modrm_seg_index,modrm_offset);
                u16 segment = mem_r16(modrm_seg_index,modrm_offset+2);
                u16 old_segment = descriptor_cache[(int)SEG::CS].data;
                push(old_segment);
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
                if (modrm_is_register)
                    throw 6;
                //u32 addr = modrm_seg+modrm_offset;
                u16 address = mem_r16(modrm_seg_index,modrm_offset);
                u16 segment = mem_r16(modrm_seg_index,modrm_offset+2);
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
        else if (instruction == 0x27) // DAA
        {
            u8 old_AL = registers[AX]&0xFF;

            u8 added{};

            set_flag(F_AUX_CARRY, (registers[AX] & 0x0F) > 9 || flag(F_AUX_CARRY));
            if (flag(F_AUX_CARRY))
                added += 0x06;

            set_flag(F_CARRY, old_AL > 0x99 || flag(F_CARRY));
            if (flag(F_CARRY))
                added += 0x60;

            get_r8(0) += added;

            set_flag(F_ZERO, (registers[AX]&0xFF) == 0);
            set_flag(F_SIGN, (registers[AX] & 0x80));
            set_flag(F_PARITY, parity(registers[AX]));
            set_flag(F_OVERFLOW, (old_AL ^ registers[AX]) & (added ^ registers[AX])&0x80);
            cycles_used += 3; //286
        }
        else if (instruction == 0x37) // AAA
        {
            u16 old_AX = registers[AX];
            bool add_ax = (registers[AX] & 0x0F) > 9 || flag(F_AUX_CARRY);
            if (add_ax)
            {
                registers[AX] += 0x106;
            }
            u16 added = registers[AX]-old_AX;

            set_flag(F_AUX_CARRY, add_ax);
            set_flag(F_CARRY, add_ax);
            set_flag(F_PARITY, parity(registers[AX]));
            set_flag(F_ZERO, (registers[AX]&0xFF) == 0);
            set_flag(F_SIGN, (registers[AX] & 0x80));
            set_flag(F_OVERFLOW, (old_AX ^ registers[AX]) & (added ^ registers[AX])&0x80);

            get_r8(0) &= 0x0F;
            cycles_used += 3; //286
        }
        else if (instruction == 0x2F) // DAS
        {
            u8 old_AL = registers[AX] & 0xFF;

            u8 subtracted{};

            bool sub_al = ((registers[AX] & 0x0F) > 9 || flag(F_AUX_CARRY));
            if (sub_al)
                subtracted += 0x06;

            set_flag(F_AUX_CARRY, sub_al);
            bool sub_al2 = (old_AL > 0x99 || flag(F_CARRY));
            if (sub_al2)
                subtracted += 0x60;

            get_r8(0) -= subtracted;
            set_flag(F_CARRY, sub_al2 | (sub_al & bool(old_AL<6)));
            set_flag(F_ZERO, (registers[AX] & 0xFF) == 0);
            set_flag(F_SIGN, (registers[AX] & 0x80));
            set_flag(F_PARITY, parity(registers[AX]));
            set_flag(F_OVERFLOW, ((old_AL ^ subtracted) & (old_AL ^ registers[AX]))&0x80);

            cycles_used += 3; //286
        }
        else if (instruction == 0x3F) // AAS
        {
            u16 old_AX = registers[AX];
            bool sub_ax = (registers[AX] & 0x0F) > 9 || flag(F_AUX_CARRY);
            if (sub_ax)
            {
                registers[AX] -= 0x106;
            }
            u16 subtracted = old_AX-registers[AX];
            set_flag(F_AUX_CARRY, sub_ax);
            set_flag(F_CARRY, sub_ax);
            set_flag(F_ZERO, (registers[AX] & 0xFF) == 0);
            set_flag(F_SIGN, (registers[AX] & 0x80));
            set_flag(F_PARITY, parity(registers[AX]));
            set_flag(F_OVERFLOW, (old_AX ^ subtracted) & (old_AX ^ registers[AX])&0x80);

            get_r8(0) &= 0x0F;
            cycles_used += 3; //286
        }
        else
        {
            //std::cout << "#" << std::dec << cycles << std::hex << ": " << "Executing 0x" << u32(instruction) << " at CS:IP = " << registers[CS] << ":" << registers[IP]-1 << " = " << registers[CS]*16+registers[IP]-1 << std::endl;
            //std::cout << "# " << std::dec << cycles << std::hex << ", Unknown opcode: 0x" << u32(instruction) << std::endl;
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

        /*if (cycles_used > 0)
        {
            if (lockstep)
                delay += cycles_used-1;
            else
            {
                cpu_steps += cycles_used -1;
            }
            cycles_used = 0;
        }*/
        /*else
        {
            cout << "Instruction without timing: " << u32(instruction) << endl;
            delay += 1;
        }*/
        }
        catch(int exception)
        {
            registers[IP] = original_ip;
            interrupt(exception, true, exception==13, 0);

            /*if (exception == 6)
            {
                invalid_instruction(original_ip,0);
            }
            protection_fault(original_ip, 0);*/
            //interrupt(exception, true);
        }
    }
};


#undef mem_r8
#undef mem_r16
#undef mem_w8
#undef mem_w16
