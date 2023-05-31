#pragma once

#include "Vector.hh"

// TODO move constructor and operators

class String : public Vector<char>
{
public:
    String() {}
    String(const char ch) : Vector<char>() { SetString(ch); }
    String(const char* c_str) : Vector<char>() { SetString(c_str); }
    String(const unsigned char ch) : Vector<char>() { SetString(ch); }
    String(const unsigned char* c_str) : Vector<char>() { SetString(c_str); }
    String(const String& str) : Vector<char>() { SetString(str); }
    String(const String&& str) : Vector<char>() { SetString(str); }
    virtual ~String() {}

    String& operator=(const char ch) { return SetString(ch); }
    String& operator=(const char* c_str) { return SetString(c_str); }
    String& operator=(const unsigned char ch) { return SetString(ch); }
    String& operator=(const unsigned char* c_str) { return SetString(c_str); }
    String& operator=(const String& str) { return SetString(str); }
    String& operator=(const String&& str) { return SetString(str); }

    String& operator+(const char ch)
    {
        push_back(ch);
        return *this;
    }

    String& operator+=(const char ch)
    {
        push_back(ch);
        return *this;
    }

    String& operator+(const char *c_str)
    {
        return AppendString(c_str);
    }

    String& operator+=(const char *c_str)
    {
        return AppendString(c_str);
    }

    String& operator+(const String &str)
    {
        return AppendString(str);
    }

    String& operator+=(const String &str)
    {
        return AppendString(str);
    }

    String& operator+(const String *str)
    {
        return AppendString(*str);
    }

    String& operator+=(const String *str)
    {
        return AppendString(*str);
    }

    bool operator==(const char* c_str)
    {
        // Assumed to be a c_str

        // Get the size of the c_str
        unsigned int sz = 0;
        while (c_str[sz] != '\0')
            sz++;

        // They are not the same size
        if (sz != _size)
            return false;

        // They are the same size so compare each character
        unsigned int i = 0;
        while (i < _size)
        {
            if (c_str[i] != array[i])
            {
                return false;
            }

            i++;
        }
        return true;
    }

    bool operator==(const String str)
    {
        if (_size != str.length()) return false;

        for (unsigned int i = 0; i < _size; i++)
        {
            if (array[i] != str[i]) return false;
        }
        return true;
    }

    unsigned int length() const
    {
        return _size;
    }

    const char* c_str() const
    {
        return array;
    }

    void remove(unsigned int idx)
    {
        erase(idx);
    }

    String substring(unsigned int from)
    {
        String str;

        for (unsigned int i = from; i < _size; i++)
        {
            str.push_back(array[i]);
        }

        return str;
    }

    String substring(unsigned int start, unsigned int end)
    {
        String str;

        for (unsigned int i = start; i < end && i < _size; i++)
        {
            str.push_back(array[i]);
        }

        return str;
    }

    long find(const char ch) const
    {
        for (unsigned int i = 0; i < _size; i++)
        {
            if (ch == array[i]) return i;
        }

        return -1;
    }

    long ToNumber()
    {
        long val = 0;
        char ch = 0;
        for (unsigned int i = 0; i < length(); ++i)
        {
            ch = array[i];
            if (ch < '0' || ch > '9')
                return -1;

            // Multiply by 10
            val *= 10;

            // Add the next number
            val += ch - '0';
        }

        return val;
    }

    static String int_to_string(const unsigned long input)
    {
        unsigned long value = input;
        // Get the num of powers
        unsigned long tmp = value;
        unsigned long sz = 0;
        do
        {
            sz++;
            tmp = tmp / 10;
        } while (tmp > 0);
        //159 -> 15 sz 1
        //15  -> 1 sz 2
        //1   -> 0 sz 3

        char *str = new char[sz+1];
        str[sz] = '\0';
        unsigned long mod = 0;
        do
        {
            mod = value % 10;
            str[sz-1] = '0' + mod;
            sz--;
            value = value / 10;
        } while (value > 0);

        String res = str;

        // Delete to match my new
        delete [] str;

        // Return a String
        return res;
    }

    unsigned int size() = delete;

private:

    inline String& SetString(const char ch)
    {
        this->clear();
        push_back(ch);
        return *this;
    }
    inline String& SetString(const char *c_str)
    {
        this->clear();

        char ch = c_str[_size];
        while (ch != '\0')
        {
            push_back(ch);
            ch = c_str[_size];
        }

        return *this;
    }
    inline String& SetString(const unsigned char ch)
    {
        this->clear();
        push_back(ch);
        return *this;
    }
    inline String& SetString(const unsigned char *c_str)
    {
        this->clear();

        unsigned char ch = c_str[_size];
        while (ch != '\0')
        {
            push_back(ch);
            ch = c_str[_size];
        }

        return *this;
    }
    inline String& SetString(const String &str)
    {
        this->clear();

        for (unsigned int i = 0; i < str.length(); i++)
        {
            push_back(str[i]);
        }

        return *this;
    }
    inline String& SetString(String &&str)
    {
        this->clear();
        resize(str._capacity);
        array = str.array;
        str.array = nullptr;
        return *this;
    }
    inline String& AppendString(const char *c_str)
    {
        const char *ch = c_str;
        while (*ch != '\0')
        {
            push_back(*ch);
            ch++;
        }

        return *this;
    }
    inline String& AppendString(const String &str)
    {
        unsigned int idx = 0;
        while (idx < str.length())
        {
            push_back(str[idx++]);
        }

        return *this;
    }
};