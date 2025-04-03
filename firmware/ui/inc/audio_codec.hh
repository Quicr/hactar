#pragma once


#include <iostream>

#include <map>
#include <cstdint>

class AudioCodec
{
public:
    AudioCodec() = delete;
    ~AudioCodec() = delete;

    static void ALawCompand(const uint16_t* input, const size_t input_len,
        uint8_t* output, const size_t output_len, const bool input_stereo, const bool output_stereo);

    static void ALawExpand(const uint8_t* input, const size_t input_len,
        uint16_t* output, const size_t output_len, const bool input_stereo, const bool output_stereo);

    static void ALawCompand(const uint16_t* input, uint8_t* output, const size_t len);
    static void ALawExpand(const uint8_t* input, uint16_t* output, const size_t len);

    static uint8_t ALawCompand(const uint16_t sample);
    static uint16_t ALawExpand(uint8_t sample);
private:
};