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
    std::stringstream ss;
    ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << cond_int;
    std::string cond = ss.str();
    if (false);
    else if (cond_int == 0x00) cond = "cond=is_memory_operand";
    else if (cond_int == 0x04) cond = "cond=stack_empty";
    else if (cond_int == 0x0d) cond = "cond=!(pop_at_end)";
    else if (cond_int == 0x14) cond = "cond=MODRM&1";
    else if (cond_int == 0x15) cond = "cond=!(MODRM&1)";
    else if (cond_int == 0x17) cond = "cond=!(double_pop_at_end)";
    else if (cond_int == 0x44) cond = "cond?=tmpa bad?";
    else if (cond_int == 0x46) cond = "cond=stack overflow";
    else if (cond_int == 0x4e) cond = "cond=has_error";
    else if (cond_int == 0x4f) cond = "cond=!(has_error)";
    else if (cond_int == 0x7c) cond = "unconditional";
    else if (cond_int == 0x7d) cond = "cond=!(unconditional)=?never?";
    else
    {
        cond = "cond="+cond;
    }
    return cond;
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
    else if ((op&0xF800) == 0xC000 || (op&0xF800) == 0xD800 /*|| (op&0xF800) == 0xC800|| (op&0xF800) == 0xD000*/) //+ -local jump ABCDE, FGHI, JKLMNOP
    {
        std::string cond = make_cond_string(op&0x7F);

        if ((op&0xF800) == 0xD800)
        {
            std::cout << "-jmp->#" << std::setw(4) << lineNum+1-((op>>7)&0xF) << " " << cond;
        }
        else if ((op&0xF800) == 0xC000)
        {
            std::cout << "+jmp->#" << std::setw(4) << lineNum+1+((op>>7)&0xF) << " " << cond;
        }
        //these are a bit shit
        /*else if ((op&0xf800) == 0xC800)
        {
            std::cout << "jmpA?->#" << std::setw(4) << lineNum+1+((op>>7)&0xF) << " or #" << std::setw(4) << lineNum+1-((op>>7)&0xF) << " " << cond;
        }
        else
        {
            std::cout << "jmpB?->#" << std::setw(4) << lineNum+1+((op>>7)&0xF) << " or #" << std::setw(4) << lineNum+1-((op>>7)&0xF) << " " << cond;
        }*/
    }
    else if ((op&0xF07F) == 0x0078)
    {
        std::cout << "?load constant [" << ((op>>7)&0x01F) << "]?";
    }
    else if ((op&0xE0C1) == 0)
    {
        const char*const regs[] =
        {
            "[0x0]", "[0x1]", "st(0)", "st(i)",
            "[0x4]", "?constant?", "zero", "[0x7]",
            "[0x8]", "[0x9]", "[0xa]", "tmpA",
            "[0xc]", "[0xd]", "[0xe]", "tmpB",
            "[0x10]", "[0x11]", "[0x12]", "[0x13]",
            "[0x14]", "[0x15]", "[0x16]", "[0x17]",
            "[0x18]", "[0x19]", "[0x1a]", "[0x1b]",
            "[0x1c]", "[0x1d]", "[0x1e]", "QNaN",
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
    else if (op == 0xe07c)
    {
        std::cout << "--------------return--------------";
    }
    else if (op == 0x7800)
    {
        std::cout << "stack pop";
    }
    else if (op == 0x7802)
    {
        std::cout << "stack push";
    }
    else if (op == 0x6340)
    {
        std::cout << "set st(0) empty?";
    }
    else if (op == 0x600a)
    {
        std::cout << "set st(0) valid?";
    }
    else if (op == 0x6084)
    {
        std::cout << "?store tmpA sign?";
    }
    else if (op == 0x60c0)
    {
        std::cout << "?clear tmpA sign?";
    }
    else if (op == 0x6380)
    {
        std::cout << "?take stored sign, flip, and store to tmpA?";
    }
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
