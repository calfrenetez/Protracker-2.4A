#ifndef PT_WAVETABLE_PROBE_H
#define PT_WAVETABLE_PROBE_H
enum pt_wavetable_probe_mode {
    PT_WAVETABLE_GLOBAL_STATUS,
    PT_WAVETABLE_SELECTED_CONTROL,
    PT_WAVETABLE_SELECTED_VOICE
};

int pt_wavetable_probe(enum pt_wavetable_probe_mode);
#endif
