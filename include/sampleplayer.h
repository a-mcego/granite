#pragma once

struct SamplePlayer
{
    i16 sound_out_l{}, sound_out_r{};

    struct Sample
    {
        std::vector<i16> data;
    };
    std::vector<Sample> samples;

    void load_sample(const std::string& filename)
    {
        FILE* filu = fopen(filename.c_str(), "rb");
        fseek(filu,0,SEEK_END);
        u32 size = ftell(filu);
        fseek(filu,0,SEEK_SET);

        Sample sample;
        sample.data.assign(size,0);
        fread(sample.data.data(), size, 1, filu);
        samples.push_back(sample);

        fclose(filu);
    }

    void load_sample(const vector<i16>& data)
    {
        Sample sample;
        sample.data = data;
        samples.push_back(sample);
    }

    struct Channel
    {
        bool playing{};
        u8 sample_id{};
        u32 position{};
    };
    static constexpr int CHANNELS = 12;
    Channel channels[CHANNELS];

    void play(u8 sample_id)
    {
        if (sample_id < samples.size())
        {
            for(int i=0; i<CHANNELS; ++i)
            {
                if (!channels[i].playing)
                {
                    //std::cout << "start playing on chan " << i << std::endl;
                    channels[i].sample_id = sample_id;
                    channels[i].position = 0;
                    channels[i].playing = true;
                    break;
                }
            }
            //what to do if free channel wasnt found?
        }
    }

    void cycle() //48000 Hz
    {
        sound_out_l = 0;
        sound_out_r = 0;
        for(int i=0; i<CHANNELS; ++i)
        {
            if (channels[i].playing)
            {
                channels[i].position += 1;
                if (channels[i].position >= samples[channels[i].sample_id].data.size())
                {
                    channels[i].playing = false;
                }
                else
                {
                    i16 sample = samples[channels[i].sample_id].data[channels[i].position];
                    sound_out_l += (sample>>3);
                    sound_out_r += (sample>>3);
                }
            }
        }
    }
} sampleplayer;
