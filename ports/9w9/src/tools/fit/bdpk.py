import numpy as np
fs=88200
def ld(f): d=np.fromfile(f,dtype=np.float32).reshape(-1,2); return d[:,0].astype(float),d[:,1].astype(float)
tri,cmp=ld('bd_t.raw'); va,vb=ld('bd_v.raw')
b=np.fromfile('bt.raw',dtype=np.float32).reshape(-1,4).astype(float); c9,_=ld('bd_c.raw')
j=int(np.argmax(b[:,0]>1.0)); j0=int(np.argmax(c9>1.0)); bt=b[j-j0:j-j0+len(tri),1]; bc=b[j-j0:j-j0+len(tri),2]
def turns(x,c,lo,hi):
    f=np.where(np.diff(np.sign(c[lo:hi]))!=0)[0]+lo
    return [(i/fs*1e3,x[i],x[max(lo,i-30):i+30].max() if c[i+1]<0 else x[max(lo,i-30):i+30].min()) for i in f]
L=turns(tri,cmp,int(0.019*fs),int(0.06*fs)); M=turns(bt,bc,int(0.019*fs),int(0.06*fs))
for a,b2 in zip(L,M): print('lemus flip %.3f ms at tri %+.3f (peak %+.3f) | model %.3f ms at %+.3f (peak %+.3f)'%(a+b2))
