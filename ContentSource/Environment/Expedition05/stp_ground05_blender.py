import bpy,random,math,os,json
OUT=r'C:\UE5\SurviveThePlanet 5.8\ContentSource\Environment\Expedition05';os.makedirs(OUT,exist_ok=True)
sc=bpy.data.scenes.new('STP_GroundTransition05');bpy.context.window.scene=sc
manifest=[]
for name,count,radius,seed in [('SM_Pebbles05_Scatter',24,1.2,512),('SM_Pebbles05_Shards',14,.85,713)]:
 rng=random.Random(seed);parts=[]
 for i in range(count):
  a=rng.random()*math.tau;d=radius*math.sqrt(rng.random());size=rng.uniform(.035,.14) if seed==512 else rng.uniform(.09,.23)
  bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1,radius=1,location=(math.cos(a)*d,math.sin(a)*d,0))
  ob=bpy.context.object
  for v in ob.data.vertices:
   v.co.x*=size*rng.uniform(.8,1.25);v.co.y*=size*(.55 if seed==713 else .9);v.co.z*=size*.45
  ob.location.z=max(v.co.z for v in ob.data.vertices)*.65
  ob.rotation_euler[2]=rng.random()*math.tau;parts.append(ob)
 bpy.ops.object.select_all(action='DESELECT')
 for ob in parts:ob.select_set(True)
 bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();ob=bpy.context.object;ob.name=name
 sc.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR');bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
 bpy.ops.export_scene.fbx(filepath=OUT+'/'+name+'.fbx',use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False)
 manifest.append({'mesh':name,'triangles':len(ob.data.polygons)});ob.select_set(False)
bpy.ops.wm.save_as_mainfile(filepath=OUT+'/STP_GroundTransition05.blend',copy=True)
json.dump(manifest,open(OUT+'/manifest.json','w'),indent=2)
print(manifest)
