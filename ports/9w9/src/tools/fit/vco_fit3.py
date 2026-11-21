import numpy as np, sys
fs=88200
def ld(f): d=np.fromfile(f,dtype=np.float32).reshape(-1,2); return d[:,0].astype(float),d[:,1].astype(float)
pre=sys.argv[1]; vdd,vss=ld(pre+'_rails.raw'); o_l,y_l=ld(pre+'_vco.raw'); a,o43a=ld(pre+'_a.raw')
R,C=47e3,float(sys.argv[2])
K=0.996
vs_hi=np.percentile(o43a,99.5); vs_lo=np.percentile(o43a,0.5)
0 and print('comparator rails %.2f / %.2f V, VDD %.2f..%.2f, VSS %.2f..%.2f'%(vs_hi,vs_lo,vdd.min(),vdd.max(),vss.min(),vss.max()))
reset=a>1.0
end=np.argmax(np.diff(reset.astype(int))<0)+1
0 and print('line A ends at %.2f ms'%(end/fs*1e3))
def model(useY,delay=0.0):
    # ideal: integrator from the inverter (K x the rail the comparator selects); comparator at
    # +-half its swing, switching `delay` after the crossing (sub-sample: the rest of the sample on the new rail)
    o=np.copy(o_l); hi=o43a[end-1]>0; dt=1/fs; t_sw=None
    for n in range(end,len(o)):
        ycur=K*(vss[n-1] if hi else vdd[n-1]); sl=-ycur/(R*C)
        on=o[n-1]+sl*dt
        if t_sw is None:
            thr=(vs_hi if hi else vs_lo)/2
            if (hi and on>thr) or ((not hi) and on<thr):
                t_sw=-(on-thr)/sl+delay   # from the sample's start... time after crossing
                t_sw=(thr-o[n-1])/sl+delay
        if t_sw is not None and t_sw<dt:
            rest=dt-t_sw; hi=not hi; ynew=K*(vss[n-1] if hi else vdd[n-1]); on=o[n-1]+sl*t_sw-ynew/(R*C)*rest; t_sw=None
        elif t_sw is not None:
            t_sw-=dt
        o[n]=on
    return o
def crossings(x):
    s=np.where((x[:-1]<0)&(x[1:]>=0))[0]; return s+(-x[s]/(x[s+1]-x[s]))
def pitch(x):
    c=crossings(x[end:]); f=fs/np.diff(c); t=(c[1:]+end)/fs; return t,f
tl,fl=pitch(o_l)
for useY,dl in ((False,10e-6),):
    om=model(useY,dl); tm,fm=pitch(om)
    n=min(len(fl),len(fm)); cents=1200*np.log2(fm[:n]/fl[:n])
    def at(ms): i=np.searchsorted(tl[:n],ms/1e3); return cents[min(i,n-1)]
    w=slice(end,end+int(0.1*fs)); e=np.sqrt(np.mean((om[w]-o_l[w])**2))/np.sqrt(np.mean(o_l[w]**2))
    # phase drift: cycles difference after 300 ms
    cl=crossings(o_l[end:]); cm=crossings(om[end:]); k=np.searchsorted(cl,0.3*fs)
    print('%s: pitch %.0f Hz at start; error at 10/50/200/500 ms: %+.1f %+.1f %+.1f %+.1f cents; waveform error first 100 ms %.1f%%; cycles in 300 ms: lemus %d model %d'%(
      pre,fl[0],at(10),at(50),at(200),at(500),100*e,k,np.searchsorted(cm,0.3*fs)))
