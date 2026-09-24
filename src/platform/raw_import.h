#ifndef PT_RAW_IMPORT_H
#define PT_RAW_IMPORT_H
#include "../editor/sampler.h"
/* Bounded file reader into unpublished sampler staging; closes before commit. */
enum pt_edit_result pt_raw_file_import(const char *,size_t,struct pt_sampler *,struct pt_project *,struct pt_pattern_history *,unsigned,const char *,const struct pt_raw_format *);
/* Header sniff only; full parsing remains required by import. */
int pt_wav_file_candidate(const char *);
enum pt_edit_result pt_wav_file_import(const char *,size_t,struct pt_sampler *,struct pt_project *,struct pt_pattern_history *,unsigned,const char *);
#endif
