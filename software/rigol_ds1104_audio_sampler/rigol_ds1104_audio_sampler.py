import enum
import socket
import wave
import numpy as np
import time
import math
import matplotlib.pyplot as plt
from scipy.signal import resample_poly, butter, sosfiltfilt
import ipaddress
import socket
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
import argparse
import re

COMMON_PORTS = (5555, 5025)
TIMEOUT = 0.01
MAX_WORKERS = 64

DEFAULT_SUBNET = "10.0.1.1/24"


def probe_scpi(ip: str, port: int, timeout: float = TIMEOUT) -> str | None:
    try:
        with socket.create_connection((ip, port), timeout=timeout) as sock:
            sock.settimeout(timeout)
            sock.sendall(b"*IDN?\n")
            response = sock.recv(4096).decode(errors="ignore").strip()
            return response or None
    except OSError:
        return None


def scan_host(ip: str) -> tuple[str, int, str] | None:
    for port in COMMON_PORTS:
        idn = probe_scpi(ip, port)
        if idn and "RIGOL" in idn.upper():
            return ip, port, idn
    return None


def scan_rigols(subnet: str):
    network = ipaddress.ip_network(subnet, strict=False)
    results = []

    with ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
        futures = {
            executor.submit(scan_host, str(ip)): str(ip) for ip in network.hosts()
        }

        idx = 1
        for future in as_completed(futures):
            result = future.result()
            if result:
                results.append(result)
                ip, port, idn = result
                print(f"[{idx}] - Found Rigol at {ip}:{port} -> {idn}")
                idx += 1

    return sorted(results)


def plot_samples(samples, time_axis=None):
    if time_axis is None:
        plt.plot(samples)
        plt.xlabel("Sample")
    else:
        plt.plot(time_axis, samples)
        plt.xlabel("Time (s)")

    plt.ylabel("Voltage (V)")
    plt.title("Captured Waveform")
    plt.grid(True)
    plt.tight_layout()
    plt.show()


class RigolScope:
    class Timescale(enum.Enum):
        ns_5 = 5 * 10**-9
        ns_10 = 10 * 10**-9
        ns_20 = 20 * 10**-9
        ns_50 = 50 * 10**-9
        ns_100 = 100 * 10**-9
        ns_200 = 200 * 10**-9
        ns_500 = 500 * 10**-9
        us_1 = 1 * 10**-6
        us_2 = 2 * 10**-6
        us_5 = 5 * 10**-6
        us_10 = 10 * 10**-6
        us_20 = 20 * 10**-6
        us_50 = 50 * 10**-6
        us_100 = 100 * 10**-6
        us_200 = 200 * 10**-6
        us_500 = 500 * 10**-6
        ms_1 = 1 * 10**-3
        ms_2 = 2 * 10**-3
        ms_5 = 5 * 10**-3
        ms_10 = 10 * 10**-3
        ms_20 = 20 * 10**-3
        ms_50 = 50 * 10**-3
        ms_100 = 100 * 10**-3
        ms_200 = 200 * 10**-3
        ms_500 = 500 * 10**-3
        s_1 = 1
        s_2 = 2
        s_5 = 5
        s_10 = 10
        s_20 = 20
        s_50 = 50

    def __init__(self, ip, port=5555, timeout=10):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(timeout)
        self.sock.connect((ip, port))

        self.channels = {}
        time.sleep(0.1)

    def send(self, cmd):
        self.sock.sendall((cmd + "\n").encode())
        time.sleep(0.05)  # small delay — scope needs it

    def query(self, cmd):
        self.send(cmd)
        return self._recv_until_newline()

    def _recv_until_newline(self):
        buf = b""
        while not buf.endswith(b"\n"):
            buf += self.sock.recv(4096)
        return buf.decode().strip()

    def read_raw_block(self):
        """Read an IEEE 488.2 binary block response (#NXXXXXX...)"""
        # Read until we get the '#' header
        header = b""
        while not header.endswith(b"#"):
            header += self.sock.recv(1)

        # Next byte tells us how many digits describe the length
        n_digits = int(self.sock.recv(1).decode())
        length = int(self.sock.recv(n_digits).decode())

        # Read exactly `length` bytes
        data = b""
        while len(data) < length:
            chunk = self.sock.recv(length - len(data))
            if not chunk:
                raise RuntimeError("Connection closed before data complete")
            data += chunk

        return np.frombuffer(data, dtype=np.uint8)

    def get_scope_stats(self):
        raw = self.query(":WAVeform:PREamble?")
        fields = raw.split(",")
        sample_rate = self.query(":ACQ:SRAT?")
        return {
            "format": int(fields[0]),  # 0=BYTE, 1=WORD, 2=ASC
            "type": int(fields[1]),  # 0=NORMal, 1=MAX, 2=RAW
            "points": int(fields[2]),
            "count": int(fields[3]),
            "x_inc": float(fields[4]),  # seconds per point
            "x_origin": float(fields[5]),
            "x_ref": float(fields[6]),
            "y_inc": float(fields[7]),  # volts per step
            "y_origin": float(fields[8]),
            "y_ref": float(fields[9]),
            "sample_rate": int(float(sample_rate)),
        }

    def get_channel_data(self, channel: int):
        ch = f"CHANnel{channel}"
        scale = float(self.query(f":{ch}:SCALe?"))
        units = self.query(f":{ch}:UNITs?")

        return {"scale": scale, "units": units}

    def run_single(self, time_scale: Timescale):
        self.send(f":TIM:SCAL {time_scale.value}")
        self.send(":SINGle")

        time.sleep(0.5)

        print(f"Scope single capture.", end="")
        while True:
            time.sleep(0.2)
            print(".", end="", flush=True)
            status = self.query(":TRIG:STAT?").strip()
            if status == "STOP":
                break

        self.send(":STOP")
        time.sleep(0.2)
        print()
        return self.get_scope_stats()

    def read_samples(self, preamble: dict, channel: int, start: int, sample_size: int):
        chunk_size = sample_size
        if chunk_size > 250_000:
            chunk_size = 250_000

        if start < 1:
            raise Exception("Start must be >=1")

        self.send(f":WAVeform:SOURce CHAN{channel}")
        self.send(":WAVeform:MODE RAW")
        self.send(":WAVeform:FORMat BYTE")

        data = np.array([], dtype=np.uint8)

        points_to_read = sample_size

        while points_to_read > 0:
            points = min(points_to_read, chunk_size)
            stop = (start - 1) + points

            self.send(f":WAVeform:STARt {start}")
            self.send(f":WAVeform:STOP {stop}")
            self.send(":WAVeform:DATA?")
            chunk = self.read_raw_block()
            data = np.append(data, chunk)

            start += points
            points_to_read -= points

            print(f"Reading {stop}/{preamble["points"]} points...", end="\r")

        print()
        volts = (
            data.astype(float) - preamble["y_ref"] - preamble["y_origin"]
        ) * preamble["y_inc"]
        time_axis = np.arange(len(volts)) * preamble["x_inc"] + preamble["x_origin"]

        return time_axis, volts

    def capture_as_wav(
        self,
        time_scale: Timescale,
        channels: list[int],
        down_sample_rate: int,
        output_prefix: str = "capture",
    ):
        # stats = self.run_single(time_scale)
        stats = self.get_scope_stats()

        print(f"Channels to capture {len(channels)}")
        for channel in channels:
            start = 1
            remaining_points = stats["points"]
            captured_chunks = []

            while remaining_points > 0:
                points = min(remaining_points, 250_000)
                _, volts = self.read_samples(stats, channel, start, points)
                captured_chunks.append(volts)

                start += points
                remaining_points -= points

            samples = (
                np.concatenate(captured_chunks) if captured_chunks else np.array([])
            )
            duration = (
                len(samples) / stats["sample_rate"] if stats["sample_rate"] else 0
            )
            pcm = self.convert_to_pcm(samples, stats["sample_rate"], down_sample_rate)
            output_file = f"{output_prefix}_ch{channel}.wav"
            self.write_wav(output_file, pcm, down_sample_rate)
            print(
                f"Wrote {len(pcm)} samples to {output_file} "
                f"({duration:.3f}s captured -> {len(pcm) / down_sample_rate:.3f}s wav)"
            )

    def convert_to_pcm(self, samples, scope_rate, sample_rate):
        if len(samples) == 0:
            return np.array([], dtype=np.int16)

        if scope_rate > sample_rate:
            samples = self.apply_lowpass_filter(samples, scope_rate)

        if sample_rate != scope_rate:
            g = math.gcd(scope_rate, sample_rate)
            up = sample_rate // g
            down = scope_rate // g

            samples = resample_poly(samples, up, down)

        samples = samples - np.mean(samples)

        peak = np.max(np.abs(samples))

        if peak == 0:
            return np.zeros(len(samples), dtype=np.int16)

        pcm = np.clip(samples / peak, -1.0, 1.0)
        pcm = (pcm * np.iinfo(np.int16).max).astype(np.int16)

        return pcm

    def write_wav(self, filename, pcm_data, sample_rate):
        with wave.open(filename, "wb") as wav_file:
            wav_file.setnchannels(1)
            wav_file.setsampwidth(2)
            wav_file.setframerate(sample_rate)
            wav_file.writeframes(pcm_data.tobytes())

    def apply_lowpass_filter(
        self, samples, sample_rate, cutoff_hz: float = 20_000, order: int = 6
    ):
        if len(samples) == 0:
            return samples
        nyquist = sample_rate / 2
        effective_cutoff = min(cutoff_hz, nyquist * 0.95)
        if effective_cutoff <= 0 or effective_cutoff >= nyquist:
            return samples
        sos = butter(order, effective_cutoff, btype="low", fs=sample_rate, output="sos")
        return sosfiltfilt(sos, samples)

    def close(self):
        self.sock.close()


def parse_timescale(value: str) -> RigolScope.Timescale:
    match = re.fullmatch(r"\s*(\d+)\s*(ns|us|ms|s)\s*", value.lower())
    if not match:
        raise argparse.ArgumentTypeError("Timescale must look like 200us, 100ms, or 1s")

    amount, unit = match.groups()
    enum_name = f"{unit}_{amount}"

    try:
        return RigolScope.Timescale[enum_name]
    except KeyError as exc:
        valid_values = ", ".join(
            member.name.replace("_", "", 1) for member in RigolScope.Timescale
        )
        raise argparse.ArgumentTypeError(
            f"Unsupported timescale '{value}'. Valid values: {valid_values}"
        ) from exc


def parse_channels(value: str) -> list[int]:
    channels = []

    for sub in value.split(","):
        channel_text = sub.strip()
        if not channel_text:
            raise argparse.ArgumentTypeError(
                "Channel must be a comma-separated list containing 1,2,3 or 4. Ex: 1 or 1,4, or 1,2,3"
            )

        try:
            channel = int(channel_text)
        except ValueError as ex:
            raise argparse.ArgumentTypeError(
                f"Invalid channel '{channel_text}'. Channels must be integers in range 1-4"
            ) from ex

        if channel not in (1, 2, 3, 4):
            raise argparse.ArgumentTypeError(
                f"Invalid channel '{channel}'. Channels must be in the range 1-4"
            )
        channels.append(channel)

    if not channels:
        raise argparse.ArgumentTypeError("At least one channel must be provided")

    return channels


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Capture Rigol scope data and write it as WAV files."
    )
    parser.add_argument(
        "--subnet",
        nargs="?",
        default=DEFAULT_SUBNET,
        help="Subnet to scan",
    )
    parser.add_argument(
        "--time_scale",
        nargs="?",
        default=RigolScope.Timescale.ms_100,
        type=parse_timescale,
        help="Scope timescale such as 200us, 100ms, or 1s.",
    )
    parser.add_argument(
        "--down_sample_rate",
        nargs="?",
        default=44_100,
        type=int,
        help="Sample rate to covnert to.",
    )
    parser.add_argument(
        "--channels",
        default="1",
        type=parse_channels,
        help="Comman-separated channels to capture.",
    )
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()

    scopes = scan_rigols(args.subnet)
    idx = 1
    while len(scopes) > 1:
        try:
            print(f"Select device from [1-{len(scopes)}]")
            idx = int(input("> "))

            if idx < 0 and idx > len(scopes):
                print(f"Error: Pick a nubmer between [1-{len(scopes)}]")

            break
        except:
            print("Error: Enter a value number")

    ip, port, idn = scopes[idx - 1]
    print(f"Selected scope [{idx}] - {idn}")

    scope = RigolScope(ip, port)

    ts = args.time_scale

    try:
        print("Connected:", scope.query("*IDN?"))
        scope.capture_as_wav(ts, args.channels, args.down_sample_rate)
    finally:
        scope.close()
