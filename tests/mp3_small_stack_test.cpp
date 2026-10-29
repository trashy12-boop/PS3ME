// SPDX-License-Identifier: GPL-2.0-only
#include "Mp3Engine.hpp"
#include <pthread.h>
#include <vector>
#include <fstream>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <sys/resource.h>
struct Job{ps3me::Mp3Engine engine;std::vector<char> input;ps3me::Mp3Track* track;float output[2048];bool valid;};
static void* prepare(void* arg){Job*j=(Job*)arg;j->track=j->engine.open(j->input.data(),j->input.size());return 0;}
static void* callback(void* arg){Job*j=(Job*)arg;j->valid=true;unsigned blocks=0;while(j->engine.playing(j->track)&&blocks++<10000){for(float&v:j->output)v=0;j->engine.render(j->output,1024);for(float v:j->output)if(!std::isfinite(v))j->valid=false;}return 0;}
static void run(void*(*entry)(void*),Job*j,size_t stack){pthread_attr_t a;assert(!pthread_attr_init(&a));assert(!pthread_attr_setstacksize(&a,stack));assert(!pthread_attr_setguardsize(&a,4096));pthread_t t;assert(!pthread_create(&t,&a,entry,j));pthread_attr_destroy(&a);assert(!pthread_join(t,0));}
int main(int argc,char**argv){rlimit lim={0,0};setrlimit(RLIMIT_CORE,&lim);assert(argc>1);for(int i=1;i<argc;i++){Job*j=new Job();std::ifstream f(argv[i],std::ios::binary);j->input.assign(std::istreambuf_iterator<char>(f),{});run(prepare,j,256*1024);assert(j->track);j->engine.start(j->track,1);run(callback,j,16*1024);assert(j->valid&&!j->engine.playing(j->track));delete j;printf("PASS: 256 KiB decoder / guarded 16 KiB playback stack: %s\n",argv[i]);}}
