#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <pt_host_intuition.h>
#include "../src/native/recovery_requester.h"
/* Actual controller/binder, with deterministic DOS/stat boundaries, as in the
 * existing preferences fixture. No ENV/source/recovery path is accessed. */
static int fixture_stat(const char *,struct stat *);
#define stat(path,info) fixture_stat(path,info)
#include "../src/native/recovery.c"
#undef stat
#define MAX_MESSAGES 64
static struct IntuiMessage messages[MAX_MESSAGES];
static unsigned count,next_message,current_message,replied[MAX_MESSAGES];
static unsigned font_opens,font_closes,window_opens,window_closes,waits,blits;
static unsigned paints_enabled,paints_refused,refresh_begin,refresh_end;
static unsigned env_calls,directory_locks,unlocks;
static int fail_font,bad_font,fail_window;
static struct Screen screen={1};static struct RastPort rast={1};
static struct MsgPort child_port={4},parent_port={6};
static struct Window parent={&screen,&rast,&parent_port,4,10,0};
static struct Window child={&screen,&rast,&child_port,4,10,0};
static struct TextFont font={8,8};
static void before_policy(void)
{if(current_message<count)assert(replied[current_message]==1);}
LONG GetVar(STRPTR name,STRPTR out,LONG capacity,ULONG flags)
{(void)name;(void)out;assert(capacity>1 && flags==(GVF_GLOBAL_ONLY|GVF_BINARY_VAR));
 before_policy();++env_calls;return -1;}
LONG IoErr(void) {before_policy();return ERROR_OBJECT_NOT_FOUND;}
BPTR Lock(STRPTR name,LONG access)
{before_policy();assert(access==ACCESS_READ && !strcmp(name,"Work:Recovery"));++directory_locks;return 1;}
void UnLock(BPTR lock) {before_policy();assert(lock==1);++unlocks;}
LONG Examine(BPTR lock,struct FileInfoBlock *info)
{before_policy();assert(lock==1);info->fib_DirEntryType=1;return 1;}
LONG NameFromLock(BPTR lock,STRPTR out,LONG capacity)
{before_policy();assert(lock==1 && capacity>14);strcpy(out,"Work:Recovery");return 1;}
LONG ExNext(BPTR lock,struct FileInfoBlock *info) {(void)lock;(void)info;abort();}
LONG AddPart(STRPTR path,STRPTR name,LONG capacity) {(void)path;(void)name;(void)capacity;abort();}
static int fixture_stat(const char *path,struct stat *info) {(void)path;(void)info;abort();}
static void *allocate(void *context,size_t bytes) {(void)context;(void)bytes;abort();}
static void release(void *context,void *memory) {(void)context;(void)memory;abort();}
static int quiet(void *context) {return *(int *)context;}
static void reset(void)
{
 memset(messages,0,sizeof(messages));memset(replied,0,sizeof(replied));count=next_message=0;
 current_message=UINT_MAX;font_opens=font_closes=window_opens=window_closes=waits=blits=0;
 paints_enabled=paints_refused=refresh_begin=refresh_end=0;
 fail_font=bad_font=fail_window=0;env_calls=directory_locks=unlocks=0;font.tf_XSize=font.tf_YSize=8;
}
static void initialize(struct pt_native_recovery *r)
{
 struct pt_allocator a={NULL,allocate,release};struct pt_native_recovery_configuration c;
 memset(r,0,sizeof(*r));assert(!pt_native_recovery_configure(r,&a));
 assert(pt_native_recovery_get_configuration(r,&c));assert(!c.enabled);
 assert(pt_native_recovery_apply_configuration(r,&c));
}
static void event(ULONG kind,UWORD code,UWORD qualifier,int x,int y)
{
 struct IntuiMessage *m;assert(count<MAX_MESSAGES);m=&messages[count];
 m->ExecMessage.identity=count;m->Class=kind;m->Code=code;m->Qualifier=qualifier;
 m->MouseX=x+child.BorderLeft;m->MouseY=y+child.BorderTop;++count;
}
static void character(UWORD code) {event(IDCMP_VANILLAKEY,code,0,0,0);}
static void click(unsigned focus)
{
 const struct pt_recovery_requester_rect *r=&pt_recovery_requester_controls[focus];
 event(IDCMP_MOUSEBUTTONS,SELECTDOWN,0,r->left+3,r->top+3);
 event(IDCMP_MOUSEBUTTONS,SELECTUP,0,r->left+3,r->top+3);
}
struct TextFont *OpenFont(struct TextAttr *a)
{assert(!strcmp(a->ta_Name,"topaz.font") && a->ta_YSize==8);++font_opens;
 if(fail_font)return NULL;
 if(bad_font)font.tf_XSize=9;
 return &font;}
void CloseFont(struct TextFont *f) {assert(f==&font);++font_closes;}
struct Window *OpenWindowTags(void *a,...)
{assert(!a);++window_opens;return fail_window?NULL:&child;}
void CloseWindow(struct Window *w)
{unsigned i;assert(w==&child && next_message==count);for(i=0;i<count;++i)assert(replied[i]==1);
 assert(blits==1);++window_closes;}
ULONG Wait(ULONG signal)
{assert(signal==((ULONG)1<<child_port.mp_SigBit));assert(next_message<count);++waits;return signal;}
struct Message *GetMsg(struct MsgPort *p)
{assert(p==&child_port);if(next_message==count)return NULL;
 current_message=next_message++;return &messages[current_message].ExecMessage;}
void ReplyMsg(struct Message *m)
{unsigned id=m->identity;assert(id<count && !replied[id]);replied[id]=1;}
void WaitBlit(void) {++blits;}
void SetFont(struct RastPort *p,struct TextFont *f) {assert(p==&rast && f==&font);}
void Move(struct RastPort *p,int x,int y) {(void)x;(void)y;assert(p==&rast);}
void Draw(struct RastPort *p,int x,int y) {(void)x;(void)y;assert(p==&rast);}
void Text(struct RastPort *p,STRPTR s,ULONG n)
{
 assert(p==&rast && n==strlen(s));
 if(!strcmp(s,"[X] Recovery enabled"))++paints_enabled;
 if(!strcmp(s,"Apply refused. Check values/path; current settings are kept."))++paints_refused;
}
void SetAPen(struct RastPort *p,unsigned n) {(void)n;assert(p==&rast);}
void SetBPen(struct RastPort *p,unsigned n) {(void)n;assert(p==&rast);}
void SetDrMd(struct RastPort *p,unsigned n) {assert(p==&rast && n==JAM2);}
void RectFill(struct RastPort *p,int x,int y,int r,int b) {(void)x;(void)y;(void)r;(void)b;assert(p==&rast);}
void BeginRefresh(struct Window *w) {assert(w==&child);++refresh_begin;}
void EndRefresh(struct Window *w,int complete) {assert(w==&child && complete);++refresh_end;}
static enum pt_recovery_requester_result run(struct pt_native_recovery *r,
 const struct pt_native_recovery_source *s)
{int idle=1;return pt_native_recovery_requester(&parent,r,s,quiet,&idle);}
static void closed(void)
{unsigned i;assert(font_opens==1 && font_closes==1 && window_opens==1 && window_closes==1 && waits==1);
 for(i=0;i<count;++i)assert(replied[i]==1);}
static void admission(void)
{
 struct pt_native_recovery r,before;int no=0;
 reset();initialize(&r);before=r;
 assert(pt_native_recovery_requester(NULL,&r,NULL,quiet,&no)==PT_RECOVERY_REQUESTER_REFUSED);
 assert(pt_native_recovery_requester(&parent,&r,NULL,quiet,&no)==PT_RECOVERY_REQUESTER_REFUSED);
 assert(!font_opens && !window_opens && !memcmp(&r,&before,sizeof(r)));
}
static void construction(void)
{
 struct pt_native_recovery r,before;
 reset();initialize(&r);before=r;fail_font=1;
 assert(run(&r,NULL)==PT_RECOVERY_REQUESTER_UNAVAILABLE && !font_closes && !window_opens);
 assert(!memcmp(&r,&before,sizeof(r)));
 reset();initialize(&r);before=r;bad_font=1;
 assert(run(&r,NULL)==PT_RECOVERY_REQUESTER_UNAVAILABLE && font_closes==1 && !window_opens);
 assert(!memcmp(&r,&before,sizeof(r)));
 reset();initialize(&r);before=r;fail_window=1;
 assert(run(&r,NULL)==PT_RECOVERY_REQUESTER_UNAVAILABLE && font_closes==1 && !window_closes);
 assert(!memcmp(&r,&before,sizeof(r)));
}
static void close_drains(void)
{
 struct pt_native_recovery r,before;
 reset();initialize(&r);before=r;event(IDCMP_CLOSEWINDOW,0,0,0,0);
 character(' ');click(PT_RECOVERY_FOCUS_APPLY);
 assert(run(&r,NULL)==PT_RECOVERY_REQUESTER_CANCELLED);closed();
 assert(!memcmp(&r,&before,sizeof(r)) && !paints_enabled && !directory_locks);
}
static void key_up_and_refresh(void)
{
 struct pt_native_recovery r,before;
 reset();initialize(&r);before=r;
 event(IDCMP_RAWKEY,0xc2,0,0,0);character(' ');event(IDCMP_REFRESHWINDOW,0,0,0,0);character(27);
 assert(run(&r,NULL)==PT_RECOVERY_REQUESTER_CANCELLED);closed();
 assert(paints_enabled==2 && refresh_begin==1 && refresh_end==1 && !memcmp(&r,&before,sizeof(r)));
}
static void releases(void)
{
 struct pt_native_recovery r,before;const struct pt_recovery_requester_rect *a=&pt_recovery_requester_controls[PT_RECOVERY_FOCUS_APPLY];
 reset();initialize(&r);before=r;
 event(IDCMP_MOUSEBUTTONS,SELECTDOWN,0,20,20);event(IDCMP_MOUSEBUTTONS,SELECTUP,0,0,0);
 event(IDCMP_MOUSEBUTTONS,SELECTUP,0,20,20);event(IDCMP_MOUSEBUTTONS,SELECTDOWN,0,20,20);
 event(IDCMP_MOUSEBUTTONS,SELECTUP,0,20,20);
 event(IDCMP_MOUSEBUTTONS,SELECTDOWN,0,a->left+3,a->top+3);character(9);
 event(IDCMP_MOUSEBUTTONS,SELECTUP,0,a->left+3,a->top+3);
 event(IDCMP_REFRESHWINDOW,0,0,0,0);character(27);
 assert(run(&r,NULL)==PT_RECOVERY_REQUESTER_CANCELLED);closed();
 assert(paints_enabled>0 && !directory_locks && !memcmp(&r,&before,sizeof(r)));
 assert(refresh_begin==1 && refresh_end==1);
}
static void refused_apply(void)
{
 struct pt_native_recovery r,before;
 reset();initialize(&r);before=r;character(9);character(9);character(21);character('1');
 click(PT_RECOVERY_FOCUS_APPLY);character(9);character(13);
 assert(run(&r,NULL)==PT_RECOVERY_REQUESTER_CANCELLED);closed();
 assert(paints_refused>0 && !memcmp(&r,&before,sizeof(r)));
}
static void noop_drains(void)
{
 struct pt_native_recovery r,before;unsigned env;
 reset();initialize(&r);before=r;env=env_calls;click(PT_RECOVERY_FOCUS_APPLY);
 character(' ');character(9);event(IDCMP_CLOSEWINDOW,0,0,0,0);
 assert(run(&r,NULL)==PT_RECOVERY_REQUESTER_NOOP);closed();
 assert(!memcmp(&r,&before,sizeof(r)) && env_calls==env && !directory_locks && !paints_enabled);
}
static void changed_apply(void)
{
 struct pt_native_recovery r;struct pt_native_recovery_configuration c;
 struct pt_native_recovery_source source={0},before;const char *text="Work:Recovery";unsigned i;
 reset();initialize(&r);assert(pt_native_recovery_source_commit(&source,PT_NATIVE_RECOVERY_SOURCE_NEW,1,NULL,0));before=source;
 character(' ');character(9);character(21);for(i=0;text[i];++i)character((UWORD)text[i]);
 character(9);character(21);character('6');character('0');character(9);character(' ');
 click(PT_RECOVERY_FOCUS_APPLY);character(' ');
 assert(run(&r,&source)==PT_RECOVERY_REQUESTER_APPLIED);closed();
 assert(pt_native_recovery_get_configuration(&r,&c));
 assert(c.enabled && c.interval_seconds==60 && c.media==PT_NATIVE_RECOVERY_MEDIA_FIXED && !strcmp(c.directory,text));
 assert(r.bound && directory_locks==1 && unlocks==1 && !memcmp(&source,&before,sizeof(source)));
}
int main(void)
{
 admission();construction();close_drains();key_up_and_refresh();releases();refused_apply();noop_drains();changed_apply();
 puts("RECOVERY REQUESTER LOOP HOST PASS: 8 groups; actual renderer/controller with deterministic owned-port stubs");
 puts("NOT TESTED: real Intuition ABI/input, parent IDCMP isolation, main timer, emulator or A1200");return 0;
}
