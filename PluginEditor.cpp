#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    // Layout (window is 340 x 590)
    const juce::Rectangle<float> screenArea { 14.0f, 14.0f, 312.0f, 362.0f };
    const juce::Rectangle<int> statusBar { 14, 392, 312, 34 };
}

JohnPorkCallEditor::JohnPorkCallEditor (JohnPorkCallProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    background = juce::ImageCache::getFromMemory (BinaryData::background_png,
                                                  BinaryData::background_pngSize);

    addAndMakeVisible (decline);
    addAndMakeVisible (hangUp);
    addAndMakeVisible (accept);

    decline.onClick = [this] { proc.decline(); };
    accept.onClick  = [this] { proc.answer(); };
    hangUp.onClick  = [this] { proc.resetCall(); };

    intensity.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    intensity.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    intensity.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xfff3a3b6));
    intensity.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0x55ffffff));
    intensity.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible (intensity);
    intensityAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        proc.apvts, "grit", intensity);

    ringtoneToggle.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    ringtoneToggle.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xff2fa84a));
    ringtoneToggle.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colours::grey);
    addAndMakeVisible (ringtoneToggle);
    ringtoneAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.apvts, "ringtone", ringtoneToggle);

    setSize (340, 590);
    startTimerHz (30);
}

void JohnPorkCallEditor::resized()
{
    decline.setBounds (38, 288, 64, 64);
    hangUp.setBounds  (138, 288, 64, 64);
    accept.setBounds  (238, 288, 64, 64);

    intensity.setBounds (40, 446, 92, 92);
    ringtoneToggle.setBounds (190, 480, 130, 30);
}

void JohnPorkCallEditor::paint (juce::Graphics& g)
{
    const bool ringing = proc.ringing.load();
    const bool declined = ! ringing && (int) proc.apvts.getRawParameterValue ("mode")->load() == 1;

    // Window background
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff7a5a86), 0.0f, 0.0f,
                                             juce::Colour (0xff3b2a47), 0.0f, (float) getHeight(), false));
    g.fillAll();

    // Phone screen with the picture
    {
        juce::Graphics::ScopedSaveState save (g);
        juce::Path clip;
        clip.addRoundedRectangle (screenArea, 26.0f);
        g.reduceClipRegion (clip);

        if (background.isValid())
            g.drawImage (background,
                         (int) screenArea.getX(), (int) screenArea.getY(),
                         (int) screenArea.getWidth(), (int) screenArea.getHeight(),
                         0, 0, background.getWidth(), background.getHeight());
        else
            g.fillAll();
    }
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawRoundedRectangle (screenArea, 26.0f, 2.0f);

    // Button labels, over the picture
    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    auto label = [&g] (const juce::String& text, int centreX)
    {
        juce::Rectangle<int> r (centreX - 40, 354, 80, 18);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawText (text, r.translated (1, 1), juce::Justification::centred);
        g.setColour (juce::Colours::white);
        g.drawText (text, r, juce::Justification::centred);
    };
    label ("Decline", 70);
    label ("Hang up", 170);
    label ("Accept", 270);

    // Status bar
    const auto bar = statusBar.toFloat();
    g.setColour (juce::Colour (0xff141018));
    g.fillRoundedRectangle (bar, 6.0f);
    g.setColour (juce::Colour (0xff3b3344));
    g.drawRoundedRectangle (bar, 6.0f, 1.5f);

    auto dot = juce::Colour (0xff35e06a);
    juce::String text = "ON THE LINE";
    if (ringing)       { dot = juce::Colour (0xffffb020); text = "RINGING..."; }
    else if (declined) { dot = juce::Colour (0xffe0352b); text = "CALL DECLINED"; }
    else
    {
        const double rate = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 44100.0;
        const int secs = (int) ((double) proc.callSamples.load() / rate);
        text << "  " << juce::String::formatted ("%02d:%02d", secs / 60, secs % 60);
    }

    g.setColour (dot);
    g.fillEllipse (bar.getX() + 12.0f, bar.getCentreY() - 6.0f, 12.0f, 12.0f);

    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    g.drawText (text, statusBar.withTrimmedLeft (32).withWidth (150),
                juce::Justification::centredLeft);

    // Level meter
    const auto meter = juce::Rectangle<float> (bar.getRight() - 118.0f, bar.getCentreY() - 4.0f, 106.0f, 8.0f);
    g.setColour (juce::Colour (0xff2a2330));
    g.fillRoundedRectangle (meter, 4.0f);
    g.setColour (dot);
    g.fillRoundedRectangle (meter.withWidth (meter.getWidth() * juce::jlimit (0.0f, 1.0f, proc.level.load())), 4.0f);

    // Knob labels
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.drawText ("INTENSITY", juce::Rectangle<int> (26, 540, 120, 16), juce::Justification::centred);
    g.drawText (juce::String ((int) std::round (intensity.getValue() * 100.0)),
                juce::Rectangle<int> (26, 556, 120, 16), juce::Justification::centred);
}
