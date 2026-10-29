// SPDX-License-Identifier: GPL-2.0-only
#include "AudioBudget.hpp"
#include "MidiEngine.hpp"
#include <vector>
#include <cassert>
#include <cmath>
#include <cstdio>
static void put(std::vector<unsigned char>& b,int at,unsigned v,int n){for(int i=0;i<n;i++)b[at+i]=v>>(8*i);}
int main(int argc,char**argv){
 assert(argc==2);
 std::vector<unsigned char> wav(8044);put(wav,0,0x46464952,4);put(wav,8,0x45564157,4);put(wav,12,0x20746d66,4);put(wav,16,16,4);put(wav,20,1,2);put(wav,22,1,2);put(wav,24,8000,4);put(wav,32,1,2);put(wav,34,8,2);put(wav,36,0x61746164,4);put(wav,40,8000,4);
 assert(ps3me::waveFitsBudget(wav.data(),wav.size(),ps3me::AudioBudgetBytes));
 assert(!ps3me::waveFitsBudget(wav.data(),wav.size(),1000));
 put(wav,24,1,4);assert(!ps3me::waveFitsBudget(wav.data(),wav.size(),ps3me::AudioBudgetBytes));put(wav,24,8000,4);
 put(wav,40,0xffffffff,4);assert(!ps3me::waveFitsBudget(wav.data(),wav.size(),ps3me::AudioBudgetBytes));put(wav,40,8000,4);
 for(size_t n=0;n<wav.size();++n)assert(!ps3me::waveFitsBudget(wav.data(),n,ps3me::AudioBudgetBytes));
 const unsigned char midi[]={'M','T','h','d',0,0,0,6,0,0,0,1,0,96,'M','T','r','k',0,0,0,12,0,0x90,60,100,96,0x80,60,0,0,0xff,47,0};
 ps3me::MidiEngine e;float pcm[1024];
 for(int round=0;round<30;++round){assert(e.initialize(argv[1],48000));ps3me::MidiTrack* t[32];for(int i=0;i<32;++i){t[i]=e.open(midi,sizeof(midi));assert(t[i]);e.start(t[i],1);}assert(!e.open(midi,sizeof(midi)));for(int n=0;n<8;++n){e.render(pcm,512);for(float v:pcm)assert(std::isfinite(v));}for(int i=0;i<32;i+=2){e.close(t[i]);t[i]=e.open(midi,sizeof(midi));assert(t[i]);e.start(t[i],1);e.rewind(t[i]);}e.shutdown();e.shutdown();}
 puts("PASS: WAV memory preflight/truncation; 960 MIDI Player allocations; slot reuse; repeated shutdown");
}
