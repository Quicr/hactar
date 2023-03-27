#pragma once

class Setting
{
public:
    Setting(const unsigned short id, const unsigned short data);
    ~Setting();

    const unsigned short& id() const;

    unsigned long& data();
    const unsigned long& data() const;

    const unsigned short& address() const;
    unsigned short& address();

    unsigned char* ToBytes();

private:
    unsigned short _id;
    unsigned long _data;
    unsigned short _address;
};