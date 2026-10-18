import unreal,json,time,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
project=Path('C:/UE5/SurviveThePlanet 5.8')
backups={p:p.read_bytes() for p in (project/'Saved/SaveGames').glob('*.sav')}
state={'stage':0,'started':time.monotonic(),'checks':[],'map':0}
maps=['Nexaris','Aridus','Borealis']
editor=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
statics=unreal.get_default_object(unreal.GameplayStatics)
def spawn(cls,transform):
    actor=statics.call_method('BeginDeferredActorSpawnFromClass',(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world(),cls,transform,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    return statics.call_method('FinishSpawningActor',(actor,transform,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
editor.editor_request_begin_play()
def tick_mine_test(delta):
    try:
        if time.monotonic()-state['started']>240:raise AssertionError('Test timeout')
        world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if state['stage']==0:
            if not world or time.monotonic()-state.get('map_started',state['started'])<7:return
            surface=unreal.GameplayStatics.get_actor_of_class(world,unreal.PlanetSurfaceManager);assert surface
            grid=unreal.GameplayStatics.get_actor_of_class(world,unreal.HexSectorGrid);assert grid
            # Isolate geometric placement from the separate exploration permission gate.
            for sector in grid.get_editor_property('sectors'):
                grid.set_sector_state(sector.id,unreal.SectorState.ESTABLISHED)
            sources=[s for s in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseResourceSource) if grid.get_sector_at_world_location(s.get_actor_location())==grid.get_editor_property('starting_sector_id')]
            assert sources,'No generated resource sources'
            tested=0
            for source in sources:
                mine_class=source.get_mine_blueprint()
                if not mine_class:continue
                mine=unreal.get_default_object(mine_class)
                assert mine.can_mine_resource_source(source),'Mine already blocked: '+source.get_name()
                footprint=mine.get_grid_footprint()
                placement=surface.get_placement_for_world_location(mine.get_placement_transform_for_source(source).translation,footprint)
                clearance=surface.get_building_clearance_cells()
                blocked=False;open_found=False
                # The enlarged area beyond the raw deposit must also be protected.
                for x in range(placement.origin_cell.x-clearance,placement.origin_cell.x+footprint.x+clearance):
                    cell=unreal.STPGridCell(x=x,y=placement.origin_cell.y)
                    loc=surface.get_world_location_for_cell(cell)
                    if surface.can_occupy_cells(cell,unreal.IntPoint(1,1)):
                        assert not surface.get_building_placement_for_world_location(loc,unreal.IntPoint(1,1)).valid
                        blocked=True
                        # Final placement must reject the same cell, even without preview.
                        ordinary=spawn(unreal.BaseBuilding,unreal.Transform(location=loc))
                        assert not surface.reserve_cells(ordinary,cell,unreal.IntPoint(1,1))
                        ordinary.destroy_actor()
                        break
                assert blocked,'No empty protected test cell'
                # Navigation occupancy remains separate from future building restrictions.
                assert surface.can_occupy_cells(cell,unreal.IntPoint(1,1))
                actual_mine=spawn(mine_class,mine.get_placement_transform_for_source(source))
                assert actual_mine.attach_to_resource_source(source),'Matching mine cannot reserve protected footprint'
                actual_mine.destroy_actor()
                tested+=1
            assert tested>=3
            state['checks'].append(maps[state['map']]+': all '+str(tested)+' starting deposits retain buildable mines; nearby empty cells reject building preview and final reservation without blocking navigation')
            editor.editor_request_end_play();state['stage']=1;return
        if state['stage']==1:
            if world:return
            state['map']+=1
            if state['map']<len(maps):
                editor.load_level('/Game/Planet/Maps/L_'+maps[state['map']])
                state['map_started']=time.monotonic();state['stage']=0;editor.editor_request_begin_play();return
            for p,b in backups.items():p.write_bytes(b)
            (project/'Saved/MinePlacementProtectionTest.json').write_text(json.dumps(dict(passed=True,checks=state['checks']),indent=2))
            editor.load_level('/Game/Planet/Maps/L_PlanetSelection')
            unreal.unregister_slate_post_tick_callback(mine_test_handle)
    except:
        (project/'Saved/MinePlacementProtectionTest.json').write_text(json.dumps(dict(passed=False,checks=state['checks'],error=traceback.format_exc()),indent=2))
        editor.editor_request_end_play()
        unreal.unregister_slate_post_tick_callback(mine_test_handle)
mine_test_handle=unreal.register_slate_post_tick_callback(tick_mine_test)
