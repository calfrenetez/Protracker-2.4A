#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <proto/dos.h>
static void check(const char *name,const char *text)
{char buf[8];FILE *f=fopen(name,"rb");assert(f);assert(fread(buf,1,8,f)==8);assert(!memcmp(buf,text,8));assert(!fclose(f));}
int main(void)
{
    int fd;FILE *f;BPTR lock=CreateDir((STRPTR)"reservation.fixture");
    assert(lock);UnLock(lock);assert(!CreateDir((STRPTR)"reservation.fixture"));
    assert(IoErr()==ERROR_OBJECT_EXISTS);
    fd=open("reservation.fixture/data",O_WRONLY|O_CREAT|O_TRUNC,0600);assert(fd>=0);
    assert(write(fd,"ORIGINAL",8)==8);assert(!close(fd));
    check("reservation.fixture/data","ORIGINAL");
    f=fopen("rename.fixture","wb");assert(f);assert(fwrite("CANDIDAT",1,8,f)==8);assert(!fclose(f));
    assert(!Rename((STRPTR)"rename.fixture",(STRPTR)"reservation.fixture/data"));
    check("reservation.fixture/data","ORIGINAL");check("rename.fixture","CANDIDAT");
    assert(!unlink("reservation.fixture/data"));assert(!unlink("rename.fixture"));assert(DeleteFile((STRPTR)"reservation.fixture"));
    puts("FILE SAFETY PASS: exclusive-directory reservation and DOS Rename refuse existing destinations without modifying them");return 0;
}
