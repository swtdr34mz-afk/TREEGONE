/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
/* The editor's size: kept with the project (the processor's uiScale), and the last size used anywhere as the start
 * for a new instance - so the window opens as large as it was left, also after closing and reopening the DAW. */
#include <juce_core/juce_core.h>

namespace winsize
{
inline juce::File file(const juce::String &plugin)
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile(plugin).getChildFile("window.txt");
}
/* the scale last used in any instance, or def */
inline double last(const juce::String &plugin, double def)
{
    auto f = file(plugin);
    if (!f.existsAsFile())
        return def;
    double s = f.loadFileAsString().trim().getDoubleValue();
    return s > 0.1 && s < 4.0 ? s : def;
}
inline void remember(const juce::String &plugin, double s)
{
    if (s <= 0.1 || s >= 4.0)
        return;
    auto f = file(plugin);
    f.getParentDirectory().createDirectory();
    f.replaceWithText(juce::String(s, 4));
}
} // namespace winsize
