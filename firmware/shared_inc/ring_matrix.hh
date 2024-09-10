#pragma once

#include <stdint.h>

#include "ring_buffer.hh"


class RingMatrix
{
private:
    typedef struct
    {
        uint16_t write = 0;
        uint16_t read = 0;
        uint16_t idx;
        uint16_t size;
    } data_t;


public:
    RingMatrix(uint16_t size=16) : sz(size)
    {
        if (sz == 0) sz = 1;
        data_arr = new uint8_t*[sz];
        data_info_arr = new data_t[sz];
    }

    ~RingMatrix()
    {
        delete [] data_info_arr;
        delete [] data_arr;
    }

    void Allocate(uint16_t num_bytes)
    {
        // Push a new info object onto the ring buffer
        data_t info;
        info.idx = write;
        info.size = num_bytes;

        data_info_arr[write] = info;

        // Push a new data_arr allocation to the byte data_arr
        data_arr[write++] = new uint8_t[num_bytes];

        if (write == sz)
        {
            write = 0;
        }

    }

    // All data needed should be immediately pushed after allocate, and
    // before the next allocate has begun
    template <typename T>
    void Push(T input)
    {
        // Get the size of the input data_arr
        size_t data_size = sizeof(T);

        // Cast the data into bytes
        uint8_t* byte_input = (uint8_t*)(void*)&input;

        // Get the current data_info
        data_info_p = &data_info_arr[write-1];

        // Get the write
        uint16_t& d_write = data_info_p->write;

        // Get the max size from the data_arr info
        const uint16_t& max_size = data_info_p->size;

        uint16_t in_read = 0;

        // Transfer the data_arr
        while (d_write < max_size && in_read < data_size)
        {
            data_arr[write-1][d_write++] = byte_input[in_read++];
        }
    }

    // This should only be called AFTER allocating and pushing
    // all of the data_arr that is required.
    // You can however, allocate more memory than you plan to push
    // and write to it later with this function
    template<typename T>
    void Write(T input, uint16_t idx)
    {
        // TODO error checking

        uint16_t data_size = sizeof(T);

        // Cast the data into bytes
        uint8_t* byte_input = (uint8_t*)(void*)&input;

        // Get the index for this data
        const uint16_t& read_idx = data_info_arr[read].idx;

        // Get the max size for the current data arr
        const uint16_t& max_size = data_info_arr[read].size;

        uint16_t d_write = idx;
        uint16_t in_read = 0;

        while (d_write < max_size && in_read < data_size)
        {
            data_arr[read_idx][d_write++] = byte_input[in_read++];
        }
    }

    template <typename T>
    T Read(uint16_t idx)
    {
        T ret = 0;
        uint16_t sz = sizeof(T);
        uint8_t* in = (uint8_t*)(void*)&ret;

        data_t& data_info = data_info_arr[read];

        if (sz == 0 || idx >= data_info.size)
            return ret;

        uint16_t d_read = idx;
        uint16_t r_write = 0;

        while (d_read < data_info.size && r_write < sz)
        {
            in[r_write++] = data_arr[read][d_read++];
        }

        return ret;
    }

    void Swap()
    {
        delete [] data_arr[read];
        // TODO delete the previous block of memory
        if (++read >= sz)
        {
            read = 0;
        }
    }
private:
    uint16_t sz;
    uint16_t write;
    uint16_t read;

    data_t* data_info_arr;
    data_t* data_info_p;
    uint8_t** data_arr;
};