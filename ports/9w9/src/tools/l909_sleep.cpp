#include "tr909/core/voiceboard.h"
#include <cstdio>
#include <cmath>
using namespace tr909;
static const int kDac[11]={1,2,3,4,5,6,6,7,8,8,7}, kReg[11]={9,9,9,9,9,10,10,10,10,10,10}, kBit[11]={1,2,3,4,5,1,6,2,4,5,2};
int main(){ const int sr=44100,B=128; VoiceBoard b; b.setCalibration(unitCalibration()); b.prepare(sr); b.reset();
 float L[B],R[B],jb[10][B]; float*jp[10]; for(int i=0;i<10;i++)jp[i]=jb[i];
 const char*nm[]={"BD","SD","LT","MT","HT","RS","HC","HH","CR","RD"};
 for(int k=0;k<172*5;k++) b.render(L,R,B,jp);
 printf("awake before hit:"); for(int v=0;v<10;v++) printf(" %s%d",nm[v],b.voiceAwake(Voice(v))); printf("\n");
 for(int v=0;v<10;v++){ b.write(kDac[v],80); b.write(kReg[v],uint8_t(b.latch(kReg[v])|(1u<<kBit[v]))); }
 double asleepAt[10]; double lastLoud[10]; for(int v=0;v<10;v++){asleepAt[v]=-1;lastLoud[v]=0;}
 for(int k=0;k<172*30;k++){ b.render(L,R,B,jp); if(k==1){b.write(9,0);b.write(10,0);}
  for(int v=0;v<10;v++){ for(int i=0;i<B;i++) if(fabs(jb[v][i])>2.5e-5f) lastLoud[v]=(k*B+i)/(double)sr;
   if(asleepAt[v]<0 && !b.voiceAwake(Voice(v))) asleepAt[v]=k*B/(double)sr; } }
 for(int v=0;v<10;v++) printf("%s: under -92 dB after %.2f s, asleep after %.2f s\n",nm[v],lastLoud[v],asleepAt[v]); }
