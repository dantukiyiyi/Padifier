#include "PluginEditor.h"
#include <algorithm>

PadifierEditor::PadifierEditor (PadifierProcessor& p)
    : juce::AudioProcessorEditor (&p)
{
    auto& apvts = p.apvts;
    std::function<void()> relayout = [this] { layoutSections(); };

    auto add = [&] (const char* title, std::vector<KnobSpec> mains, std::vector<KnobSpec> subs)
    {
        sections.push_back (std::make_unique<EffectSection> (
            title, apvts, std::move (mains), std::move (subs), relayout));
        addAndMakeVisible (*sections.back());
    };

    add ("Armonias", { { "armonias", "Mezcla", "%" } },
         { { "armonias_sens", "Sensibilidad", "%" },
           { "armonias_hi", "High End", "%" },
           { "armonias_lo", "Low End", "%" } });

    add ("Elasticidad", { { "elast", "Mezcla", "%" } },
         { { "elast_hi", "High End", "%" },
           { "elast_lo", "Low End", "%" } });

    add ("Reverb", { { "reverb", "Mezcla", "%" } },
         { { "reverb_hi", "High End", "%" },
           { "reverb_lo", "Low End", "%" } });

    add ("Distancia", { { "dist", "Mezcla", "%" } },
         { { "dist_hi", "High End", "%" },
           { "dist_lo", "Low End", "%" } });

    add ("Respiraciones", { { "resp", "Mezcla", "%" } },
         { { "resp_hi", "High End", "%" },
           { "resp_lo", "Low End", "%" } });

    add ("Stereo", { { "stereo", "Stereo", "%" } }, {});

    add ("Saturacion", { { "sat", "Mezcla", "%" } },
         { { "sat_agg", "Agresividad", "%" },
           { "sat_hi", "High End", "%" },
           { "sat_lo", "Low End", "%" } });

    add ("Flanger", { { "flanger", "Mezcla", "%" } },
         { { "flg_rate", "Rate", "Hz" },
           { "flg_depth", "Profundidad", "%" },
           { "flg_fb", "Feedback", "%" },
           { "flg_hi", "High End", "%" },
           { "flg_lo", "Low End", "%" } });

    add ("Master", { { "master_hi", "Filtro High End", "%" },
                     { "master_lo", "Filtro Low End", "%" } }, {});

    layoutSections();
}

void PadifierEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1b1b1f));
    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawText ("PADIFIER", getLocalBounds().removeFromTop (44), juce::Justification::centred);
}

void PadifierEditor::layoutSections()
{
    constexpr int colW = 250;
    constexpr int gap = 10;
    constexpr int top = 54;
    constexpr int cols = 3;

    int colH[cols] = { top, top, top };

    for (auto& s : sections)
    {
        const int c = (int) (std::min_element (colH, colH + cols) - colH);
        const int h = s->getPreferredHeight();
        s->setBounds (gap + c * (colW + gap), colH[c], colW, h);
        colH[c] += h + gap;
    }

    const int maxH = *std::max_element (colH, colH + cols);
    setSize (gap + cols * (colW + gap), maxH + gap);
}
