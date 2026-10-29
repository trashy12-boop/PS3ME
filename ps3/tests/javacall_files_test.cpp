// SPDX-License-Identifier: GPL-2.0-only
#include <javacall_file.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
int main() {
    char root[]="/tmp/ps3me-io-XXXXXX";assert(mkdtemp(root));
    std::u16string name;for(const char* p=root;*p;++p)name+=*p;
    name+=u"/a\u00e7\u00e3o_\U0001f3ae.dat";
    const javacall_utf16* path=reinterpret_cast<const javacall_utf16*>(name.c_str());
    javacall_handle h=0;
    assert(javacall_file_open(path,-1,JAVACALL_FILE_O_RDWR|JAVACALL_FILE_O_CREAT|JAVACALL_FILE_O_TRUNC,&h)==JAVACALL_OK);
    const unsigned char bytes[]={1,0,255,9};unsigned char readback[4]={0};
    assert(javacall_file_write(h,bytes,4)==4);
    assert(javacall_file_sizeofopenfile(h)==4);
    assert(javacall_file_seek(h,0,JAVACALL_FILE_SEEK_SET)==0);
    assert(javacall_file_read(h,readback,4)==4&&!std::memcmp(bytes,readback,4));
    assert(javacall_file_truncate(h,2)==JAVACALL_OK);
    assert(javacall_file_close(h)==JAVACALL_OK);
    assert(javacall_file_sizeof(path,-1)==2);
    std::u16string renamed=name+u".new";
    const javacall_utf16* target=reinterpret_cast<const javacall_utf16*>(renamed.c_str());
    assert(javacall_file_rename(path,-1,target,-1)==JAVACALL_OK);
    assert(javacall_file_exist(path,-1)==JAVACALL_FAIL);
    assert(javacall_file_delete(target,-1)==JAVACALL_OK);
    const javacall_utf16 invalid[]={0xd800,0};
    assert(javacall_file_open(invalid,1,0,&h)==JAVACALL_FAIL);
    assert(javacall_file_read(0,readback,4)==-1);
    assert(rmdir(root)==0);
    puts("PS3ME_JAVACALL_FILES_OK unicode, read/write, seek, truncate, rename, invalid UTF16");
}
