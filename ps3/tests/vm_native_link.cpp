// SPDX-License-Identifier: GPL-2.0-only
// Development integration check. It does not contain a MIDP launcher or games.
#include <jvm.h>
#include <jvmspi.h>
#include <stdio.h>
void JVMSPI_PrintRaw(const char* text,int length) { if(text && length>0) fwrite(text,1,length,stdout); }
void JVMSPI_Exit(int) {}
int main(int argc,char** argv) {
    JVM_Initialize();
    char heap[]="=HeapCapacity16M";
    char* options[]={heap};
    if(JVM_ParseOneArg(1,options)<0)return 2;
    if(argc<3) { JVMSPI_PrintRaw("CLDC PPU integration linked; no MIDlet runtime installed.\n",56);return 0; }
    return JVM_Start(argv[1],argv[2],argc-3,argv+3);
}
