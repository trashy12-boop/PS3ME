// SPDX-License-Identifier: GPL-2.0-only
#include <SDL.h>
#include <SDL_mixer.h>
#include "MidiEngine.hpp"
#include "Mp3Engine.hpp"
#include "AudioBudget.hpp"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/thread.h>
#include <sys/systime.h>
extern "C" {


#include <kni.h>
}
#include <NativesTable.hpp>
namespace {
// The SDK SDL audio thread has only 16 KiB stack. minimp3 needs >17 KiB.
// Decode on a dedicated joinable worker; only publish immutable PCM to audio.
struct DecodeJob {const void* data;size_t size,budget;ps3me::Mp3Track* result;unsigned done;};
static void decodeEntry(void* argument){DecodeJob* job=(DecodeJob*)argument;
 job->result=ps3me::Mp3Engine::prepare(job->data,job->size,job->budget);
 __atomic_store_n(&job->done,1u,__ATOMIC_RELEASE);sysThreadExit(0);
}
static ps3me::Mp3Track* prepareMp3(const void* data,size_t n,size_t budget){
 DecodeJob job={data,n,budget,0,0};sys_ppu_thread_t thread;char name[]="PS3ME MP3 decode";
 int code=sysThreadCreate(&thread,decodeEntry,&job,1501,256*1024,THREAD_JOINABLE,name);
 if(code){std::fprintf(stderr,"[PS3ME audio] decoder worker create failed=%d\n",code);return 0;}
 u64 result=0;code=sysThreadJoin(thread,&result);
 // Even on a join error, retain job/input lifetime until the worker finishes.
 while(!__atomic_load_n(&job.done,__ATOMIC_ACQUIRE))sysUsleep(1000);
 if(code)std::fprintf(stderr,"[PS3ME audio] decoder worker join=%d\n",code);
 return job.result;
}
struct Slot {ps3me::Mp3Track* mp3;ps3me::MidiTrack* midi;Mix_Chunk* chunk;int channel,loops;SDL_RWops* rw;unsigned char* data;int token,volume;bool playing;unsigned decodedBytes;char path[160];};
ps3me::MidiEngine engine;ps3me::Mp3Engine mp3;
SDL_mutex* playerMutex=0;
struct PlayerGuard { bool locked; PlayerGuard():locked(playerMutex && SDL_LockMutex(playerMutex)==0){} ~PlayerGuard(){if(locked)SDL_UnlockMutex(playerMutex);} };
unsigned decodedTotal=0;bool bankReady=false;
static unsigned mixed=0,nonzero=0,limited=0,invalid=0;
static void music(void*,Uint8* b,int n){engine.render((float*)b,n/8);mp3.render((float*)b,n/8);}
static void postmix(void*,Uint8* b,int n){mixed+=n;float* pcm=(float*)b;for(int i=0;i<n/4;i++){float v=pcm[i];if(!std::isfinite(v)){v=0;invalid++;}else if(std::fabs(v)>1.f)limited++;v=std::max(-1.f,std::min(1.f,v));pcm[i]=v;if(v)nonzero++;}}
// Doodle Jump 1.8.2 keeps a cache of 27 sound Players. Keep enough native
// Player slots and SDL channels for that cache, with a little spare capacity.
Slot slots[32]={};int serial=0;bool ready=false;
Slot* find(int id){for(int i=0;i<32;i++)if(slots[i].token==id&&id>0)return &slots[i];return 0;}
void release(Slot& s){if(s.token&&ready){SDL_LockAudio();unsigned n=nonzero;SDL_UnlockAudio();std::fprintf(stderr,"[PS3ME audio] closing handle=%d cumulative nonzero=%u\n",s.token,n);}if(s.mp3){SDL_LockAudio();mp3.close(s.mp3);SDL_UnlockAudio();}if(s.midi){SDL_LockAudio();engine.close(s.midi);SDL_UnlockAudio();}if(s.decodedBytes<=decodedTotal)decodedTotal-=s.decodedBytes;if(s.chunk){if(s.channel>=0&&Mix_GetChunk(s.channel)==s.chunk)Mix_HaltChannel(s.channel);Mix_FreeChunk(s.chunk);}if(s.rw)SDL_FreeRW(s.rw);std::free(s.data);if(s.path[0])std::remove(s.path);s=Slot();}
}
extern "C" KNIEXPORT KNI_RETURNTYPE_INT Java_com_ps3me_media_AudioPlayer_open(){
 PlayerGuard guard;int result=-1;if(!guard.locked)KNI_ReturnInt(-1);KNI_StartHandles(1);KNI_DeclareHandle(input);KNI_GetParameterAsObject(1,input);
 int size=KNI_GetArrayLength(input);
 if(size>0&&size<=8388608){
  if(!ready){if(SDL_InitSubSystem(SDL_INIT_AUDIO)==0&&Mix_OpenAudio(48000,AUDIO_F32SYS,2,1024)==0){int rate=0,ch=0;Uint16 fmt=0;Mix_QuerySpec(&rate,&fmt,&ch);if(fmt==AUDIO_F32SYS&&ch==2){ready=true;std::fprintf(stderr,"[PS3ME audio] rate=%d format=%x MIDI deferred\n",rate,fmt);Mix_AllocateChannels(32);Mix_HookMusic(music,0);Mix_SetPostMix(postmix,0);}else Mix_CloseAudio();}}
  if(ready)for(int i=0;i<32;i++)if(!slots[i].token){Slot& s=slots[i];s.data=(unsigned char*)std::malloc(size);if(!s.data)break;KNI_GetRawArrayRegion(input,0,size,(jbyte*)s.data);s.rw=SDL_RWFromMem(s.data,size);s.channel=-1;s.loops=1;s.playing=false;if(s.rw&&size>=12&&!std::memcmp(s.data,"RIFF",4)){if(ps3me::waveFitsBudget(s.data,size,ps3me::AudioBudgetBytes-decodedTotal))s.chunk=Mix_LoadWAV_RW(s.rw,0);if(s.chunk){if(s.chunk->alen>ps3me::AudioBudgetBytes-decodedTotal){Mix_FreeChunk(s.chunk);s.chunk=0;}else{s.decodedBytes=s.chunk->alen;decodedTotal+=s.decodedBytes;}}}else if(size>=4&&!std::memcmp(s.data,"MThd",4)){
    if(!bankReady){SDL_LockAudio();bankReady=engine.initialize("/dev_hdd0/game/PS3ME0001/USRDIR/soundfont/TimGM6mb.sf2",48000);SDL_UnlockAudio();}if(bankReady&&size<=1048576){SDL_LockAudio();s.midi=engine.open(s.data,size);SDL_UnlockAudio();}
   }else if(((size>=3&&!std::memcmp(s.data,"ID3",3))||(size>=2&&s.data[0]==255&&(s.data[1]&224)==224))){
    // Never decode or create/join a worker while holding SDL's audio lock.
    s.mp3=prepareMp3(s.data,size,ps3me::AudioBudgetBytes-decodedTotal);
    if(s.mp3){SDL_LockAudio();bool attached=mp3.attach(s.mp3);SDL_UnlockAudio();if(!attached){ps3me::Mp3Engine::discard(s.mp3);s.mp3=0;}else{s.decodedBytes=ps3me::Mp3Engine::residentBytes(s.mp3);decodedTotal+=s.decodedBytes;std::fprintf(stderr,"[PS3ME audio] MP3 PCM resident=%u budget=%u\n",s.decodedBytes,decodedTotal);}}
   }if(!s.midi&&!s.chunk&&!s.mp3){std::fprintf(stderr,"[PS3ME audio] unsupported/invalid media\n");release(s);break;}if(s.rw){SDL_FreeRW(s.rw);s.rw=0;}std::free(s.data);s.data=0;if(serial==2147483647)serial=0;s.token=++serial;s.volume=100;result=s.token;std::fprintf(stderr,"[PS3ME audio] loaded handle=%d bytes=%d\n",result,size);break;}
 }
 KNI_EndHandles();KNI_ReturnInt(result);
}
extern "C" KNIEXPORT KNI_RETURNTYPE_INT Java_com_ps3me_media_AudioPlayer_command(){
 PlayerGuard guard;if(!guard.locked)KNI_ReturnInt(-1);int id=KNI_GetParameterAsInt(1),op=KNI_GetParameterAsInt(2),value=KNI_GetParameterAsInt(3),r=-1;Slot* s=find(id);
 if(s){if(s->mp3){SDL_LockAudio();switch(op){case 1:mp3.start(s->mp3,value);r=0;break;case 2:mp3.stop(s->mp3);r=0;break;case 4:mp3.volume(s->mp3,value);r=0;break;case 5:r=mp3.playing(s->mp3);break;case 6:mp3.rewind(s->mp3);r=0;break;}SDL_UnlockAudio();}
 else if(s->midi){SDL_LockAudio();switch(op){case 1:if(value==-1||value>0){engine.start(s->midi,value);r=0;}break;case 2:engine.stop(s->midi);r=0;break;case 4:engine.volume(s->midi,value);r=0;break;case 5:r=s->midi->playing;break;case 6:engine.rewind(s->midi);r=0;break;}SDL_UnlockAudio();}
 else switch(op){case 1:if(value==-1||value>0){s->loops=value;Mix_VolumeChunk(s->chunk,s->volume*MIX_MAX_VOLUME/100);s->channel=Mix_PlayChannel(-1,s->chunk,value==-1?-1:value-1);s->playing=s->channel>=0;r=s->playing?0:-1;}break;case 2:if(s->channel>=0&&Mix_GetChunk(s->channel)==s->chunk)Mix_HaltChannel(s->channel);s->channel=-1;s->playing=false;r=0;break;case 4:s->volume=std::max(0,std::min(100,value));Mix_VolumeChunk(s->chunk,s->volume*MIX_MAX_VOLUME/100);r=0;break;case 5:r=s->channel>=0&&Mix_GetChunk(s->channel)==s->chunk&&Mix_Playing(s->channel);s->playing=r!=0;break;case 6:if(s->playing&&s->chunk){if(s->channel>=0&&Mix_GetChunk(s->channel)==s->chunk)Mix_HaltChannel(s->channel);Mix_VolumeChunk(s->chunk,s->volume*MIX_MAX_VOLUME/100);s->channel=Mix_PlayChannel(-1,s->chunk,s->loops==-1?-1:s->loops-1);s->playing=s->channel>=0;r=s->playing?0:-1;}else r=0;break;}
 if(op==3){release(*s);r=0;}}
 KNI_ReturnInt(r);
}
extern const JvmNativesTable ps3me_original_natives_table[];
JvmNativesTable audioNativeTable[512] __asm__("jvm_natives_table");
static const JvmNativeFunction audioMethods[]={
 JVM_NATIVE("open","([B)I",Java_com_ps3me_media_AudioPlayer_open),
 JVM_NATIVE("command","(III)I",Java_com_ps3me_media_AudioPlayer_command),{0,0,0}};
extern "C" void ps3me_audio_register_natives(){if(!playerMutex)playerMutex=SDL_CreateMutex();int i=0;bool haveAudio=false;for(;i<510&&ps3me_original_natives_table[i].name;i++){audioNativeTable[i]=ps3me_original_natives_table[i];if(!std::strcmp(audioNativeTable[i].name,"com/ps3me/media/AudioPlayer"))haveAudio=true;}if(!haveAudio)audioNativeTable[i++]=JvmNativesTable{(char*)"com/ps3me/media/AudioPlayer",audioMethods,0};audioNativeTable[i]=JvmNativesTable{0,0,0};}
extern "C" void ps3me_audio_shutdown(){PlayerGuard guard;if(!guard.locked)return;if(ready){Mix_HookMusic(0,0);Mix_SetPostMix(0,0);}for(int i=0;i<32;i++)release(slots[i]);if(ready){Mix_HookMusic(0,0);Mix_SetPostMix(0,0);Mix_CloseAudio();engine.shutdown();mp3.shutdown();bankReady=false;decodedTotal=0;std::fprintf(stderr,"[PS3ME audio] mixed bytes=%u nonzero samples=%u limited=%u invalid=%u\n",mixed,nonzero,limited,invalid);SDL_QuitSubSystem(SDL_INIT_AUDIO);ready=false;}}
