"""Compact colony-style mine, authored through native Blender MCP."""
import bpy, math
from pathlib import Path
from mathutils import Vector
root=Path('C:/UE5/SurviveThePlanet 5.8')
exec((root/'Scripts/create_geothermal_blender.py').read_text().split("box('Platform'")[0])
scene.name='UraniumMineWorkshop'
out=root/'ContentSource/UraniumMine';out.mkdir(parents=True,exist_ok=True)

box('Ground foundation',(0,0,8),(390,350,16),'Dark')
box('Ivory deck',(0,0,20),(380,340,8),'Ivory')
for y in [-162,162]:box('Safety edging',(0,y,25),(346,5,3),'Orange')
for x in [-178,178]:
 for y in [-155,155]:
  box('Foundation anchor',(x,y,27),(22,22,8),'Ivory');cyl('Anchor bolt',(x,y,34),4,4,'Steel')
  box('Worklight',(x,y,39),(11,11,7),'Lamp')
# A covered shaft and reinforced headframe, distinct from a power generator.
cyl('Shaft collar',(-80,0,30),48,14,'Steel')
torus('Shaft safety rim',(-80,0,40),43,4,'Orange')
cyl('Sealed shaft recess',(-80,0,36),35,6,'Dark')
for x in [-142,-18]:
 for y in [-75,75]:
  box('Frame foot',(x,y,35),(32,32,18),'Steel')
  box('Ivory frame upright',(x,y,147),(14,14,224),'Ivory')
  box('Frame safety band',(x,y,73),(15,15,12),'Orange')
  pipe('Diagonal steel brace',(x,y,43),(-80,y,240),4,'Steel')
for y in [-75,75]:
 box('Upper frame crossbeam',(-80,y,255),(150,22,22),'Ivory')
 box('Orange beam inset',(-80,y-12,255),(118,2,8),'Orange')
 for x in [-136,-24]:cyl('Beam fastener',(x,y,267),3.5,4,'Steel')
box('Upper hoist support',(-80,0,255),(42,155,15),'Steel')
cyl('Drive motor',(-80,0,226),27,45,'Ivory')
torus('Motor collar',(-80,0,208),29,3,'Orange')
for y in [-54,54]:
 pipe('Hydraulic sleeve',(-80,y,92),(-80,y,163),8,'Dark')
 pipe('Hydraulic piston',(-80,y,163),(-80,y,230),4,'Steel')
# Light painted service enclosure with readable seams and restrained dirt skirt.
box('Service enclosure',(92,20,91),(148,190,130),'Dark')
box('Ivory roof',(92,20,161),(155,199,14),'Ivory')
for y in [-77,117]:
 for i in range(3):
  x=42+i*50
  box('Service wall panel',(x,y,94),(47,3,111),'Ivory')
  box('Dirty lower skirt',(x,y-2,43),(46,2,8),'Steel')
  for dx in [-18,18]:cyl('Panel screw',(x+dx,y,143),2.3,3,'Steel').rotation_euler.x=math.pi/2
 for x in [22,164]:box('Service corner',(x,y,91),(7,7,129),'Ivory')
for x in [15,169]:
 for i in range(4):box('Side panel',(x,-50+i*47,94),(3,43,110),'Ivory')
box('Inspection door',(80,-81,91),(56,5,83),'Steel')
box('Door inset',(80,-85,91),(48,2,73),'Ivory')
box('Door handle',(100,-87,86),(3,3,17),'Dark')
box('Door orange sill',(80,-86,49),(54,3,6),'Orange')
box('Diagnostic screen',(132,-83,119),(25,4,22),'Dark')
box('Cyan readout',(132,-86,120),(19,2,9),'Cyan')
box('Front step',(80,-96,31),(86,30,10),'Steel')
for x in [55,130]:
 box('Roof vent',(x,39,175),(48,65,15),'Dark')
 for y in range(7):box('Vent grille',(x,14+y*8,184),(44,3,3),'Steel')
pipe('Hydraulic supply',(-14,0,62),(18,0,62),9,'Orange')
for x in [-8,10]:cyl('Supply coupling',(x,0,62),12,5,'Steel').rotation_euler.y=math.pi/2
pipe('Cable conduit',(-18,70,118),(15,70,118),4,'Dark')
box('Tool cabinet',(125,136,63),(76,34,70),'Ivory')
for x in [110,140]:box('Cabinet seam',(x,155,63),(2,2,55),'Steel')
body=export('SM_UraniumMine_Refined');body.hide_render=True
# Separate centered rotor, allowing rotation and a short axial feed stroke.
cyl('Drill drive shaft',(0,0,85),15,156,'Steel')
cyl('Drill barrel',(0,0,66),26,90,'Ivory')
for z in [22,60,108]:torus('Drill collar',(0,0,z),27,3,'Orange' if z==60 else 'Steel')
for i in range(48):
 a=i*math.tau/12;b=(i+1)*math.tau/12
 pipe('Helical cutter',(25*math.cos(a),25*math.sin(a),24+i*1.6),(25*math.cos(b),25*math.sin(b),24+(i+1)*1.6),3,'Steel')
cyl('Drill tip',(0,0,7),19,12,'Dark')
drill=export('SM_UraniumMine_Drill');drill.location=(-80,0,44)
body.hide_render=False
scene.render.engine='CYCLES';scene.cycles.samples=24
bpy.ops.object.camera_add(location=(650,-820,620));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,100))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=580;cam.data.clip_end=10000;scene.camera=cam
for pos,energy in [((0,-300,700),20000000),((-300,150,400),10000000)]:
 bpy.ops.object.light_add(type='AREA',location=pos);light=bpy.context.object;light.data.energy=energy;light.data.size=450;light.rotation_euler=(Vector((0,0,90))-light.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('UraniumMineStudio');scene.world.color=(.2,.2,.2)
scene.render.resolution_x=512;scene.render.resolution_y=512;scene.render.resolution_percentage=100;scene.render.film_transparent=True;scene.render.image_settings.file_format='PNG';scene.render.filepath=str(out/'T_UraniumMine_Refined.png');bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=str(out/'UraniumMine.blend'))
result={'source':str(out),'meshes':[body.name,drill.name],'dimensions_cm':[390,350,270]}
