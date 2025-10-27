#pragma once

struct BEEPER
{
    //u8 timer{};

    i16 sampleA{}, sampleB{}, sampleC{};

    u8 pb1{}; //speaker data
    u8 pb0{}; //timer gate

    void set_output_from_pit(bool value)
    {
        /*if (globalsettings.sound_on)
        {
            static FILE* filu = nullptr;
            if (filu == nullptr)
                filu = fopen("d:\\out2.raw", "wb");
            fwrite(&sampleA, 2, 1, filu);
        }*/



        sampleA = ((value&&(globalsettings.global_port0x61&0x02))?60*256:0)*(pb0?-1:1); //TODO: make pc work again
        sampleB = ((sampleB<<5)-sampleB+sampleA)>>5; //crude lowpass
        sampleC = ((sampleC<<5)-sampleC+sampleB)>>5; //crude lowpass
    }

};
