import unreal as u,os,json,math,random,time
ROOT=r'C:\UE5\SurviveThePlanet 5.8'
exec(open(ROOT+r'\ContentSource\Environment\Expedition02\stp_ue02_build.py').read().split('try:main()')[0])
DST='/Game/Environment/Expedition06';SRC=ROOT+r'\ContentSource\Environment\Expedition06';LOG=ROOT+r'\Saved\Expedition06';os.makedirs(LOG,exist_ok=True)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert world.get_name()=='PlanetLevel_RockPreview'
backup=DST+'/PlanetLevel_Before06'
if not u.EditorAssetLibrary.does_asset_exist(backup):save(u.EditorAssetLibrary.duplicate_asset('/Game/PlanetLevel_RockPreview',backup))
palette={'Stem':(.047,.059,.022),'Leaves':(.115,.145,.035),'Cap':(.28,.072,.018),'Gills':(.095,.063,.032),'Coral':(.20,.026,.010),'Olive':(.095,.12,.025),'Gold':(.22,.14,.025),'Tube':(.27,.075,.020),'Inside':(.045,.026,.015),'Ember':(.36,.07,.015),'DarkStem':(.045,.035,.018)}
cache={}
def detailed(kind):
 if kind in cache:return cache[kind]
 mat=plant_material('M_Plant06_'+kind,palette[kind]);pos=node(mat,u.MaterialExpressionWorldPosition);vert=node(mat,u.MaterialExpressionVertexColor);col=vector(mat,'Pigment',palette[kind])
 code='''float h=saturate(W.r);float mott=.9+.1*sin(P.x*.11+sin(P.y*.15)*2)*sin(P.z*.17);float fleck=pow(saturate(sin(P.x*1.6)*sin(P.y*1.9)*sin(P.z*1.3)),9);return C.rgb*lerp(.62,1.17,h)*mott+fleck*float3(.032,.018,.007);'''
 tint=custom(mat,code,{'P':(pos,''),'W':(vert,''),'C':(col,'')});prop(tint,'',u.MaterialProperty.MP_BASE_COLOR);finishmat(mat);cache[kind]=mat;return mat
new=[]
for name in ['SM_Alien06_LowShrub','SM_Alien06_DiscFungus']:
 mesh=importfile(SRC+'/'+name+'.fbx',name,'mesh')
 for i,slot in enumerate(mesh.static_materials):
  text=str(slot.material_slot_name);kind=next((k for k in ['Leaves','Cap','Gills','Stem'] if k in text),'Stem');mesh.set_material(i,detailed(kind))
 save(mesh);new.append(mesh)
centers=[(-1550,-1650),(700,-2350)];improved=0
for a in ae.get_all_level_actors():
 if a.actor_has_tag('STP_Expedition06'):ae.destroy_actor(a);continue
 if not a.get_actor_label().startswith('E02_Plant_'):continue
 p=a.get_actor_location()
 if min(math.hypot(p.x-x,p.y-y) for x,y in centers)>650:continue
 comp=a.static_mesh_component
 for i in range(comp.get_num_materials()):
  old=comp.get_material(i);name=old.get_name() if old else ''
  kind=next((k for k in ['DarkStem','Ember','Coral','Olive','Gold','Tube','Inside'] if k in name),'Olive');comp.set_material(i,detailed(kind))
 improved+=1
for g,(cx,cy) in enumerate(centers):
 rng=random.Random(606+g)
 pockets=[(-290,-175),(290,-140),(-210,190),(235,165)]
 for j in range(32):
  px,py=pockets[j%4];x=cx+px+rng.gauss(0,48);y=cy+py+rng.gauss(0,48)
  mesh=new[1 if j%5==0 else 0];s=rng.uniform(.65,1.10) if j%5 else rng.uniform(.7,1.0)
  a=ae.spawn_actor_from_class(u.StaticMeshActor,u.Vector(x,y,56),u.Rotator(pitch=0,yaw=rng.uniform(0,360),roll=0));a.static_mesh_component.set_static_mesh(mesh);a.set_actor_scale3d(u.Vector(s,s,s));a.set_actor_enable_collision(False);a.static_mesh_component.set_editor_property('bounds_scale',1.4);a.tags=['STP_Expedition06'];a.set_actor_label('E06_Vegetation_%d_%02d'%(g,j));a.set_folder_path('Expedition06/Vegetation')
u.EditorLoadingAndSavingUtils.save_map(world,'/Game/PlanetLevel_RockPreview')
json.dump({'added':64,'existing_materials_improved_actors':improved,'opening_blooms':'untouched','wind':'same vertex-weighted WPO, retained and applied to both new meshes','collision':'none'},open(LOG+'/build.json','w'),indent=2)
cap=ae.spawn_actor_from_class(u.SceneCapture2D,u.Vector(-1550,-2800,1000),u.Rotator(pitch=-36,yaw=90,roll=0));cc=cap.get_component_by_class(u.SceneCaptureComponent2D);cc.capture_every_frame=False;cc.capture_on_movement=False;cc.fov_angle=57
rt=u.RenderingLibrary.create_render_target2d(world,1500,1000,u.TextureRenderTargetFormat.RTF_RGBA8);rt.set_editor_property('target_gamma',2.2);cc.texture_target=rt;cc.capture_source=u.SceneCaptureSource.SCS_FINAL_COLOR_LDR
started=time.monotonic();phase=0
def review06(dt):
 global phase
 t=time.monotonic()-started
 if t>15 and phase==0:
  cc.capture_scene();u.RenderingLibrary.export_render_target(world,rt,LOG,'Vegetation_A.png');phase=1
 elif t>18 and phase==1:
  cc.capture_scene();u.RenderingLibrary.export_render_target(world,rt,LOG,'Vegetation_A_WindLater.png');cap.set_actor_location(u.Vector(700,-3600,1000),False,False);phase=2
 elif t>25 and phase==2:
  cc.capture_scene();u.RenderingLibrary.export_render_target(world,rt,LOG,'Vegetation_B.png');ae.destroy_actor(cap);u.EditorLoadingAndSavingUtils.save_map(world,'/Game/PlanetLevel_RockPreview');json.dump({'complete':True},open(LOG+'/review.json','w'));u.unregister_slate_post_tick_callback(handle06);u.EditorPythonScripting.set_keep_python_script_alive(False)
handle06=u.register_slate_post_tick_callback(review06);u.EditorPythonScripting.set_keep_python_script_alive(True)
