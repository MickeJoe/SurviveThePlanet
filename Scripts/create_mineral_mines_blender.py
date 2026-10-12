import bpy,math
from pathlib import Path
from mathutils import Vector
root=Path('C:/UE5/SurviveThePlanet 5.8')
exec((root/'Scripts/create_geothermal_blender.py').read_text().split("box('Platform'")[0])
scene.name='MineralMineWorkshop';out=root/'ContentSource/MineralMines';out.mkdir(parents=True,exist_ok=True)
def foundation():
 box('Foundation',(0,0,8),(390,350,16),'Dark');box('Ivory deck',(0,0,20),(380,340,8),'Ivory')
 for y in [-162,162]:box('Orange safety edging',(0,y,25),(345,5,3),'Orange')
 for x in [-178,178]:
  for y in [-155,155]:
   box('Anchored footing',(x,y,28),(23,23,10),'Ivory');cyl('Foundation bolt',(x,y,35),4,4,'Steel');box('Work lamp',(x,y,41),(12,12,7),'Lamp')
 for x in [-165,165]:
  for y in range(-105,100,16):box('Drainage grille',(x,y,25),(20,5,2),'Steel')
 cyl('Recessed shaft',(-80,0,31),46,14,'Dark');torus('Shaft collar',(-80,0,38),48,5,'Steel');torus('Safety lip',(-80,0,41),43,3,'Orange')
def cabinet(center,size):
 x,y,z=center;w,d,h=size;box('Service cabinet',center,size,'Dark');box('Overhanging roof',(x,y,z+h/2+4),(w+8,d+8,10),'Ivory')
 for side in [-1,1]:
  yy=y+side*(d/2+1)
  for i in range(3):
   xx=x-w/2+w*(i+.5)/3
   box('Painted access panel',(xx,yy,z+2),(w/3-3,3,h-8),'Ivory');box('Weathered skirt',(xx,yy+side*2,z-h/2+12),(w/3-4,2,12),'Steel')
   box('Safety band',(xx,yy+side*2,z-h/2+26),(w/3-4,2,5),'Orange')
   for dx in [-w/9,w/9]:
    bolt=cyl('Panel fastener',(xx+dx,yy+side*3,z+h/2-12),2.6,4,'Steel');bolt.rotation_euler.x=math.pi/2
 for xx in [x-w/2-1,x+w/2+1]:
  box('Side access panel',(xx,y,z),(3,d-8,h-8),'Ivory')
  for yy in range(int(y-d/2+14),int(y+d/2-6),13):box('Cooling louvre',(xx+(-2 if xx<x else 2),yy,z+13),(2,7,33),'Dark')
 box('Inspection door',(x-w*.16,y-d/2-4,z),(w*.42,5,h*.72),'Steel');box('Door insert',(x-w*.16,y-d/2-7,z),(w*.36,2,h*.65),'Ivory')
 box('Door handle',(x,y-d/2-9,z),(3,3,18),'Dark');box('Status recess',(x+w*.29,y-d/2-6,z+h*.23),(24,4,21),'Dark');box('Cyan status',(x+w*.29,y-d/2-9,z+h*.23),(18,2,9),'Cyan')
 box('Service step',(x-w*.16,y-d/2-18,29),(w*.6,26,8),'Steel')
def fan_seat(x,y,z):
 cyl('Fan recess',(x,y,z),29,5,'Dark');torus('Fan shroud',(x,y,z+4),30,4,'Steel')
 for a in range(8):
  angle=a*math.tau/8;pipe('Fan guard',(x+28*math.cos(angle),y+28*math.sin(angle),z+7),(x-28*math.cos(angle),y-28*math.sin(angle),z+7),1.2,'Steel')
def drill_mesh(name,wide=False):
 cyl('Chrome drill spindle',(0,0,84),12,155,'Steel');cyl('Drive barrel',(0,0,107),26 if wide else 23,46,'Ivory')
 for z in [85,130]:torus('Drive flange',(0,0,z),27,3,'Orange')
 cyl('Cutter body',(0,0,40),23 if wide else 19,60,'Dark')
 for i in range(48):
  a=i*math.tau/12;b=(i+1)*math.tau/12;r=27 if wide else 24
  pipe('Spiral cutting flute',(r*math.cos(a),r*math.sin(a),12+i*1.3),(r*math.cos(b),r*math.sin(b),12+(i+1)*1.3),3.3,'Steel')
 for a in range(6):
  angle=a*math.tau/6;box('Tungsten cutter',(14*math.cos(angle),14*math.sin(angle),8),(10,8,12),'Steel').rotation_euler.z=angle
 return export(name)
def fan_mesh(name):
 cyl('Fan hub',(0,0,0),8,6,'Steel')
 for i in range(6):
  a=i*math.tau/6;o=box('Fan blade',(16*math.cos(a),16*math.sin(a),0),(24,9,3),'Steel');o.rotation_euler.z=a+.25
 return export(name)
models=[]
# Silica: a compact enclosed auger with dust collection, inspection hood and cyclone.
foundation();cabinet((98,-18,87),(132,145,122))
for y in [-66,66]:
 box('Auger column',(-80,y,134),(17,18,201),'Ivory');box('Column wear boot',(-80,y,57),(19,20,35),'Steel')
 pipe('Reinforcement brace',(-141,y,40),(-80,y,225),4,'Steel')
 box('Head crossbeam',(-80,y,236),(133,23,20),'Ivory');box('Orange head inset',(-80,y-13,236),(102,3,6),'Orange')
box('Drive cradle',(-80,0,235),(45,150,17),'Steel');cyl('Drive motor',(-80,0,210),27,36,'Ivory')
for y in [-48,48]:pipe('Actuator sleeve',(-80,y,97),(-80,y,166),7,'Dark');pipe('Chrome ram',(-80,y,166),(-80,y,211),4,'Steel')
box('Dust enclosure back',(-80,48,103),(103,5,92),'Ivory')
for x in [-133,-27]:box('Auger guard',(x,0,99),(5,100,87),'Ivory')
box('Inspection frame',(-80,-51,107),(103,5,90),'Steel')
for z in [73,108,142]:box('Inspection opening',(-80,-55,z),(87,2,22),'Dark')
pipe('Cyclone feed',(-27,41,135),(111,101,135),9,'Steel');pipe('Cyclone elbow',(111,101,135),(111,101,201),9,'Steel')
cyl('Cyclone collector',(109,115,171),29,94,'Ivory')
for z in [127,177,219]:torus('Cyclone flange',(109,115,z),30,3,'Orange' if z==177 else 'Steel')
cyl('Filter cap',(109,115,224),31,10,'Steel')
for a in range(8):
 angle=a*math.tau/8;pipe('Collector seam',(109+29*math.cos(angle),115+29*math.sin(angle),139),(109+29*math.cos(angle),115+29*math.sin(angle),210),1,'Dark')
box('Filter drawer',(109,115,72),(60,55,67),'Ivory');box('Filter drawer seam',(109,144,73),(48,2,3),'Dark')
fan_seat(104,-19,154)
for y in [-122,132]:
 pipe('Handrail',(-142,y,63),(-18,y,63),2,'Orange')
 for x in [-142,-18]:pipe('Railing post',(x,y,26),(x,y,63),2,'Steel')
silica=export('SM_SilicaExtractor_Refined');silica.hide_render=True
sd=drill_mesh('SM_SilicaExtractor_Drill');sd.hide_render=True
sf=fan_mesh('SM_SilicaExtractor_Fan');sf.hide_render=True
models.append(('SilicaExtractor',silica,sd,sf,Vector((104,-19,158))))
# Rare minerals: a taller reinforced coring rig and enclosed mineral sorting module.
foundation();cabinet((100,20,93),(134,193,134))
for x in [-140,-20]:
 for y in [-72,72]:
  box('Heavy column base',(x,y,40),(32,32,28),'Steel');box('Box-section upright',(x,y,163),(19,19,243),'Ivory');box('Column safety stripe',(x,y,73),(21,21,12),'Orange')
  pipe('Triangulated support',(x,y,54),(-80,y,270),4.5,'Steel')
for y in [-72,72]:
 box('Top beam',(-80,y,285),(151,29,24),'Ivory');box('Top beam band',(-80,y-16,285),(126,3,7),'Orange')
 for x in [-139,-21]:cyl('Beam bolt',(x,y,300),4,5,'Steel')
box('Hoist deck',(-80,0,281),(68,156,16),'Steel');cyl('Core drill motor',(-80,0,249),29,48,'Ivory');torus('Motor flange',(-80,0,226),30,4,'Orange')
for y in [-48,48]:pipe('Hydraulic cylinder',(-80,y,95),(-80,y,185),9,'Dark');pipe('Hydraulic ram',(-80,y,185),(-80,y,246),5,'Steel')
for z in [98,154,210]:box('Sliding carriage',(-80,0,z),(91,116,11),'Steel')
pipe('Hydraulic supply',(-15,38,75),(32,38,75),10,'Orange')
for x in [-5,22]:cyl('Bolted pipe flange',(x,38,75),14,6,'Steel').rotation_euler.y=math.pi/2
box('Ore classifier',(102,130,80),(100,54,98),'Ivory')
for x in [66,101,136]:box('Classifier access seam',(x,159,80),(2,2,78),'Dark')
box('Sample chute',(-81,-110,43),(75,54,30),'Steel')
for x in [-103,-79,-55]:box('Conveyor roller',(x,-113,59),(16,44,5),'Dark')
box('Sealed sample tray',(-81,-137,45),(78,7,30),'Orange')
fan_seat(104,27,166)
for x in [-164,-153]:pipe('Service ladder rail',(x,80,30),(x,80,269),2,'Steel')
for z in range(44,265,20):pipe('Ladder rung',(-166,80,z),(-150,80,z),2,'Steel')
rare=export('SM_RareMineralMine_Refined');rare.hide_render=True
rd=drill_mesh('SM_RareMineralMine_Drill',True);rd.hide_render=True
rf=fan_mesh('SM_RareMineralMine_Fan');rf.hide_render=True
models.append(('RareMineralMine',rare,rd,rf,Vector((104,27,170))))
scene.render.engine='CYCLES';scene.cycles.samples=24
bpy.ops.object.camera_add(location=(650,-820,630));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,110))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=590;cam.data.clip_end=10000;scene.camera=cam
for pos,energy in [((0,-300,700),20000000),((-300,150,400),10000000)]:
 bpy.ops.object.light_add(type='AREA',location=pos);light=bpy.context.object;light.data.energy=energy;light.data.size=450;light.rotation_euler=(Vector((0,0,90))-light.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('MineralMineStudio');scene.world.color=(.2,.2,.2)
scene.render.resolution_x=512;scene.render.resolution_y=512;scene.render.resolution_percentage=100;scene.render.film_transparent=True;scene.render.image_settings.file_format='PNG'
for key,body,drill,fan,pivot in models:
 body.hide_render=False;drill.hide_render=False;fan.hide_render=False;drill.location=(-80,0,44);fan.location=pivot;scene.render.filepath=str(out/('T_'+key+'_Refined.png'));bpy.ops.render.render(write_still=True)
 body.hide_render=True;drill.hide_render=True;fan.hide_render=True
for i,(_,body,drill,fan,pivot) in enumerate(models):
 body.hide_render=False;drill.hide_render=False;fan.hide_render=False;body.location=(i*520,0,0);drill.location=Vector((-80,0,44))+body.location;fan.location=pivot+body.location
bpy.data.libraries.write(str(out/'MineralMines.blend'),{scene},fake_user=True)
result={'source':str(out),'models':[m[0] for m in models],'footprint_cm':[390,350]}
