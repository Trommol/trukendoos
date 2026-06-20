import argparse
import math
import wave
from pathlib import Path

import numpy as np


SAMPLE_RATE = 48_000
FFT_SIZE = 2048
HOP_SIZE = FFT_SIZE // 4


def db_to_gain(db):
    return 10.0 ** (db / 20.0)


def make_source(seconds):
    samples = int(SAMPLE_RATE * seconds)
    t = np.arange(samples, dtype=np.float64) / SAMPLE_RATE

    # A small pitch pattern that behaves like a producer auditioning bass notes.
    notes = np.array([43.65, 55.0, 65.41, 49.0], dtype=np.float64)
    note_index = np.floor(t * 2.0).astype(np.int64) % len(notes)
    fundamental = notes[note_index]
    phase = np.cumsum(fundamental / SAMPLE_RATE) * 2.0 * math.pi

    sine = np.sin(phase)
    pre_distorted = np.tanh(sine * 3.0) * 0.7
    fifth = np.sin(phase * 1.5) * 0.12
    movement = np.sin(2.0 * math.pi * 0.27 * t) * 0.08
    return (pre_distorted + fifth + movement).astype(np.float64)


def local_average(magnitudes, radius=5):
    padded = np.pad(magnitudes, (radius, radius), mode="edge")
    result = np.zeros_like(magnitudes)

    for offset in range(radius * 2 + 1):
        result += padded[offset:offset + len(magnitudes)]

    return result / float(radius * 2 + 1)


def protection_curve(frequencies, sub_guard_hz):
    width = max(20.0, sub_guard_hz * 0.5)
    protection = 1.0 - np.clip((frequencies - sub_guard_hz) / width, 0.0, 1.0)
    protection[frequencies <= sub_guard_hz] = 1.0
    return protection


def process_frame(spectrum, mode, drive, rot, lock, tilt, sub_guard_hz, frozen, frame_index):
    magnitudes = np.abs(spectrum) + 1.0e-12
    phases = np.angle(spectrum)
    envelope = local_average(magnitudes)
    frequencies = np.fft.rfftfreq(FFT_SIZE, 1.0 / SAMPLE_RATE)
    bin_norm = np.linspace(0.0, 1.0, len(magnitudes))
    tilt_gain = db_to_gain(tilt * (bin_norm - 0.35) * 18.0)
    drive_gain = 1.0 + drive * 14.0
    processed = magnitudes.copy()
    phase_offset = frame_index * (0.11 + rot * 0.31)

    if mode == "melt":
        excess = magnitudes - envelope
        shaped = np.tanh((excess * drive_gain * tilt_gain) / (envelope + 1.0e-5))
        processed = envelope + shaped * envelope * (0.45 + drive * 2.2)

    elif mode == "rust":
        instability = 1.0 + np.sin(phase_offset + np.arange(len(magnitudes)) * 0.137) * 0.35 * rot
        threshold = envelope * (1.05 - rot * 0.45)
        excess = np.maximum(0.0, magnitudes - threshold)
        processed = magnitudes + excess * drive_gain * tilt_gain * instability * 0.7

        fold_distance = 1 + int(rot * 18.0)
        for index, amount in enumerate(excess):
            if index <= 0 or index >= len(processed) - 1:
                continue
            target = index + (fold_distance if index % 2 == 0 else -fold_distance)
            target = int(np.clip(target, 1, len(processed) - 2))
            processed[target] += amount * rot * (1.0 - lock) * 0.55

    elif mode == "glass":
        shard = np.sin(np.arange(len(magnitudes)) * 0.71 + phase_offset * (3.0 + rot * 9.0))
        bright_bias = np.clip((bin_norm - 0.25) * 1.6, 0.0, 1.0)
        edge = np.maximum(0.0, shard) ** 4.0 * bright_bias
        processed = magnitudes * (1.0 + edge * drive_gain * (0.8 + rot * 3.0) * tilt_gain)
        phases = phases + edge * rot * (1.0 - lock) * math.pi

    elif mode == "meat":
        body_center = 190.0 + tilt * 110.0
        body_distance = np.abs(np.log2(np.maximum(20.0, frequencies) / body_center))
        body_weight = np.exp(-body_distance * body_distance * 2.8)
        compression = np.tanh((magnitudes * drive_gain) / (envelope + 1.0e-5))
        processed = magnitudes + compression * envelope * body_weight * (0.7 + rot * 2.4)

        for index, amount in enumerate(magnitudes * body_weight * rot * 0.18):
            target = max(1, min(len(processed) - 2, index // 2))
            processed[target] += amount

    elif mode == "void":
        hole_pattern = np.sin(phase_offset * 1.7 + np.arange(len(magnitudes)) * (0.043 + rot * 0.09))
        holes = hole_pattern > (0.72 - rot * 0.55)
        freeze_blend = np.clip(rot * (1.0 - lock), 0.0, 0.85)
        processed = (magnitudes * (1.0 - freeze_blend)) + (frozen * freeze_blend)
        processed *= 1.0 - holes * rot * (1.0 - lock) * 0.95

    locked = envelope + (processed - envelope) * (1.0 - lock * 0.85)
    protected = protection_curve(frequencies, sub_guard_hz)
    processed = locked * (1.0 - protected) + magnitudes * protected

    return processed * np.exp(1j * phases)


def spectral_rot(audio, mode, drive, rot, lock, tilt, sub_guard_hz, mix):
    window = np.hanning(FFT_SIZE)
    padded = np.pad(audio, (FFT_SIZE, FFT_SIZE), mode="constant")
    output = np.zeros(len(padded), dtype=np.float64)
    norm = np.zeros(len(output), dtype=np.float64)
    frozen = np.zeros(FFT_SIZE // 2 + 1, dtype=np.float64)

    frame_index = 0
    for start in range(0, len(padded) - FFT_SIZE, HOP_SIZE):
        frame = padded[start:start + FFT_SIZE] * window
        spectrum = np.fft.rfft(frame)

        if mode == "void" and frame_index % max(3, int(20 - rot * 15)) == 0:
            frozen = np.abs(spectrum)

        processed = process_frame(spectrum, mode, drive, rot, lock, tilt, sub_guard_hz, frozen, frame_index)
        rendered = np.fft.irfft(processed, FFT_SIZE) * window
        output[start:start + FFT_SIZE] += rendered
        norm[start:start + FFT_SIZE] += window * window
        frame_index += 1

    output = output[FFT_SIZE:FFT_SIZE + len(audio)]
    norm = norm[FFT_SIZE:FFT_SIZE + len(audio)]
    output = np.divide(output, np.maximum(norm, 1.0e-4))
    return (audio * (1.0 - mix)) + (output * mix)


def write_wav(path, audio):
    path.parent.mkdir(parents=True, exist_ok=True)
    audio = audio / max(1.0, np.max(np.abs(audio)) * 1.05)
    pcm = np.round(audio * 32767.0).astype(np.int16)

    with wave.open(str(path), "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(SAMPLE_RATE)
        handle.writeframes(pcm.tobytes())


def main():
    parser = argparse.ArgumentParser(description="Render Spectral Rot audition WAVs.")
    parser.add_argument("--mode", choices=["melt", "rust", "glass", "meat", "void", "all"], default="all")
    parser.add_argument("--seconds", type=float, default=5.0)
    parser.add_argument("--drive", type=float, default=0.62)
    parser.add_argument("--rot", type=float, default=0.58)
    parser.add_argument("--lock", type=float, default=0.45)
    parser.add_argument("--tilt", type=float, default=0.12)
    parser.add_argument("--sub-guard", type=float, default=90.0)
    parser.add_argument("--mix", type=float, default=1.0)
    parser.add_argument("--out-dir", type=Path, default=Path("renders"))
    args = parser.parse_args()

    source = make_source(args.seconds)
    write_wav(args.out_dir / "source.wav", source)

    modes = ["melt", "rust", "glass", "meat", "void"] if args.mode == "all" else [args.mode]

    for mode in modes:
        rendered = spectral_rot(source, mode, args.drive, args.rot, args.lock, args.tilt, args.sub_guard, args.mix)
        write_wav(args.out_dir / f"spectral_rot_{mode}.wav", rendered)
        print(f"rendered {mode}")


if __name__ == "__main__":
    main()
