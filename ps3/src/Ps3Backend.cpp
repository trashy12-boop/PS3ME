// SPDX-License-Identifier: GPL-2.0-only
#include "Ps3Backend.hpp"
#include "Video.hpp"
#include "Controls.hpp"
#include "CooperativeTimers.hpp"
#include "hal/GameStorage.hpp"
#include "hal/LcdDevice.hpp"
#include "hal/Keypad.hpp"
#include "hal/EventQueue.hpp"
#include "hal/SystemClock.hpp"
#include "hal/MemoryManager.hpp"
#include "hal/Logger.hpp"
#include <io/pad.h>
#include <sys/systime.h>
#include <cstdlib>
#include <cstring>

extern "C" void ps3me_on_start() __attribute__((weak));
namespace ps3me {
static_assert(sizeof(javacall_pixel)==2,"MIDP requires RGB565 pixels");
static_assert(sizeof(javacall_int32)==4,"JavaCall integers must remain 32 bits");
static_assert(sizeof(javacall_int64)==8,"JavaCall longs must remain 64 bits");
Framebuffer::Framebuffer():raster_(0),screen_(0),converted_(0),width_(0),height_(0),haveFrame_(false),paintedX_(-1),paintedY_(-1),paintedDown_(false),lastFlip_(0) {}
Framebuffer::~Framebuffer(){unmap();if(converted_)SDL_FreeSurface(converted_);}
bool Framebuffer::initializeVideo() {
    if(screen_&&converted_)return true;
    if(SDL_InitSubSystem(SDL_INIT_VIDEO)!=0)return false;
    if(!screen_)screen_=SDL_SetVideoMode(1280,720,32,SDL_FULLSCREEN|SDL_SWSURFACE);
    if(!screen_)screen_=SDL_SetVideoMode(640,480,32,SDL_FULLSCREEN|SDL_SWSURFACE);
    if(!screen_)return false;
    // Explicit masks make the numerical RGB conversion independent of byte order.
    converted_=SDL_CreateRGBSurface(SDL_SWSURFACE,screen_->w,screen_->h,32,
                                   0x00ff0000,0x0000ff00,0x000000ff,0);
    return converted_!=0;
}
javacall_pixel* Framebuffer::map(int w,int h) {
    if(w<=0||h<=0||w>MAX_WIDTH||h>MAX_HEIGHT)return 0;
    if(!raster_)raster_=static_cast<javacall_pixel*>(::calloc(MAX_WIDTH*MAX_HEIGHT,sizeof(javacall_pixel)));
    if(!raster_)return 0;
    width_=w;height_=h;haveFrame_=false;return raster_;
}
void Framebuffer::unmap(){::free(raster_);raster_=0;width_=height_=0;haveFrame_=false;}
void Framebuffer::present() {
    if(!raster_||!converted_||!screen_)return;
    // Confirm the diagnostic's actual RGB565 raster reached the native backend.
    static bool bandsReported=false;
    if(!bandsReported&&width_==240&&height_>=240){
        for(int y=0;y<height_;++y){
            if(raster_[y*width_+10]==0xf800&&raster_[y*width_+100]==0x07e0&&raster_[y*width_+200]==0x001f){
                std::fprintf(stderr,"[PS3ME video] RGB565 diagnostic bands received %dx%d\n",width_,height_);
                bandsReported=true;break;
            }
        }
    }
    if(SDL_LockSurface(converted_)!=0)return;
    scaleRgb565(raster_,width_,height_,static_cast<uint32_t*>(converted_->pixels),
                converted_->w,converted_->h,converted_->pitch/4);
    SDL_UnlockSurface(converted_);
    haveFrame_=true;ps3me_input_frame_ready();blitCursor();
}

void Framebuffer::blitCursor(){
 SDL_BlitSurface(converted_,0,screen_,0);
 if(pointerVisible()&&screen_->format->BytesPerPixel==4&&SDL_LockSurface(screen_)==0){
  drawCursor((uint32_t*)screen_->pixels,screen_->w,screen_->h,screen_->pitch/4,fit(width_,height_,screen_->w,screen_->h),pointerX(),pointerY(),width_,height_,SDL_MapRGB(screen_->format,0,0,0),pointerDown()?SDL_MapRGB(screen_->format,255,210,40):SDL_MapRGB(screen_->format,255,255,255));
  SDL_UnlockSurface(screen_);
 }
 SDL_Flip(screen_);lastFlip_=sysGetSystemTime();paintedX_=pointerX();paintedY_=pointerY();paintedDown_=pointerDown();
}
void Framebuffer::refreshCursor(){
 if(!haveFrame_||!screen_||!converted_||!pointerVisible())return;
 if(pointerX()==paintedX_&&pointerY()==paintedY_&&pointerDown()==paintedDown_)return;
 if(sysGetSystemTime()-lastFlip_<33000)return;
 blitCursor();
}
static Framebuffer* activeFramebuffer=0;
void Framebuffer::presentRegion(int first,int last){if(first<=last&&last>=0&&first<height_)present();}
Controller::~Controller(){if(initialized_)ioPadEnd();}
bool Controller::ensureReady() {
    if(!initialized_){if(ioPadInit(MAX_PORT_NUM)!=PAD_OK)return false;initialized_=true;}
    padInfo info;std::memset(&info,0,sizeof(info));
    if(ioPadGetInfo(&info)!=PAD_OK)return false;
    if(port_>=0&&info.status[port_])return true;
    port_=-1;last_=ps2::hal::PadButtons();
    for(int p=0;p<MAX_PORT_NUM;++p)if(info.status[p]){port_=p;return true;}
    return false;
}
bool Controller::read(ps2::hal::PadButtons* out) {
    if(!out)return false;
    if(!ensureReady()){*out=ps2::hal::PadButtons();return true;} // release held keys on disconnect
    padData data;std::memset(&data,0,sizeof(data));
    if(ioPadGetData(port_,&data)!=PAD_OK)return false;
    // A zero-length pad report means no state change, not that all keys were released.
    if(data.len>0) {
        if(data.BTN_START&&!last_.start&&ps3me_on_start)ps3me_on_start();
        last_.up=data.BTN_UP;last_.down=data.BTN_DOWN;last_.left=data.BTN_LEFT;last_.right=data.BTN_RIGHT;
        last_.cross=data.BTN_CROSS;last_.circle=data.BTN_CIRCLE;
        last_.triangle=data.BTN_TRIANGLE;last_.square=data.BTN_SQUARE;
        last_.l1=data.BTN_L1;last_.r1=data.BTN_R1;last_.l2=data.BTN_L2;last_.r2=data.BTN_R2;
        last_.l3=data.BTN_L3;last_.r3=data.BTN_R3;last_.start=data.BTN_START;last_.select=data.BTN_SELECT;
        last_.lx=data.ANA_L_H;last_.ly=data.ANA_L_V;last_.rx=data.ANA_R_H;last_.ry=data.ANA_R_V;
    }
    *out=last_;return true;
}
javacall_int64 Clock::wallClockMillis(){u64 s=0,n=0;return sysGetCurrentTime(&s,&n)==0?static_cast<javacall_int64>(s*1000+n/1000000):0;}
javacall_int64 Clock::monotonicCounter(){return static_cast<javacall_int64>(sysGetSystemTime());}
javacall_int64 Clock::monotonicFrequency(){return 1000000;}
void Clock::sleep(javacall_uint64 ms){while(ms){unsigned int chunk=ms>1000?1000:static_cast<unsigned int>(ms);sysUsleep(chunk*1000);ms-=chunk;}}
void* Heap::allocate(unsigned int n){return ::malloc(n);}
void* Heap::reallocate(void* p,unsigned int n){return ::realloc(p,n);}
void Heap::deallocate(void* p){::free(p);}
Log::~Log(){if(file_)::fclose(file_);}
bool Log::open(const char* path){if(file_)::fclose(file_);file_=::fopen(path,"ab");return file_!=0;}
void Log::write(const char* p,int n){if(!p||n<=0)return;if(file_){::fwrite(p,1,n,file_);::fflush(file_);}::fwrite(p,1,n,stderr);}
Audio::Audio():head_(0),count_(0),open_(false){std::memset(samples_,0,sizeof(samples_));}
Audio::~Audio(){if(open_)SDL_CloseAudio();}
bool Audio::initialize(int rate) {
    if(open_)return true;
    if(rate<=0||SDL_InitSubSystem(SDL_INIT_AUDIO)!=0)return false;
    SDL_AudioSpec want;std::memset(&want,0,sizeof(want));
    want.freq=rate;want.format=AUDIO_S16SYS;want.channels=2;want.samples=1024;
    want.callback=&Audio::callback;want.userdata=this;
    if(SDL_OpenAudio(&want,0)!=0)return false;
    open_=true;SDL_PauseAudio(0);return true;
}
unsigned int Audio::enqueue(const int16_t* data,unsigned int frames) {
    if(!open_||!data)return 0;
    SDL_LockAudio();unsigned int accepted=frames<CAPACITY-count_?frames:CAPACITY-count_;
    for(unsigned int i=0;i<accepted;++i){unsigned int pos=((head_+count_+i)%CAPACITY)*2;samples_[pos]=data[i*2];samples_[pos+1]=data[i*2+1];}
    count_+=accepted;SDL_UnlockAudio();return accepted;
}
void Audio::callback(void* user,Uint8* data,int bytes) {
    Audio* self=static_cast<Audio*>(user);std::memset(data,0,bytes);
    int16_t* out=reinterpret_cast<int16_t*>(data);
    unsigned int n=static_cast<unsigned int>(bytes)/4;if(n>self->count_)n=self->count_;
    for(unsigned int i=0;i<n;++i){unsigned int pos=((self->head_+i)%CAPACITY)*2;out[i*2]=self->samples_[pos];out[i*2+1]=self->samples_[pos+1];}
    self->head_=(self->head_+n)%CAPACITY;self->count_-=n;
}
void installPlatform(Storage& storage,Framebuffer& video,Controller& pad,Clock& clock,Heap& heap,Log& log) {
    activeFramebuffer=&video;
    ps2::hal::GameStorage::instance().setBackend(&storage);
    ps2::hal::LcdDevice::instance().setFramebuffer(&video);
    ps2::hal::Keypad::instance().setPad(&pad);
    ps2::hal::EventQueue::instance().setPollHook(&ps2::hal::Keypad::instance());
    ps2::hal::SystemClock::instance().setClock(&clock);
    ps2::hal::MemoryManager::instance().setHeap(&heap);
    ps2::hal::Logger::instance().setSink(&log);
    installRuntimeServices();
}
}

extern "C" void ps3me_refresh_cursor(){if(ps3me::activeFramebuffer)ps3me::activeFramebuffer->refreshCursor();}
