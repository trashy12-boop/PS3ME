/* SPDX-License-Identifier: GPL-2.0-only
 * PS3 file adapter. Overrides the upstream in-ROM generic filesystem.
 */
#include "incls/_precompiled.incl"
#include "incls/_OsFile_generic.cpp.incl"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
extern "C" {
struct OsFile { FILE* stream; };
OsFile_Handle OsFile_open(const PathChar* name,const char* mode) {
    if(!name || !mode) return NULL;
    FILE* stream=fopen(name,mode);
    if(!stream) return NULL;
    OsFile* file=(OsFile*)malloc(sizeof(OsFile));
    if(!file) { fclose(stream); return NULL; }
    file->stream=stream;return file;
}
int OsFile_close(OsFile_Handle file) {
    if(!file)return EOF;
    int status=fclose(file->stream);free(file);return status;
}
int OsFile_flush(OsFile_Handle file) { return file ? fflush(file->stream) : EOF; }
size_t OsFile_read(OsFile_Handle file,void* data,size_t size,size_t count) {
    return file && data ? fread(data,size,count,file->stream) : 0;
}
size_t OsFile_write(OsFile_Handle file,const void* data,size_t size,size_t count) {
    return file && data ? fwrite(data,size,count,file->stream) : 0;
}
long OsFile_length(OsFile_Handle file) {
    if(!file)return -1;
    long position=ftell(file->stream);
    if(position<0 || fseek(file->stream,0,SEEK_END)!=0)return -1;
    long length=ftell(file->stream);
    if(fseek(file->stream,position,SEEK_SET)!=0)return -1;
    return length;
}
bool OsFile_exists(const PathChar* name) { struct stat status;return name && stat(name,&status)==0; }
long OsFile_seek(OsFile_Handle file,long offset,int origin) {
    return file ? fseek(file->stream,offset,origin) : -1;
}
int OsFile_error(OsFile_Handle file) { return file ? ferror(file->stream) : 1; }
int OsFile_eof(OsFile_Handle file) { return file ? feof(file->stream) : 1; }
bool OsFile_rename(const char* from,const char* to) { return from && to && rename(from,to)==0; }
int OsFile_remove(const char* name) { return name ? remove(name) : -1; }
}
