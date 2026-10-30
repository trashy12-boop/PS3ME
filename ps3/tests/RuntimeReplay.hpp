// SPDX-License-Identifier: GPL-2.0-only
// Developer-only input/capture harness. Not compiled into release packages.
#include "hal/LcdDevice.hpp"
extern "C" {
#include <javacall_keypress.h>
}
static void ps3me_runtime_replay() {
    struct Event {unsigned ms;int key,down;};
    static Event events[128];static unsigned count=0,next=0;
    static u64 began=0;static unsigned capture=0;
    if(!began){
        began=sysGetSystemTime();
        FILE* f=std::fopen("/dev_hdd0/game/PS3ME0001/USRDIR/logs/runtime_replay.txt","rb");
        if(f){while(count<128&&std::fscanf(f,"%u %d %d",&events[count].ms,&events[count].key,&events[count].down)==3)++count;std::fclose(f);}
    }
    unsigned elapsed=(unsigned)((sysGetSystemTime()-began)/1000);
    while(next<count&&elapsed>=events[next].ms){
        const Event& e=events[next++];
        javanotify_key_event((javacall_key)e.key,e.down?JAVACALL_KEYPRESSED:JAVACALL_KEYRELEASED);
        std::fprintf(stderr,"[PS3ME replay] ms=%u key=%d down=%d\n",elapsed,e.key,e.down);
    }
    if(elapsed/5000>capture && capture<24){
        capture=elapsed/5000;
        int width=0,height=0;
        javacall_pixel* pixels=ps2::hal::LcdDevice::instance().screen(&width,&height);
        if(!pixels||width<=0||height<=0)return;
        char path[160];::snprintf(path,sizeof(path),"/dev_hdd0/game/PS3ME0001/USRDIR/logs/frame-%02u.ppm",capture);
        FILE* f=std::fopen(path,"wb");if(!f)return;
        std::fprintf(f,"P6\n%d %d\n255\n",width,height);
        for(int y=0;y<height;y++)for(int x=0;x<width;x++){
            unsigned p=pixels[y*width+x];
            unsigned char rgb[3]={(unsigned char)(((p>>11)*255+15)/31),
                (unsigned char)((((p>>5)&63)*255+31)/63),(unsigned char)(((p&31)*255+15)/31)};
            std::fwrite(rgb,1,3,f);
        }
        std::fclose(f);
    }
}
