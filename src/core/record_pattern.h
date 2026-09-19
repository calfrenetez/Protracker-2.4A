#ifndef PT_RECORD_PATTERN_H
#define PT_RECORD_PATTERN_H
#include "record.h"
#include "pattern.h"
struct pt_record_pattern {struct pt_project *project;struct pt_pattern_history *history;};
/* pt_recorder.store adapter for a validated project. Logical song row maps
   through the order list. MIDI tracks only; never overwrites an existing note.
   Existing effect/instrument data is retained unless the input supplies an
   instrument. The new note's velocity replaces prior velocity; slice is cleared.
   Each accepted input is an ordinary undoable pattern command. */
int pt_record_pattern_store(void *,uint32_t,unsigned,const struct pt_event *);
#endif
