#pragma once
#include <utility>
// TODO function for is network, or is debug
// TODO Make a inherited class that generally knows the data and type positions
// TODO flip a bit in type to determine if the message is main or net?
// or a couple bits.

/* Standard SerialPacket
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      type      |               id              |    len...    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      ...len  |                 data...                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                          ...data...                           |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

type = 1 byte
id = 2 bytes
len = 2 bytes
data = len bytes

*/

/* Message SerialPacket
len above = 27 bytes + data length
m_type = 1 byte
publish_uri = 16 bytes
expiry_time = 4 bytes
creation_time = 6 bytes
*/

// TODO add start patterns

// TODO move all other enum classes into appropriate classes

// @note This is assumed to be little endian architectures
class SerialPacket
{
public:
    enum class Types
    {
        LocalDebug = 1,
        QMessage,
        Setting,
        Command,
    };

    enum class QMessages
    {
        // TODO use this instead?
        // pretty much move it to qchat.
        Text = 1,
        GetRooms,
        WatchRoom,
        UnwatchRoom,
        Audio
        // TODO
    };

    enum class Settings
    {
        // TODO
    };

    enum class Commands
    {
        // Commands for wifi
        Wifi = 1,

        // TODO move into qmessages
        RoomsGet
    };

    enum class User
    {
        UserGet = 1,
        UserLogin,
        UserLogout
    };

    enum class WifiTypes
    {
        Status = 1,
        SSIDs,
        Connect,
        Disconnect,
        Disconnected,
        FailedToConnect,
        SignalStrength,
    };

    SerialPacket(const unsigned int created_at = 0,
        const unsigned int capacity = 1,
        const bool dynamic = true):
        created_at(created_at),
        capacity(capacity),
        dynamic(dynamic),
        size(0),
        retries(0),
        data(new unsigned char[capacity]{0})
    {
    }

    // Copy
    SerialPacket(const SerialPacket& other) :
        created_at(other.created_at),
        capacity(other.capacity),
        dynamic(other.dynamic),
        size(other.size),
        retries(other.retries),
        data(new unsigned char[capacity]{0})
    {
        for (unsigned int i = 0; i < capacity; ++i)
        {
            data[i] = other.data[i];
        }

    }

    // Move
    SerialPacket(SerialPacket&& other) noexcept :
        created_at(other.created_at),
        capacity(other.capacity),
        dynamic(other.dynamic),
        size(other.size),
        retries(other.retries),
        data(other.data)
    {
        other.created_at = 0;
        other.capacity = 0;
        other.dynamic = 0;
        other.size = 0;
        other.retries = 0;
        other.data = nullptr;
    }

    ~SerialPacket()
    {
        delete [] data;
    }

    SerialPacket& operator=(const SerialPacket& other)
    {
        if (data)
        {
            delete [] data;
        }
        created_at = other.created_at;
        capacity = other.capacity;
        size = other.size;
        dynamic = other.dynamic;
        retries = other.retries;

        data = new unsigned char[capacity];

        // Copy over the the data
        for (unsigned int i = 0; i < capacity; i++)
        {
            data[i] = other.data[i];
        }

        return *this;
    }

    SerialPacket& operator=(SerialPacket&& other)
    {
        // Move operator
        if (data)
        {
            delete [] data;
        }

        created_at = other.created_at;
        capacity = other.capacity;
        size = other.size;
        dynamic = other.dynamic;
        retries = other.retries;

        data = other.data;
        other.data = nullptr;
        return *this;
    }

    unsigned char& operator[](unsigned int idx)
    {
        return data[idx];
    }

    // template<typename T, bool = std::is_arithmetic<T>::value>
    // inline void SetData(const T val)
    // {
    //     UpdateData(val, size, -1);
    // }

    template<typename T, bool = std::is_arithmetic<T>::value>
    inline void SetData(const T val,
        unsigned int num_bytes)
    {
        UpdateData(val, size, num_bytes);
    }

    template<typename T, bool = std::is_arithmetic<T>::value>
    inline void SetData(const T val,
        unsigned int offset,
        const int num_bytes)
    {
        UpdateData(val, offset, num_bytes);
    }

    template <typename T, typename K, bool = std::is_integral<K>::value>
    typename std::enable_if<!std::is_lvalue_reference<K>::value, void>::type
        SetData(const T val,
            K offset,
            const int num_bytes)
    {
        UpdateData(val, offset, num_bytes);
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
            offset += UpdateData(val[i], offset, num_bytes);
        }
    }

    template <typename T, typename std::enable_if<std::is_fundamental<T>::value, T>::type=0>
    T GetData(const unsigned int offset, const int num_bytes) const
    {
        size_t byte_width = sizeof(T);
        if (num_bytes > 0)
        {
            byte_width = num_bytes;
        }

        T output = 0;
        GetData(output, offset, byte_width);

        return output;
    }

    template <typename T>
    void GetData(T& output, const unsigned int offset, const int num_bytes) const
    {
        output = 0;

        size_t byte_width = sizeof(output);
        if (num_bytes > 0)
        {
            byte_width = num_bytes;
        }

        unsigned char* output_ptr = (unsigned char*)(void*)&output;
        unsigned char* data_ptr = data + offset;

        for (unsigned int i = 0; i < byte_width && i < capacity; ++i)
        {
            *output_ptr = *data_ptr;
            output_ptr++;
            data_ptr++;
        }
    }

    unsigned char* Buffer() const
    {
        unsigned char* buffer = new unsigned char[capacity];

        for (unsigned int i = 0; i < capacity; ++i)
        {
            buffer[i] = data[i];
        }
        return buffer;
    }

    unsigned char* Data() const
    {
        return data;
    }

    void SetCapacity(unsigned int new_capacity)
    {
        if (new_capacity == 0)
            new_capacity = 1;

        // Make new data
        unsigned char* new_data = new unsigned char[new_capacity]{0};

        // Copy the data
        unsigned int iter = 0;
        while (iter < new_capacity && iter < capacity)
        {
            new_data[iter] = data[iter];
            iter++;
        }

        delete [] data;
        data = new_data;
        capacity = new_capacity;
    }

    unsigned int Capacity() const
    {
        return capacity;
    }

    unsigned int NumBytes() const
    {
        return size;
    }

    unsigned int NumBits() const
    {
        return size * 8;
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

    template <typename T>
    unsigned int inline UpdateData(const T val,
        unsigned int offset,
        const int num_bytes)
    {
        unsigned int in_sz = sizeof(T);
        if (num_bytes > 0)
        {
            in_sz = num_bytes;
        }

        if (offset + in_sz > capacity)
        {
            // Resize
            if (!dynamic) return 0;
            while (offset + in_sz >= capacity)
            {
                // Double the size lazily
                DoubleCapacity();
            }
        }

        unsigned char* input_ptr = (unsigned char*)(void*)&val;
        unsigned char* data_ptr = data + offset;
        for (unsigned int i = 0; i < in_sz && i < capacity; ++i)
        {
            *data_ptr = *input_ptr;
            data_ptr++;
            input_ptr++;
        }

        // Only update the size if the offset bytes are greater than
        // the current size. 3 Scenarios
        // 1. You set a byte where: offset + 1 > size
        // 2. You set a byte where: offset + 1 <= size and nothing happens
        // 3. Append data where size + 1 > size
        if (offset + num_bytes > size)
        {
            size = offset + num_bytes;
        }

        return in_sz;
    }

    void inline DoubleCapacity()
    {
        SetCapacity(capacity * 2);
    }

    // TODO rename created_at
    unsigned int created_at;
    unsigned int capacity;
    bool dynamic;
    unsigned int size; // last byte used
    unsigned int retries;
    unsigned char* data;
};