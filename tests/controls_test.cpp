// SPDX-License-Identifier: GPL-2.0-only
#include "Controls.hpp"
#include "Video.hpp"
#include <vector>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <cerrno>
#include <string>
extern "C" int __real_rename(const char*,const char*);
extern "C" int __wrap_rename(const char* from,const char* to){FILE*f=std::fopen(to,"rb");if(f){std::fclose(f);errno=EEXIST;return -1;}return __real_rename(from,to);}
using namespace ps3me;
using ps2::hal::PadButtons;
struct Event{int type,x,y;};
struct Sink:InputSink {std::vector<Event> events;void key(int c,bool down){events.push_back(Event{down?10:11,c,0});}void pen(int x,int y,int t){events.push_back(Event{t,x,y});}};
int main(int argc,char**argv){
 assert(argc==2);ControlConfig c;assert(c.valid());assert(c.map[R2]==TOUCH);c.map[CROSS]=NUM5;c.speed=270;assert(c.save(argv[1]));ControlConfig restored;assert(restored.load(argv[1]));assert(restored.map[CROSS]==NUM5&&restored.speed==270);
 {FILE*f=std::fopen(argv[1],"ab");std::fprintf(f,"Cross 9000\n");std::fclose(f);}ControlConfig saved=restored;assert(!restored.load(argv[1]));assert(restored.map[CROSS]==saved.map[CROSS]);assert(c.save(argv[1]));
 Sink s;InputRouter r(s);PadButtons b;r.begin(c,240,320);b.cross=true;r.update(b,0);r.update(b,10);assert(s.events.empty());b.cross=false;r.update(b,20);assert(s.events.empty());b.cross=true;r.update(b,30);assert(s.events.size()==1&&s.events[0].x=='5');b.cross=false;r.update(b,40);assert(s.events.back().type==11);
 // Simulate PS3 rename-overwrite rejection and recover an interrupted save.
 assert(c.save(argv[1]));std::remove(argv[1]);ControlConfig recovered;assert(recovered.load(argv[1])&&recovered.map[CROSS]==NUM5);
 {FILE*f=std::fopen(argv[1],"wb");std::fputs("broken",f);std::fclose(f);}assert(recovered.load(argv[1])&&recovered.map[CROSS]==NUM5);assert(c.save(argv[1]));assert(recovered.load(argv[1]));
 // Two buttons sharing a key must release it only after both are released.
 c.map[CIRCLE]=NUM5;s.events.clear();r.begin(c,240,320);b=PadButtons();r.update(b,0);b.cross=true;r.update(b,10);b.circle=true;r.update(b,20);b.cross=false;r.update(b,30);assert(s.events.size()==1);b.circle=false;r.update(b,40);assert(s.events.size()==2&&s.events.back().type==11);
 // Pointer press, throttled drag and release share logical game coordinates.
 s.events.clear();r.begin(c,240,320);b=PadButtons();r.update(b,0);b.r2=true;r.update(b,10);assert(s.events[0].type==1&&s.events[0].x==120&&s.events[0].y==160);b.rx=255;r.update(b,20);assert(s.events.size()==1);r.update(b,50);assert(s.events.back().type==3&&s.events.back().x>120);b=PadButtons();r.update(b,60);assert(s.events.back().type==2);
 // Neutral stick drift/deadzone, viewport boundaries, disconnect release.
 s.events.clear();r.begin(c,176,220);b=PadButtons();r.update(b,0);int oldX=r.x();b.rx=150;r.update(b,50);assert(r.x()==oldX);b.rx=0;b.ry=0;for(int t=100;t<4000;t+=50)r.update(b,t);assert(r.x()==0&&r.y()==0);b.r2=true;r.update(b,4010);b=PadButtons();r.update(b,4020);assert(s.events.back().type==2);
 // Independent movement rate at different polling frequencies.
 Sink a1,a2;InputRouter r1(a1),r2(a2);r1.begin(c,480,800);r2.begin(c,480,800);b=PadButtons();r1.update(b,0);r2.update(b,0);b.rx=200;for(int t=10;t<=500;t+=10)r1.update(b,t);for(int t=20;t<=500;t+=20)r2.update(b,t);assert(std::abs(r1.x()-r2.x())<=1);
 c.pointer=false;s.events.clear();r.begin(c,240,320);b=PadButtons();r.update(b,0);b.r2=true;b.rx=255;r.update(b,50);assert(s.events.empty()&&!r.visible());
 // Stick/D-pad uses the same configured action and emits no extra arrow.
 c.map[UP]=NUM2;s.events.clear();r.begin(c,240,320);b=PadButtons();r.update(b,0);b.ly=0;r.update(b,20);assert(s.events.size()==1&&s.events[0].x=='2');
 // Aspect-correct cursor tip. Surface stride guards and source buffers intact.
 std::vector<uint32_t> pixels(1290*720+2,0x12345678);Rect v=fit(240,320,1280,720);drawCursor(&pixels[1],1280,720,1290,v,120,160,240,320,1,2);assert(pixels[1+360*1290+640]==1);assert(pixels.front()==0x12345678&&pixels.back()==0x12345678);drawCursor(&pixels[1],1280,720,1290,v,239,319,240,320,1,2);assert(pixels.back()==0x12345678);
 std::remove(argv[1]);std::puts("CONTROLS PASS: persistence, validation, suppression, duplicate keys, remapping, pen lifecycle, deadzone, clamping, timing, cursor geometry");
}
