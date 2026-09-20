import unreal as u,os,json,time,math,traceback
ROOT=r'C:\UE5\SurviveThePlanet 5.8';OUT=ROOT+r'\Saved\Expedition07';os.makedirs(OUT,exist_ok=True)
ae=u.get_editor_subsystem(u.EditorActorSubsystem);world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert world.get_name()=='PlanetLevel_RockPreview'
backup='/Game/Environment/Expedition07/PlanetLevel_Before07'
if not u.EditorAssetLibrary.does_asset_exist(backup):u.EditorAssetLibrary.save_loaded_asset(u.EditorAssetLibrary.duplicate_asset('/Game/PlanetLevel_RockPreview',backup),False)
report={'lights_before':[],'lights_after':[],'moved_decorations':[],'camera':'same transforms/FOV before and after','base_clearance_cm':800}
def lightstate(a):return {'label':a.get_actor_label(),'intensity':a.light_component.intensity,'rotation':str(a.get_actor_rotation()),'color':str(a.light_component.light_color)}
lights=[a for a in ae.get_all_level_actors() if isinstance(a,u.DirectionalLight)]
report['lights_before']=[lightstate(a) for a in lights]
cap=ae.spawn_actor_from_class(u.SceneCapture2D,u.Vector(-2200,-3300,2300),u.Rotator(pitch=-34,yaw=56,roll=0));cc=cap.get_component_by_class(u.SceneCaptureComponent2D);cc.capture_every_frame=False;cc.capture_on_movement=False;cc.fov_angle=60
rt=u.RenderingLibrary.create_render_target2d(world,1600,1000,u.TextureRenderTargetFormat.RTF_RGBA8);rt.set_editor_property('target_gamma',2.2);cc.texture_target=rt;cc.capture_source=u.SceneCaptureSource.SCS_FINAL_COLOR_LDR
def capture(name):cc.capture_scene();u.RenderingLibrary.export_render_target(world,rt,OUT,name)
started=time.monotonic();phase=0
def tick07(dt):
 global phase
 try:
  t=time.monotonic()-started
  if t>15 and phase==0:
   capture('Overview_Before.png');cap.set_actor_location(u.Vector(-1550,-2800,1000),False,False);cap.set_actor_rotation(u.Rotator(pitch=-36,yaw=90,roll=0),False);cc.fov_angle=57;phase=1
  elif t>19 and phase==1:
   capture('Detail_Before.png')
   for a in lights:
    c=a.light_component
    if a.get_actor_label()=='E02_SoftFill':
     c.set_intensity(2.5);c.set_light_color(u.LinearColor(.82,.88,1,1));c.set_cast_shadows(False);c.set_editor_property('forward_shading_priority',0)
    else:
     c.set_intensity(3.456);c.set_light_color(u.LinearColor(1,.91,.80,1));a.set_actor_rotation(u.Rotator(pitch=-43,yaw=-35,roll=0),False);c.set_editor_property('forward_shading_priority',1);c.set_editor_property('light_source_angle',2.0)
   report['lights_after']=[lightstate(a) for a in lights]
   # Maintain a quiet base clearing; no new terrain or blocking objects.
   for a in ae.get_all_level_actors():
    if not (a.actor_has_tag('STP_Expedition05') or a.actor_has_tag('STP_Expedition06')):continue
    p=a.get_actor_location();dx=p.x-600;dy=p.y-150;d=math.hypot(dx,dy)
    if d<800:
     a.set_actor_location(u.Vector(600+dx/max(d,1)*850,150+dy/max(d,1)*850,p.z),False,False);report['moved_decorations'].append(a.get_actor_label())
   phase=2
  elif t>31 and phase==2:
   capture('Detail_After.png');cap.set_actor_location(u.Vector(-2200,-3300,2300),False,False);cap.set_actor_rotation(u.Rotator(pitch=-34,yaw=56,roll=0),False);cc.fov_angle=60;phase=3
  elif t>36 and phase==3:
   capture('Overview_After.png');ae.destroy_actor(cap)
   cams=[a for a in ae.get_all_level_actors() if a.get_actor_label()=='E07_CompositionCamera']
   cam=cams[0] if cams else ae.spawn_actor_from_class(u.CameraActor,u.Vector(-2200,-3300,2300),u.Rotator(pitch=-34,yaw=56,roll=0))
   cam.set_actor_label('E07_CompositionCamera');cam.set_folder_path('Environment/Review');cam.camera_component.set_field_of_view(60);cam.camera_component.set_editor_property('post_process_blend_weight',0)
   u.EditorLevelLibrary.set_level_viewport_camera_info(cam.get_actor_location(),cam.get_actor_rotation());u.EditorLoadingAndSavingUtils.save_map(world,'/Game/PlanetLevel_RockPreview');json.dump(report,open(OUT+'/review.json','w'),indent=2);u.unregister_slate_post_tick_callback(handle07);u.EditorPythonScripting.set_keep_python_script_alive(False)
 except Exception:
  open(OUT+'/error.txt','w').write(traceback.format_exc());u.unregister_slate_post_tick_callback(handle07);u.EditorPythonScripting.set_keep_python_script_alive(False)
handle07=u.register_slate_post_tick_callback(tick07);u.EditorPythonScripting.set_keep_python_script_alive(True)

