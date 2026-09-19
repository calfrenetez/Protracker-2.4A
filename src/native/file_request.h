#ifndef PT_FILE_REQUEST_H
#define PT_FILE_REQUEST_H
#include <stddef.h>
struct Window;
/* saving: 0 load, 1 new project, 2 new classic MOD.
   Returns 1 selected, 0 cancelled, -1 unavailable/invalid path. Output changes only
   after a complete bounded path has been obtained. */
int pt_file_request(struct Window *,int saving,const char *initial,char *path,size_t capacity);
#endif
