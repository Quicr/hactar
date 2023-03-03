#pragma once

template <typename T>
class RingBuffer
{
public:
    RingBuffer(unsigned short size) :
        buffer(new T[size]{0}), size(size), read_idx(0), write_idx(0)
    {}

    ~RingBuffer()
    {
        delete [] buffer;
    }

    void Write(const T d_in)
    {
        buffer[write_idx++] = d_in;
        write_idx %= size;
    }

    void Write(const T&& d_in)
    {
        buffer[write_idx++] = d_in;
        write_idx %= size;
    }

    T Read()
    {
        T d_out = buffer[read_idx++];
        read_idx %= size;
        return d_out;
    }

    void Read(T& d_out, bool& is_end)
    {
        d_out = buffer[read_idx++];
        read_idx %= size;
        is_end = read_idx == write_idx;
    }

private:
    T* buffer;
    unsigned short size;
    unsigned short read_idx; // start
    unsigned short write_idx; // end
};