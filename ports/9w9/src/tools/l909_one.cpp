// r909 VOICE(0-9, 10 = open hat) CODE(0-63 accent DAC) OUT.raw: 1.5 s of that voice's jack at 44.1 kHz, ROMs from ~/roms/tr909
#include "tr909/core/voiceboard.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
using namespace tr909;
static const int kDac[11]={1,2,3,4,5,6,6,7,8,8,7}, kReg[11]={9,9,9,9,9,10,10,10,10,10,10}, kBit[11]={1,2,3,4,5,1,6,2,4,5,2};
static std::vector<uint8_t> rd(const std::string&p){ std::vector<uint8_t> d(32768); FILE*f=fopen(p.c_str(),"rb"); if(!f){perror(p.c_str());exit(1);} fread(d.data(),1,d.size(),f); fclose(f); return d; }
int main(int c,char**a){ int v=atoi(a[1]), code=atoi(a[2]); const int sr=44100,B=128; std::string h=std::string(getenv("HOME"))+"/roms/tr909/";
 auto hh=rd(h+"hn61256p__c43.ic69"), cr=rd(h+"hn61256p__c42.ic62"), ri=rd(h+"hn61256p__c44.ic54");
 VoiceBoard b; SampleRoms r; r.hihat=hh.data(); r.crash=cr.data(); r.ride=ri.data(); b.setSampleRoms(r);
 b.setCalibration(unitCalibration()); b.prepare(sr); b.reset();
 float L[B],R[B],jb[10][B]; float*jp[10]; for(int i=0;i<10;i++)jp[i]=jb[i]; int jack=v==10?7:v;
 for(int k=0;k<172;k++) b.render(L,R,B,jp);
 b.write(kDac[v],uint8_t(code<<1)); if(v==7||v==10) b.write(10,uint8_t((b.latch(10)&~0x08)|(v==10?0:0x08)));
 b.write(kReg[v],uint8_t(b.latch(kReg[v])|(1u<<kBit[v])));
 FILE*o=fopen(a[3],"wb"); for(int k=0;k<517;k++){ b.render(L,R,B,jp); if(k==7){b.write(9,0);b.write(10,uint8_t(b.latch(10)&0x08));} fwrite(jb[jack],4,B,o);} fclose(o); }
