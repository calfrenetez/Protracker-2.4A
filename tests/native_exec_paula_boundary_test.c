/* First musical boundary with actual strict clocks and only zero24 PCM.
 * Separate scoped-priority diagnostic; no production scheduling policy. */
#define PT_NATIVE_PAULA_WAIT_SCOPED_PRIORITY 5
#define PT_NATIVE_PAULA_BOUNDARY_ONLY 1
#include "native_exec_paula_wait_test.c"
