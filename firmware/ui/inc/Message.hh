#pragma once

#include <string>

class Message
{
public:
    Message();
    Message(const std::string& timestamp, const std::string& sender, const std::string& body);
    Message(const char* c_str);

    std::string Concatenate();
    void Timestamp(const std::string& timestamp);
    void Sender(const std::string& sender);
    void Body(const std::string& body);

    const std::string& Timestamp() const;
    const std::string& Sender() const;
    const std::string& Body() const;
    unsigned int Length();

private:
    void Parse(const char* c_str);
    void CalculateLength();

    std::string timestamp;
    std::string sender;
    std::string body;
    unsigned int total_length;
};