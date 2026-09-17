#include "wave_signal_generator.hh"
#include <algorithm>
#include <cmath>

namespace
{
constexpr float Two_Pi = 6.28318530717958647692F;

double NormalizePhase(double phase)
{
    phase -= std::floor(phase);
    return phase < 0.0 ? phase + 1.0 : phase;
}

double CyclesPerSample(const WaveSignalGenerator& generator)
{
    if (!std::isfinite(generator.frequency_hz) || !std::isfinite(generator.sample_rate_hz)
        || generator.sample_rate_hz <= 0.0F)
    {
        return 0.0;
    }

    const double cycles_per_sample =
        static_cast<double>(generator.frequency_hz) / generator.sample_rate_hz;
    return std::isfinite(cycles_per_sample) ? cycles_per_sample : 0.0;
}

uint16_t EncodeWaveform(const WaveSignalGenerator& generator, float waveform)
{
    constexpr uint16_t Max_Amplitude = 32767;
    const float amplitude = static_cast<float>(std::min(generator.amplitude, Max_Amplitude));
    const long signed_sample = std::lround(amplitude * std::clamp(waveform, -1.0F, 1.0F));
    return static_cast<uint16_t>(signed_sample);
}

template <typename SampleFunction>
void GenerateWave(WaveSignalGenerator& generator,
                  uint16_t* buffer,
                  size_t size,
                  SampleFunction sample_function)
{
    if (buffer == nullptr || size == 0)
    {
        return;
    }

    double phase_cycles = std::isfinite(generator.phase) ? NormalizePhase(generator.phase) : 0.0;
    const double cycles_per_sample = CyclesPerSample(generator);

    for (size_t i = 0; i < size; ++i)
    {
        const float waveform = sample_function(phase_cycles);
        buffer[i] = EncodeWaveform(generator, waveform);
        phase_cycles = NormalizePhase(phase_cycles + cycles_per_sample);
    }

    generator.phase = phase_cycles;
}
} // namespace

void GenerateSineWave(WaveSignalGenerator& generator, uint16_t* buffer, size_t size)
{
    GenerateWave(generator, buffer, size,
                 [](float phase_cycles) { return std::sin(Two_Pi * phase_cycles); });
}

void GenerateRampWave(WaveSignalGenerator& generator, uint16_t* buffer, size_t size)
{
    GenerateWave(generator, buffer, size,
                 [](float phase_cycles) { return 2.0F * phase_cycles - 1.0F; });
}

void GenerateSawtoothWave(WaveSignalGenerator& generator, uint16_t* buffer, size_t size)
{
    GenerateWave(generator, buffer, size,
                 [](float phase_cycles) { return 1.0F - 2.0F * phase_cycles; });
}

void GenerateSquareWave(WaveSignalGenerator& generator, uint16_t* buffer, size_t size)
{
    const float duty_cycle = std::clamp(generator.duty_cycle, 0.0F, 1.0F);
    GenerateWave(generator, buffer, size, [duty_cycle](float phase_cycles) {
        return phase_cycles < duty_cycle ? 1.0F : -1.0F;
    });
}

void GenerateTriangleWave(WaveSignalGenerator& generator, uint16_t* buffer, size_t size)
{
    GenerateWave(generator, buffer, size, [](float phase_cycles) {
        return phase_cycles < 0.5F ? 4.0F * phase_cycles - 1.0F : 3.0F - 4.0F * phase_cycles;
    });
}

uint16_t SampleSineWave(WaveSignalGenerator& generator)
{
    uint16_t sample = 0;
    GenerateSineWave(generator, &sample, 1);
    return sample;
}

uint16_t SampleRampWave(WaveSignalGenerator& generator)
{
    uint16_t sample = 0;
    GenerateRampWave(generator, &sample, 1);
    return sample;
}

uint16_t SampleSawtoothWave(WaveSignalGenerator& generator)
{
    uint16_t sample = 0;
    GenerateSawtoothWave(generator, &sample, 1);
    return sample;
}

uint16_t SampleSquareWave(WaveSignalGenerator& generator)
{
    uint16_t sample = 0;
    GenerateSquareWave(generator, &sample, 1);
    return sample;
}

uint16_t SampleTriangleWave(WaveSignalGenerator& generator)
{
    uint16_t sample = 0;
    GenerateTriangleWave(generator, &sample, 1);
    return sample;
}
