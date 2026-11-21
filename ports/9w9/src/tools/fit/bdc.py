import numpy as np
fs=88200
def ld(f): d=np.fromfile(f,dtype=np.float32).reshape(-1,2); return d[:,0].astype(float),d[:,1].astype(float)
tri,cmp=ld('bd_t.raw'); c9,q7c=ld('bd_c.raw')
b=np.fromfile('bt.raw',dtype=np.float32).reshape(-1,4).astype(float)
pre=172*128*2  # board samples before the hit
j=int(np.argmax(b[:,0]>1.0)); j0=int(np.argmax(c9>1.0)); b=b[j-j0:]
print('trace len',len(b),'lemus',len(tri))
for ms in (0.1,1,5,12,16,18,20,22,25,30,40,60,100,200):
    n=int(ms*fs/1000)
    print('%5.1f ms  lemus c9 %6.3f tri %6.3f cmp %5.1f q7c %5.2f | beh c9 %6.3f tri %6.3f cmp %5.1f q7c %5.2f'%(ms,c9[n],tri[n],cmp[n],q7c[n],b[n,0],b[n,1],b[n,2],b[n,3]))
