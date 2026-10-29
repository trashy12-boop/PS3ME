// SPDX-License-Identifier: GPL-2.0-only
// Adapt the SDK's integer-only TiMidity to SDL's native PS3 float mixer.
#include <SDL.h>
#include <cstdlib>
#include <cstring>
extern "C" int ps3me_timidity_init_s16(int,int,int,int);
extern "C" int ps3me_timidity_play_s16(void*,int);
static bool floatOutput=false;
static int channels=0,capacity=0;
static Sint16* pcm=0;
extern "C" int Timidity_Init(int rate,int format,int count,int samples){
 std::free(pcm);pcm=0;floatOutput=(format==AUDIO_F32SYS);channels=count;capacity=samples;
 if(floatOutput){if(samples<=0||samples>65536||count<1||count>6)return -1;pcm=(Sint16*)std::calloc(samples*count,sizeof(Sint16));if(!pcm)return -1;}
 int r=ps3me_timidity_init_s16(rate,floatOutput?AUDIO_S16SYS:format,count,samples);
 if(r<0){std::free(pcm);pcm=0;}return r;
}
extern "C" int Timidity_PlaySome(void* output,int frames){
 if(!floatOutput)return ps3me_timidity_play_s16(output,frames);
 float* dst=(float*)output;int result=0;
 while(frames>0){int n=frames<capacity?frames:capacity;if(!pcm||n<=0)return -1;std::memset(pcm,0,n*channels*sizeof(Sint16));result=ps3me_timidity_play_s16(pcm,n);for(int i=0;i<n*channels;i++)*dst++=(float)pcm[i]/32768.0f;frames-=n;}
 return result;
}
extern "C" void ps3me_midi_pcm_shutdown(){std::free(pcm);pcm=0;capacity=0;floatOutput=false;}
