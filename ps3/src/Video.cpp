// SPDX-License-Identifier: GPL-2.0-only
#include "Video.hpp"
#include <cstring>
namespace ps3me {
Rect fit(int sw,int sh,int dw,int dh) {
    Rect r={0,0,0,0};
if(sw<=0||sh<=0||dw<=0||dh<=0)return r;

    if(static_cast<int64_t>(dw)*sh<=static_cast<int64_t>(dh)*sw){r.w=dw;
r.h=static_cast<int>(static_cast<int64_t>(dw)*sh/sw);
}
    else{r.h=dh;
r.w=static_cast<int>(static_cast<int64_t>(dh)*sw/sh);
}
    if(r.w<1)r.w=1;

    if(r.h<1)r.h=1;

    r.x=(dw-r.w)/2;
r.y=(dh-r.h)/2;
return r;

}
uint32_t rgb565ToXrgb(uint16_t p) {
    uint32_t r=(p>>11)&31,g=(p>>5)&63,b=p&31;

    return 0xff000000u|(((r<<3)|(r>>2))<<16)|(((g<<2)|(g>>4))<<8)|((b<<3)|(b>>2));

}
void scaleRgb565(const uint16_t* src,int sw,int sh,uint32_t* dst,int dw,int dh,int pitch) {
    if(!src||!dst||sw<=0||sh<=0||dw<=0||dh<=0||pitch<dw)return;

    // Integer quotient/remainder stepping is exactly floor(x*sw/r.w),
    // without a division per TV pixel. Repeated destination rows are copies.
    Rect r=fit(sw,sh,dw,dh);
    // Only letterbox pixels need clearing; image pixels are all overwritten.
    for(int y=0;y<dh;++y) {
        uint32_t* row=dst+y*pitch;
        if(y<r.y||y>=r.y+r.h) {
            for(int x=0;x<dw;++x)row[x]=0xff000000u;
        } else {
            for(int x=0;x<r.x;++x)row[x]=0xff000000u;
            for(int x=r.x+r.w;x<dw;++x)row[x]=0xff000000u;
        }
    }
    const int xStep=sw/r.w,xRem=sw%r.w;
    const int yStep=sh/r.h,yRem=sh%r.h;
    int sy=0,yError=0,previous=-1;
    for(int y=0;y<r.h;++y) {
        uint32_t* row=dst+(y+r.y)*pitch+r.x;
        if(sy==previous) {
            std::memcpy(row,row-pitch,r.w*sizeof(uint32_t));
        } else {
            int sx=0,xError=0,last=-1;uint32_t color=0;
            for(int x=0;x<r.w;++x) {
                if(sx!=last){color=rgb565ToXrgb(src[sy*sw+sx]);last=sx;}
                row[x]=color;
                sx+=xStep;xError+=xRem;
                if(xError>=r.w){xError-=r.w;++sx;}
            }
        }
        previous=sy;sy+=yStep;yError+=yRem;
        if(yError>=r.h){yError-=r.h;++sy;}
    }
}
void drawCursor(uint32_t* p,int w,int h,int pitch,Rect r,int x,int y,int sw,int sh,uint32_t dark,uint32_t light){
 if(!p||w<=0||h<=0||pitch<w||sw<=0||sh<=0||r.w<=0||r.h<=0)return;

 if(x<0)x=0;
if(y<0)y=0;
if(x>=sw)x=sw-1;
if(y>=sh)y=sh-1;

 int px=r.x+(int)((int64_t)x*r.w/sw),py=r.y+(int)((int64_t)y*r.h/sh);

 // The tip marks the exact Java ME pen coordinate. Arrow pixels touch only the
 // native output surface, never the game's raster or the saved scaled frame.
 static const char* shape[]={"B","BB","BWB","BWWB","BWWWB","BWWWWB","BWWWWWB","BWWWWWWB","BWWWWWWWB","BWWWWWWWWB","BWWWWWWWWWB","BWWWWWWWWWWB","BWWWWWWBBBBBB","BWWWBWWB","BWWB BWWB","BWB  BWWB","BB    BWWB","B     BWWB","       BB"};

 for(int j=0;
j<19;
j++)for(int i=0;
shape[j][i];
i++){char c=shape[j][i];
int dx=px+i,dy=py+j;
if(c!=' '&&dx>=0&&dy>=0&&dx<w&&dy<h)p[dy*pitch+dx]=c=='B'?dark:light;
}
}

}
