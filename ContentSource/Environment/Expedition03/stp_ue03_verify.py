import unreal as u,json,time,os,traceback
ROOT=r'C:\UE5\SurviveThePlanet 5.8';OUT=ROOT+r'\Saved\Expedition03'
ae=u.get_editor_subsystem(u.EditorActorSubsystem)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert world.get_name()=='PlanetLevel_RockPreview'
for a in ae.get_all_level_actors():
    if isinstance(a,u.DirectionalLight):a.light_component.set_editor_property('forward_shading_priority',0 if a.get_actor_label()=='E02_SoftFill' else 1)
surface=next(a for a in ae.get_all_level_actors() if 'SurfaceManager' in a.get_name())
assert len(surface.get_components_by_class(u.StaticMeshComponent))==80
center,extent=surface.get_actor_bounds(False);assert abs(center.z+extent.z-56.55)<1
u.EditorLoadingAndSavingUtils.save_map(world,'/Game/PlanetLevel_RockPreview')
started=None;done=False
def tick(dt):
    global started,done
    try:
        worlds=u.EditorLevelLibrary.get_pie_worlds(False)
        if not worlds:return
        if started is None:started=time.monotonic()
        if time.monotonic()-started<12 or done:return
        done=True;pie=worlds[0]
        actors=u.GameplayStatics.get_all_actors_of_class(pie,u.Actor)
        s=next(a for a in actors if 'SurfaceManager' in a.get_name())
        sources=[(-570,-1070),(2130,130),(630,1630),(1830,1330),(-870,130),(630,-1370),(1830,-1970),(1830,2230),(-1770,130)]
        results=[]
        for x,y in sources:
            successful=False
            for dx,dy in [(450,0),(-450,0),(0,450),(0,-450)]:
                goal=s.get_placement_for_world_location(u.Vector(x+dx,y+dy,56.55),u.IntPoint(x=1,y=1))
                result=s.find_grid_path(u.Vector(600,-350,56.55),goal.origin_cell,u.IntPoint(x=1,y=1))
                ok=bool(result[0]) if isinstance(result,tuple) and isinstance(result[0],bool) else bool(result)
                if ok:successful=True;break
            results.append({'resource_xy':[x,y],'reachable_approach':successful})
        report={'map':pie.get_name(),'ground_chunks':len(s.get_components_by_class(u.StaticMeshComponent)),'routes':results,'reachable':sum(r['reachable_approach'] for r in results)}
        with open(OUT+'/gameplay_verified.json','w') as f:json.dump(report,f,indent=2)
        u.SystemLibrary.execute_console_command(pie,'Shot showui')
        u.unregister_slate_post_tick_callback(handle)
    except Exception:
        with open(OUT+'/gameplay_error.txt','w') as f:f.write(traceback.format_exc())
        u.unregister_slate_post_tick_callback(handle)
handle=u.register_slate_post_tick_callback(tick)
u.EditorPythonScripting.set_keep_python_script_alive(True)
