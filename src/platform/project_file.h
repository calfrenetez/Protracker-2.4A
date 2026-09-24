#ifndef PT_PROJECT_FILE_H
#define PT_PROJECT_FILE_H
#include "file_save.h"
#include "project.h"
/* Immutable synchronous master save; bounded heap and core stack workspace.
 * Computes format CRC and verifies exact bytes before no-replace publication. */
enum pt_save_result pt_project_file_save(const char *,const struct pt_project *,const struct pt_allocator *);
#endif
