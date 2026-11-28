/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
/* Saving your own sounds, made sure:
 * - dialogs open inside the plugin's window (a window of their own could open behind the host's plugin window,
 *   unseen, so nothing got saved);
 * - the folder: Documents, or AppData when Windows keeps apps out of Documents (Controlled folder access, OneDrive);
 * - a failed save says so. */
#include <juce_audio_processors/juce_audio_processors.h>
#include <mutex>

namespace pdlg
{
/* Documents if the plugin may write there, else the user's AppData (checked once) */
inline juce::File docsOrAppData()
{
    static juce::File chosen;
    static std::once_flag once;
    std::call_once(once, [] {
        auto docs = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
        auto probe = docs.getChildFile("swtdr34mz-write-test.tmp");
        bool ok = docs.isDirectory() && probe.replaceWithText("ok") && probe.existsAsFile();
        probe.deleteFile();
        chosen = ok ? docs : juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    });
    return chosen;
}

/* the plugin's window: the editor that holds c (or c itself) */
inline juce::Component *host(juce::Component &c)
{
    if (dynamic_cast<juce::AudioProcessorEditor *>(&c) != nullptr)
        return &c;
    if (auto *e = c.findParentComponentOfClass<juce::AudioProcessorEditor>())
        return e;
    return &c;
}

/* put a dialog into the plugin's window, centred (call before enterModalState) */
inline void showInside(juce::Component &owner, juce::Component *w)
{
    auto *h = host(owner);
    h->addAndMakeVisible(w);
    w->setCentrePosition(h->getLocalBounds().getCentre());
    w->toFront(true);
}

/* a message inside the plugin's window, with OK */
inline void message(juce::Component &owner, const juce::String &title, const juce::String &text)
{
    auto *w = new juce::AlertWindow(title, text, juce::MessageBoxIconType::NoIcon);
    w->setLookAndFeel(&owner.getLookAndFeel());
    w->addButton("OK", 0, juce::KeyPress(juce::KeyPress::returnKey));
    showInside(owner, w);
    w->enterModalState(true, nullptr, true);
}

/* a save's result (its program number, -1: failed): a failure is told */
inline int checked(int result, juce::Component &owner)
{
    if (result < 0)
        juce::MessageManager::callAsync([sp = juce::Component::SafePointer<juce::Component>(&owner)] {
            if (sp != nullptr)
                message(*sp, "NOT SAVED", "The sound could not be saved (no name, or the folder is not writable).");
        });
    return result;
}
} // namespace pdlg
