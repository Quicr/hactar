#pragma once

#include <cstddef>
#include <cstdint>

struct WaveSignalGenerator
{
    float frequency_hz = 440.0F;
    float sample_rate_hz = 48000.0F;
    double phase = 0.0;         // Cycle position in [0, 1); 0.25 is a quarter-cycle shift.
    uint16_t amplitude = 32767; // Center-to-peak amplitude, limited to signed 16-bit range.
    float duty_cycle = 0.5F;
};

// Output samples are signed 16-bit values stored as two's-complement uint16_t words.
// Phase is normalized to [0, 1) and is updated after every generated sample.
uint16_t SampleSineWave(WaveSignalGenerator& generator);
uint16_t SampleRampWave(WaveSignalGenerator& generator);
uint16_t SampleSawtoothWave(WaveSignalGenerator& generator);
uint16_t SampleSquareWave(WaveSignalGenerator& generator);
uint16_t SampleTriangleWave(WaveSignalGenerator& generator);

void GenerateSineWave(WaveSignalGenerator& generator, uint16_t* buffer, size_t size);
void GenerateRampWave(WaveSignalGenerator& generator, uint16_t* buffer, size_t size);
void GenerateSawtoothWave(WaveSignalGenerator& generator, uint16_t* buffer, size_t size);
void GenerateSquareWave(WaveSignalGenerator& generator, uint16_t* buffer, size_t size);
void GenerateTriangleWave(WaveSignalGenerator& generator, uint16_t* buffer, size_t size);
