/* SPDX-License-Identifier: GPL-2.0-only */
/* Exercise the actual MIDP reader/inflater with host stdio, not a substitute. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jar.h>
static long size(void* f) { long n,p=ftell(f); fseek(f,0,SEEK_END); n=ftell(f); fseek(f,p,SEEK_SET); return n; }
static long read_bytes(void* f,unsigned char* b,long n) { return fread(b,1,n,f); }
static int seek_bytes(void* f,long n,int whence) { return fseek(f,n,whence); }
static int read_char(void* f) { return fgetc(f); }
static void* alloc(void* s,int n) { (void)s; return malloc(n); }
static void release(void* s,void* p) { (void)s; free(p); }
static void* address(void* s,void* p) { (void)s; return p; }
int main(int argc,char** argv) {
    FILE* f; FileObj file; HeapManObj heap; JarInfo jar; JarEntryInfo entry;
    unsigned char* data; unsigned char* compare; int status;
    if(argc!=3) return 2;
    f=fopen(argv[1],"rb"); if(!f) return 3;
    file=(FileObj){f,size,read_bytes,seek_bytes,read_char};
    heap=(HeapManObj){NULL,alloc,release,address};
    jar=getJarInfo(&file); if(jar.status) { fclose(f);return 4; }
    compare=malloc(strlen(argv[2])+1);
    entry=findJarEntryInfo(&file,&jar,(unsigned char*)argv[2],strlen(argv[2]),compare);
    free(compare);
    if(entry.status) { fclose(f);return 5; }
    data=malloc(entry.decompLen+1);
    status=inflateJarEntry(&file,&heap,&entry,data,0);
    if(!status) fwrite(data,1,entry.decompLen,stdout);
    else fprintf(stderr,"MIDP JAR status: %d\n",status);
    free(data);fclose(f);return status?6:0;
}
