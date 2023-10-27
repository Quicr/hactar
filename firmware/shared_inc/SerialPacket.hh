#pragma once

// TODO function for is network, or is debug
// TODO Make a inherited class that generally knows the data and type positions

#include <utility>

/* Standard SerialPacket
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  type   |      id      |       len        |.......data........|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          data                                 |
|                          ....                                 |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

/* Message SerialPacket
len above = 27 bytes + data length
m_type = 1 byte
publish_uri = 16 bytes
expiry_time = 4 bytes
creation_time = 6 bytes
*/

// TODO add start patterns

// TODO rewrite SerialPacket to use 8 bits instead of 32 bits

class SerialPacket
{
public:
    enum Types
    {
        Ok = 1,
        Error,
        Busy,
        Debug,
        LocalDebug,
        Message,
        Setting,
        Command,
    };

    enum Settings
    {
        // TODO
    };

    enum Commands
    {
        SSIDs = 1,
        ConnectToSSID,
        WifiStatus
    };

    enum MessageTypes
    {
        Ascii = 1,
        Watch,
        Unwatch
    };

    // TODO important to have different data lengths for each type..

    SerialPacket(const unsigned int created_at = 0,
        const unsigned int size = 0,
        const bool dynamic = true):
        created_at(created_at),
        size(size),
        dynamic(dynamic),
        bits_in_use(0),
        retries(0),
        data(new unsigned char[size]())
    {
        InitializeToZero(0, this->size);
    }

    // Copy
    SerialPacket(const SerialPacket& other)
    {
        *this = other;
    }

    // Move
    SerialPacket(SerialPacket&& other) noexcept
    {
        *this = std::move(other);
    }

    ~SerialPacket()
    {
        delete [] data;
    }

    SerialPacket& operator=(const SerialPacket& other)
    {
        delete [] data;
        created_at = other.created_at;
        size = other.size;
        bits_in_use = other.bits_in_use;
        dynamic = other.dynamic;
        retries = other.retries;

        data = new unsigned char[size];

        // Copy over the the data
        for (unsigned int i = 0; i < size; i++)
            data[i] = other.data[i];

        return *this;
    }

    SerialPacket& operator=(SerialPacket&& other)
    {
        // Move operator
        delete [] data;

        created_at = other.created_at;
        size = other.size;
        bits_in_use = other.bits_in_use;
        dynamic = other.dynamic;
        retries = other.retries;

        data = other.data;
        other.data = nullptr;
        return *this;
    }

    // TODO only allow for integer bases
    template<typename T, bool = std::is_arithmetic<T>::value>
    T operator[](unsigned int idx)
    {
        // TODO
        return static_cast<T>(GetData(idx, sizeof(T) * 4));
    }

    template<typename T, bool = std::is_arithmetic<T>::value>
    inline void SetData(const T val,
        unsigned int& offset,
        const unsigned int num_bytes)
    {
        // Get the end of the offset + number of bytes are are adding
        const unsigned int in_sz = sizeof(val);
        const unsigned int end = in_sz + offset;
        unsigned char* input = (unsigned char*)(void*)&val;

        if (end > size)
        {
            // Resize
            if (!dynamic) return;
            SetSize(end);
        }

        unsigned int i_iter = 0;
        while (offset < end && i_iter < in_sz)
        {
            data[offset++] = input[i_iter++];
        }
    }


    template <typename T, typename K, bool = std::is_arithmetic<K>::value>
    typename std::enable_if<!std::is_lvalue_reference<K>::value, void>::type
        SetData(const T val,
            unsigned int offset,
            const unsigned int num_bytes)
    {
        SetData(val, offset, num_bytes);
    }

    /**
     * @brief Takes an array and saves the entire array to a byte array
     *  @note num_bytes is the number of bytes to be pushed in per element
     */
    template<typename T>
    void SetData(const T* val,
        const unsigned int sz,
        unsigned int& offset,
        const unsigned int num_bytes)
    {
        for (unsigned int i = 0; i < sz; ++i)
        {
            SetData(val[i], offset, num_bytes);
        }
    }

    template <typename T>
    T GetData(unsigned int offset_bits, unsigned char bits) const
    {

    }

    void SetSize(unsigned int new_size)
    {
        if (new_size == 0)
            new_size = 1;

        // Make new data
        unsigned char* new_data = new unsigned char[new_size] {};

        // Copy the data
        unsigned int iter = 0;
        while (iter < new_size && iter < size)
        {
            new_data[iter] = data[iter];
            iter++;
        }

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
        return size * 4;
    }

    unsigned int BitsUsed() const
    {
        return bits_in_use;
    }

    unsigned int BytesInUse() const
    {
        unsigned int bytes = bits_in_use / 8;

        // ex 1 bit = 1 byte in use, 8 bits = 1 byte in use,
        // and 9 bits = 2 bytes in use, 16 bits = 2 bytes in use etc.
        unsigned int floating_bits = bits_in_use % 8;
        if (floating_bits > 0)
        {
            bytes += 1;
        }

        return bytes;
    }

    // TODO use string
    // TODO review
    const char* ToBinaryString() const
    {
        if (this->size == 0)
            return new char[1] { '\0' };

        char* res = new char[(this->size * 32) + 1];
        unsigned char* data_ptr = data;

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

    // TODO review
    unsigned char* ToBytes() const
    {
        unsigned int sz = SizeInBytes();
        unsigned char* bytes = new unsigned char[sz];

        for (unsigned int i = 0; i < sz; ++i)
        {
            bytes[i] = static_cast<unsigned char>(GetData(i * 8, 8));
        }

        return bytes;
    }

    // TODO review
    unsigned char* ToBytes(unsigned char start_byte) const
    {
        unsigned int sz = SizeInBytes();
        unsigned char* bytes = new unsigned char[sz + 1];
        bytes[0] = start_byte;

        for (unsigned int i = 0; i < sz; ++i)
        {
            bytes[i + 1] = static_cast<unsigned char>(GetData(i * 8, 8));
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

    unsigned int GetRetries() const
    {
        return retries;
    }

    void IncrementRetry()
    {
        retries++;
    }

protected:
    void InitializeToZero(unsigned int start, unsigned int end)
    {
        if (start > end) return;
        if (start > size) return;
        if (end > size) return;
        if (data == nullptr) return;
        for (unsigned int i = 0; i < end; i++)
            data[i] = 0;
    }

    // TODO rename created_at
    unsigned int created_at;
    unsigned int size;
    bool dynamic;
    unsigned int bits_in_use;
    unsigned int retries;
    unsigned char* data;
};