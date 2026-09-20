import unreal as u,os,json,time,traceback,math,random
ROOT=r'C:\UE5\SurviveThePlanet 5.8'
exec(open(ROOT+r'\ContentSource\Environment\Expedition02\stp_ue02_build.py').read().split('try:main()')[0])
DST='/Game/Environment/Expedition04';SRC=ROOT+r'\ContentSource\Environment\Expedition04';LOG=ROOT+r'\Saved\Expedition04';os.makedirs(LOG,exist_ok=True)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert world.get_name()=='PlanetLevel_RockPreview'
try:
 backup=DST+'/PlanetLevel_Before04'
 if not u.EditorAssetLibrary.does_asset_exist(backup):
  b=u.EditorAssetLibrary.duplicate_asset('/Game/PlanetLevel_RockPreview',backup);assert b;save(b)
 names=['SM_Rock04_Pillar','SM_Rock04_Slab','SM_Rock04_Wedge','SM_Rock04_Foot'];meshes=[]
 for name in names:
  mesh=importfile(SRC+'/'+name+'.fbx',name,'mesh');textures={}
  for suffix in ['BaseColor','Normal']:
   t=importfile(SRC+'/T_'+name+'_'+suffix+'.png','T_'+name+'_'+suffix,'texture')
   if suffix=='Normal':t.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP);t.set_editor_property('srgb',False);t.set_editor_property('flip_green_channel',True)
   save(t);textures[suffix]=t
  mesh.set_material(0,rock_material(name,textures));u.get_editor_subsystem(u.StaticMeshEditorSubsystem).set_convex_decomposition_collisions(mesh,1,16,30000);save(mesh);meshes.append(mesh)
 for a in ae.get_all_level_actors():
  if a.actor_has_tag('STP_Expedition04') or a.get_actor_label() in ['E02_Cliff_00','E02_Cliff_01']:ae.destroy_actor(a)
 placements=[]
 layouts=[(-1550,-1650,-35,[(0,0,0,0,1.05,0),(0,105,25,-12,.85,25),(0,-80,30,-15,.65,-20),(2,30,-90,0,.8,65),(2,-75,-90,0,.65,0)]),(700,-2350,12,[(1,0,0,0,1.3,70),(1,160,10,0,1.05,45),(1,-140,20,0,1.1,-15),(2,60,-90,0,.8,10),(2,-100,-80,0,.7,40)])]
 for g,(cx,cy,angle,items) in enumerate(layouts):
  rng=random.Random(800+g)
  for k in range(12):items.append((3,rng.uniform(-215,215),rng.uniform(-160,130),-5,rng.uniform(.35,.8),rng.uniform(0,360)))
  for j,(mi,dx,dy,dz,s,yaw) in enumerate(items):
   a=math.radians(angle);x=cx+dx*math.cos(a)-dy*math.sin(a);y=cy+dx*math.sin(a)+dy*math.cos(a)
   ob=ae.spawn_actor_from_class(u.StaticMeshActor,u.Vector(x,y,50+dz),u.Rotator(pitch=0,yaw=angle+yaw,roll=0));ob.static_mesh_component.set_static_mesh(meshes[mi]);ob.set_actor_scale3d(u.Vector(s,s,s));ob.set_actor_label('E04_Formation_%d_Module_%02d'%(g,j));ob.set_folder_path('Expedition04/Formation_%d'%g);ob.tags=['STP_Expedition04']
   if mi==3:ob.set_actor_enable_collision(False)
   placements.append({'actor':ob.get_actor_label(),'mesh':names[mi],'position':[x,y,50+dz]})
 u.EditorLoadingAndSavingUtils.save_map(world,'/Game/PlanetLevel_RockPreview')
 with open(LOG+'/build.json','w') as f:json.dump({'modules':names,'placements':placements,'backup':backup},f,indent=2)
 # Review capture actor is temporary and never saved in the map.
 capture=ae.spawn_actor_from_class(u.SceneCapture2D,u.Vector(-1550,-2800,1000),u.Rotator(pitch=-36,yaw=90,roll=0))
 cc=capture.get_component_by_class(u.SceneCaptureComponent2D);cc.capture_every_frame=False;cc.capture_on_movement=False;cc.fov_angle=57
 rt=u.RenderingLibrary.create_render_target2d(world,1500,1000,u.TextureRenderTargetFormat.RTF_RGBA8);rt.set_editor_property('target_gamma',2.2);cc.texture_target=rt;cc.capture_source=u.SceneCaptureSource.SCS_FINAL_COLOR_LDR
 u.EditorLevelLibrary.set_level_viewport_camera_info(capture.get_actor_location(),capture.get_actor_rotation())
 start=time.monotonic();state=0
 def tick(dt):
  global state
  try:
   t=time.monotonic()-start
   if state==0 and t>20:
    cc.capture_scene();u.RenderingLibrary.export_render_target(world,rt,LOG,'Formation_A.png');capture.set_actor_location(u.Vector(700,-3600,1000),False,False);state=1
   elif state==1 and t>30:
    cc.capture_scene();u.RenderingLibrary.export_render_target(world,rt,LOG,'Formation_B.png');ae.destroy_actor(capture)
    camera=next(a for a in ae.get_all_level_actors() if a.get_actor_label()=='E02_OverviewCamera');u.EditorLevelLibrary.set_level_viewport_camera_info(camera.get_actor_location(),camera.get_actor_rotation());u.unregister_slate_post_tick_callback(handle)
    with open(LOG+'/review_ready.json','w') as f:json.dump({'captures':2,'saved':True},f)
  except Exception:
   open(LOG+'/error.txt','w').write(traceback.format_exc());u.unregister_slate_post_tick_callback(handle)
 handle=u.register_slate_post_tick_callback(tick);u.EditorPythonScripting.set_keep_python_script_alive(True)
except Exception:open(LOG+'/error.txt','w').write(traceback.format_exc())

