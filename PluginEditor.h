#pragma once
#include "PluginProcessor.h"

// Round call button: Accept (green), Decline (red), HangUp (red, small bar icon)
class CircleButton : public juce::Button
{
public:
    enum Kind { Accept, Decline, HangUp };

    explicit CircleButton (Kind k) : juce::Button (""), kind (k) {}

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto b = getLocalBounds().toFloat().reduced (2.0f);
        auto c = (kind == Accept) ? juce::Colour (0xff2fa84a) : juce::Colour (0xffe0352b);
        if (over) c = c.brighter (0.15f);
        if (down) c = c.darker (0.25f);

        g.setColour (c);
        g.fillEllipse (b);

        g.setColour (juce::Colours::white);
        const float cx = b.getCentreX(), cy = b.getCentreY(), r = b.getWidth() * 0.2f;

        if (kind == Accept)
        {
            juce::Path p;
            p.startNewSubPath (cx - r, cy);
            p.lineTo (cx - r * 0.3f, cy + r * 0.75f);
            p.lineTo (cx + r, cy - r * 0.75f);
            g.strokePath (p, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
        }
        else if (kind == Decline)
        {
            g.drawLine (cx - r, cy - r, cx + r, cy + r, 4.0f);
            g.drawLine (cx - r, cy + r, cx + r, cy - r, 4.0f);
        }
        else
        {
            g.drawLine (cx - r * 1.2f, cy, cx + r * 1.2f, cy, 5.0f);
        }
    }

private:
    Kind kind;
};

class JohnPorkCallEditor : public juce::AudioProcessorEditor,
                           private juce::Timer
{
public:
    explicit JohnPorkCallEditor (JohnPorkCallProcessor&);
    ~JohnPorkCallEditor() override { stopTimer(); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override { repaint(); }

    JohnPorkCallProcessor& proc;
    juce::Image background;

    CircleButton decline { CircleButton::Decline };
    CircleButton hangUp  { CircleButton::HangUp };
    CircleButton accept  { CircleButton::Accept };

    juce::Slider intensity;
    juce::ToggleButton ringtoneToggle { "Ringtone" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> intensityAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ringtoneAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JohnPorkCallEditor)
};
