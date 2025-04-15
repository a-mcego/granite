#pragma once

#include "interrupt.h"
#include "mem8088.h"

//NON WORKING NEW VERSION
struct CPU8088MC
{
    MemoryManager8088& mem;
    CHIP8259& pic;
    CHIP8259& pic2;
    IOSystem& iosystem;
    CPU8088MC(MemoryManager8088& mem_, CHIP8259& pic_, CHIP8259& pic2_, IOSystem& iosystem_) : mem(mem_), pic(pic_), pic2(pic2_), iosystem(iosystem_) {}

    enum REG
    {
        ES,CS,SS,DS,IP, IND, OPR, Q,
        unk8, unk9, M, unk11, SIGMA, unk13,unk14,unk15,
        unk16,unk17,unk18,unk19, TMPA, TMPB, TMPC, FLAGS,
        AX,CX,DX,BX, SP,BP,SI,DI,
    };

    //used for M
    bool modrm_is_register{};
    u16 modrm_seg{};
    u16 modrm_offset{};
    u8 modrm_reg{}; //register from modRM part
    u8 modrm_width{};
    u8 modrm_r{}; //register from R part

    void writeM(u16 data)
    {
        cycles_used += (modrm_width == 16?7:3);
        //std::cout << "writeM: " << std::hex;
        if (modrm_is_register)
        {
            if (modrm_width == 16)
                get_r16(modrm_reg) = data;
            else
                get_r8(modrm_reg) = data;
        }
        else
        {
            mem._8(modrm_seg, modrm_offset) = (data&0xFF);
            if (modrm_width == 16)
                mem._8(modrm_seg, modrm_offset+1) = ((data>>8)&0xFF);
            //std::cout << modrm_seg << ":" << modrm_offset << " ";
        }
        //std::cout << data << std::endl;
    }
    u16 readM()
    {
        cycles_used += (modrm_width == 16?8:4);
        //std::cout << "readM: " << std::hex;
        u16 ret{};
        if (modrm_is_register)
        {
            if (modrm_width == 16)
                ret = get_r16(modrm_reg);
            else
                ret = get_r8(modrm_reg);
        }
        else
        {
            ret = mem._8(modrm_seg, modrm_offset);
            if (modrm_width == 16)
                ret |= (mem._8(modrm_seg, modrm_offset+1)<<8);
            //std::cout << modrm_seg << ":" << modrm_offset << " ";
        }
        //std::cout << ret << std::endl;
        return ret;
    }

    //start_address: where to start executing
    //bitwidth: 8 or 16
    //X0: jump condition for jump types 0:B, 5:3 and 7:3 (phew) - often bit 3 of opcode
    //f1: jump condition for jump types 0:D, 5:5 and 7:5, flipped by 4:2. (used by mul sign determination)
    //XIvalue: ALU OP used when XI
    void mc_execute(u16 start_address, u8 bitwidth, bool X0, bool f1, u8 XIvalue)
    {
        //cout << "-------------------------------------------" << endl;

        u16 ALU_HIGH_BIT{};
        u16 ALU_MASK{};

        auto set_width = [&]()
        {
            ALU_HIGH_BIT = (1<<(bitwidth-1));
            ALU_MASK = (1<<bitwidth)-1;
            modrm_width = bitwidth;
        };

        set_width();

        u16 counter{}, aluflags{registers[FLAGS]};
        u16& sigma = registers[SIGMA];

        auto setflag = [&](auto flag, bool value)
        {
            aluflags = (aluflags&~flag) | (value?flag:0);
        };

        enum OPER
        {
            ADD = 0,
            OR = 1, //my addition for completeness
            ADC = 2,
            SBB = 3, //my addition for completeness
            AND = 4,
            SUBT = 5,
            XOR = 6, //my addition for completeness
            CMP = 7, //my addition for completeness

            ROL = 8,
            ROR = 9,
            LRCY = 10,
            RRCY = 11,
            SHL = 12,
            SHR = 13,
            SETMO = 14, //not actually sal. does something weird.
            SAR = 15,
            PASS = 16, //equality/nop?
            XI = 17, //reads the op from somewhere else?

            DAA = 20,
            DAS = 21,
            AAA = 22,
            AAS = 23,
            INC = 24,
            DEC = 25,
            COM1 = 26,
            NEG = 27,
            INC2 = 28,
            DEC2 = 29,
        };
        enum FLAG
        {
            CARRY=(1<<0),
            PARITY=(1<<2),
            AUX_CARRY=(1<<4),
            ZERO=(1<<6),
            SIGN=(1<<7),
            TRAP=(1<<8),
            INTERRUPTFLAG=(1<<9),
            DIRECTIONAL=(1<<10),
            OVERFLOW=(1<<11)
        };

        auto execute = [&](u16 line) -> void
        {
            const char* const regnames[32] =
            {
                "RA(es)", //0
                "RC(cs)",
                "RS(ss)",
                "RD(ds)",
                "PC",
                "IND",
                "OPR",
                "Q(read), none(write)",
                "X(ah)", //8
                "[5]",
                "M",
                "R",
                "SIGMA(read), tmpaL(write)",//12
                "ONES(read), tmpbL(write)",
                "CR(read), tmpaH(write)",
                "ZERO(read), tmpbH(write)",
                "A(al)", //16
                "",
                "[a]",
                "",
                "tmpa", //20
                "tmpb", //21
                "tmpc", //22
                "F(flags)",
                "XA(ax)", //24
                "BC(cx)",
                "DE(dx)",
                "HL(bx)",
                "SP(sp)",
                "MP(bp)",
                "IJ(si)",
                "IK(di)",
            };

            enum LOCATIONS
            {
                FARCALL = 0x06b,
                FARCALL2 = 0x06c,
                NEARCALL = 0x077,
                FARRET = 0x0c2,
                RELJMP = 0x0d2,
                RPTS = 0x112,
                RPTI = 0x118,
                AAEND = 0x179,
                CORX = 0x17f,
                CORD = 0x188,
                INT1 = 0x198,
                INT2 = 0x199,
                IRQ = 0x19a,
                INTR = 0x19d,
                INT0 = 0x1a7,
                PREIDIV = 0x1b4,
                NEGATE = 0x1b6,
                PREIMUL = 0x1c0,
                POSTIDIV = 0x1c4,
                IMULCOF = 0x1cd,
                MULCOF = 0x1d2,
                EALOAD = 0x1e1,
                EADONE = 0x1e3,
                RESET = 0x1e4,
            };

            const u16 lj_table[16] =
            {
                FARCALL, NEARCALL, RELJMP, EALOAD /*actually EAOFFSET???*/,
                EADONE /*actually EAFINISH???*/, FARCALL2, INTR, INT0,
                RPTI, AAEND, 0, 0,
                0, 0, 0, 0,
            };
            const u16 lc_table[16] =
            {
                FARRET, RPTS, CORX, CORD,
                PREIMUL, NEGATE, IMULCOF, MULCOF,
                PREIDIV, POSTIDIV, 0, 0,
                0, 0, 0, 0,
            };

            auto decode_target_index = [](u16 input)
            {
                u8 b0 = (input&0x01)?1:0;
                u8 b1 = (input&0x02)?1:0;
                u8 b2 = (input&0x04)?1:0;
                u8 b3 = (input&0x08)?1:0;
                u8 b4 = (input&0x10)?1:0;

                return (b1<<4) | (b0<<3) | (b2<<2) | (b3<<1) | (b4<<0);
            };

            auto decode_source_index = [](u16 input)
            {
                u8 b0 = (input&0x01)?1:0;
                u8 b1 = (input&0x02)?1:0;
                u8 b2 = (input&0x04)?1:0;
                u8 b3 = (input&0x08)?1:0;
                u8 b4 = (input&0x10)?1:0;

                return (b3<<4) | (b4<<3) | (b1<<2) | (b0<<1) | (b2<<0);
            };

            u16 current_sub = line;
            u16 current_ip = 0;

            u16 ret_sub{}, ret_ip{};

            auto read = [&](u16 index)
            {
                if (false);
                else if (index == 7)
                    return u16(read_inst<u8>());
                else if (index == 8)
                    return u16(registers[AX]>>8);
                else if (index == 10) // M
                    return readM();
                else if (index == 11) // R
                {
                    if (modrm_width == 16)
                        return get_r16(modrm_r);
                    else
                        return (u16)get_r8(modrm_r);
                }
                else if (index == 13)
                    return u16(0xFFFFU&ALU_MASK);
                else if (index == 14) //CR
                    return current_ip;
                else if (index == 15)
                    return u16(0x0000U);
                else if (index == 16)
                    return u16(registers[AX]&0xFF);
                else if ((index >= 0 && index <= 6) || (index >= 20 && index <= 31) || (index == 12))
                    return registers[index];
                else
                {
                    cout << std::dec << "Reading index " << index << "(" << regnames[index]<< ") not implemented." << endl;
                    std::abort();
                }
            };

            auto write = [&](u16 index, u16 data)
            {
                if (false);
                else if (index == 7); //no write! reading this is Q tho.
                else if (index == 8)
                    registers[AX] = (data<<8)|(registers[AX]&0xFF);
                else if (index == 10) //M
                    writeM(data);
                else if (index == 11) // R
                {
                    if (modrm_width == 16)
                        get_r16(modrm_r) = data;
                    else
                        get_r8(modrm_r) = data;
                }
                else if (index >= 12 && index <= 15)
                {
                    u16& tmp = registers[TMPA+(index&1)];

                    if (index&2)
                        tmp = (tmp&0xFF)|((data&0xFF)<<8);
                    else
                        tmp = (tmp&0xFF00)|(data&0xFF);
                }
                else if (index == 16)
                    registers[AX] = (data&0xFF)|(registers[AX]&0xFF00);
                else if (index <= 6 || (index >= 20 && index <= 31))
                    registers[index] = data;
                else
                {
                    cout << std::dec << "Writing index " << index << "(" << regnames[index]<< ") not implemented." << endl;
                    std::abort();
                }
            };

            u16 alu_op{}, alu_param{}, alu_NXT{};

            auto run_alu = [&]()
            {
                const u16& reg1 = registers[TMPA+alu_param];
                //cout << "ALU operation! " << alu_op << ", param=" << alu_param << ", next?=" << alu_NXT << endl;
                if (alu_op == XI)
                {
                    alu_op = XIvalue;
                }
                if(false);
                else if (alu_op == INC)
                {
                    sigma = (reg1+1);
                    setflag(OVERFLOW,sigma==ALU_HIGH_BIT);
                    setflag(AUX_CARRY,(sigma&0x0F) == 0x00);
                    setflag(ZERO,sigma==0);
                    setflag(SIGN,sigma&ALU_HIGH_BIT);
                    setflag(PARITY,byte_parity[sigma&0xFF]);
                }
                else if (alu_op == INC2) //flags?
                {
                    sigma = (reg1+2)&ALU_MASK;
                }
                else if (alu_op == DEC)
                {
                    sigma = (reg1-1);
                    setflag(OVERFLOW,reg1==ALU_HIGH_BIT);
                    setflag(AUX_CARRY,(sigma&0x0F) == 0x0F);
                    setflag(ZERO,sigma==0);
                    setflag(SIGN,sigma&ALU_HIGH_BIT);
                    setflag(PARITY,byte_parity[sigma&0xFF]);
                }
                else if (alu_op == PASS)
                {
                    sigma = reg1;
                    //setflag(CARRY, false);
                    setflag(PARITY,byte_parity[sigma&0xFF]);
                    setflag(AUX_CARRY, false);
                    setflag(ZERO,sigma==0);
                    setflag(SIGN,sigma&ALU_HIGH_BIT);
                    setflag(OVERFLOW,false);
                }
                else if (alu_op == DEC2) //flags?
                {
                    sigma = (reg1-2)&ALU_MASK;
                }
                else if (alu_op == COM1)
                {
                    sigma = (~reg1)&ALU_MASK;
                    setflag(CARRY, reg1&ALU_HIGH_BIT);
                    setflag(PARITY,byte_parity[sigma&0xFF]);
                    setflag(ZERO, reg1==0);
                    setflag(SIGN,sigma&ALU_HIGH_BIT);
                    setflag(AUX_CARRY, (reg1 ^ 1 ^ sigma) & 0x10);
                    setflag(OVERFLOW,(((~reg1) ^ sigma) & (1 ^ sigma))&ALU_HIGH_BIT);
                }
                else if (alu_op == NEG)
                {
                    sigma = (-reg1)&ALU_MASK;
                    setflag(CARRY, reg1!=0);
                    setflag(PARITY,byte_parity[sigma&0xFF]);
                    setflag(ZERO, reg1==0);
                    setflag(SIGN,sigma&ALU_HIGH_BIT);
                    setflag(AUX_CARRY, (reg1 ^ 1 ^ sigma) & 0x10);
                    setflag(OVERFLOW,(((~reg1) ^ sigma) & (1 ^ sigma))&ALU_HIGH_BIT);
                }
                else if (alu_op == AND)
                {
                    sigma = (reg1&registers[TMPB])&ALU_MASK;

                    setflag(CARRY, false);
                    setflag(PARITY,byte_parity[sigma&0xFF]);
                    setflag(AUX_CARRY, false);
                    setflag(ZERO,sigma==0);
                    setflag(SIGN,sigma&ALU_HIGH_BIT);
                    setflag(OVERFLOW,((reg1 ^ sigma) & (registers[TMPB] ^ sigma))&ALU_HIGH_BIT);
                }
                else if (alu_op == ADD)
                {
                    sigma = ((reg1&ALU_MASK)+(registers[TMPB]&ALU_MASK))&ALU_MASK;

                    //cout << "ADD: " << reg1 << "-" << registers[TMPB] << "=" << sigma << endl;
                    setflag(CARRY, (sigma&ALU_MASK)<(reg1&ALU_MASK));
                    setflag(PARITY,byte_parity[sigma&0xFF]);
                    setflag(AUX_CARRY, (reg1 ^ registers[TMPB] ^ sigma) & 0x10);
                    setflag(ZERO,sigma==0);
                    setflag(SIGN,sigma&ALU_HIGH_BIT);
                    setflag(OVERFLOW,((reg1 ^ sigma) & (registers[TMPB] ^ sigma))&ALU_HIGH_BIT);
                }
                else if (alu_op == ADC)
                {
                    u32 totalsigma = (reg1&ALU_MASK)+(registers[TMPB]&ALU_MASK)+(aluflags&CARRY?1:0);
                    sigma = totalsigma&ALU_MASK;

                    //cout << "ADC: " << reg1 << "-" << registers[TMPB] << "=" << sigma << endl;
                    setflag(PARITY,byte_parity[sigma&0xFF]);
                    //setflag(AUX_CARRY, (reg1&0xF)+(registers[TMPB]&0xF)+flag(F_CARRY) >= 0x10);
                    setflag(AUX_CARRY, (reg1&0xF)+(registers[TMPB]&0xF)+((aluflags&CARRY)?1:0) >= 0x10);
                    setflag(ZERO,sigma==0);
                    setflag(SIGN,sigma&ALU_HIGH_BIT);
                    setflag(OVERFLOW,((reg1 ^ sigma) & (registers[TMPB] ^ sigma))&ALU_HIGH_BIT);

                    //set_flag(F_CARRY, ((p1+p2+flag(F_CARRY))>>(sizeof(T)*8)) > 0);
                    setflag(CARRY, u32(totalsigma>>bitwidth)>0);
                    //setflag(CARRY, (sigma&ALU_MASK)<(reg1&ALU_MASK));
                }
                else if (alu_op == SUBT)
                {
                    //cout << std::hex << "SUBT: " << reg1 << "-" << registers[TMPB] << "=" << sigma << endl;
                    sigma = ((reg1&ALU_MASK)-(registers[TMPB]&ALU_MASK))&ALU_MASK;
                    if (current_sub+current_ip != 0x196) //UGLY HACK! but this makes div work when the divisor highest bit is set.
                    {
                        setflag(CARRY, u16(reg1&ALU_MASK)<u16(registers[TMPB]&ALU_MASK));
                        setflag(PARITY,byte_parity[sigma&0xFF]);
                        setflag(AUX_CARRY, (reg1 ^ registers[TMPB] ^ sigma) & 0x10);
                        setflag(ZERO,sigma==0);
                        setflag(SIGN,sigma&ALU_HIGH_BIT);
                        setflag(OVERFLOW,((reg1 ^ registers[TMPB]) & (reg1 ^ sigma))&ALU_HIGH_BIT);
                    }
                }
                else if (alu_op == ROL)
                {
                    sigma = (reg1 << 1)&ALU_MASK;
                    sigma |= ((reg1&ALU_HIGH_BIT)?1:0);
                    setflag(CARRY,reg1&ALU_HIGH_BIT);
                    setflag(OVERFLOW, bool(reg1&ALU_HIGH_BIT) != bool(reg1&(ALU_HIGH_BIT>>1)));
                    sigma &= ALU_MASK;
                }
                else if (alu_op == ROR)
                {
                    sigma = (reg1 >> 1)&ALU_MASK;
                    sigma |= ((reg1&1)?ALU_HIGH_BIT:0);
                    setflag(CARRY,reg1&1);
                    setflag(OVERFLOW, bool(sigma&ALU_HIGH_BIT) != bool(sigma&(ALU_HIGH_BIT>>1)));
                    sigma &= ALU_MASK;
                }
                else if (alu_op == LRCY)
                {
                    sigma = (reg1 << 1)&ALU_MASK;
                    sigma |= ((aluflags&CARRY)?1:0);
                    setflag(CARRY,reg1&ALU_HIGH_BIT);
                    setflag(OVERFLOW, bool(reg1&ALU_HIGH_BIT) != bool(reg1&(ALU_HIGH_BIT>>1)));
                    sigma &= ALU_MASK;
                }
                else if (alu_op == RRCY)
                {
                    sigma = (reg1 >> 1)&ALU_MASK;
                    sigma |= ((aluflags&CARRY)?ALU_HIGH_BIT:0);
                    setflag(CARRY,reg1&1);
                    setflag(OVERFLOW, bool(sigma&ALU_HIGH_BIT) != bool(sigma&(ALU_HIGH_BIT>>1)));
                    sigma &= ALU_MASK;
                }
                else if (alu_op == SHL)
                {
                    sigma = (reg1 << 1)&ALU_MASK;
                    setflag(CARRY,reg1&ALU_HIGH_BIT);
                    setflag(OVERFLOW, bool(reg1&ALU_HIGH_BIT) != bool(reg1&(ALU_HIGH_BIT>>1)));
                    setflag(SIGN, sigma&ALU_HIGH_BIT);
                    setflag(ZERO, sigma==0);
                    setflag(AUX_CARRY, sigma&0x10);
                    setflag(PARITY, byte_parity[sigma&0xFF]);
                }
                else if (alu_op == SHR)
                {
                    sigma = (reg1 >> 1)&ALU_MASK;
                    setflag(CARRY,reg1&1);
                    setflag(OVERFLOW, bool(sigma&ALU_HIGH_BIT) != bool(sigma&(ALU_HIGH_BIT>>1)));
                    setflag(SIGN, sigma&ALU_HIGH_BIT);
                    setflag(ZERO, sigma==0);
                    setflag(AUX_CARRY, false);
                    setflag(PARITY, byte_parity[sigma&0xFF]);
                }
                else if (alu_op == SETMO)
                {
                    sigma = ALU_MASK;
                    setflag(CARRY, false);
                    setflag(OVERFLOW, false);
                    setflag(SIGN, true);
                    setflag(ZERO, false);
                    setflag(AUX_CARRY, false);
                    setflag(PARITY, byte_parity[0xFF]);
                }
                else if (alu_op == SAR)
                {
                    sigma = ((reg1 >> 1)&ALU_MASK) | (reg1&ALU_HIGH_BIT);
                    setflag(CARRY,reg1&1);
                    setflag(OVERFLOW, bool(sigma&ALU_HIGH_BIT) != bool(sigma&(ALU_HIGH_BIT>>1)));
                    setflag(SIGN, sigma&ALU_HIGH_BIT);
                    setflag(ZERO, sigma==0);
                    setflag(AUX_CARRY, false);
                    setflag(PARITY, byte_parity[sigma&0xFF]);
                }
                else if (alu_op == DAA)
                {
                    u8 old_val = reg1&0xFF;
                    bool weird_special_case = (!(aluflags&CARRY)) && (aluflags&AUX_CARRY);

                    u8 added{};

                    setflag(AUX_CARRY, (old_val & 0x0F) > 9 || (aluflags&AUX_CARRY));
                    if (aluflags&AUX_CARRY)
                        added += 0x06;

                    setflag(CARRY, old_val > 0x99+(weird_special_case?6:0) || (aluflags&CARRY));
                    if (aluflags&CARRY)
                        added += 0x60;

                    sigma = old_val+added;

                    setflag(OVERFLOW, (added ^ sigma) & (old_val ^ sigma)&0x80);
                    setflag(ZERO, (sigma&0xFF) == 0);
                    setflag(SIGN, (sigma & 0x80));
                    setflag(PARITY, byte_parity[sigma&0xFF]);
                }
                else if (alu_op == DAS)
                {
                    u8 old_val = reg1&0xFF;
                    bool weird_special_case = (!(aluflags&CARRY)) && (aluflags&AUX_CARRY);

                    u8 added{};

                    setflag(AUX_CARRY, (old_val & 0x0F) > 9 || (aluflags&AUX_CARRY));
                    if (aluflags&AUX_CARRY)
                        added += -0x06;

                    setflag(CARRY, old_val > (0x99+(weird_special_case?6:0)) || (aluflags&CARRY));
                    if (aluflags&CARRY)
                        added += -0x60;

                    sigma = old_val+added;
                    setflag(OVERFLOW, ((old_val ^ -added) & (old_val ^ sigma))&0x80);
                    setflag(ZERO, (sigma&0xFF) == 0);
                    setflag(SIGN, (sigma & 0x80));
                    setflag(PARITY, byte_parity[sigma & 0xFF]);
                }
                else if (alu_op == AAA)
                {
                    u16 old_AX = reg1;
                    bool add_ax = (old_AX & 0x0F) > 9 || (aluflags&AUX_CARRY);
                    sigma = old_AX;
                    if (add_ax)
                    {
                        sigma = ((old_AX+0x06)&0xFF) + ((old_AX&0xFF00)+0x100);
                    }
                    u16 added = sigma-old_AX;

                    setflag(AUX_CARRY, add_ax);
                    setflag(CARRY, add_ax);
                    setflag(PARITY, byte_parity[sigma&0xFF]);
                    setflag(ZERO, (sigma&0xFF) == 0);
                    setflag(SIGN, (sigma & 0x80));
                    setflag(OVERFLOW, (old_AX ^ sigma) & (added ^ sigma)&0x80);

                    sigma &= 0xFF0F;
                }
                else if (alu_op == AAS)
                {
                    u16 old_AX = reg1;
                    bool add_ax = (old_AX & 0x0F) > 9 || (aluflags&AUX_CARRY);
                    sigma = old_AX;
                    if (add_ax)
                    {
                        sigma = ((old_AX-0x06)&0xFF) + ((old_AX&0xFF00)-0x100);
                    }
                    u16 added = old_AX-sigma;

                    setflag(AUX_CARRY, add_ax);
                    setflag(CARRY, add_ax);
                    setflag(PARITY, byte_parity[sigma&0xFF]);
                    setflag(ZERO, (sigma&0xFF) == 0);
                    setflag(SIGN, (sigma & 0x80));
                    setflag(OVERFLOW, (old_AX ^ sigma) & (added ^ old_AX)&0x80);

                    sigma &= 0xFF0F;
                }
                else
                {
                    //cout << "Unknown alu op number " << std::dec << u32(alu_op) << endl;
                    //std::abort();
                }
                //sigma &= ALU_MASK;
            };

            while(true)
            {
                ++cycles_used;
                u16 current_opcode = current_sub+current_ip;

                //hacks for instructions that are split
                if (current_sub == 0x11C && current_ip >= 4)
                    current_opcode = 0x1ec+current_ip;
                else if (current_sub == 0x120 && current_ip >= 12)
                    current_opcode = 0x1e8+current_ip;
                else if (current_sub == 0x12c && current_ip >= 8)
                    current_opcode = 0x1f0+current_ip;

                //if (do_spam)
                //    cout << std::hex << "MC: " << current_opcode << " " << registers[TMPA] << " " << registers[TMPB] << " " << registers[TMPC] << endl;
                if (current_opcode == INT0) //special case?
                {
                    bitwidth = 16;
                    set_width();
                }
                u32 instr = mc8088[current_opcode];

                /*std::cout << std::hex << current_opcode <<": " << aluflags << ":" << registers[FLAGS] << std::endl;

#define preg(x) cout << #x << std::hex << "=" << registers[x] << " ";
                preg(TMPA);
                preg(TMPB);
                preg(TMPC);
                preg(SP);
                preg(SIGMA);
                preg(IND);
                preg(FLAGS)
#undef preg
				cout << " alucarry=" << ((aluflags&CARRY)?1:0)  << " carry=" << ((registers[FLAGS]&CARRY)?1:0);
                cout << endl;

                cout << "EXECUTING " << std::hex << current_opcode << ". " << endl;*/

                u16 target_id = decode_target_index((instr>>16)&0x1F);
                u16 source_id = decode_source_index((instr>>11)&0x1F);
                bool update_flags = (instr>>10)&0x01;

                if (source_id == 12)
                {
                    //cout << "ALU run: " << std::hex << current_opcode << endl;
                    //aluflags = registers[FLAGS];
                    run_alu();
                }
                u16 operation = instr&0x3ff;
                bool next_opcode{};

                current_ip = (current_ip+1)&0x0F;

                //cout << "move " << regnames[source_id] << "->" << regnames[target_id] << endl;

                write(target_id, read(source_id));

                if (update_flags)
                {
                    registers[FLAGS] = aluflags;
                }
                //cout << std::hex << current_opcode << ": " << regnames[source_id] << " -> " << regnames[target_id] << endl;//" " << (update_flags?'F':'_') << ", op=" << std::hex << operation << endl;

                u16 optype{};
                u16 opdata{};

                if (operation&0x200) //is 4,5,6,7
                    optype = (operation>>7)&0x07, opdata=(operation&0x7F);
                else
                    optype = (operation>>8)&0x03, opdata=(operation&0xFF);

                if (optype == 0) //short jump
                {

                    ++cycles_used; //extra cycle for jump
                    u16 condition = opdata>>4;
                    u16 jump_target = opdata&0xF;

                    bool do_jump{};

                    if (false);
                    else if (condition == 0x00) //F1 ^ Z - repne=1 repe=2
                        do_jump = ((string_prefix==2) && !bool(registers[FLAGS]&ZERO)) || (string_prefix==1 && bool(registers[FLAGS]&ZERO));// != bool(aluflags&ZERO));
                    else if (condition == 0x02) //jump if 8-bit width
                        do_jump = (bitwidth==8);
                    else if (condition == 0x03) //zero
                        do_jump = (aluflags&ZERO);
                    else if (condition == 0x04)
                    {
                        //cout << "counter: " << counter << endl;
                        do_jump = (counter!=0);
                        counter -= (do_jump?1:0);
                    }
                    else if(condition == 0x07)
                        do_jump = (aluflags&CARRY);
                    else if (condition == 0x08)
                        do_jump = true; //unconditional jump!
                    else if (condition == 0x09)
                        do_jump = !f1; //jump if not REP
                    else if (condition == 0x0A)
                        do_jump = !(aluflags&ZERO); //not zero
                    else if (condition == 0x0B)
                        do_jump = X0;
                    else if (condition == 0x0C)
                        do_jump = !(aluflags&CARRY);
                    else if (condition == 0x0D)
                        do_jump = f1; //jump if "either" f1 flag is active? wtf?
                    else
                    {
                        cout << std::hex << current_opcode << ": Unknown short jump condition=" << condition << endl;
                        std::abort();
                    }

                    //cout << "JUMP cond=" << condition<< " to " << jump_target << endl;
                    if (do_jump)
                    {
                        //cout << "Jump taken." << endl;
                        current_ip = jump_target;
                    }
                }
                else if (optype == 1) //ALU
                {
                    alu_op = (opdata>>3);
                    alu_param = (opdata>>1)&0x03;
                    alu_NXT = (opdata&0x01);
                    //cout << "ALU primed: " << std::hex << current_opcode << endl;
                }
                else if (optype == 4) //bookkeeping, misc
                {
                    u16 op1 = (opdata>>3)&0x0F;
                    u16 op2 = (opdata&0x07);
                    //cout << "Bookkeeping op " << op1 << ":" << op2 << endl;
                    if (false);
                    else if (op1 == 0)
                    {
                        counter = bitwidth-1;
                    }
                    else if (op1 == 1) //flush prefetch queue
                    {
                        //TODO: do this!
                    }
                    else if (op1 == 2) //flip f1 flag
                    {
                        f1 = !f1;
                    }
                    else if (op1 == 3) //clear interrupt, trap
                    {
                        aluflags &= ~INTERRUPTFLAG;
                        aluflags &= ~TRAP;
                        registers[FLAGS] &= ~INTERRUPTFLAG;
                        registers[FLAGS] &= ~TRAP;
                    }
                    else if (op1 == 4)
                    {
                        aluflags &= ~CARRY;
                        registers[FLAGS] &= ~CARRY;
                    }
                    else if (op1 == 6)
                    {
                        aluflags &= ~CARRY;
                        aluflags &= ~OVERFLOW;
                        registers[FLAGS] &= ~CARRY;
                        registers[FLAGS] &= ~OVERFLOW;
                    }
                    else if (op1 == 7)
                    {
                        aluflags |= CARRY|OVERFLOW;
                        registers[FLAGS] |= CARRY|OVERFLOW;
                    }
                    else if (op1 == 15); //NOP
                    else
                    {
                        cout << std::hex << current_opcode << ": op1=" << op1 << " not known" << endl;
                        std::abort();
                    }

                    if (false);
                    else if (op2==0x00) //NEXT OPCODE! :)
                    {
                        next_opcode=true;
                    }
                    else if (op2 == 0x02)
                    {
                        //cout << "FIX INSTRUCTION POINTER" << endl; //TODO: implement this 4reals
                    }
                    else if (op2 == 0x03)
                    {
                        //cout << "SUSPEND FETCHING." << endl; //TODO: implement this 4reals
                    }
                    else if (op2==0x04)
                    {
                        //return!
                        current_sub = ret_sub;
                        current_ip = ret_ip;
                    }
                    else if (op2==0x05)
                    {
                        //don't write back ea, start processing next opcode
                    }
                    else if (op2==0x07); //NOP
                    else
                    {
                        cout << "op2=" << op2 << " not known" << endl;
                        std::abort();
                    }
                }
                else if (optype == 6) // memory read/write
                {
                    bool is_write = opdata&0x40;
                    cycles_used += is_write?3:4;
                    if (bitwidth == 16)
                        cycles_used += 4;
                    bool interrupt_ack = opdata&0x20;
                    next_opcode = opdata&0x10;

                    u16 seg_id       = (opdata>>2)&0x03; //ES, ZERO, SS, DS(overridable)
                    u16 segment{};

                    switch(seg_id)
                    {
                        case 0: segment = registers[ES]; break;
                        case 1: segment = 0; break;
                        case 2: segment = registers[SS]; break;
                        case 3: segment = registers[get_segment(DS)]; break;
                    }

                    u16 addr_factor = (opdata&0x03); //P2, BL, M2, P0

                    const char* const names[4] = {"p2", "bl", "m2", "p0"};

                    const i16 add[4] = {2, 0, -2, 0};

                    //print_regs();
                    //cout <<std::hex << "Memory " << (is_write?"write":"read") << " " << seg_id << " " << addr_factor << ". addr=" << segment << ":" << registers[IND] << " = " << segment*16+registers[IND] << endl;
                    //cout << names[addr_factor] << endl;

                    if (is_write)
                    {
                        mem._8(segment, registers[IND]) = registers[OPR];
                        if (bitwidth==16)
                            mem._8(segment, registers[IND]+1) = registers[OPR]>>8;
                    }
                    else
                    {
                        registers[OPR] = mem._8(segment, registers[IND]);
                        if (bitwidth==16)
                            registers[OPR] |= (mem._8(segment, registers[IND]+1)<<8);
                    }

                    //addr_factor==1, modify IND according to word size and direction flag
                    if (addr_factor == 1)
                    {
                        registers[IND] += (aluflags&DIRECTIONAL)?-(bitwidth>>3):(bitwidth>>3);
                    }
                    else
                        registers[IND] += add[addr_factor];
                    //cout << "OPR is now " << std::hex << registers[OPR] << ", new IND " << registers[IND] << endl;
                }
                else if (optype == 5 || optype == 7) // long jump/call
                {
                    ++cycles_used;
                    //conds:
                    //0=UNC, 1=????, 2=NZ, 3=X0, 4=NCY, 5=F1, 6=INT, 7=XC

                    u16 cond = (opdata>>4)&0x07;
                    u16 target_address = (optype==5?lj_table:lc_table)[opdata&0x0F];
                    //cout << std::hex << "Long " << (optype==5?"jump":"call") << ", cond=" << cond << ", target_address=" << target_address << endl;

                    bool do_jump{};

                    if (false);
                    else if (cond == 0)
                        do_jump = true;
                    else if (cond == 3)
                        do_jump = X0;
                    else if (cond == 4)
                        do_jump = !(aluflags&CARRY);
                    else if (cond == 5)
                        do_jump = f1;
                    else if (cond == 6)
                        do_jump = false; //TODO! FIXME! this is jumped if interrupt is pending!
                    else
                    {
                        std::cout << std::hex << current_opcode << " cond=" << cond << " not implemented." << endl;
                        std::abort();
                    }

                    if (do_jump)
                    {
                        if (optype == 7)
                        {
                            ret_sub = current_sub;
                            ret_ip = current_ip;
                        }
                        current_sub = target_address-mc8088_localaddr[target_address];
                        current_ip = mc8088_localaddr[target_address];
                    }
                }
                if (next_opcode)
                {
                    return;
                }
            }
        };

        execute(start_address);
    }

    u16 registers[32] = {};

    u8 segment_override{};
    u8 string_prefix{};
    u8 lock{};
    u32 delay{};
    u32 cpu_steps{};

    enum SP_VALUES
    {
        SP_REPNZ = 1,
        SP_REPZ = 2 //also REP
    };

    static constexpr u16 registermap[14] = //i wish we didnt need this
    {
        AX,CX,DX,BX, SP,BP,SI,DI, ES,CS,SS,DS, FLAGS, IP
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
        for(u32 i=0; i<32; ++i)
            registers[i] = 0x0000;
        registers[CS] = ~registers[CS]; //set code segment to 0xFFFF for reset
        registers[unk13] = ~registers[unk13]; //uh, what is this?
    }


    u8 prefetch_queue_size = 6;
    u8 prefetch_queue[6] = {};
    u32 prefetch_address{0xFFFFFFFF};

    bool do_prefetch_delay{};
    template<typename T>
    T read_inst() requires integral<T>
    {
        T result{};
        u32 position = ((registers[CS]<<4) + registers[IP])&0xFFFFF;
        if (prefetch_address != position)
        {
            prefetch_address = position;
            for(u32 i=0; i<prefetch_queue_size; ++i)
            {
                prefetch_queue[i] = mem._8(registers[CS], registers[IP]+i);
            }
        }

        result = *(T*)(prefetch_queue);
        prefetch_address += sizeof(T);
        registers[IP] += sizeof(T);

        for(u32 i=0; i<prefetch_queue_size-sizeof(T); ++i)
        {
            prefetch_queue[i] = prefetch_queue[i+sizeof(T)];
        }
        for(u32 i=prefetch_queue_size-sizeof(T); i<prefetch_queue_size; ++i)
        {
            prefetch_queue[i] = mem._8(registers[CS], registers[IP]+i);
        }
        if (startprinting)
        {
            //cout << (sizeof(T)==2?"w":"b") << u32(result) << " ";
        }
        //do_prefetch_delay = !do_prefetch_delay;
        //cycles_used += 2*sizeof(T);
        //std::cout << "used " << 2*sizeof(T) << " for read_inst()" << std::endl;
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
        //std::cout << "used " << u32(effective_address_cycles[(mod<<3)+rm]) << " for EA" << std::endl;

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
				if (segment_override == 0)
                    segment_override = SS+8;
            }
		}
		//cycles_used += 4;//((offset&0x01)<<2); //4 cycles for odd accesses
        segment = registers[get_segment(segment)];
        modrm_seg = segment;
        modrm_offset = offset;
    }

    u8& decode_modrm_u8(u8 modrm)
    {
        modrm_width = 8;
        u8 mod = (modrm >> 6) & 0x03;
        modrm_r = (modrm >> 3) & 0x07;
        u8 rm = modrm & 0x07;
        modrm_is_register = (mod==0x03);
		if (mod == 0x03)
        {
            modrm_reg = rm;
			return get_r8(rm);
        }
        u16 offset{}, segment{};
        decode_modrm(mod,rm,segment,offset);
        return mem._8(segment, offset);
    }

    u16& decode_modrm_u16(u8 modrm)
    {
        modrm_width = 16;
        u8 mod = (modrm >> 6) & 0x03;
        modrm_r = (modrm >> 3) & 0x07;
        u8 rm = modrm & 0x07;
        modrm_is_register = (mod==0x03);
		if (mod == 0x03)
        {
            modrm_reg = rm;
			return get_r16(rm);
        }
        u16 offset{}, segment{};
        decode_modrm(mod,rm,segment,offset);
        return mem._16(segment, offset);
    }

    void decode_modrm_mc(u8 modrm)
    {
        u8 mod = (modrm >> 6) & 0x03;
        modrm_r = (modrm >> 3) & 0x07;
        u8 rm = modrm & 0x07;
        modrm_is_register = (mod==0x03);
		if (mod == 0x03)
        {
            modrm_reg = rm;
            return;
        }
        u16 offset{}, segment{DS};
        decode_modrm(mod,rm,segment,offset);
        registers[IND] = offset;
        registers[OPR] = mem._8(segment,offset);
        registers[OPR] |= mem._8(segment,offset+1)<<8;
    }

    u32 effective_address(u8 modrm)
    {
        u8 mod = (modrm >> 6) & 0x03;
        u8 rm = modrm & 0x07;
        modrm_is_register = (mod==0x03);
		if (mod == 0x03)
        {
            cout << "Loading effective address of a register? are you gone mad?" << endl;
            return 0;
        }
        u16 offset{}, segment{};
        decode_modrm(mod,rm,segment,offset);
        return offset | (segment << 16);
    }


    u8* reg8() { return (u8*)(void*)(registers); }

    u16& get_r16(u8 value)
    {
        return registers[AX+(value&0x7)];
    }
    u8& get_r8(u8 value)
    {
        return reg8()[AX*2+(((value&0x3)<<1)+((value&0x4)>>2))];
    }

    u16& get_segment_r16(u8 value)
    {
        return registers[value&0x03];
    }

    void print_regs()
    {
        for(int i=0; i<8; ++i)
            std::cout << " " << r16_names[i] << "=" << std::setw(4) << std::setfill('0') << registers[i+24];
        for(int i=0; i<4; ++i)
            std::cout << " " << seg_names[i] << "=" << std::setw(4) << std::setfill('0') << registers[i];
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
        return segment_override?(segment_override-8):default_segment;
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
            //std::cout << "used " << 2 << " for seg override" << std::endl;
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
    bool inhibit_ss{};
    u32 interrupt_table[256] = {};

    bool accepts_interrupts()
    {
        return interrupt_true_cycles >= 2 && !inhibit_ss && (delay==0 || halt);
    }

    void interrupt(u8 n, bool forced=false)
    {
        if (inhibit_ss)
            return;
        if (flag(F_INTERRUPT) || forced)
        {
            if (startprinting)
                cout << "INTERRUPT " << std::hex << u32(n) << "!" << endl;
            halt = false;
            cycles_used += 80;
            delay = 0;

            ++interrupt_table[n];

            push(registers[FLAGS]);
            push(registers[CS]);
            push(registers[IP]);
            mem.update();

            registers[IP] = mem._16(0, n*4);
            registers[CS] = mem._16(0, n*4+2);
            set_flag(F_INTERRUPT,false);
            set_flag(F_TRAP,false);
            if (!forced)
            {
                if constexpr(DEBUG_LEVEL > 0)
                    cout << "IRQ: CPU ACK " << u32(n-8) << endl;
                pic.cpu_ack_irq(n-8);
            }
        }
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
        bool previous_trap = flag(F_TRAP);
        ++cycles;

        if (delay)
        {
            --delay;
            return;
        }
        mem.update();
        inhibit_ss = false;
        if (halt)
        {
            return;
        }

        if (registers[CS] == 0 && registers[IP] == 0)
        {
            cout << "Trying to run code at CS:IP 0:0... resetting." << endl;
            reset();
        }

        u16 original_ip = registers[IP];

        is_inside_multi_part_instruction = false;
        globalsettings.current_IP = registers[CS]*16+registers[IP];

        //if (globalsettings.current_IP == 0x7C00)
        //    startprinting = true;

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
                    //std::cout << "used " << (modrm_is_register?3:13) << " for inst=" << u32(instruction) << std::endl;
                }
                else //towards modrm byte
                {
                    cycles_used += (modrm_is_register?3:24);
                    //std::cout << "used " << (modrm_is_register?3:24) << " for inst=" << u32(instruction) << std::endl;
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
                inhibit_ss = true;
            }
            else
            {
                cycles_used += 14;
                push(reg);
            }
        }
        //void mc_execute(u16 start_address, u8 bitwidth, bool X0, bool f1, u8 XIvalue)
        else if (instruction == 0x27 || instruction == 0x2F) // DAA DAS
        {
            mc_execute(0x144, 16, instruction&0x08, false, instruction==0x27?20:21);
        }
        else if (instruction == 0x37 || instruction == 0x3F) // AAA AAS
        {
            mc_execute(0x148, 16, instruction&0x08, false, instruction==0x37?22:23);
        }
        else if ((instruction&0xF0) == 0x40) //INC/DEC register
        {
            u16 result = get_r16(instruction&0x07)+(1-((instruction&0x08)?2:0));
            set_flag(F_SIGN, result&0x8000);
            set_flag(F_ZERO, result==0);
            set_flag(F_AUX_CARRY, (result&0x0F) == ((instruction&0x08)?0x0F:0x00));
            set_flag(F_PARITY, byte_parity[result&0xFF]);
            set_flag(F_OVERFLOW, result==0x8000-((instruction&0x08)?1:0));
            get_r16(instruction&0x07) = result;
            cycles_used += 3;
        }
        else if ((instruction&0xF8) == 0x50) // push reg
        {
            if (instruction == 0x54) //push SP
            {
                push(get_r16(instruction&0x07)-2);
            }
            else
            {
                push(get_r16(instruction&0x07));
            }
            cycles_used += 15;
        }
        else if ((instruction&0xF8) == 0x58) //pop reg
        {
            get_r16(instruction&0x07) = pop();
            cycles_used += 12;
        }
        else if ((instruction&0xE0) == 0x60) //various short jumps, note that 0x60-6F is mapped to 0x70-7F
        {
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
            r = (effective_address(modrm)&0xFFFF);
            cycles_used += 2;
        }
        else if (instruction == 0x8E) // MOV SwEw
        {
            u8 modrm = read_inst<u8>();
            u16& rm = decode_modrm_u16(modrm);
            u16& r = get_segment_r16((modrm>>3)&0x07);
            r = rm;
            inhibit_ss = true;
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
            u16 tmp = get_r16(AX);
            get_r16(AX) = get_r16(reg_id);
            get_r16(reg_id) = tmp;
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
            halt = true; // we don't have 8087 so we wait for an interrupt. it's not true halt but close enough.
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
            registers[FLAGS] = (registers[FLAGS]&~FLAG_MASK) | (newflags&FLAG_MASK) | 0xF002;
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
        /*else if (instruction >= 0xA0 && instruction <= 0xA3) //AL/X=MEM  MEM=AL/X
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
        }*/
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
        else if (instruction >= 0xA0 && instruction <= 0xAF)
        {
            modrm_is_register = true;
            modrm_width = (instruction&1)?16:8;
            modrm_reg = 0;

            const u16 programs[16] = // lol actual microcode
            {
                0x060, 0x060, 0x064, 0x064, 0x12C, 0x12C, 0x120, 0x120,
                0x09C, 0x09C, 0x11C, 0x11C, 0x12C, 0x12C, 0x120, 0x120
            };

            mc_execute(programs[instruction&0x0F], modrm_width, (instruction&0x08), string_prefix, 0);
        }
        else if ((instruction&0xF0) == 0xB0) //mov reg8, Ib/Iv
        {
            modrm_is_register = true;
            modrm_reg = (instruction&0x07);
            modrm_width = (instruction&0x08)?16:8;
            mc_execute(0x1C, modrm_width, (instruction&0x08), string_prefix, 0);
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
            decode_modrm_mc(modrm);
            modrm_width = 16;
            mc_execute((instruction&0x01)?0xF4:0xF0, modrm_width, (instruction&0x08), string_prefix, 0);

            /*u8 modrm = read_inst<u8>();
            //u16& rm = decode_modrm_u16(modrm);

            u32 addr = effective_address(modrm);
            u16& r = get_r16((modrm>>3)&0x07);
            r = mem._16(addr>>16, addr&0xFFFF);
            registers[(instruction&1)?DS:ES] = mem._16(addr>>16, (addr&0xFFFF)+2); //ES or DS, based on the opcode
            cycles_used += 24;*/
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
            if (startprinting)
                cout << "Calling interrupt 3... AX=" << registers[AX] << endl;
            interrupt(3, true);
            //cycles_used += 72;
        }
        else if (instruction == 0xCD) // INT imm8
        {
            u8 int_num = read_inst<u8>();
            if (startprinting)
                cout << "Calling interrupt... " << u32(int_num) << " AX=" << registers[AX] << endl;
            interrupt(int_num, true);
            //cycles_used += 71;
        }
        else if (instruction == 0xCE) // INTO
        {
            if (flag(F_OVERFLOW))
            {
                if (startprinting)
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
            registers[FLAGS] = (registers[FLAGS]&~FLAG_MASK) | (newflags&FLAG_MASK) | 0xF002;

            if (startprinting)
                cout << "RETURN FROM INTERRUPT to " << registers[IP]<< ":" << registers[CS] << "|" << newflags << endl;
            cycles_used += 44;
        }
        else if (instruction >= 0xD0 && instruction <= 0xD1)
        {
            u8 modrm = read_inst<u8>();
            decode_modrm_mc(modrm);
            mc_execute(0x088, (instruction&0x01)?16:8, 0, 0, modrm_r+8);
        }
        else if (instruction >= 0xD2 && instruction <= 0xD3)
        {
            u8 modrm = read_inst<u8>();
            decode_modrm_mc(modrm);
            mc_execute(0x08C, (instruction&0x01)?16:8, 0, 0, modrm_r+8);
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
                if (inst_type == 6)
                {
                    rm = 0xFF;
                    set_flag(F_CARRY, false);
                    set_flag(F_OVERFLOW, false);
                    set_flag(F_SIGN, true);
                    set_flag(F_ZERO, false);
                    set_flag(F_AUX_CARRY, false);
                    set_flag(F_PARITY, byte_parity[0xFF]);
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
                if (inst_type == 6)
                {
                    rm = 0xFFFF;
                    set_flag(F_CARRY, false);
                    set_flag(F_OVERFLOW, false);
                    set_flag(F_SIGN, true);
                    set_flag(F_ZERO, false);
                    set_flag(F_AUX_CARRY, false);
                    set_flag(F_PARITY, byte_parity[0xFF]);
                    break;
                }
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
            mc_execute(0x174, 8, 0, 0, 0);
        }
        else if (instruction == 0xD5) // AAD
        {
            mc_execute(0x170, 8, 0, 0, 0);
        }
        else if (instruction == 0xD6) // SALC (undocumented!)
        {
            mc_execute(0x0a0, 8, 0, 0, 0);
        }
        else if (instruction == 0xD7) // XLAT
        {
            mc_execute(0x10C, 16, 0, 0, 0);
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
            get_r8(0) = iosystem.io_in<u8>(read_inst<u8>());
            cycles_used += 14;
        }
        else if (instruction == 0xE5) // IN
        {
            u8 port = read_inst<u8>();
            u8 low = iosystem.io_in<u8>(port);
            u8 high = iosystem.io_in<u8>(port+1);
            registers[AX] = (high<<8)|low;
            cycles_used += 14;
        }
        else if (instruction == 0xE6) // OUT
        {
            iosystem.io_out<u8>(read_inst<u8>(), registers[AX]&0xFF);
            cycles_used += 14;
        }
        else if (instruction == 0xE7) // OUT
        {
            u8 port = read_inst<u8>();
            iosystem.io_out<u8>(port, registers[AX]&0xFF);
            iosystem.io_out<u8>(port+1, registers[AX]>>8);
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
            get_r8(0) = iosystem.io_in<u8>(registers[DX]);
            cycles_used += 12;
        }
        else if (instruction == 0xED) // IN
        {
            u16 port = registers[DX];
            u8 low = iosystem.io_in<u8>(port);
            u8 high = iosystem.io_in<u8>(port+1);
            registers[AX] = (high<<8)|low;
            cycles_used += 12;
        }
        else if (instruction == 0xEE) // OUT
        {
            iosystem.io_out<u8>(registers[DX], registers[AX]&0xFF);
            cycles_used += 12;
        }
        else if (instruction == 0xEF) // OUT
        {
            iosystem.io_out<u8>(registers[DX], registers[AX]&0xFF);
            iosystem.io_out<u8>(registers[DX]+1, registers[AX]>>8);
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
            modrm_width = 8;
            decode_modrm_mc(modrm);
            u8 op = ((modrm>>3)&0x07);

            //cout << "DIV: " << std::dec << get_r16(AX)<< "/" << u32(*regM8) << std::hex << endl;
            //cout << "DIV: 0x" << std::hex << get_r16(AX)<< "/0x" << u32(*regM8) << std::hex << endl;

            const u16 start_addr[8] =
            {
                0x098, 0x098, 0x04c, 0x050, 0x150, 0x150, 0x160, 0x160,
            };
            mc_execute(start_addr[op], 8, op&1, string_prefix, 0x18+op);
        }
        else if(instruction == 0xF7) //word param
        {
            u8 modrm = read_inst<u8>();
            modrm_width = 16;
            decode_modrm_mc(modrm);
            u8 op = ((modrm>>3)&0x07);

            const u16 start_addr[8] =
            {
                0x098, 0x098, 0x04c, 0x050, 0x158, 0x158, 0x168, 0x168,
            };
            mc_execute(start_addr[op], 16, op&1, string_prefix, 0x18+op);
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
            if (op == 0 || op == 1)
            {
                set_flag(F_OVERFLOW,result==0x80-op);
                set_flag(F_AUX_CARRY,(result&0x0F) == ((op&0x01)?0x0F:0x00));
                set_flag(F_ZERO,result==0);
                set_flag(F_SIGN,result&0x80);
                set_flag(F_PARITY,byte_parity[result&0xFF]);
                //no carry!
                reg = result;
                cycles_used += (modrm_is_register?3:23);
            }
            else if (op >= 2)
            {
                cout << "*" << u32(instruction) << "-" << u32(modrm);
                //std::cout << "Invalid opcode combo: 0x" << u32(instruction) << " 0x" << u32(modrm) << std::endl;
                //std::abort();

                const u16 fecyclesreg[8] = {0, 0, 3,20,20,11,11,15};
                const u16 fecyclesmem[8] = {0, 0, 23,29,53,18,24,24};

                cycles_used += modrm_is_register?fecyclesreg[op]:fecyclesmem[op];
            }
        }
        else if (instruction == 0xFF)
        {
            u8 modrm = read_inst<u8>();
            u8 op = (modrm>>3)&0x07;
            if (op == 0 || op == 1)
            {
                u16& reg = decode_modrm_u16(modrm);
                u16 result = reg+1-(op<<1);
                set_flag(F_OVERFLOW,result==(0x8000-op));
                set_flag(F_AUX_CARRY,(result&0x0F) == ((op&0x01)?0x0F:0x00));
                set_flag(F_ZERO,result==0);
                set_flag(F_SIGN,result&0x8000);
                set_flag(F_PARITY,byte_parity[result&0xFF]);
                reg = result;
                cycles_used += (modrm_is_register?3:23);
            }
            else if (op == 2) //call near
            {
                u16& reg = decode_modrm_u16(modrm);
                u16 address = reg;
                push(registers[IP]);
                registers[IP] = address; //have to do this because reg could be SP :')
                cycles_used += (modrm_is_register?20:29);
            }
            else if (op == 3) //call far
            {
                u32 addr = effective_address(modrm);
                u16 address = mem._16(addr>>16, addr&0xFFFF);
                u16 segment = mem._16(addr>>16, (addr&0xFFFF)+2);
                push(registers[CS]);
                push(registers[IP]);
                registers[CS] = segment;
                registers[IP] = address;
                cycles_used += (modrm_is_register?20:53);
            }
            else if (op == 4) //jmp near
            {
                u16& reg = decode_modrm_u16(modrm);
                registers[IP] = reg;
                cycles_used += (modrm_is_register?11:18);
            }
            else if (op == 5) //jmp far
            {
                u32 addr = effective_address(modrm);
                u16 address = mem._16(addr>>16, addr&0xFFFF);
                u16 segment = mem._16(addr>>16, (addr&0xFFFF)+2);
                registers[CS] = segment;
                registers[IP] = address;
                cycles_used += (modrm_is_register?11:24);
            }
            else if (op == 6 || op == 7)
            {
                u16& reg = decode_modrm_u16(modrm);
                if ((modrm&0b11000111) == 0b11000100) //reg is SP
                {
                    push(reg-2);
                }
                else
                {
                    push(reg);
                }
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

        mem.update();

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
            //cout << "Instruction without timing: " << u32(instruction) << endl;
            //std::abort();
        }
    }
};


