#pragma once

// TODO function for is network, or is debug
// TODO Make a inherited class that generally knows the data and type positions


// TODO add a 0xFF start to packet to force cohesion and hide the implementation
// so the user isn't stuck having to remember it (should be custom at creation time)

/*
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  type   |      id      |       len        |.......data........|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          data                                 |
|                          ....                                 |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
class Packet
{
public:
    enum PacketTypes
    {
        NetworkDebug = 1,
        NetworkMessageSentOK,
        NetworkMessageSentError,
        UIDebug,
        UIMessage,
        UIMessageSentOK,
        UIMessageSentError,
        ReceiveOk,
        ReceiveError
    };

    // TODO important to have different data lengths for each type..

    Packet(const unsigned int created_at=0,
           const unsigned int size=0,
           const bool dynamic=true) :
        created_at(created_at),
        size(size),
        dynamic(dynamic),
        bits_in_use(0),
        data(new unsigned int[size]())
    {
        InitializeToZero(0, this->size);
    }

    // Copy
    Packet(const Packet& other)
    {
        created_at = other.created_at;
        size = other.size;
        bits_in_use = other.bits_in_use;
        dynamic = other.dynamic;

        data = new unsigned int[size];

        // Copy over the the data
        for (unsigned int i = 0; i < size; i++)
            data[i] = other.data[i];
    }

    // Move
    Packet(Packet&& other) noexcept
    {
        created_at = other.created_at;
        size = other.size;
        bits_in_use = other.bits_in_use;
        dynamic = other.dynamic;

        data = other.data;
        other.data = nullptr;
    }

    ~Packet()
    {
        if (data) delete [] data;
    }

    Packet& operator=(const Packet &other)
    {
        delete [] data;
        created_at = other.created_at;
        size = other.size;
        bits_in_use = other.bits_in_use;
        dynamic = other.dynamic;

        data = new unsigned int[size];

        // Copy over the the data
        for (unsigned int i = 0; i < size; i++)
            data[i] = other.data[i];

        return *this;
    }

    Packet& operator=(Packet &&other)
    {
        // Move operator
        delete [] data;

        created_at = other.created_at;
        size = other.size;
        bits_in_use = other.bits_in_use;
        dynamic = other.dynamic;

        data = other.data;
        other.data = nullptr;
        return *this;
    }

    // TODO only allow for integer bases
    template<typename T>
    T operator[](unsigned int idx)
    {
        return static_cast<T>(GetData(idx, sizeof(T) * 4));
    }

    void SetData(const unsigned int val, unsigned int offset_bits,
                 unsigned int bits)
    {
        // If the data is outside of the range, resize it
        if (offset_bits + bits > size * 32)
        {
            if (!dynamic) return;

            unsigned int new_size = ((offset_bits + bits)/32U) + 1;

            SetSize(new_size);
        }


        // Get the pointer of the current bits we are manipulating
        unsigned int* data_ptr = data + (offset_bits / 32);

        unsigned int val_bit = 0;

        unsigned int msb_offset;
        unsigned int data_mask;
        unsigned int in_mask;
        unsigned int iter;

        // Append how many bits are being used
        bits_in_use += bits;

        // While we still have bits to push onto the Packet
        while (bits > 0)
        {
            // Get the msb from the offset
            msb_offset = 32 - (offset_bits % 32);

            // Get the mask from the msb
            data_mask = 1U << (msb_offset - 1);

            // Get the mask for the bits we will be using
            in_mask = 1U << (bits - 1);
            iter = 0;
            while (iter < bits && iter < msb_offset)
            {
                // Clear the bit
                *data_ptr = *data_ptr & ~data_mask;

                // Get the current bit value
                val_bit = (val & in_mask);

                // Shift it to match the data bit if needed
                if (msb_offset > bits)
                    val_bit = val_bit << (msb_offset - bits);
                else if (msb_offset < bits)
                    val_bit = val_bit >> (bits - msb_offset);

                // Set the data bit to the val bit
                *data_ptr |= val_bit;

                // Shift the masks down
                data_mask = data_mask >> 1;
                in_mask = in_mask >> 1;
                ++iter;
            }

            // Remove the used up bits
            bits -= iter;

            // The offset is now zero with the next group
            offset_bits = 0;

            // Set the pointer to the next set of bits
            data_ptr += 1;
        }
    }

    void SetDataArray(const unsigned char* message, const unsigned int size,
        const unsigned int offset_bits)
    {
        const unsigned int Byte_Bits = 8;
        for (unsigned int i = 0; i < size; i++)
        {
            SetData(message[i], offset_bits + (i * Byte_Bits), Byte_Bits);
        }
    }

    // Note this function is dangerous to use and must be used
    // exclusively
    void AppendData(const unsigned int val, const unsigned int bits)
    {
        SetData(val, bits_in_use, bits);
    }

    unsigned int GetData(unsigned int offset_bits, unsigned char bits) const
    {
        if (offset_bits + bits > size * 32)
            return 0;

        // Get the data pointer from the start and offset it
        unsigned int* data_ptr = data + (offset_bits / 32);

        unsigned int output = 0;
        unsigned int msb_offset;
        unsigned int data_mask;
        unsigned int data_bit;
        unsigned int iter;

        while (bits > 0)
        {
            // Get the msb from the offset
            msb_offset = 32 - (offset_bits%32);

            // Get the mask from the msb
            data_mask = 1U << (msb_offset - 1);

            iter = 0;
            while (iter < bits && iter < msb_offset)
            {
                // Get the data
                data_bit = *data_ptr & data_mask;

                // Shift it to the spot we are starting from in the output val
                if (msb_offset > bits)
                    data_bit = data_bit >> (msb_offset - bits);
                else if (msb_offset < bits)
                    data_bit = data_bit << (bits - msb_offset);

                // Set the bit in the return value
                output |= data_bit;

                // Shift the mask to the next bit
                data_mask = data_mask >> 1;
                ++iter;
            }

            // Remove the used bits
            bits -= iter;

            // The offset is now zero with the next group
            offset_bits = 0;

            // Set the pointer to the next set of bits
            data_ptr += 1;
        }

        return output;
    }

    void SetSize(unsigned int new_size)
    {
        if (new_size == 0)
            new_size = 1;

        // Make new data
        unsigned int* new_data = new unsigned int[new_size]();

        // Copy the data
        unsigned int iter = 0;
        while (iter < new_size && iter < size)
        {
            new_data[iter] = data[iter];
            iter++;
        }

        InitializeToZero(iter, new_size);

        delete [] data;
        data = new_data;
        size = new_size;
    }

    unsigned int GetSize() const
    {
        return size;
    }

    unsigned int SizeInBytes() const
    {
        return size*4;
    }

    unsigned int BitsUsed() const
    {
        return bits_in_use;
    }

// TODO use string
    const char* ToBinaryString() const
    {
        if (this->size == 0)
            return new char[1] { '\0' };

        char *res = new char[(this->size * 32) + 1];
        unsigned int* data_ptr = data;

        unsigned int mask = 1 << 31;
        for (unsigned int i = 0; i < (this->size * 32); ++i)
        {
            if (i % 32 == 0 && i != 0)
            {
                mask = 1 << 31;
                data_ptr += 1;
            }

            res[i] = (*data_ptr & mask) ? '1' : '0';
            mask = mask >> 1;
        }

        res[(this->size * 32) + 1] = '\0';

        return res;
    }

    unsigned char* ToBytes() const
    {
        unsigned int sz = SizeInBytes();
        unsigned char* bytes = new unsigned char[sz];

        for (unsigned int i = 0; i < sz; ++i)
        {
            bytes[i] = static_cast<unsigned char>(GetData(i*8, 8));
        }

        return bytes;
    }

    unsigned int GetCreatedAt() const
    {
        return created_at;
    }

    void UpdateCreatedAt(unsigned int time)
    {
        created_at = time;
    }

private:
    void InitializeToZero(unsigned int start, unsigned int end)
    {
        if (start > end) return;
        if (start > size) return;
        if (end > size) return;
        for (unsigned int i = 0; i < end; i++)
            data[i] = 0;
    }

    // TODO rename created_at
    unsigned int created_at;
    unsigned int size;
    bool dynamic;
    unsigned int bits_in_use;
    unsigned int* data;
};