import bpy,math,json
from pathlib import Path
from mathutils import Vector
root=Path('C:/UE5/SurviveThePlanet 5.8')
exec((root/'Scripts/create_nuclear_buildings_blender.py').read_text().split('deck(450,410)')[0])
scene.name='WaterCoolingWorkshop'
out=root/'ContentSource/WaterCooling';out.mkdir(parents=True,exist_ok=True)
buildings=[]
for key in ['WaterPurifier','CoolingSystemsPlant']:
 deck(450,380)
 if key=='WaterPurifier':
  # Twin filtration towers and an enclosed UV treatment module.
  for x in [-125,125]:
   tank(x,70,44,190)
   pipe('Water inlet',(x,65,77),(x,-65,77),9,'Steel')
   pipe('Clean water return',(x,65,166),(x,-35,166),7,'Orange')
  box('UV treatment enclosure',(0,-72,110),(200,165,154),'Dark')
  panels(0,-156,114,195,140)
  panels(0,12,114,195,140)
  for x in [-102,102]:
   for y in [-125,-72,-19]:box('Side service panel',(x,y,111),(3,48,137),'Ivory')
   box('Side hazard stripe',(x, -72,49),(4,151,6),'Orange')
  box('Treatment roof',(0,-72,191),(208,173,10),'Ivory')
  for x in [-69,0,69]:box('Roof plate',(x,-72,198),(65,158,3),'Ivory')
  for x in [-72,0,72]:box('UV monitoring window',(x,-161,128),(34,3,28),'Cyan')
  box('Clean water outlet',(0,-118,57),(63,105,32),'Steel')
  pipe('Transfer manifold',(-125,-65,77),(125,-65,77),9,'Steel')
  pivot=(0,-72,201)
 else:
  # Central heat exchanger with visible radiator banks and coolant reservoirs.
  box('Heat exchanger chassis',(0,15,111),(246,220,148),'Dark')
  panels(0,-97,110,244,139)
  panels(0,127,110,244,139)
  for x in [-125,125]:
   for y in [-58,15,88]:box('Heat exchanger side panel',(x,y,111),(3,66,137),'Ivory')
   box('Side hazard stripe',(x,15,49),(4,204,6),'Orange')
  box('Fan deck',(0,15,191),(254,228,12),'Ivory')
  for x in [-84,0,84]:box('Roof service plate',(x,15,199),(79,212,3),'Ivory')
  for x in [-170,170]:
   tank(x,65,32,142)
   for j in range(9):box('Radiator fin',(x,-112+j*13,104),(50,5,106),'Steel')
   pipe('Coolant manifold',(x,65,90),(x,-116,90),8,'Orange')
   pipe('Heat exchanger feed',(x,65,135),(x*.55,65,135),8,'Steel')
  for x in [-74,74]:box('Production status',(x,-101,129),(28,3,14),'Cyan')
  box('Output hatch',(0,-104,81),(84,9,65),'Steel')
  box('Hatch inset',(0,-111,84),(66,3,43),'Dark')
  pivot=(0,30,206)
 cyl('Fan recess',pivot,43,5,'Dark');torus('Fan guard',(pivot[0],pivot[1],pivot[2]+4),44,3,'Steel')
 for i in range(8):
  a=i*math.tau/8;pipe('Fan guard spokes',(pivot[0],pivot[1],pivot[2]+9),(pivot[0]+42*math.cos(a),pivot[1]+42*math.sin(a),pivot[2]+9),1.4,'Steel')
 for x in [-196,196]:
  for y in [-159,159]:box('Deck worklight',(x,y,40),(12,12,7),'Lamp')
 body=export('SM_'+key+'_Refined');body.hide_render=True
 buildings.append((key,body,pivot))
cyl('Fan spindle',(0,0,0),9,6,'Steel')
for i in range(6):
 a=i*math.tau/6;o=box('Cooling fan blade',(23*math.cos(a),23*math.sin(a),0),(33,11,3),'Steel');o.rotation_euler.z=a+.28
fan=export('SM_ThermalFan');fan.hide_render=True
scene.render.engine='CYCLES';scene.cycles.samples=24
bpy.ops.object.camera_add(location=(650,-800,590));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,100))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=610;cam.data.clip_end=10000;scene.camera=cam
for pos,energy in [((0,-300,700),20000000),((-300,150,400),10000000)]:
 bpy.ops.object.light_add(type='AREA',location=pos);light=bpy.context.object;light.data.energy=energy;light.data.size=450;light.rotation_euler=(Vector((0,0,90))-light.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('ThermalStudio');scene.world.color=(.2,.2,.2)
scene.render.resolution_x=512;scene.render.resolution_y=512;scene.render.resolution_percentage=100;scene.render.film_transparent=True;scene.render.image_settings.file_format='PNG'
for key,body,pivot in buildings:
 body.hide_render=False;fan.hide_render=False;fan.location=pivot
 scene.render.filepath=str(out/('T_'+key+'_Refined.png'));bpy.ops.render.render(write_still=True);body.hide_render=True;fan.hide_render=True
bpy.ops.wm.save_as_mainfile(filepath=str(out/'WaterCooling.blend'))
result={'source':str(out),'pivots':{key:[p[0],-p[1],p[2]] for key,body,p in buildings}}
