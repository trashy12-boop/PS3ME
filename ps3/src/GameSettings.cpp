// SPDX-License-Identifier: GPL-2.0-only
#include "GameSettings.hpp"
#include <sys/stat.h>
#include <cstdio>
#include <stdio.h>
#include <cstdint>
namespace ps3me {
static bool dimensions(int w,int h){return w>=128&&w<=1024&&h>=128&&h<=1024;}
void GameSettings::begin(const std::string& game,const std::string& root,ControlConfig& c,int& w,int& h){global_=c;defaultW_=w;defaultH_=h;uint64_t hash=14695981039346656037ULL;for(unsigned char b:game){hash^=b;hash*=1099511628211ULL;}char key[24];::snprintf(key,sizeof(key),"%016llx",(unsigned long long)hash);::mkdir(root.c_str(),0700);base_=root+"/"+key;c.load((base_+".controls.cfg").c_str());FILE* f=std::fopen((base_+".display.cfg").c_str(),"rb");if(f){int a=0,b=0;char extra;bool ok=std::fscanf(f,"PS3ME_DISPLAY 1 %d %d %c",&a,&b,&extra)==2;std::fclose(f);if(ok&&dimensions(a,b)){w=a;h=b;}}}
bool GameSettings::save(const ControlConfig& c,int w,int h)const{if(base_.empty()||!dimensions(w,h))return false;if(!c.save((base_+".controls.cfg").c_str()))return false;std::string path=base_+".display.cfg",temp=path+".tmp";FILE* f=std::fopen(temp.c_str(),"wb");if(!f)return false;bool ok=std::fprintf(f,"PS3ME_DISPLAY 1 %d %d\n",w,h)>0;if(std::fclose(f))ok=false;if(!ok){std::remove(temp.c_str());return false;}if(std::rename(temp.c_str(),path.c_str())==0)return true;std::string backup=path+".bak";std::remove(backup.c_str());if(std::rename(path.c_str(),backup.c_str()))return false;if(std::rename(temp.c_str(),path.c_str())==0){std::remove(backup.c_str());return true;}std::rename(backup.c_str(),path.c_str());return false;}
bool GameSettings::useGlobal(ControlConfig& c,int& w,int& h)const{if(base_.empty())return false;const char* tails[]={".controls.cfg",".controls.cfg.bak",".display.cfg",".display.cfg.bak"};for(const char* tail:tails){std::string p=base_+tail;FILE* f=std::fopen(p.c_str(),"rb");if(f){std::fclose(f);if(std::remove(p.c_str()))return false;}}c=global_;w=defaultW_;h=defaultH_;return true;}
GameSettings& gameSettings(){static GameSettings settings;return settings;}
}
