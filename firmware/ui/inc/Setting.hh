#pragma once


// TODO struct?
class Setting
{
public:
    Setting(const unsigned short id, const unsigned short data);
    ~Setting();

    const unsigned short& id() const;

    unsigned long& data();
    const unsigned long& data() const;

    const unsigned short& address() const;

private:
    // THINK Should this come from the eeprom?
    static unsigned short Next_Address;

    unsigned short _id;
    unsigned long _data;
    unsigned short _address;
};