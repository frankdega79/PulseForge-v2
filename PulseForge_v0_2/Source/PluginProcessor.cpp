#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace { constexpr double pi = juce::MathConstants<double>::pi; }

PulseForgeAudioProcessor::PulseForgeAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    for (auto& row : pattern) for (auto& step : row) step.store(0);
    for (auto& mode : sampleModes) mode.store(0);
    for (int s : { 0, 8 }) pattern[0][s].store(1);
    for (int s : { 4, 12 }) pattern[1][s].store(1);
    for (int s : { 2, 6, 10, 14 }) pattern[2][s].store(1);
    for (int s : { 7, 15 }) pattern[3][s].store(1);
}

void PulseForgeAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    lastStep = -1;
    for (auto& v : voices) v = Voice{};
}
void PulseForgeAudioProcessor::releaseResources() {}

bool PulseForgeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void PulseForgeAudioProcessor::setStep(int voice, int step, bool enabled)
{
    if (voice >= 0 && voice < 4 && step >= 0 && step < 16)
        pattern[(size_t)voice][(size_t)step].store(enabled ? 1 : 0);
}
bool PulseForgeAudioProcessor::getStep(int voice, int step) const
{
    return voice >= 0 && voice < 4 && step >= 0 && step < 16
        && pattern[(size_t)voice][(size_t)step].load() != 0;
}
void PulseForgeAudioProcessor::setMutation(float value) { mutation.store(juce::jlimit(0.0f, 1.0f, value)); }
void PulseForgeAudioProcessor::setOutputGain(float value) { outputGain.store(juce::jlimit(0.0f, 1.5f, value)); }
void PulseForgeAudioProcessor::setVoiceSampleMode(int voice, bool sampleMode)
{
    if (voice >= 0 && voice < 4) sampleModes[(size_t)voice].store(sampleMode ? 1 : 0);
}
bool PulseForgeAudioProcessor::getVoiceSampleMode(int voice) const
{
    return voice >= 0 && voice < 4 && sampleModes[(size_t)voice].load() != 0;
}
juce::String PulseForgeAudioProcessor::getSampleName(int voice) const
{
    const juce::ScopedLock lock(sampleLock);
    if (voice < 0 || voice >= 4 || samplePaths[(size_t)voice].isEmpty()) return "No sample loaded";
    return juce::File(samplePaths[(size_t)voice]).getFileName();
}
void PulseForgeAudioProcessor::loadSampleAsync(int voice, const juce::File& file)
{
    if (voice < 0 || voice >= 4 || !file.existsAsFile()) return;
    loaderPool.addJob([this, voice, file]() {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(file));
        if (reader == nullptr || reader->lengthInSamples <= 0 || reader->lengthInSamples > 100000000) return;
        const int channels = (int)juce::jlimit<int64>(1, 2, reader->numChannels);
        const int length = (int)reader->lengthInSamples;
        auto data = std::make_shared<SampleData>();
        data->audio.setSize(channels, length);
        data->sampleRate = reader->sampleRate;
        data->path = file.getFullPathName();
        if (!reader->read(&data->audio, 0, length, 0, true, channels > 1)) return;
        const juce::ScopedLock lock(sampleLock);
        samples[(size_t)voice] = data;
        samplePaths[(size_t)voice] = file.getFullPathName();
    });
}
void PulseForgeAudioProcessor::triggerVoice(int type, float velocity)
{
    Voice* chosen = nullptr;
    for (auto& v : voices) if (!v.active) { chosen = &v; break; }
    if (!chosen) {
        chosen = &voices[0];
        for (auto& v : voices) if (v.env < chosen->env) chosen = &v;
    }
    *chosen = Voice{};
    chosen->type = juce::jlimit(0, 3, type);
    chosen->velocity = juce::jlimit(0.0f, 1.0f, velocity);
    chosen->env = 1.0f;
    chosen->active = true;
    chosen->sampleMode = getVoiceSampleMode(type);
    {
        const juce::ScopedLock lock(sampleLock);
        chosen->sample = samples[(size_t)type];
    }
}
void PulseForgeAudioProcessor::triggerStep(int step)
{
    static constexpr float vel[4] = { .95f, .78f, .52f, .66f };
    for (int v = 0; v < 4; ++v) if (getStep(v, step)) triggerVoice(v, vel[v]);
}

float PulseForgeAudioProcessor::renderVoice(Voice& v)
{
    if (!v.active) return 0.0f;
    const float m = mutation.load();
    const double dt = 1.0 / sr;
    float out = 0.0f;
    if (v.sampleMode && v.sample && v.sample->audio.getNumSamples() > 0) {
        const auto& data = *v.sample;
        const int len = data.audio.getNumSamples();
        const double ratio = data.sampleRate / sr;
        const int index = (int)v.samplePosition;
        if (index >= len) { v.active = false; return 0.0f; }
        const int next = juce::jmin(index + 1, len - 1);
        const float frac = (float)(v.samplePosition - index);
        float mono = 0.0f;
        for (int ch = 0; ch < data.audio.getNumChannels(); ++ch) {
            const float a = data.audio.getSample(ch, index);
            const float b = data.audio.getSample(ch, next);
            mono += a + (b - a) * frac;
        }
        mono /= (float)data.audio.getNumChannels();
        out = mono * v.velocity;
        // Mutation adds sample saturation while preserving sample timing.
        out = std::tanh(out * (1.0f + m * 2.5f));
        v.samplePosition += ratio;
        return out;
    }

    const double t = v.phase;
    if (v.type == 0) {
        const double freq = 42.0 + 125.0 * std::exp(-t * (18.0 + 15.0 * m));
        out = (float)std::sin(v.phase2) * v.env;
        v.phase2 += 2.0 * pi * freq * dt;
        if (t < .012) out += (float)(.16 * (1.0 - t / .012) * v.env);
        v.env *= (float)std::exp(-dt * (7.5 + 7.0 * m));
    } else if (v.type == 1) {
        const float n = random.nextFloat() * 2.0f - 1.0f;
        out = (.56f * n + .44f * (float)std::sin(v.phase2)) * v.env;
        v.phase2 += 2.0 * pi * (185.0 + 30.0 * m) * dt;
        v.env *= (float)std::exp(-dt * (15.0 + 10.0 * m));
    } else if (v.type == 2) {
        const float n = random.nextFloat() * 2.0f - 1.0f;
        out = n * v.env * (.65f + .25f * m);
        v.env *= (float)std::exp(-dt * (48.0 + 38.0 * m));
    } else {
        out = (float)(.65 * std::sin(v.phase2) + .35 * std::sin(v.phase2 * 1.47)) * v.env;
        v.phase2 += 2.0 * pi * (310.0 + 260.0 * m) * dt;
        v.env *= (float)std::exp(-dt * (11.0 + 5.0 * m));
    }
    v.phase += dt;
    if (v.env < .0008f || v.phase > 3.0) v.active = false;
    return out * v.velocity;
}

void PulseForgeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    auto* head = getPlayHead();
    juce::AudioPlayHead::PositionInfo pos;
    const bool hasPos = head != nullptr && head->getPosition().hasValue();
    if (hasPos) pos = *head->getPosition();
    const double bpm = hasPos && pos.getBpm().hasValue() ? *pos.getBpm() : 120.0;
    const double ppqStart = hasPos && pos.getPpqPosition().hasValue() ? *pos.getPpqPosition() : 0.0;
    const bool playing = hasPos && pos.getIsPlaying();
    const double samplesPerBeat = sr * 60.0 / bpm;

    for (const auto metadata : midi) {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
            const int n = msg.getNoteNumber();
            if (n == 36) triggerVoice(0, msg.getFloatVelocity());
            else if (n == 38 || n == 40) triggerVoice(1, msg.getFloatVelocity());
            else if (n == 42 || n == 44 || n == 46) triggerVoice(2, msg.getFloatVelocity());
            else if (n == 45 || n == 47 || n == 50) triggerVoice(3, msg.getFloatVelocity());
        }
    }
    midi.clear();

    if (playing) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            const double ppq = ppqStart + (double)i / samplesPerBeat;
            const int step = ((int)std::floor(ppq * 4.0 + 1.0e-6)) & 15;
            if (step != lastStep) { triggerStep(step); lastStep = step; }
        }
    } else lastStep = -1;

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float s = 0.0f;
        for (auto& v : voices) s += renderVoice(v);
        s = std::tanh(s * (1.0f + mutation.load() * 1.5f)) * outputGain.load();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) buffer.setSample(ch, i, s);
    }
}

void PulseForgeAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream out(destData, false);
    out.writeFloat(mutation.load());
    out.writeFloat(outputGain.load());
    for (int v = 0; v < 4; ++v) {
        out.writeInt(sampleModes[(size_t)v].load());
        juce::String path;
        {
            const juce::ScopedLock lock(sampleLock);
            path = samplePaths[(size_t)v];
        }
        out.writeString(path);
        for (int s = 0; s < 16; ++s) out.writeInt(getStep(v, s) ? 1 : 0);
    }
}
void PulseForgeAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::MemoryInputStream in(data, (size_t)sizeInBytes, false);
    if (in.getNumBytesRemaining() < 2 * (int)sizeof(float)) return;
    setMutation(in.readFloat());
    setOutputGain(in.readFloat());
    for (int v = 0; v < 4; ++v) {
        if (in.getNumBytesRemaining() < (int)sizeof(int)) return;
        setVoiceSampleMode(v, in.readInt() != 0);
        const auto path = in.readString();
        if (path.isNotEmpty()) {
            {
                const juce::ScopedLock lock(sampleLock);
                samplePaths[(size_t)v] = path;
            }
            loadSampleAsync(v, juce::File(path));
        }
        for (int s = 0; s < 16; ++s) {
            if (in.getNumBytesRemaining() < (int)sizeof(int)) return;
            setStep(v, s, in.readInt() != 0);
        }
    }
}
juce::AudioProcessorEditor* PulseForgeAudioProcessor::createEditor() { return new PulseForgeAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PulseForgeAudioProcessor(); }
