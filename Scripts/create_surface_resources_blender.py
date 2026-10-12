"""Author the three approved game deposits through Blender's native MCP."""
import bpy, math, random, json
from pathlib import Path
from mathutils import Vector
root=Path('C:/UE5/SurviveThePlanet 5.8')
# Reuse the project's centimetre FBX exporter and world-projected surface UVs.
exec(compile((root/'Scripts/create_geothermal_blender.py').read_text().split("box('Platform'")[0], 'geothermal_helpers', 'exec'))
scene.name='SurfaceResourceWorkshop';out=root/'ContentSource/SurfaceResources';out.mkdir(parents=True,exist_ok=True)
palette={'PaleRock':(.46,.39,.27),'Quartz':(.78,.82,.78),'QuartzShadow':(.40,.48,.49),
 'RareRock':(.035,.046,.059),'Amethyst':(.30,.045,.47),'RareVein':(.22,.04,.37),'MetalFleck':(.33,.29,.36),
 'Obsidian':(.022,.033,.034),'AlienShell':(.047,.070,.058),'AlienCore':(.015,.38,.34),'AlienVein':(.012,.24,.21)}
mats={}
for key,color in palette.items():
 mat=bpy.data.materials.new('Deposit_'+key);mat.diffuse_color=(*color,1);mat.use_nodes=True
 nodes=mat.node_tree.nodes;links=mat.node_tree.links;p=nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1)
 p.inputs['Roughness'].default_value=.32 if key in ['Quartz','QuartzShadow','Amethyst','AlienCore'] else .76
 p.inputs['Metallic'].default_value=.35 if key in ['MetalFleck','Amethyst','Obsidian'] else .05
 tex=nodes.new('ShaderNodeTexNoise');tex.inputs['Scale'].default_value=6;tex.inputs['Detail'].default_value=3
 ramp=nodes.new('ShaderNodeValToRGB');ramp.color_ramp.elements[0].color=tuple(v*.60 for v in color)+(1,);ramp.color_ramp.elements[1].color=tuple(min(1,v*1.2) for v in color)+(1,)
 links.new(tex.outputs['Fac'],ramp.inputs[0]);links.new(ramp.outputs['Color'],p.inputs['Base Color'])
 bump=nodes.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.24;bump.inputs['Distance'].default_value=.10;links.new(tex.outputs['Fac'],bump.inputs['Height']);links.new(bump.outputs['Normal'],p.inputs['Normal'])
 if key in ['AlienCore','AlienVein','RareVein']:
  p.inputs['Emission Color'].default_value=(*color,1);p.inputs['Emission Strength'].default_value=1.3 if key.startswith('Alien') else .3
 mats[key]=mat
rng=random.Random(61011)
def rock(name,pos,size,material,sub=2):
 bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=sub,radius=1,location=pos);o=bpy.context.object
 for v in o.data.vertices:v.co*=rng.uniform(.82,1.16)
 o.scale=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 # Keep a flat ground contact rather than a floating spheroid.
 for v in o.data.vertices:
  if v.co.z+pos[2]<1:v.co.z=1-pos[2]
 return finish(o,name,material)
def prism(name,pos,radius,height,material,tilt=(0,0)):
 vertices=[]
 for z,r in [(0,radius*.88),(height*.72,radius),(height*.83,radius*.86)]:
  vertices.extend([(r*math.cos(i*math.tau/6),r*math.sin(i*math.tau/6),z) for i in range(6)])
 vertices.append((radius*.10,-radius*.12,height));faces=[tuple(reversed(range(6)))]
 for ring in range(2):
  for i in range(6):faces.append((ring*6+i,ring*6+(i+1)%6,(ring+1)*6+(i+1)%6,(ring+1)*6+i))
 for i in range(6):faces.append((12+i,12+(i+1)%6,18))
 mesh=bpy.data.meshes.new(name);mesh.from_pydata(vertices,[],faces);mesh.update();o=bpy.data.objects.new(name,mesh);scene.collection.objects.link(o);o.location=pos;o.rotation_euler=(tilt[0],tilt[1],rng.uniform(0,math.tau));finish(o,name,material)
 bevel=o.modifiers.new('Worn crystal edges','BEVEL');bevel.width=.7;bevel.segments=1
 return o
def vein(points,radius,material):
 # Low-poly continuous tubes remain visible at the strategy camera distance.
 verts=[];faces=[]
 for i,p in enumerate(points):
  direction=Vector(points[min(i+1,len(points)-1)])-Vector(points[max(0,i-1)])
  tangent=direction.normalized();normal=tangent.cross(Vector((0,0,1)))
  if normal.length<.01:normal=Vector((1,0,0))
  normal.normalize();other=tangent.cross(normal)
  for j in range(6):verts.append(Vector(p)+radius*(normal*math.cos(j*math.tau/6)+other*math.sin(j*math.tau/6)))
 for i in range(len(points)-1):
  for j in range(6):faces.append((i*6+j,i*6+(j+1)%6,(i+1)*6+(j+1)%6,(i+1)*6+j))
 mesh=bpy.data.meshes.new('Mineral seam');mesh.from_pydata(verts,[],faces);mesh.update();o=bpy.data.objects.new('Mineral seam',mesh);scene.collection.objects.link(o);return finish(o,o.name,material)
def rubble(material,n=22):
 for i in range(n):
  a=rng.uniform(0,math.tau);r=rng.uniform(97,145);scale=rng.uniform(4,13)
  rock('Loose fragments',(r*math.cos(a),r*.78*math.sin(a),2),(scale,scale*.7,scale*.45),material,1)
deposits=[]
# Milky quartz points emerge from a pale, low host outcrop.
rock('Pale weathered host',(-6,0,20),(112,81,39),'PaleRock')
for x,y,z,r,h,tx,ty in [(-52,14,34,18,92,.14,-.18),(-12,27,44,23,132,-.08,.08),(32,4,35,21,105,.1,.22),(-26,-32,28,18,78,.28,-.14),(62,30,28,15,69,-.16,.30),(30,-39,21,13,65,.38,.10),(-78,-15,19,11,48,.18,-.32)]:
 prism('Milky quartz point',(x,y,z),r,h,'Quartz', (tx,ty))
for x,y in [(-62,-42),(62,-32),(-6,-59)]:prism('Smoky quartz shard',(x,y,9),9,31,'QuartzShadow',(.35,.2))
rubble('PaleRock');deposits.append(export('SM_SilicaDeposit'));deposits[-1].hide_render=True
# Mostly charcoal host with short purple crystals and readable exposed seams.
for pos,size in [((-38,10,28),(70,65,62)),((38,18,31),(68,59,78)),((0,-35,18),(91,46,42))]:rock('Dark mineral host',pos,size,'RareRock')
for x,y,z,r,h,tx,ty in [(-52,-15,54,14,36,.3,-.3),(-13,12,72,16,45,-.15,.20),(35,-11,76,13,38,.2,.1),(62,23,90,11,31,-.2,.1),(-20,-48,35,13,27,.35,-.18),(62,-39,30,10,24,.3,.3)]:prism('Embedded amethyst',(x,y,z),r,h,'Amethyst',(tx,ty))
for pts in [[(-80,-17,42),(-53,-39,53),(-15,-57,45),(15,-62,30)],[(19,-35,71),(42,-31,81),(65,-14,85)], [(-42,8,85),(-25,21,82),(2,26,75)]]:vein(pts,3.1,'RareVein')
for i in range(16):rock('Metal inclusions',(rng.uniform(-70,75),rng.uniform(-40,40),rng.uniform(31,70)),(3,3,2),'MetalFleck',1)
rubble('RareRock');deposits.append(export('SM_RareMineralsDeposit'));deposits[-1].hide_render=True
# Three organic fossil ribs curve around a recessed crystalline core.
rock('Obsidian bed',(0,0,10),(107,76,26),'Obsidian')
for i,(x,y) in enumerate([(-22,0),(12,-8),(34,20),(-8,25),(-34,-24)]):prism('Turquoise core',(x,y,25),14 if i==0 else 10,72 if i==0 else 45,'AlienCore',(.15,-.18))
def rib(name,origin,height,width,lean):
 points=[];count=13
 for i in range(count):
  t=i/(count-1);curve=(1-math.cos(t*math.pi*.7))/(1-math.cos(math.pi*.7));points.append(Vector((origin[0]+lean*curve,origin[1]+16*math.sin(t*math.pi),18+height*math.sin(t*math.pi*.62))))
 verts=[];faces=[]
 for i,p in enumerate(points):
  t=i/(count-1);r=width*(1-t*.91);tangent=(points[min(i+1,count-1)]-points[max(i-1,0)]).normalized();normal=Vector((0,1,0));other=tangent.cross(normal).normalized()
  for j in range(8):
   angle=j*math.tau/8;scallop=1+.16*math.sin(i*math.pi*.8)
   verts.append(p+normal*(r*.70*math.cos(angle))+other*(r*scallop*math.sin(angle)))
 for i in range(count-1):
  for j in range(8):faces.append((i*8+j,i*8+(j+1)%8,(i+1)*8+(j+1)%8,(i+1)*8+j))
 faces.extend([tuple(reversed(range(8))),tuple(range((count-1)*8,count*8))])
 mesh=bpy.data.meshes.new(name);mesh.from_pydata(verts,[],faces);mesh.update();o=bpy.data.objects.new(name,mesh);scene.collection.objects.link(o);finish(o,name,'AlienShell');o.data.materials.append(mats['Obsidian'])
 for face in o.data.polygons:
  if face.index//8%3==1:face.material_index=1
 # Cyan channels in the inner surface, separate geometry shares an animated UE instance.
 vein([tuple(p+Vector((0,-width*.57*(1-i/(count-1)*.91),0))) for i,p in enumerate(points[:-2])],1.7,'AlienVein')
for orig,h,w,lean in [((-68,22),155,23,50),((64,26),117,26,-49),((8,62),143,22,-22)]:rib('Layered fossil rib',orig,h,w,lean)
for i in range(9):
 a=i*math.tau/9;prism('Small bioluminescent pores',(77*math.cos(a),53*math.sin(a),18),4,9,'AlienVein',(.2,.1))
rubble('Obsidian');deposits.append(export('SM_AlienMaterialDeposit'));deposits[-1].hide_render=True
# One persistent workshop with all editable models; transparent catalogue renders.
scene.render.engine='CYCLES';scene.cycles.samples=24
bpy.ops.object.camera_add(location=(340,-420,350));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,63))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=350;cam.data.clip_end=10000;scene.camera=cam
for pos,energy in [((0,-300,600),12000000),((-300,160,400),6500000)]:
 bpy.ops.object.light_add(type='AREA',location=pos);lamp=bpy.context.object;lamp.data.energy=energy;lamp.data.size=400;lamp.rotation_euler=(Vector((0,0,50))-lamp.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('DepositStudio');scene.world.color=(.18,.18,.18)
scene.render.resolution_x=512;scene.render.resolution_y=512;scene.render.resolution_percentage=100;scene.render.film_transparent=True;scene.render.image_settings.file_format='PNG'
for key,obj in zip(['Silica','RareMinerals','AlienMaterial'],deposits):
 obj.hide_render=False;scene.render.filepath=str(out/('T_'+key+'.png'));bpy.ops.render.render(write_still=True);obj.hide_render=True
for i,obj in enumerate(deposits):obj.hide_render=False;obj.location.x=i*400
bpy.data.libraries.write(str(out/'SurfaceResources.blend'),{scene},fake_user=True)
result={'meshes':[o.name for o in deposits],'vertices':[len(o.data.vertices) for o in deposits],'source':str(out)}
