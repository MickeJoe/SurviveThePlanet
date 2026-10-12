import bpy, math, random
from pathlib import Path
from mathutils import Vector
out=Path('C:/UE5/SurviveThePlanet 5.8/ContentSource/Geothermal');out.mkdir(parents=True,exist_ok=True)
scene=bpy.data.scenes.new('GeothermalWorkshop');bpy.context.window.scene=scene
scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.01
palette={'Ivory':(.72,.70,.63),'Orange':(.85,.21,.025),'Dark':(.06,.075,.085),'Steel':(.25,.30,.32),'Cyan':(.02,.65,.9),'Rock':(.11,.08,.055),'Mineral':(.55,.35,.055),'Heat':(1,.18,.008),'Lamp':(1,.75,.36)}
mats={}
for k,c in palette.items():
 m=bpy.data.materials.new('Geo_'+k);m.diffuse_color=(*c,1);m.use_nodes=True
 p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*c,1);p.inputs['Roughness'].default_value=.57
 if k in ['Cyan','Heat','Lamp']:p.inputs['Emission Color'].default_value=(*c,1);p.inputs['Emission Strength'].default_value=2
 mats[k]=m
parts=[]
def finish(o,name,mat):
 o.name=name;o.data.materials.append(mats[mat]);parts.append(o);return o
def box(name,p,d,mat):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.scale=d
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 b=o.modifiers.new('Machined edges','BEVEL');b.width=1.3;b.segments=2
 return finish(o,name,mat)
def cyl(name,p,r,h,mat):
 bpy.ops.mesh.primitive_cylinder_add(vertices=24,radius=r,depth=h,location=p);return finish(bpy.context.object,name,mat)
def pipe(name,a,b,r,mat):
 a,b=Vector(a),Vector(b);o=cyl(name,(a+b)/2,r,(b-a).length,mat);o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();return o
def torus(name,p,r,minor,mat):
 bpy.ops.mesh.primitive_torus_add(major_segments=32,minor_segments=8,major_radius=r,minor_radius=minor,location=p);return finish(bpy.context.object,name,mat)
def export(name):
 bpy.ops.object.select_all(action='DESELECT')
 for o in parts:
  o.select_set(True);bpy.context.view_layer.objects.active=o
  for mod in list(o.modifiers):bpy.ops.object.modifier_apply(modifier=mod.name)
 for obj in parts:
  uv=obj.data.uv_layers.new(name='SurfaceUV')
  for face in obj.data.polygons:
   normal=face.normal;axis=max(range(3),key=lambda a:abs(normal[a]))
   axes=[a for a in range(3) if a!=axis]
   for index in face.loop_indices:
    p=obj.matrix_world @ obj.data.vertices[obj.data.loops[index].vertex_index].co
    uv.data[index].uv=(p[axes[0]]/120,p[axes[1]]/120)
 bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
 scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
 bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,apply_unit_scale=True,axis_forward='-Y',axis_up='Z',bake_anim=False,mesh_smooth_type='FACE',object_types={'MESH'})
 parts.clear();return o
box('Platform',(0,0,9),(280,260,18),'Dark');box('Deck',(0,0,20),(270,250,8),'Ivory')
# Offset the towers in depth so both read from the gameplay camera.
for x,y,h,r in [(-65,30,172,42),(65,90,125,37)]:
 cyl('Tower foot',(x,y,32),r+7,14,'Dark')
 cyl('Heat exchanger',(x,y,35+h/2),r,h,'Steel')
 for section in range(4):
  z=43+section*(h-16)/4
  cyl('Painted jacket',(x,y,z+(h-24)/8),r+.6,(h-24)/4-3,'Ivory')
 for z in [43,35+h*.62,35+h-8]:
  cyl('Flanged collar',(x,y,z),r+3,6,'Orange' if z==35+h*.62 else 'Dark')
  for a in range(8):
   ang=a*math.tau/8
   cyl('Collar bolt',(x+(r+1)*math.cos(ang),y+(r+1)*math.sin(ang),z+4),2.2,3,'Steel')
 cyl('Vent recess',(x,y,35+h+1),r-6,3,'Dark')
 torus('Vent rim',(x,y,35+h),r,4,'Steel')
 for a in range(8):
  ang=a*math.tau/8
  xx=x+(r+2)*math.cos(ang); yy=y+(r+2)*math.sin(ang)
  pipe('Jacket seam',(xx,yy,49),(xx,yy,35+h-15),1.5,'Dark')
 for a in range(8):
  ang=a*math.tau/8
  xx=x+(r+9)*math.cos(ang);yy=y+(r+9)*math.sin(ang)
  pipe('Handrail post',(xx,yy,35+h-30),(xx,yy,35+h-5),1.4,'Steel')
 torus('Service rail',(x,y,35+h-5),r+9,1.8,'Orange')
 box('Inspection panel',(x,y-r-2,104),(26,4,34),'Dark')
 box('Panel insert',(x,y-r-4,104),(21,2,27),'Steel')
 box('Status',(x,y-r-6,104),(15,2,6),'Cyan')
 for z in [60,75,90]:box('Service ladder rung',(x+r+5,y,z),(12,20,3),'Steel')
 pipe('Hot pipe',(x,y-r,66),(x,-12,66),12,'Orange')
 pipe('Hot elbow',(x,-12,66),(x*.45,-35,42),12,'Orange')
 for yy in [y-r-4,-8]:
  flange=cyl('Pipe flange',(x,yy,66),17,6,'Dark');flange.rotation_euler.x=math.pi/2
  for a in range(6):
   angle=a*math.tau/6
   bolt=cyl('Flange bolt',(x+14*math.cos(angle),yy-4,66+14*math.sin(angle)),2,4,'Steel');bolt.rotation_euler.x=math.pi/2
 torus('Valve wheel',(x,-16,90),11,2,'Orange')
 pipe('Valve stem',(x,-16,67),(x,-16,90),2,'Steel')
cyl('Wellhead',(0,-30,35),24,20,'Steel');torus('Well rim',(0,-30,47),24,4,'Orange')
cyl('Heat core',(0,-30,47),17,2,'Heat')
box('Turbine plinth',(0,-65,30),(132,74,12),'Dark')
box('Turbine housing',(0,-65,57),(125,64,62),'Ivory')
for x in [-44,0,44]:box('Housing seam',(x,-98,58),(2,2,52),'Dark')
box('Access panel',(0,-100,57),(35,3,32),'Steel')
for x in [-13,13]:
 for z in [45,69]:cyl('Access screw',(x,-103,z),2,3,'Dark').rotation_euler.x=math.pi/2
for x in [-53,-42,42,53]:box('Intake slot',(x,-99,60),(5,2,27),'Dark')
pipe('Turbine shroud',(64,-65,57),(74,-65,57),31,'Steel')
ring=torus('Turbine lip',(75,-65,57),29,3,'Orange');ring.rotation_euler.y=math.pi/2
pipe('Generator shaft',(-72,-65,57),(72,-65,57),25,'Dark')
for x in [-52,-26,0,26,52]:box('Cooling fin',(x,-65,89),(4,54,2),'Steel')
box('Housing band',(0,-99,55),(120,2,5),'Orange')
for x in [-122,122]:
 for y in [-106,106]:
  box('Floodlight post',(x,y,34),(4,4,20),'Dark');box('Floodlight',(x,y,46),(11,11,7),'Lamp')
for x in [-100,100]:
 for j in range(8):box('Deck grille',(x,-20+j*10,25),(24,3,1),'Dark')
for x in [-90,90]:
 pipe('Rail',(x,-115,43),(x,0,43),1.4,'Orange')
 for y in [-115,-60,0]:pipe('Rail post',(x,y,25),(x,y,43),1.4,'Steel')
plant=export('SM_GeothermalPlant')
# Rotor pivots around the horizontal X axis, matching Unreal roll animation.
pipe('Rotor axle',(-17,0,0),(17,0,0),8,'Steel')
for a in range(8):
 ang=a*math.tau/8;o=box('Blade',(0,15*math.cos(ang),15*math.sin(ang)),(5,22,6),'Steel');o.rotation_euler.x=ang
rotor=export('SM_GeothermalRotor');rotor.location=(74,-65,57)
# Fractured basalt: clipped Voronoi polygons with broad connected gaps.
rng=random.Random(98117)
outline=[]
for i in range(23):
 angle=i*math.tau/23;radius=rng.uniform(137,165)
 outline.append(Vector((radius*math.cos(angle),radius*math.sin(angle))))
seeds=[]
while len(seeds)<32:
 p=Vector((rng.uniform(-145,145),rng.uniform(-140,140)))
 if p.length<143 and all((p-q).length>29 for q in seeds):seeds.append(p)
def clip(poly,n,d):
 result=[]
 for a,b in zip(poly,poly[1:]+poly[:1]):
  da=a.dot(n)-d;db=b.dot(n)-d
  if da<=0:result.append(a)
  if (da<0)!=(db<0):result.append(a+(b-a)*da/(da-db))
 return result
def polygon_mesh(name,poly,z,material):
 mesh=bpy.data.meshes.new(name)
 mesh.from_pydata([(p.x,p.y,z) for p in poly],[],[tuple(range(len(poly)))])
 mesh.update();obj=bpy.data.objects.new(name,mesh);scene.collection.objects.link(obj)
 return finish(obj,name,material)
polygon_mesh('Dark scorched bed',outline,.2,'Dark')
polygon_mesh('Connected molten fissures',[p*.97 for p in outline],1.5,'Heat')
for seed in seeds:
 poly=outline[:]
 for other in seeds:
  if other is seed:continue
  n=other-seed;poly=clip(poly,n,(other.length_squared-seed.length_squared)/2)
  if len(poly)<3:break
 if len(poly)<3:continue
 center=sum(poly,Vector((0,0)))/len(poly)
 # Keep 4-8 cm open fissures rather than a hidden glowing underlay.
 poly=[center+(p-center)*rng.uniform(.82,.88) for p in poly]
 height=rng.uniform(6,13);count=len(poly)
 verts=[(p.x,p.y,2) for p in poly]+[(p.x,p.y,height+rng.uniform(-2,2)) for p in poly]
 faces=[tuple(range(count,2*count))]+[(i,(i+1)%count,(i+1)%count+count,i+count) for i in range(count)]
 mesh=bpy.data.meshes.new('Fractured basalt');mesh.from_pydata(verts,[],faces);mesh.update()
 obj=bpy.data.objects.new('Fractured basalt',mesh);scene.collection.objects.link(obj);finish(obj,obj.name,'Rock')
 bevel=obj.modifiers.new('Chipped edges','BEVEL');bevel.width=1.6;bevel.segments=1
for i in range(39):
 angle=rng.uniform(0,math.tau);radius=rng.uniform(150,190)
 bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1,radius=1,location=(radius*math.cos(angle),radius*math.sin(angle),rng.uniform(1,6)))
 obj=bpy.context.object;obj.scale=(rng.uniform(5,17),rng.uniform(4,13),rng.uniform(2,9))
 finish(obj,'Scattered basalt','Rock')
 # Low, subdued mineral crust instead of bright gold perimeter rocks.
 if i%3==0:
  bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1,radius=1,location=(radius*.94*math.cos(angle),radius*.94*math.sin(angle),1.8))
  obj=bpy.context.object;obj.scale=(rng.uniform(6,16),rng.uniform(4,11),1)
  finish(obj,'Mineral crust','Mineral')
site=export('SM_GeothermalSite');site.location=(400,0,0)
scene.render.engine='CYCLES';scene.cycles.samples=24
bpy.ops.object.camera_add(location=(560,-680,580));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,85))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=425;cam.data.clip_end=10000;scene.camera=cam
for pos,energy in [((0,-300,700),15000000),((-300,150,400),8000000)]:
 bpy.ops.object.light_add(type='AREA',location=pos);l=bpy.context.object;l.data.energy=energy;l.data.shape='DISK';l.data.size=450
 l.rotation_euler=(Vector((0,0,70))-l.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('GeoStudio');scene.world.color=(.2,.2,.2)
scene.render.resolution_x=512;scene.render.resolution_y=512;scene.render.resolution_percentage=100;scene.render.film_transparent=True
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(out/'T_GeothermalPlant.png')
site.hide_render=True;bpy.ops.wm.save_as_mainfile(filepath=str(out/'GeothermalWorkshop.blend'));bpy.ops.render.render(write_still=True)
result={'meshes':['SM_GeothermalPlant','SM_GeothermalRotor','SM_GeothermalSite'],'platform_cm':[280,260],'site_diameter_cm':360,'source':str(out)}
