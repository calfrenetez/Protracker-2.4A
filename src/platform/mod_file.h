#ifndef PT_MOD_FILE_H
#define PT_MOD_FILE_H
#include "file_save.h"
#include "mod_project.h"
/* Explicit direct/round8/TPDF policy; bounded immutable master export. */
enum pt_save_result pt_mod_file_save(const char *,const struct pt_project *,unsigned,const struct pt_allocator *);
#endif
