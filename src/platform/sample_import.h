#ifndef PT_SAMPLE_IMPORT_H
#define PT_SAMPLE_IMPORT_H
#include "../editor/editor.h"
/* Bounded native sample dispatch. Unknown formats leave master/history intact. */
enum pt_edit_result pt_editor_sample_file_import(struct pt_editor *,const char *,int,int,int *);
#endif
