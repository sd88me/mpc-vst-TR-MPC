// r909p VOICE CODE PROBE0 PROBE1 OUT.raw: 1.5 s after a hit, the two probes at the board rate (float pairs); prints the rate
#include "tr909/core/voiceboard.h"
#include <cstdio>
#include <cstdlib>
using namespace tr909;
static const int kDac[11]={1,2,3,4,5,6,6,7,8,8,7}, kReg[11]={9,9,9,9,9,10,10,10,10,10,10}, kBit[11]={1,2,3,4,5,1,6,2,4,5,2};
int main(int c,char**a){ int v=atoi(a[1]),code=atoi(a[2]); const int sr=44100,B=128; VoiceBoard b; b.setCalibration(unitCalibration()); b.prepare(sr); b.reset();
 if(!b.setProbe(0,a[3])||!b.setProbe(1,a[4])){fprintf(stderr,"no such probe\n");return 1;} float L[B],R[B];
 for(int k=0;k<172;k++) b.render(L,R,B);
 b.write(kDac[v],uint8_t(code<<1)); b.write(kReg[v],uint8_t(b.latch(kReg[v])|(1u<<kBit[v])));
 FILE*o=fopen(a[5],"wb"); const int n=B*b.factor();
 for(int k=0;k<517;k++){ b.render(L,R,B); if(k==7){b.write(9,0);b.write(10,0);} const float*p0=b.probeSamples(0),*p1=b.probeSamples(1); for(int i=0;i<n;i++){fwrite(p0+i,4,1,o);fwrite(p1+i,4,1,o);} }
 fclose(o); printf("%g\n",b.internalRate()); }
