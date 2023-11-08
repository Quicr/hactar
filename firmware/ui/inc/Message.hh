#pragma once

#include "String.hh"

class Message
{
public:
    Message();
    Message(const String& timestamp, const String& sender, const String& body);
    Message(const char* c_str);

    String Concatenate();
    void Timestamp(const String& timestamp);
    void Sender(const String& sender);
    void Body(const String& body);

    const String& Timestamp() const;
    const String& Sender() const;
    const String& Body() const;
    unsigned int Length();

private:
    void Parse(const char* c_str);
    void CalculateLength();

    String timestamp;
    String sender;
    String body;
    unsigned int total_length;
};