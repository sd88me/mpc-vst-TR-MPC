import numpy as np,sys
r=np.fromfile(sys.argv[1],dtype=np.float32); y=np.fromfile(sys.argv[2],dtype=np.float32); fr=441
e=lambda x: 10*np.log10(np.array([np.mean(x[i:i+fr]**2)+1e-30 for i in range(0,len(x)-fr,fr)]))
er,ey=e(r),e(y); pk=er.max()
for i in list(range(2,16))+list(range(20,150,10)):
    if er[i]>pk-50: print('%4d ms  ref %6.1f dB  diff %+5.2f'%(i*10,er[i]-pk,ey[i]-er[i]))
