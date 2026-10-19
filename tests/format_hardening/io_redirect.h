#include "io_faults.h"
#define open fh_open
#define read fh_read
#define write fh_write
#define lseek fh_lseek
#define fsync fh_fsync
#define close fh_close
#define link fh_link
#define unlink fh_unlink
