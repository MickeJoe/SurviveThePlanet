import bpy,bmesh,math,random,os,json
from mathutils import Vector,noise
OUT=r'C:\UE5\SurviveThePlanet 5.8\ContentSource\Environment\Expedition04'
os.makedirs(OUT,exist_ok=True)
old=bpy.context.scene
sc=bpy.data.scenes.new('STP_ModularRock04');bpy.context.window.scene=sc
sc.render.engine='CYCLES';sc.cycles.samples=16
sc.render.resolution_x=1400;sc.render.resolution_y=900;sc.render.resolution_percentage=100
sc.world=bpy.data.worlds.new('Rock04World');sc.world.use_nodes=True
sc.world.node_tree.nodes['Background'].inputs[0].default_value=(.13,.16,.20,1)
sc.world.node_tree.nodes['Background'].inputs[1].default_value=.45
mat=bpy.data.materials.new('M_Rock04_BakedSource');mat.use_nodes=True
n=mat.node_tree.nodes;l=mat.node_tree.links;bs=n.get('Principled BSDF');bs.inputs['Roughness'].default_value=.87
tex=n.new('ShaderNodeTexCoord');mapping=n.new('ShaderNodeVectorMath');mapping.operation='MULTIPLY';mapping.inputs[1].default_value=(2.0,2.0,4.5);l.new(tex.outputs['Object'],mapping.inputs[0])
ns=n.new('ShaderNodeTexNoise');ns.inputs['Scale'].default_value=2.8;ns.inputs['Detail'].default_value=5;ns.inputs['Roughness'].default_value=.72;l.new(mapping.outputs[0],ns.inputs['Vector'])
ramp=n.new('ShaderNodeValToRGB');ramp.color_ramp.elements[0].position=.2;ramp.color_ramp.elements[0].color=(.065,.052,.040,1);ramp.color_ramp.elements[1].position=.78;ramp.color_ramp.elements[1].color=(.33,.255,.175,1);l.new(ns.outputs['Fac'],ramp.inputs[0]);l.new(ramp.outputs[0],bs.inputs['Base Color'])
fine=n.new('ShaderNodeTexNoise');fine.inputs['Scale'].default_value=37;fine.inputs['Detail'].default_value=3;l.new(tex.outputs['Object'],fine.inputs[0])
bump=n.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.45;bump.inputs['Distance'].default_value=.045;l.new(fine.outputs['Fac'],bump.inputs['Height']);l.new(bump.outputs[0],bs.inputs['Normal'])
def block(name,dimensions,seed):
 r=random.Random(seed);bm=bmesh.new();bmesh.ops.create_cube(bm,size=2)
 # Convex fracture planes produce broad unequal faces, not repeated cuboids.
 for i in range(15):
  normal=Vector((r.uniform(-1,1),r.uniform(-1,1),r.uniform(-.8,.8))).normalized()
  d=r.uniform(.96,1.28)
  geom=list(bm.verts)+list(bm.edges)+list(bm.faces)
  res=bmesh.ops.bisect_plane(bm,geom=geom,dist=.00001,plane_co=normal*d,plane_no=normal,clear_outer=True,clear_inner=False)
  boundary=[e for e in res['geom_cut'] if isinstance(e,bmesh.types.BMEdge) and e.is_boundary]
  if boundary:bmesh.ops.holes_fill(bm,edges=boundary,sides=0)
 bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
 mesh=bpy.data.meshes.new(name);bm.to_mesh(mesh);bm.free()
 ob=bpy.data.objects.new(name,mesh);sc.collection.objects.link(ob)
 for v in mesh.vertices:
  v.co.x*=dimensions[0]/2;v.co.y*=dimensions[1]/2;v.co.z*=dimensions[2]/2
 bpy.context.view_layer.objects.active=ob;ob.select_set(True)
 bevel=ob.modifiers.new('Weathered fracture edges','BEVEL');bevel.width=.065;bevel.segments=2
 bpy.ops.object.modifier_apply(modifier=bevel.name)
 bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=1.15,island_margin=.015);bpy.ops.object.mode_set(mode='OBJECT')
 # Dense surface permits chips while preserving the broad planes.
 sub=ob.modifiers.new('Surface support','SUBSURF');sub.subdivision_type='SIMPLE';sub.levels=3;bpy.ops.object.modifier_apply(modifier=sub.name)
 for v in ob.data.vertices:
  p=v.co.copy();q=p+Vector((seed,3,7))
  v.co+=v.normal*(noise.noise(q*2.4)*.075+noise.noise(q*13)*.018)
  # Slight geological bedding offset, shared across a coherent rock family.
  v.co.x+=.035*math.sin(p.z*15+noise.noise(q)*2)
 zmin=min(v.co.z for v in ob.data.vertices)
 for v in ob.data.vertices:v.co.z-=zmin
 tri=ob.modifiers.new('Stable export triangles','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tri.name)
 ob.data.materials.append(mat)
 for typ,suffix in [('DIFFUSE','BaseColor'),('NORMAL','Normal')]:
  im=bpy.data.images.new(name+'_'+suffix,width=1024,height=1024)
  if typ=='NORMAL':im.colorspace_settings.name='Non-Color'
  target=n.new('ShaderNodeTexImage');target.image=im;n.active=target
  bpy.ops.object.bake(type=typ,pass_filter={'COLOR'} if typ=='DIFFUSE' else {'DIRECT','INDIRECT','COLOR'},margin=8)
  im.filepath_raw=OUT+'/T_'+name+'_'+suffix+'.png';im.file_format='PNG';im.save();n.remove(target)
 bpy.ops.export_scene.fbx(filepath=OUT+'/'+name+'.fbx',use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
 ob.select_set(False);return ob
modules=[]
for name,dim,seed in [('SM_Rock04_Pillar',(1.45,1.25,3.5),71),('SM_Rock04_Slab',(2.4,1.05,1.8),83),('SM_Rock04_Wedge',(1.7,1.5,1.35),97),('SM_Rock04_Foot',(1.0,.85,.7),112)]:
 ob=block(name,dim,seed);modules.append(ob)
# Two reusable assemblies shown together; modules remain separate objects.
layouts=[(-4,[(0,0,0,1,0),(1.05,.25,-.12,.85,25),(-.8,.3,-.15,.65,-20),(.3,-.9,0,.8,65),(-.75,-.9,0,.65,0)]),(3,[(0,0,0,1,70),(1.6,.1,0,.85,45),(-1.4,.2,0,.85,-15),(.6,-.9,0,.8,10),(-1,-.8,0,.7,40)])]
for idx,ob in enumerate(modules):ob.hide_render=True;ob.location=(0,8+idx*3,0)
for group,(x,items) in enumerate(layouts):
 for j,(dx,dy,z,s,yaw) in enumerate(items):
  source=modules[0 if group==0 and j<3 else 1 if j<3 else 2]
  ob=bpy.data.objects.new('Assembly%d_%d'%(group,j),source.data);sc.collection.objects.link(ob);ob.location=(x+dx,dy,z);ob.scale=(s,s,s);ob.rotation_euler[2]=math.radians(yaw)
 for j in range(9):
  r=random.Random(group*100+j);ob=bpy.data.objects.new('Foot_%d_%d'%(group,j),modules[3].data);sc.collection.objects.link(ob);ob.location=(x+r.uniform(-2.2,2.2),r.uniform(-1.6,1.2),-.04);s=r.uniform(.3,.8);ob.scale=(s,s,s);ob.rotation_euler[2]=r.random()*6.28
bpy.ops.mesh.primitive_plane_add(size=200);floor=bpy.context.object
fm=bpy.data.materials.new('StudioFloor');fm.diffuse_color=(.055,.065,.075,1);floor.data.materials.append(fm);floor.location.z=-.08
ld=bpy.data.lights.new('Key','AREA');lo=bpy.data.objects.new('Key',ld);sc.collection.objects.link(lo);lo.location=(-4,-6,10);ld.energy=2200;ld.shape='DISK';ld.size=7;lo.rotation_euler=(Vector((0,0,1))-lo.location).to_track_quat('-Z','Y').to_euler()
cam=bpy.data.cameras.new('ReviewCamera');co=bpy.data.objects.new('ReviewCamera',cam);sc.collection.objects.link(co);co.location=(10,-17,12);co.rotation_euler=(Vector((0,0,1.2))-co.location).to_track_quat('-Z','Y').to_euler();cam.type='ORTHO';cam.ortho_scale=15;sc.camera=co
sc.render.filepath=OUT+'/ModularRock04_Studio.png'
bpy.ops.wm.save_as_mainfile(filepath=OUT+'/STP_ModularRock04.blend',copy=True)
bpy.ops.render.render(write_still=True)
with open(OUT+'/modules.json','w') as f:json.dump({o.name:len(o.data.polygons) for o in modules},f,indent=2)
print('ROCK04_COMPLETE '+OUT)

