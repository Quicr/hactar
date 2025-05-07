#pragma once

template <typename T>
class SwappingBuffer
{
public:
    struct swap_buffer_t
    {
        bool is_ready;
        size_t len;
        T* data;
    };

    SwappingBuffer(const size_t num_buffs, const size_t buffer_size) :
        num_buffs(num_buffs),
        buffer_size(buffer_size),
        dual_buffers{
            swap_buffer_t{0, 0, new T[buffer_size]},
            swap_buffer_t{0, 0, new T[buffer_size]}
    },
        back_buff(&dual_buffers[0]),
        front_buff(&dual_buffers[1])
    {
    }

    ~SwappingBuffer()
    {
        back_buff = nullptr;
        front_buff = nullptr;
        delete[] dual_buffers[0].data;
        delete[] dual_buffers[1].data;
    }

    bool Write(T* data, const size_t count, const bool is_ready)
    {
        if (count > buffer_size)
        {
            return false;
        }

        return true;
    }

    swap_buffer_t* Swap()
    {
        swap_buffer_t* tmp = back_buff;
        back_buff = front_buff;
        front_buff = tmp;

        return front_buff;
    }

    swap_buffer_t* GetFront()
    {
        return front_buff;
    }

    swap_buffer_t* GetBack()
    {
        return back_buff;
    }

    bool BackIsReady() const
    {
        return back_buff->is_ready;
    }

    size_t BufferSize() const
    {
        return buffer_size;
    }

private:
    size_t num_buffs;
    size_t buffer_size;
    swap_buffer_t dual_buffers[2];
    swap_buffer_t* back_buff;
    swap_buffer_t* front_buff;
};