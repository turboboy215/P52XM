# P52XM
Paragon 5 (GBC) to XM converter

This tool converts music from Game Boy Color games using Paragon 5's GBC sound engine to XM (FastTracker II) format. XM was chosen as a destination format rather than MIDI, since this is a tracker-based sound engine, and the format is much closer, similar to GHX.

It works with ROM images. To use it, you must specify the name of the ROM followed by the number of the bank containing the sound data (in hex).
In order to convert all the music from each game, the program must be called multiple times with each bank that contains different sound data. The sequence format is rather "bloated", and sound data for a single game can take as much as 20 banks total (Shantae). At least one game, Project S-11, uses a multi-banked version of the driver and the sound data is spread throughout the ROM.

Examples:
* P52XM "Atlantis - The Lost Empire (U) [C][!].gbc" 58
* P52XM "Atlantis - The Lost Empire (U) [C][!].gbc" 59
* P52XM "Atlantis - The Lost Empire (U) [C][!].gbc" 5A
* ...
* P52XM "Shantae (U) [C][!].gbc" 2D
* P52XM "Shantae (U) [C][!].gbc" 2E
* P52XM "Shantae (U) [C][!].gbc" 2F
* ...
* P52XM "LEGO Stunt Rally (E) (M10) [C][!].gbc" 22
* P52XM "LEGO Stunt Rally (E) (M10) [C][!].gbc" 37
* P52XM "LEGO Stunt Rally (E) (M10) [C][!].gbc" 38

This tool was based on my own reverse-engineering of the file format from the officially released tracker, as well as the source code for the driver. The resulting XM files don't sound "great" since all the instruments are represented by the same sine waveform, and the converted effects do not work the same way as MOD-based trackers including XM work; for example, arpeggio after turned on is always on until it is manually turned off, but in MOD-based formats, it needs to be set every row.

Also included is another program, GHX2TXT, which prints out information about the song data from each game. This is essentially a prototype of P52XM, much like my other converters. Do not delete the xmdata.dat file! It is necessary for XM conversions.

Supported games:
 * Atlantis: The Lost Empire
 * David Beckham Soccer
 * ESPN National Hockey Night
 * European Super League
 * Hands of Time
 * LEGO Stunt Rally
 * M&M's Minis Madness
 * Project S-11
 * Q-bert
 * RoboCop (GBC)
 * Shantae
 * Tom and Jerry: Mouse Hunt
 * Trouballs
 * Who Wants to Be a Millionnaire?: 2nd Edition

## To do:
  * Support for the GBA version of the driver
  * GBS file support
