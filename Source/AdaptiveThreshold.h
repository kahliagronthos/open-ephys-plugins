#pragma once

#include <ProcessorHeaders.h>

/*
 * AdaptiveThreshold
 *
 * Monitors one incoming continuous channel and fires a TTL ON event
 * when the signal crosses above an adaptive threshold derived from a
 * real-time Exponential Moving Average (EMA) of the signal's absolute
 * deviation (a lightweight approximation of MAD).
 *
 * Threshold = multiplier × EMA_MAD
 *
 * A TTL OFF event is fired once the signal drops back below the
 * threshold (rising-edge / hysteresis behaviour).
 */

 class AdaptiveThreshold : public GenericProcessor
{
public:
    AdaptiveThreshold();

    ~AdaptiveThreshold() {}

    AudioProcessorEditor* createEditor() override;

    void registerParameters();

    void updateSettings() override;

    void process(AudioBuffer<float>& buffer) override;

    bool startAcquisition() override;

    void parameterValueChanged(Parameter* param) override;

private:
    // TTL output 
    EventChannel* ttlChannel = nullptr;

    // Detection state
    bool  aboveThreshold = false;   // true while signal is above threshold

    // EMA accumulators (reset on startAcquisition)
    double emaMean = 0.0;           // EMA of the raw signal
    double emaMad  = 0.0;           // EMA of |signal - emaMean|
    bool   emaInitialised = false;  // false until first sample seen

    // Parameters (written from UI thread via parameterValueChanged)
    int    monitoredChannel = 0;    // zero-indexed continuous channel to watch
    float  multiplier       = 4.0f; // threshold = multiplier × emaMad
    double alpha            = 0.01; // EMA smoothing factor (0 < α ≤ 1)
    int    outputLine       = 0;    // TTL line to fire on (0–7)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdaptiveThreshold)
};