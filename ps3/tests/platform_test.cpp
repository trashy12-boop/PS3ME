// SPDX-License-Identifier: GPL-2.0-only
#include "Storage.hpp"
#include "Video.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

int main() {
    char path[]="/tmp/ps3me-test-XXXXXX"; assert(mkdtemp(path));
    std::string root=path,games=root+"/games";assert(mkdir(games.c_str(),0700)==0);
    {std::ofstream f((games+"/Zoo.JAR").c_str(),std::ios::binary);f<<"PK\003\004payload";}
    {std::ofstream f((games+"/ignore.txt").c_str());f<<"x";}
    assert(symlink("Zoo.JAR",(games+"/link.jar").c_str())==0);
    ps3me::Storage storage(std::vector<std::string>{games,root+"/absent"});
    assert(storage.list()==1);assert(std::strcmp(storage.nameAt(0),"Zoo.JAR")==0);
    assert(storage.nameAt(1)==0&&storage.nameAt(-1)==0);
    assert(storage.openAt(0)==11);
    char b[32]={0};assert(storage.read(b,2)==2&&!std::memcmp(b,"PK",2));
    assert(storage.readAt(4,b,7)==7&&!std::memcmp(b,"payload",7));
    assert(storage.read(b,2)==2&&b[0]==3&&b[1]==4); // random read preserved stream position
    assert(storage.readAt(100,b,1)==0);assert(storage.read(b,-1)==-1);
    storage.close();assert(storage.read(b,1)==-1);
    ps3me::SaveStore saves(root+"/saves");
    const char save1[]={ 'R','M','S','1',0,1,0,0 };
    assert(saves.write("WZOO",save1,sizeof(save1)));
    std::memset(b,0,sizeof(b));assert(saves.read("WZOO",b,sizeof(b))==8&&!std::memcmp(save1,b,8));
    assert(!saves.write("../bad",save1,8));assert(!saves.write("WZOO",0,8));
    assert(saves.read("WZOO",b,7)==-1); // never silently restore a truncated save
    assert(saves.read("absent",b,sizeof(b))==-1);
    assert(saves.write("WZOO","replacement",11));
    assert(saves.read("WZOO",b,sizeof(b))==11&&!std::memcmp(b,"replacement",11));
    assert(ps3me::rgb565ToXrgb(0xf800)==0xffff0000u);
    assert(ps3me::rgb565ToXrgb(0x07e0)==0xff00ff00u);
    assert(ps3me::rgb565ToXrgb(0x001f)==0xff0000ffu);
    assert(ps3me::rgb565ToXrgb(0xffff)==0xffffffffu);
    ps3me::Rect r=ps3me::fit(240,320,1280,720);assert(r.x==370&&r.y==0&&r.w==540&&r.h==720);
    r=ps3me::fit(320,240,1280,720);assert(r.x==160&&r.w==960&&r.h==720);
    uint16_t pixels[]={0xf800,0x001f};uint32_t out[12];
    for(unsigned int i=0;i<12;++i)out[i]=0x12345678;
    ps3me::scaleRgb565(pixels,2,1,out,4,2,6);
    assert(out[0]==0xffff0000u&&out[1]==0xffff0000u&&out[2]==0xff0000ffu&&out[3]==0xff0000ffu);
    assert(out[4]==0x12345678&&out[5]==0x12345678); // pitch padding untouched
    assert(out[6]==out[0]&&out[9]==out[3]);
    // Remove only this test's files; preserve an external data path if supplied elsewhere.
    unlink((games+"/link.jar").c_str());unlink((games+"/Zoo.JAR").c_str());unlink((games+"/ignore.txt").c_str());
    unlink((root+"/saves/WZOO/rms.dat").c_str());rmdir((root+"/saves/WZOO").c_str());rmdir((root+"/saves").c_str());rmdir(games.c_str());rmdir(root.c_str());
    std::puts("PASS: JAR enumeration/streaming, save round-trip/replacement/rejection, RGB565, aspect ratio and pitch");
}
