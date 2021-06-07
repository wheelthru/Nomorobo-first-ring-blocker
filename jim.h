#ifndef __JIM_H
#define __JIM_H
#define sp  Serial.print
#define spl Serial.println
#define sps Serial.print(" ")
#define spps(x) {sp(x);sps;}
#define TRUE 1
#define FALSE 0
#define spp sp(" ")
#define sp2(x,y) {sp(x);sp(y);}
#define spl2(x,y) {sp(x);spl(y);}
#endif
