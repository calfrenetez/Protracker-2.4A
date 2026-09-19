#ifndef PT_FILE_SAVE_H
#define PT_FILE_SAVE_H
#include "safe_save.h"
/* Verify and atomically publish a NEW file only. Never replaces a destination. */
enum pt_save_result pt_file_save_new(const char *,const void *,size_t);
#endif
