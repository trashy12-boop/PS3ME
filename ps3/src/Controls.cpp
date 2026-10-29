// SPDX-License-Identifier: GPL-2.0-only
#include "Controls.hpp"
#include <cstdio>
#include <cstring>
#include <string>
namespace ps3me {
static const char* buttons[]={"Up","Down","Left","Right","Cross","Circle","Square","Triangle","L1","R1","L2","R2","Select","L3","R3"};

static const char* actions[]={"Disabled","Up","Down","Left","Right","Fire / Center","Left softkey","Right softkey","Key 0","Key 1","Key 2","Key 3","Key 4","Key 5","Key 6","Key 7","Key 8","Key 9","Key *","Key #","Touch / hold"};

const char* buttonName(int i){return i>=0&&i<BUTTON_COUNT?buttons[i]:"?";
}
const char* actionName(int i){return i>=0&&i<ACTION_COUNT?actions[i]:"?";
}
int actionKey(int i){if(i>=KEY_UP&&i<=SOFT2)return -i;
if(i>=NUM0&&i<=NUM9)return '0'+i-NUM0;
if(i==STAR)return '*';
if(i==POUND)return '#';
return 0;
}
ControlConfig::ControlConfig():pointer(true),speed(180),deadzone(28){int defaults[]={KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT,FIRE,NONE,STAR,NUM0,SOFT1,SOFT2,POUND,TOUCH,NONE,NONE,NONE};
std::memcpy(map,defaults,sizeof(map));
}
bool ControlConfig::valid()const {if(speed<60||speed>600||deadzone<12||deadzone>64)return false;
for(int i=0;
i<BUTTON_COUNT;
i++)if(map[i]<NONE||map[i]>=ACTION_COUNT)return false;
return true;
}
static bool readSettings(const char* path,ControlConfig& result){
if(!path)return false;
 ControlConfig candidate;
FILE* f=std::fopen(path,"rb");
if(!f)return false;
char line[128];
bool seen[BUTTON_COUNT+3]={};
bool ok=std::fgets(line,sizeof(line),f)&&std::strcmp(line,"PS3ME_CONTROLS 1\n")==0;

 while(ok&&std::fgets(line,sizeof(line),f)){char name[32],extra[2];
int n;
if(std::sscanf(line,"%31s %d %1s",name,&n,extra)!=2){ok=false;
break;
}int k=-1;
for(int i=0;
i<BUTTON_COUNT;
i++)if(std::strcmp(name,buttons[i])==0)k=i;

  if(std::strcmp(name,"pointer")==0)k=BUTTON_COUNT;
if(std::strcmp(name,"speed")==0)k=BUTTON_COUNT+1;
if(std::strcmp(name,"deadzone")==0)k=BUTTON_COUNT+2;

  if(k<0||seen[k]){ok=false;
break;
}seen[k]=true;
if(k<BUTTON_COUNT)candidate.map[k]=n;
else if(k==BUTTON_COUNT){if(n!=0&&n!=1){ok=false;
break;
}candidate.pointer=n!=0;
}else if(k==BUTTON_COUNT+1)candidate.speed=n;
else candidate.deadzone=n;

 }
 if(std::ferror(f))ok=false;
std::fclose(f);
for(int i=0;
i<BUTTON_COUNT+3;
i++)if(!seen[i])ok=false;
if(!candidate.valid())ok=false;
if(ok)result=candidate;
return ok;

}
bool ControlConfig::load(const char* path){
 if(!path)return false;
 if(readSettings(path,*this))return true;
 std::string backup=std::string(path)+".bak";
 return readSettings(backup.c_str(),*this);
}
bool ControlConfig::save(const char* path)const {
 if(!valid()||!path)return false;
std::string temp=std::string(path)+".tmp";
FILE* f=std::fopen(temp.c_str(),"wb");
if(!f)return false;

 bool ok=std::fprintf(f,"PS3ME_CONTROLS 1\n")>0;
for(int i=0;
i<BUTTON_COUNT;
i++)if(std::fprintf(f,"%s %d\n",buttons[i],map[i])<0)ok=false;

 if(std::fprintf(f,"pointer %d\nspeed %d\ndeadzone %d\n",pointer?1:0,speed,deadzone)<0)ok=false;
if(std::fflush(f)!=0)ok=false;
if(std::fclose(f)!=0)ok=false;

 if(!ok){std::remove(temp.c_str());return false;}
 std::string backup=std::string(path)+".bak";
 if(std::rename(temp.c_str(),path)==0){std::remove(backup.c_str());return true;}
 // CellFs refuses to rename over an existing file. Preserve a recoverable
 // previous configuration until the new complete file has been committed.
 FILE* existing=std::fopen(path,"rb");
 if(!existing){std::remove(temp.c_str());return false;}
 std::fclose(existing);
 ControlConfig previous,recovery;
 if(!readSettings(path,previous)&&readSettings(backup.c_str(),recovery)){
  // The primary is already corrupt; retain the valid recovery file.
  if(std::remove(path)==0&&std::rename(temp.c_str(),path)==0)return true;
  std::remove(temp.c_str());return false;
 }
 FILE* oldBackup=std::fopen(backup.c_str(),"rb");
 if(oldBackup){std::fclose(oldBackup);if(std::remove(backup.c_str())!=0){std::remove(temp.c_str());return false;}}
 if(std::rename(path,backup.c_str())!=0){std::remove(temp.c_str());return false;}
 if(std::rename(temp.c_str(),path)==0)return true;
 std::rename(backup.c_str(),path);
 std::remove(temp.c_str());return false;

}
static bool contains(const int* p,int n,int k){for(int i=0;
i<n;
i++)if(p[i]==k)return true;
return false;
}
void InputRouter::begin(const ControlConfig& c,int w,int h){config_=c.valid()?c:ControlConfig();
width_=w>0?w:1;
height_=h>0?h:1;
x_=(width_-1)*0.5f;
y_=(height_-1)*0.5f;
active_=true;
seeded_=false;
touchDown_=previousTouch_=false;
keyCount_=0;
lastTime_=lastDrag_=0;
lastPenX_=x();
lastPenY_=y();
std::memset(suppressed_,0,sizeof(suppressed_));
}
void InputRouter::update(const ps2::hal::PadButtons& b,uint64_t now){
 if(!active_)return;

 bool down[]={b.up||b.ly<72,b.down||b.ly>184,b.left||b.lx<72,b.right||b.lx>184,b.cross,b.circle,b.square,b.triangle,b.l1,b.r1,b.l2,b.r2,b.select,b.l3,b.r3};

 if(!seeded_){for(int i=0;
i<BUTTON_COUNT;
i++)suppressed_[i]=down[i];
seeded_=true;
lastTime_=now;
}
 uint64_t dt=now>=lastTime_?now-lastTime_:0;
if(dt>50)dt=50;
lastTime_=now;

 if(config_.pointer){int axes[]={b.rx,b.ry};
float* coords[]={&x_,&y_};
for(int i=0;
i<2;
i++){int v=axes[i]<0?0:axes[i]>255?255:axes[i];
v-=128;
int mag=v<0?-v:v;
if(mag>127)mag=127;
if(mag>config_.deadzone)*coords[i]+=(v<0?-1.0f:1.0f)*(mag-config_.deadzone)/(127-config_.deadzone)*config_.speed*(width_<height_?width_:height_)/240.0f*(float)dt/1000.0f;
}
  if(x_<0)x_=0;
if(y_<0)y_=0;
if(x_>width_-1)x_=(float)(width_-1);
if(y_>height_-1)y_=(float)(height_-1);

 }
 int keys[BUTTON_COUNT],n=0;
bool touch=false;

 for(int i=0;
i<BUTTON_COUNT;
i++){if(!down[i])suppressed_[i]=false;
if(!down[i]||suppressed_[i])continue;
int action=config_.map[i];
if(action==TOUCH){if(config_.pointer)touch=true;
}else {int k=actionKey(action);
if(k&&!contains(keys,n,k))keys[n++]=k;
}}
 for(int i=0;
i<keyCount_;
i++)if(!contains(keys,n,keys_[i]))sink_.key(keys_[i],false);

 for(int i=0;
i<n;
i++)if(!contains(keys_,keyCount_,keys[i]))sink_.key(keys[i],true);

 std::memcpy(keys_,keys,n*sizeof(int));
keyCount_=n;

 if(touch&&!previousTouch_){sink_.pen(x(),y(),1);
touchDown_=true;
lastPenX_=x();
lastPenY_=y();
lastDrag_=now;
}
 else if(!touch&&touchDown_){sink_.pen(x(),y(),2);
touchDown_=false;
}
 else if(touchDown_&&(x()!=lastPenX_||y()!=lastPenY_)&&now-lastDrag_>=33){sink_.pen(x(),y(),3);
lastPenX_=x();
lastPenY_=y();
lastDrag_=now;
}
 previousTouch_=touch;

}
}
