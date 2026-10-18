from pathlib import Path
from PIL import Image, ImageDraw
root=Path('C:/UE5/SurviveThePlanet 5.8/ContentSource/UI/ObjectiveRewards');root.mkdir(parents=True,exist_ok=True)
scale=4
def glyph(name,draw):
 im=Image.new('RGBA',(64*scale,64*scale));d=ImageDraw.Draw(im)
 def line(points,width=4,fill=(35,205,242,255)):
  d.line([(int(x*scale),int(y*scale)) for x,y in points],fill=fill,width=width*scale,joint='curve')
 draw(line,d)
 im.resize((64,64),Image.Resampling.LANCZOS).save(root/(name+'.png'))
glyph('T_RewardProgress',lambda line,d:(line([(9,52),(9,43),(24,43),(24,31),(39,31),(39,17),(55,17)]),line([(44,7),(55,17),(44,27)])))
glyph('T_RewardConfidence',lambda line,d:(line([(32,5),(53,13),(51,35),(43,48),(32,57),(21,48),(13,35),(11,13),(32,5)]),line([(23,30),(30,37),(43,23)])))
glyph('T_RewardBlueprint',lambda line,d:(line([(13,7),(43,7),(53,17),(53,57),(13,57),(13,7)],3),line([(43,7),(43,17),(53,17)],3),line([(22,42),(22,30),(33,23),(44,30),(44,42),(22,42)],2),line([(32,42),(32,33),(38,33),(38,42)],2),line([(22,49),(44,49)],2)))
print('Created three reusable cyan UI glyphs.')
