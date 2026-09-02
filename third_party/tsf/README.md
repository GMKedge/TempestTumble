# TinySoundFont (vendored)

Header-only SoundFont renderer (`tsf.h`, MIT) and MIDI loader (`tml.h`, zlib)
from https://github.com/schellingb/TinySoundFont — raw files only, not a git
clone of that repo.

`Audio.cpp` defines `TSF_IMPLEMENTATION` and `TML_IMPLEMENTATION`. Music is
`.mid` + `assets/music/gm.sf2`; miniaudio still plays WAV sound effects.
