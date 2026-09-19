# PT2.3F Converter Checklist

The authoritative requirements are in `FINAL_SCOPE_2.4G.md`.

Required architecture:
- shared portable conversion core
- in-ProTracker Amiga export
- standalone Amiga utility
- later macOS front end using the same core

Never make the Mac mandatory for conversion.

Test:
- classic-compatible lossless project
- 16/24 -> 8-bit samples
- resampling
- slices
- enhanced loops/panning
- sparse 16->4 voice allocation
- dense bounce-to-four-channel conversion
- MIDI exclusion/captured-audio path
- 31-sample limit conflicts
- generated MOD loading in actual 2.3F
- source 2.4G project remains unchanged
