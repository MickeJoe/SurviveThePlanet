"""Render the vertices read back from the saved Unreal map, not design inputs."""
import json
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

root=Path(__file__).resolve().parents[1]
data=json.loads((root/'Saved'/'FirstSectorVerification.json').read_text())
im=Image.new('RGB',(1100,1100),'#17222b')
draw=ImageDraw.Draw(im)
font=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',28)
small=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',20)
def pixel(p):
    return (550+p[0]*.1125,550-p[1]*.1125)
draw.polygon([pixel(p) for p in data['boundary']],fill='#253740',outline='#f1d65c',width=4)
for shape in data['shapes']:
    pts=[pixel(p) for p in shape['points']]
    draw.polygon(pts,fill='#3d7980',outline='#78e2dc',width=3)
    cx=sum(p[0] for p in pts)/len(pts)
    cy=sum(p[1] for p in pts)/len(pts)
    draw.text((cx,cy),str(int(shape['name'].split('_')[-1])),font=font,fill='white',anchor='mm')
draw.text((550,35),'FIRST SECTOR — SAVED ASSET PLAN VIEW',font=font,fill='white',anchor='mt')
draw.text((550,1040),'PlanetLevel hex: R = '+str(int(data['radius']))+' cm | 6 footprints | XY plane',font=small,fill='#c2d4df',anchor='mt')
im.save(root/'Saved'/'FirstSectorPlan.png')
print(root/'Saved'/'FirstSectorPlan.png')
