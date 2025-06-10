#pragma once

#include "array"
#include "ring_buffer.hh"
#include <stdint.h>

template <typename T>
class RingMatrix
{
public:
    RingMatrix(const uint16_t num_rings, const uint16_t buffer_sz) :
        num_rings(num_rings),
        buffer_sz(buffer_sz),
        write(0),
        read(0),
        idx(0)
    {
        matrix = new RingBuffer<T>*[num_rings];
        for (uint16_t i = 0; i < num_rings; ++i)
        {
            matrix[i] = new RingBuffer<T>(buffer_sz);
        }
    }

    ~RingMatrix()
    {
        for (uint16_t i = 0; i < num_rings; ++i)
        {
            delete matrix[i];
        }

        delete[] matrix;
    }

    RingBuffer<T>* Current()
    {
        return matrix[idx];
    }

    RingBuffer<T>* Next()
    {
        if (++idx >= num_rings)
        {
            idx = 0;
        }

        std::cout << "idx " << idx << std::endl;
        return matrix[idx];
    }

private:
    const uint16_t num_rings;
    const uint16_t buffer_sz;
    uint16_t write;
    uint16_t read;
    uint16_t idx;

    RingBuffer<T>** matrix;
};
