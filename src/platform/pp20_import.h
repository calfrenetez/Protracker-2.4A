#ifndef PT_PP20_IMPORT_H
#define PT_PP20_IMPORT_H
#include "document.h"
int pt_pp20_file_candidate(const char *);
enum pt_project_result pt_pp20_file_load(struct pt_document *,const char *,size_t,size_t);
#endif
