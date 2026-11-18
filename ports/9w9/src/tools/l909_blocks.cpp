#include "tr909/core/voiceboard.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
using namespace tr909;
static const int kDac[11]={1,2,3,4,5,6,6,7,8,8,7}, kReg[11]={9,9,9,9,9,10,10,10,10,10,10}, kBit[11]={1,2,3,4,5,1,6,2,4,5,2};
int main(int c,char**a){ int v=atoi(a[1]); const int sr=44100,B=128; const double bud=1e6*B/sr; VoiceBoard b; b.setCalibration(unitCalibration()); b.prepare(sr); b.reset(); float L[B],R[B];
 for(int k=0;k<400;k++) b.render(L,R,B);
 for(int rep=0;rep<3;rep++){ b.write(kDac[v],uint8_t(63<<1)); b.write(kReg[v],uint8_t(b.latch(kReg[v])|(1u<<kBit[v])));
  double tot=0; int over=0; printf("hit %d:",rep);
  for(int k=0;k<300;k++){ auto t0=std::chrono::steady_clock::now(); b.render(L,R,B); double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-t0).count(); if(k==1){b.write(9,0);b.write(10,0);}
   tot+=us; if(k<24) printf(" %.0f",100*us/bud); else if(100*us/bud>15){ over++; } }
  printf("  | later blocks >15%%: %d of 276, mean %.1f%%\n",over,100*tot/300/bud); } }
