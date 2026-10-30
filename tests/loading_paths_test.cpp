// SPDX-License-Identifier: GPL-2.0-only
#include "Video.hpp"
#include "../vm/ps3_os/OsFile_generic.cpp"
#include <vector>
#include <random>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
static void reference(const uint16_t* s,int sw,int sh,uint32_t* d,int dw,int dh,int pitch){
 for(int y=0;y<dh;y++)for(int x=0;x<dw;x++)d[y*pitch+x]=0xff000000u;
 ps3me::Rect r=ps3me::fit(sw,sh,dw,dh);
 for(int y=0;y<r.h;y++)for(int x=0;x<r.w;x++)d[(y+r.y)*pitch+x+r.x]=ps3me::rgb565ToXrgb(s[((int64_t)y*sh/r.h)*sw+(int64_t)x*sw/r.w]);
}
static double elapsed(std::chrono::steady_clock::time_point a){return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-a).count();}
int main(int argc,char** argv){
 std::mt19937 random(42);
 for(int i=0;i<200;i++){
  int sw=1+random()%1024,sh=1+random()%1024,dw=1+random()%1400,dh=1+random()%800,pitch=dw+5;
  std::vector<uint16_t> s(sw*sh);for(auto& p:s)p=random();
  std::vector<uint32_t> old(pitch*dh,0x12345678),out(old);
  reference(s.data(),sw,sh,old.data(),dw,dh,pitch);ps3me::scaleRgb565(s.data(),sw,sh,out.data(),dw,dh,pitch);
  assert(old==out);
 }
 std::vector<uint16_t> pixels(240*320,0x12ab);std::vector<uint32_t> dest(1280*720);
 auto start=std::chrono::steady_clock::now();for(int i=0;i<120;i++)reference(pixels.data(),240,320,dest.data(),1280,720,1280);double old=elapsed(start);
 start=std::chrono::steady_clock::now();for(int i=0;i<120;i++)ps3me::scaleRgb565(pixels.data(),240,320,dest.data(),1280,720,1280);double fast=elapsed(start);
 printf("Host scaler: reference %.2f ms, optimized %.2f ms, %.2fx (120 frames)\n",old,fast,old/fast);
 assert(argc==2);
 OsFile_Handle jar=OsFile_open(argv[1],"rb");assert(jar&&jar->image);assert(archiveResident==jar->size);
 FILE* raw=fopen(argv[1],"rb");assert(raw);std::vector<unsigned char> a(100),b(100);size_t n=jar->size;
 for(int i=0;i<2000;i++){
  size_t pos=random()%(n+20);assert(!OsFile_seek(jar,pos,SEEK_SET));assert(!fseek(raw,pos,SEEK_SET));
  size_t size=1+random()%5,count=1+random()%20;
  assert(OsFile_read(jar,a.data(),size,count)==fread(b.data(),size,count,raw));
  assert(a==b);assert(!!OsFile_eof(jar)==!!feof(raw));
 }
 assert(OsFile_seek(jar,-1,SEEK_SET)==-1);assert(OsFile_seek(jar,LONG_MIN,SEEK_CUR)==-1);
 assert(OsFile_seek(jar,LONG_MAX,SEEK_SET)==0);assert(OsFile_seek(jar,1,SEEK_CUR)==-1);
 assert(OsFile_seek(jar,0,SEEK_END)==0&&!OsFile_eof(jar));assert(OsFile_read(jar,a.data(),0,1)==0&&!OsFile_eof(jar));
 assert(OsFile_read(jar,a.data(),1,1)==0&&OsFile_eof(jar));
 start=std::chrono::steady_clock::now();for(int i=0;i<100000;i++){fseek(raw,i%(n-16),SEEK_SET);assert(fread(b.data(),1,16,raw)==16);}double io=elapsed(start);
 start=std::chrono::steady_clock::now();for(int i=0;i<100000;i++){OsFile_seek(jar,i%(n-16),SEEK_SET);OsFile_read(jar,a.data(),1,16);}double mem=elapsed(start);
 printf("Host archive small reads: stdio %.2f ms, snapshot %.2f ms, %.2fx\n",io,mem,io/mem);
 fclose(raw);OsFile_close(jar);assert(archiveResident==n);
 jar=OsFile_open(argv[1],"rb");assert(jar&&jar->image&&archiveResident==n);ps3me_archive_reset();
 assert(archiveResident==n);assert(OsFile_read(jar,a.data(),1,2)==2);OsFile_close(jar);assert(archiveResident==0);
 // Shared 12 MiB cap, 8 MiB per-file cap, and writable-file fallback.
 char path[]="/tmp/ps3me-cache-XXXXXX.jar";int fd=mkstemps(path,4);assert(fd>=0);assert(!ftruncate(fd,7*1024*1024));close(fd);
 OsFile_Handle first=OsFile_open(path,"rb"),second=OsFile_open(path,"rb");assert(first&&first->image&&second&&second->image==first->image);assert(archiveResident==7*1024*1024);OsFile_close(first);OsFile_close(second);
 char path2[]="/tmp/ps3me-cache2-XXXXXX.jar";fd=mkstemps(path2,4);assert(fd>=0);assert(!ftruncate(fd,7*1024*1024));close(fd);
 first=OsFile_open(path2,"rb");assert(first&&first->stream);OsFile_close(first);unlink(path2);ps3me_archive_reset();assert(!archiveResident);
 fd=open(path,O_WRONLY);assert(fd>=0);assert(!ftruncate(fd,9*1024*1024));close(fd);
 first=OsFile_open(path,"rb");assert(first&&first->stream);OsFile_close(first);
 first=OsFile_open(path,"wb+");assert(first&&first->stream);assert(OsFile_write(first,"RMS",1,3)==3);assert(!OsFile_seek(first,0,SEEK_SET));assert(OsFile_read(first,a.data(),1,3)==3&&!memcmp(a.data(),"RMS",3));OsFile_close(first);unlink(path);
 ps3me_archive_reset();assert(!archiveResident);
 puts("PASS: 200 pixel-exact comparisons; shared snapshots/reset lifetime; archive random reads/EOF/overflow/budget/write fallback");
}
