// raw TR-909 voicing board: a busy beat and an all-voices burst, per-block time vs the 128-frame budget
#include "tr909/core/voiceboard.h"
#include "tr909/core/parallel.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
using namespace tr909;
static const int kDac[11]={1,2,3,4,5,6,6,7,8,8,7}, kReg[11]={9,9,9,9,9,10,10,10,10,10,10}, kBit[11]={1,2,3,4,5,1,6,2,4,5,2};
int main(int argc,char**argv){ const int sr=44100,B=128; const double budget=1e6*B/sr;
 int workers=argc>1?atoi(argv[1]):0; unsigned mask=argc>2?strtoul(argv[2],0,0):0x7ff; Parallel*par=workers?new Parallel(workers):nullptr;
 VoiceBoard b; b.setCalibration(unitCalibration()); b.prepare(sr); b.reset(); if(par) b.setParallel(par);
 float L[B],R[B];
 auto hit=[&](int v){ if(!(mask>>v&1)) return; b.write(kDac[v],uint8_t(40<<1)); if(v==7||v==10) b.write(10,uint8_t((b.latch(10)&~0x08)|(v==10?0:0x08))); b.write(kReg[v],uint8_t(b.latch(kReg[v])|(1u<<kBit[v]))); };
 auto rel=[&](){ b.write(9,0); b.write(10,uint8_t(b.latch(10)&0x08)); };
 auto stage=[&](const char*nm,int blocks,auto pat){ std::vector<double> t; for(int k=0;k<blocks;k++){ bool h=pat(k,t.size()); (void)h;
   auto t0=std::chrono::steady_clock::now(); b.render(L,R,B); t.push_back(std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-t0).count()); rel(); }
   double m=0; for(double x:t)m+=x; m/=t.size(); auto s=t; std::sort(s.begin(),s.end());
   printf("%-8s mean %5.1f%% p99 %5.1f%% max %5.1f%%\n",nm,100*m/budget,100*s[s.size()*99/100]/budget,100*s.back()/budget); };
 stage("idle",700,[&](int,size_t){return false;});
 // 16ths at 125 BPM = 41.3 blocks; step every 41 blocks
 stage("beat",41*64,[&](int k,size_t){ if(k%41) return false; int st=(k/41)%16;
   if(st%4==0) hit(0); if(st==4||st==12){hit(1);hit(6);} if(st%2==0) hit(7); else hit(10); hit(9);
   if(st==14) hit(2); if(st==15) hit(4); if(st==0&&(k/41)%64==0) hit(8); return true;});
 stage("all10",172*8,[&](int k,size_t){ if(k%172) return false; for(int v=0;v<10;v++) hit(v); return true;});
 stage("tail",172*6,[&](int,size_t){return false;});

 }
