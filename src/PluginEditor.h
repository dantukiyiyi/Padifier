#pragma once

#include "PluginProcessor.h"
#include <functional>
#include <memory>
#include <vector>

struct KnobSpec
{
    const char* paramId;
    const char* label;
    const char* suffix;
};

class KnobWithLabel : public juce::Component
{
public:
    KnobWithLabel (juce::AudioProcessorValueTreeState& apvts, const KnobSpec& spec)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 16);
        slider.setTextValueSuffix (spec.suffix);

        label.setText (spec.label, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);

        addAndMakeVisible (label);
        addAndMakeVisible (slider);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, spec.paramId, slider);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        label.setBounds (b.removeFromTop (16));
        slider.setBounds (b);
    }

private:
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class EffectSection : public juce::Component
{
public:
    EffectSection (const char* title, juce::AudioProcessorValueTreeState& apvts,
                   std::vector<KnobSpec> mains, std::vector<KnobSpec> subs,
                   std::function<void()> onLayout)
        : onLayoutChanged (std::move (onLayout)),
          numSubs ((int) subs.size())
    {
        titleLabel.setText (title, juce::dontSendNotification);
        addAndMakeVisible (titleLabel);

        for (auto& m : mains)
        {
            mainKnobs.push_back (std::make_unique<KnobWithLabel> (apvts, m));
            addAndMakeVisible (*mainKnobs.back());
        }

        for (auto& s : subs)
        {
            subKnobs.push_back (std::make_unique<KnobWithLabel> (apvts, s));
            addChildComponent (*subKnobs.back());
        }

        if (numSubs > 0)
        {
            toggle.setButtonText (arrowText (false));
            toggle.onClick = [this]
            {
                expanded = ! expanded;
                toggle.setButtonText (arrowText (expanded));
                for (auto& k : subKnobs)
                    k->setVisible (expanded);
                onLayoutChanged();
            };
            addAndMakeVisible (toggle);
        }
    }

    int getPreferredHeight() const
    {
        int h = 8 + 22 + 112 + 8;
        if (numSubs > 0)
        {
            h += 26;
            if (expanded)
                h += ((numSubs + 2) / 3) * 96;
        }
        return h;
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced (8);

        titleLabel.setBounds (b.removeFromTop (22));

        auto mainArea = b.removeFromTop (112);
        const int count = (int) mainKnobs.size();
        if (count > 0)
        {
            const int w = mainArea.getWidth() / count;
            for (auto& k : mainKnobs)
                k->setBounds (mainArea.removeFromLeft (w));
        }

        if (numSubs > 0)
        {
            toggle.setBounds (b.removeFromTop (26).reduced (30, 3));

            if (expanded)
            {
                const int cw = b.getWidth() / 3;
                for (int i = 0; i < numSubs; ++i)
                {
                    juce::Rectangle<int> cell (b.getX() + (i % 3) * cw,
                                               b.getY() + (i / 3) * 96,
                                               cw, 96);
                    subKnobs[(size_t) i]->setBounds (cell);
                }
            }
        }
    }

private:
    static juce::String arrowText (bool open)
    {
        return juce::String (open ? juce::CharPointer_UTF8 ("Ajustes \xe2\x96\xb2")
                                  : juce::CharPointer_UTF8 ("Ajustes \xe2\x96\xbc"));
    }

    juce::Label titleLabel;
    juce::TextButton toggle;
    std::vector<std::unique_ptr<KnobWithLabel>> mainKnobs;
    std::vector<std::unique_ptr<KnobWithLabel>> subKnobs;
    bool expanded = false;
    int numSubs = 0;
    std::function<void()> onLayoutChanged;
};

class PadifierEditor : public juce::AudioProcessorEditor
{
public:
    explicit PadifierEditor (PadifierProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override {}

private:
    void layoutSections();

    std::vector<std::unique_ptr<EffectSection>> sections;
};
