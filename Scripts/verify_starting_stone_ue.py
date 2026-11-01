"""Verify starting stone location and mineability in transient PIE worlds on all three planets."""
import unreal,json,time,traceback
from pathlib import Path
root=Path('C:/UE5/SurviveThePlanet 5.8')
assert Path(unreal.Paths.project_dir()).resolve()==root.resolve()
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
maps=['Nexaris','Aridus','Borealis']
state={'map':0,'stage':0,'started':time.monotonic(),'map_started':time.monotonic(),'checks':[]}
def report(passed,error=None):
    (root/'Saved/StartingStonePosition.json').write_text(json.dumps(dict(passed=passed,checks=state['checks'],error=error),indent=2))
    unreal.unregister_slate_post_tick_callback(stone_check_handle)
    unreal.log('STARTING STONE VERIFICATION '+('PASSED' if passed else 'FAILED: '+str(error)))
def tick(delta):
    try:
        if time.monotonic()-state['started']>180:raise AssertionError('Timeout')
        world=editor.get_game_world()
        if state['stage']==0:
            if not world or time.monotonic()-state['map_started']<5:return
            grid=unreal.GameplayStatics.get_actor_of_class(world,unreal.HexSectorGrid)
            hq=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseBuilding) if a.get_building_type()==unreal.STPBuildingType.BASE_MODULE)
            sources=[s for s in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseResourceSource) if grid.get_sector_at_world_location(s.get_actor_location())==grid.get_editor_property('starting_sector_id')]
            stone=next(s for s in sources if s.get_resource_type()==unreal.ResourceType.STONE)
            preferred=hq.get_actor_location()+unreal.MathLibrary.transform_direction(grid.get_actor_transform(),unreal.Vector(3000,0,0))
            loc=stone.get_actor_location()
            anchor_distance=((loc.x-preferred.x)**2+(loc.y-preferred.y)**2)**0.5
            assert anchor_distance<=501,('Stone fell back to outer ring',str(loc),anchor_distance)
            for source in sources:
                cls=source.get_mine_blueprint()
                if cls:assert unreal.get_default_object(cls).can_mine_resource_source(source),source.get_name()
            spawner=unreal.get_default_object(unreal.GameplayStatics)
            tr=unreal.get_default_object(stone.get_mine_blueprint()).get_placement_transform_for_source(stone)
            actual=spawner.call_method('BeginDeferredActorSpawnFromClass',(world,stone.get_mine_blueprint(),tr,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
            actual=spawner.call_method('FinishSpawningActor',(actual,tr,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
            assert actual.attach_to_resource_source(stone),'Actual stone mine could not attach'
            actual.destroy_actor()
            state['checks'].append(dict(planet=maps[state['map']],stone=[loc.x,loc.y,loc.z],hq=[hq.get_actor_location().x,hq.get_actor_location().y],distance_to_preferred_cm=anchor_distance,starting_mines_buildable=True,actual_stone_mine_attached=True))
            levels.editor_request_end_play();state['stage']=1;return
        if world:return
        state['map']+=1
        if state['map']<len(maps):
            assert levels.load_level('/Game/Planet/Maps/L_'+maps[state['map']])
            state['map_started']=time.monotonic();state['stage']=0;levels.editor_request_begin_play()
        else:
            levels.load_level('/Game/Planet/Maps/L_Nexaris')
            report(True)
    except:
        report(False,traceback.format_exc())
        if editor.get_game_world():levels.editor_request_end_play()
assert levels.load_level('/Game/Planet/Maps/L_Nexaris')
stone_check_handle=unreal.register_slate_post_tick_callback(tick)
levels.editor_request_begin_play()
