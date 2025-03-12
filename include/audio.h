#pragma once

#include "beeper.h"
#include "YM3812.h"
#include "gameblaster.h"
#include "soundblaster.h"

u64 totalframes = 0;

//we need to somehow sync the "real audio timing" to the "emulator timing"
//this takes some thinking.
//we need to dynamically resample things
i16 additional_samples = 0;
const float SAMPLERATE = 48000.0;
float veer = (14318180.0/298.0)/SAMPLERATE;
bool audio_started{false};
i16 audio_buffer[1<<16] = {};
u16 audio_write_offset{};
u16 audio_read_offset{};
void audio_method3(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    u16 offset_end = audio_write_offset;
    if (!audio_started)
    {
        audio_started = true;
        audio_read_offset = offset_end-256;
    }
    u16 offset_start = audio_read_offset;
    u16 done_count = u16(offset_end-offset_start);
    if (frameCount == 0 || done_count == 0)
        return;

    totalframes += frameCount;
    u64 frame_counter=0;

    done_count = frameCount*veer+additional_samples;
    i16* pi16Output = (i16*)pOutput;
    for(u32 done_frames=0; done_frames<frameCount; ++done_frames)
    {
        i16 data = audio_buffer[offset_start];
        *pi16Output = data;
        ++pi16Output;
        data = audio_buffer[offset_start+1];
        *pi16Output = data;
        ++pi16Output;
        frame_counter += done_count;
        while(frame_counter >= frameCount)
        {
            offset_start += 2;
            frame_counter -= frameCount;
            if (offset_start == offset_end)
                goto double_break; //oh no :o
        }
    }
double_break: // oh no :O
    u16 left = (offset_end-offset_start);

    if (u16(offset_end-offset_start) >= 2048)
        offset_start = offset_end-256;

    if (left > 1024)
        additional_samples = 2;
    else if (left > 260)
        additional_samples = 1;
    else if (left < 252)
        additional_samples = -1;
    else
        additional_samples = 0;

    audio_read_offset = offset_start;
}

struct MiniAudio
{
    BEEPER& beeper;
    YM3812& ym3812;
    GameBlaster& gameblaster;
    SoundBlaster& soundblaster;

    ma_result result;
    ma_device_config deviceConfig;
    ma_device device;

    MiniAudio(BEEPER& beeper_, YM3812& ym3812_, GameBlaster& gameblaster_, SoundBlaster& soundblaster_) : beeper(beeper_), ym3812(ym3812_), gameblaster(gameblaster_), soundblaster(soundblaster_)
    {
        deviceConfig = ma_device_config_init(ma_device_type_playback);
        deviceConfig.playback.format   = ma_format_s16;
        deviceConfig.playback.channels = 2;
        deviceConfig.sampleRate        = u32(SAMPLERATE);
        deviceConfig.dataCallback      = audio_method3;

        deviceConfig.noPreSilencedOutputBuffer = true;
        deviceConfig.noClip = true;
        deviceConfig.noFixedSizedCallback = true;

        if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS)
        {
            cout << "MINIAUDIO init not succesful." << endl;
            std::abort();
        }
        if (ma_device_start(&device) != MA_SUCCESS)
        {
            cout << "MINIAUDIO device start not successful. No audio will be output." << endl;
            ma_device_uninit(&device);
        }
    }

    ~MiniAudio()
    {
        ma_device_uninit(&device);
    }

    void cycle()
    {
        {
            i32 data = beeper.sampleC+ym3812.sample+gameblaster.sound_out_l+soundblaster.sound_out_l;
            data = (data<-32768?-32768:data);
            data = (data>32767?32767:data);
            audio_buffer[audio_write_offset] = globalsettings.sound_on?i16(data):i16(0);
            ++audio_write_offset;
        }
        {
            i32 data = beeper.sampleC+ym3812.sample+gameblaster.sound_out_r+soundblaster.sound_out_r;
            data = (data<-32768?-32768:data);
            data = (data>32767?32767:data);
            audio_buffer[audio_write_offset] = globalsettings.sound_on?i16(data):i16(0);
            ++audio_write_offset;
        }
        //audio_buffer[audio_write_offset] = globalsettings.sound_on?i16(data):i16(0);
        //++audio_write_offset;
    }
};


