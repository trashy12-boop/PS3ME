#include "MidiEngine.hpp"
#include <vector>
#include <fstream>
#include <iterator>
#include <cassert>
#include <cmath>
#include <cstdio>
std::vector<char> data(const char*p){std::ifstream f(p,std::ios::binary);return std::vector<char>((std::istreambuf_iterator<char>(f)),{});}
int main(int argc,char**argv){assert(argc==4);ps3me::MidiEngine e;assert(e.initialize(argv[1],48000));auto a=data(argv[2]),b=data(argv[3]);auto bg=e.open(a.data(),a.size()),fx=e.open(b.data(),b.size());assert(bg&&fx);e.start(bg,-1);float pcm[1024],peak=0;double sum=0;for(int i=0;i<800;i++){if(i==30)e.start(fx,1);if(i==100){assert(bg->playing);e.close(fx);}if(i==200)e.rewind(bg);e.render(pcm,512);for(float v:pcm){assert(std::isfinite(v));peak=std::fmax(peak,std::fabs(v));sum+=v*v;}}assert(bg->playing&&peak>.01&&peak<1);e.volume(bg,0);e.render(pcm,512);for(float v:pcm)assert(v==0);e.close(bg);e.render(pcm,512);for(float v:pcm)assert(v==0);printf("PASS independent players / rewind / close / volume; peak=%f rms=%f\n",peak,sqrt(sum/819200));}
