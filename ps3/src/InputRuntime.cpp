// SPDX-License-Identifier: GPL-2.0-only
#include "Controls.hpp"
#include "GameSettings.hpp"
#include "hal/LcdDevice.hpp"
extern "C" void JVM_Stop(int);
#include "hal/Keypad.hpp"
#include "hal/EventQueue.hpp"
#include <sys/systime.h>
#include <cstdio>
extern "C" {
#include <javacall_keypress.h>
#include <javacall_penevent.h>
void ps3me_refresh_cursor() __attribute__((weak));

}
namespace ps3me {
class JavaInput:public InputSink {public:void key(int c,bool down){javanotify_key_event((javacall_key)c,down?JAVACALL_KEYPRESSED:JAVACALL_KEYRELEASED);
}void pen(int x,int y,int type){javanotify_pen_event(x,y,(javacall_penevent_type)type);
}};

static JavaInput sink;
static InputRouter router(sink);
static ControlConfig config;

static bool running=false,test=false,testReported=false;
static uint64_t firstFrame=0;
static bool chordHeld=false,overlayTest=false;

ControlConfig& controlConfig(){return config;
}
void beginInput(int w,int h){ps2::hal::EventQueue::instance().clear();
router.begin(config,w,h);
running=true;chordHeld=false;
FILE* menuTest=std::fopen("/dev_hdd0/game/PS3ME0001/USRDIR/logs/ingame_test.once","rb");overlayTest=menuTest!=0;if(menuTest){std::fclose(menuTest);std::remove("/dev_hdd0/game/PS3ME0001/USRDIR/logs/ingame_test.once");}
firstFrame=0;
testReported=false;
const char* path="/dev_hdd0/game/PS3ME0001/USRDIR/logs/input_test.once";
FILE* f=std::fopen(path,"rb");
test=f!=0;
if(f){std::fclose(f);
std::remove(path);
std::fprintf(stderr,"[PS3ME input test] enabled\n");
}}
void endInput(){running=false;
router.end();
}
void pollInput(uint64_t now){if(!running)return;
ps2::hal::PadButtons b;

 if(test){if(firstFrame){uint64_t dt=now-firstFrame;
if(dt>=500&&dt<1100)b.r2=true;
if(dt>=650&&dt<950)b.rx=240;
if(dt>=1300&&dt<1450)b.cross=true;
if(dt>=1600&&!testReported){testReported=true;
std::fprintf(stderr,"[PS3ME input test] sequence completed cursor=%d,%d\n",router.x(),router.y());
}}}
 else {ps2::hal::IPad* pad=ps2::hal::Keypad::instance().pad();
if(pad)pad->read(&b);
}
 if(!b.l3&&!b.r3)chordHeld=false;
 if((b.l3&&b.r3&&!chordHeld)||(overlayTest&&firstFrame&&now-firstFrame>=1000)){
  bool automated=overlayTest;overlayTest=false;chordHeld=true;
  router.update(ps2::hal::PadButtons(),now);
  int quit=showInGameMenu(automated);
  auto& lcd=ps2::hal::LcdDevice::instance();router.begin(config,lcd.width(),lcd.height());
  if(quit){JVM_Stop(0);}
  return;
 }
 if(chordHeld){b.l3=false;b.r3=false;}
 router.update(b,now);
if(ps3me_refresh_cursor&&router.visible())ps3me_refresh_cursor();

}
bool pointerVisible(){return router.visible();
}int pointerX(){return router.x();
}int pointerY(){return router.y();
}bool pointerDown(){return router.touching();
}
void frameReady(){if(running&&!firstFrame)firstFrame=sysGetSystemTime()/1000;
}
}
extern "C" int ps3me_pointer_supported(){return ps3me::controlConfig().pointer?1:0;
}
extern "C" void ps3me_input_frame_ready(){ps3me::frameReady();
}
