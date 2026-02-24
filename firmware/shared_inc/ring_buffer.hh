#pragma once

#include <stdint.h>
#include <cstddef>
#include <utility>

#define DEFAULT_BUFFER_SIZE 32

template <typename T>
class RingBuffer
{
public:
    RingBuffer(T* buff, const uint16_t size) :
        size(size),
        read_idx(0),
        write_idx(0),
        buffer(buff),
        busy(false),
        owner(false)
    {
    }

    RingBuffer(const uint16_t size = DEFAULT_BUFFER_SIZE) :
        size(size),
        read_idx(0),
        write_idx(0),
        buffer(nullptr),
        busy(false),
        owner(true)
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
        if (owner)
        {
            delete[] buffer;
        }
    }

    T* Write(uint16_t& len) noexcept
    {
        const uint16_t idx = write_idx;
        if (write_idx + len > size)
        {
            len = size - write_idx;
        }

        write_idx += len;

        if (write_idx >= size)
        {
            write_idx = 0;
        }

        return buffer + idx;
    }

    T& Write() noexcept
    {
        BusySet();
        if (write_idx >= size)
        {
            write_idx = 0;
        }

        T& out = buffer[write_idx++];

        BusyReset();
        return out;
    }

    void Write(T d_in) noexcept
    {
        BusySet();
        buffer[write_idx++] = std::move(d_in);

        if (write_idx >= size)
            write_idx = 0;

        BusyReset();
    }

    void Write(const T* input, const uint16_t count)
    {
        BusySet();

        size_t input_idx = 0;
        size_t remaining = count;

        while (remaining)
        {
            uint16_t to_copy = remaining;
            if (write_idx + to_copy > size)
            {
                to_copy = size - write_idx;
            }

            for (size_t i = 0; i < to_copy; ++i)
            {
                buffer[write_idx++] = std::move(input[input_idx++]);
            }

            remaining -= to_copy;
            input_idx += to_copy;

            if (write_idx >= size)
            {
                write_idx = 0;
            }
        }

        BusyReset();
    }

    T Take() noexcept
    {
        BusySet();

        T d_out = std::move(buffer[read_idx++]);

        if (read_idx >= size)
        {
            read_idx = 0;
        }

        BusyReset();
        return d_out;
    }

    T& Read() noexcept
    {
        BusySet();

        T& d_out = buffer[read_idx++];

        if (read_idx >= size)
        {
            read_idx = 0;
        }

        BusyReset();
        return d_out;
    }

    void Read(T& d_out, bool& is_end)
    {
        BusySet();

        d_out = buffer[read_idx++];

        if (read_idx >= size)
        {
            read_idx = 0;
        }

        is_end = read_idx == write_idx;
        BusyReset();
    }

    void Read(T* to, const size_t len)
    {
        size_t remaining = len;
        size_t out_idx = 0;
        while (remaining)
        {
            size_t to_copy = remaining;
            if (remaining + read_idx > size)
            {
                to_copy = size - read_idx;
            }

            remaining -= to_copy;
            while (to_copy)
            {
                to[out_idx++] = buffer[read_idx++];
                --to_copy;
            }

            if (read_idx >= size)
            {
                read_idx = 0;
            }
        }
    }

    T* Read(size_t& len)
    {
        size_t idx = read_idx;
        if (read_idx + len > size)
        {
            len = size - read_idx;
        }

        read_idx += len;

        if (read_idx >= size)
        {
            read_idx = 0;
        }

        return buffer + idx;
    }

    const T& Peek() const
    {
        return buffer[read_idx];
    }

    uint16_t Unread() const
    {
        if (write_idx >= read_idx)
        {
            return write_idx - read_idx;
        }
        else
        {
            return size - read_idx + write_idx;
        }
    }

    uint16_t NumContiguousRead() const
    {
        const uint16_t num_read = Unread();
        if (read_idx + num_read > size)
        {
            return size - read_idx;
        }
        return num_read;
    }

    bool IsFull() const
    {
        return size == Unread();
    }

    T* Buffer()
    {
        return buffer;
    }

    uint16_t WriteIdx() const
    {
        return write_idx;
    }

    uint16_t ReadIdx() const
    {
        return read_idx;
    }

    inline uint16_t Size() const
    {
        return size;
    }

    void MoveWriteHead(const uint16_t idx)
    {
        if (idx >= size)
        {
            return;
        }

        write_idx = idx;
    }

    void MoveReadHead(const uint16_t amt)
    {
        if (read_idx + amt >= size)
        {
            read_idx = size - (read_idx + amt);
        }

        read_idx += amt;
    }

    void SyncReadToWrite()
    {
        read_idx = write_idx;
    }

private:
    inline void BusySet()
    {
        if (busy)
        {
            while (true)
            {
            }
        }
        busy = true;
    }

    inline void BusyReset()
    {
        busy = false;
    }

    uint16_t size;
    uint16_t read_idx;  // start
    uint16_t write_idx; // end
    T* buffer;
    bool busy;
    bool owner;
};
