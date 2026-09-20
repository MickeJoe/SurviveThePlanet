import unreal as u,json,os,time,math,traceback
ROOT=r'C:\UE5\SurviveThePlanet 5.8';OUT=ROOT+r'\Saved\Expedition08'
os.makedirs(OUT,exist_ok=True)
ae=u.get_editor_subsystem(u.EditorActorSubsystem)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert world.get_name()=='PlanetLevel_RockPreview'
def mesh(name):
    ob=u.load_asset('/Game/Environment/Expedition08/Library/'+name)
    assert ob
    return ob
surface=next(a for a in ae.get_all_level_actors() if 'SurfaceManager' in a.get_name())
large=[]
for label,name in [('E02_Cliff_04','SM_Cliff08_Corner'),('E02_Cliff_06','SM_Cliff08_End')]:
    a=next(a for a in ae.get_all_level_actors() if a.get_actor_label()==label)
    a.static_mesh_component.set_static_mesh(mesh(name));large.append(a)
placements=[('SM_Cliff08_Arch',-2100,-2800,1.05,0,50),('SM_Organic08_DeadTree',-1850,-1850,.65,25,53),('SM_Organic08_DeadTree',1040,-2550,.65,110,53),('SM_Organic08_DeadLog',-1200,-1870,.8,45,53),('SM_Organic08_DeadLog',1010,-2090,.8,10,53),('SM_Organic08_Roots',-1710,-1470,.9,90,53),('SM_Organic08_Roots',-1370,-1750,.9,160,53),('SM_Organic08_Roots',850,-2470,.9,210,53),('SM_Organic08_Roots',500,-2110,.9,30,53),('SM_Organic08_RibBones',-1150,-2340,.95,25,53),('SM_Atmos08_WreckFrame',-2380,-1130,1.2,65,53),('SM_Atmos08_Vent',1130,-2810,1.1,0,53),('SM_Atmos08_CraterRim',-650,-2590,.9,0,55),('SM_Mineral08_CrystalCluster',-1890,-1410,.9,32,53)]
created=[]
for i,(name,x,y,s,yaw,z) in enumerate(placements):
    label='E08_'+name[3:]+'_'+str(i)
    a=next((a for a in ae.get_all_level_actors() if a.get_actor_label()==label),None)
    if not a:a=ae.spawn_actor_from_class(u.StaticMeshActor,u.Vector(x,y,z),u.Rotator(pitch=0,yaw=yaw,roll=0))
    a.static_mesh_component.set_static_mesh(mesh(name));a.set_actor_scale3d(u.Vector(s,s,s));a.set_actor_enable_collision(i==0);a.set_actor_label(label);a.tags=['STP_Expedition08'];a.set_folder_path('Expedition08/LibraryPreview');created.append(a)
# Keep the arch passage open, reserve its piers and the two updated cliff footprints.
reserved=0
for a in large:
    c,e=a.get_actor_bounds(False);fp=u.IntPoint(x=max(1,math.ceil(e.x*1.6/100)),y=max(1,math.ceil(e.y*1.6/100)));p=surface.get_placement_for_world_location(c,fp)
    for y in range(fp.y):
        for x in range(fp.x):
            if surface.reserve_cells(a,u.STPGridCell(x=p.origin_cell.x+x,y=p.origin_cell.y+y),u.IntPoint(x=1,y=1)):reserved+=1
for dx in [-163,163]:
    p=surface.get_placement_for_world_location(u.Vector(-2100+dx,-2800,56),u.IntPoint(x=2,y=2))
    for y in range(2):
        for x in range(2):
            if surface.reserve_cells(created[0],u.STPGridCell(x=p.origin_cell.x+x,y=p.origin_cell.y+y),u.IntPoint(x=1,y=1)):reserved+=1
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/PlanetLevel_RockPreview')
json.dump({'assets':11,'new_actors':14,'replaced_cliffs':2,'reserved_cells':reserved,'placements':placements},open(OUT+'/build.json','w'),indent=2)
cap=ae.spawn_actor_from_class(u.SceneCapture2D,u.Vector(-2200,-3800,1200),u.Rotator(pitch=-38,yaw=76,roll=0))
cc=cap.get_component_by_class(u.SceneCaptureComponent2D);cc.capture_every_frame=False;cc.capture_on_movement=False;cc.fov_angle=65
rt=u.RenderingLibrary.create_render_target2d(world,1600,1000,u.TextureRenderTargetFormat.RTF_RGBA8);rt.set_editor_property('target_gamma',2.2);cc.texture_target=rt;cc.capture_source=u.SceneCaptureSource.SCS_FINAL_COLOR_LDR
started=time.monotonic();phase=0
views=[('Arch_Details',(-2200,-3800,1200),(-38,76)),('Organic_Details',(650,-3480,850),(-34,90)),('Overview',(-2200,-3300,2300),(-34,56))]
def tick(dt):
    global phase
    try:
        if time.monotonic()-started < 25+phase*8:return
        if phase<len(views):
            name,loc,rot=views[phase]
            cap.set_actor_location(u.Vector(*loc),False,False);cap.set_actor_rotation(u.Rotator(pitch=rot[0],yaw=rot[1],roll=0),False);cc.capture_scene();u.RenderingLibrary.export_render_target(world,rt,OUT,name+'.png');phase+=1
        else:
            ae.destroy_actor(cap)
            assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/PlanetLevel_RockPreview')
            json.dump({'complete':True,'views':[v[0] for v in views]},open(OUT+'/review.json','w'))
            u.unregister_slate_post_tick_callback(handle);u.EditorPythonScripting.set_keep_python_script_alive(False)
    except Exception:
        open(OUT+'/error.txt','w').write(traceback.format_exc());u.unregister_slate_post_tick_callback(handle);u.EditorPythonScripting.set_keep_python_script_alive(False)
handle=u.register_slate_post_tick_callback(tick);u.EditorPythonScripting.set_keep_python_script_alive(True)
