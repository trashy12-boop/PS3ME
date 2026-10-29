// SPDX-License-Identifier: GPL-2.0-only
#include "Mp3Engine.hpp"
#include <vector>
#include <fstream>
#include <cassert>
#include <cstdio>
int main(int argc,char**argv){size_t limit=16*1024*1024;for(int i=1;i<argc;i++){std::ifstream f(argv[i],std::ios::binary);std::vector<char>d((std::istreambuf_iterator<char>(f)),{});auto*t=ps3me::Mp3Engine::prepare(d.data(),d.size(),limit);assert(t);size_t bytes=ps3me::Mp3Engine::residentBytes(t);assert(bytes<=limit&&bytes>0);ps3me::Mp3Engine::discard(t);assert(!ps3me::Mp3Engine::prepare(d.data(),d.size(),bytes-1));assert(!ps3me::Mp3Engine::prepare(d.data(),d.size(),0));t=ps3me::Mp3Engine::prepare(d.data(),d.size(),bytes);assert(t);ps3me::Mp3Engine e;assert(e.attach(t));assert(!e.attach(t));e.close(t);e.shutdown();printf("PASS %s PCM resident=%zu; exact/insufficient/zero budget; unpublished cleanup; attach/close\n",argv[i],bytes);}}
