#!/usr/bin/env python3
import argparse
import logging
from pathlib import Path
import numpy as np
import librosa
from scipy.signal import iirnotch, lfilter, welch

# Setup professional logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    datefmt='%H:%M:%S'
)
logger = logging.getLogger(__name__)

EPSILON = 1e-12


def amplitude_to_db(value):
    return 20 * np.log10(max(float(value), EPSILON))


def power_to_db(value):
    return 10 * np.log10(max(float(value), EPSILON))


def band_power(freqs, psd, low_hz, high_hz):
    band = (freqs >= low_hz) & (freqs <= high_hz)
    if not np.any(band):
        return 0.0
    bin_width = float(np.median(np.diff(freqs))) if len(freqs) > 1 else 1.0
    return float(np.sum(psd[band]) * bin_width)


def detect_clipping(audio, threshold=0.999, min_consecutive=3):
    """Return clipping stats for samples pinned near the digital rails."""
    near_rail = np.abs(audio) >= threshold
    clipped_samples = int(np.count_nonzero(near_rail))
    run_length = 0
    max_run = 0

    for sample_is_near_rail in near_rail:
        if sample_is_near_rail:
            run_length += 1
            max_run = max(max_run, run_length)
        else:
            run_length = 0

    return {
        "is_clipped": max_run >= min_consecutive,
        "sample_count": clipped_samples,
        "sample_percent": 100 * clipped_samples / max(len(audio), 1),
        "max_consecutive": max_run,
        "threshold": threshold,
    }


def find_fundamental(y, sr):
    windowed = y * np.hanning(len(y))
    fft = np.abs(np.fft.rfft(windowed))
    freqs = np.fft.rfftfreq(len(y), 1 / sr)

    if len(fft) > 1:
        fft[0] = 0
    peak_index = int(np.argmax(fft))
    return float(freqs[peak_index]), fft, freqs


def calculate_thd(fundamental_hz, fft, freqs, max_harmonic=5):
    if fundamental_hz <= 0:
        return None

    fundamental_index = int(np.argmin(np.abs(freqs - fundamental_hz)))
    fundamental_amp = float(fft[fundamental_index])
    if fundamental_amp <= EPSILON:
        return None

    harmonic_amps = []
    nyquist = freqs[-1]
    for harmonic in range(2, max_harmonic + 1):
        harmonic_freq = fundamental_hz * harmonic
        if harmonic_freq >= nyquist:
            break
        harmonic_index = int(np.argmin(np.abs(freqs - harmonic_freq)))
        harmonic_amps.append(float(fft[harmonic_index]))

    if not harmonic_amps:
        return None

    thd_ratio = np.sqrt(np.sum(np.square(harmonic_amps))) / fundamental_amp
    return float(100 * thd_ratio)


def analyze_hum(freqs, psd, fundamental_hz=None):
    noise_floor = float(np.median(psd[(freqs >= 20) & (freqs <= min(5000, freqs[-1]))]))
    hum_results = []

    for base_freq in (50, 60):
        lines = []
        for harmonic in range(1, 7):
            center = base_freq * harmonic
            if center >= freqs[-1]:
                break
            if fundamental_hz and abs(center - fundamental_hz) < 5:
                continue

            low_hz = max(0, center - 1.5)
            high_hz = center + 1.5
            line_power = band_power(freqs, psd, low_hz, high_hz)
            expected_floor = noise_floor * max(high_hz - low_hz, np.median(np.diff(freqs)))
            relative_db = power_to_db(line_power / max(expected_floor, EPSILON))
            if relative_db >= 10:
                lines.append({
                    "frequency": center,
                    "relative_db": relative_db,
                })

        score = max([line["relative_db"] for line in lines], default=0.0)
        hum_results.append({
            "base_frequency": base_freq,
            "score_db": score,
            "lines": lines,
        })

    return max(hum_results, key=lambda item: item["score_db"])


def analyze_dropouts(y, sr):
    frame_length = max(int(0.05 * sr), 1)
    hop_length = max(int(0.025 * sr), 1)
    rms = librosa.feature.rms(
        y=y,
        frame_length=frame_length,
        hop_length=hop_length,
    )[0]
    median_rms = float(np.median(rms))
    threshold = max(median_rms * 0.05, 1e-5)
    quiet_frames = rms <= threshold

    longest_run = 0
    current_run = 0
    for is_quiet in quiet_frames:
        if is_quiet:
            current_run += 1
            longest_run = max(longest_run, current_run)
        else:
            current_run = 0

    return {
        "quiet_percent": 100 * int(np.count_nonzero(quiet_frames)) / max(len(rms), 1),
        "longest_ms": 1000 * longest_run * hop_length / sr,
    }


def collect_measurements(sr, y, fundamental_hz=None, fft=None, fft_freqs=None):
    """Collect forensic measurements for alerts and CLI reporting."""
    audio = np.asarray(y, dtype=np.float32)
    rms = float(np.sqrt(np.mean(audio ** 2)))
    peak = float(np.max(np.abs(audio)))
    dc_offset = float(np.mean(audio))
    clipping = detect_clipping(audio)
    crest_factor_db = amplitude_to_db(peak / max(rms, EPSILON))

    if fundamental_hz is None or fft is None or fft_freqs is None:
        fundamental_hz, fft, fft_freqs = find_fundamental(audio, sr)
    thd_percent = calculate_thd(fundamental_hz, fft, fft_freqs)

    # Harmonic = tone/hum, percussive = pops, clicks, and other transient noise.
    _harmonic, percussive = librosa.effects.hpss(y)
    percussive_rms = float(np.sqrt(np.mean(percussive ** 2)))
    percussive_ratio = percussive_rms / max(rms, EPSILON)
    percussive_peak_ratio = float(np.max(np.abs(percussive))) / max(peak, EPSILON)

    nperseg = min(8192, len(audio))
    freqs, psd = welch(audio, fs=sr, nperseg=nperseg)
    total_power = band_power(freqs, psd, 0, freqs[-1])
    rumble_power = band_power(freqs, psd, 0, 30)
    hiss_low = min(10000, freqs[-1] * 0.65)
    hiss_power = band_power(freqs, psd, hiss_low, freqs[-1])
    hum = analyze_hum(freqs, psd, fundamental_hz)
    dropouts = analyze_dropouts(y, sr)

    onset_strength = librosa.onset.onset_strength(y=y, sr=sr)
    if len(onset_strength):
        onset_median = float(np.median(onset_strength))
        onset_mad = float(np.median(np.abs(onset_strength - onset_median)))
        transient_spikes = int(np.count_nonzero(onset_strength > onset_median + 8 * max(onset_mad, EPSILON)))
    else:
        transient_spikes = 0

    return {
        "duration": len(audio) / sr,
        "sample_rate": sr,
        "fundamental_hz": fundamental_hz,
        "thd_percent": thd_percent,
        "rms": rms,
        "peak": peak,
        "dc_offset": dc_offset,
        "peak_dbfs": amplitude_to_db(peak),
        "rms_dbfs": amplitude_to_db(rms),
        "headroom_db": -amplitude_to_db(peak),
        "crest_factor_db": crest_factor_db,
        "clipping": clipping,
        "hum": hum,
        "rumble_percent": 100 * rumble_power / max(total_power, EPSILON),
        "hiss_percent": 100 * hiss_power / max(total_power, EPSILON),
        "percussive_ratio": percussive_ratio,
        "percussive_peak_ratio": percussive_peak_ratio,
        "transient_spikes": transient_spikes,
        "dropouts": dropouts,
    }


def run_forensics(sr, y):
    """Identify likely hardware, capture-chain, and signal-quality issues."""
    logger.info("Starting forensic analysis...")

    measurements = collect_measurements(sr, y)
    alerts = []

    if measurements["rms"] <= EPSILON:
        return ["SILENCE: File contains no measurable signal."]

    if abs(measurements["dc_offset"]) > 0.005:
        alerts.append(
            f"DC OFFSET: {measurements['dc_offset']:.5f}. "
            "Check for missing DC-blocking caps or bias issues."
        )

    clipping = measurements["clipping"]
    if clipping["is_clipped"]:
        alerts.append(
            "CLIPPING: Signal is hitting digital rails "
            f"({clipping['sample_count']} samples, max run {clipping['max_consecutive']})."
        )
    elif measurements["peak"] >= 0.98:
        alerts.append(f"LOW HEADROOM: Peak level is {measurements['peak_dbfs']:.2f} dBFS.")

    if measurements["rms_dbfs"] < -45:
        alerts.append(f"LOW LEVEL: RMS is {measurements['rms_dbfs']:.1f} dBFS. Check gain staging.")

    if measurements["crest_factor_db"] > 22:
        alerts.append(
            f"IMPULSIVE PEAKS: Crest factor is {measurements['crest_factor_db']:.1f}dB. "
            "Check for clicks or pops."
        )
    elif measurements["crest_factor_db"] < 3:
        alerts.append(
            f"LOW DYNAMIC RANGE: Crest factor is {measurements['crest_factor_db']:.1f}dB. "
            "Check compression or limiting."
        )

    thd_percent = measurements["thd_percent"]
    if thd_percent is not None and thd_percent > 2.0:
        alerts.append(
            f"HARMONIC DISTORTION: THD estimate is {thd_percent:.2f}% "
            f"around {measurements['fundamental_hz']:.1f}Hz."
        )

    hum = measurements["hum"]
    if hum["score_db"] >= 18:
        alerts.append(
            f"MAINS HUM: {hum['base_frequency']}Hz family is {hum['score_db']:.1f}dB above the local floor."
        )
    if measurements["rumble_percent"] > 15:
        alerts.append("LOW-FREQUENCY RUMBLE: Excess energy below 30Hz. Check handling noise, wind, or mechanical coupling.")
    if measurements["hiss_percent"] > 20:
        alerts.append("HIGH-FREQUENCY HISS: Broadband energy is elevated near the top of the spectrum.")
    if (
        measurements["percussive_ratio"] > 0.35
        or measurements["transient_spikes"] >= 5
        or measurements["percussive_peak_ratio"] > 0.80
    ):
        alerts.append(
            "TRANSIENT NOISE: Percussive component suggests pops, clicks, or intermittent disturbances "
            f"({100 * measurements['percussive_ratio']:.1f}% RMS share, "
            f"{measurements['transient_spikes']} spike frames)."
        )

    dropouts = measurements["dropouts"]
    if dropouts["longest_ms"] >= 100:
        alerts.append(f"DROPOUTS: Quiet stretch detected for {dropouts['longest_ms']:.0f}ms.")
    elif dropouts["quiet_percent"] >= 10:
        alerts.append(f"INTERMITTENT LOW LEVEL: {dropouts['quiet_percent']:.1f}% of short windows are near silence.")

    return alerts


def print_measurements(measurements):
    clipping = measurements["clipping"]
    hum = measurements["hum"]
    dropouts = measurements["dropouts"]

    print("-" * 50)
    print("MEASUREMENTS")
    print(f"Duration:       {measurements['duration']:.2f}s")
    print(f"Sample rate:    {measurements['sample_rate']} Hz")
    print(f"Fundamental:    {measurements['fundamental_hz']:.2f} Hz")
    if measurements["thd_percent"] is not None:
        print(f"THD estimate:   {measurements['thd_percent']:.3f}%")
    print(f"DC offset:      {measurements['dc_offset']:.6f}")
    print(f"Peak level:     {measurements['peak_dbfs']:.2f} dBFS")
    print(f"RMS level:      {measurements['rms_dbfs']:.2f} dBFS")
    print(f"Headroom:       {measurements['headroom_db']:.2f} dB")
    print(f"Crest factor:   {measurements['crest_factor_db']:.2f} dB")
    print(
        f"Clipping:       {clipping['sample_count']} samples "
        f"({clipping['sample_percent']:.4f}%), max run {clipping['max_consecutive']}"
    )
    print(f"Hum candidate:  {hum['base_frequency']}Hz family, {hum['score_db']:.1f}dB over floor")
    print(f"Rumble share:   {measurements['rumble_percent']:.2f}% below 30Hz")
    print(f"Hiss share:     {measurements['hiss_percent']:.2f}% in upper band")
    print(f"Percussive mix: {100 * measurements['percussive_ratio']:.2f}%")
    print(f"Transient hits: {measurements['transient_spikes']}")
    print(f"Quiet frames:   {dropouts['quiet_percent']:.2f}%, longest {dropouts['longest_ms']:.0f}ms")


def write_plots(plot_file, y, sr, times, snr_t, snr_db, noise_only):
    logging.getLogger("matplotlib").setLevel(logging.WARNING)
    import matplotlib.pyplot as plt

    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(11, 11), constrained_layout=True)

    ax1.plot(times, snr_t, label='Instantaneous SNR', color='#1f77b4')
    ax1.axhline(y=snr_db, color='r', linestyle='--', label=f'Avg: {snr_db:.2f}dB')
    ax1.set_title("Instantaneous SNR")
    ax1.set_ylabel("SNR (dB)")
    ax1.legend()

    f_psd, p_psd = welch(noise_only, fs=sr, nperseg=4096)
    ax2.semilogy(f_psd, p_psd, color='red')
    ax2.set_title("Noise Power Spectral Density")
    ax2.set_ylabel("Power")
    ax2.set_xlabel("Frequency (Hz)")

    _spectrogram, _freqs, _bins, image = ax3.specgram(
        y,
        NFFT=2048,
        Fs=sr,
        noverlap=1536,
        cmap='magma',
        scale='dB',
    )
    fig.colorbar(image, ax=ax3, label='Power (dB)')
    ax3.set_title("Spectrogram")
    ax3.set_ylabel("Frequency (Hz)")
    ax3.set_xlabel("Time (s)")

    if plot_file:
        output_path = Path(plot_file)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(output_path, dpi=150)
        logger.info(f"Wrote diagnostic plots to {output_path}")

    return plt


def analyze_audio(filepath, show_plot=False, plot_file=None):
    logger.info(f"Analyzing: {filepath}")

    y, sr = librosa.load(filepath, sr=None)
    actual_freq, fft, freqs = find_fundamental(y, sr)
    logger.info(f"Fundamental frequency detected at {actual_freq:.2f}Hz")

    if 0 < actual_freq < sr / 2:
        b, a = iirnotch(actual_freq, Q=30, fs=sr)
        noise_only = lfilter(b, a, y)
    else:
        noise_only = y

    rms_sig = np.sqrt(np.mean(y**2))
    rms_noise = np.sqrt(np.mean(noise_only**2))
    snr_db = amplitude_to_db(rms_sig / max(rms_noise, EPSILON))

    hop = int(0.05 * sr)
    rms_t_sig = librosa.feature.rms(y=y, hop_length=hop)[0]
    rms_t_noise = librosa.feature.rms(y=noise_only, hop_length=hop)[0]
    snr_t = 20 * np.log10(np.maximum(rms_t_sig, 1e-10) / np.maximum(rms_t_noise, 1e-10))
    times = librosa.frames_to_time(np.arange(len(snr_t)), sr=sr, hop_length=hop)

    measurements = collect_measurements(sr, y, actual_freq, fft, freqs)
    alerts = run_forensics(sr, y)

    print("\n" + "=" * 50)
    print(f"AUDIO PATH REPORT: {filepath}")
    print(f"SNR: {snr_db:.2f} dB")
    print("=" * 50)
    if alerts:
        for alert in alerts:
            print(f" [!] {alert}")
    else:
        print(" [+] No high-confidence electrical or capture-chain artifacts detected.")
    print_measurements(measurements)
    print("=" * 50 + "\n")

    if show_plot or plot_file:
        plt = write_plots(plot_file, y, sr, times, snr_t, snr_db, noise_only)
        if show_plot:
            plt.show()
        else:
            plt.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("file", help="Input WAV file")
    parser.add_argument("-p", "--plot", action="store_true", help="Show diagnostic plots")
    parser.add_argument("-f", "--plot-file", help="Write diagnostic plots to this image file")
    args = parser.parse_args()
    analyze_audio(args.file, args.plot, args.plot_file)
