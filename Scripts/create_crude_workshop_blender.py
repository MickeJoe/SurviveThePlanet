"""Create the approved small Base Module-family workshop via native Blender MCP."""
import bpy, math, json, random
from pathlib import Path
from mathutils import Vector
root=Path('C:/UE5/SurviveThePlanet 5.8')
out=root/'ContentSource/CrudeWorkshop';out.mkdir(parents=True,exist_ok=True)
scene=bpy.data.scenes.new('STP_CrudeWorkshop');bpy.context.window.scene=scene
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.01
palette={'Ivory':(.78,.76,.69),'Orange':(.8,.18,.028),'Graphite':(.055,.066,.076),'Steel':(.38,.41,.42),'Cyan':(.02,.55,.7)}
mats={};parts=[]
for name,color in palette.items():
 m=bpy.data.materials.new('Extender_'+name);m.diffuse_color=(*color,1);m.use_nodes=True
 n=m.node_tree.nodes;p=n.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Metallic'].default_value=.15 if name=='Ivory' else .55
 noise=n.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=7
 ramp=n.new('ShaderNodeValToRGB');ramp.color_ramp.elements[0].color=(*(v*.62 for v in color),1);ramp.color_ramp.elements[1].color=(*color,1)
 m.node_tree.links.new(noise.outputs['Fac'],ramp.inputs[0]);m.node_tree.links.new(ramp.outputs[0],p.inputs['Base Color'])
 p.inputs['Roughness'].default_value=.58
 bump=n.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.12;bump.inputs['Distance'].default_value=.12;m.node_tree.links.new(noise.outputs['Fac'],bump.inputs['Height']);m.node_tree.links.new(bump.outputs[0],p.inputs['Normal'])
 if name=='Cyan':p.inputs['Emission Color'].default_value=(*color,1);p.inputs['Emission Strength'].default_value=2
 mats[name]=m
def finish(o,name,mat):
 o.name=name;o.data.materials.append(mats[mat]);parts.append(o);return o
def box(name,p,d,mat,bevel=1.4):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.scale=d;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:b=o.modifiers.new('Rounded prefab edges','BEVEL');b.width=bevel;b.segments=3
 return finish(o,name,mat)
def cyl(name,p,r,h,mat):
 bpy.ops.mesh.primitive_cylinder_add(vertices=24,radius=r,depth=h,location=p);o=finish(bpy.context.object,name,mat)
 b=o.modifiers.new('Machined edges','BEVEL');b.width=.7;b.segments=2;return o
def pipe(name,a,b,r,mat):
 a,b=Vector(a),Vector(b);o=cyl(name,(a+b)/2,r,(b-a).length,mat);o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();return o
def export(name):
 bpy.ops.object.select_all(action='DESELECT')
 for o in parts:
  o.select_set(True);bpy.context.view_layer.objects.active=o
  for mod in list(o.modifiers):bpy.ops.object.modifier_apply(modifier=mod.name)
  uv=o.data.uv_layers.new(name='SurfaceUV')
  for f in o.data.polygons:
   axis=max(range(3),key=lambda a:abs(f.normal[a]));axes=[a for a in range(3) if a!=axis]
   for i in f.loop_indices:
    v=o.matrix_world@o.data.vertices[o.data.loops[i].vertex_index].co;uv.data[i].uv=(v[axes[0]]/100,v[axes[1]]/100)
 bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name;scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
 bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,apply_unit_scale=True,axis_forward='-Y',axis_up='Z',bake_anim=False,mesh_smooth_type='FACE',object_types={'MESH'})
 parts.clear();return o
# Open-front shell: broad panels and orange belts remain readable at gameplay scale.
box('Foundation',(0,0,7),(368,310,14),'Graphite',5)
box('Inset deck',(0,-2,17),(352,296,9),'Steel',3)
box('Back shell',(0,126,103),(338,16,166),'Graphite',6)
for x in [-163,163]:box('Side shell',(x,6,103),(15,242,166),'Graphite',5)
for x in [-166,166]:
 for y in [-115,120]:
  box('Armored rounded corner',(x,y,108),(28,30,181),'Ivory',9)
  box('Corner orange collar',(x,y,58),(29,31,10),'Orange',2)
for side in [-1,1]:
 for y in [-70,10,90]:
  box('Side access panel',(side*173,y,110),(5,74,135),'Ivory',2)
  box('Panel stripe',(side*176,y,59),(2,73,7),'Orange',.4)
  for yy in [y-27,y+27]:
   for z in [52,165]:pipe('Panel fastener',(side*176,yy,z),(side*179,yy,z),2,'Steel')
 box('Side ventilation recess',(side*177,50,123),(3,51,29),'Graphite',2)
 for y in [30,40,50,60,70]:box('Vent slat',(side*180,y,123),(3,3,22),'Steel',.5)
for x in [-116,-39,39,116]:
 box('Rear panel',(x,136,112),(72,5,140),'Ivory',2)
 box('Rear orange belt',(x,139,58),(72,2,8),'Orange',.4)
box('Roof dark gasket',(0,5,194),(355,270,17),'Graphite',7)
box('Rounded roof cap',(0,5,205),(358,274,20),'Ivory',9)
for x in [-113,0,113]:
 box('Roof service panel',(x,10,218),(104,220,7),'Ivory',4)
 for y in [-86,108]:
  for xx in [x-41,x+41]:cyl('Roof hex bolt',(xx,y,223),2.3,2,'Steel')
for x in [-143,143]:box('Orange roof runner',(x,8,219),(9,242,5),'Orange',2)
box('Portal header',(0,-124,182),(305,27,28),'Ivory',5)
box('Door rail',(0,-141,175),(274,7,10),'Graphite',1)
for z in [181,188,195]:box('Retracted shutter',(0,-139,z),(253,4,4),'Steel',.7)
for x in [-143,143]:
 box('Portal pillar',(x,-126,103),(19,29,147),'Ivory',3)
 box('Portal orange stripe',(x,-142,58),(20,2,9),'Orange',1)
box('Threshold',(0,-145,25),(274,18,9),'Steel',2)
box('Entry ramp',(0,-164,10),(260,31,10),'Graphite',2)
for x in range(-120,121,24):
 o=box('Safety threshold',(x,-153,30),(13,8,1),'Orange',.3);o.rotation_euler.z=.35
# Interior is physical geometry, no flat black doorway.
box('Bench frame',(0,79,73),(246,53,14),'Graphite',2)
box('Bench surface',(0,77,83),(250,58,7),'Steel',2)
for x in [-108,108]:box('Bench legs',(x,87,49),(10,36,53),'Steel',1)
for x in [-75,-25,25,75]:
 box('Tool board',(x,117,126),(42,4,67),'Graphite',2)
 for z in [111,132,151]:box('Visible hand tool',(x,112,z),(29,6,5),'Steel',1)
box('Small furnace',(78,55,112),(55,61,52),'Ivory',4)
box('Furnace port',(78,23,113),(34,4,25),'Graphite',2)
box('Press base',(-62,30,91),(69,65,13),'Steel',2)
for x in [-88,-36]:pipe('Press column',(x,42,96),(x,42,163),4,'Steel')
box('Press crosshead',(-62,42,162),(76,22,17),'Orange',3)
box('Control cabinet',(119,-60,61),(37,50,76),'Ivory',3)
box('Cabinet control face',(119,-87,77),(27,3,26),'Graphite',1)
box('Status screen',(119,-90,81),(18,2,9),'Cyan',.5)
for x in [-119,-67]:
 box('Input storage bin',(x,-62,37),(41,45,30),'Steel',2)
 box('Bin dark interior',(x,-62,53),(32,37,2),'Graphite',1)
 for i in range(3):box('Raw stock',(x-9+i*9,-62,56+i%2*3),(7,29,7),'Orange' if x==-67 else 'Steel',1)
# A short vent and side cable connector, rather than a large factory chimney.
cyl('Vent socket',(87,75,228),25,13,'Graphite')
cyl('Roof vent',(87,75,244),19,27,'Steel')
cyl('Vent cap',(87,75,262),26,7,'Ivory')
for x in [-15,15]:pipe('Vent cap support',(87+x,75,250),(87+x,75,260),2,'Steel')
pipe('Power socket',(173,88,45),(188,88,45),10,'Graphite')
pipe('Power cable',(188,88,45),(191,62,28),5,'Graphite')
for x in [-164,164]:
 for y in [-125,125]:cyl('Foundation anchor',(x,y,26),6,7,'Steel')
body=export('SM_CrudeWorkshop')
# Separate moving parts have local pivots and are animated only during real production.
cyl('Spindle',(0,0,0),9,19,'Steel')
for a in range(6):
 ang=a*math.tau/6;o=box('Fan blade',(18*math.cos(ang),18*math.sin(ang),0),(25,8,3),'Graphite',1);o.rotation_euler.z=ang+.35
fan=export('SM_WorkshopFan');fan.location=(-82,64,226)
# Add a static visible fan well to the body.
cyl('Fan grille well',(-82,64,224),34,4,'Graphite')
for y in [43,54,65,76,87]:box('Fan cage',(-82,y,231),(54,2,2),'Steel',.3)
grille=export('WorkshopGrille')
bpy.ops.object.select_all(action='DESELECT');body.select_set(True);grille.select_set(True);bpy.context.view_layer.objects.active=body;bpy.ops.object.join()
bpy.ops.export_scene.fbx(filepath=str(out/'SM_CrudeWorkshop.fbx'),use_selection=True,apply_unit_scale=True,axis_forward='-Y',axis_up='Z',bake_anim=False,mesh_smooth_type='FACE',object_types={'MESH'})
box('Ram',(0,0,0),(34,28,12),'Steel',2);cyl('Ram piston',(0,0,16),6,26,'Steel')
ram=export('SM_WorkshopRam');ram.location=(-62,32,121)
for o in [fan,ram]:
 o.keyframe_insert(data_path='rotation_euler' if o==fan else 'location',frame=1)
 if o==fan:o.rotation_euler.z=math.tau
 else:o.location.z-=14
 o.keyframe_insert(data_path='rotation_euler' if o==fan else 'location',frame=30)
 if o==ram:o.location.z+=14;o.keyframe_insert(data_path='location',frame=60)
scene.frame_set(1)
scene.render.engine='CYCLES';scene.cycles.samples=24
scene.world=bpy.data.worlds.new('WorkshopStudio');scene.world.color=(.3,.3,.3)
bpy.ops.object.camera_add(location=(540,-720,510));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,105))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=510;cam.data.clip_end=10000;scene.camera=cam
for pos,energy,size in [((80,-290,640),19000000,430),((-390,100,380),11000000,400),((0,-110,125),550000,120)]:
 bpy.ops.object.light_add(type='AREA',location=pos);o=bpy.context.object;o.data.energy=energy;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(Vector((0,25,80))-o.location).to_track_quat('-Z','Y').to_euler()
scene.render.resolution_x=768;scene.render.resolution_y=768;scene.render.resolution_percentage=100;scene.render.film_transparent=True
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(out/'T_CrudeWorkshop.png')
bpy.ops.wm.save_as_mainfile(filepath=str(out/'CrudeWorkshop.blend'));bpy.ops.render.render(write_still=True)
result={'source':str(out),'footprint_cm':[368,341],'vertices':len(body.data.vertices),'parts':['SM_CrudeWorkshop','SM_WorkshopFan','SM_WorkshopRam']}
(out/'ModelReport.json').write_text(json.dumps(result,indent=2))
