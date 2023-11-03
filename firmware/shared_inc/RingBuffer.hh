#pragma once

#include <stdint.h>

template <typename T>
class RingBuffer
{
public:
    RingBuffer(const unsigned short size) :
        size(size),
        read_idx(0),
        write_idx(0),
        unread_values(0),
        buffer(nullptr)
    {
        // Don't allow a zero for buffer size
        if (this->size == 0) this->size = 1;

        buffer = new T[this->size]{ 0 };
    }

    ~RingBuffer()
    {
        delete [] buffer;
    }

    void Write(const T d_in)
    {
        buffer[write_idx++] = d_in;

        if (write_idx >= size)
            write_idx = 0;

        unread_values++;
    }

    void Write(const T&& d_in)
    {
        buffer[write_idx++] = d_in;

        if (write_idx >= size)
            write_idx = 0;

        unread_values++;
    }

    T Read()
    {
        if (unread_values > 0) unread_values--;
        T d_out = buffer[read_idx++];

        if (read_idx >= size)
            read_idx = 0;

        return d_out;
    }

    void Read(T& d_out, bool& is_end)
    {
        if (unread_values > 0) unread_values--;
        d_out = buffer[read_idx++];

        if (read_idx >= size)
            read_idx = 0;

        is_end = read_idx == write_idx;
    }

    size_t AvailableBytes() const
    {
        return unread_values;
    }

    bool IsFull() const
    {
        return size == unread_values;
    }

    T* Buffer()
    {
        return buffer;
    }

    inline unsigned short Size() const
    {
        return size;
    }

    void MoveWriteHead(unsigned short idx)
    {
        // TODO put into some error state so that programmer knows of the error
        if (idx > size)
        {
            return;
        }

        write_idx = idx;
    }

    inline void UpdateWriteHead(unsigned short num_received)
    {
        uint16_t delta_bytes = num_received - write_idx;
        write_idx += delta_bytes;
        unread_values += delta_bytes;

        if (write_idx >= size)
        {
            write_idx = 0;
        }
    }

private:
    unsigned short size;
    unsigned short read_idx; // start
    unsigned short write_idx; // end
    size_t unread_values;
    T* buffer;
};