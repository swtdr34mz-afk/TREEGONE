/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Sysex.h"

using namespace tg;

/* ------------------------------------------------------------------------------- parameters --- */
juce::AudioProcessorValueTreeState::ParameterLayout TreegoneProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int p = 0; p < NPARAM; p++) {
        const ParamDef &d = PARAMS[p];
        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{d.id, 1}, d.name, d.lo, d.hi, d.def,
            juce::AudioParameterIntAttributes().withStringFromValueFunction([p](int v, int) {
                char t[40];
                paramText(p, v, t, sizeof t);
                return juce::String(t);
            })));
    }
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"voices", 1}, "Voices",
                                                            juce::StringArray{"6 Voice", "12 Voice"}, 0));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"host_sync", 1}, "Host Sync", true));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"quality", 1}, "Quality",
                                                            juce::StringArray{"ECO (2x)", "HIGH (4x)"}, 1));
    layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"transpose", 1}, "Transpose", -2, 2, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"volume", 1}, "Master Volume", juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) {
            return v <= 0.0001f ? juce::String("-inf") : juce::String(20.0f * std::log10(v * v * 2.0f), 1) + " dB";
        })));
    return layout;
}

TreegoneProcessor::TreegoneProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "TREEGONE", createLayout())
{
    for (int p = 0; p < NPARAM; p++)
        params[p] = apvts.getParameter(PARAMS[p].id);
    loadImported();
    loadUserPrograms();
    live = FACTORY[0];
    applyProgram(FACTORY[0], true);
}

int TreegoneProcessor::value(int p) const { return (int)std::lround(params[p]->convertFrom0to1(params[p]->getValue())); }
void TreegoneProcessor::setValue(int p, int v)
{
    auto *q = params[p];
    v = juce::jlimit(PARAMS[p].lo, PARAMS[p].hi, v);
    q->beginChangeGesture();
    q->setValueNotifyingHost(q->convertTo0to1((float)v));
    q->endChangeGesture();
}

void TreegoneProcessor::command(Cmd c, int v)
{
    int s1, n1, s2, n2;
    fifo.prepareToWrite(1, s1, n1, s2, n2);
    if (n1 > 0)
        cmds[(size_t)s1] = {(int)c, v};
    else if (n2 > 0)
        cmds[(size_t)s2] = {(int)c, v};
    fifo.finishedWrite(n1 + n2);
}

/* ------------------------------------------------------------------------------- programs --- */
const Program &TreegoneProcessor::programAt(int index) const
{
    if (index < NFACTORY)
        return FACTORY[index];
    index -= NFACTORY;
    if (index < (int)imported.size())
        return imported[(size_t)index].prog;
    index -= (int)imported.size();
    return userPrograms[(size_t)index];
}
int TreegoneProcessor::sourceOf(int index) const
{
    if (index < NFACTORY)
        return 0;
    return index < NFACTORY + (int)imported.size() ? 1 : 2;
}
int TreegoneProcessor::firstOf(int source) const
{
    if (source == 0)
        return 0;
    if (source == 1)
        return imported.empty() ? -1 : NFACTORY;
    return userPrograms.empty() ? -1 : NFACTORY + (int)imported.size();
}
int TreegoneProcessor::numberInSource(int index) const
{
    int s = sourceOf(index);
    if (s == 1)
        return imported[(size_t)(index - NFACTORY)].bank * 100 + imported[(size_t)(index - NFACTORY)].number;
    return index - (s == 0 ? 0 : NFACTORY + (int)imported.size());
}
juce::String TreegoneProcessor::trigonPlace(int index) const
{
    if (sourceOf(index) != 1)
        return {};
    auto &im = imported[(size_t)(index - NFACTORY)];
    return im.bank < 0 ? juce::String("Edit") : juce::String(im.bank * 100 + im.number).paddedLeft('0', 3);
}

void TreegoneProcessor::applyProgram(const Program &p0, bool toHost)
{
    Program p = p0;
    p.p[P_SEQ_ON] = 0;
    {
        juce::SpinLock::ScopedLockType lock(progLock);
        pending = p;
        live = p;
    }
    pendingSet.store(true);
    if (toHost)
        for (int i = 0; i < NPARAM; i++)
            params[i]->setValueNotifyingHost(params[i]->convertTo0to1((float)p.p[i]));
}

void TreegoneProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;
    program = index;
    applyProgram(programAt(index), true);
    updateHostDisplay(ChangeDetails().withProgramChanged(true));
}

const juce::String TreegoneProcessor::getProgramName(int index)
{
    if (index < 0 || index >= getNumPrograms())
        return {};
    juce::String n(programAt(index).name);
    if (sourceOf(index) == 1)
        return "T" + trigonPlace(index) + " " + n;
    if (sourceOf(index) == 2)
        return "U" + juce::String(numberInSource(index) + 1).paddedLeft('0', 2) + " " + n;
    return n;
}

juce::String TreegoneProcessor::programName() const
{
    juce::SpinLock::ScopedLockType lock(progLock);
    return juce::String(live.name);
}
void TreegoneProcessor::setProgramName(const juce::String &n)
{
    juce::SpinLock::ScopedLockType lock(progLock);
    std::snprintf(live.name, NAME_LEN, "%s", n.substring(0, NAME_LEN - 1).toRawUTF8());
}
std::array<int8_t, NCHORD> TreegoneProcessor::chordCopy() const
{
    juce::SpinLock::ScopedLockType lock(progLock);
    std::array<int8_t, NCHORD> c;
    for (int i = 0; i < NCHORD; i++)
        c[(size_t)i] = live.chord[i];
    return c;
}
SeqData TreegoneProcessor::seqCopy() const
{
    juce::SpinLock::ScopedLockType lock(progLock);
    return live.seq;
}

Program TreegoneProcessor::currentProgram() const
{
    Program p;
    {
        juce::SpinLock::ScopedLockType lock(progLock);
        p = live;
    }
    for (int i = 0; i < NPARAM; i++)
        p.p[i] = (int16_t)value(i);
    return p;
}

juce::File TreegoneProcessor::programFolder()
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("TREEGONE").getChildFile("Programs");
}
juce::File TreegoneProcessor::importFolder()
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("TREEGONE").getChildFile("Imported");
}

void TreegoneProcessor::programToXml(const Program &p, juce::XmlElement &x)
{
    x.setAttribute("name", juce::String(p.name));
    x.setAttribute("version", 1);
    for (int i = 0; i < NPARAM; i++)
        x.setAttribute(PARAMS[i].id, (int)p.p[i]);
    juce::MemoryBlock mb(&p.seq, sizeof p.seq);
    x.setAttribute("seq", mb.toBase64Encoding());
    juce::MemoryBlock cb(p.chord, sizeof p.chord);
    x.setAttribute("chord", cb.toBase64Encoding());
}

bool TreegoneProcessor::programFromXml(const juce::XmlElement &x, Program &p)
{
    programInit(p);
    std::snprintf(p.name, NAME_LEN, "%s", x.getStringAttribute("name", "Program").substring(0, NAME_LEN - 1).toRawUTF8());
    for (int i = 0; i < NPARAM; i++)
        p.p[i] = (int16_t)juce::jlimit(PARAMS[i].lo, PARAMS[i].hi, x.getIntAttribute(PARAMS[i].id, PARAMS[i].def));
    juce::MemoryBlock mb;
    if (mb.fromBase64Encoding(x.getStringAttribute("seq")) && mb.getSize() == sizeof p.seq)
        std::memcpy(&p.seq, mb.getData(), sizeof p.seq);
    juce::MemoryBlock cb;
    if (cb.fromBase64Encoding(x.getStringAttribute("chord")) && cb.getSize() == sizeof p.chord)
        std::memcpy(p.chord, cb.getData(), sizeof p.chord);
    if (p.seq.length > PSEQ_STEPS)
        p.seq.length = PSEQ_STEPS;
    return true;
}

void TreegoneProcessor::loadImported()
{
    imported.clear();
    auto files = importFolder().findChildFiles(juce::File::findFiles, false, "*.syx");
    files.sort();
    for (auto &f : files) {
        juce::MemoryBlock mb;
        if (!f.loadFileAsData(mb))
            continue;
        auto ps = readSyx((const unsigned char *)mb.getData(), mb.getSize());
        for (auto &sp : ps)
            imported.push_back({sp.prog, sp.bank, sp.number, f.getFileNameWithoutExtension()});
    }
}

int TreegoneProcessor::importSyx(const juce::File &f, juce::String &message)
{
    juce::MemoryBlock mb;
    if (!f.loadFileAsData(mb)) {
        message = "Could not read " + f.getFileName();
        return 0;
    }
    auto ps = readSyx((const unsigned char *)mb.getData(), mb.getSize());
    if (ps.empty()) {
        message = f.getFileName() + ": no Trigon-6 programs in it";
        return 0;
    }
    /* kept as it is, on this computer only: next time TREEGONE opens, its programs are there */
    auto folder = importFolder();
    folder.createDirectory();
    auto dest = folder.getChildFile(f.getFileName());
    if (dest != f)
        f.copyFileTo(dest);
    int before = NFACTORY + (int)imported.size();
    loadImported();
    int at = before;
    for (size_t i = 0; i < imported.size(); i++)
        if (imported[i].file == dest.getFileNameWithoutExtension()) {
            at = NFACTORY + (int)i;
            break;
        }
    updateHostDisplay(ChangeDetails().withProgramChanged(true));
    setCurrentProgram(at);
    message = juce::String((int)ps.size()) + " Trigon programs loaded from " + f.getFileName();
    return (int)ps.size();
}

void TreegoneProcessor::clearImported()
{
    for (auto &f : importFolder().findChildFiles(juce::File::findFiles, false, "*.syx"))
        f.deleteFile();
    bool wasImported = sourceOf(program) == 1;
    imported.clear();
    if (wasImported)
        program = 0;
    updateHostDisplay(ChangeDetails().withProgramChanged(true));
}

void TreegoneProcessor::loadUserPrograms()
{
    userPrograms.clear();
    auto files = programFolder().findChildFiles(juce::File::findFiles, false, "*.treegone");
    files.sort();
    for (auto &f : files)
        if (auto xml = juce::XmlDocument::parse(f))
            if (xml->hasTagName("TREEGONEPROGRAM")) {
                Program p;
                if (programFromXml(*xml, p))
                    userPrograms.push_back(p);
            }
}

int TreegoneProcessor::writeProgram(const juce::String &name)
{
    auto clean = name.trim().substring(0, NAME_LEN - 1);
    if (clean.isEmpty())
        return -1;
    Program p = currentProgram();
    std::snprintf(p.name, NAME_LEN, "%s", clean.toRawUTF8());
    juce::XmlElement xml("TREEGONEPROGRAM");
    programToXml(p, xml);
    auto folder = programFolder();
    folder.createDirectory();
    if (!xml.writeTo(folder.getChildFile(juce::File::createLegalFileName(clean) + ".treegone")))
        return -1;
    setProgramName(clean);
    loadUserPrograms();
    for (size_t i = 0; i < userPrograms.size(); i++)
        if (juce::String(userPrograms[i].name) == clean) {
            program = NFACTORY + (int)imported.size() + (int)i;
            updateHostDisplay(ChangeDetails().withProgramChanged(true));
            return program;
        }
    return -1;
}

/* ------------------------------------------------------------------------------- the block --- */
bool TreegoneProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void TreegoneProcessor::prepareToPlay(double sampleRate, int)
{
    engine.prepare(sampleRate);
    Program cur = currentProgram();
    {
        juce::SpinLock::ScopedLockType lock(progLock);
        pending = cur;
    }
    pendingSet.store(true);
    sentVoices = -1;
    lastGain = -1.0f;
}

void TreegoneProcessor::handleMidi(const juce::MidiMessage &m)
{
    if (m.isNoteOn() && splitLearn.load()) {     /* SET: this key is the split point (it plays above it) */
        learnedNote = juce::jlimit(0, 127, m.getNoteNumber() + 12 * transpose);
        splitLearn = false;
        return;
    }
    if (m.isNoteOn())
        engine.noteOn(juce::jlimit(0, 127, m.getNoteNumber() + 12 * transpose), m.getFloatVelocity());
    else if (m.isNoteOff())
        engine.noteOff(juce::jlimit(0, 127, m.getNoteNumber() + 12 * transpose));
    else if (m.isPitchWheel())
        engine.bend((float)(m.getPitchWheelValue() - 8192) / 8192.0f);
    else if (m.isChannelPressure())
        engine.pressure((float)m.getChannelPressureValue() / 127.0f);
    else if (m.isAftertouch())
        engine.pressure((float)m.getAfterTouchValue() / 127.0f);
    else if (m.isSustainPedalOn() || m.isSustainPedalOff())
        engine.sustain(m.isSustainPedalOn());
    else if (m.isController()) {
        int cc = m.getControllerNumber();
        float v = (float)m.getControllerValue() / 127.0f;
        switch (cc) {
        case 1: engine.modWheel(v); break;
        case 120: engine.panic(); break;
        case 123: engine.allOff(); break;
        case 121:
            engine.bend(0.0f);
            engine.modWheel(0.0f);
            engine.pressure(0.0f);
            engine.sustain(false);
            break;
        default: break;
        }
    } else if (m.isAllNotesOff() || m.isAllSoundOff())
        engine.allOff();
}

void TreegoneProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    float *L = buffer.getWritePointer(0), *R = buffer.getWritePointer(buffer.getNumChannels() > 1 ? 1 : 0);

    if (pendingSet.exchange(false)) {
        Program p;
        {
            juce::SpinLock::ScopedLockType lock(progLock);
            p = pending;
        }
        engine.setProgram(p);
        for (int i = 0; i < NPARAM; i++)
            sent[i] = p.p[i];
    }
    for (int i = 0; i < NPARAM; i++) {
        int v = value(i);
        if (v != sent[i]) {
            if (i == P_SEQ_ON)
                engine.seqPlay(v != 0);
            engine.setParam(i, v);
            sent[i] = (int16_t)v;
        }
    }
    int voices = apvts.getRawParameterValue("voices")->load() > 0.5f ? 12 : 6;
    if (voices != sentVoices) {
        engine.setVoices(voices);
        sentVoices = voices;
    }
    transpose = (int)std::lround(apvts.getRawParameterValue("transpose")->load());
    engine.setQuality(apvts.getRawParameterValue("quality")->load() > 0.5f ? 1 : 0);

    {
        int s1, n1, s2, n2;
        fifo.prepareToRead(fifo.getNumReady(), s1, n1, s2, n2);
        auto run = [&](const Command &c) {
            switch (c.c) {
            case C_PLAY: engine.seqPlay(c.v != 0); break;
            case C_RECORD: engine.seqRecord(c.v != 0); break;
            case C_REST: engine.seqRest(); break;
            case C_TIE: engine.seqTie(); break;
            case C_BACK: engine.seqBack(); break;
            case C_HOLD: engine.hold(c.v != 0); break;
            case C_CHORD: engine.unisonChord(); break;
            case C_PANIC: engine.panic(); break;
            default: break;
            }
        };
        for (int i = 0; i < n1; i++)
            run(cmds[(size_t)(s1 + i)]);
        for (int i = 0; i < n2; i++)
            run(cmds[(size_t)(s2 + i)]);
        fifo.finishedRead(n1 + n2);
    }

    bool sync = apvts.getRawParameterValue("host_sync")->load() > 0.5f;
    double bpm = 120.0, ppq = 0.0;
    bool hostPlaying = false;
    if (auto *ph = getPlayHead())
        if (auto pos = ph->getPosition()) {
            if (auto b = pos->getBpm())
                bpm = *b;
            hostPlaying = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition())
                ppq = *q;
        }
    engine.host(hostPlaying, ppq, bpm, sync);

    keyboardState.processNextMidiBuffer(midi, 0, n, true);

    int done = 0;
    for (const auto meta : midi) {
        int at = juce::jlimit(0, n, meta.samplePosition);
        if (at > done) {
            engine.render(L + done, R + done, at - done);
            done = at;
        }
        handleMidi(meta.getMessage());
    }
    if (n > done)
        engine.render(L + done, R + done, n - done);

    for (int v = 0; v < MAXV; v++)
        voiceLevel[v].store(engine.voiceLevel(v), std::memory_order_relaxed);
    seqStepNow.store(engine.seqStep(), std::memory_order_relaxed);
    recStepNow.store(engine.seqRecStep(), std::memory_order_relaxed);
    recording.store(engine.seqRecording(), std::memory_order_relaxed);
    playingNow.store(engine.seqPlaying(), std::memory_order_relaxed);
    beatLed.store(engine.clockBeat(), std::memory_order_relaxed);
    lfoNow.store(engine.lfoShown(), std::memory_order_relaxed);
    bpmNow.store(engine.bpmShown(), std::memory_order_relaxed);
    holdNow.store(engine.holding(), std::memory_order_relaxed);
    {
        const Program &ep = engine.program();
        juce::SpinLock::ScopedTryLockType lock(progLock);
        if (lock.isLocked() && !pendingSet.load()) {
            live.seq = ep.seq;
            std::memcpy(live.chord, ep.chord, sizeof live.chord);
        }
    }

    float vol = apvts.getRawParameterValue("volume")->load();
    float gain = vol * vol * 2.0f;
    if (lastGain < 0.0f)
        lastGain = gain;
    buffer.applyGainRamp(0, 0, n, lastGain, gain);
    if (buffer.getNumChannels() > 1)
        buffer.applyGainRamp(1, 0, n, lastGain, gain);
    lastGain = gain;
    for (int c = 2; c < buffer.getNumChannels(); c++)
        buffer.clear(c, 0, n);
}

/* ------------------------------------------------------------------------------ state --- */
void TreegoneProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    auto state = apvts.copyState();
    state.setProperty("program", program, nullptr);
    state.setProperty("uiScale", (double)uiScale.load(), nullptr);
    state.setProperty("uiScaleSet", uiScaleSet.load(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    auto *pg = xml->createNewChildElement("TREEGONEPROGRAM");
    programToXml(currentProgram(), *pg);
    copyXmlToBinary(*xml, destData);
}

void TreegoneProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (!xml || !xml->hasTagName(apvts.state.getType()))
        return;
    Program p;
    bool hasProg = false;
    if (auto *pg = xml->getChildByName("TREEGONEPROGRAM")) {
        hasProg = programFromXml(*pg, p);
        xml->removeChildElement(pg, true);
    }
    auto state = juce::ValueTree::fromXml(*xml);
    program = juce::jlimit(0, juce::jmax(0, getNumPrograms() - 1), (int)state.getProperty("program", 0));
    uiScale = (float)(double)state.getProperty("uiScale", 0.9);
    uiScaleSet = (bool)state.getProperty("uiScaleSet", false);
    apvts.replaceState(state);
    if (hasProg) {
        for (int i = 0; i < NPARAM; i++)
            p.p[i] = (int16_t)value(i);
        applyProgram(p, false);
    }
}

juce::AudioProcessorEditor *TreegoneProcessor::createEditor() { return new TreegoneEditor(*this); }

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new TreegoneProcessor(); }
