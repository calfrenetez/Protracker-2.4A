#ifndef PT_FILE_REQUEST_H
#define PT_FILE_REQUEST_H
#include <stddef.h>
struct Window;
/* saving: 0 load, 1 new project, 2 new classic MOD, 3 load WAV/IFF, 4 new WAV, 5 new IFF, 6 RAW load, 7 new RAW, 8 MOD sample source, 9 new rendered WAV.
   Returns 1 selected, 0 cancelled, -1 unavailable/invalid path. Output changes only
   after a complete bounded path has been obtained. */
int pt_file_request(struct Window *,int saving,const char *initial,char *path,size_t capacity);
#endif
