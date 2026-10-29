// SPDX-License-Identifier: GPL-2.0-only
#include "Mp3Engine.hpp"
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_SIMD
#define MINIMP3_ONLY_MP3
#include "../../third_party/minimp3/minimp3.h"
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <new>
#include <cstdint>
namespace ps3me {
struct Mp3Track {
 mp3d_sample_t* pcm; size_t frames,bytes; int rate,channels,loops,volume;
 bool playing; uint64_t position;
};
struct DecodeWork {mp3dec_t decoder;mp3d_sample_t frame[MINIMP3_MAX_SAMPLES_PER_FRAME];};
Mp3Engine::Mp3Engine(){std::memset(tracks_,0,sizeof(tracks_));}
Mp3Engine::~Mp3Engine(){shutdown();}
Mp3Track* Mp3Engine::prepare(const void* input,size_t n,size_t budget){
 if(!input||n<4||n>8388608||budget<=sizeof(Mp3Track))return 0;
 const unsigned char* data=(const unsigned char*)input;
 DecodeWork* w=new(std::nothrow)DecodeWork();if(!w)return 0;
 // First scan counts frame samples without decoding. Reject before allocation.
 size_t offset=0,samples=0;int rate=0,channels=0;bool valid=true;
 mp3dec_init(&w->decoder);
 while(offset<n){mp3dec_frame_info_t info={};
  int frames=mp3dec_decode_frame(&w->decoder,data+offset,int(n-offset),0,&info);
  if(info.frame_bytes<=0)break;
  if(size_t(info.frame_bytes)>n-offset){valid=false;break;}offset+=info.frame_bytes;
  if(frames<=0)continue;
  if(info.layer!=3||info.channels<1||info.channels>2||info.hz<8000||info.hz>48000){valid=false;break;}
  if(!rate){rate=info.hz;channels=info.channels;}
  if(rate!=info.hz||channels!=info.channels){valid=false;break;}
  size_t count=size_t(frames)*channels;
  if(count>(budget-sizeof(Mp3Track))/sizeof(mp3d_sample_t)-samples){valid=false;break;}
  samples+=count;
 }
 if(!valid||!samples){delete w;return 0;}
 Mp3Track* t=new(std::nothrow)Mp3Track();if(!t){delete w;return 0;}
 t->pcm=(mp3d_sample_t*)std::malloc(samples*sizeof(mp3d_sample_t));
 if(!t->pcm){delete t;delete w;return 0;}
 t->bytes=samples*sizeof(mp3d_sample_t)+sizeof(Mp3Track);
 t->rate=rate;t->channels=channels;t->volume=100;
 offset=0;size_t written=0;mp3dec_init(&w->decoder);
 while(offset<n){mp3dec_frame_info_t info={};
  int frames=mp3dec_decode_frame(&w->decoder,data+offset,int(n-offset),w->frame,&info);
  if(info.frame_bytes<=0)break;
  if(size_t(info.frame_bytes)>n-offset){valid=false;break;}offset+=info.frame_bytes;
  if(frames<=0)continue;
  if(info.layer!=3||info.hz!=rate||info.channels!=channels||frames>MINIMP3_MAX_SAMPLES_PER_FRAME/channels){valid=false;break;}
  size_t count=size_t(frames)*channels;
  if(count>samples-written){valid=false;break;}
  std::memcpy(t->pcm+written,w->frame,count*sizeof(mp3d_sample_t));written+=count;
 }
 delete w;
 if(!valid||!written){discard(t);return 0;}
 t->frames=written/channels;return t;
}
void Mp3Engine::discard(Mp3Track* t){if(t){std::free(t->pcm);delete t;}}
size_t Mp3Engine::residentBytes(const Mp3Track* t){return t?t->bytes:0;}
bool Mp3Engine::attach(Mp3Track* t){if(!t)return false;for(int i=0;i<32;i++)if(tracks_[i]==t)return false;for(int i=0;i<32;i++)if(!tracks_[i]){tracks_[i]=t;return true;}return false;}
Mp3Track* Mp3Engine::open(const void* data,size_t n){
 int i=0;while(i<32&&tracks_[i])++i;if(i==32)return 0;
 Mp3Track* t=prepare(data,n,16*1024*1024);if(t&&!attach(t)){discard(t);return 0;}return t;
}
void Mp3Engine::close(Mp3Track* t){if(!t)return;for(int i=0;i<32;i++)if(tracks_[i]==t){tracks_[i]=0;discard(t);return;}}
void Mp3Engine::start(Mp3Track* t,int loops){if(!t||!loops||loops< -1)return;if(t->position>=uint64_t(t->frames)*48000)t->position=0;t->loops=loops;t->playing=true;}
void Mp3Engine::stop(Mp3Track* t){if(t)t->playing=false;}
void Mp3Engine::rewind(Mp3Track* t){if(t)t->position=0;}
void Mp3Engine::volume(Mp3Track* t,int v){if(t)t->volume=std::max(0,std::min(100,v));}
bool Mp3Engine::playing(Mp3Track* t)const{return t&&t->playing;}
// Audio thread: no decoder, allocations, waits, locks, file IO or large arrays.
void Mp3Engine::render(float* out,int frames){
 for(int k=0;k<32;k++){Mp3Track* t=tracks_[k];if(!t||!t->playing)continue;
  const uint64_t end=uint64_t(t->frames)*48000;const float gain=t->volume*(.01f/32768.f);
  for(int i=0;i<frames&&t->playing;i++){
   size_t at=size_t(t->position/48000),next=at+1;
   if(next>=t->frames)next=(t->loops==-1||t->loops>1)?0:at;
   float fraction=float(t->position%48000)*(1.f/48000.f);
   for(int c=0;c<2;c++){int ch=t->channels==1?0:c;float a=t->pcm[at*t->channels+ch],b=t->pcm[next*t->channels+ch];out[i*2+c]+=(a+(b-a)*fraction)*gain;}
   t->position+=t->rate;
   if(t->position>=end){if(t->loops==-1||t->loops>1){if(t->loops>1)--t->loops;t->position-=end;}else t->playing=false;}
  }
 }
}
void Mp3Engine::shutdown(){for(int i=0;i<32;i++)close(tracks_[i]);}
}
