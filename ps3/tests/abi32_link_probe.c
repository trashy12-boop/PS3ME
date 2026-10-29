/* Development-only ABI probe; not an emulator. */
#include <sys/systime.h>
int main(void) { return sysUsleep(1000); }
