#!/usr/bin/env python3
import math, random

# Synthetic test: rotated ellipse, hard-iron offset, deliberately nonuniform
# dwell time, Gaussian noise and a few outliers. The geometric fit should be
# much less sensitive to dwell weighting than covariance whitening.
try:
    import numpy as np
except Exception:
    print('SKIP: numpy not available')
    raise SystemExit(0)

rng=np.random.default_rng(7)
n=1800
t=np.concatenate([
    rng.uniform(0,2*np.pi,int(n*0.45)),
    rng.normal(np.deg2rad(70),0.16,int(n*0.35)),
    rng.normal(np.deg2rad(245),0.12,n-int(n*0.80)),
])%(2*np.pi)
rot=np.deg2rad(28)
R=np.array([[np.cos(rot),-np.sin(rot)],[np.sin(rot),np.cos(rot)]])
pts=np.stack([1180*np.cos(t),720*np.sin(t)],axis=1)@R.T
pts+=np.array([320.0,-210.0])
pts+=rng.normal(0,5,pts.shape)
for _ in range(4):
    i=rng.integers(0,len(pts)); pts[i]+=rng.normal(0,220,2)

# robust provisional center from grouped extrema
k=max(12,min(32,len(pts)//40))
xs=np.sort(pts[:,0]); ys=np.sort(pts[:,1])
c0=np.array([(xs[:k].mean()+xs[-k:].mean())/2,
             (ys[:k].mean()+ys[-k:].mean())/2])
scale=max(xs[-k:].mean()-xs[:k].mean(), ys[-k:].mean()-ys[:k].mean())/2
ang=np.arctan2(pts[:,1]-c0[1],pts[:,0]-c0[0])%(2*np.pi)
bi=np.floor(ang/(2*np.pi)*36).astype(int)
reps=[]
for b in range(36):
    q=pts[bi==b]
    if len(q): reps.append(q.mean(axis=0))
reps=np.array(reps)
assert len(reps)>=24
u=(reps-c0)/scale
X=np.column_stack([u[:,0]**2,u[:,0]*u[:,1],u[:,1]**2,u[:,0],u[:,1]])
p=np.linalg.lstsq(X,np.ones(len(u)),rcond=None)[0]
A,B,C,D,E=p
Q=np.array([[A,B/2],[B/2,C]])
l=np.array([D,E])
dc=-0.5*np.linalg.solve(Q,l)
center=c0+dc*scale
kappa=1+dc@Q@dc
Q=Q/kappa
w,V=np.linalg.eigh(Q)
assert (w>0).all()
M=V@np.diag(np.sqrt(w))@V.T
M*=np.linalg.det(M)**-0.5
corr=(pts-center)@M.T
h=np.arctan2(corr[:,1],corr[:,0])
off=np.angle(np.mean(np.exp(1j*(h-t))))
err=np.angle(np.exp(1j*(h-t-off)))
core=np.abs(np.rad2deg(err))
print('estimated center:', center)
print('true center:     ', np.array([320.0,-210.0]))
print('median abs heading error:', float(np.median(core)))
print('p90 abs heading error:   ', float(np.percentile(core,90)))
assert np.linalg.norm(center-np.array([320.0,-210.0])) < 25
assert np.percentile(core,90) < 3.0
print('PASS compass geometric calibration synthetic test')
