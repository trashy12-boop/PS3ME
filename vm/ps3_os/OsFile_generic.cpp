/* SPDX-License-Identifier: GPL-2.0-only
 * Narrow-path PS3 VM filesystem. Read-only JAR/ZIP snapshots avoid repeated
 * seeks/reads during class verification and resource inflation. RMS and
 * writable files keep the existing stdio behavior. Session cache is reset between games.
 * Standalone so recovered prebuilt VMs use the same adapter at link time.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
extern "C" {
struct ArchiveSnapshot {char* path;unsigned char* image;size_t size;time_t modified;unsigned references;bool retired;ArchiveSnapshot* next;};
struct OsFile {FILE* stream;unsigned char* image;size_t size,position;bool eof;ArchiveSnapshot* snapshot;};
typedef OsFile* OsFile_Handle;
static size_t archiveResident=0;
static ArchiveSnapshot* snapshots=0;
static void collectSnapshots(){
 ArchiveSnapshot** link=&snapshots;
 while(*link){ArchiveSnapshot* entry=*link;if(entry->retired&&!entry->references){*link=entry->next;archiveResident-=entry->size;free(entry->image);free(entry->path);free(entry);}else link=&entry->next;}
}
void ps3me_archive_reset(){
 // Retire active snapshots rather than freeing buffers still owned by handles.
 for(ArchiveSnapshot* e=snapshots;e;e=e->next)e->retired=true;
 collectSnapshots();
}
static const size_t archiveLimit=12*1024*1024,archiveFileLimit=8*1024*1024;
static void retirePath(const char* name){
 if(!name)return;
 for(ArchiveSnapshot* e=snapshots;e;e=e->next)if(!strcmp(e->path,name))e->retired=true;
 collectSnapshots();
}
static bool archiveName(const char* name){
 size_t n=strlen(name);return n>=4&&(!strcmp(name+n-4,".jar")||!strcmp(name+n-4,".zip"));
}
OsFile_Handle OsFile_open(const char* name,const char* mode){
 if(!name||!mode)return 0;
 if(strchr(mode,'w')||strchr(mode,'a')||strchr(mode,'+'))retirePath(name);
 FILE* stream=fopen(name,mode);if(!stream)return 0;
 OsFile* f=(OsFile*)calloc(1,sizeof(OsFile));if(!f){fclose(stream);return 0;}f->stream=stream;
 // All handles and the aggregate budget are owned by the single VM thread.
 struct stat st;
 if((!strcmp(mode,"rb")||!strcmp(mode,"r"))&&archiveName(name)&&
    fstat(fileno(stream),&st)==0&&S_ISREG(st.st_mode)&&st.st_size>0&&
    (uint64_t)st.st_size<=archiveFileLimit){
  size_t bytes=(size_t)st.st_size;
  for(ArchiveSnapshot* e=snapshots;e;e=e->next){
   if(e->retired||strcmp(e->path,name))continue;
   if(e->size!=bytes||e->modified!=st.st_mtime){e->retired=true;continue;}
   ++e->references;f->snapshot=e;f->image=e->image;f->size=e->size;fclose(stream);f->stream=0;return f;
  }
  collectSnapshots();
  if(bytes<=archiveLimit-archiveResident){
   ArchiveSnapshot* e=(ArchiveSnapshot*)calloc(1,sizeof(ArchiveSnapshot));
   if(e){e->image=(unsigned char*)malloc(bytes);e->path=(char*)malloc(strlen(name)+1);
    if(e->image&&e->path&&fread(e->image,1,bytes,stream)==bytes){
     strcpy(e->path,name);e->size=bytes;e->modified=st.st_mtime;e->references=1;e->next=snapshots;snapshots=e;
     archiveResident+=bytes;f->snapshot=e;f->image=e->image;f->size=bytes;fclose(stream);f->stream=0;
     fprintf(stderr,"[PS3ME archive] cached %lu bytes: %s\n",(unsigned long)bytes,name);
    }else{free(e->image);free(e->path);free(e);clearerr(stream);if(fseek(stream,0,SEEK_SET)!=0){fclose(stream);free(f);return 0;}}
   }
  }
 }

 return f;
}
int OsFile_close(OsFile_Handle f){if(!f)return EOF;int status=0;if(f->stream)status=fclose(f->stream);if(f->snapshot){--f->snapshot->references;collectSnapshots();}free(f);return status;}
int OsFile_flush(OsFile_Handle f){return !f?EOF:f->stream?fflush(f->stream):0;}
size_t OsFile_read(OsFile_Handle f,void* data,size_t size,size_t count){
 if(!f||!data||!size||!count)return 0;
 if(f->stream)return fread(data,size,count,f->stream);
 if(count>SIZE_MAX/size)return 0;
 size_t requested=size*count,remaining=f->position<f->size?f->size-f->position:0;
 size_t bytes=requested<remaining?requested:remaining;
 if(bytes)memcpy(data,f->image+f->position,bytes);
 f->position+=bytes;if(bytes<requested)f->eof=true;
 return bytes/size;
}
size_t OsFile_write(OsFile_Handle f,const void* data,size_t size,size_t count){return f&&f->stream&&data?fwrite(data,size,count,f->stream):0;}
long OsFile_length(OsFile_Handle f){
 if(!f)return -1;
 if(f->image)return (long)f->size;
 long pos=ftell(f->stream);if(pos<0||fseek(f->stream,0,SEEK_END))return -1;
 long length=ftell(f->stream);if(fseek(f->stream,pos,SEEK_SET))return -1;return length;
}
bool OsFile_exists(const char* name){struct stat st;return name&&stat(name,&st)==0;}
long OsFile_seek(OsFile_Handle f,long offset,int origin){
 if(!f)return -1;
 if(f->stream)return fseek(f->stream,offset,origin);
 int64_t base=origin==SEEK_SET?0:origin==SEEK_CUR?(int64_t)f->position:origin==SEEK_END?(int64_t)f->size:-1;
 if(base<0||(offset>0&&base>LONG_MAX-offset)||(offset<0&&offset < -base))return -1;
 int64_t pos=base+offset;if(pos<0||pos>LONG_MAX)return -1;
 f->position=(size_t)pos;f->eof=false;return 0;
}
int OsFile_error(OsFile_Handle f){return !f?1:f->stream?ferror(f->stream):0;}
int OsFile_eof(OsFile_Handle f){return !f?1:f->stream?feof(f->stream):f->eof;}
bool OsFile_rename(const char* from,const char* to){if(!from||!to)return false;retirePath(from);retirePath(to);return rename(from,to)==0;}
int OsFile_remove(const char* name){if(!name)return -1;retirePath(name);return remove(name);}
}
