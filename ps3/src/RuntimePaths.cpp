// SPDX-License-Identifier: GPL-2.0-only
#include <javacall_dir.h>
#include <string>
namespace {
std::string app="/dev_hdd0/game/PS3ME0001/USRDIR/appdb/";
std::string config="/dev_hdd0/game/PS3ME0001/USRDIR/lib/";
javacall_result copyPath(const std::string& p,javacall_utf16* out,int* len) {
    if(!out||!len||*len<static_cast<int>(p.size()))return JAVACALL_FAIL;
    for(size_t i=0;i<p.size();++i)out[i]=static_cast<unsigned char>(p[i]);
    *len=p.size();return JAVACALL_OK;
}
}
extern "C" {
// Paths are ASCII internal paths; external game paths use UTF-16 -> UTF-8.
void ps3me_set_runtime_paths(const char* a,const char* c){if(a)app=a;if(c)config=c;}
javacall_result javacall_dir_get_root_path(javacall_utf16* out,int* len){return copyPath(app,out,len);}
javacall_result javacall_dir_get_config_path(javacall_utf16* out,int* len){return copyPath(config,out,len);}
}
