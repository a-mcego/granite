#pragma once

struct MemBytes
{
    u64 size{};
    u8* bytes{};

    void set_size(u64 size_)
    {
        size = size_;
        if (size < (1<<20))
            size = 1<<20;

        if (bytes)
            delete [] bytes;
        bytes = new u8[size];
    }
};
