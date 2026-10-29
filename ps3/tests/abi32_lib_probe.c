/* Probe standard C linkage against the installed PS3 SDK. */
extern int close(int);
int main(void) { return close(-1); }
