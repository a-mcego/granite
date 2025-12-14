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
    else if (cond_code == 0x01) cond += " (opcode&0x7C0) == 0x4C0";
    else if (cond_code == 0x02) cond += " stack_empty";
    else if (cond_code == 0x06) cond += " pop_at_end";
    else if (cond_code == 0x0a) cond += " opcode&1";
    else if (cond_code == 0x0b) cond += " double_pop_at_end";
    else if (cond_code == 0x20) cond += " tmpA zero";
    else if (cond_code == 0x22) cond += " tmpA bad?";
    else if (cond_code == 0x23) cond += " stack overflow";
    else if (cond_code == 0x24) cond += " tmpB zero";
    else if (cond_code == 0x26) cond += " tmpB bad?";
    else if (cond_code == 0x27) cond += " st(i) doesn't exist (A)";
    else if (cond_code == 0x2f) cond += " st(i) doesn't exist (B)";
    else if (cond_code == 0x38) cond += " accumulator!=0";
    else if (cond_code == 0x3e) cond += " unconditional";

    if (cond_int&1)
    {
        cond = "!(" + cond + ")";
    }

    return "cond=" + cond;
}

void decodeMicroOp(uint16_t op, std::string comment, int lineNum)
{
    std::cout << std::dec << "#" << std::setw(4) << std::setfill('0') << lineNum << "\t";

    std::cout << opString(op) << "\t";
    std::cout << std::hex << std::setw(4) << std::setfill('0') << op << std::dec << "\t";

    // ABCD EFGH IJKL MNOP


    if (((op & 0xFFF0) == 0xF000) && ((op&0xF)<6*2)) //set exception
    {
        const char* flags[] = {"invalid", "denorm", "division by zero", "overflow",
                               "underflow", "precision", "???[6]", "???[7]"};
        u8 param = (op>>1)&7;
        std::cout << "exception: " << flags[param^1]; //flip O
    }
    else if (op == 0xFFFE) //RNI
    {
        std::cout << "---------------RNI----------------";
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
    else if ((op&0xF07F) == 0x0078)
    {
        std::cout << "?load constant [0x" << std::hex << ((op>>7)&0x01F) << std::dec << "]?";
    }
    else if ((op&0xE0C1) == 0)
    {
        const char*const regs[] =
        {
            "[0x0]", "[0x1]", "[0x2]st(0)", "[0x3]st(i)",
            "[0x4]", "[0x5]?constant?", "[0x6]zero", "[0x7]",
            "[0x8]", "[0x9]", "[0xa]", "[0xb]tmpA",
            "[0xc]", "[0xd]", "[0xe]", "[0xf]tmpB",
            "[0x10]", "[0x11]", "[0x12]", "[0x13]",
            "[0x14]", "[0x15]", "[0x16]", "[0x17]",
            "[0x18]", "[0x19]", "[0x1a] st0 something?", "[0x1b] sti something?",
            "[0x1c]", "[0x1d]", "[0x1e]", "[0x1f]NaN?",
        };
        std::cout << regs[(op>>8)&0x1F] << " -> " << regs[(op>>1)&0x1F];
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

    else if (op == 0xe800) std::cout << "?increment something A?";
    else if (op == 0xe802) std::cout << "?decrement something A?";

    else if (op == 0x7c00) std::cout << "?increment something B?";
    else if (op == 0x7c02) std::cout << "?decrement something B ?";


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

    else if (op == 0xec00) std::cout << "?swap tmpA and tmpB?"; // ABC.EF..........


    else if (op == 0x5006)
    {
        std::cout << "?increment tmpA exponent?";
    }
    else if (op == 0x5008)
    {
        std::cout << "?decrement tmpA exponent?";
    }
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
