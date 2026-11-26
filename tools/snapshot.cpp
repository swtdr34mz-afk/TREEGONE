/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
/* A picture of the editor (a PNG), without a window: for checking the panel's drawing.
 *   TreegoneShot out.png [scale] [program] [a .syx to load first] */
#include <cstdlib>
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main(int argc, char **argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    juce::String out = argc > 1 ? argv[1] : "shot.png";
    float scale = argc > 2 ? (float)std::atof(argv[2]) : 1.0f;
    int prog = argc > 3 ? std::atoi(argv[3]) : 0;
    TreegoneProcessor proc;
    if (argc > 4) {
        juce::String msg;
        proc.importSyx(juce::File(juce::String(argv[4])), msg);
        std::printf("%s\n", msg.toRawUTF8());
    }
    proc.setCurrentProgram(prog);
    proc.uiScale = 1.0f;
    {
        proc.setPlayConfigDetails(0, 2, 48000.0, 512);
        proc.prepareToPlay(48000.0, 512);
        juce::AudioBuffer<float> buf(2, 512);
        float peak = 0.0f;
        for (int b = 0; b < 200; b++) {
            juce::MidiBuffer m;
            if (b == 2)
                m.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)100), 10);
            if (b == 150)
                m.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
            buf.clear();
            proc.processBlock(buf, m);
            peak = juce::jmax(peak, buf.getMagnitude(0, 512));
        }
        juce::MemoryBlock st;
        proc.getStateInformation(st);
        proc.setStateInformation(st.getData(), (int)st.getSize());
        std::printf("processed: peak %.3f, state %d bytes, %d programs\n", peak, (int)st.getSize(), proc.programCount());
    }
    std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
    ed->setSize(TreegonePanel::W, TreegonePanel::H);
    for (auto *c : ed->getChildren())
        for (auto *k : c->getChildren())
            if (auto *lv = dynamic_cast<TreegonePanel::Live *>(k))
                lv->poll();
    if (std::getenv("TREEGONE_RIGHTCLICK_TEST") != nullptr) {
        /* a right click (down, a drag, up, a double click) on every control: no parameter may change */
        auto values = [&] {
            std::vector<float> v;
            for (auto *p : proc.getParameters())
                v.push_back(p->getValue());
            return v;
        };
        auto before = values();
        int n = 0;
        auto src = juce::Desktop::getInstance().getMainMouseSource();
        juce::ModifierKeys right(juce::ModifierKeys::rightButtonModifier);
        for (auto *c : ed->getChildren())
            for (auto *k : c->getChildren())
                if (dynamic_cast<TreegonePanel::Live *>(k) != nullptr) {
                    auto at = k->getLocalBounds().getCentre().toFloat();
                    auto t = juce::Time::getCurrentTime();
                    juce::MouseEvent down(src, at, right, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, k, k, t, at, t, 1, false);
                    juce::MouseEvent drag(src, at.translated(0.0f, -60.0f), right, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, k, k, t, at, t, 1, true);
                    k->mouseDown(down);
                    k->mouseDrag(drag);
                    k->mouseUp(drag);
                    k->mouseDoubleClick(down);
                    n++;
                }
        auto after = values();
        int changed = 0;
        for (size_t i = 0; i < before.size(); i++)
            if (before[i] != after[i]) {
                std::printf("changed by a right click: %s\n", proc.getParameters()[(int)i]->getName(40).toRawUTF8());
                changed++;
            }
        std::printf("right clicks on %d controls: %d parameters changed\n", n, changed);
        if (changed)
            return 1;
    }
    auto img = ed->createComponentSnapshot(ed->getLocalBounds(), true, scale);
    juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile(out);
    f.deleteFile();
    juce::FileOutputStream os(f);
    juce::PNGImageFormat().writeImageToStream(img, os);
    std::printf("wrote %s (%d x %d)\n", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    ed.reset();
    return 0;
}
