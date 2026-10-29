// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <cstddef>
#include <cstdint>
namespace ps3me {
static const unsigned AudioBudgetBytes=16*1024*1024;
inline unsigned wave16(const unsigned char* p){return unsigned(p[0])|(unsigned(p[1])<<8);}
inline uint32_t wave32(const unsigned char* p){return uint32_t(wave16(p))|(uint32_t(wave16(p+2))<<16);}
// Preflight PCM and IMA ADPCM WAVs before SDL allocates the expanded 48kHz
// stereo float buffer. Unknown encodings fail instead of allocating unboundedly.
inline bool waveFitsBudget(const unsigned char* p,size_t n,uint64_t remaining){
 if(n<12||wave32(p)!=0x46464952||wave32(p+8)!=0x45564157)return false;
 unsigned format=0,rate=0,align=0,channels=0,bits=0,samplesPerBlock=0;uint64_t bytes=0;
 for(size_t at=12;at+8<=n;){uint32_t length=wave32(p+at+4);size_t start=at+8;if(length>n-start)return false;
  uint32_t tag=wave32(p+at);if(tag==0x20746d66){if(length<16)return false;format=wave16(p+start);channels=wave16(p+start+2);rate=wave32(p+start+4);align=wave16(p+start+12);bits=wave16(p+start+14);if(format==17&&length>=20)samplesPerBlock=wave16(p+start+18);}
  if(tag==0x61746164)bytes+=length;
  size_t next=start+length;if(length&1){if(next==n)break;++next;}at=next;
 }
 if(!bytes||!rate||rate>192000||channels<1||channels>2||!align)return false;
 uint64_t frames=0;
 if(format==1){if((bits!=8&&bits!=16)||align!=channels*(bits/8)||bytes%align)return false;frames=bytes/align;}
 else if(format==17){if(bits!=4||align<4*channels||samplesPerBlock!=1+(align-4*channels)*2/channels)return false;frames=((bytes+align-1)/align)*samplesPerBlock;}
 else return false;
 return ((frames*48000+rate-1)/rate+4096)*8<=remaining;
}
}
