#pragma once

template <typename T>
class Setting
{
public:
    Setting(const unsigned short id);
    ~Setting();

    const unsigned short& id() const;

    const unsigned short& address() const;
    unsigned short& address();

    unsigned char* ToBytes();

private:
    unsigned short _id;
    unsigned short _address;
};