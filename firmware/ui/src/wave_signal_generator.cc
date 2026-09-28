#include "wave_signal_generator.hh"
#include <algorithm>
#include <cmath>

constexpr float Two_Pi = 6.28318530717958647692F;

double WaveSignalGenerator::NormalizePhase(double phase)
{
    phase -= std::floor(phase);
    return phase < 0.0 ? phase + 1.0 : phase;
}

double WaveSignalGenerator::CyclesPerSample() const
{
    if (!std::isfinite(frequency_hz) || !std::isfinite(sample_rate_hz) || sample_rate_hz <= 0.0F)
    {
        return 0.0;
    }

    const double cycles_per_sample = static_cast<double>(frequency_hz) / sample_rate_hz;
    return std::isfinite(cycles_per_sample) ? cycles_per_sample : 0.0;
}

uint16_t WaveSignalGenerator::EncodeWaveform(float waveform) const
{
    constexpr uint16_t Max_Amplitude = 32767;
    const float clamped_amplitude = static_cast<float>(std::min(amplitude, Max_Amplitude));
    const float biased_waveform = (std::clamp(waveform, -1.0F, 1.0F) + 1.0F) * 0.5F;
    return static_cast<uint16_t>(std::lround(clamped_amplitude * biased_waveform));
}

uint16_t WaveSignalGenerator::Sample()
{
    phase = std::isfinite(phase) ? NormalizePhase(phase) : 0.0;

    uint16_t sample = 0;
    switch (wave_type)
    {
    case WaveType::Ramp:
        sample = SampleRampWave();
        break;
    case WaveType::Sine:
        sample = SampleSineWave();
        break;
    case WaveType::Square:
        sample = SampleSquareWave();
        break;
    }

    phase = NormalizePhase(phase + CyclesPerSample());
    return sample;
}

uint16_t WaveSignalGenerator::SampleRampWave() const
{
    return EncodeWaveform(2.0F * static_cast<float>(phase) - 1.0F);
}

uint16_t WaveSignalGenerator::SampleSineWave() const
{
    return EncodeWaveform(std::sin(Two_Pi * static_cast<float>(phase)));
}

uint16_t WaveSignalGenerator::SampleSquareWave() const
{
    const float clamped_duty_cycle = std::clamp(duty_cycle, 0.0F, 1.0F);
    return EncodeWaveform(phase < clamped_duty_cycle ? 1.0F : -1.0F);
}
