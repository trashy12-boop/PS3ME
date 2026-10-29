/* SPDX-License-Identifier: GPL-2.0-only */
/* Exercise the production primitives, including the exact wide-store paths. */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../deps/phoneME/midp/src/lowlevelui/graphics/gx_putpixel/native/gxj_putpixel.c"
int main(void) {
    unsigned checks=0;
    for(int width=1;width<=67;width++)for(int shift=0;shift<4;shift++) {
        size_t bytes=(width*9+16)*2;
        unsigned char* allocation=malloc(bytes+8);
        gxj_pixel_type* raster=(gxj_pixel_type*)(allocation+shift*2);
        gxj_pixel_type* expected=calloc(width*9+16,2);
        gxj_screen_buffer screen;memset(&screen,0,sizeof(screen));
        screen.width=width;screen.height=9;screen.pixelData=raster;
        for(int x=0;x<width;x++)for(int count=1;count<=width-x;count++) {
            memset(raster,0,bytes);memset(expected,0,bytes);
            primDrawHorzLine(&screen,0xa53c,x,3,x+count-1,3);
            for(int i=0;i<count;i++)expected[3*width+x+i]=0xa53c;
            assert(!memcmp(raster,expected,bytes));checks++;
            memset(raster,0,bytes);memset(expected,0,bytes);
            primDrawFilledRect(&screen,0xc35a,x,1,x+count,8);
            for(int y=1;y<8;y++)for(int i=0;i<count;i++)expected[y*width+x+i]=0xc35a;
            assert(!memcmp(raster,expected,bytes));checks++;
        }
        free(allocation);free(expected);
    }
    printf("PASS: %u production fill comparisons; widths 1..67; offsets 0/2/4/6\n",checks);
    return 0;
}
