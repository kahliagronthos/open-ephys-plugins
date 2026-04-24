#include "AdaptiveThresholdEditor.h"

AdaptiveThresholdEditor::AdaptiveThresholdEditor(GenericProcessor* parentNode)
    : GenericEditor(parentNode)
{
    // Three sliders (80x65px each) side by side, combobox (80x42px) below
    
    desiredWidth = 310;

    // Row 1 - three sliders side by side
    addSliderParameterEditor("channel",    15,  25);
    addSliderParameterEditor("multiplier", 105, 25);
    addSliderParameterEditor("alpha",      195, 25);

    // Row 2 - output line combobox, vertically centred in remaining space
    addComboBoxParameterEditor("output_line", 15, 100);
}