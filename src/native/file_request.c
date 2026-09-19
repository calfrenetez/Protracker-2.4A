#include <libraries/asl.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/asl.h>
#include <string.h>
#include "file_request.h"
struct Library *AslBase;
int pt_file_request(struct Window *window,int saving,const char *initial,char *path,size_t capacity)
{
    struct FileRequester *request=NULL;char drawer[1024],chosen[1024],file[108];
    const char *part;size_t prefix;int result=-1;
    if(!initial)initial="";
    if(strlen(initial)>=sizeof(drawer))return -1;
    part=(const char *)FilePart((STRPTR)initial);prefix=(size_t)(part-initial);
    if(strlen(part)>=sizeof(file))return -1;
    memcpy(drawer,initial,prefix);drawer[prefix]=0;strcpy(file,part);
    AslBase=OpenLibrary("asl.library",39);if(!AslBase)return -1;
    request=AllocAslRequestTags(ASL_FileRequest,
        ASLFR_Window,(ULONG)window,ASLFR_PrivateIDCMP,TRUE,ASLFR_SleepWindow,TRUE,
        ASLFR_TitleText,(ULONG)(saving==2?"Export classic MOD to a NEW file":saving?"Save ProTracker project to a NEW file":"Load ProTracker MOD or project"),
        ASLFR_PositiveText,(ULONG)(saving?"Save new":"Load"),
        ASLFR_InitialDrawer,(ULONG)drawer,ASLFR_InitialFile,(ULONG)file,
        ASLFR_InitialLeftEdge,70,ASLFR_InitialTopEdge,80,
        ASLFR_InitialWidth,500,ASLFR_InitialHeight,340,
        ASLFR_DoSaveMode,saving,ASLFR_RejectIcons,TRUE,TAG_DONE);
    if(!request)goto done;
    if(!AslRequest(request,NULL)) {result=0;goto done;}
    if(!request->fr_File[0] || strlen((const char *)request->fr_Drawer)>=sizeof(chosen))goto done;
    strcpy(chosen,(const char *)request->fr_Drawer);
    if(!AddPart((STRPTR)chosen,request->fr_File,sizeof(chosen)) || strlen(chosen)>=capacity)goto done;
    strcpy(path,chosen);result=1;
done:
    if(request)FreeAslRequest(request);
    CloseLibrary(AslBase);AslBase=NULL;return result;
}
