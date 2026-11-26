#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr const char* kHiIds[7] = { "armonias_hi", "elast_hi", "reverb_hi", "dist_hi",
                                    "resp_hi", "sat_hi", "flg_hi" };
constexpr const char* kLoIds[7] = { "armonias_lo", "elast_lo", "reverb_lo", "dist_lo",
                                    "resp_lo", "sat_lo", "flg_lo" };
}

PadifierProcessor::PadifierProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout PadifierProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    const auto pct  = juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f);
    const auto bip  = juce::NormalisableRange<float> (-100.0f, 100.0f, 0.01f);
    const auto fb   = juce::NormalisableRange<float> (0.0f, 90.0f, 0.01f);
    const auto rate = juce::NormalisableRange<float> (0.05f, 5.0f, 0.001f, 0.4f);

    auto add = [&params] (const char* id, const char* name,
                          juce::NormalisableRange<float> range, float def)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, def));
    };

    add ("armonias",       "Armonias",                pct,  0.0f);
    add ("armonias_sens",  "Armonias Sensibilidad",   pct, 50.0f);
    add ("armonias_hi",    "Armonias High End",       pct,  0.0f);
    add ("armonias_lo",    "Armonias Low End",        pct,  0.0f);

    add ("elast",          "Elasticidad",             pct,  0.0f);
    add ("elast_hi",       "Elasticidad High End",    pct,  0.0f);
    add ("elast_lo",       "Elasticidad Low End",     pct,  0.0f);

    add ("reverb",         "Reverb",                  pct,  0.0f);
    add ("reverb_hi",      "Reverb High End",         pct,  0.0f);
    add ("reverb_lo",      "Reverb Low End",          pct,  0.0f);

    add ("dist",           "Distancia",               pct,  0.0f);
    add ("dist_hi",        "Distancia High End",      pct,  0.0f);
    add ("dist_lo",        "Distancia Low End",       pct,  0.0f);

    add ("resp",           "Respiraciones",           pct,  0.0f);
    add ("resp_hi",        "Respiraciones High End",  pct,  0.0f);
    add ("resp_lo",        "Respiraciones Low End",   pct,  0.0f);

    add ("stereo",         "Stereo",                  bip,  0.0f);

    add ("sat",            "Saturacion",              pct,  0.0f);
    add ("sat_agg",        "Saturacion Agresividad",  pct,  0.0f);
    add ("sat_hi",         "Saturacion High End",     pct,  0.0f);
    add ("sat_lo",         "Saturacion Low End",      pct,  0.0f);

    add ("flanger",        "Flanger",                 pct,  0.0f);
    add ("flg_rate",       "Flanger Rate",            rate, 0.5f);
    add ("flg_depth",      "Flanger Profundidad",     pct, 50.0f);
    add ("flg_fb",         "Flanger Feedback",        fb,  30.0f);
    add ("flg_hi",         "Flanger High End",        pct,  0.0f);
    add ("flg_lo",         "Flanger Low End",         pct,  0.0f);

    add ("master_hi",      "Filtro High End",         pct,  0.0f);
    add ("master_lo",      "Filtro Low End",          pct,  0.0f);

    return { params.begin(), params.end() };
}

void PadifierProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentFs = sampleRate;
    for (auto& c : ch)
        c.prepare (sampleRate);
    env = 0.0f;
    lfoPhase = 0.0;
}

bool PadifierProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void PadifierProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    if (n == 0 || buffer.getNumChannels() < 2)
        return;

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    Params P;
    P.arm     = get ("armonias") / 100.0f;
    P.armSens = get ("armonias_sens") / 100.0f;
    P.elast   = get ("elast") / 100.0f;
    P.rev     = get ("reverb") / 100.0f;
    P.dist    = get ("dist") / 100.0f;
    P.resp    = get ("resp") / 100.0f;
    P.sat     = get ("sat") / 100.0f;
    P.satAgg  = get ("sat_agg") / 100.0f;
    P.flg     = get ("flanger") / 100.0f;
    P.flgDepth = get ("flg_depth") / 100.0f;
    P.flgFb   = get ("flg_fb") / 100.0f;

    const float rate   = get ("flg_rate");
    const float stereo = get ("stereo") / 100.0f;

    // Reverb tail: feedback per comb so the tail decays 60 dB over T60.
    const double t60 = 0.005 + (double) P.rev * 3.0;
    for (int k = 0; k < 2; ++k)
        P.combFb[k] = (float) std::pow (10.0, -3.0 * ch[0].comb[k].len / (t60 * currentFs));

    for (int c = 0; c < 2; ++c)
    {
        auto& s = ch[c];
        for (int i = 0; i < 7; ++i)
            s.fx[i].setup (get (kHiIds[i]), get (kLoIds[i]), currentFs);
        s.master.setup (get ("master_hi"), get ("master_lo"), currentFs);
        s.distLp.setCutoff (20000.0 * std::pow (0.1, (double) P.dist), currentFs);
    }

    const float* inL = buffer.getReadPointer (0);
    const float* inR = buffer.getReadPointer (1);
    float* outL = buffer.getWritePointer (0);
    float* outR = buffer.getWritePointer (1);

    const double twoPi = 6.283185307179586;
    const double quarterPi = 1.5707963267948966;

    for (int i = 0; i < n; ++i)
    {
        const float xL = inL[i];
        const float xR = inR[i];

        const float mono = 0.5f * (std::abs (xL) + std::abs (xR));
        env = mono > env ? mono : env * 0.9995f;

        const float lfoL = (float) std::sin (twoPi * lfoPhase);
        const float lfoR = (float) std::sin (twoPi * lfoPhase + quarterPi);
        lfoPhase += (double) rate / currentFs;
        while (lfoPhase >= 1.0) lfoPhase -= 1.0;

        float yL = processChain (ch[0], xL, env, lfoL, P, currentFs);
        float yR = processChain (ch[1], xR, env, lfoR, P, currentFs);

        // Stereo: mid/side. Positive widens the sides; negative folds sides into mid.
        float M = 0.5f * (yL + yR);
        float S = 0.5f * (yL - yR);
        if (stereo > 0.0f)
        {
            S *= 1.0f + stereo;
        }
        else if (stereo < 0.0f)
        {
            const float k = -stereo;
            M += k * S;
            S *= 1.0f - k;
        }
        yL = M + S;
        yR = M - S;

        outL[i] = ch[0].master.process (yL);
        outR[i] = ch[1].master.process (yR);
    }
}

juce::AudioProcessorEditor* PadifierProcessor::createEditor()
{
    return new PadifierEditor (*this);
}

void PadifierProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PadifierProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// JUCE entry point for the plugin host.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PadifierProcessor();
}
