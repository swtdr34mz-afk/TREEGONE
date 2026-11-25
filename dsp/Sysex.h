/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
/* Reading Trigon-6 programs from a MIDI System Exclusive file (.syx): the factory banks Sequential hands out, or a
 * dump of your own. Each message is a program dump (F0 01 <id> 02 <bank> <program> ... F7) or an edit buffer
 * (F0 01 <id> 03 ... F7); the program, 1024 bytes, is packed as DSI packs it (eight MIDI bytes for seven: the first
 * holds the seven top bits). Its bytes: the panel's parameters (0..115), the name (116..135), the poly sequence (six
 * tracks of 64 notes and 64 velocities from 137), the chord memory (108..113).
 *
 * Nothing of a Trigon's programs is in TREEGONE itself: the user's own file is read on the user's own computer. */
#pragma once
#include "Params.h"
#include <cstddef>
#include <vector>

namespace tg
{
struct SyxProgram {
    int bank = 0, number = 0;                 /* the Trigon's place (bank 0..9, program 0..99); -1: an edit buffer */
    Program prog;
};
/* every program in the data (the messages that are not Trigon programs are skipped) */
std::vector<SyxProgram> readSyx(const unsigned char *data, size_t size);
/* one unpacked Trigon program (1024 bytes) as a TREEGONE program */
void fromTrigon(const unsigned char *b, Program &p);
} // namespace tg
