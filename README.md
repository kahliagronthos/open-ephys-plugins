# open-ephys-plugins

Custom plugins for the [Open Ephys GUI](https://open-ephys.org/gui) (v0.6.5, Plugin API v8), built on macOS for real-time electrophysiology signal processing.


## Plugins

### Adaptive Threshold
A filter plugin that monitors one continuous input channel and fires TTL events when the signal crosses an adaptive threshold derived from a real-time EMA approximation of the Median Absolute Deviation (MAD).

**Algorithm:**
- Maintains a running EMA of the signal mean and absolute deviation
- Threshold = `multiplier × EMA_MAD`
- Fires TTL ON on rising edge, TTL OFF when signal drops back below threshold

**UI Parameters:**

| Parameter | Default | Description |
|---|---|---|
| Channel | 0 | Zero-indexed continuous channel to monitor |
| Multiplier | 4.0 | Threshold sensitivity (higher = fewer events) |
| Alpha | 0.01 | EMA smoothing factor (smaller = slower adaptation) |
| Output Line | 0 | TTL line to fire on (0–7) |

**Verification:**

Each plugin includes a Python verification script (threshold_verification.py) that independently reimplements the algorithm and compares its output against a recorded session sample-by-sample.
