#pragma once

template <typename T>
class RingBuffer
{
public:
    RingBuffer(const unsigned short size) :
        size(size), read_idx(0), write_idx(0), available_bytes(0), buffer(nullptr)
    {
        // Don't allow a zero for buffer size
        if (this->size == 0) this->size = 1;

        buffer = new T[this->size]{0};
    }

    ~RingBuffer()
    {
        delete [] buffer;
    }

    void Write(const T d_in)
    {
        buffer[write_idx++] = d_in;
        write_idx %= size;
        available_bytes++;
    }

    void Write(const T&& d_in)
    {
        buffer[write_idx++] = d_in;
        write_idx %= size;
        available_bytes++;
    }

    T Read()
    {
        if (available_bytes > 0) available_bytes--;
        T d_out = buffer[read_idx++];
        read_idx %= size;
        return d_out;
    }

    void Read(T& d_out, bool& is_end)
    {
        if (available_bytes > 0) available_bytes--;
        d_out = buffer[read_idx++];
        read_idx %= size;
        is_end = read_idx == write_idx;
    }

    unsigned int AvailableBytes() const
    {
        return available_bytes;
    }

    bool IsFull() const
    {
        return size == available_bytes;
    }

private:
    unsigned short size;
    unsigned short read_idx; // start
    unsigned short write_idx; // end
    unsigned int available_bytes;
    T* buffer;
};