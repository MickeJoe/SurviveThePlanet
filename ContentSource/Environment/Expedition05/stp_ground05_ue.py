import unreal as u,os,json,math,random,time,traceback
ROOT=r'C:\UE5\SurviveThePlanet 5.8'
exec(open(ROOT+r'\ContentSource\Environment\Expedition02\stp_ue02_build.py').read().split('try:main()')[0])
DST='/Game/Environment/Expedition05';SRC=ROOT+r'\ContentSource\Environment\Expedition05';LOG=ROOT+r'\Saved\Expedition05';os.makedirs(LOG,exist_ok=True)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert world.get_name()=='PlanetLevel_RockPreview'
backup=DST+'/PlanetLevel_Before05'
if not u.EditorAssetLibrary.does_asset_exist(backup):save(u.EditorAssetLibrary.duplicate_asset('/Game/PlanetLevel_RockPreview',backup))
# Reuse the existing ground recipe, adding only two local transition masks.
source=open(ROOT+r'\ContentSource\Environment\Expedition03\stp_ue03_build.py').read()
recipe=source[source.index("mat=newmat('M_Terrain_03')"):source.index('mesh.set_material(0,mat)')]
recipe=recipe.replace('M_Terrain_03','M_Terrain_05')
detail='''float patch=0;float2 centers[2]={float2(-1550,-1650),float2(700,-2350)};
for(int k=0;k<2;k++){float2 q=p-centers[k];q.y*=1.22;float d=length(q)+(n.v(p/90)-.5)*150;patch=max(patch,1-smoothstep(250,610,d));}
float2 pp=p/38+float2(n.v(p/140),n.v(p/140+13))*1.4;
float2 ip=floor(pp),fp=frac(pp);float f1=20,f2=20;
for(int a=-1;a<=1;a++)for(int b=-1;b<=1;b++){float2 id=ip+float2(a,b);float2 j=float2(n.h(id),n.h(id+17));float dd=length(float2(a,b)+j-fp);if(dd<f1){f2=f1;f1=dd;}else f2=min(f2,dd);}
float crack=(1-smoothstep(.014,.045,f2-f1))*smoothstep(.42,.67,n.v(p/170))*patch;
float grit=step(.68,n.v(p/2.5))*patch;
float3 base=lerp(c,float3(.25,.225,.185)*(.8+rnd*.3),peb*.65);
float3 earth=lerp(float3(.105,.079,.052),float3(.18,.145,.098),n.v(p/30));
base=lerp(base,earth,patch*.72);base+=grit*.032;return lerp(base,float3(.046,.033,.022),crack*.78);'''
recipe=recipe.replace('return lerp(c,float3(.25,.225,.185)*(.8+rnd*.3),peb*.65);',detail)
exec(recipe)
surface=next(a for a in ae.get_all_level_actors() if 'SurfaceManager' in a.get_name())
chunks=surface.get_components_by_class(u.StaticMeshComponent);assert len(chunks)==80
terrainpath=DST+'/Meshes/SM_TerrainFlat_05'
terrain=asset(terrainpath) or u.EditorAssetLibrary.duplicate_asset('/Game/Environment/Expedition03/Meshes/SM_TerrainFlat_03',terrainpath)
terrain.set_material(0,mat);save(terrain);surface.set_editor_property('chunk_meshes',[terrain])
for c in chunks:c.set_static_mesh(terrain);c.set_material(0,mat)
rockmat=newmat('M_Pebbles_05');wp=node(rockmat,u.MaterialExpressionWorldPosition)
co=custom(rockmat,noise+'float v=n.v(p/13);return lerp(float3(.105,.085,.062),float3(.25,.21,.15),v);',{'P':(wp,'')});prop(co,'',u.MaterialProperty.MP_BASE_COLOR);prop(scalar(rockmat,'Roughness',.94),'',u.MaterialProperty.MP_ROUGHNESS);finishmat(rockmat)
meshes=[]
for name in ['SM_Pebbles05_Scatter','SM_Pebbles05_Shards']:
 mesh=importfile(SRC+'/'+name+'.fbx',name,'mesh');mesh.set_material(0,rockmat);save(mesh);meshes.append(mesh)
for a in ae.get_all_level_actors():
 if a.actor_has_tag('STP_Expedition05'):ae.destroy_actor(a)
count=0
for g,(cx,cy) in enumerate([(-1550,-1650),(700,-2350)]):
 rng=random.Random(505+g)
 for j in range(34):
  angle=rng.random()*math.tau;d=rng.uniform(160,490);x=cx+math.cos(angle)*d;y=cy+math.sin(angle)*d*.8
  ob=ae.spawn_actor_from_class(u.StaticMeshActor,u.Vector(x,y,56.8),u.Rotator(pitch=0,yaw=rng.uniform(0,360),roll=0));ob.static_mesh_component.set_static_mesh(meshes[j%2]);s=rng.uniform(.65,1.25);ob.set_actor_scale3d(u.Vector(s,s,s));ob.set_actor_enable_collision(False);ob.tags=['STP_Expedition05'];ob.set_actor_label('E05_Pebbles_%d_%02d'%(g,j));ob.set_folder_path('Expedition05/GroundTransitions');count+=1
u.EditorLoadingAndSavingUtils.save_map(world,'/Game/PlanetLevel_RockPreview')
json.dump({'pebble_actors':count,'ground_chunks':len(chunks),'local_masks':2,'collision':'decorative; no added blockers'},open(LOG+'/build.json','w'),indent=2)
cap=ae.spawn_actor_from_class(u.SceneCapture2D,u.Vector(-1550,-2800,1000),u.Rotator(pitch=-36,yaw=90,roll=0));cc=cap.get_component_by_class(u.SceneCaptureComponent2D);cc.capture_every_frame=False;cc.capture_on_movement=False;cc.fov_angle=57
rt=u.RenderingLibrary.create_render_target2d(world,1500,1000,u.TextureRenderTargetFormat.RTF_RGBA8);rt.set_editor_property('target_gamma',2.2);cc.texture_target=rt;cc.capture_source=u.SceneCaptureSource.SCS_FINAL_COLOR_LDR
started=time.monotonic();phase=0
def review05(dt):
 global phase
 if time.monotonic()-started>15 and phase==0:
  cc.capture_scene();u.RenderingLibrary.export_render_target(world,rt,LOG,'Transition_A.png');cap.set_actor_location(u.Vector(700,-3600,1000),False,False);phase=1
 elif time.monotonic()-started>23 and phase==1:
  cc.capture_scene();u.RenderingLibrary.export_render_target(world,rt,LOG,'Transition_B.png');ae.destroy_actor(cap);u.EditorLoadingAndSavingUtils.save_map(world,'/Game/PlanetLevel_RockPreview');u.unregister_slate_post_tick_callback(handle05);u.EditorPythonScripting.set_keep_python_script_alive(False);json.dump({'complete':True},open(LOG+'/review.json','w'))
handle05=u.register_slate_post_tick_callback(review05)
u.EditorPythonScripting.set_keep_python_script_alive(True)

