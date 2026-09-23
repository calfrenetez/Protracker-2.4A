/* Link with render_file.c compiled using -Dread=pt_test_read. */
#ifdef read
#undef read
#endif
#include <unistd.h>
#include <errno.h>
/* Exercise every verification with repeated EINTR and short reads, including
 * the final EOF probe. Delegation still reads real staged bytes. */
ssize_t pt_test_read(int fd,void *buffer,size_t count)
{
    static unsigned calls;
    if(++calls%3==1) {errno=EINTR;return -1;}
    return read(fd,buffer,count>7?7:count);
}
