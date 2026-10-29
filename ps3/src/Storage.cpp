// SPDX-License-Identifier: GPL-2.0-only
#include "Storage.hpp"
#include <algorithm>
#include <cctype>
#include <climits>
#include <cerrno>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

namespace ps3me {
namespace {
bool isJar(const std::string& n) {
    if (n.size() < 5) return false;
    std::string s = n.substr(n.size()-4);
    for (size_t i=0;i<s.size();++i) s[i]=static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
    return s==".jar";
}
bool makeDirs(const std::string& path) {
    if (path.empty()) return false;
    for(size_t i=1;i<=path.size();++i) {
        if(i!=path.size() && path[i]!='/') continue;
        std::string part=path.substr(0,i);
        if(::mkdir(part.c_str(),0700)!=0 && errno!=EEXIST) return false;
        struct stat st;
        if(::stat(part.c_str(),&st)!=0 || !S_ISDIR(st.st_mode)) return false;
    }
    return true;
}
}
Storage::Storage(const std::vector<std::string>& roots):roots_(roots),file_(0) {}
Storage::~Storage(){close();}
int Storage::list() {
    close(); entries_.clear();
    bool accessible=false;
    for(size_t r=0;r<roots_.size();++r) {
        DIR* dir=::opendir(roots_[r].c_str()); if(!dir) continue;
        accessible=true;
        struct dirent* ent;
        while((ent=::readdir(dir))) {
            std::string name=ent->d_name;
            if(!isJar(name) || name.find('/')!=std::string::npos) continue;
            std::string path=roots_[r]+"/"+name;
            struct stat st;
            // PSL1GHT newlib has stat(), but no lstat()/symlink API.
#ifdef PS3ME_PPU
            if(::stat(path.c_str(),&st)!=0 || !S_ISREG(st.st_mode) || st.st_size>INT_MAX) continue;
#else
            if(::lstat(path.c_str(),&st)!=0 || !S_ISREG(st.st_mode) || st.st_size>INT_MAX) continue;
#endif
            Entry e; e.name=name; e.path=path; entries_.push_back(e);
        }
        ::closedir(dir);
    }
    std::sort(entries_.begin(),entries_.end(),[](const Entry& a,const Entry& b){
        return a.name==b.name ? a.path<b.path : a.name<b.name;
    });
    return accessible ? static_cast<int>(entries_.size()) : -1;
}
const char* Storage::nameAt(int i) {
    return i>=0 && static_cast<size_t>(i)<entries_.size()?entries_[i].name.c_str():0;
}
int Storage::openAt(int i) {
    close();
    if(i<0 || static_cast<size_t>(i)>=entries_.size()) return -1;
    file_=::fopen(entries_[i].path.c_str(),"rb"); if(!file_) return -1;
    if(::fseek(file_,0,SEEK_END)!=0) {close();return -1;}
    long size=::ftell(file_);
    if(size<0 || size>INT_MAX || ::fseek(file_,0,SEEK_SET)!=0){close();return -1;}
    return static_cast<int>(size);
}
int Storage::read(void* dst,int n) {
    if(!file_ || n<0 || (!dst && n)) return -1;
    if(n==0)return 0;
    size_t got=::fread(dst,1,static_cast<size_t>(n),file_);
    return ::ferror(file_) ? -1 : static_cast<int>(got);
}
int Storage::readAt(unsigned int off,void* dst,int n) {
    if(!file_ || n<0 || (!dst && n)) return -1;
    long pos=::ftell(file_); if(pos<0 || ::fseek(file_,off,SEEK_SET)!=0) return -1;
    int got=read(dst,n);
    if(::fseek(file_,pos,SEEK_SET)!=0) return -1;
    return got;
}
void Storage::close(){if(file_)::fclose(file_);file_=0;}
bool SaveStore::validKey(const std::string& key) {
    if(key.empty() || key.size()>64) return false;
    for(size_t i=0;i<key.size();++i) {
        unsigned char c=key[i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'))return false;
    }
    return true;
}
bool SaveStore::write(const std::string& key,const void* data,unsigned int size) {
    if(!validKey(key) || !data || size==0 || size>4*1024*1024)return false;
    std::string dir=root_+"/"+key;
    if(!makeDirs(dir))return false;
    // One VM/save owner; leave the committed file untouched if writing fails.
    std::string path=dir+"/rms.dat",tmp=path+".tmp";
    FILE* f=::fopen(tmp.c_str(),"wb"); if(!f)return false;
    bool ok=::fwrite(data,1,size,f)==size;
    if(::fflush(f)!=0)ok=false;
    if(::fclose(f)!=0)ok=false;
    if(ok && ::rename(tmp.c_str(),path.c_str())==0)return true;
    ::remove(tmp.c_str()); return false;
}
int SaveStore::read(const std::string& key,void* data,unsigned int cap)const {
    if(!validKey(key) || !data || !cap)return -1;
    std::string path=root_+"/"+key+"/rms.dat";
    FILE* f=::fopen(path.c_str(),"rb"); if(!f)return -1;
    if(::fseek(f,0,SEEK_END)!=0){::fclose(f);return -1;}
    long size=::ftell(f);
    if(size<0 || static_cast<unsigned long>(size)>cap || size>4*1024*1024 || ::fseek(f,0,SEEK_SET)!=0){::fclose(f);return -1;}
    size_t got=::fread(data,1,size,f); bool ok=got==static_cast<size_t>(size)&&!::ferror(f);
    ::fclose(f);return ok?static_cast<int>(got):-1;
}
}
