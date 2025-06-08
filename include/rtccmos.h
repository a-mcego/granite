#pragma once

#include <ctime>

struct CHIP146818 // RTC & CMOS
{
    u64 rtc_cycle_count = 0;
    static const u32 RTC_TICKS_PER_SECOND = 32;

    u8 CMOSdata[64] = {};
    u8 current_reg = 0x0D;

    std::string filename;

    bool changed{};
    void save()
    {
        if (changed && !filename.empty())
        {
            FILE* filu = fopen(filename.c_str(),"wb");
            if (filu != NULL)
            {
                fwrite(CMOSdata, 64, 1, filu);
                fclose(filu);
            }
        }
        changed = false;
    }

    void load()
    {
        if (!filename.empty())
        {
            FILE* filu = fopen(filename.c_str(),"rb");
            if (filu != NULL)
            {
                fread(CMOSdata, 64, 1, filu);
                fclose(filu);
            }
        }
    }

    void update_time()
    {
        time_t now = time(nullptr);
        tm* t = localtime(&now);

        auto toBCD = [](int val) -> u8 { return ((val / 10) << 4) | (val % 10); };

        CMOSdata[0x00] = toBCD(t->tm_sec);
        CMOSdata[0x02] = toBCD(t->tm_min);
        CMOSdata[0x04] = toBCD(t->tm_hour);
        CMOSdata[0x06] = toBCD(t->tm_wday + 1);
        CMOSdata[0x07] = toBCD(t->tm_mday);
        CMOSdata[0x08] = toBCD(t->tm_mon + 1);
        CMOSdata[0x09] = toBCD(t->tm_year % 100);
        CMOSdata[0x32] = toBCD(t->tm_year / 100 + 19);

        //disable alarm - top two bits set to 1
        CMOSdata[0x01] = 0xFF;
        CMOSdata[0x03] = 0xFF;
        CMOSdata[0x05] = 0xFF;

        CMOSdata[0x0A] = 0b00100110;
        // Status Register B
        // bit 2: 1 = binary mode (0 = BCD)
        // bit 1: 1 = 24h mode (0 = 12h)
        CMOSdata[0x0B] = 0b00000010;
        CMOSdata[0x0D] = 0x80;
    }

    CHIP146818(std::string filename_=""):filename(filename_)
    {
        load();
        update_time();
    }

    void printtime()
    {
        std::cout << std::dec << "19" << u16(CMOSdata[0x09]) << "-" << u16(CMOSdata[0x08]) << "-" << u16(CMOSdata[0x07]) << " " << u16(CMOSdata[0x04]) << ":" << u16(CMOSdata[0x02]) << ":" << u16(CMOSdata[0x00]) << std::endl << std::hex;
        std::cout << u16(CMOSdata[0x0A]) << " ";
        std::cout << u16(CMOSdata[0x0B]) << " ";
        std::cout << u16(CMOSdata[0x0C]) << " ";
        std::cout << u16(CMOSdata[0x0D]) << endl;
    }

    u8 read(u8 port) //port from 0 to 1! inclusive
    {
        u8 ret=0;
        if (port == 1) //read
        {
            if (current_reg < 0x40)
                ret = CMOSdata[current_reg];
            if (current_reg == 0x0C)
            {
                // Reading Register C clears bits 4,5,6 and 7 (PF, AF, UF, IRQF)
                // Other bits of Reg C are reserved and should remain unchanged (typically 0).
                // For now, we simply clear all bits we've set.
                // A more robust implementation might preserve reserved bits if they could be non-zero.
                CMOSdata[0x0C] &= ~0xF0; // Clear bits 4,5,6,7. Alarm Flag (bit 5) isn't set yet but good to clear.
            }
            current_reg = 0x0D; // This is often set to a non-volatile register after read/write
        }
        return ret;
    }

    void write(u8 port, u8 data) //port from 0 to 1! inclusive.
    {
        if (port == 0)
        {
            current_reg = (data&0x7F);
        }
        else if (port == 1)
        {
            if (current_reg == 0x0A)
            {
                CMOSdata[current_reg] = (CMOSdata[current_reg]&0x80)|(data&0x7F);
                changed = true;
            }
            else if (current_reg != 0x0C && current_reg != 0x0D && current_reg < 0x40) //EXPLAIN: why this IF?
            {
                CMOSdata[current_reg] = data;
                changed = true;
            }
            current_reg = 0x0D;
        }
    }

    void cycle()
    {
        printtime();
        rtc_cycle_count++;
        CMOSdata[0x0A] = (CMOSdata[0x0A]&0x7F) | (rtc_cycle_count>=(RTC_TICKS_PER_SECOND*32/64)?0x80:0x00);

        if (rtc_cycle_count >= RTC_TICKS_PER_SECOND)
        {
            // Helper lambda to convert BCD to binary
            auto fromBCD = [](u8 val) -> int { return (val >> 4) * 10 + (val & 0x0F); };
            // Helper lambda to convert binary to BCD (similar to update_time)
            auto toBCD = [](int val) -> u8 { return ((val / 10) << 4) | (val % 10); };

            rtc_cycle_count = 0;

            // --- Time Increment Logic ---
            int second = fromBCD(CMOSdata[0x00]);
            int minute = fromBCD(CMOSdata[0x02]);
            int hour = fromBCD(CMOSdata[0x04]);
            int day = fromBCD(CMOSdata[0x07]);
            int month = fromBCD(CMOSdata[0x08]);
            int year = fromBCD(CMOSdata[0x09]);
            int century = fromBCD(CMOSdata[0x32]);

            second++;
            if (second >= 60)
            {
                second = 0;
                minute++;
                if (minute >= 60)
                {
                    minute = 0;
                    hour++;
                    if (hour >= 24)
                    {
                        hour = 0;
                        day++;

                        const int current_year_full = century * 100 + year;
                        const bool is_leap_year = ((current_year_full % 4 == 0) && (current_year_full % 100 != 0)) || (current_year_full % 400 == 0);

                        const int daysInMonth[] = {0, 31, (is_leap_year?29:28), 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

                        if (day > daysInMonth[month])
                        {
                            day = 1;
                            month++;
                            if (month > 12)
                            {
                                month = 1;
                                year++;
                                if (year >= 100)
                                {
                                    year = 0;
                                    century++;
                                    CMOSdata[0x32] = toBCD(century);
                                }
                                CMOSdata[0x09] = toBCD(year);
                            }
                            CMOSdata[0x08] = toBCD(month);
                        }
                        CMOSdata[0x07] = toBCD(day);

                        // Update weekday
                        int weekday = fromBCD(CMOSdata[0x06]);
                        weekday++;
                        if (weekday > 7) {
                            weekday = 1;
                        }
                        CMOSdata[0x06] = toBCD(weekday);
                    }
                    CMOSdata[0x04] = toBCD(hour);
                }
                CMOSdata[0x02] = toBCD(minute);
            }
            CMOSdata[0x00] = toBCD(second);

            //std::cout << std::dec << century << " " << year << "-" << month << "-" << day << " " << hour << ":" << minute << ":" << second << std::endl;

            // Handle Periodic Interrupt
            /*if (CMOSdata[0x0B] & 0x40) // Check if PIE (bit 6 of Status Register B) is set
            {
                CMOSdata[0x0C] |= 0x40; // Set PF (bit 6 of Status Register C)
                // Additionally, if Alarm Interrupt Enable (AIE bit 5 of Reg B) is set,
                // then AF (bit 5 of Reg C) should be set when alarm time matches. (Not implemented here)
                // If Update Interrupt Enable (UIE bit 4 of Reg B) is set,
                // then UF (bit 4 of Reg C) should be set. (Should be set here)
                if (CMOSdata[0x0B] & 0x10) // UIE (Update-ended Interrupt Enable)
                {
                    CMOSdata[0x0C] |= 0x10; // Set UF (Update-ended Interrupt Flag)
                }
                // If any of PF, AF(not implemented), UF is set, then set IRQF (bit 7)
                if (CMOSdata[0x0C] & 0x70) // Check if any of bits 4,5,6 are set
                {
                    CMOSdata[0x0C] |= 0x80; // Set IRQF
                }
            }*/
        }
    }
};

