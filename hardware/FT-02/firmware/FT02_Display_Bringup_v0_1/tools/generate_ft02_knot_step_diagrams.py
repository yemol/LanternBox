from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
import math
OUT=Path('/mnt/data/_knot_steps')
OUT.mkdir(exist_ok=True)
W,H=688,128
try:
    FONT=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',26)
    SMALL=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',20)
except:
    FONT=SMALL=None

def img():
    return Image.new('1',(W,H),1)

def line(d,pts,width=9,fill=0):
    d.line(pts,fill=fill,width=width,joint='curve')

def dashed(d,p1,p2,width=7,dash=14,gap=10):
    x1,y1=p1;x2,y2=p2
    L=math.hypot(x2-x1,y2-y1)
    if L==0:return
    ux=(x2-x1)/L;uy=(y2-y1)/L
    t=0
    while t<L:
        e=min(L,t+dash)
        d.line((x1+ux*t,y1+uy*t,x1+ux*e,y1+uy*e),fill=0,width=width)
        t+=dash+gap

def arrow(d,p1,p2,width=4):
    d.line((p1,p2),fill=0,width=width)
    ang=math.atan2(p2[1]-p1[1],p2[0]-p1[0])
    size=15
    a1=ang+2.55;a2=ang-2.55
    pts=[p2,(p2[0]+size*math.cos(a1),p2[1]+size*math.sin(a1)),(p2[0]+size*math.cos(a2),p2[1]+size*math.sin(a2))]
    d.polygon(pts,fill=0)

def label(d,xy,text):
    d.text(xy,text,font=FONT,fill=0)

def circle_arrow(d,c,r,a1,a2):
    # arc plus arrowhead
    box=(c[0]-r,c[1]-r,c[0]+r,c[1]+r)
    d.arc(box,start=a1,end=a2,fill=0,width=4)
    ang=math.radians(a2)
    p=(c[0]+r*math.cos(ang), c[1]+r*math.sin(ang))
    tang=ang+math.pi/2
    size=13
    pts=[p,(p[0]+size*math.cos(tang+2.4),p[1]+size*math.sin(tang+2.4)),(p[0]+size*math.cos(tang-2.4),p[1]+size*math.sin(tang-2.4))]
    d.polygon(pts,fill=0)

def save(im,name):
    im.save(OUT/name)

# ---- Square/Reef knot ----
# step 1: cross A over B
im=img();d=ImageDraw.Draw(im)
# B under from upper-left to lower-right dashed
dashed(d,(100,25),(420,105),8)
# white gap at crossing then A over
d.ellipse((245,52,290,92),fill=1)
line(d,[(100,105),(270,70),(430,28)],10)
label(d,(448,12),'A');label(d,(445,88),'B')
arrow(d,(530,65),(470,65))
save(im,'tool_01_flat_01.png')

# step2 first half knot, A passes under B and ends swap sides
im=img();d=ImageDraw.Draw(im)
# draw stylized half knot: left tail B rises over center then to right; A loops under
line(d,[(75,88),(220,62),(305,72),(440,96)],10) # B main
# A path from right upper, curves under intersection to left lower
line(d,[(455,26),(350,42),(300,72)],10)
d.ellipse((274,58,326,88),fill=1)
line(d,[(300,72),(245,94),(105,104)],10)
label(d,(470,12),'A');label(d,(55,76),'B')
arrow(d,(520,72),(440,92))
arrow(d,(170,40),(105,95))
save(im,'tool_01_flat_02.png')

# step3 reverse crossing B over A
im=img();d=ImageDraw.Draw(im)
# tails after first half: A left, B right; show cross reverse
line(d,[(100,30),(270,70),(430,105)],10) # A under
# erase crossing then B over from bottom-left to top-right
d.ellipse((245,50,295,90),fill=1)
line(d,[(100,105),(270,70),(430,30)],10)
label(d,(55,12),'A');label(d,(450,12),'B')
arrow(d,(520,62),(462,42))
save(im,'tool_01_flat_03.png')

# step4 final reef knot and pull outward
im=img();d=ImageDraw.Draw(im)
# draw central woven knot with four tails
# left top to central then right bottom
line(d,[(65,35),(220,55),(275,64)],10)
line(d,[(275,64),(330,85),(500,100)],10)
# left bottom to center -> right top, crossing weave
line(d,[(65,98),(220,82),(275,65)],10)
d.ellipse((257,55,298,78),fill=1)
line(d,[(275,65),(335,44),(500,28)],10)
# small central overpass to give knot appearance
line(d,[(238,75),(275,92),(315,74)],8)
# pull arrows
arrow(d,(58,22),(20,22));arrow(d,(58,110),(20,110));arrow(d,(510,18),(555,18));arrow(d,(510,112),(555,112))
label(d,(580,46),'OK')
save(im,'tool_01_flat_04.png')

# ---- Bowline ----
# step1 small loop, A is working end left-ish
im=img();d=ImageDraw.Draw(im)
# standing part horizontal to right with loop in middle
line(d,[(90,82),(280,82)],10)
# loop circle-like curve via sampled points
pts=[]
for a in range(200,521,10):
    rad=math.radians(a)
    pts.append((300+55*math.cos(rad),76+48*math.sin(rad)))
line(d,pts,10)
line(d,[(335,45),(570,45)],10)
label(d,(60,90),'A');label(d,(585,30),'S')
arrow(d,(170,30),(245,55))
save(im,'tool_02_bowline_01.png')

# step2 A up through loop
im=img();d=ImageDraw.Draw(im)
# main standing S right
line(d,[(350,78),(610,78)],10)
# small loop at 300
pts=[]
for a in range(200,521,10):
    rad=math.radians(a);pts.append((330+55*math.cos(rad),78+45*math.sin(rad)))
line(d,pts,10)
# working end A from left bottom through loop upward
dashed(d,(85,105),(300,88),8)
line(d,[(300,88),(330,42),(330,18)],10)
label(d,(55,92),'A');label(d,(620,63),'S')
arrow(d,(275,112),(330,44))
save(im,'tool_02_bowline_02.png')

# step3 A around behind S
im=img();d=ImageDraw.Draw(im)
line(d,[(350,75),(610,75)],10)
# loop left
pts=[]
for a in range(210,515,10):
    rad=math.radians(a);pts.append((320+52*math.cos(rad),80+42*math.sin(rad)))
line(d,pts,10)
# A up then around S: solid up, dashed behind horizontal standing, solid return
dashed(d,(110,105),(285,92),7)
line(d,[(285,92),(320,38),(410,35)],9)
# around standing at x430
circle_arrow(d,(430,74),42,240,80)
label(d,(435,12),'A');label(d,(620,60),'S')
save(im,'tool_02_bowline_03.png')

# step4 A back down through original loop
im=img();d=ImageDraw.Draw(im)
line(d,[(350,75),(610,75)],10)
pts=[]
for a in range(210,515,10):
    rad=math.radians(a);pts.append((320+52*math.cos(rad),80+42*math.sin(rad)))
line(d,pts,10)
# A comes around S then back toward loop
line(d,[(470,28),(430,42),(395,55),(360,45),(330,38)],9)
arrow(d,(340,25),(315,73))
label(d,(478,12),'A');label(d,(620,60),'S')
save(im,'tool_02_bowline_04.png')

# step5 final bowline loop
im=img();d=ImageDraw.Draw(im)
# large fixed loop left
pts=[(320,70),(260,35),(150,30),(90,64),(145,105),(260,102),(320,70)]
line(d,pts,9)
# standing right
line(d,[(320,70),(610,70)],10)
# knot nub / tail near center
line(d,[(300,58),(345,42),(380,55),(345,78),(305,87)],8)
line(d,[(345,42),(405,25)],8)
label(d,(415,10),'A');label(d,(620,55),'S')
arrow(d,(120,118),(80,118));arrow(d,(560,100),(620,100))
label(d,(500,12),'OK')
save(im,'tool_02_bowline_05.png')

# ---- Clove hitch on post ----
def post(d):
    d.rounded_rectangle((295,8,390,120),radius=10,outline=0,width=5)
    d.line((320,10,320,118),fill=0,width=2); d.line((365,10,365,118),fill=0,width=2)

# step1 wrap once around post
im=img();d=ImageDraw.Draw(im);post(d)
line(d,[(70,75),(295,75)],9)
# behind post dashed on right
dashed(d,(390,75),(520,45),7)
arrow(d,(515,32),(550,52));label(d,(545,55),'A')
save(im,'tool_03_clove_01.png')
# step2 cross over standing and wrap second time
im=img();d=ImageDraw.Draw(im);post(d)
line(d,[(70,85),(295,85)],9)
dashed(d,(390,85),(515,40),7)
line(d,[(515,40),(390,28),(295,48),(170,35)],9)
# crossing front of post
line(d,[(285,50),(405,88)],8)
arrow(d,(155,28),(115,45));label(d,(530,22),'A')
save(im,'tool_03_clove_02.png')
# step3 pass A under crossing section
im=img();d=ImageDraw.Draw(im);post(d)
line(d,[(70,88),(295,88)],9); line(d,[(295,45),(440,90)],9)
# A coming from right and under X
line(d,[(560,28),(445,40),(390,55)],9)
arrow(d,(455,20),(368,68));label(d,(575,12),'A')
save(im,'tool_03_clove_03.png')
# step4 final clove hitch with X front and tails opposite
im=img();d=ImageDraw.Draw(im);post(d)
line(d,[(70,92),(310,82),(390,50),(590,35)],9)
line(d,[(70,38),(290,50),(390,82),(590,96)],9)
# front X clearer
line(d,[(300,42),(390,82)],9); d.ellipse((330,55,370,80),fill=1); line(d,[(300,86),(390,48)],9)
arrow(d,(65,112),(25,112));arrow(d,(595,112),(635,112));label(d,(510,8),'OK')
save(im,'tool_03_clove_04.png')

# ---- Trucker's hitch ----
def anchor(d,x=590):
    d.ellipse((x-18,40,x+18,76),outline=0,width=6)
    d.line((x,76,x,118),fill=0,width=6)
# step1 fixed small loop in main line
im=img();d=ImageDraw.Draw(im)
line(d,[(55,72),(270,72)],9)
# small bight loop above line
pts=[(270,72),(305,42),(350,42),(375,72),(345,92),(305,90),(270,72)]
line(d,pts,9)
line(d,[(375,72),(610,72)],9)
label(d,(610,55),'S')
arrow(d,(320,112),(320,88))
save(im,'tool_04_trucker_01.png')
# step2 working end A around anchor
im=img();d=ImageDraw.Draw(im);anchor(d)
line(d,[(55,48),(420,48)],9)
# loop in main line around x270
pts=[(245,48),(275,25),(315,25),(340,48),(315,68),(275,68),(245,48)]
line(d,pts,8)
# work end from main to anchor and back
dashed(d,(420,48),(570,48),7)
line(d,[(570,48),(615,58),(570,82),(480,92)],9)
label(d,(450,94),'A')
arrow(d,(500,112),(450,98))
save(im,'tool_04_trucker_02.png')
# step3 feed A through small loop
im=img();d=ImageDraw.Draw(im);anchor(d)
line(d,[(55,42),(245,42)],9)
pts=[(245,42),(275,20),(315,20),(340,42),(315,65),(275,65),(245,42)]
line(d,pts,8)
line(d,[(340,42),(565,42)],9)
# return from anchor to loop
line(d,[(590,70),(510,96),(390,96),(315,64)],9)
arrow(d,(365,108),(305,67));label(d,(520,92),'A')
save(im,'tool_04_trucker_03.png')
# step4 pull free end to tension
im=img();d=ImageDraw.Draw(im);anchor(d)
line(d,[(55,38),(245,38)],9)
pts=[(245,38),(275,18),(315,18),(340,38),(315,58),(275,58),(245,38)]
line(d,pts,8)
line(d,[(340,38),(565,38)],9)
line(d,[(590,70),(510,95),(315,58)],9)
line(d,[(315,58),(445,110)],9)
arrow(d,(445,110),(530,118));label(d,(540,100),'A')
# tension arrows on main line
arrow(d,(160,18),(90,18));arrow(d,(440,18),(520,18))
save(im,'tool_04_trucker_04.png')
# step5 lock with two half hitches, simplified around main line
im=img();d=ImageDraw.Draw(im);anchor(d)
line(d,[(55,55),(565,55)],9)
# two loops around main line near x390/440
for cx in (390,450):
    d.arc((cx-26,30,cx+26,82),start=30,end=330,fill=0,width=7)
line(d,[(450,78),(525,108)],8)
label(d,(535,94),'A');label(d,(585,36),'OK')
arrow(d,(505,90),(560,115))
save(im,'tool_04_trucker_05.png')

# contact sheets for inspection
for prefix,n in [('tool_01_flat',4),('tool_02_bowline',5),('tool_03_clove',4),('tool_04_trucker',5)]:
    ims=[Image.open(OUT/f'{prefix}_{i:02d}.png').convert('L') for i in range(1,n+1)]
    sheet=Image.new('L',(W,max(H*n,1)),255)
    for j,im2 in enumerate(ims): sheet.paste(im2,(0,j*H))
    sheet.save(OUT/f'{prefix}_contact.png')
print('generated', len(list(OUT.glob('*.png'))), 'files')
