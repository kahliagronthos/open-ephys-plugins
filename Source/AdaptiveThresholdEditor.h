#pragma once

#include <EditorHeaders.h>
#include "AdaptiveThreshold.h"

/*
 * AdaptiveThresholdEditor
 *
 * Provides four parameter editors in the plugin's tab:
 *   • Channel index         (BoundedValue, processor scope)
 *   • Threshold multiplier  (BoundedValue, processor scope)
 *   • EMA alpha             (BoundedValue, processor scope)
 *   • Output TTL line       (TtlLine,      stream scope)
 */

 class AdaptiveThresholdEditor : public GenericEditor
{
public:
    AdaptiveThresholdEditor(GenericProcessor* parentNode);

    ~AdaptiveThresholdEditor() {}

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdaptiveThresholdEditor)
};