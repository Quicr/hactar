#include "Setting.hh"

unsigned short Next_Address = 0;

Setting::Setting(const unsigned short id, const unsigned short data) :
    _id(id),
    _data(data),
    _address(Next_Address)
{
    Next_Address += 32;
}

Setting::~Setting()
{

}

const unsigned short& Setting::id() const
{
    return _id;
}


unsigned long& Setting::data()
{
    return _data;
}

const unsigned long& Setting::data() const
{
    return _data;
}

const unsigned short& Setting::address() const
{
    return _address;
}



