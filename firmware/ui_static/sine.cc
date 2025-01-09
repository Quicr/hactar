#include <iostream>
#include <math.h>

void SampleSineWave(uint16_t* buff, const uint16_t num_samples,
    const uint16_t start_idx, const double amplitude, const double freq,
    double& phase, const uint16_t sample_rate, const bool stereo)
{
    constexpr uint16_t offset = 2000;
    constexpr double TWO_PI = M_PI * 2;
    const double angular_freq = TWO_PI * freq;
    double current_phase = 0.0f;
    uint16_t samples = num_samples;
    if (stereo)
    {
        samples = samples / 2;

        for (uint16_t i = 0; i < samples; ++i)
        {
            const double step = (double)i / sample_rate;
            const double sample = amplitude * sin(angular_freq * step + phase);

            // Add offset to handle negative numbers and overflow back around
            // to their regular values for the positive numbers
            const uint16_t int_sample = uint16_t(offset + sample) + 1;

            buff[start_idx + (i * 2)] = int_sample;
            buff[start_idx + (i * 2 + 1)] = int_sample;
        }
    }
    else
    {
        for (uint16_t i = 0; i < samples; ++i)
        {
            const double step = (double)i / sample_rate;
            const double sample = amplitude * sin(angular_freq * step + phase);

            // Add offset to handle negative numbers and overflow back around
            // to their regular values for the positive numbers
            buff[start_idx + i] = uint16_t(offset + sample);
        }
    }

    phase += angular_freq * (double(samples) / sample_rate);
    while (phase > TWO_PI)
    {
        phase -= TWO_PI;
    }
}

int main()
{
    uint16_t num_samples = 340;
    uint16_t sample_rate = 16'000;
    uint16_t buff[num_samples];
    uint16_t amp = 1000;
    double freq = 425;
    double phase = 0;


    SampleSineWave(buff, num_samples, 0, amp, freq, phase, sample_rate, false);


    for (int i = 0; i < num_samples; ++i)
    {
        std::cout << buff[i] << std::endl;
    }
}