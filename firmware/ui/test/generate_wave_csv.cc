#include "wave_signal_generator.hh"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

namespace
{
using GenerateFunction = void (*)(WaveSignalGenerator&, uint16_t*, size_t);

GenerateFunction GeneratorFor(std::string_view waveform)
{
    if (waveform == "sine")
        return GenerateSineWave;
    if (waveform == "ramp")
        return GenerateRampWave;
    if (waveform == "sawtooth")
        return GenerateSawtoothWave;
    if (waveform == "square")
        return GenerateSquareWave;
    if (waveform == "triangle")
        return GenerateTriangleWave;
    return nullptr;
}

void PrintUsage(std::string_view program)
{
    std::cerr << "Usage: " << program
              << " <waveform> <cycles> <frequency_hz> <sample_rate_hz> <amplitude> <output.csv>"
                 " [phase] [duty_cycle]\n"
                 "Waveforms: sine, ramp, sawtooth, square, triangle\n"
                 "Example: "
              << program << " sine 3 440 48000 32767 sine.csv\n";
}

bool ParsePositiveFloat(const char* text, float& value)
{
    char* end = nullptr;
    value = std::strtof(text, &end);
    return end != text && *end == '\0' && std::isfinite(value) && value > 0.0F;
}

bool ParseFloat(const char* text, float& value)
{
    char* end = nullptr;
    value = std::strtof(text, &end);
    return end != text && *end == '\0' && std::isfinite(value);
}

bool ParseDouble(const char* text, double& value)
{
    char* end = nullptr;
    value = std::strtod(text, &end);
    return end != text && *end == '\0' && std::isfinite(value);
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 7 || argc > 9)
    {
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }

    const GenerateFunction generate = GeneratorFor(argv[1]);
    WaveSignalGenerator generator;
    float cycles = 0.0F;
    float amplitude = 0.0F;
    if (generate == nullptr || !ParsePositiveFloat(argv[2], cycles)
        || !ParsePositiveFloat(argv[3], generator.frequency_hz)
        || !ParsePositiveFloat(argv[4], generator.sample_rate_hz) || !ParseFloat(argv[5], amplitude)
        || amplitude < 0.0F || amplitude > 32767.0F
        || (argc >= 8 && !ParseDouble(argv[7], generator.phase))
        || (argc >= 9 && !ParseFloat(argv[8], generator.duty_cycle)))
    {
        PrintUsage(argv[0]);
        return EXIT_FAILURE;
    }

    generator.amplitude = static_cast<uint16_t>(std::lround(amplitude));
    generator.phase -= std::floor(generator.phase);

    const double exact_sample_count =
        static_cast<double>(cycles) * generator.sample_rate_hz / generator.frequency_hz;
    if (!std::isfinite(exact_sample_count)
        || exact_sample_count > static_cast<double>(std::numeric_limits<size_t>::max()))
    {
        std::cerr << "Requested output is too large\n";
        return EXIT_FAILURE;
    }
    const size_t sample_count = static_cast<size_t>(std::ceil(exact_sample_count));

    std::ofstream output(argv[6]);
    if (!output)
    {
        std::cerr << "Could not open " << argv[6] << '\n';
        return EXIT_FAILURE;
    }

    output << "sample,time_seconds,phase_cycles,signed_value,uint16_value\n";
    output.precision(9);
    for (size_t i = 0; i < sample_count; ++i)
    {
        const double phase = generator.phase;
        uint16_t value = 0;
        generate(generator, &value, 1);
        const int32_t signed_value =
            value <= 32767 ? static_cast<int32_t>(value) : static_cast<int32_t>(value) - 65536;
        output << i << ',' << static_cast<double>(i) / generator.sample_rate_hz << ',' << phase
               << ',' << signed_value << ',' << value << '\n';
    }

    std::cout << "Wrote " << sample_count << " samples (" << cycles << " cycles) to " << argv[6]
              << '\n';
    return EXIT_SUCCESS;
}
