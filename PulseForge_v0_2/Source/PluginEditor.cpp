#include "PluginEditor.h"

namespace {
const juce::Colour bg(0xff101216), panel(0xff1b1f25), accent(0xffff8a32), muted(0xff8f98a5);
const juce::StringArray voiceNames { "KICK", "SNARE", "HAT", "PERC" };
}

PulseForgeAudioProcessorEditor::PulseForgeAudioProcessorEditor(PulseForgeAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(980, 470);
    title.setText("PULSE FORGE", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(27.0f, juce::Font::bold)));
    title.setColour(juce::Label::textColourId, juce::Colours::white); addAndMakeVisible(title);
    subtitle.setText("HYBRID DRUM LAB  /  SYNTH + SAMPLE  /  16-STEP ENGINE", juce::dontSendNotification);
    subtitle.setFont(juce::Font(juce::FontOptions(11.0f)));
    subtitle.setColour(juce::Label::textColourId, muted); addAndMakeVisible(subtitle);

    mutationLabel.setText("MUTATION", juce::dontSendNotification);
    mutationLabel.setColour(juce::Label::textColourId, accent); addAndMakeVisible(mutationLabel);
    mutation.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mutation.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    mutation.setRange(0.0, 1.0, .01); mutation.setValue(processor.getMutation());
    mutation.setColour(juce::Slider::rotarySliderFillColourId, accent);
    mutation.onValueChange = [this] { processor.setMutation((float)mutation.getValue()); };
    addAndMakeVisible(mutation);

    gainLabel.setText("OUTPUT", juce::dontSendNotification);
    gainLabel.setColour(juce::Label::textColourId, accent); addAndMakeVisible(gainLabel);
    gain.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gain.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    gain.setRange(0.0, 1.5, .01); gain.setValue(processor.getOutputGain());
    gain.setColour(juce::Slider::rotarySliderFillColourId, accent);
    gain.onValueChange = [this] { processor.setOutputGain((float)gain.getValue()); };
    addAndMakeVisible(gain);

    for (int row = 0; row < 4; ++row) {
        rowLabels[(size_t)row].setText(voiceNames[row], juce::dontSendNotification);
        rowLabels[(size_t)row].setColour(juce::Label::textColourId, row == 0 ? accent : juce::Colours::white);
        rowLabels[(size_t)row].setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        addAndMakeVisible(rowLabels[(size_t)row]);

        sourceLabels[(size_t)row].setText(processor.getVoiceSampleMode(row) ? "SAMPLE" : "SYNTH", juce::dontSendNotification);
        sourceLabels[(size_t)row].setColour(juce::Label::textColourId, muted); addAndMakeVisible(sourceLabels[(size_t)row]);

        auto src = std::make_unique<juce::TextButton>(processor.getVoiceSampleMode(row) ? "SAMPLE" : "SYNTH");
        src->setColour(juce::TextButton::buttonColourId, panel);
        src->onClick = [this, row, ptr = src.get()] {
            const bool next = !processor.getVoiceSampleMode(row);
            processor.setVoiceSampleMode(row, next);
            ptr->setButtonText(next ? "SAMPLE" : "SYNTH");
        };
        addAndMakeVisible(*src); sourceButtons[(size_t)row] = std::move(src);

        auto load = std::make_unique<juce::TextButton>("LOAD WAV");
        load->setColour(juce::TextButton::buttonColourId, panel);
        load->onClick = [this, row] {
            chooser.launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this, row](const juce::FileChooser& fc) {
                    const auto file = fc.getResult();
                    if (file.existsAsFile()) processor.loadSampleAsync(row, file);
                });
        };
        addAndMakeVisible(*load); loadButtons[(size_t)row] = std::move(load);

        sampleLabels[(size_t)row].setText(processor.getSampleName(row), juce::dontSendNotification);
        sampleLabels[(size_t)row].setColour(juce::Label::textColourId, muted);
        sampleLabels[(size_t)row].setFont(juce::Font(juce::FontOptions(10.0f)));
        addAndMakeVisible(sampleLabels[(size_t)row]);

        for (int col = 0; col < 16; ++col) {
            auto b = std::make_unique<juce::TextButton>(juce::String(col + 1));
            b->setClickingTogglesState(true);
            b->setToggleState(processor.getStep(row, col), juce::dontSendNotification);
            b->setColour(juce::TextButton::buttonColourId, panel);
            b->setColour(juce::TextButton::buttonOnColourId, accent.withAlpha(.92f));
            b->setColour(juce::TextButton::textColourOffId, muted);
            b->setColour(juce::TextButton::textColourOnId, juce::Colours::black);
            b->onClick = [this, row, col, ptr = b.get()] { processor.setStep(row, col, ptr->getToggleState()); };
            addAndMakeVisible(*b); steps[(size_t)row][(size_t)col] = std::move(b);
        }
    }
}

void PulseForgeAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg); g.setColour(panel);
    g.fillRoundedRectangle(16.0f, 16.0f, (float)getWidth()-32.0f, (float)getHeight()-32.0f, 12.0f);
    g.setColour(accent); g.fillRoundedRectangle(30.0f, 35.0f, 4.0f, 34.0f, 2.0f);
    g.setColour(juce::Colour(0xff303640)); g.drawHorizontalLine(88, 30.0f, (float)getWidth()-30.0f);
    g.setColour(muted); g.setFont(11.0f);
    g.drawText("VERSATILE MODE  •  HOST TEMPO SYNC  •  MIDI: 36 / 38 / 42 / 45", 32, getHeight()-28, 700, 16, juce::Justification::left);
}

void PulseForgeAudioProcessorEditor::resized()
{
    title.setBounds(42, 25, 300, 35); subtitle.setBounds(42, 59, 470, 20);
    mutationLabel.setBounds(725, 26, 90, 18); mutation.setBounds(710, 40, 90, 94);
    gainLabel.setBounds(840, 26, 90, 18); gain.setBounds(825, 40, 90, 94);

    const int y0 = 145, rowH = 57, gridX = 245, cellW = 36, cellGap = 4;
    for (int row = 0; row < 4; ++row) {
        const int y = y0 + row * rowH;
        rowLabels[(size_t)row].setBounds(32, y + 4, 68, 22);
        sourceButtons[(size_t)row]->setBounds(102, y, 70, 28);
        loadButtons[(size_t)row]->setBounds(176, y, 64, 28);
        sampleLabels[(size_t)row].setBounds(32, y + 29, 205, 18);
        for (int col = 0; col < 16; ++col)
            steps[(size_t)row][(size_t)col]->setBounds(gridX + col * (cellW + cellGap), y, cellW, 40);
    }
}
