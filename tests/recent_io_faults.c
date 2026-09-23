#undef read
#undef write
#include <unistd.h>
#include <errno.h>
ssize_t pt_test_read(int fd,void *p,size_t n)
{static unsigned calls;if(++calls%3==1){errno=EINTR;return -1;}return read(fd,p,n>7?7:n);}
int pt_test_write_failure;
ssize_t pt_test_write(int fd,const void *p,size_t n)
{static unsigned calls;if(pt_test_write_failure && --pt_test_write_failure==0){errno=EIO;return -1;}if(++calls%3==1){errno=EINTR;return -1;}return write(fd,p,n>7?7:n);}
