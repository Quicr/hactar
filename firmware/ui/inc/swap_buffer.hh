#pragma once

template <typename T>
class SwapBuffer
{
public:
    struct swap_buffer_t
    {
        GPIO_PinState dc_level;
        bool is_ready;
        size_t len;
        T* data;
    };

    SwapBuffer(size_t buffer_size):
        buffer_size(buffer_size),
        dual_buffers{ swap_buffer_t{GPIO_PIN_RESET, 0, 0, new T[buffer_size]},
                      swap_buffer_t{GPIO_PIN_RESET, 0, 0, new T[buffer_size]} },
        back_buff(&dual_buffers[0]),
        front_buff(&dual_buffers[1])
    {

    }

    ~SwapBuffer()
    {
        back_buff = nullptr;
        front_buff = nullptr;
        delete [] dual_buffers[0].data;
        delete [] dual_buffers[1].data;
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
    size_t buffer_size;
    swap_buffer_t dual_buffers[2];
    swap_buffer_t* back_buff;
    swap_buffer_t* front_buff;


};