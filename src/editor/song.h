#ifndef PT_SONG_H
#define PT_SONG_H
#include "document.h"
#include "pattern.h"
struct pt_song_storage;
/* Owns only replacement order/event arrays. The original document must outlive
 * editing; disposal may follow document replacement and never reads its data.
 * Release journal resources before this owner. Active arrays cease to be valid
 * on release. Allocation is lazy, checked against a separate caller budget. */
struct pt_song {
    struct pt_allocator allocator;
    struct pt_song_storage *current;
    size_t bytes,budget;
    unsigned generation;
};
void pt_song_init(struct pt_song *,const struct pt_allocator *,size_t);
void pt_song_release(struct pt_song *);
/* Append a position referencing an existing pattern, or append a new empty
 * pattern and a position referencing it. All share chronological note undo. */
enum pt_edit_result pt_song_append(struct pt_song *,struct pt_project *,struct pt_pattern_history *,unsigned,int);
enum pt_edit_result pt_song_assign(struct pt_song *,struct pt_project *,struct pt_pattern_history *,unsigned,unsigned);
#endif
