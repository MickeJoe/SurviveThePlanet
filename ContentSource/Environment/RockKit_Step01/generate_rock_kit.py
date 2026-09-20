import bpy, math, random, os, json
from mathutils import Vector
out=r'C:\UE5\SurviveThePlanet 5.8\ContentSource\Environment\RockKit_Step01'
os.makedirs(out,exist_ok=True)
scene=bpy.data.scenes.new('STP_RockKit_Step01')
bpy.context.window.scene=scene
scene.unit_settings.system='METRIC'
scene.unit_settings.scale_length=1.0
rng=random.Random(92026)
mats=[]
for i,c in enumerate([(0.115,0.092,0.073,1),(0.18,0.145,0.109,1),(0.24,0.19,0.135,1),(0.095,0.089,0.081,1)]):
 m=bpy.data.materials.new('STP_Rock_Earth_'+str(i)); m.diffuse_color=c; m.use_nodes=True
 n=m.node_tree.nodes; l=m.node_tree.links; bs=n.get('Principled BSDF'); bs.inputs['Base Color'].default_value=c; bs.inputs['Roughness'].default_value=.88
 noise=n.new('ShaderNodeTexNoise'); noise.inputs['Scale'].default_value=38; noise.inputs['Detail'].default_value=3
 bump=n.new('ShaderNodeBump'); bump.inputs['Strength'].default_value=.24; bump.inputs['Distance'].default_value=.035
 l.new(noise.outputs['Fac'],bump.inputs['Height']); l.new(bump.outputs['Normal'],bs.inputs['Normal']); mats.append(m)
def chunk(center,radius,height,seed):
 r=random.Random(seed); sides=r.choice([5,6,7]); angles=[2*math.pi*i/sides+r.uniform(-.12,.12) for i in range(sides)]; radii=[radius*r.uniform(.78,1.12) for i in angles]; verts=[]
 lean=(r.uniform(-.2,.2)*radius,r.uniform(-.2,.2)*radius)
 for level,(z,scale) in enumerate([(0,.82),(.12,1),(.76,.92),(1,.63)]):
  for a,rad in zip(angles,radii):
   verts.append((center[0]+math.cos(a)*rad*scale+lean[0]*z,center[1]+math.sin(a)*rad*scale+lean[1]*z,center[2]+height*z+(r.uniform(-.075,.075)*height if level==3 else 0)))
 faces=[tuple(reversed(range(sides))),tuple(range(3*sides,4*sides))]
 for j in range(3):
  for k in range(sides): faces.append((j*sides+k,j*sides+(k+1)%sides,(j+1)*sides+(k+1)%sides,(j+1)*sides+k))
 mesh=bpy.data.meshes.new('FracturedStone');mesh.from_pydata(verts,[],faces);mesh.update();ob=bpy.data.objects.new('StonePart',mesh);scene.collection.objects.link(ob)
 for m in mats:mesh.materials.append(m)
 for p in mesh.polygons:p.material_index=r.choices(range(4),[5,3,1,2])[0]
 return ob
specs=[('SM_Rock_Large_01',(-5,0,0),[(0,0,.85,3.9),(-.95,.1,.66,2.45),(.83,.16,.56,2.8),(.2,-.7,.5,1.5)]),('SM_Rock_Ridge_01',(1,0,0),[(-1.6,0,.7,1.45),(-.55,.1,.85,2.15),(.65,.1,.77,1.9),(1.65,.05,.62,1.3)]),('SM_Rock_Medium_01',(6,0,0),[(0,0,.86,1.35),(.7,.15,.43,.7)])]
assets=[];manifest=[]
for name,display,forms in specs:
 parts=[]
 for x,y,rad,h in forms:
  cut=h*.49
  parts.append(chunk((x,y,0),rad,cut,rng.randrange(999999)))
  parts.append(chunk((x+.025,y+.025,cut+.018),rad*.91,h-cut,rng.randrange(999999)))
 for k in range(12 if 'Medium' not in name else 5):
  a=rng.uniform(0,math.tau); dist=rng.uniform(1.05,1.85) if 'Ridge' not in name else rng.uniform(1.1,2.3)
  parts.append(chunk((math.cos(a)*dist,math.sin(a)*dist*.64,0),rng.uniform(.12,.3),rng.uniform(.15,.4),rng.randrange(999999)))
 bpy.ops.object.select_all(action='DESELECT')
 for ob in parts:ob.select_set(True)
 bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();ob=parts[0];ob.name=name
 bevel=ob.modifiers.new('Worn edges','BEVEL');bevel.width=.022;bevel.segments=2
 bpy.ops.object.modifier_apply(modifier=bevel.name)
 tri=ob.modifiers.new('Export triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tri.name)
 ob['purpose']='Static environment rock; collision and terrain blocking to configure in UE.'
 bpy.ops.export_scene.fbx(filepath=os.path.join(out,name+'.fbx'),use_selection=True,object_types={'MESH'},apply_unit_scale=True,add_leaf_bones=False,axis_forward='-Y',axis_up='Z',bake_anim=False)
 manifest.append({'name':name,'dimensions_m':list(ob.dimensions),'triangles':len(ob.data.polygons),'pivot':'ground origin','fbx':name+'.fbx'})
 ob.location=display;assets.append(ob)
# A separate staging floor is not included in the FBX exports.
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.06));floor=bpy.context.object;floor.name='PreviewFloor'
m=bpy.data.materials.new('Preview charcoal');m.diffuse_color=(.045,.052,.055,1);floor.data.materials.append(m)
world=bpy.data.worlds.new('RockKit Studio');world.use_nodes=True;world.node_tree.nodes['Background'].inputs[0].default_value=(.18,.21,.25,1);world.node_tree.nodes['Background'].inputs[1].default_value=.45;scene.world=world
for loc,energy,size,col in [((-3,-6,11),2200,7,(1,.79,.59)),((5,4,8),1900,6,(.68,.8,1))]:
 data=bpy.data.lights.new('Studio Area','AREA');data.energy=energy;data.shape='DISK';data.size=size;data.color=col;o=bpy.data.objects.new('Studio Area',data);scene.collection.objects.link(o);o.location=loc;o.rotation_euler=(Vector((0,0,1))-o.location).to_track_quat('-Z','Y').to_euler()
data=bpy.data.cameras.new('PreviewCamera');cam=bpy.data.objects.new('PreviewCamera',data);scene.collection.objects.link(cam);cam.location=(10,-21,14);cam.rotation_euler=(Vector((.4,0,1.25))-cam.location).to_track_quat('-Z','Y').to_euler();data.type='ORTHO';data.ortho_scale=17;scene.camera=cam
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True;scene.render.resolution_x=1500;scene.render.resolution_y=850;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.render.filepath=os.path.join(out,'RockKit_Step01_Preview.png')
with open(os.path.join(out,'manifest.json'),'w') as f:json.dump(manifest,f,indent=2)
with open(os.path.join(out,'CONTINUE.md'),'w') as f:f.write('STP environment kit - step 01\nCreated three procedural rock form prototypes via official Blender Labs MCP. Seed 92026. Units meters. Each FBX exported separately at origin with a ground pivot. Blender file contains display offsets and a studio floor.\nMaterials: flat earth colors with Blender procedural bump; bump is NOT baked and does not transfer to FBX. UE material setup, UV/texturing, collision, import, performance and in-game visual validation remain pending. No gameplay or WBP changes.\nNext: review silhouettes in game camera, refine fractured surfaces, prepare shared UE rock material and collision. Then 3 small rocks and 4 wind-ready alien plants. Building spacing deferred by user.\n')
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out,'STP_RockKit_Step01.blend'),copy=True)
bpy.ops.render.render(write_still=True)
print(json.dumps({'output':out,'assets':manifest}))
