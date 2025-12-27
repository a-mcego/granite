#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>

using u8 = uint8_t;
using u16 = uint16_t;

// Microcode instruction structure
struct MicroOp
{
    uint16_t bits{};
    std::string comment;
};

MicroOp parseLine(const std::string& line)
{
    MicroOp op;

    op.comment = (line.size() > 19?line.substr(19):"");

    if (op.comment.size() > 1)
    {
        op.comment = "-------"+op.comment+"-------";
    }

    for (size_t i = 0; i < 16; i++)
    {
        char c = line[i];
        op.bits <<= 1;
        if (c >= 'A' && c <= 'P')
        {
            op.bits |= 1;
        }
    }
    return op;
}

std::string opString(u16 op)
{
    std::string ret;
    for(int i=0; i<16; ++i)
    {
        ret += ((op>>(15-i))&1)?'A'+i:' ';
    }
    return ret;
}

std::string opStringNoSpace(u16 op)
{
    std::string ret;
    for(int i=0; i<16; ++i)
    {
        if (((op>>(15-i))&1))
        {
            ret += 'A'+i;
        }
    }
    return ret;
}

std::string make_cond_string(int cond_int)
{
    int cond_code = (cond_int>>1);
    std::stringstream ss;
    ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << cond_code;
    std::string cond = ss.str();
    if (false);
    else if (cond_code == 0x00) cond += " is_memory_operand";
    else if (cond_code == 0x01) cond += " (opcode&0x4C0) == 0x4C0";
    else if (cond_code == 0x02) cond += " (opcode&0x4C0) == 0x0C0";
    else if (cond_code == 0x03) cond += " (opcode&0x200) == 0, fld not fild";
    else if (cond_code == 0x04) cond += " f32 not f64";
    else if (cond_code == 0x06) cond += " pop_at_end";
    else if (cond_code == 0x0a) cond += " opcode&1";
    else if (cond_code == 0x0b) cond += " double_pop_at_end";
    else if (cond_code == 0x1e) cond += " [0x10]>0";
    else if (cond_code == 0x20) cond += " tmpA==0";
    else if (cond_code == 0x22) cond += " tmpA bad?";
    else if (cond_code == 0x23) cond += " stack overflow";
    else if (cond_code == 0x24) cond += " tmpB==0";
    else if (cond_code == 0x26) cond += " tmpB bad?";
    else if (cond_code == 0x27) cond += " st(i) doesn't exist?";
    else if (cond_code == 0x30) cond += " [0x10]<0";
    else if (cond_code == 0x32) cond += " [0x12]leading0s==0";
    else if (cond_code == 0x38) cond += " accumulator!=0";
    else if (cond_code == 0x3b) cond += " Asign != Bsign";
    else if (cond_code == 0x3e) cond += " unconditional";

    if (cond_int&1)
    {
        cond = "!(" + cond + ")";
    }

    return "cond=" + cond;
}

std::string getBopOp(int op)
{
    std::string ret = " ";
    if (false);
    else if (op == 0) ret += "ADD [0x6]";
    else if (op == 3) ret += "INC";
    else if (op == 4) ret += "DEC";
    else if (op == 5) ret += "SUB [0x6]";
    else              ret.clear();

    return ret;
}

std::string getBCopOp(int op, int mno)
{
    std::string ret = " ";
    if (false);
    else if (op == 0b000010 && mno == 2) ret += "tmpA.sgn -> sgnS";

    else if (op == 0b000011 && mno == 0) ret += "0 -> tmpA.sgn";
    else if (op == 0b000111 && mno == 0) ret += "1 -> tmpA.sgn";

    else if (op == 0b001010 && mno == 0) ret += "sgnS -> tmpA.sgn";
    else if (op == 0b001110 && mno == 0) ret += "!sgnS -> tmpA.sgn";

    else if (op == 0b000001 && (mno&5) == 4) ret += "set C0=0/C3=0 ft>" + std::string((mno&2)?" N":""); //ftst greater
    else if (op == 0b000101 && (mno&5) == 4) ret += "set C0=0/C3=1 ft=" + std::string((mno&2)?" N":""); //ftst equal
    else if (op == 0b001001 && (mno&5) == 4) ret += "set C0=1/C3=0 ft<" + std::string((mno&2)?" N":""); //ftst lesser
    else if (op == 0b001101 && (mno&5) == 4) ret += "set C0=1/C3=1 ft!" + std::string((mno&2)?" N":""); //ftst unordered
    else if (op == 0b000011 && mno == 7) ret += "set C2=1";

    else if (op == 34 && mno == 0) ret += "CORDIC a?";
    else if (op == 36 && mno == 2) ret += "CORDIC b?";
    else if (op == 54 && mno == 4) ret += "CORDIC c?";
    else if (op == 53 && mno == 6) ret += "CORDIC d? iterative algo";
    else              ret += "??no idea??";//ret.clear();

    return ret;
}

void decodeMicroOp(uint16_t op, std::string comment, int lineNum)
{
    std::cout << std::dec << "#" << std::setw(4) << std::setfill('0') << lineNum << "\t";

    std::cout << opString(op) << "\t";
    std::cout << std::hex << std::setw(4) << std::setfill('0') << op << std::dec << "\t";

    // ABCD EFGH IJKL MNOP
    const char*const regs[] =
    {
        "[0x0]",          "[0x1]",          "[0x2]st(0)",      "[0x3]st(i)",
        "[0x4]BIU",       "[0x5]expConst?", "[0x6]Bop_inX",    "[0x7]Bop_inY",
        "[0x8]expA",      "[0x9]expA raw",  "[0xa]fracA",      "[0xb]tmpA",
        "[0xc]expB",      "[0xd]expB raw",  "[0xe]fracB",      "[0xf]tmpB",
        "[0x10]Bop_outX", "[0x11]Bop_outY", "[0x12]ldz_counter","[0x13]",
        "[0x14]SHL_out1", "[0x15]SHR_out",  "[0x16]",          "[0x17]tmpC?",
        "[0x18]tanCordic","[0x19]logCordic","[0x1a]fracConst?","[0x1b]tmpA 3?",
        "[0x1c]SHL_out2", "[0x1d]",         "[0x1e]",          "[0x1f]NaN?",
    };

    const char*const regs_in[] =
    {
        nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, "[0x6]zero", nullptr,
        nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
    };


    if (((op & 0xFFF0) == 0xF000) && ((op&0xF)<6*2)) //set exception
    {
        const char* flags[] = {"invalid", "denorm", "division by zero", "overflow",
                               "underflow", "precision", "???[6]", "???[7]"};
        u8 param = (op>>1)&7;
        std::cout << "exception: " << flags[param^1]; //flip O
    }
    else if ((op&0xE3F1) == 0xE000) //ABC op! ABC. ..gh ijkl ...p
    {
        int operation =  ((op>>10)&7);
        int param = ((op>>1)&7);
        if (operation == 3 && param == 0)
            std::cout << "swap tmpA<>tmpB";
        else if (operation == 2 && param < 2)
        {
            if (param == 1)
                std::cout << "++accumulator";
            else
                std::cout << "--accumulator";
        }
        else
            std::cout << "ABC op=" << operation << " param=" << param;
    }
    else if (op == 0xFFFE) //RNI
    {
        std::cout << "---------------RNI----------------";
    }
    else if (op == 0xFEFE) //RNI2
    {
        std::cout << "---------------RNI2---------------";
    }
    else if (op == 0xFFF8) //NOP
    {
        std::cout << "nop";
    }
    else if ((op&0xE000) == 0xC000) //+ -local jump ABC, DEFGHI, JKLMNOP
    {
        std::string cond = make_cond_string(op&0x7F);

        int offset = ((op>>7)&0x3F);
        if (offset >= 32)
            offset -= 64;

        if (op&0x1000)
        {
            std::cout << "-jmp->#" << std::setw(4) << lineNum+1+offset << " " << cond;
        }
        else
        {
            std::cout << "+jmp->#" << std::setw(4) << lineNum+1+offset << " " << cond;
        }
    }
    /*else if ((op&0xF07F) == 0x0078)
    {
        std::cout << "?load constant [0x" << std::hex << ((op>>7)&0x01F) << std::dec << "]?";
    }*/
    else if ((op&0xE0C0) == 0)
    {
        std::cout << (regs_in[(op>>8)&0x1F]?regs_in[(op>>8)&0x1F]:regs[(op>>8)&0x1F]) << " -> " << regs[(op>>1)&0x1F] << ((op&1)?" with P":"");
    }
    else if ((op&0xE040) == 0x0040)
    {
        int reg = ((op>>1)&0x1F);
        if (reg == 0x15)
        {
            std::cout << "mov imm0x" <<std::hex << ((op>>7)&0x3F) << std::dec << " -> accumulator";
        }
        else if (reg == 0x1C)
        {
            std::cout << "load constant /0x" <<std::hex << ((op>>7)&0x3F) << std::dec << "/ -> [0x5]exp and [0x1a]frac";
        }
        else
        {
            std::cout << "J mov /0x" << std::hex << ((op>>7)&0x3F) << std::dec << "/ -> " << regs[reg] << ((op&1)?" with P":"");
        }
    }
    else if ((op&0xF000) == 0xA000) //A.C.
    {
        std::string cond = make_cond_string(op&0x7F);
        //function EFGHI
        //cond JKLMNOP
        std::cout << "call J" << ((op>>7)&0x1F) << ", " << cond;
    }
    else if ((op&0xF000) == 0x8000) //A...
    {
        std::string cond = make_cond_string(op&0x7F);
        //function EFGHI
        //cond JKLMNOP
        std::cout << "farjump J" << ((op>>7)&0x1F) << ", " << cond;
    }
    else if ((op&0xFF80) == 0xe000)
    {
        std::string cond = make_cond_string(op&0x7F);
        std::cout << "----return, " << cond;
    }
    else if (op == 0x7800)
    {
        std::cout << "stack pop (++stack ptr)";
    }
    else if (op == 0x7802)
    {
        std::cout << "stack push (--stack ptr)";
    }
    else if (op == 0x6340) //.BC...GH.J......
        std::cout << "set st(0) empty";
    else if (op == 0x6342) //.BC...GH.J....O.
        std::cout << "set st(i) empty";

    else if (op == 0x7c00) std::cout << "?increment something B?";
    else if (op == 0x7c02) std::cout << "?decrement something B?";

    /*else if ((op&0xfc78) == 0x6048) // set C0=G, C3=H
    {
        std::cout << "set ";
        std::cout << "C0=" << ((op&0x0200)?1:0) << ", ";
        std::cout << "C2=" << ((op&0x0080)?1:0) << ", ";
        std::cout << "C3=" << ((op&0x0100)?1:0) << ", ";
        std::cout << "NO=" << ((op>>1)&0x03);
    }
    else if ((op&0xff7f) == 0x604e) // set C2=1
    {
        std::cout << "set C2=" << ((op&0x080)?1:0);
    }*/
    else if((op&0xF000) == 0x6000)
    {
        int operation = ((op>>6)&0x3F);
        //std::cout << "BCop op=" << ((op>>6)&0x3F) << " MNO=" << ((op>>1)&7) << getBCopOp(operation, ((op>>1)&7));
        std::cout << "BCop ";
        std::cout << "IJ=" << std::setw(2) << ((op>>6)&0x3) << " ";
        std::cout << "MNO=" << ((op>>1)&7) << " ";
        std::cout << "op=" << std::setw(2) << ((op>>8)&0xF) << " ";
        std::cout << getBCopOp(operation, ((op>>1)&7)) << " ";
    }


    else if (op == 0x600a)
    {
        std::cout << "set st(0) valid?";
    }

    else if (op == 0x6084)
        std::cout << "?sgn store tmpA sign?"; //.BC.....I....N..
    else if (op == 0x6044)
        std::cout << "?sgn store tmpB sign?"; //.BC......J...N..
    else if (op == 0x6584)
        std::cout << "?store? flag? something?" ; //.BC..F.HI....N..
    else if (op == 0x60c0)
        std::cout << "?sgn clear tmpA sign?"; //.BC.....IJ......
    else if (op == 0x61c0)
        std::cout << "?sgn set tmpA sign?"; //.BC....HIJ......
    else if (op == 0x6280)
        std::cout << "?sgn take stored sign, and store to tmpA?"; //.BC...G.I.......
    else if (op == 0x6380)
        std::cout << "?sgn take stored sign, flip, and store to tmpA?"; //.BC...GHI.......

    else if (op == 0xec00) std::cout << "?negate something?"; // ABC.EF..........


    else if ((op&0xE000)==0x4000)
    {
        //ABC opcode
        //DEFGH input
        //IJ ??
        //K always zero
        //L output reg
        //MNO operation
        //P ??
        int operation = ((op>>1)&0x7);
        if ((op&0x40) == 0) //J==0
        {
            std::cout << "B" << ((op&0x80)?"I":"") << "op! param=" << regs[(op>>8)&0x1F] << " out=" << ((op&0x10)?"0x11":"0x10") << " op=" << operation << getBopOp(operation) << ((op&1)?"_P":"");
        }
        else
        {
            std::cout << "B" << ((op&0x80)?"I":"") << "Jop! param=" << regs[(op>>8)&0x1F] << " out=" << ((op&0x10)?"0x11":"0x10") << " op=" << operation << getBopOp(operation) << ((op&1)?"_P":"");
            //std::cout << "Bop! param=" << "/0x" << std::hex << ((op>>7)&0x3F) << std::dec << "/" << " out=" << ((op&0x10)?"0x11":"0x10") << " op=" << operation << getBopOp(operation) << ((op&1)?"_P":"");
        }
    }
    else if ((op&0xE000)==0x2000) //check ABC
    {
        if ((op&0x40) == 0) //J==0
        {
            std::cout << "C" << ((op&0x80)?"I":"") << "op shift! " << regs[(op>>8)&0x1F] << ((op&0x80)?">>":"<<") << (op&0x3F) << " -> " << ((op&0x80)?regs[0x15]:regs[0x14]);
        }
        else
        {
            std::cout << "C" << ((op&0x80)?"I":"") << "Jop! " << regs[(op>>8)&0x1F] << ((op&0x80)?" ?1 ":" ?2 ") << regs[((op>>1)&0x1F)] << " -> " << ((op&0x80)?regs[0x15]:regs[0x14]);
            //std::cout << "Cop shift! " << "/0x" << std::hex << ((op>>7)&0x3F) << std::dec << "/" << ((op&0x80)?">>":"<<") << (op&0x3F) << " -> " << ((op&0x80)?regs[0x15]:regs[0x1C]);

            //std::cout << "Cop! param=" << regs[(op>>8)&0x1F] << " IJ=" << ((op&0x80)?1:0) << ((op&0x40)?1:0) << " KLM=" << ((op>>3)&0x7) << " NOP=" << ((op>>0)&0x7);
        }
    }

    /*else if (op == 0x5006)
    {
        std::cout << "?increment tmpA exponent?";
    }
    else if (op == 0x5008)
    {
        std::cout << "?decrement tmpA exponent?";
    }*/
    else
    {
        std::cout << "?";
    }

    std::cout << "\t" << comment;

    std::cout << std::endl;
}

std::vector<MicroOp> readMicrocodeFile(const std::string& filename)
{
    std::vector<MicroOp> program;
    std::ifstream file(filename);

    if (!file.is_open())
    {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return program;
    }

    std::string line;
    int linenum = 0;
    while (std::getline(file, line))
    {
        // Skip empty lines
        if (line.length() < 17)
        {
            continue;
        }

        MicroOp op = parseLine(line);
        decodeMicroOp(op.bits, op.comment, linenum);
        program.push_back(op);
        ++linenum;
    }

    file.close();
    return program;
}

int main()
{
    readMicrocodeFile("8087mc.txt");
}
