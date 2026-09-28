#!/usr/bin/env python3
import math

def eig_matrix_cal(points):
    n=len(points)
    minx=min(x for x,y in points); maxx=max(x for x,y in points)
    miny=min(y for x,y in points); maxy=max(y for x,y in points)
    cx=(minx+maxx)/2; cy=(miny+maxy)/2
    sx=sum(x for x,y in points); sy=sum(y for x,y in points)
    sxx=sum(x*x for x,y in points); sxy=sum(x*y for x,y in points); syy=sum(y*y for x,y in points)
    cxx=(sxx-2*cx*sx+n*cx*cx)/n
    cyy=(syy-2*cy*sy+n*cy*cy)/n
    cxy=(sxy-cx*sy-cy*sx+n*cx*cy)/n
    tr=cxx+cyy; diff=cxx-cyy
    disc=math.sqrt(diff*diff+4*cxy*cxy)
    l1=(tr+disc)/2; l2=(tr-disc)/2
    th=0.5*math.atan2(2*cxy,diff); c=math.cos(th); s=math.sin(th)
    gain=math.sqrt(math.sqrt(l1*l2)); g1=gain/math.sqrt(l1); g2=gain/math.sqrt(l2)
    return cx,cy,c*c*g1+s*s*g2,c*s*(g1-g2),s*s*g1+c*c*g2

def circdiff(a,b): return (a-b+180)%360-180

def apply(A,v): return (A[0][0]*v[0]+A[0][1]*v[1],A[1][0]*v[0]+A[1][1]*v[1])
def mm(A,B): return [[sum(A[i][k]*B[k][j] for k in range(2)) for j in range(2)] for i in range(2)]
def rot(t):
    c=math.cos(t); s=math.sin(t); return [[c,-s],[s,c]]
R=rot(math.radians(28)); Rt=[[R[j][i] for j in range(2)] for i in range(2)]
A=mm(mm(R,[[1.45,0],[0,0.72]]),Rt)
pts=[]; truth=[]
for i in range(1440):
    a=2*math.pi*i/1440
    u=(1200*math.cos(a),1200*math.sin(a)); q=apply(A,u)
    pts.append((q[0]+380,q[1]-240)); truth.append(math.degrees(a)%360)
cx,cy,m00,m01,m11=eig_matrix_cal(pts)
errs=[]
for p,t in zip(pts,truth):
    dx=p[0]-cx; dy=p[1]-cy
    x=m00*dx+m01*dy; y=m01*dx+m11*dy
    h=math.degrees(math.atan2(y,x))%360
    errs.append(circdiff(h,t))
mx=max(abs(e) for e in errs); rms=math.sqrt(sum(e*e for e in errs)/len(errs))
assert mx < 0.01 and rms < 0.01, (mx,rms)
print(f'PASS full 2D matrix synthetic rotated ellipse max_err={mx:.6f} rms={rms:.6f}')
