#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 swtdr34mz (TREEGONE)
"""TREEGONE for the MPC: builds the plugin folder with Akai's MPC Plugin SDK.

The MPC SDK builds a project of C++ files in one src/ folder. This script makes that project from this repository,
so the MPC plugin always has the same engine as the VST3 (nothing copied by hand, nothing in two places):

  1. mpc/build/TREEGONE/      the SDK project: plugin.json, README.md and skins/ (the VST3's panel as the MPC's screen:
                            mpc/skin/) from mpc/plugin/, and in src/ mpc/plugin/src/* and the engine (dsp/)
  2. the SDK's tools/build.py   native build + host simulator, arm32 and arm64, the skins, version.xml, validation

    python3 mpc/build.py --sdk /path/to/mpc-plugin-builder-sdk [--target native|arm32|arm64|all] [--toolchain zig|gcc]

The result: mpc/build/TREEGONE/build/swtdr34mz - VST - TREEGONE/ (copy it into "VST2 Plugins" on a drive of the MPC).
"""
import argparse
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
NAME = "TREEGONE"
ENGINE = ["dsp/Engine.cpp", "dsp/Engine.h", "dsp/Params.cpp", "dsp/Params.h", "dsp/Presets.cpp", "dsp/Presets.h",
          "dsp/Dsp.h", "dsp/Fx.h"]


def run(argv, **kw):
    print("+ " + " ".join('"%s"' % a if " " in a else a for a in argv), flush=True)
    subprocess.check_call(argv, **kw)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--sdk", default=os.environ.get("MPC_SDK", os.path.join(ROOT, "..", "mpc-plugin-builder-sdk")),
                    help="the MPC Plugin SDK's folder (default: $MPC_SDK, or beside this repository)")
    ap.add_argument("--target", default="all", help="native, arm32, arm64 or all (the SDK's build.py)")
    ap.add_argument("--toolchain", default=None, help="zig (the SDK's default), gcc or docker")
    ap.add_argument("--no-validate", action="store_true")
    a = ap.parse_args()

    sdk = os.path.abspath(a.sdk)
    sdk_build = os.path.join(sdk, "tools", "build.py")
    if not os.path.isfile(sdk_build):
        sys.exit("the MPC Plugin SDK is not at %s (--sdk or MPC_SDK; git clone "
                 "https://github.com/inmusic-software/mpc-plugin-builder-sdk)" % sdk)

    proj = os.path.join(HERE, "build", NAME)
    src = os.path.join(proj, "src")
    if os.path.isdir(src):
        shutil.rmtree(src)
    os.makedirs(src)
    for f in ("plugin.json", "README.md"):
        shutil.copy2(os.path.join(HERE, "plugin", f), os.path.join(proj, f))
    skins = os.path.join(proj, "skins")
    if os.path.isdir(skins):
        shutil.rmtree(skins)
    if os.path.isdir(os.path.join(HERE, "plugin", "skins")):
        shutil.copytree(os.path.join(HERE, "plugin", "skins"), skins)
    for f in os.listdir(os.path.join(HERE, "plugin", "src")):
        shutil.copy2(os.path.join(HERE, "plugin", "src", f), os.path.join(src, f))
    for f in ENGINE:
        shutil.copy2(os.path.join(ROOT, f), os.path.join(src, os.path.basename(f)))

    argv = [sys.executable, sdk_build, proj, "--target", a.target]
    if a.toolchain:
        argv += ["--toolchain", a.toolchain]
    if a.no_validate:
        argv.append("--no-validate")
    run(argv)
    out = os.path.join(proj, "build", "swtdr34mz - VST - " + NAME)
    if os.path.isdir(out):
        print("\nthe plugin folder: %s" % out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
