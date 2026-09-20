import unreal as u, time, os, json, traceback
OUT=r'C:\UE5\SurviveThePlanet 5.8\Saved\Expedition02'
ae=u.get_editor_subsystem(u.EditorActorSubsystem)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
capture=ae.spawn_actor_from_class(u.SceneCapture2D,u.Vector(0,0,0))
cc=capture.get_component_by_class(u.SceneCaptureComponent2D)
cc.set_editor_property('capture_every_frame',False)
cc.set_editor_property('capture_on_movement',False)
rt=u.RenderingLibrary.create_render_target2d(world,1600,1000,u.TextureRenderTargetFormat.RTF_RGBA8)
rt.set_editor_property('target_gamma',2.2)
cc.set_editor_property('texture_target',rt)
cc.set_editor_property('capture_source',u.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
cc.set_editor_property('fov_angle',57)
def screenshot(name):
    capture.set_actor_location(cam.get_actor_location(),False,False)
    capture.set_actor_rotation(cam.get_actor_rotation(),False)
    cc.capture_scene()
    u.RenderingLibrary.export_render_target(world,rt,OUT,name)
u.SystemLibrary.execute_console_command(world,'r.TextureStreaming 0')
u.SystemLibrary.execute_console_command(world,'r.Shadow.Virtual.Cache 0')
cams=[a for a in ae.get_all_level_actors() if a.get_actor_label()=='E02_OverviewCamera']
if cams:
    cam=cams[0];u.EditorLevelLibrary.set_level_viewport_camera_info(cam.get_actor_location(),cam.get_actor_rotation())
else:cam=None
debug={'camera':str(cam.get_actor_transform()) if cam else None,'forward':str(cam.get_actor_forward_vector()) if cam else None,'actors':[]}
for a in ae.get_all_level_actors():
    if isinstance(a,u.PostProcessVolume):
        settings=a.get_editor_property('settings')
        debug['postprocess']=str(settings.get_editor_property('weighted_blendables'))
        settings.set_editor_property('weighted_blendables',u.WeightedBlendables())
        a.set_editor_property('settings',settings)
    if a.actor_has_tag('STP_Expedition02') and len(debug['actors'])<4:
        debug['actors'].append({'label':a.get_actor_label(),'transform':str(a.get_actor_transform()),'bounds':str(a.get_actor_bounds(False))})
with open(OUT+'/capture_debug.json','w') as f:json.dump(debug,f,indent=2)
start=time.monotonic();state=0
def tick(dt):
    global state
    t=time.monotonic()-start
    try:
        if state==0 and t>20:
            u.log('E02 capturing overview')
            screenshot('Overview.png')
            u.AutomationLibrary.take_high_res_screenshot(1600,1000,OUT+'/Viewport_Overview.png',camera=cam,delay=2)
            state=1
        elif state==1 and t>38:
            if cam:
                cam.set_actor_location(u.Vector(-1600,-2680,650),False,False)
                cam.set_actor_rotation(u.Rotator(pitch=-35,yaw=50,roll=0),False)
                u.EditorLevelLibrary.set_level_viewport_camera_info(cam.get_actor_location(),cam.get_actor_rotation())
            screenshot('Vegetation_Detail.png')
            state=2
        elif state==2 and t>56:
            screenshot('Vegetation_Detail_Later.png')
            state=3
        elif state==3 and t>72:
            actors=ae.get_all_level_actors();rocks=[a for a in actors if a.actor_has_tag('STP_Expedition02')]
            data={'actor_count':len(rocks),'overview_exists':os.path.exists(OUT+'/Overview.png'),'detail_exists':os.path.exists(OUT+'/Vegetation_Detail.png')}
            with open(OUT+'/review.json','w') as f:json.dump(data,f,indent=2)
            u.unregister_slate_post_tick_callback(handle);u.SystemLibrary.quit_editor()
    except Exception:
        with open(OUT+'/review_error.txt','w') as f:f.write(traceback.format_exc())
        u.unregister_slate_post_tick_callback(handle);u.SystemLibrary.quit_editor()
handle=u.register_slate_post_tick_callback(tick)
u.EditorPythonScripting.set_keep_python_script_alive(True)
