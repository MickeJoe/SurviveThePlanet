"""Approved compact, low uranium surface formation; execute through native MCP."""
import bpy,math,random,json
from pathlib import Path
from mathutils import Vector
root=Path('C:/UE5/SurviveThePlanet 5.8')
exec((root/'Scripts/create_geothermal_blender.py').read_text().split("box('Platform'")[0])
scene.name='UraniumSurfaceWorkshop';out=root/'ContentSource/Uranium';out.mkdir(parents=True,exist_ok=True)
rng=random.Random(81123)
# Low uneven footprint: no tall ore pile or perfect ring.
outline=[Vector((math.cos(a)*rng.uniform(110,126),math.sin(a)*rng.uniform(91,113))) for a in [i*math.tau/19 for i in range(19)]]
seeds=[Vector((rng.uniform(-90,90),rng.uniform(-75,75))) for i in range(18)]
def clip(poly,n,d):
 result=[]
 for a,b in zip(poly,poly[1:]+poly[:1]):
  da=n.dot(a)-d;db=n.dot(b)-d
  if da<=0:result.append(a)
  if (da<=0)!=(db<=0):result.append(a+(b-a)*da/(da-db))
 return result
for index,seed in enumerate(seeds):
 poly=outline[:]
 for other in seeds:
  if other==seed:continue
  n=other-seed;poly=clip(poly,n,(other.length_squared-seed.length_squared)*.5)
  if len(poly)<3:break
 if len(poly)<3:continue
 center=sum(poly,Vector((0,0)))/len(poly);poly=[center+(p-center)*.94 for p in poly]
 height=rng.uniform(5,17);count=len(poly)
 vertices=[(p.x,p.y,-1) for p in poly]+[(p.x,p.y,height+rng.uniform(-2,2)) for p in poly]
 faces=[tuple(range(count,2*count))]+[(i,(i+1)%count,(i+1)%count+count,i+count) for i in range(count)]
 mesh=bpy.data.meshes.new('Weathered bedrock');mesh.from_pydata(vertices,[],faces);mesh.update();obj=bpy.data.objects.new('Low fractured slab',mesh);scene.collection.objects.link(obj);finish(obj,obj.name,'Rock')
 bevel=obj.modifiers.new('Broken corners','BEVEL');bevel.width=1.2;bevel.segments=1
 # Irregular mineral crust appears ON the slabs, not a luminous bed under cracks.
 for j in range(rng.randint(2,5)):
  target=center+(rng.choice(poly)-center)*rng.uniform(.1,.65)
  bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1,radius=1,location=(target.x,target.y,height+1))
  o=bpy.context.object;o.scale=(rng.uniform(3,9),rng.uniform(2,7),.7);finish(o,'Olive mineral crust','Mineral')
 if index%3==0:
  target=poly[0]*.6+center*.4
  pipe('Short mineral vein',(center.x,center.y,height+1),(target.x,target.y,height+1),.9,'Mineral')
for i in range(27):
 angle=rng.uniform(0,math.tau);radius=rng.uniform(102,131)
 bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1,radius=1,location=(radius*math.cos(angle),radius*.84*math.sin(angle),rng.uniform(0,3)))
 obj=bpy.context.object;obj.scale=(rng.uniform(3,9),rng.uniform(3,7),rng.uniform(2,6));finish(obj,'Scattered bedrock','Rock')
deposit=export('SM_UraniumSurface')
scene.render.engine='CYCLES';scene.cycles.samples=24
bpy.ops.object.camera_add(location=(350,-390,360));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,6))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=320;cam.data.clip_end=10000;scene.camera=cam
for pos,energy in [((0,-300,500),12000000),((-300,150,400),7000000)]:
 bpy.ops.object.light_add(type='AREA',location=pos);l=bpy.context.object;l.data.energy=energy;l.data.size=350;l.rotation_euler=(Vector((0,0,5))-l.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('UraniumStudio');scene.world.color=(.2,.2,.2)
scene.render.resolution_x=512;scene.render.resolution_y=512;scene.render.resolution_percentage=100;scene.render.film_transparent=True;scene.render.image_settings.file_format='PNG';scene.render.filepath=str(out/'T_UraniumSurface.png')
bpy.ops.wm.save_as_mainfile(filepath=str(out/'UraniumSurface.blend'));bpy.ops.render.render(write_still=True)
result={'mesh':deposit.name,'source':str(out),'height_cm':19,'diameter_cm':275}
