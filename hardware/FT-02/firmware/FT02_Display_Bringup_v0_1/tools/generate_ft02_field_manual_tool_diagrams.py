#!/usr/bin/env python3
from pathlib import Path
from PIL import Image, ImageDraw
import math

W,H=688,184
S=2
OUT=Path(__file__).resolve().parent/'field_manual_media_sources'
OUT.mkdir(parents=True,exist_ok=True)

def C(p): return tuple(int(v*S) for v in p)
def new(): return Image.new('L',(W*S,H*S),255)
def line(d,pts,w=4): d.line([C(p) for p in pts],fill=0,width=w*S,joint='curve')
def arc(d,box,start,end,w=4): d.arc(C(box),start,end,fill=0,width=w*S)
def circ(d,xy,r,w=4,fill=None): d.ellipse((int((xy[0]-r)*S),int((xy[1]-r)*S),int((xy[0]+r)*S),int((xy[1]+r)*S)),outline=0,width=w*S,fill=fill)
def rect(d,box,w=3,fill=None): d.rectangle(C((box[0],box[1],box[2],box[3])),outline=0,width=w*S,fill=fill)
def arrow(d,a,b,w=3):
    line(d,[a,b],w); ang=math.atan2(b[1]-a[1],b[0]-a[0]); L=9
    for da in (2.55,-2.55):
        p=(b[0]+L*math.cos(ang+da),b[1]+L*math.sin(ang+da)); line(d,[b,p],w)
def dashed(d,pts,w=3):
    # simple segment dashes along polyline
    for a,b in zip(pts,pts[1:]):
        dx,dy=b[0]-a[0],b[1]-a[1]; L=math.hypot(dx,dy); ux,uy=dx/L,dy/L
        t=0
        while t<L:
            t2=min(t+8,L); line(d,[(a[0]+ux*t,a[1]+uy*t),(a[0]+ux*t2,a[1]+uy*t2)],w); t+=14

def stage_boxes(d,n=4):
    gap=8; m=8; bw=(W-2*m-(n-1)*gap)//n
    for i in range(n):
        x=m+i*(bw+gap); rect(d,(x,8,x+bw,H-8),2); circ(d,(x+15,23),11,2,fill=255)
        # number as strokes, not font
        if i==0: line(d,[(x+15,17),(x+15,29)],3)
        elif i==1: line(d,[(x+10,19),(x+14,16),(x+20,19),(x+10,29),(x+21,29)],3)
        elif i==2: line(d,[(x+10,18),(x+15,16),(x+20,19),(x+15,23),(x+20,26),(x+15,30),(x+10,27)],3)
        else: line(d,[(x+19,16),(x+10,25),(x+22,25),(x+19,25),(x+19,30)],3)
    return [(m+i*(bw+gap),8,bw,H-16) for i in range(n)]

def finish(im,name):
    im=im.resize((W,H),Image.Resampling.LANCZOS)
    # Strong 1-bit threshold suited to e-paper.
    im=im.point(lambda p: 0 if p<170 else 255,mode='1').convert('L')
    im.save(OUT/name)

# 1 square/reef knot: two line styles, sequence topology.
im=new(); d=ImageDraw.Draw(im); boxes=stage_boxes(d)
for idx,(x,y,bw,bh) in enumerate(boxes):
    cx=x+bw/2; cy=95
    if idx==0:
        line(d,[(x+22,120),(cx,80),(x+bw-22,120)],6); dashed(d,[(x+22,70),(cx,110),(x+bw-22,70)],4); arrow(d,(cx+25,55),(cx+5,75),3)
    elif idx==1:
        line(d,[(x+20,116),(cx-8,92),(cx+8,92),(x+bw-20,116)],6); dashed(d,[(x+22,68),(cx-10,93),(cx+10,93),(x+bw-22,68)],4)
    elif idx==2:
        line(d,[(x+20,68),(cx-8,94),(cx+9,94),(x+bw-20,68)],6); dashed(d,[(x+20,116),(cx-9,93),(cx+8,93),(x+bw-20,116)],4); arrow(d,(cx-28,54),(cx-6,72),3)
    else:
        # flat knot centre + parallel tails
        line(d,[(x+18,78),(cx-18,94),(cx,86),(cx+18,94),(x+bw-18,78)],6)
        dashed(d,[(x+18,112),(cx-18,94),(cx,102),(cx+18,94),(x+bw-18,112)],4)
finish(im,'tool_01_flat_knot.png')

# 2 bowline
im=new(); d=ImageDraw.Draw(im); boxes=stage_boxes(d)
for idx,(x,y,bw,bh) in enumerate(boxes):
    cx=x+bw/2
    if idx==0:
        line(d,[(x+25,145),(cx-5,112)],5); arc(d,(cx-28,62,cx+28,118),20,335,5); line(d,[(cx+26,90),(x+bw-20,55)],5)
    elif idx==1:
        arc(d,(cx-28,64,cx+28,120),20,335,5); line(d,[(cx+2,146),(cx+2,87)],5); arrow(d,(cx+2,132),(cx+2,95),3)
    elif idx==2:
        arc(d,(cx-28,64,cx+28,120),20,335,5); line(d,[(cx+2,144),(cx+2,89),(cx+36,72),(cx+52,96)],5); arrow(d,(cx+34,72),(cx+51,91),3)
    else:
        arc(d,(cx-42,58,cx+42,142),25,335,5); circ(d,(cx+8,91),15,5); line(d,[(cx+22,94),(x+bw-18,61)],5); line(d,[(cx+4,106),(x+bw-18,135)],5)
finish(im,'tool_02_bowline.png')

# 3 clove hitch around post
im=new(); d=ImageDraw.Draw(im); boxes=stage_boxes(d)
for idx,(x,y,bw,bh) in enumerate(boxes):
    cx=x+bw/2; rect(d,(cx-10,40,cx+10,150),3)
    if idx==0:
        arc(d,(cx-45,65,cx+45,115),200,520,5); line(d,[(x+18,86),(cx-40,88)],5); line(d,[(cx+38,88),(x+bw-18,60)],5)
    elif idx==1:
        arc(d,(cx-44,54,cx+44,104),200,520,5); arc(d,(cx-44,92,cx+44,142),20,340,5); line(d,[(x+18,74),(cx-40,79)],5); line(d,[(cx+40,117),(x+bw-18,136)],5)
    elif idx==2:
        line(d,[(x+20,65),(cx-32,88),(cx+32,116),(x+bw-18,139)],5); line(d,[(x+20,136),(cx-32,116),(cx+32,88),(x+bw-18,64)],5); arrow(d,(cx+40,45),(cx+14,76),3)
    else:
        line(d,[(x+16,65),(cx-34,90),(cx+34,116),(x+bw-16,137)],5); line(d,[(x+16,137),(cx-34,115),(cx+34,89),(x+bw-16,65)],5); circ(d,(cx,102),21,4)
finish(im,'tool_03_clove_hitch.png')

# 4 trucker's hitch
im=new(); d=ImageDraw.Draw(im); boxes=stage_boxes(d)
for idx,(x,y,bw,bh) in enumerate(boxes):
    if idx==0:
        line(d,[(x+15,75),(x+bw-15,75)],5); circ(d,(x+bw*0.62,105),16,4); line(d,[(x+bw*0.62,89),(x+bw*0.62,75)],4)
    elif idx==1:
        line(d,[(x+15,75),(x+bw*0.55,75)],5); circ(d,(x+bw*0.55,105),16,4); rect(d,(x+bw-30,55,x+bw-18,135),3); line(d,[(x+bw*0.55,121),(x+bw-24,120)],5); arc(d,(x+bw-45,90,x+bw-5,145),280,80,5)
    elif idx==2:
        line(d,[(x+15,75),(x+bw*0.47,75)],5); circ(d,(x+bw*0.47,105),16,4); rect(d,(x+bw-30,55,x+bw-18,135),3); line(d,[(x+bw*0.47,121),(x+bw-24,120),(x+bw*0.47,105)],5); arrow(d,(x+bw*0.43,145),(x+bw*0.22,145),3)
    else:
        line(d,[(x+14,75),(x+bw*0.50,75)],5); circ(d,(x+bw*0.50,105),15,4); rect(d,(x+bw-30,55,x+bw-18,135),3); line(d,[(x+bw*0.50,120),(x+bw-24,120),(x+bw*0.50,105)],5); circ(d,(x+bw*0.30,79),9,4); circ(d,(x+bw*0.18,79),8,4)
finish(im,'tool_04_truckers_hitch.png')

# 5 tripod lashing: side-by-side stages
im=new(); d=ImageDraw.Draw(im); boxes=stage_boxes(d,3)
for idx,(x,y,bw,bh) in enumerate(boxes):
    if idx==0:
        for off in (-26,0,26): line(d,[(x+bw/2+off,45),(x+bw/2+off,145)],8)
        for yy in (72,80,88,96): line(d,[(x+bw/2-42,yy),(x+bw/2+42,yy)],3)
    elif idx==1:
        for off in (-26,0,26): line(d,[(x+bw/2+off,45),(x+bw/2+off,145)],8)
        for yy in (72,80,88,96): line(d,[(x+bw/2-42,yy),(x+bw/2+42,yy)],3)
        # frapping between poles
        for xx in (x+bw/2-13,x+bw/2+13): circ(d,(xx,85),10,3)
    else:
        top=(x+bw/2,55); line(d,[top,(x+30,150)],8); line(d,[top,(x+bw-30,150)],8); line(d,[top,(x+bw/2,155)],8); circ(d,(top[0],top[1]+18),20,3)
finish(im,'tool_05_tripod.png')

# 6 ground anchor ordinary + buried deadman
im=new(); d=ImageDraw.Draw(im); rect(d,(10,10,334,174),2); rect(d,(354,10,678,174),2)
# left stake
line(d,[(20,132),(325,132)],3); line(d,[(190,52),(150,148)],9); line(d,[(186,63),(310,90)],5); arrow(d,(300,90),(318,93),3); arc(d,(110,120,210,170),190,340,2)
# right deadman cross-section
line(d,[(365,88),(668,88)],3); rect(d,(440,112,590,135),5); line(d,[(515,112),(640,42)],5); dashed(d,[(515,112),(515,88)],3); line(d,[(430,142),(600,142)],2); line(d,[(430,151),(600,151)],2)
finish(im,'tool_06_ground_anchor.png')

# 7 cloth prefilter
im=new(); d=ImageDraw.Draw(im); boxes=stage_boxes(d,4)
for idx,(x,y,bw,bh) in enumerate(boxes):
    cx=x+bw/2
    if idx==0:
        rect(d,(cx-35,55,cx+35,135),4); # dirty water jar
        for p in ((cx-20,80),(cx+7,95),(cx+20,70),(cx-5,120)): circ(d,p,3,2,fill=0)
    elif idx==1:
        # folded cloth over mouth
        rect(d,(cx-40,90,cx+40,145),4); line(d,[(cx-50,65),(cx+50,65)],5); dashed(d,[(cx-50,73),(cx+50,73)],2); arrow(d,(cx,42),(cx,62),3)
    elif idx==2:
        rect(d,(cx-40,95,cx+40,150),4)
        line(d,[(cx-50,68),(cx+50,68)],5)
        for yy in (78,88):
            dashed(d,[(cx-45,yy),(cx+45,yy)],2)
        arrow(d,(cx,45),(cx,93),3)
        for p in ((cx-10,110),(cx+15,125)): circ(d,p,3,2,fill=0)
    else:
        # pot + flame to indicate further treatment
        rect(d,(cx-40,80,cx+40,125),4); arc(d,(cx-42,60,cx+42,95),180,360,4); line(d,[(cx-20,135),(cx-30,155),(cx-5,142),(cx,160),(cx+12,140),(cx+30,155)],4)
finish(im,'tool_07_cloth_prefilter.png')

# 8 carry pole
im=new(); d=ImageDraw.Draw(im)
# people stick figures, pole, load
for cx in (130,558):
    circ(d,(cx,58),16,4); line(d,[(cx,75),(cx,125)],5); line(d,[(cx,90),(cx-20,115)],4); line(d,[(cx,90),(cx+20,115)],4); line(d,[(cx,125),(cx-18,160)],4); line(d,[(cx,125),(cx+18,160)],4)
line(d,[(115,82),(573,82)],7); line(d,[(344,82),(344,120)],4); rect(d,(300,120,388,164),4); arrow(d,(344,105),(344,118),3)
# low test inset
rect(d,(470,105,665,174),2); line(d,[(490,135),(645,135)],6); rect(d,(550,143,600,166),3); arrow(d,(575,125),(575,140),2)
finish(im,'tool_08_carry_pole.png')

# 9 splint clamp
im=new(); d=ImageDraw.Draw(im); boxes=stage_boxes(d,4)
for idx,(x,y,bw,bh) in enumerate(boxes):
    cx=x+bw/2
    if idx==0:
        line(d,[(cx,45),(cx-8,90)],10); line(d,[(cx+7,100),(cx,150)],10); line(d,[(cx-8,90),(cx+7,100)],2)
    elif idx==1:
        line(d,[(cx,45),(cx-6,92)],10); line(d,[(cx+6,100),(cx,150)],10); rect(d,(cx-28,58,cx-18,140),3); rect(d,(cx+18,58,cx+28,140),3)
    elif idx==2:
        line(d,[(cx,45),(cx-6,92)],10); line(d,[(cx+6,100),(cx,150)],10); rect(d,(cx-28,58,cx-18,140),3); rect(d,(cx+18,58,cx+28,140),3); 
        for yy in (72,98,126): line(d,[(cx-36,yy),(cx+36,yy)],4)
    else:
        rect(d,(cx-26,50,cx-16,145),3)
        rect(d,(cx+16,50,cx+26,145),3)
        for yy in (72,98,126):
            circ(d,(cx,yy),32,3)
        arrow(d,(cx-48,100),(cx-30,100),2)
        arrow(d,(cx+48,100),(cx+30,100),2)
finish(im,'tool_09_splint_clamp.png')

# 10 rain catch
im=new(); d=ImageDraw.Draw(im); rect(d,(12,10,430,174),2); rect(d,(448,10,676,174),2)
# tarp pitched into low point
line(d,[(35,55),(220,105),(405,55)],6); line(d,[(35,55),(35,120)],3); line(d,[(405,55),(405,120)],3); arrow(d,(220,105),(220,130),3); rect(d,(175,130,265,168),4)
# raindrops
for x in range(60,390,45): line(d,[(x,22),(x-5,38)],3)
# right: contaminated first flush -> later collection / treatment icon
line(d,[(470,45),(560,72),(650,45)],5); arrow(d,(560,72),(560,105),3); rect(d,(525,108,595,155),4); circ(d,(625,126),24,4); line(d,[(610,126),(640,126)],4); line(d,[(625,111),(625,141)],4)
finish(im,'tool_10_rain_catch.png')

print('generated',len(list(OUT.glob('tool_*.png'))),'diagrams in',OUT)
