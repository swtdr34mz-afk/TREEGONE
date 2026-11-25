#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 swtdr34mz (TREEGONE)
"""The MPC skin (Plugin Skins/: TUI.json, Q-Links.json and the pictures) from the renderer's output.

    python3 mpc/skin/make_skin.py <render folder> mpc/plugin/skins

The renderer (render_skin.cpp) draws each MPC page with the VST3's own art and cuts every control out at every value;
this places each picture where it was cut from, bound to the MPC plugin's parameter:

  knobs      film strips (a frame a value), dragged; a double tap opens the MPC's editor (its list for a choice)
  buttons    a picture for each value; a tap switches it (or steps to the next value: TYPE, SAW / REV, KEYBOARD, A / B)
  LEDs       each a radio button (the octaves' round their knobs, the lists beside SHAPE, OCTAVES, MODE, TYPE)
  displays   film strips that show a parameter's value (the effects' 7-segment displays, the red values under knobs)

FX has two sub-pages, FX A and FX B: the FX function key pressed again switches (each its own Q-Links).

The pages (the function keys) as the Trigon-6's panel: OSC, FLT/AMP, LFO/PM, FX (A, B), AT/ARP, PLAY. On each the 16 Q-Links turn up to four groups,
the top row of Q-Links the first group and so on down. Standard library only (Pillow, if there, makes the pictures
smaller).
"""
import json
import os
import shutil
import sys

# The MPC plugin's parameters (mpc/plugin/src/TreegoneMpc.cpp): 0 VOLUME, the program's first 102, the plugin's own
# (voices, quality, hold), then the program's later ones (LOW SPLIT).
IDS = """o1_oct o1_tri o1_saw o1_pulse o1_pw o1_level sync o2_oct o2_pitch o2_tri o2_saw o2_pulse o2_pw o2_level o3_oct o3_pitch o3_tri o3_saw o3_pulse o3_pw o3_level o3_key noise fdbk_drive f_type cutoff reso f_env_amt pole2 f_key f_vel svf_mode f_att f_dec f_sus f_rel a_att a_dec a_sus a_rel a_vel lfo_freq lfo_sync lfo_shape lfo_amt lfo_f1 lfo_f2 lfo_f3 lfo_pw1 lfo_pw2 lfo_pw3 lfo_filt lfo_amp at_amt at_f1 at_f2 at_f3 at_filt at_amp at_lfo at_fxa at_fxb pm_osc3 pm_fenv pm_f1 pm_f2 pm_f3 pm_filt pm_pw1 pm_pw2 pm_fdbk glide_rate glide glide_mode unison uni_voices uni_detune key_mode bpm divide fx_on fxa_type fxa_mix fxa_p1 fxa_p2 fxa_sync fxa_div fxb_type fxb_mix fxb_p1 fxb_p2 fxb_sync fxb_div arp_on arp_oct arp_mode seq_on distortion vintage pan_spread pgm_vol pb_range split_on split_point split_oct
split_glide split_vol split_extra""".split()
assert len(IDS) == 108
NP = len(IDS)
V1 = 102                                 # 1.0's parameters keep their numbers; the later ones follow the globals
GLOBAL = {"vol": 0, "voices": 1 + V1, "quality": 2 + V1, "hold": 3 + V1, "fxsel": 4 + NP}
NPARAMS = 5 + NP


def handle(pid):
    return "Parameter %d" % mpc(pid)


def mpc(pid, layer=0):
    """the MPC index of a page's id"""
    if pid in GLOBAL:
        return GLOBAL[pid]
    i = IDS.index(pid)
    return 1 + i if i < V1 else 4 + i


# The pages (the renderer's names) and their Q-Links' groups top down (a group as (ids, False): not framed)
PAGES = [
    ("OSC", [["o1_oct", "o1_pw", "o1_level", "fdbk_drive"],
             ["o2_oct", "o2_pitch", "o2_pw", "o2_level"],
             ["o3_oct", "o3_pitch", "o3_pw", "o3_level"],
             (["noise"], False)]),
    ("FLT/AMP", [["cutoff", "reso", "f_env_amt", "svf_mode"],
                 ["f_att", "f_dec", "f_sus", "f_rel"],
                 ["a_att", "a_dec", "a_sus", "a_rel"]]),
    ("LFO/PM", [["lfo_freq", "lfo_shape", "lfo_amt"],
                ["pm_osc3", "pm_fenv"]]),
    ("FX A", [["fxa_type", "fxa_mix", "fxa_p1", "fxa_p2"],
              ["fxa_div"]]),
    ("FX B", [["fxb_type", "fxb_mix", "fxb_p1", "fxb_p2"],
              ["fxb_div"]]),
    ("AT/ARP", [["at_amt"],
                ["arp_oct", "arp_mode"],
                ["divide"]]),
    ("PLAY", [["distortion", "vintage", "pan_spread", "pgm_vol"],
              ["glide_mode", "uni_voices", "uni_detune", "vol"],
              ["split_point", "split_oct", "split_glide", "split_vol"],
              (["glide_rate", "pb_range", "key_mode"], False)]),
]
# Program (Track) mode: the knobs as the MPC's own program knobs, bottom left Q-Link 1
PROGRAM_QLINKS = ["fxb_mix", "lfo_amt", "vintage", "vol",
                  "a_att", "a_dec", "a_sus", "a_rel",
                  "f_att", "f_dec", "f_sus", "f_rel",
                  "cutoff", "reso", "f_env_amt", "fdbk_drive"]


class O(list):
    """A JSON object kept in order, with repeated keys (the overlays' two "version"s)."""
    def __init__(self, *pairs):
        super().__init__(pairs)


def dump(v, ind=0):
    sp = "  " * ind
    if isinstance(v, O):
        if not v:
            return "{}"
        return "{\n" + ",\n".join("%s  %s: %s" % (sp, json.dumps(k), dump(x, ind + 1)) for k, x in v) + "\n" + sp + "}"
    if isinstance(v, list):
        if not v:
            return "[]"
        if all(not isinstance(x, (O, list)) for x in v):
            return "[" + ", ".join(json.dumps(x) for x in v) + "]"
        return "[\n" + ",\n".join(sp + "  " + dump(x, ind + 1) for x in v) + "\n" + sp + "]"
    return json.dumps(v)


def box(r):
    return "%d %d %d %d" % tuple(r)


def image_data(img):
    return O(("version", 2), ("imageType", "Regular"), ("colour", "0"), ("image", img))


def bounds(r, focus="No", hide_handles=None, show="Show"):
    if hide_handles:
        return O(("version", 2), ("acceptsHWFocus", focus), ("showWhenDataModelInvalid", show), ("whenVisible", "Always"),
                 ("boundsType", "Absolute"), ("bounds", box(r)), ("additionalInvalidatingHandles", hide_handles))
    return O(("version", 1), ("acceptsHWFocus", focus), ("showWhenDataModelInvalid", show), ("whenVisible", "Always"),
             ("boundsType", "Absolute"), ("bounds", box(r)))


def entry(name, typ, data, b, remap=None):
    return O(("version", 2),
             ("componentData", O(("version", 1), ("name", name), ("type", typ), ("data", data))),
             ("handle remapping", O(("version", 1), ("map", [O(("key", k), ("value", v)) for k, v in (remap or [])]))),
             ("bounds", b))


def action(on, handler, handle, extra):
    return O(("version", 1), ("onAction", on), ("handler", handler), ("handleName", handle), ("additionalData", extra))


def component(actions, children, ignore_mouse=False):
    clear = O(("version", 1), ("colour", "0"), ("image", ""))
    return O(("version", 2), ("actions", actions),
             ("backgroundData", O(("version", 1), ("focussed", clear), ("unfocussed", clear))),
             ("ignoreMousePresses", ignore_mouse), ("disableCoarseDataWheel", False), ("componentsData", children))


FOCUS_LOST = O(("version", 1), ("onAction", "Focus Lost"), ("handler", "Focus Lost"), ("handleName", "Focus Lost"),
               ("additionalData", ""))


def overlay(typ, data):
    return component([], [entry("overlay", typ, data, bounds((0, 0, 1280, 800), "Yes"))], ignore_mouse=True)


def range_handle(a, b, steps, h):
    return ("IndexedEnabling/%d/%d/%s" % (a, steps, h)) if a == b else ("RangedEnabling/%d/%d/%d/%s" % (a, b, steps, h))


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    src, out = argv[1], argv[2]
    m = json.load(open(os.path.join(src, "manifest.json")))
    pages = {p["name"]: p for p in m["pages"]}
    for p in m["pages"]:
        for c in p["controls"]:
            if c["id"] not in GLOBAL and c["id"] not in IDS:
                sys.exit("%s: the page's %s is not one of the MPC plugin's parameters" % (p["name"], c["id"]))

    if os.path.isdir(out):
        shutil.rmtree(out)
    os.makedirs(out)
    used = set()

    def use(f):
        used.add(f)
        return f

    def knob_overlay(bipolar):
        return overlay("KnobOverlay", O(("version", 2), ("version", 1), ("actions", [FOCUS_LOST]), ("overlayType", "Knob With Header"),
                                        ("scaleType", "Linear"), ("polarityType", "Bipolar" if bipolar else "Unipolar"),
                                        ("showMinusSign", True), ("handleName", "Data")))
    defs = [("knob overlay", knob_overlay(False)), ("knob overlay bipolar", knob_overlay(True)),
            ("menu overlay", overlay("MenuOverlay", O(("version", 1), ("version", 1), ("actions", [FOCUS_LOST]),
                                                      ("withNumbers", False), ("handleName", "Data"))))]
    defined = set()
    focus = O(("version", 1), ("backgroundColour", "19ffffff"), ("outlineColour", "ffe81f40"), ("backgroundInset", 1.0),
              ("outlineThickness", 1.0))
    names = {}                           # the discrete parameters' names by MPC index (replacementStrings)

    def tab_children(page, layer):
        kids = [entry("panel", "Image", image_data(use(page["background"])), bounds((0, 0, 1280, 628)))]
        boxes, legends = [], []

        def fx_hide(c):
            return ["IndexedEnabling/%d/2/%s" % (c["fx"], handle("fxsel"))] if c.get("fx", -1) >= 0 else []

        for i, c in enumerate(page["controls"]):
            h = handle(c["id"])
            if c["names"]:
                names[mpc(c["id"])] = c["names"]
            if c["kind"] == "knob":
                fr = c["frame"]
                kx, ky, w, hh = c["rect"]
                ox, oy = kx - fr[0], ky - fr[1]
                ov = "menu overlay" if c["names"] else "knob overlay bipolar" if c["bipolar"] else "knob overlay"
                name = "%s knob %d %s" % (page["name"].lower(), i, c["id"])
                knob = O(("version", 3), ("knobType", "FilmStrip"), ("filmStrip", use(c["strip"])), ("numFrames", c["frames"]),
                         ("invert", False), ("dragOrientation", "Vertical"), ("handleName", "Data"))
                defs.append((name, component(
                    [action("Mouse Down", "Q-Link", "Data", ""), action("Double Click", "Show Overlay", "Data", ov),
                     action("Enter Pressed", "Show Overlay", "Data", ov)],
                    [entry("knob", "Knob", knob, bounds((ox, oy, w, hh))),
                     entry("focus", "Focus", focus, O(("version", 1), ("acceptsHWFocus", "No"), ("showWhenDataModelInvalid", "Show"),
                                                      ("whenVisible", "WhenFocussed"), ("boundsType", "Absolute"),
                                                      ("bounds", box((0, 0, fr[2], fr[3])))))])))
                kids.append(entry(c["id"], name, O(("version", 1), ("handleName", "Data")),
                                  bounds(fr, "Yes", fx_hide(c) or None, "Hide"), [("Data", h)]))
                boxes.append((c["id"], fr))
                legends.append((c["id"], fr))
            elif c["kind"] == "display":
                strip = O(("version", 2), ("imageType", "FilmStrip"), ("colour", "0"), ("image", use(c["strip"])),
                          ("numFrames", c["frames"]), ("handleName", h))
                kids.append(entry(c["id"] + " display", "Image", strip, bounds(c["rect"], "No", fx_hide(c) or None, "Hide")))
                boxes.append((c["id"], c["rect"]))   # (a knob's value: in its Q-Link frame with it)
            elif c["kind"] == "button":
                rs = c["ranges"]
                for r in rs:
                    hide = fx_hide(c) + ([range_handle(r["a"], r["b"], c["steps"], h)] if len(rs) > 1 else [])
                    data = O(("version", 2), ("onImage", use(r["image"])), ("offImage", r["image"]), ("buttonId", r["next"]),
                             ("numButtonsInGroup", c["steps"]), ("handleName", h), ("gestureBehaviour", "Instant"))
                    kids.append(entry(c["id"], "Button", data, bounds(c["rect"], "No", hide or None, "Hide")))
                boxes.append((c["id"], c["rect"]))
            else:
                if len(c["rows"]) != c["steps"]:
                    sys.exit("%s: %d LEDs for %d values" % (c["id"], len(c["rows"]), c["steps"]))
                for k, row in enumerate(c["rows"]):
                    data = O(("version", 2), ("onImage", use(row["on"])), ("offImage", use(row["off"])), ("buttonId", k),
                             ("numButtonsInGroup", c["steps"]), ("handleName", h), ("gestureBehaviour", "Instant"))
                    kids.append(entry(c["id"], "Button", data, bounds(row["rect"], "No", fx_hide(c) or None, "Hide")))
                    boxes.append((c["id"], row["rect"]))
        return kids, boxes, legends

    def frame(group, boxes, legends):
        """The Q-Link group's frame: around its controls with their legends, with an even gap that cuts no other."""
        mine = [r for p, r in boxes + legends if p in group]
        if not mine:
            return None
        x0, y0 = min(r[0] for r in mine), min(r[1] for r in mine)
        x1, y1 = max(r[0] + r[2] for r in mine), max(r[1] + r[3] for r in mine)

        def cuts(a, b, c, d):
            for _pid, r in boxes:
                ix = min(c, r[0] + r[2]) - max(a, r[0])
                iy = min(d, r[1] + r[3]) - max(b, r[1])
                inside = r[0] >= a and r[1] >= b and r[0] + r[2] <= c and r[1] + r[3] <= d
                if ix > 0 and iy > 0 and not inside:
                    return r
            return None
        while True:
            r = cuts(x0, y0, x1, y1)
            if r is None:
                break
            x0, y0, x1, y1 = min(x0, r[0]), min(y0, r[1]), max(x1, r[0] + r[2]), max(y1, r[1] + r[3])
        for pad in (6, 5, 4, 3, 2, 1, 0):
            a, b, c, d = max(0, x0 - pad), max(0, y0 - pad), min(1280, x1 + pad), min(628, y1 + pad)
            if cuts(a, b, c, d) is None:
                return (a, b, c - a, d - b)
        return (x0, y0, x1 - x0, y1 - y0)

    tabs, qmaps = [], []
    key = -1
    for name, groups in PAGES:
        page = pages[name]
        sub = 1 if name == "FX B" else 0   # (FX A and B: one function key, pressed again for B)
        if sub == 0:
            key += 1
        for layer in (sub,):
            kids, boxes, legends = tab_children(page, layer)
            rects, q = [], {}
            for row, g in enumerate(groups):
                ids, framed = (g, True) if isinstance(g, list) else g
                if framed:
                    r = frame(ids, boxes, legends)
                    if r:
                        rects.append(r)
                for col, pid in enumerate(ids):
                    q[13 + col - 4 * row] = mpc(pid, layer)
            tabs.append(O(("version", 1), ("tabName", name), ("fnKeyIndex", key),
                          ("fnKeySubIndex", layer), ("componentDefinition", component([], kids)),
                          ("qlinkBoundsData", [box(r) for r in rects])))
            qmaps.append(O(("Tab", key + 1), ("SubTab", layer + 1), ("Bank Direction", "RowTopDown"),
                           ("Q-Links", O(*[("Q-Link %d" % n, q.get(n, -1)) for n in range(1, 17)]))))

    repl = [O(("key", "Parameter %d" % k), ("value", v)) for k, v in sorted(names.items())]
    tui = O(("pageData", O(("version", 1),
                           ("componentDefinitions", O(("version", 2), ("importFiles", []),
                                                      ("localComponentDefinitions", [O(("key", k), ("value", v)) for k, v in defs]))),
                           ("replacementStrings", repl),
                           ("info", O(("version", 1), ("type", "CompleteDescription"))),
                           ("tabs", tabs))))
    ql = O(("version", 4), ("info", O(("version", 1), ("type", "CompleteDescription"))),
           ("Screen Mode Q-Links", O(("version", 4), ("map", qmaps))),
           ("Program Mode Q-Links", O(*[("Q-Link %d" % (n + 1), mpc(pid, 0)) for n, pid in enumerate(PROGRAM_QLINKS)])))
    with open(os.path.join(out, "TUI.json"), "w") as f:
        f.write(dump(tui) + "\n")
    with open(os.path.join(out, "Q-Links.json"), "w") as f:
        f.write(dump(ql) + "\n")
    try:                                 # (256 colours: the pictures a fraction of the size)
        from PIL import Image
    except ImportError:
        Image = None
        print("(no Pillow: the pictures copied as they are, many times larger)")
    for f in sorted(used):
        if Image and f.endswith("_panel.png"):   # (the panels' text smooth: in full colour)
            Image.open(os.path.join(src, f)).convert("RGB").save(os.path.join(out, f), optimize=True)
        elif Image:
            im = Image.open(os.path.join(src, f)).convert("RGB")
            im.quantize(256, method=Image.MEDIANCUT, dither=Image.NONE).save(os.path.join(out, f), optimize=True)
        else:
            shutil.copy2(os.path.join(src, f), os.path.join(out, f))
    print("TUI.json: %d tabs, %d components; Q-Links.json; %d pictures in %s" % (len(tabs), len(defs), len(used), out))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
