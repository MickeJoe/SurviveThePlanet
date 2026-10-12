"""Create matching colony-industrial buildings through Blender's native MCP."""
import bpy,math,json
from pathlib import Path
from mathutils import Vector
root=Path('C:/UE5/SurviveThePlanet 5.8')
# Reuse the geothermal palette, chamfered primitives, UV projection and FBX export.
exec((root/'Scripts/create_geothermal_blender.py').read_text().split("box('Platform'")[0])
scene.name='NuclearIndustrialWorkshop'
out=root/'ContentSource/NuclearIndustrial';out.mkdir(parents=True,exist_ok=True)

def bolts(x,y,z,r,count=12):
 for i in range(count):
  a=i*math.tau/count;cyl('Service flange bolt',(x+r*math.cos(a),y+r*math.sin(a),z),2.7,3.5,'Steel')

def deck(w,d):
 box('Ground foundation',(0,0,9),(w,d,18),'Dark')
 box('Foundation cap',(0,0,22),(w-12,d-12,8),'Steel')
 for x in [-w/2+22,w/2-22]:
  for y in [-d/2+22,d/2-22]:
   box('Foundation anchor',(x,y,31),(27,27,10),'Ivory');cyl('Anchor stud',(x,y,38),5,4,'Steel')
 for x in [-w/2+12,w/2-12]:box('Orange deck trim',(x,0,28),(5,d-58,4),'Orange')
 for y in [-d/2+10,d/2-10]:
  for x in [-w*.25,0,w*.25]:box('Deck plates',(x,y,27),(w*.22,14,2),'Ivory')

def panels(x,y,z,w,h,axis='Y'):
 # Replace broad blank walls with readable plate divisions and framing.
 count=max(2,int(w/55));step=w/count
 for i in range(count):
  p=x-w/2+(i+.5)*step
  box('Ivory service panel',(p,y,z),(step-4,3,h-7),'Ivory')
  box('Panel lower dirt skirt',(p,y-2,z-h/2+8),(step-5,2,10),'Dark')
  for dx in [-step/2+8,step/2-8]:
   o=cyl('Panel fastener',(p+dx,y-3,z+h/2-10),2.3,2,'Steel');o.rotation_euler.x=math.pi/2

def tank(x,y,r,height):
 cyl('Tank foot',(x,y,38),r+7,18,'Dark');cyl('Sealed process tank',(x,y,48+height/2),r,height,'Steel')
 for j in range(4):cyl('Tank jacket section',(x,y,54+(j+.5)*(height-12)/4),r+.6,(height-12)/4-4,'Ivory')
 for z in [56,48+height*.62,48+height-6]:
  cyl('Tank flange',(x,y,z),r+4,7,'Orange' if z==48+height*.62 else 'Dark');bolts(x,y,z+5,r+1,8)
 for i in range(6):
  a=i*math.tau/6;xx=x+(r+1)*math.cos(a);yy=y+(r+1)*math.sin(a)
  pipe('Tank jacket seam',(xx,yy,62),(xx,yy,48+height-12),1.4,'Dark')
 cyl('Tank sealed lid',(x,y,48+height+2),r-3,8,'Ivory');cyl('Tank upper hatch',(x,y,48+height+9),r*.48,6,'Dark')

deck(450,410)
# Shielded containment vessel, with a closed dome rather than exposed glowing fuel.
cyl('Containment skirt',(-42,5,51),100,40,'Dark')
cyl('Reactor pressure shell',(-42,5,124),91,116,'Steel')
for j in range(3):cyl('Containment jacket',(-42,5,80+j*33),92,29,'Ivory')
for z in [63,145,178]:cyl('Containment flange',(-42,5,z),98,8,'Orange' if z==145 else 'Dark');bolts(-42,5,z+6,95)
for i in range(12):
 a=i*math.tau/12;xx=-42+95*math.cos(a);yy=5+95*math.sin(a)
 pipe('Containment stiffener',(xx,yy,62),(xx,yy,176),2,'Steel')
# Flattened dome with visible segmented ribs.
bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=12,radius=1,location=(-42,5,180))
o=bpy.context.object;o.scale=(90,90,45);finish(o,'Shielded containment dome','Ivory')
torus('Dome seam',(-42,5,183),87,2.5,'Steel')
for i in range(8):
 a=i*math.tau/8
 points=[Vector((-42+89*math.cos(t)*math.cos(a),5+89*math.cos(t)*math.sin(a),180+46*math.sin(t))) for t in [j*math.pi/12 for j in range(7)]]
 for a1,b1 in zip(points,points[1:]):pipe('Dome panel seam',a1,b1,1.2,'Dark')
cyl('Core maintenance hatch',(-42,5,226),22,6,'Steel');bolts(-42,5,231,17,6)
for y in [-105,88]:tank(137,y,33,148)
for y in [-105,88]:
 pipe('Orange coolant main',(50,y,63),(135,y,63),10,'Orange')
 torus('Coolant flange',(102,y,63),13,3,'Steel').rotation_euler.y=math.pi/2
box('Reactor controls',(15,-151,80),(133,60,100),'Dark');panels(15,-183,85,125,88)
box('Controls lid',(15,-151,133),(138,64,8),'Ivory')
box('Status screen',(15,-186,102),(33,3,19),'Dark');box('Status screen glass',(15,-188,103),(27,2,13),'Cyan')
box('Hazard band',(15,-186,61),(110,2,7),'Orange')
for x in [-165,-133,-101]:box('Heat exchanger fins',(x,127,73),(15,94,90),'Steel')
for y in [96,140]:pipe('Heat exchanger feed',(-42,y,59),(-170,y,59),9,'Orange')
for x in [-180,180]:
 for y in [-168,168]:box('Worklight',(x,y,40),(12,12,7),'Lamp')
reactor=export('SM_NuclearReactor_Refined')
reactor.hide_render=True

deck(492,422)
box('Processor chassis',(0,-43,115),(286,245,172),'Dark')
for y in [-168,82]:panels(0,y,117,282,164)
for x in [-146,146]:
 for y in [-112,-42,28]:
  box('Side access panels',(x,y,119),(3,65,148),'Ivory')
  box('Side framing',(x,y,51),(5,61,8),'Orange')
box('Processor roof',(0,-43,205),(295,255,10),'Ivory')
for x in [-93,0,93]:box('Roof panel',(x,-43,212),(87,230,3),'Ivory')
for x in [-139,139]:box('Roof orange stripe',(x,-43,214),(8,230,3),'Orange')
for x in [-158,158]:tank(x,126,37,230)
for x in [-158,158]:
 pipe('Process return',(x,125,126),(x,54,126),10,'Orange')
 pipe('Process downcomer',(x,54,126),(x,54,51),10,'Orange')
 cyl('Pipe coupling',(x,54,72),14,8,'Steel')
box('Front access frame',(0,-172,93),(91,12,111),'Steel')
box('Armored process door',(0,-180,93),(77,5,96),'Dark')
for x in [-26,0,26]:box('Process observation window',(x,-184,109),(19,2,32),'Cyan')
box('Door hazard marker',(0,-184,60),(65,2,7),'Orange')
box('Front step',(0,-191,35),(110,32,16),'Steel')
for x in [-100,100]:
 box('Front diagnostic panel',(x,-172,137),(44,5,38),'Dark')
 for j in range(5):box('Front ventilation slit',(x,-176,126+j*6),(32,2,2),'Steel')
for x in [-78,78]:
 box('Roof heat exchanger',(x,9,223),(57,56,16),'Dark')
 for j in range(6):box('Roof cooling fins',(x-23+j*9,9,237),(3,50,20),'Steel')
# Preserve the established animated roof fan pivot, moving its guard onto the roof.
cyl('Fan recess',(0,40,216),33,8,'Dark');torus('Fan guard',(0,40,224),34,3,'Steel')
for x in [-200,200]:
 for y in [-169,169]:box('Worklight',(x,y,40),(12,12,7),'Lamp')
processor=export('SM_UraniumProcessor_Refined');processor.hide_render=True
cyl('Fan hub',(0,0,0),8,5,'Steel')
for i in range(5):
 a=i*math.tau/5;o=box('Fan blade',(16*math.cos(a),16*math.sin(a),0),(27,9,3),'Steel');o.rotation_euler.z=a+.25
fan=export('SM_UraniumProcessor_Fan');fan.hide_render=True

scene.render.engine='CYCLES';scene.cycles.samples=24
bpy.ops.object.camera_add(location=(680,-840,650));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,100))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=650;cam.data.clip_end=10000;scene.camera=cam
for pos,energy in [((0,-300,700),20000000),((-300,150,400),10000000)]:
 bpy.ops.object.light_add(type='AREA',location=pos);l=bpy.context.object;l.data.energy=energy;l.data.size=450;l.rotation_euler=(Vector((0,0,90))-l.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('NuclearStudio');scene.world.color=(.2,.2,.2)
scene.render.resolution_x=512;scene.render.resolution_y=512;scene.render.resolution_percentage=100;scene.render.film_transparent=True;scene.render.image_settings.file_format='PNG'
for key,body in [('NuclearReactor',reactor),('UraniumProcessor',processor)]:
 body.hide_render=False
 if key=='UraniumProcessor':fan.hide_render=False;fan.location=(0,40,224)
 scene.render.filepath=str(out/('T_'+key+'_Refined.png'));bpy.ops.render.render(write_still=True);body.hide_render=True;fan.hide_render=True
bpy.ops.wm.save_as_mainfile(filepath=str(out/'NuclearIndustrial.blend'))
result={'source':str(out),'meshes':[reactor.name,processor.name,fan.name]}
