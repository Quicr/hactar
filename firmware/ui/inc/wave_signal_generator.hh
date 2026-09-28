#pragma once

#include <cstdint>

enum class WaveType
{
    Ramp,
    Sine,
    Square,
};

class WaveSignalGenerator
{
public:
    explicit WaveSignalGenerator(WaveType wave_type,
                                 float frequency_hz = 440.0F,
                                 float sample_rate_hz = 48000.0F,
                                 double phase = 0.0,
                                 uint16_t amplitude = 32767,
                                 float duty_cycle = 0.5F) :
        frequency_hz(frequency_hz),
        sample_rate_hz(sample_rate_hz),
        phase(phase),
        amplitude(amplitude),
        duty_cycle(duty_cycle),
        wave_type(wave_type)
    {
    }

    // Returns one unsigned, DC-biased sample and advances phase.
    uint16_t Sample();

    float frequency_hz;
    float sample_rate_hz;
    double phase;       // Cycle position in [0, 1); 0.25 is a quarter-cycle shift.
    uint16_t amplitude; // Maximum output value, limited to 0x7fff.
    float duty_cycle;

private:
    static double NormalizePhase(double phase);
    double CyclesPerSample() const;
    uint16_t EncodeWaveform(float waveform) const;
    uint16_t SampleRampWave() const;
    uint16_t SampleSineWave() const;
    uint16_t SampleSquareWave() const;

    WaveType wave_type;
};
