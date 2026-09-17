#include "wave_signal_generator.hh"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace
{
int failures = 0;

void Check(bool condition, std::string_view description)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << description << '\n';
        ++failures;
    }
}

template <size_t Size>
void CheckSamples(const std::array<uint16_t, Size>& actual,
                  const std::array<uint16_t, Size>& expected,
                  uint16_t tolerance,
                  std::string_view description)
{
    for (size_t i = 0; i < Size; ++i)
    {
        const int difference = std::abs(static_cast<int>(actual[i]) - expected[i]);
        if (difference > tolerance)
        {
            std::cerr << "FAILED: " << description << " at sample " << i << ": expected "
                      << expected[i] << ", got " << actual[i] << '\n';
            ++failures;
        }
    }
}

WaveSignalGenerator QuarterCycleGenerator()
{
    WaveSignalGenerator generator;
    generator.frequency_hz = 1.0F;
    generator.sample_rate_hz = 4.0F;
    return generator;
}

void TestWaveShapes()
{
    std::array<uint16_t, 4> samples{};

    auto generator = QuarterCycleGenerator();
    GenerateSineWave(generator, samples.data(), samples.size());
    CheckSamples(samples, std::array<uint16_t, 4>{0, 32767, 0, 32769}, 0, "sine wave");

    generator = QuarterCycleGenerator();
    GenerateRampWave(generator, samples.data(), samples.size());
    CheckSamples(samples, std::array<uint16_t, 4>{32769, 49152, 0, 16384}, 0, "ramp wave");

    generator = QuarterCycleGenerator();
    GenerateSawtoothWave(generator, samples.data(), samples.size());
    CheckSamples(samples, std::array<uint16_t, 4>{32767, 16384, 0, 49152}, 0, "sawtooth wave");

    generator = QuarterCycleGenerator();
    GenerateSquareWave(generator, samples.data(), samples.size());
    CheckSamples(samples, std::array<uint16_t, 4>{32767, 32767, 32769, 32769}, 0, "square wave");

    generator = QuarterCycleGenerator();
    GenerateTriangleWave(generator, samples.data(), samples.size());
    CheckSamples(samples, std::array<uint16_t, 4>{32769, 0, 32767, 0}, 0, "triangle wave");
}

void TestAmplitude()
{
    auto generator = QuarterCycleGenerator();
    generator.amplitude = 1000;

    std::array<uint16_t, 4> samples{};
    GenerateSineWave(generator, samples.data(), samples.size());
    CheckSamples(samples, std::array<uint16_t, 4>{0, 1000, 0, 64536}, 0, "configured amplitude");
}

void TestPhaseAndBufferContinuity()
{
    auto generator = QuarterCycleGenerator();
    generator.phase = 0.25F;

    std::array<uint16_t, 2> first{};
    std::array<uint16_t, 2> second{};
    GenerateSineWave(generator, first.data(), first.size());
    GenerateSineWave(generator, second.data(), second.size());

    CheckSamples(first, std::array<uint16_t, 2>{32767, 0}, 0, "initial phase");
    CheckSamples(second, std::array<uint16_t, 2>{32769, 0}, 0, "buffer continuity");
    Check(std::abs(generator.phase - 0.25F) < 0.000001F,
          "phase returns to its starting position after one cycle");
}

void TestDutyCycle()
{
    auto generator = QuarterCycleGenerator();
    generator.duty_cycle = 0.25F;

    std::array<uint16_t, 4> samples{};
    GenerateSquareWave(generator, samples.data(), samples.size());
    CheckSamples(samples, std::array<uint16_t, 4>{32767, 32769, 32769, 32769}, 0,
                 "square-wave duty cycle");
}

void TestTwosComplementEncoding()
{
    auto generator = QuarterCycleGenerator();
    generator.amplitude = 1;

    Check(SampleSquareWave(generator) == 1, "+1 is encoded as 0x0001");
    generator.phase = 0.5;
    Check(SampleSquareWave(generator) == UINT16_MAX, "-1 is encoded as 0xffff");
}

void TestSingleSampleFunctions()
{
    auto generator = QuarterCycleGenerator();
    Check(SampleSineWave(generator) == 0, "single sine sample");
    Check(SampleSineWave(generator) == 32767, "single sine sample advances phase");

    generator = QuarterCycleGenerator();
    Check(SampleRampWave(generator) == 32769, "single ramp sample");

    generator = QuarterCycleGenerator();
    Check(SampleSawtoothWave(generator) == 32767, "single sawtooth sample");

    generator = QuarterCycleGenerator();
    Check(SampleSquareWave(generator) == 32767, "single square sample");

    generator = QuarterCycleGenerator();
    Check(SampleTriangleWave(generator) == 32769, "single triangle sample");
}

void TestOneMinuteOfSineSamples()
{
    constexpr size_t Sample_Rate = 48'000;
    constexpr size_t Duration_Seconds = 60;
    constexpr size_t Sample_Count = Sample_Rate * Duration_Seconds;
    constexpr double Frequency_Hz = 440.0;
    constexpr double Two_Pi = 6.28318530717958647692;
    constexpr int Max_Allowed_Error = 2;

    WaveSignalGenerator generator = {
        .frequency_hz = static_cast<float>(Frequency_Hz),
        .sample_rate_hz = static_cast<float>(Sample_Rate),
        .phase = 0.0F,
        .amplitude = 32767,
    };

    int maximum_error = 0;
    size_t maximum_error_sample = 0;
    for (size_t i = 0; i < Sample_Count; ++i)
    {
        const uint16_t encoded_sample = SampleSineWave(generator);
        const int32_t actual = encoded_sample <= 32767
                                 ? static_cast<int32_t>(encoded_sample)
                                 : static_cast<int32_t>(encoded_sample) - 65536;
        const double phase = std::fmod(static_cast<double>(i) * Frequency_Hz / Sample_Rate, 1.0);
        const int32_t expected =
            static_cast<int32_t>(std::lround(32767.0 * std::sin(Two_Pi * phase)));
        const int error = std::abs(actual - expected);
        if (error > maximum_error)
        {
            maximum_error = error;
            maximum_error_sample = i;
        }
    }

    if (maximum_error > Max_Allowed_Error)
    {
        std::cerr << "FAILED: one-minute sine maximum error was " << maximum_error << " at sample "
                  << maximum_error_sample << '\n';
        ++failures;
    }
    else
    {
        std::cout << "One-minute sine: " << Sample_Count << " samples, maximum error "
                  << maximum_error << " PCM count(s)\n";
    }
    Check(std::abs(generator.phase) < 0.00000001 || std::abs(generator.phase - 1.0) < 0.00000001,
          "one-minute sine returns to phase zero after 26400 cycles");
}

void TestEmptyOutputDoesNotAdvancePhase()
{
    auto generator = QuarterCycleGenerator();
    generator.phase = 0.25F;
    GenerateSineWave(generator, nullptr, 4);
    Check(generator.phase == 0.25F, "null output does not advance phase");

    uint16_t sample = 123;
    GenerateSineWave(generator, &sample, 0);
    Check(generator.phase == 0.25F, "empty output does not advance phase");
    Check(sample == 123, "empty output does not modify the buffer");
}
} // namespace

int main()
{
    TestWaveShapes();
    TestAmplitude();
    TestPhaseAndBufferContinuity();
    TestDutyCycle();
    TestTwosComplementEncoding();
    TestSingleSampleFunctions();
    TestOneMinuteOfSineSamples();
    TestEmptyOutputDoesNotAdvancePhase();

    if (failures != 0)
    {
        std::cerr << failures << " waveform test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All waveform tests passed\n";
    return EXIT_SUCCESS;
}
