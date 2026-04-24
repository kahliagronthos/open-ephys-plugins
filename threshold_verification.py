import numpy as np
import os

# CONFIGURATION — update path to point to your recording data directory
BASE = os.path.expanduser(
    "~/Documents/OERecordings/2026-04-23_15-42-18"
    "/Record Node 110/experiment1/recording1"
)

CONTINUOUS_DAT   = os.path.join(BASE, "continuous/File_Reader-100.example_data/continuous.dat")
SAMPLE_NUMBERS   = os.path.join(BASE, "continuous/File_Reader-100.example_data/sample_numbers.npy")
TTL_SAMPLES      = os.path.join(BASE, "events/Adaptive_Threshold-108.example_data/TTL/sample_numbers.npy")
TTL_STATES       = os.path.join(BASE, "events/Adaptive_Threshold-108.example_data/TTL/states.npy")

# Parameters used during the recording (must match your knob settings) 
MONITORED_CHANNEL = 0       # which channel index was being monitored
MULTIPLIER        = 4.0     # threshold multiplier knob value
ALPHA             = 0.01    # EMA alpha knob value
NUM_CHANNELS      = 16      # number of channels in the recording

# LOAD DATA
print("Loading continuous data...")
raw = np.memmap(CONTINUOUS_DAT, dtype='int16', mode='r')
num_samples = len(raw) // NUM_CHANNELS
signal_all = raw.reshape((num_samples, NUM_CHANNELS))
signal = signal_all[:, MONITORED_CHANNEL].astype(np.float32)
sample_numbers = np.load(SAMPLE_NUMBERS)

print(f"  {num_samples} samples loaded across {NUM_CHANNELS} channels")
print(f"  Monitoring channel {MONITORED_CHANNEL}")
print(f"  Sample number range: {sample_numbers[0]} – {sample_numbers[-1]}")

print("\nLoading TTL events...")
ttl_samples = np.load(TTL_SAMPLES)
ttl_states  = np.load(TTL_STATES)

on_events  = ttl_samples[ttl_states >  0]
off_events = ttl_samples[ttl_states <= 0]

print(f"  Total events: {len(ttl_samples)}")
print(f"  ON  events:   {len(on_events)}")
print(f"  OFF events:   {len(off_events)}")

# RERUN THE EMA ALGORITHM IN PYTHON
# This is an independent reimplementation — if it agrees with the plugin
# output, the plugin is functioning correctly.
print("\nRerunning EMA algorithm in Python...")

ema_mean = 0.0
ema_mad  = 0.0
initialised = False
above = False

expected_on  = []
expected_off = []

for i, x in enumerate(signal):
    x = float(x)

    if not initialised:
        ema_mean    = x
        ema_mad     = 0.0
        initialised = True
        continue

    ema_mean = ALPHA * x + (1.0 - ALPHA) * ema_mean
    ema_mad  = ALPHA * abs(x - ema_mean) + (1.0 - ALPHA) * ema_mad
    threshold = MULTIPLIER * ema_mad

    if not above and x > threshold:
        above = True
        expected_on.append(sample_numbers[i])
    elif above and x <= threshold:
        above = False
        expected_off.append(sample_numbers[i])

expected_on  = np.array(expected_on)
expected_off = np.array(expected_off)

print(f"  Expected ON  events: {len(expected_on)}")
print(f"  Expected OFF events: {len(expected_off)}")

# COMPARE PLUGIN OUTPUT vs PYTHON REIMPLEMENTATION
print("\n── VERIFICATION RESULTS ──────────────────────────────────────────────")

# Check ON event count
on_count_match = len(on_events) == len(expected_on)
print(f"\n[{'PASS' if on_count_match else 'FAIL'}] ON event count: "
      f"plugin={len(on_events)}, expected={len(expected_on)}")

# Check OFF event count
off_count_match = len(off_events) == len(expected_off)
print(f"[{'PASS' if off_count_match else 'FAIL'}] OFF event count: "
      f"plugin={len(off_events)}, expected={len(expected_off)}")

# Check timing of ON events (allow ±1 sample tolerance for buffer boundary effects)
TOLERANCE = 1  # samples

if on_count_match and len(on_events) > 0:
    on_diffs = np.abs(np.sort(on_events) - np.sort(expected_on))
    max_on_diff = np.max(on_diffs)
    mean_on_diff = np.mean(on_diffs)
    on_timing_pass = max_on_diff <= TOLERANCE
    print(f"[{'PASS' if on_timing_pass else 'FAIL'}] ON event timing: "
          f"max error={max_on_diff} samples, mean error={mean_on_diff:.2f} samples")
else:
    print("[SKIP] ON event timing: count mismatch, skipping timing check")

# Check timing of OFF events
if off_count_match and len(off_events) > 0:
    off_diffs = np.abs(np.sort(off_events) - np.sort(expected_off))
    max_off_diff = np.max(off_diffs)
    mean_off_diff = np.mean(off_diffs)
    off_timing_pass = max_off_diff <= TOLERANCE
    print(f"[{'PASS' if off_timing_pass else 'FAIL'}] OFF event timing: "
          f"max error={max_off_diff} samples, mean error={mean_off_diff:.2f} samples")
else:
    print("[SKIP] OFF event timing: count mismatch, skipping timing check")

# Sanity checks independent of reimplementation
print(f"\n── SANITY CHECKS ─────────────────────────────────────────────────────")

# ON and OFF counts should be equal or differ by at most 1
balance_ok = abs(len(on_events) - len(off_events)) <= 1
print(f"[{'PASS' if balance_ok else 'FAIL'}] ON/OFF balance: "
      f"ON={len(on_events)}, OFF={len(off_events)}")

# No two consecutive ON events without an OFF in between
if len(ttl_samples) > 1:
    consecutive_same = np.any(np.diff(ttl_states) == 0)
    print(f"[{'FAIL' if consecutive_same else 'PASS'}] No consecutive same-state events")
else:
    print("[SKIP] Not enough events to check consecutiveness")

# Events should be within the recorded sample range
if len(ttl_samples) > 0:
    in_range = (ttl_samples[0] >= sample_numbers[0]) and \
               (ttl_samples[-1] <= sample_numbers[-1])
    print(f"[{'PASS' if in_range else 'FAIL'}] All events within recorded sample range")

print(f"\n── SUMMARY ───────────────────────────────────────────────────────────")
print(f"  First ON event at sample: {on_events[0] if len(on_events) > 0 else 'N/A'}")
print(f"  Last  ON event at sample: {on_events[-1] if len(on_events) > 0 else 'N/A'}")
print(f"  Recording start sample:   {sample_numbers[0]}")
print(f"  Recording end sample:     {sample_numbers[-1]}")