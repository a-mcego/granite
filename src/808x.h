#pragma once


struct CPU8088
{
    u16 registers[16] = {};

    u8 segment_override{};
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
        AX,CX,DX,BX, SP,BP,SI,DI, //normal registers
        ES,CS,SS,DS,              //segment registers
        FLAGS,                    //flags, duh
        IP                        //instruction pointer
    };

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

    void set_flag(FLAG f_n, bool value)   { registers[FLAGS] = (registers[FLAGS]&~(1<<f_n))|(value?1<<f_n:0); }

    bool flag(FLAG f_n)
    {
        return registers[FLAGS]&(1<<f_n);
    }

    void reset()
    {
        halt = false;
        clear_prefix();
        for(u32 i=0; i<16; ++i)
            registers[i] = 0x0000;
        registers[CS] = ~registers[CS]; //set code segment to 0xFFFF for reset
    }


    static const u32 PREFETCH_QUEUE_SIZE = 4;
    u8 prefetch_queue[PREFETCH_QUEUE_SIZE] = {};
    u32 prefetch_address{};

    bool do_prefetch_delay{};
    template<typename T>
    T read_inst() requires integral<T>
    {
        if constexpr(PREFETCH_QUEUE_SIZE == 0)
        {
            u32 position = ((registers[CS]<<4) + registers[IP])&0xFFFFF;
            T data = *(T*)(mem.memory_bytes+position);
            registers[IP] += sizeof(T);
            return data;
        }
        T result{};
        u32 position = ((registers[CS]<<4) + registers[IP])&0xFFFFF;
        if (prefetch_address != position)
        {
            prefetch_address = position;
            for(u32 i=0; i<PREFETCH_QUEUE_SIZE; ++i)
            {
                prefetch_queue[i] = mem._8(registers[CS], registers[IP]+i);
            }
        }

        result = *(T*)(prefetch_queue);
        prefetch_address += sizeof(T);
        registers[IP] += sizeof(T);

        for(u32 i=0; i<PREFETCH_QUEUE_SIZE-sizeof(T); ++i)
        {
            prefetch_queue[i] = prefetch_queue[i+sizeof(T)];
        }
        for(u32 i=PREFETCH_QUEUE_SIZE-sizeof(T); i<PREFETCH_QUEUE_SIZE; ++i)
        {
            prefetch_queue[i] = mem._8(registers[CS], registers[IP]+i);
        }
        if (startprinting)
        {
            //cout << (sizeof(T)==2?"w":"b") << u32(result) << " ";
        }
        do_prefetch_delay = !do_prefetch_delay;
        cycles_used += (do_prefetch_delay?4*sizeof(T):0);
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

    const u8 effective_address_cycles[32] =
    {
         7, 8, 8, 7, 5, 5, 6, 5,
        11,12,12,11, 9, 9, 9, 9,
        11,12,12,11, 9, 9, 9, 9,
         0, 0, 0, 0, 0, 0, 0, 0, //reg
    };
    bool modrm_is_register{};
    void decode_modrm(u8 mod, u8 rm, u16& segment, u16& offset)
    {
        offset = 0;
        segment = DS;

		if (mod == 0x1)
        {
			offset = i16(read_inst<i8>());
        }
		else if (mod == 0x2)
        {
			offset = read_inst<u16>();
        }

        cycles_used += effective_address_cycles[(mod<<3)+rm];

		if (mod == 0x00 && rm == 0x06)
        {
			offset += read_inst<u16>();
        }
		else
		{
			if (rm < 0x06)
            {
				offset += registers[SI+(rm&0x01)]; //DI is after SI
            }
			if (((rm+1)&0x07) <= 2)
            {
				offset += registers[BX];
            }
			if ((rm&0x02) && rm != 7)
            {
				offset += registers[BP], segment = SS;
            }
		}
		cycles_used = ((offset&0x01)<<2); //4 cycles for odd accesses
        segment = registers[get_segment(segment)];
    }

    u8& decode_modrm_u8(u8 modrm)
    {
        u8 mod = (modrm >> 6) & 0x03;
        u8 rm = modrm & 0x07;
        modrm_is_register = (mod==0x03);
		if (mod == 0x03)
        {
			return get_r8(rm);
        }
        u16 offset{}, segment{};
        decode_modrm(mod,rm,segment,offset);
        if constexpr (DEBUG_LEVEL > 1)
            cout << "MEM8! " << segment << ":" << offset << " has " << u32(mem._8(segment, offset)) << " prm=" << u32(mod) << "," << u32(rm) << endl;
        return mem._8(segment, offset);
    }

    u16& decode_modrm_u16(u8 modrm)
    {
        u8 mod = (modrm >> 6) & 0x03;
        u8 rm = modrm & 0x07;
        modrm_is_register = (mod==0x03);
		if (mod == 0x03)
        {
			return get_r16(rm);
        }
        u16 offset{}, segment{};
        decode_modrm(mod,rm,segment,offset);
        if constexpr (DEBUG_LEVEL > 1)
            cout << "MEM16! " << segment << ":" << offset << " has " << mem._16(segment, offset) << " prm=" << u32(mod) << "," << u32(rm) << endl;
        return mem._16(segment, offset);
    }

    u16 effective_address(u8 modrm)
    {
        u8 mod = (modrm >> 6) & 0x03;
        u8 rm = modrm & 0x07;
        modrm_is_register = (mod==0x03);
		if (mod == 0x03)
        {
            cout << "Loading effective address of a register? are you gone mad?" << endl;
            std::abort();
        }
        u16 offset{}, segment{};
        decode_modrm(mod,rm,segment,offset);
        if constexpr (DEBUG_LEVEL > 1)
            cout << "LEA! " << segment << ":" << offset << " has " << mem._16(segment, offset) << " prm=" << u32(mod) << "," << u32(rm) << endl;
        return offset;
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

    u16& get_segment_r16(u8 value)
    {
        return registers[(value&0x03)+8]; //&0x03 for 8086 compatibility
    }

    void print_regs()
    {
        for(int i=0; i<8; ++i)
            std::cout << " " << r16_names[i] << "=" << std::setw(4) << std::setfill('0') << registers[i];
        for(int i=0; i<4; ++i)
            std::cout << " " << seg_names[i] << "=" << std::setw(4) << std::setfill('0') << registers[i+8];
        std::cout << " FL=" << std::setw(4) << std::setfill('0') << registers[FLAGS] << " IP=" << std::setw(4) << std::setfill('0') << registers[IP]-1;

        std::cout << " S ";
        std::cout << std::setw(4) << std::setfill('0') << mem._16(registers[SS],registers[SP]) << ' ';
        std::cout << std::setw(4) << std::setfill('0') << mem._16(registers[SS],registers[SP]+2) << ' ';
        std::cout << std::setw(4) << std::setfill('0') << mem._16(registers[SS],registers[SP]+4) << ' ';
        std::cout << std::setw(4) << std::setfill('0') << mem._16(registers[SS],registers[SP]+6) << ' ';
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
            case 0x02: //ADC
                out = p1+p2+flag(F_CARRY);
                commonflags(p1,p2,out);
                set_flag(F_AUX_CARRY, (p1&0xF)+(p2&0xF)+flag(F_CARRY) >= 0x10);
                set_flag(F_OVERFLOW, ((p1 ^ out) & (p2 ^ out)) >> (sizeof(T)*8-1));
                set_flag(F_CARRY, ((p1+p2+flag(F_CARRY))>>(sizeof(T)*8)) > 0);
                break;
            case 0x01: //OR
                out = p1|p2;
                break;
            case 0x04: //AND
                out = p1&p2;
                break;
            case 0x03: //SBB
                out = p1-(p2+flag(F_CARRY));
                commonflags(p1,p2,out);
                set_flag(F_AUX_CARRY, (p1&0xF)-((p2&0xF)+flag(F_CARRY)) < 0x00);
                set_flag(F_OVERFLOW, ((p1 ^ p2) & (p1 ^ out)) >> (sizeof(T)*8-1));
                set_flag(F_CARRY, ((p1-(p2+flag(F_CARRY)))>>(sizeof(T)*8)) < 0);
                break;
            case 0x05: //SUB
            case 0x07: //CMP
                out = p1-p2;
                cmp_flags(p1,p2,out);
                break;
            case 0x06://XOR
                out = p1^p2;
                break;
        }

        if (instr_choice == 0x01 || instr_choice == 0x04 || instr_choice == 0x06) //or and xor
            test_flags(out);

        if (instr_choice == 0x07) //cmp
            out = p1;
        return out;
    }

    u8 get_segment(u8 default_segment)
    {
        return segment_override?segment_override:default_segment;
    }

    void clear_prefix()
    {
        segment_override = 0;
        string_prefix = 0;
        lock = 0;
    }
    bool set_prefix(u8 instruction)
    {
        if ((instruction&0xE7) == 0x26) // segment override:
        {
            cycles_used += 2;
            segment_override = ((instruction>>3)&0x3)|0x8;
            return true;
        }
        if ((instruction&0xFE) == 0xF2)
        {
            cycles_used += 2;
            string_prefix = 1+(instruction&0x01); //REPNZ REPZ
            return true;
        }
        if (instruction == 0xF0 || instruction == 0xF1) // LOCK
        {
            lock = 1;
            cycles_used += 2;
            return true;
        }
        return false;
    }

    u32 interrupt_true_cycles{};

    u32 interrupt_table[256] = {};

    bool accepts_interrupts()
    {
        return interrupt_true_cycles >= 2;
    }

    void interrupt(u8 n, bool forced=false)
    {
        if (flag(F_INTERRUPT) || forced)
        {
            halt = false;
            cycles_used += 80;

            ++interrupt_table[n];

            push(registers[FLAGS]);
            push(registers[CS]);
            push(registers[IP]);

            registers[IP] = mem._16(0, n*4);
            registers[CS] = mem._16(0, n*4+2);
            set_flag(F_INTERRUPT,false);
            if (!forced)
            {
                if constexpr(DEBUG_LEVEL > 0)
                    cout << "IRQ: CPU ACK " << u32(n-8) << endl;
                pic.cpu_ack_irq(n-8);
            }
        }
    }

    void invalid_instruction()
    {
        cout << "Invalid instruciton." << endl;
        cycles_used += 60;
        interrupt(6, true);
    }
    void outside_bound()
    {
        cout << "Bounds violation." << endl;
        cycles_used += 60;
        interrupt(5, true);
    }

    void irq(u8 n)
    {
        interrupt(n+8, false);
    }

    void push(u16 data)
    {
        registers[SP] -= 2;
        mem._16(registers[SS], registers[SP]) = data;
    }
    u16 pop()
    {
        u16 data = mem._16(registers[SS], registers[SP]);
        registers[SP] += 2;
        return data;
    }

    bool halt{false};
    u64 cycles_used{};
    bool is_inside_multi_part_instruction{};

    void cycle()
    {
        ++cycles;

        if (delay)
        {
            --delay;
            return;
        }
        if (halt)
        {
            return;
        }

        if (registers[CS] == 0 && registers[IP] == 0)
        {
            cout << "Trying to run code at CS:IP 0:0... resetting." << endl;
            reset();
        }
        is_inside_multi_part_instruction = false;
        u8 instruction = read_inst<u8>();
        if (startprinting)
        {
            std::cout << "#" << std::dec << cycles << std::hex << ": " << u32(instruction) << " @ " << registers[CS]*16+registers[IP]-1;
            print_regs();
        }

        u32 prefix_byte_n = 0;
        while (set_prefix(instruction))
        {
            ++prefix_byte_n;
            instruction = read_inst<u8>();
            if (startprinting || DEBUG_LEVEL > 1)
                std::cout << "prefix read. #" << std::dec << cycles << std::hex << ": " << "Executing 0x" << u32(instruction) << " at CS:IP = " << registers[CS] << ":" << registers[IP]-1 << " = " << registers[CS]*16+registers[IP]-1 << std::endl;
        }

        if (false);
        else if (instruction < 0x40 && (instruction&0x07) < 6)
        {
            u8 instr_choice = (instruction&0x38)>>3;
            if (instruction&0x04)
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
                cycles_used += 4;
            }
            else
            {
                u8 modrm = read_inst<u8>();
                if (instruction&0x01)//16bit
                {
                    u16& rm = decode_modrm_u16(modrm);
                    u16& r = get_r16((modrm>>3)&0x07);
                    u16& rout = (instruction&0x02?r:rm);
                    u16& rin = (instruction&0x02?rm:r);
                    rout = run_arith(rout, rin, instr_choice);
                }
                else//8bit
                {
                    u8& rm = decode_modrm_u8(modrm);
                    u8& r = get_r8((modrm>>3)&0x07);
                    u8& rout = (instruction&0x02?r:rm);
                    u8& rin = (instruction&0x02?rm:r);
                    rout = run_arith(rout, rin, instr_choice);
                }
                if (instruction&0x02) //towards general reg
                {
                    cycles_used += (modrm_is_register?3:13);
                }
                else //towards modrm byte
                {
                    cycles_used += (modrm_is_register?3:24);
                }
            }
        }
        else if ((instruction&0xE6) == 0x06)
        {
            if (instruction == 0x0F)
            {
                cout << "POP CS?? ASDFGH" << endl;
            }
            u16& reg = get_segment_r16((instruction>>3)&0x03);
            if (instruction&0x01)
            {
                cycles_used += 12;
                reg = pop();
            }
            else
            {
                cycles_used += 14;
                push(reg);
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
            cycles_used += 4;
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
            cycles_used += 8;
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
            cycles_used += 4;
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
            cycles_used += 8;
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
            cycles_used += 3;
        }
        else if ((instruction&0xF8) == 0x50) // push reg
        {
            push(registers[instruction&0x07]);
            cycles_used += 15;
        }
        else if ((instruction&0xF8) == 0x58) //pop reg
        {
            registers[instruction&0x07] = pop();
            cycles_used += 12;
        }
        else if ((instruction&0xE0) == 0x60) //various short jumps
        {
            if ((instruction&0xF0) == 0x60)
            {
                cout << "*" << u32(instruction);
            }
            u8 type = (instruction&0x0F)>>1;
            u16 f = (registers[FLAGS]&0xFFFD) | (flag(F_SIGN) != flag(F_OVERFLOW) ? 0x2:0x0);
            const u16 masks[8] =
            {
                0x800,0x001,0x040,0x041,0x080,0x004,0x002,0x042
            };
            i8 offset = read_inst<i8>();
            cycles_used += 4;
            if (bool(f&masks[type])^(instruction&0x01))
            {
                registers[IP] += offset;
                cycles_used += 12;
            }
        }
        else if (instruction == 0x80 || instruction == 0x82)
        {
            u8 modrm = read_inst<u8>();
            u8& rm = decode_modrm_u8(modrm);
            u8 imm = read_inst<u8>();
            rm = run_arith(rm, imm, (modrm>>3)&0x07);
            cycles_used += (modrm_is_register?4:23);
        }
        else if (instruction == 0x81)
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            u16 imm = read_inst<u16>();
            rm = run_arith(rm, imm, (modrm>>3)&0x07);
            cycles_used += (modrm_is_register?4:23);
        }
        else if (instruction == 0x83)
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            u16 imm = i16(read_inst<i8>());
            rm = run_arith(rm, imm, (modrm>>3)&0x07);
            cycles_used += (modrm_is_register?4:23);
        }
        else if (instruction == 0x84)
        {
            u8 modrm = read_inst<u8>();
            u8& rm = decode_modrm_u8(modrm);
            u8& r = get_r8((modrm>>3)&0x07);
            test_flags(u8(rm&r));
            cycles_used += (modrm_is_register?5:11);
        }
        else if (instruction == 0x85)
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            u16& r = get_r16((modrm>>3)&0x07);
            test_flags(u16(rm&r));
            cycles_used += (modrm_is_register?5:11);
        }
        else if (instruction == 0x86)
        {
            u8 modrm = read_inst<u8>();
            u8& rm = decode_modrm_u8(modrm);
            u8& r = get_r8((modrm>>3)&0x07);
            u8 temp = rm;
            rm = r;
            r = temp;
            cycles_used += (modrm_is_register?4:25);
        }
        else if (instruction == 0x87)
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            u16& r = get_r16((modrm>>3)&0x07);
            u16 temp = rm;
            rm = r;
            r = temp;
            cycles_used += (modrm_is_register?4:25);
        }
        else if ((instruction&0xFC) == 0x88) // MOV EbGb, EvGv, GbEb, GvEv
        {
            u8 modrm = read_inst<u8>();
            if (instruction&0x01)//16bit
            {
                u16& rm = decode_modrm_u16(modrm);
                u16& r = get_r16((modrm>>3)&0x07);
                u16& rout = (instruction&0x02?r:rm);
                u16& rin = (instruction&0x02?rm:r);
                rout = rin;
            }
            else//8bit
            {
                u8& rm = decode_modrm_u8(modrm);
                u8& r = get_r8((modrm>>3)&0x07);
                u8& rout = (instruction&0x02?r:rm);
                u8& rin = (instruction&0x02?rm:r);
                rout = rin;
            }

            if (modrm_is_register)
            {
                cycles_used += 2;
            }
            else
            {
                if (instruction&0x02) // towards general register
                {
                    cycles_used += 12;
                }
                else //towards modrm
                {
                    cycles_used += 13;
                }
            }
        }
        else if (instruction == 0x8C) // MOV EwSw
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            u16& r = get_segment_r16((modrm>>3)&0x07);
            rm = r;
            cycles_used += (modrm_is_register?2:13);
        }
        else if (instruction == 0x8D) // LEA Gv M
        {
            u8 modrm = read_inst<u8>();
            u16& r = get_r16((modrm>>3)&0x07);
            r = effective_address(modrm);
            cycles_used += 2;
        }
        else if (instruction == 0x8E) // MOV SwEw
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            u16& r = get_segment_r16((modrm>>3)&0x07);
            r = rm;
            cycles_used += (modrm_is_register?2:12);
        }
        else if (instruction == 0x8F) //POP modrm
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            rm = pop();
            cycles_used += 25;
        }
        else if ((instruction&0xF8) == 0x90) // XCHG AX, r16 - note how 0x90 is effectively NOP :-)
        {
            u8 reg_id = instruction&0x7;
            u16 tmp = registers[AX];
            registers[AX] = registers[reg_id];
            registers[reg_id] = tmp;
            cycles_used += 3;
        }
        else if (instruction == 0x98) //CBW
        {
            u16 r = (registers[AX])&0xFF;
            r |= (r&0x80)?0xFF00:0x0000;
            registers[AX] = r;
            cycles_used += 2;
        }
        else if (instruction == 0x99) //CWD
        {
            registers[DX] = (registers[AX]&0x8000)?0xFFFF:0x0000;
            cycles_used += 5;
        }
        else if (instruction == 0x9A) //call Ap
        {
            u16 pointer = read_inst<u16>();
            u16 segment = read_inst<u16>();
            push(registers[CS]);
            push(registers[IP]);
            registers[CS] = segment;
            registers[IP] = pointer;
            cycles_used += 28;
        }
        else if (instruction == 0x9B) // WAIT/FWAIT
        {
            // waits for floating point exceptions.
            // basically a NOP because I don't have a FPU yet
            cycles_used += 4;
        }
        else if (instruction == 0x9C) //pushf
        {
            push(registers[FLAGS]);
            cycles_used += 14;
        }
        else if (instruction == 0x9D) //popf
        {
            u16 newflags = pop();
            const u16 FLAG_MASK = 0b0000'1111'1101'0101;
            registers[FLAGS] = (registers[FLAGS]&~FLAG_MASK) | (newflags&FLAG_MASK);
            cycles_used += 12;
        }
        else if (instruction == 0x9E) //sahf
        {
            reg8()[FLAGS*2] = (get_r8(4)&0xD5) | 0x02;
            cycles_used += 4;
        }
        else if (instruction == 0x9F) //lahf
        {
            get_r8(4) = reg8()[FLAGS*2];
            cycles_used += 4;
        }
        else if (instruction >= 0xA0 && instruction <= 0xA3) //AL/X=MEM  MEM=AL/X
        {
            u16 source_segment = registers[get_segment(DS)];
            u16 source_offset = read_inst<u16>();
            switch(instruction)
            {
                case 0xA0: get_r8(0) = mem._8(source_segment, source_offset); break;
                case 0xA1: registers[AX] = mem._16(source_segment, source_offset); break;
                case 0xA2: mem._8(source_segment, source_offset) = get_r8(0); break;
                case 0xA3: mem._16(source_segment, source_offset) = registers[AX]; break;
            }
            cycles_used += 14;
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
            cycles_used += 4;
        }
        else if (instruction >= 0xA0 && instruction <= 0xAF) //MOVSB/W CMPSB/W --- STOSB/W LODSB/W SCASB/W
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
                 0,  0,  0,  0, 18, 26, 30, 30,
                 0,  0, 11, 15, 16, 16, 19, 19,
            };
            const u8 cycles_rep_mult[16] =
            {
                0, 0, 0, 0,17,25,30,30,
                0, 0,10,14,15,15,15,19,//check LOD
            };

            u16 source_segment = registers[get_segment(DS)];

            u16 program = programs[(instruction>>1)&0x07];
            if (registers[CX] != 0 || string_prefix == 0) do
            {
                u16 value1{}, value2{};
                if (big)
                {
                    if (program&0x01)
                        value1 = mem._16(source_segment, registers[SI]);
                    if (program&0x02)
                        value2 = mem._16(registers[ES], registers[DI]);
                    if (program&0x04)
                        value1 = registers[AX];
                    if (program&0x10)
                        mem._16(source_segment, registers[SI]) = value1;
                    if (program&0x20)
                        mem._16(registers[ES],registers[DI]) = value1;
                    if (program&0x40)
                        registers[AX] = value1;
                    if (program&0x100)
                        cmp_flags<u16>(value1,value2,u16(value1-value2));
                }
                else
                {
                    if (program&0x01)
                        value1 = mem._8(source_segment, registers[SI]);
                    if (program&0x02)
                        value2 = mem._8(registers[ES], registers[DI]);
                    if (program&0x04)
                        value1 = get_r8(0);
                    if (program&0x10)
                        mem._8(source_segment, registers[SI]) = value1;
                    if (program&0x20)
                        mem._8(registers[ES],registers[DI]) = value1;
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
                        cycles_used += 9;
                        break;
                    }
                    if (string_prefix == SP_REPZ && !flag(F_ZERO))
                    {
                        cycles_used += 9;
                        break;
                    }
                }
                if (registers[CX] == 0)
                {
                    cycles_used += 9;
                    break;
                }
                registers[IP] -= 1+prefix_byte_n;
                is_inside_multi_part_instruction = true;
            } while(false);
        }
        else if ((instruction&0xF8) == 0xB0) //mov reg8, Ib
        {
            get_r8(instruction&0x07) = read_inst<u8>();
            cycles_used += 4;
        }
        else if ((instruction&0xF8) == 0xB8) //mov reg16, Iv
        {
            get_r16(instruction&0x07) = read_inst<u16>();
            cycles_used += 4;
        }
        else if (instruction == 0xC0 || instruction == 0xC2) // near return w/imm
        {
            u16 imm = read_inst<u16>();
            registers[IP] = pop();
            registers[SP] += imm;
            cycles_used += 24;
        }
        else if (instruction == 0xC1 || instruction == 0xC3) // near return
        {
            registers[IP] = pop();
            cycles_used += 20;
        }
        else if (instruction == 0xC8 || instruction == 0xCA) // far return w/imm
        {
            u16 imm = read_inst<u16>();
            registers[IP] = pop();
            registers[CS] = pop();
            registers[SP] += imm;
            cycles_used += 33;
        }
        else if (instruction == 0xC9 || instruction == 0xCB) // far return
        {
            registers[IP] = pop();
            registers[CS] = pop();
            cycles_used += 34;
        }
        else if ((instruction&0xFE) == 0xC4) // LES LDS
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            u16& r = get_r16((modrm>>3)&0x07);
            r = rm;
            registers[(instruction&1)?DS:ES] = *((&rm)+1); //ES or DS, based on the opcode
            cycles_used += 24;
        }
        else if (instruction == 0xC6)
        {
            u8 modrm = read_inst<u8>();
            u8& rm = decode_modrm_u8(modrm);
            rm = read_inst<u8>();
            cycles_used += (modrm_is_register?4:14);
        }
        else if (instruction == 0xC7)
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            rm = read_inst<u16>();
            cycles_used += (modrm_is_register?4:14);
        }
        else if (instruction == 0xCC) // INT 3
        {
            if constexpr (DEBUG_LEVEL > 0)
                cout << "Calling interrupt 3... AX=" << registers[AX] << endl;
            interrupt(3, true);
            //cycles_used += 72;
        }
        else if (instruction == 0xCD) // INT imm8
        {
            u8 int_num = read_inst<u8>();
            if constexpr (DEBUG_LEVEL > 0)
                cout << "Calling interrupt... " << u32(int_num) << " AX=" << registers[AX] << endl;
            interrupt(int_num, true);
            //cycles_used += 71;
        }
        else if (instruction == 0xCE) // INTO
        {
            if (flag(F_OVERFLOW))
            {
                if constexpr (DEBUG_LEVEL > 0)
                    cout << "Calling int 4... AX=" << registers[AX] << endl;
                interrupt(4, true);
                //cycles_used += 69;
            }
            cycles_used += 4;
        }
        else if (instruction == 0xCF) // IRET!
        {
            registers[IP] = pop();
            registers[CS] = pop();
            u16 newflags = pop();
            const u16 FLAG_MASK = 0b0000'1111'1101'0101;
            registers[FLAGS] = (registers[FLAGS]&~FLAG_MASK) | (newflags&FLAG_MASK);

            if (startprinting)
                cout << "RETURN FROM INTERRUPT to " << registers[IP]<< ":" << registers[CS] << "|" << newflags << endl;
            cycles_used += 44;
        }
        else if (instruction == 0xD0 || instruction == 0xD2)
        {
            bool single_shift = ((instruction&0x02) == 0) || ((registers[CX]&0xFF) == 1);
            u8 modrm = read_inst<u8>();
            u8& rm = decode_modrm_u8(modrm);

            u8 inst_type = (modrm>>3)&0x07;
            u8 amount = (single_shift)?1:(registers[CX]&0xFF);

            if ((instruction&0x02) == 0)
            {
                cycles_used += (modrm_is_register?23:2);
            }
            else
            {
                cycles_used += (modrm_is_register?28:8) + 4*(registers[CX]&0xFF);
            }

            for(u32 i=0; i<amount; ++i)
            {
                //F_OVERFLOW, F_SIGN, F_ZERO, F_AUX_CARRY, F_PARITY, F_CARRY
                u8 original=rm;
                u8 result=0;
                if(inst_type == 6)
                {
                    cout << "¤";
                    break;
                }
                //ROL ROR RCL RCR SHL SHR SAL SAR
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
        }
        else if (instruction == 0xD1 || instruction == 0xD3)
        {
            bool single_shift = ((instruction&0x02) == 0) || ((registers[CX]&0xFF) == 1);
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);

            u8 inst_type = (modrm>>3)&0x07;
            u8 amount = (single_shift)?1:(registers[CX]&0xFF);

            if ((instruction&0x02) == 0)
            {
                cycles_used += (modrm_is_register?23:2);
            }
            else
            {
                cycles_used += (modrm_is_register?28:8) + 4*(registers[CX]&0xFF);
            }

            for(u32 i=0; i<amount; ++i)
            {
                //F_OVERFLOW, F_SIGN, F_ZERO, F_AUX_CARRY, F_PARITY, F_CARRY
                u16 original=rm;
                u16 result=0;
                if (false);
                //ROL ROR RCL RCR SHL SHR SAL SAR
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
                interrupt(0, true);
            }
            cycles_used += 83;
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
            cycles_used += 60;
        }
        else if (instruction == 0xD6) // SALC (undocumented!)
        {
            get_r8(0) = flag(F_CARRY)?0xFF:0x00;
            cycles_used += 4; //TODO: make sure this is correct!
        }
        else if (instruction == 0xD7) // XLAT
        {
            u8 result = mem._8(registers[get_segment(DS)],registers[BX]+(registers[AX]&0xFF));
            get_r8(0) = result;
            cycles_used += 11;
        }
        else if (instruction >= 0xD8 && instruction <= 0xDF)
        {
            //cout << "Trying to run floating point instruction! :(" << endl;
            u8 modrm = read_inst<u8>(); //read modrm data anyway to sync up
            decode_modrm_u8(modrm);
            //FLOATING POINT INSTRUCTIONS! 8087! we don't have this. yet?
            cycles_used += 4; //TODO: check that this is right!
        }
        else if ((instruction & 0xFC) == 0xE0) // LOOPNZ LOOPZ LOOP JCXZ
        {
            i8 offset = read_inst<i8>();
            registers[CX] -= ((instruction&0x03)!=3);
            if ((registers[CX] != 0) == ((instruction&0x03) != 3) && (instruction&0x02 ? true:(flag(F_ZERO) == (instruction&0x01))))
            {
                registers[IP] = i16(registers[IP]) + offset;
                cycles_used += 12 + ((instruction&3)?0:2);
            }
            //not taken:  5  6  5  6
            //taken:     19 18 17 18 (delta: 14 12 12 12)
            cycles_used += 5 + (instruction&1);
        }
        else if (instruction == 0xE4) // IN
        {
            get_r8(0) = IO::in(read_inst<u8>());
            cycles_used += 14;
        }
        else if (instruction == 0xE5) // IN
        {
            u8 port = read_inst<u8>();
            u8 low = IO::in(port);
            u8 high = IO::in(port+1);
            registers[AX] = (high<<8)|low;
            cycles_used += 14;
        }
        else if (instruction == 0xE6) // OUT
        {
            IO::out(read_inst<u8>(), registers[AX]&0xFF);
            cycles_used += 14;
        }
        else if (instruction == 0xE7) // OUT
        {
            u8 port = read_inst<u8>();
            IO::out(port, registers[AX]&0xFF);
            IO::out(port+1, registers[AX]>>8);
            cycles_used += 14;
        }
        else if (instruction == 0xE8)
        {
            i16 ip_offset = read_inst<i16>();
            push(registers[IP]);
            registers[IP] = i16(registers[IP])+ip_offset;
            cycles_used += 23; //TODO: check that this is the correct one
        }
        else if (instruction == 0xE9)
        {
            i16 ip_offset = read_inst<i16>();
            registers[IP] = i16(registers[IP])+ip_offset;
            cycles_used += 15;
        }
        else if (instruction == 0xEA) //far jump
        {
            u16 new_ip = read_inst<u16>();
            u16 new_cs = read_inst<u16>();

            registers[IP] = new_ip;
            registers[CS] = new_cs;
            cycles_used += 15;
        }
        else if (instruction == 0xEB)
        {
            i8 ip_offset = read_inst<i8>();
            registers[IP] = i16(registers[IP])+ip_offset;
            cycles_used += 15;
        }
        else if (instruction == 0xEC) // IN
        {
            get_r8(0) = IO::in(registers[DX]);
            cycles_used += 12;
        }
        else if (instruction == 0xED) // IN
        {
            u16 port = registers[DX];
            u8 low = IO::in(port);
            u8 high = IO::in(port+1);
            registers[AX] = (high<<8)|low;
            cycles_used += 12;
        }
        else if (instruction == 0xEE) // OUT
        {
            IO::out(registers[DX], registers[AX]&0xFF);
            cycles_used += 12;
        }
        else if (instruction == 0xEF) // OUT
        {
            IO::out(registers[DX], registers[AX]&0xFF);
            IO::out(registers[DX]+1, registers[AX]>>8);
            cycles_used += 12;
        }
        else if (instruction == 0xF4) // HALT / HLT
        {
            halt = true;
            cycles_used += 2;
        }
        else if (instruction == 0xF5) // cmc
        {
            set_flag(F_CARRY, !flag(F_CARRY));
            cycles_used += 2;
        }
        else if(instruction == 0xF6) //byte param
        {
            u8 modrm = read_inst<u8>();
            u8& rm = decode_modrm_u8(modrm);
            u8 op = ((modrm>>3)&0x07);
            if (op == 0) // TEST
            {
                u8 imm = read_inst<u8>();
                test_flags(u8(rm&imm));
                cycles_used += (modrm_is_register?5:11);
            }
            else if (op == 1) //invalid!!!
            {
                //[[maybe_unused]] u8 imm = read_inst<u8>();
                cout << "$";
                cycles_used += 4; // TODO: check this ???
            }
            else if (op==2) // NOT
            {
                rm = ~rm;
                cycles_used += (modrm_is_register?3:24);
            }
            else if (op == 3) // NEG
            {
                cmp_flags(u8(0),rm,u8(-rm));
                set_flag(F_CARRY,rm!=0);
                rm = -rm;
                cycles_used += (modrm_is_register?3:24);
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
                registers[AX] = result;
                cycles_used += (modrm_is_register?70:76); //TODO: make this more accurate, it's actually 70-77, 76-83
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
                cycles_used += (modrm_is_register?80:86); //TODO: make this more accurate, it's actually 80-98, 86-104
            }
            else if (op==6 || op == 7) //DIV IDIV
            {
                if (rm == 0)
                {
                    interrupt(0, true);
                    cycles_used += 80; //FIXME: this is made up
                }
                else if (op == 6)
                {
                    u8 denominator = rm;
                    u16 result = registers[AX]/denominator;
                    if (result >= 0x100)
                    {
                        set_flag(F_PARITY,false);
                        set_flag(F_SIGN, registers[AX]&0x8000);
                        interrupt(0, true);
                    }
                    else
                    {
                        u8 quotient = result;
                        u8 remainder = registers[AX] % denominator;
                        registers[AX] = (remainder<<8)|quotient;
                        set_flag(F_PARITY,false);
                        set_flag(F_SIGN, remainder^0x80);
                    }
                    cycles_used += (modrm_is_register?80:86); //TODO: 80-90, 86-96
                }
                else if (op == 7)
                {
                    i8 denominator = i8(rm);
                    i16 result = i16(registers[AX]) / denominator;
                    if (result <= -0x80 || result >= 0x80) // 8088/6 doesn't accept -0x80
                    {
                        interrupt(0, true);
                    }
                    else
                    {
                        i8 quotient = result&0xFF;
                        if (string_prefix != 0)
                            quotient = -quotient;
                        i8 remainder = i16(registers[AX]) % denominator;
                        registers[AX] = (remainder<<8)|u8(quotient);
                        set_flag(F_PARITY,false);
                    }
                    cycles_used += (modrm_is_register?101:107); //TODO: 101-112, 107-118
                }
            }
            else
            {
                std::cout << "0xF6 op " << u32(op) << " is invalid!" << std::endl;
                std::abort();
            }
        }
        else if(instruction == 0xF7) //word param
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            u8 op = ((modrm>>3)&0x07);
            if (op == 0) // TEST
            {
                u16 imm = read_inst<u16>();
                test_flags(u16(rm&imm));
                cycles_used += (modrm_is_register?5:11);
            }
            else if (op == 1) //invalid!!!
            {
                cout << "%";
                cycles_used += 4; //what?
            }
            else if (op==2) // NOT
            {
                rm = ~rm;
                cycles_used += (modrm_is_register?3:24);
            }
            else if (op == 3) // NEG
            {
                cmp_flags(u16(0),rm,u16(-rm));
                set_flag(F_CARRY,rm!=0);
                rm = -rm;
                cycles_used += (modrm_is_register?3:24);
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
                cycles_used += (modrm_is_register?118:124); //TODO: make this more accurate, it's actually 118-133, 124-139
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
                cycles_used += (modrm_is_register?128:134); //TODO: make this more accurate, it's actually 128-154, 134-160
            }
            else if (op == 6 || op == 7) //DIV IDIV
            {
                if (rm == 0)
                {
                    interrupt(0, true); //division by zero
                    cycles_used += 80; //FIXME: this is made up
                }
                else if (op == 6)
                {
                    u32 numerator = (registers[DX]<<16)|registers[AX];
                    u16 denominator = rm;
                    u32 result = numerator / denominator;
                    if (result >= 0x10000)
                    {
                        interrupt(0,true);
                    }
                    else
                    {
                        registers[AX] = result;
                        registers[DX] = numerator % denominator;
                    }
                    cycles_used += (modrm_is_register?144:150); //TODO: 144-162, 150-168
                }
                else if (op == 7)
                {
                    i32 numerator = i32((registers[DX]<<16)|registers[AX]);
                    i16 denominator = i16(rm);
                    i32 result = numerator / denominator;
                    if (result <= -0x8000 || result >= 0x8000)
                    {
                        interrupt(0,true);
                    }
                    else
                    {
                        registers[AX] = numerator / denominator;
                        registers[DX] = numerator % denominator;
                        if (string_prefix != 0)
                            registers[AX] = -registers[AX];
                    }
                    cycles_used += (modrm_is_register?165:171); //TODO: 165-184, 171-190
                }
            }
            else
            {
                std::cout << "0xF7 op " << u32(op) << " is invalid!" << std::endl;
                std::abort();
            }
        }
        else if ((instruction&0xFE) == 0xF8) //CLC STC carry flag bit 0
        {
            set_flag(F_CARRY, instruction&0x01);
            cycles_used += 2;
        }
        else if ((instruction&0xFE) == 0xFA) //CLI STI interrupt flag bit 9
        {
            set_flag(F_INTERRUPT, instruction&0x01);
            cycles_used += 2;
        }
        else if ((instruction&0xFE) == 0xFC) //CLD STD direction flag bit 10
        {
            set_flag(F_DIRECTIONAL, instruction&0x01);
            cycles_used += 2;
        }
        else if (instruction == 0xFE)
        {
            u8 modrm = read_inst<u8>();
            u8& reg = decode_modrm_u8(modrm);
            u8 op = (modrm>>3)&0x07;
            u8 result = reg+1-(op<<1);
            if (op >= 2)
            {
                cout << "*" << u32(instruction) << "-" << u32(modrm);
                //std::cout << "Invalid opcode combo: 0x" << u32(instruction) << " 0x" << u32(modrm) << std::endl;
                //std::abort();
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
                reg = result;
                cycles_used = (modrm_is_register?3:23);
            }
        }
        else if (instruction == 0xFF)
        {
            u8 modrm = read_inst<u8>();
            u16& reg = decode_modrm_u16(modrm);
            u8 op = (modrm>>3)&0x07;
            if (op == 0 || op == 1)
            {
                u16 result = reg+1-(op<<1);
                set_flag(F_OVERFLOW,result==(0x8000-op));
                set_flag(F_AUX_CARRY,(result&0x0F) == ((op&0x01)?0x0F:0x00));
                set_flag(F_ZERO,result==0);
                set_flag(F_SIGN,result&0x8000);
                set_flag(F_PARITY,byte_parity[result&0xFF]);
                reg = result;
                cycles_used = (modrm_is_register?3:23);
            }
            else if (op == 2) //call near
            {
                u16 address = reg;
                push(registers[IP]);
                registers[IP] = address; //have to do this because reg could be SP :')
                cycles_used += (modrm_is_register?20:29);
            }
            else if (op == 3) //call far
            {
                u16 segment = *((&reg)+1);
                u16 address = reg;
                push(registers[CS]);
                push(registers[IP]);
                registers[CS] = segment;
                registers[IP] = address;
                cycles_used += (modrm_is_register?20:53);
            }
            else if (op == 4) //jmp near
            {
                registers[IP] = reg;
                cycles_used += (modrm_is_register?11:18);
            }
            else if (op == 5) //jmp far
            {
                registers[IP] = reg;
                registers[CS] = *((&reg)+1);
                cycles_used += (modrm_is_register?11:24);
            }
            else if (op == 6 || op == 7)
            {
                push(reg);
                cycles_used += (modrm_is_register?15:24);
            }
            else
            {
                std::cout << "#" << std::dec << cycles << std::hex << ": " << "Executing 0x" << u32(instruction) << " at CS:IP = " << registers[CS] << ":" << registers[IP]-1 << " = " << registers[CS]*16+registers[IP]-1 << std::endl;
                std::cout << "Unimplemented opcode combo: 0x" << u32(instruction) << " 0x" << u32(modrm) << std::endl;
                std::abort();
            }
        }
        else
        {
            std::cout << "#" << std::dec << cycles << std::hex << ": " << "Executing 0x" << u32(instruction) << " at CS:IP = " << registers[CS] << ":" << registers[IP]-1 << " = " << registers[CS]*16+registers[IP]-1 << std::endl;
            std::cout << "# " << std::dec << cycles << std::hex << ", Unknown opcode: 0x" << u32(instruction) << std::endl;
            std::abort();
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

        if (flag(F_TRAP))
        {
            registers[IP] = original_ip;
            interrupt(1, true);
        }

        mem.update();

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
            std::abort();
        }
    }
} cpu;

