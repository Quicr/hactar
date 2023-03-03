#include "Message.hh"

Message::Message()
    : timestamp(),
      sender(),
      body(),
      total_length(0)
{
}

Message::Message(const String& timestamp,
                 const String& sender,
                 const String& body)
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

String Message::Concatenate()
{
    return timestamp + sender + body;
}

void Message::Timestamp(const String& timestamp)
{
    // Subtract the previous timestamp's length
    total_length -= this->timestamp.length();

    // Update to the new timestamp
    this->timestamp = timestamp;

    // Add the timestamp new length
    total_length += this->timestamp.length();
}

void Message::Sender(const String& sender)
{
    // Subtract the previous sender's length
    total_length -= this->sender.length();

    // Update to the new sender
    this->sender = sender;

    // Add the sender new length
    total_length += this->sender.length();
}

void Message::Body(const String& body)
{
    // Subtract the previous body's length
    total_length -= this->body.length();

    // Update to the new body
    this->body = body;

    // Add the body new length
    total_length += this->body.length();
}

const String& Message::Timestamp() const
{
    return timestamp;
}

const String& Message::Sender() const
{
    return sender;
}

const String& Message::Body() const
{
    return body;
}

void Message::Parse(const char* c_str)
{
    // TODO parse a byte stream into a message
}

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