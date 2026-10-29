// SPDX-License-Identifier: GPL-2.0-only
#include <javacall_file.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdio>
#include <string>
namespace {
bool pathUtf8(const javacall_utf16* p,int n,std::string& out) {
    if(!p||n==0||n>4096)return false;
    if(n<0){for(n=0;n<4096&&p[n];++n){}if(n==4096)return false;}
    out.clear();
    for(int i=0;i<n;++i) {
        uint32_t c=p[i];if(!c)return false;
        if(c>=0xd800&&c<=0xdbff) {
            if(++i>=n||p[i]<0xdc00||p[i]>0xdfff)return false;
            c=0x10000+((c-0xd800)<<10)+(p[i]-0xdc00);
        } else if(c>=0xdc00&&c<=0xdfff)return false;
        if(c<0x80)out+=static_cast<char>(c);
        else if(c<0x800){out+=static_cast<char>(0xc0|(c>>6));out+=static_cast<char>(0x80|(c&63));}
        else if(c<0x10000){out+=static_cast<char>(0xe0|(c>>12));out+=static_cast<char>(0x80|((c>>6)&63));out+=static_cast<char>(0x80|(c&63));}
        else {out+=static_cast<char>(0xf0|(c>>18));out+=static_cast<char>(0x80|((c>>12)&63));out+=static_cast<char>(0x80|((c>>6)&63));out+=static_cast<char>(0x80|(c&63));}
    }
    return !out.empty();
}
int fd(javacall_handle h){return h?static_cast<int>(reinterpret_cast<intptr_t>(h)-1):-1;}
javacall_result status(int rc){return rc==0?JAVACALL_OK:JAVACALL_FAIL;}
}
extern "C" {
javacall_result javacall_file_init(){return JAVACALL_OK;}
javacall_result javacall_file_finalize(){return JAVACALL_OK;}
javacall_result javacall_file_open(const javacall_utf16* name,int n,int flags,javacall_handle* handle) {
    if(!handle)return JAVACALL_FAIL;
    *handle=0;
    std::string path;if(!pathUtf8(name,n,path))return JAVACALL_FAIL;
    int mode=flags&3;if(mode==3)return JAVACALL_FAIL;
    int f=mode==0?O_RDONLY:mode==1?O_WRONLY:O_RDWR;
    if(flags&JAVACALL_FILE_O_CREAT)f|=O_CREAT;
    if(flags&JAVACALL_FILE_O_TRUNC)f|=O_TRUNC;
    if(flags&JAVACALL_FILE_O_APPEND)f|=O_APPEND;
    int result=::open(path.c_str(),f,0600);
    if(result<0)return JAVACALL_FAIL;
    *handle=reinterpret_cast<javacall_handle>(static_cast<intptr_t>(result)+1);return JAVACALL_OK;
}
javacall_result javacall_file_close(javacall_handle h){return status(::close(fd(h)));}
long javacall_file_read(javacall_handle h,unsigned char* b,long n){return n<0||(!b&&n)?-1: ::read(fd(h),b,n);}
long javacall_file_write(javacall_handle h,const unsigned char* b,long n){return n<0||(!b&&n)?-1: ::write(fd(h),b,n);}
javacall_result javacall_file_delete(const javacall_utf16* name,int n){std::string p;return pathUtf8(name,n,p)?status(::remove(p.c_str())):JAVACALL_FAIL;}
javacall_result javacall_file_truncate(javacall_handle h,javacall_int64 n){return n<0?JAVACALL_FAIL:status(::ftruncate(fd(h),n));}
javacall_int64 javacall_file_seek(javacall_handle h,javacall_int64 n,javacall_file_seek_flags f){return f<0||f>2?-1: ::lseek(fd(h),n,f==0?SEEK_SET:f==1?SEEK_CUR:SEEK_END);}
javacall_int64 javacall_file_sizeofopenfile(javacall_handle h){struct stat s;return ::fstat(fd(h),&s)==0?s.st_size:-1;}
javacall_int64 javacall_file_sizeof(const javacall_utf16* name,int n){std::string p;struct stat s;return pathUtf8(name,n,p)&&::stat(p.c_str(),&s)==0?s.st_size:-1;}
javacall_result javacall_file_exist(const javacall_utf16* name,int n){return javacall_file_sizeof(name,n)>=0?JAVACALL_OK:JAVACALL_FAIL;}
// POSIX writes have no application-side stdio buffer. PCSL uses the same policy.
javacall_result javacall_file_flush(javacall_handle h){return fd(h)>=0?JAVACALL_OK:JAVACALL_FAIL;}
javacall_result javacall_file_rename(const javacall_utf16* a,int an,const javacall_utf16* b,int bn){std::string p,q;return pathUtf8(a,an,p)&&pathUtf8(b,bn,q)?status(::rename(p.c_str(),q.c_str())):JAVACALL_FAIL;}
}
