#include "message.hh"

Message::Message()
    : timestamp(),
      sender(),
      body(),
      total_length(0)
{
}

Message::Message(const std::string& timestamp,
                 const std::string& sender,
                 const std::string& body)
    : timestamp(timestamp),
      sender(sender),
      body(body),
      total_length(0)
{
    CalculateLength();
}

Message::Message(const char* c_str)
    : timestamp(),
      sender(),
      body(),
      total_length(0)
{
    Parse(c_str);
    CalculateLength();
}

std::string Message::Concatenate()
{
    return timestamp + sender + body;
}

void Message::Timestamp(const std::string& timestamp)
{
    // Subtract the previous timestamp's length
    total_length -= this->timestamp.length();

    // Update to the new timestamp
    this->timestamp = timestamp;

    // Add the timestamp new length
    total_length += this->timestamp.length();
}

void Message::Sender(const std::string& sender)
{
    // Subtract the previous sender's length
    total_length -= this->sender.length();

    // Update to the new sender
    this->sender = sender;

    // Add the sender new length
    total_length += this->sender.length();
}

void Message::Body(const std::string& body)
{
    // Subtract the previous body's length
    total_length -= this->body.length();

    // Update to the new body
    this->body = body;

    // Add the body new length
    total_length += this->body.length();
}

const std::string& Message::Timestamp() const
{
    return timestamp;
}

const std::string& Message::Sender() const
{
    return sender;
}

const std::string& Message::Body() const
{
    return body;
}

// void Message::Parse(const char* c_str)
// {
//     // TODO parse a byte stream into a message
// }

unsigned int Message::Length()
{
    return total_length;
}

void Message::CalculateLength()
{
    total_length = this->timestamp.length() +
                   this->sender.length() +
                   this->body.length();
}