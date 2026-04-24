#include "AdaptiveThreshold.h"
#include "AdaptiveThresholdEditor.h"

AdaptiveThreshold::AdaptiveThreshold()
    : GenericProcessor("Adaptive Threshold")
{
    // Register parameters here — must be done in the constructor in v0.6.5

    addIntParameter(Parameter::GLOBAL_SCOPE,
                    "channel",
                    "Continuous channel index to monitor (0-indexed)",
                    0, 0, 255);

    addFloatParameter(Parameter::GLOBAL_SCOPE,
                      "multiplier",
                      "Threshold = multiplier x EMA(MAD). Typical range 2-8.",
                      4.0f, 1.0f, 20.0f, 0.5f);

    addFloatParameter(Parameter::GLOBAL_SCOPE,
                      "alpha",
                      "EMA smoothing factor. Smaller = slower adaptation.",
                      0.01f, 0.001f, 0.5f, 0.001f);

    addCategoricalParameter(Parameter::STREAM_SCOPE,
                            "output_line",
                            "TTL line for threshold-crossing events",
                            {"0", "1", "2", "3", "4", "5", "6", "7"},
                            0);
}

AudioProcessorEditor* AdaptiveThreshold::createEditor()
{
    editor = std::make_unique<AdaptiveThresholdEditor>(this);
    return editor.get();
}

void AdaptiveThreshold::registerParameters()
{
    // Not used in v0.6.5 — parameters registered in constructor instead
}

// Creates a new TTL output channel every time the signal chain changes
void AdaptiveThreshold::updateSettings()
{
    if (dataStreams.size() == 0)
        return;

    EventChannel::Settings settings{
        EventChannel::Type::TTL,
        "Adaptive Threshold Output",
        "TTL events fired on adaptive threshold crossings",
        "adaptivethreshold.events",
        dataStreams[0]
    };

    ttlChannel = new EventChannel(settings);
    eventChannels.add(ttlChannel);
    eventChannels.getLast()->addProcessor(processorInfo.get()); 
}

// Resets all running state so each recording starts clean
bool AdaptiveThreshold::startAcquisition()
{
    aboveThreshold  = false;
    emaMean         = 0.0;
    emaMad          = 0.0;
    emaInitialised  = false;

    return true;
}

// parameterValueChanged - called on the UI thread whenever a parameter editor changes a value
void AdaptiveThreshold::parameterValueChanged(Parameter* param)
{
    if (param->getName().equalsIgnoreCase("channel"))
    {
        monitoredChannel = (int) param->getValue();
        // Reset EMA accumulators when the channel changes
        emaMean        = 0.0;
        emaMad         = 0.0;
        emaInitialised = false;
        aboveThreshold = false;
        LOGD("AdaptiveThreshold: monitoring channel ", monitoredChannel);
    }
    else if (param->getName().equalsIgnoreCase("multiplier"))
    {
        multiplier = (float) param->getValue();
        LOGD("AdaptiveThreshold: multiplier = ", multiplier);
    }
    else if (param->getName().equalsIgnoreCase("alpha"))
    {
        alpha = (double) param->getValue();
        LOGD("AdaptiveThreshold: alpha = ", alpha);
    }
    else if (param->getName().equalsIgnoreCase("output_line"))
    {
        outputLine = (int) param->getValue(); // categorical index == line number
        LOGD("AdaptiveThreshold: output line = ", outputLine);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Core DSP loop, runs on the audio thread
//
// Algorithm (per sample):
//   1.  EMA update for signal mean:
//           emaMean = α·x + (1−α)·emaMean
//   2.  EMA update for absolute deviation (≈ MAD):
//           emaMad  = α·|x − emaMean| + (1−α)·emaMad
//   3.  Threshold = multiplier × emaMad
//   4.  Rising-edge detection:
//           ON  event when x > threshold  AND !aboveThreshold
//           OFF event when x ≤ threshold  AND  aboveThreshold
// ─────────────────────────────────────────────────────────────────────────────
void AdaptiveThreshold::process(AudioBuffer<float>& buffer)
{
    if (ttlChannel == nullptr)
        return;

    for (auto stream : getDataStreams())
    {
        // Only process the first data stream
        if (stream != getDataStreams()[0])
            continue;

        const uint16 streamId        = stream->getStreamId();
        const int    totalSamples    = getNumSamplesInBlock(streamId);
        const uint64 startSample     = getFirstSampleNumberForBlock(streamId);

        // Bounds-check the requested channel against what this stream carries
        const int numChannels = buffer.getNumChannels();
        if (monitoredChannel >= numChannels)
            continue;

        const float* channelData = buffer.getReadPointer(monitoredChannel);

        for (int i = 0; i < totalSamples; i++)
        {
            const double x = static_cast<double>(channelData[i]);

            // Initialise EMA accumulators on the very first sample
            if (!emaInitialised)
            {
                emaMean        = x;
                emaMad         = 0.0;
                emaInitialised = true;
                continue;
            }

            // Update EMA mean
            emaMean = alpha * x + (1.0 - alpha) * emaMean;

            // Update EMA of absolute deviation
            const double deviation = std::abs(x - emaMean);
            emaMad = alpha * deviation + (1.0 - alpha) * emaMad;

            // Compute adaptive threshold 
            const double threshold = static_cast<double>(multiplier) * emaMad;

            // Rising-edge detection
            if (!aboveThreshold && x > threshold)
            {
                aboveThreshold = true;

                TTLEventPtr eventPtr = TTLEvent::createTTLEvent(
                    ttlChannel,
                    startSample + static_cast<uint64>(i),
                    static_cast<uint8>(outputLine),
                    true);   // ON

                addEvent(eventPtr, i);
                eventPtr.release();

                LOGD("AdaptiveThreshold: TTL ON  at sample ", startSample + i,
                     "  (x=", x, " thr=", threshold, ")");
            }
            else if (aboveThreshold && x <= threshold)
            {
                aboveThreshold = false;

                TTLEventPtr eventPtr = TTLEvent::createTTLEvent(
                    ttlChannel,
                    startSample + static_cast<uint64>(i),
                    static_cast<uint8>(outputLine),
                    false);  // OFF

                addEvent(eventPtr, i);
                eventPtr.release();

                LOGD("AdaptiveThreshold: TTL OFF at sample ", startSample + i);
            }
        }
    }
}