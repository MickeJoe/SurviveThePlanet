from pathlib import Path
from PIL import Image,ImageDraw,ImageFilter
import math,random,shutil
root=Path(r'C:\UE5\SurviveThePlanet 5.8')
assert (root/'SurviveThePlanet.uproject').exists()
target=root/'ContentSource/Planet';target.mkdir(parents=True,exist_ok=True)
random.seed(71237)
background=Image.new('RGB',(1600,900),(3,9,17));d=ImageDraw.Draw(background)
for _ in range(850):
    x=random.randrange(1600);y=random.randrange(900);b=random.randrange(45,190)
    d.ellipse((x,y,x+1,y+1),fill=(b,b,min(255,b+30)))
for radius in [155,260,370,460]:
    d.ellipse((620-radius,480-radius*.65,620+radius,480+radius*.65),outline=(13,80,98),width=2)
for end in [(690,240),(255,510),(810,680)]:d.line((620,480,*end),fill=(23,140,170),width=2)
for radius in [60,42,25]:d.ellipse((620-radius,480-radius*.4,620+radius,480+radius*.4),outline=(56,211,235),width=3)
d.polygon([(600,465),(620,415),(640,465)],outline=(110,235,255),fill=(22,68,95))
background.save(target/'T_System.png')
for name,base,land in [('Nexaris',(13,74,120),(57,115,61)),('Aridus',(140,70,29),(189,115,49)),('Borealis',(54,93,125),(150,188,205))]:
    side=384;im=Image.new('RGBA',(side,side));pixels=im.load()
    for y in range(side):
        for x in range(side):
            nx=(x-192)/178;ny=(y-192)/178;r=nx*nx+ny*ny
            if r>1:continue
            z=math.sqrt(1-r)
            noise=math.sin(nx*13+math.sin(ny*10)*2)+math.sin(ny*21+nx*8)*.5+math.cos(nx*25-ny*15)*.3
            c=land if noise>.3 else base
            if name=='Borealis' and abs(ny)>.65:c=(205,225,234)
            shade=max(.12,min(1,.18+max(0,-nx*.55-ny*.4+z*.7)))
            atmosphere=(1-z)**5
            pixels[x,y]=tuple(int(min(255,v*shade+atmosphere*(20 if i==0 else 55))) for i,v in enumerate(c))+(255,)
    ImageDraw.Draw(im).ellipse((12,12,372,372),outline=(30,180,205,230),width=2)
    im.save(target/('T_'+name+'.png'))
script=Path(__file__).with_name('create_planet_assets.py')
if script.resolve()!=(root/'Scripts/create_planet_assets.py').resolve():
    shutil.copy2(script,root/'Scripts/create_planet_assets.py')
print('Planet artwork and editor authoring script ready')
