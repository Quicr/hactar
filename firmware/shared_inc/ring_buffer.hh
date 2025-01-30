#pragma once

#include <stdint.h>
#include <utility>

#define DEFAULT_BUFFER_SIZE 32

template <typename T>
class RingBuffer
{
public:
// TODO constructor that takes a bucket space of type T
    RingBuffer(const uint16_t size = DEFAULT_BUFFER_SIZE):
        size(size),
        read_idx(0),
        write_idx(0),
        unread(0),
        buffer(nullptr)
    {
        // Don't allow a zero for buffer size
        if (this->size == 0)
        {
            this->size = 1;
        }

        buffer = new T[this->size]{};
    }

    ~RingBuffer()
    {
        delete [] buffer;
    }

    T& operator [](const size_t i)
    {
        return buffer[i];
    }

    T& Write() noexcept
    {
        if (write_idx >= size)
        {
            write_idx = 0;
        }

        ++unread;
        return buffer[write_idx++];
    }

    void Write(T d_in) noexcept
    {
        buffer[write_idx++] = std::move(d_in);

        if (write_idx >= size)
            write_idx = 0;

        ++unread;
    }

    void Write(T* input, const uint16_t count)
    {
        size_t input_off = 0;

        // Buffer is nearly full so we need to wrap around to the front
        if (write_idx + count >= size)
        {
            const uint16_t write_space = size - write_idx;
            const uint16_t remaining = write_space < count ? write_space : count;

            while (input_off < remaining)
            {
                buffer[write_idx++] = std::move(input[input_off++]);
            }

            write_idx = 0;
        }

        for (uint16_t i = input_off; i < count; ++i)
        {
            buffer[write_idx++] = std::move(input[i]);
        }
    }

    T Take() noexcept
    {
        if (unread > 0)
        {
            unread--;
        }

        T d_out = std::move(buffer[read_idx++]);

        if (read_idx >= size)
        {
            read_idx = 0;
        }

        return d_out;
    }

    T& Read() noexcept
    {
        if (unread > 0)
        {
            unread--;
        }
        T& d_out = buffer[read_idx++];

        if (read_idx >= size)
        {
            read_idx = 0;
        }

        return d_out;
    }

    void Read(T& d_out, bool& is_end)
    {
        if (unread > 0)
        {
            unread--;
        }

        d_out = buffer[read_idx++];

        if (read_idx >= size)
        {
            read_idx = 0;
        }

        is_end = read_idx == write_idx;
    }

    const T& Peek() const
    {
        return buffer[read_idx];
    }

    uint16_t Unread() const
    {
        return unread;
    }

    bool IsFull() const
    {
        return size == unread;
    }

    T* Buffer()
    {
        return buffer;
    }

    uint16_t WriteIdx() const
    {
        return write_idx;
    }

    inline uint16_t Size() const
    {
        return size;
    }

    void MoveWriteHead(uint16_t idx)
    {
        // TODO put into some error state so that programmer knows of the error
        if (idx > size)
        {
            return;
        }

        write_idx = idx;
    }

    // This is only really useful for the ring buffering from the
    // stm lib
    inline void UpdateWriteHead(uint16_t idx)
    {
        const uint16_t num_recv = idx - write_idx;
        unread += num_recv;
        write_idx += num_recv;

        if (write_idx >= size)
        {
            write_idx = write_idx - size;
        }
    }

private:
    uint16_t size;
    uint16_t read_idx; // start
    uint16_t write_idx; // end
    uint16_t unread;
    T* buffer;
};