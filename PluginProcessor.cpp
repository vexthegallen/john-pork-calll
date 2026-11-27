#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

JohnPorkCallProcessor::JohnPorkCallProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    loadEmbeddedRingtone();
}

juce::AudioProcessorValueTreeState::ParameterLayout JohnPorkCallProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "mode", 1 }, "Call", juce::StringArray { "Answer", "Decline" }, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "grit", 1 }, "Intensity",
        juce::NormalisableRange<float> (0.0f, 1.0f), 1.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "ringtone", 1 }, "Ringtone", true));

    return layout;
}

void JohnPorkCallProcessor::loadEmbeddedRingtone()
{
    auto* stream = new juce::MemoryInputStream (BinaryData::ringtone_wav,
                                                (size_t) BinaryData::ringtone_wavSize,
                                                false);
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatReader> reader (wav.createReaderFor (stream, true));

    if (reader == nullptr || reader->lengthInSamples <= 0)
        return;

    const int len = (int) reader->lengthInSamples;
    customRing.setSize ((int) juce::jmin (2u, reader->numChannels), len);
    reader->read (&customRing, 0, len, 0, true, true);
    customRate = reader->sampleRate;
    hasCustomRing = true;
}

bool JohnPorkCallProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == layouts.getMainInputChannelSet();
}

void JohnPorkCallProcessor::setMode (int mode)
{
    if (auto* p = apvts.getParameter ("mode"))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) mode));
        p->endChangeGesture();
    }
}

void JohnPorkCallProcessor::answer()
{
    setMode (0);
    callSamples = 0;
    ringing = false;
}

void JohnPorkCallProcessor::decline()
{
    setMode (1);
    callSamples = 0;
    ringing = false;
}

void JohnPorkCallProcessor::resetCall()
{
    callSamples = 0;
    ringPosD = 0.0;
    ringClock = 0;
    ringing = true;
}

void JohnPorkCallProcessor::updateFilters (int mode)
{
    if (mode == 0)
    {
        // ANSWER: classic telephone band, roughly 400 Hz to 3.4 kHz
        *highPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 400.0f);
        *lowPass.state  = *juce::dsp::IIR::Coefficients<float>::makeLowPass  (sr, 3400.0f);
    }
    else
    {
        // DECLINE: muffled, like the line getting cut
        *highPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 300.0f);
        *lowPass.state  = *juce::dsp::IIR::Coefficients<float>::makeLowPass  (sr, 900.0f);
    }
    lastMode = mode;
}

void JohnPorkCallProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) juce::jmax (1, getTotalNumOutputChannels());

    highPass.prepare (spec);
    lowPass.prepare (spec);
    compressor.prepare (spec);
    compressor.setThreshold (-18.0f);
    compressor.setRatio (4.0f);
    compressor.setAttack (5.0f);
    compressor.setRelease (80.0f);

    const int mode = (int) apvts.getRawParameterValue ("mode")->load();
    updateFilters (mode);
    lastMode = mode;
}

void JohnPorkCallProcessor::addRingtone (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (hasCustomRing.load())
    {
        // Embedded ringtone, looped with a short pause between plays
        const int len = customRing.getNumSamples();
        const double period = (double) len + customRate * 0.8;
        const double step = customRate / sr;
        const int srcChannels = customRing.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            const int idx = (int) ringPosD;
            if (idx < len)
            {
                for (int ch = 0; ch < numChannels; ++ch)
                    buffer.addSample (ch, i, customRing.getSample (juce::jmin (ch, srcChannels - 1), idx));
            }
            ringPosD += step;
            if (ringPosD >= period)
                ringPosD -= period;
        }
        return;
    }

    // Fallback: plain two-tone phone ring
    const int onSamples = (int) (sr * 0.9);
    const int period = (int) (sr * 2.4);
    const int fade = (int) (sr * 0.01);

    for (int i = 0; i < numSamples; ++i)
    {
        if (ringClock < onSamples)
        {
            const float env = juce::jmin (1.0f,
                                          (float) ringClock / (float) fade,
                                          (float) (onSamples - ringClock) / (float) fade);
            const float s = 0.2f * env * (std::sin ((float) phase1) + std::sin ((float) phase2));
            for (int ch = 0; ch < numChannels; ++ch)
                buffer.addSample (ch, i, s);

            phase1 += juce::MathConstants<double>::twoPi * 440.0 / sr;
            phase2 += juce::MathConstants<double>::twoPi * 480.0 / sr;
        }
        if (++ringClock >= period)
            ringClock = 0;
    }
}

void JohnPorkCallProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int mode = (int) apvts.getRawParameterValue ("mode")->load();
    const float grit = apvts.getRawParameterValue ("grit")->load();
    const bool ringtoneOn = apvts.getRawParameterValue ("ringtone")->load() > 0.5f;
    const int numSamples = buffer.getNumSamples();

    // Still ringing: audio passes through untouched, with the ringtone on top
    if (ringing.load())
    {
        if (ringtoneOn)
            addRingtone (buffer);

        level.store (juce::jmax (buffer.getMagnitude (0, numSamples), level.load() * 0.92f));
        return;
    }

    if (mode != lastMode)
        updateFilters (mode);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    highPass.process (ctx);
    lowPass.process (ctx);

    // Distortion (more in "answer" mode, the speaker crunch)
    const float gain = 1.0f + grit * (mode == 0 ? 6.0f : 3.0f);
    const float norm = 1.0f / std::tanh (gain);

    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            data[i] = std::tanh (data[i] * gain) * norm;
    }

    // Decline only: sample-rate reduction + bit reduction
    if (mode == 1)
    {
        const int holdSamples = 6;
        const float levels = 64.0f; // about 6 bits

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                if (holdCounter[ch] <= 0)
                {
                    held[ch] = std::round (data[i] * levels) / levels;
                    holdCounter[ch] = holdSamples;
                }
                --holdCounter[ch];
                data[i] = held[ch];
            }
        }
    }

    compressor.process (ctx);

    buffer.applyGain (mode == 1 ? 0.5f : 1.0f);

    if (mode == 0)
        callSamples.fetch_add (numSamples);

    level.store (juce::jmax (buffer.getMagnitude (0, numSamples), level.load() * 0.92f));
}

juce::AudioProcessorEditor* JohnPorkCallProcessor::createEditor()
{
    return new JohnPorkCallEditor (*this);
}

void JohnPorkCallProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void JohnPorkCallProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JohnPorkCallProcessor();
}
