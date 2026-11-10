#pragma once
#include "ControlLogic.h"

// Same order as BRAIDS/src/braids/settings.h; the special '?' model is omitted.
constexpr const char* kModelNames[] = {
    "CSAW", "MORPH", "SAW/SQ", "SIN/TR", "BUZZ",
    "SQ/SUB", "SW/SUB", "SQ/SYN", "SW/SYN", "3/SAW",
    "3/SQ", "3/TRI", "3/SIN", "3/RING", "SWARM", "COMB", "TOY",
    "FILT/LP", "FILT/PK", "FILT/BP", "FILT/HP", "VOSIM", "VOWEL", "V/FOF",
    "HARM", "FM", "FB/FM", "CHAOS", "PLUCK", "BOWED", "BLOWN", "FLUTE",
    "BELL", "DRUM", "KICK", "CYMBAL", "SNARE", "WTBL", "WMAP", "WLINE",
    "W/PARA", "NOISE", "TWIN", "CLK/NS", "CLOUD", "PARTCL", "DIG/MOD"};
static_assert(sizeof(kModelNames) / sizeof(kModelNames[0]) == controls::kModelCount,
              "Each selectable Braids model needs a name");
