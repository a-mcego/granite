#pragma once

struct MemBytes
{
    u64 size{};
    u8* bytes{};

    void set_size(u64 size_)
    {
        size = size_;

        if (bytes)
            delete [] bytes;
        bytes = new u8[size];
    }
};
