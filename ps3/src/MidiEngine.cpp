// SPDX-License-Identifier: GPL-2.0-only
#include "MidiEngine.hpp"
#define TSF_IMPLEMENTATION
#define TML_IMPLEMENTATION
#include "../../third_party/tinysoundfont/tsf.h"
#include "../../third_party/tinysoundfont/tml.h"
#include <cstring>
#include <algorithm>
#include <new>
namespace ps3me {
MidiEngine::MidiEngine():bank_(0),rate_(48000){std::memset(tracks_,0,sizeof(tracks_));}MidiEngine::~MidiEngine(){shutdown();}
bool MidiEngine::initialize(const char* p,int rate){if(bank_)return rate==rate_;rate_=rate;bank_=tsf_load_filename(p);return bank_!=0;}
void MidiEngine::reset(MidiTrack* t){for(int v=0;v<t->synth->voiceNum;v++)t->synth->voices[v].playingPreset=-1;for(int ch=0;ch<16;ch++){tsf_channel_midi_control(t->synth,ch,121,0);tsf_channel_set_bank(t->synth,ch,0);tsf_channel_set_presetnumber(t->synth,ch,0,ch==9);tsf_channel_set_pitchwheel(t->synth,ch,8192);}t->frame=0;t->next=t->events;}
MidiTrack* MidiEngine::open(const void* b,int size){if(!bank_||size<1||size>8388608)return 0;int i=0;while(i<32&&tracks_[i])i++;if(i==32)return 0;tml_message* e=tml_load_memory(b,size);if(!e)return 0;MidiTrack* t=new(std::nothrow)MidiTrack();if(!t){tml_free(e);return 0;}t->events=e;t->synth=tsf_copy(bank_);if(!t->synth||!tsf_set_max_voices(t->synth,64)){tsf_close(t->synth);tml_free(e);delete t;return 0;}tsf_set_output(t->synth,TSF_STEREO_INTERLEAVED,rate_,0);for(int ch=15;ch>=0;ch--)if(!tsf_channel_set_presetnumber(t->synth,ch,0,ch==9)){tsf_close(t->synth);tml_free(e);delete t;return 0;}tml_message* last=e;while(last->next)last=last->next;t->end=std::max<uint64_t>(1,(uint64_t)last->time*rate_/1000);if(t->end>(uint64_t)rate_*3600){tsf_close(t->synth);tml_free(e);delete t;return 0;}t->volume=100;reset(t);tsf_set_volume(t->synth,.35f);tracks_[i]=t;return t;}
void MidiEngine::close(MidiTrack* t){if(!t)return;for(int i=0;i<32;i++)if(tracks_[i]==t){tracks_[i]=0;tsf_close(t->synth);tml_free(t->events);delete t;return;}}
void MidiEngine::start(MidiTrack* t,int loops){if(t&&(loops==-1||loops>0)){reset(t);t->loops=loops;t->playing=true;}}void MidiEngine::stop(MidiTrack* t){if(t){t->playing=false;reset(t);}}void MidiEngine::rewind(MidiTrack* t){if(t)reset(t);}void MidiEngine::volume(MidiTrack* t,int v){if(t)t->volume=std::max(0,std::min(100,v));}
void MidiEngine::render(float* out,int frames){std::memset(out,0,frames*2*sizeof(float));for(int i=0;i<32;i++){MidiTrack* t=tracks_[i];if(!t||!t->playing)continue;int done=0;while(done<frames&&t->playing){while(t->next&&(uint64_t)t->next->time*rate_/1000<=t->frame){tml_message* e=t->next;int ch=e->channel;switch(e->type){case TML_PROGRAM_CHANGE:tsf_channel_set_presetnumber(t->synth,ch,(unsigned char)e->program,ch==9);break;case TML_NOTE_ON:tsf_channel_note_on(t->synth,ch,(unsigned char)e->key,(unsigned char)e->velocity/127.f);break;case TML_NOTE_OFF:tsf_channel_note_off(t->synth,ch,(unsigned char)e->key);break;case TML_PITCH_BEND:tsf_channel_set_pitchwheel(t->synth,ch,e->pitch_bend);break;case TML_CONTROL_CHANGE:tsf_channel_midi_control(t->synth,ch,(unsigned char)e->control,(unsigned char)e->control_value);break;}t->next=e->next;}if(!t->next&&t->frame>=t->end){if(t->loops==-1||t->loops>1){if(t->loops>1)--t->loops;reset(t);continue;}if(t->frame>=t->end+(uint64_t)rate_||!tsf_active_voice_count(t->synth)){t->playing=false;break;}}int n=std::min(64,frames-done);if(t->next)n=std::min<uint64_t>(n,(uint64_t)t->next->time*rate_/1000-t->frame);if(n<1)n=1;float scratch[128];tsf_render_float(t->synth,scratch,n,0);for(int k=0;k<n*2;k++)out[done*2+k]+=scratch[k]*t->volume/100.f;done+=n;t->frame+=n;}}}
void MidiEngine::shutdown(){for(int i=0;i<32;i++)close(tracks_[i]);if(bank_)tsf_close(bank_);bank_=0;}
}
