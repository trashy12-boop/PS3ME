// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "hal/IPad.hpp"
#include <stdint.h>
namespace ps3me {
enum Button {UP,DOWN,LEFT,RIGHT,CROSS,CIRCLE,SQUARE,TRIANGLE,L1,R1,L2,R2,SELECT,L3,R3,BUTTON_COUNT};
enum Action {NONE,KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT,FIRE,SOFT1,SOFT2,NUM0,NUM1,NUM2,NUM3,NUM4,NUM5,NUM6,NUM7,NUM8,NUM9,STAR,POUND,TOUCH,ACTION_COUNT};
const char* buttonName(int);const char* actionName(int);int actionKey(int);
struct ControlConfig {
 int map[BUTTON_COUNT];bool pointer;int speed,deadzone;
 ControlConfig();bool valid()const;bool load(const char*);bool save(const char*)const;
};
struct InputSink {virtual ~InputSink(){};virtual void key(int code,bool pressed)=0;virtual void pen(int x,int y,int type)=0;};
class InputRouter {
public:
 explicit InputRouter(InputSink& sink):sink_(sink),active_(false){}
 void begin(const ControlConfig&,int width,int height);void end(){active_=false;}
 void update(const ps2::hal::PadButtons&,uint64_t milliseconds);
 bool visible()const{return active_&&config_.pointer;}
 int x()const{return (int)(x_+0.5f);}int y()const{return (int)(y_+0.5f);}bool touching()const{return touchDown_;}
private:
 InputSink& sink_;ControlConfig config_;bool active_,seeded_,touchDown_,previousTouch_;
 bool suppressed_[BUTTON_COUNT];int keys_[BUTTON_COUNT],keyCount_,width_,height_,lastPenX_,lastPenY_;
 float x_,y_;uint64_t lastTime_,lastDrag_;
};
ControlConfig& controlConfig();
void beginInput(int width,int height);void endInput();void pollInput(uint64_t milliseconds);
bool pointerVisible();int pointerX();int pointerY();bool pointerDown();
}
extern "C" int ps3me_pointer_supported();
extern "C" void ps3me_input_frame_ready();
