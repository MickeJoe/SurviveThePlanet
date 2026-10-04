import unreal,time,json
from pathlib import Path
root=Path(unreal.Paths.project_dir())
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();assert world
population=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SectorPopulation)[0]
grid=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.HexSectorGrid)[0]
for sector in grid.get_editor_property('sectors'):population.reveal_sector(sector.id)
sources=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseResourceSource)
headquarters=next(b for b in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseBuilding) if b.get_building_type()==unreal.STPBuildingType.BASE_MODULE)
source=min((s for s in sources if s.get_resource_type()==unreal.ResourceType.COAL and not s.get_reserved_mining_machine()),key=lambda s:(s.get_actor_location()-headquarters.get_actor_location()).length())
statics=unreal.get_default_object(unreal.GameplayStatics)
def spawn(cls,transform):
    actor=statics.call_method('BeginDeferredActorSpawnFromClass',(world,cls,transform,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    statics.call_method('FinishSpawningActor',(actor,transform,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT));return actor
cls=source.get_mine_blueprint();defaults=unreal.get_default_object(cls)
mine=spawn(cls,defaults.get_placement_transform_for_source(source))
assert mine.attach_to_resource_source(source)
mine.set_construction_progress(1)
drone=spawn(unreal.load_class(None,'/Game/BluePrints/MiningDrone/BP_MiningDrone.BP_MiningDrone_C'),unreal.Transform(location=mine.get_actor_location()+unreal.Vector(0,-250,50)))
assert drone.assign_to_building(mine,0)
comps={c.get_name():c for c in mine.get_components_by_class(unreal.StaticMeshComponent)}
for key in ['CoalCutter','ConveyorCoal','CuttingChips','CuttingDust']:
    assert comps[key].get_editor_property('static_mesh'),key+' missing mesh after asset reload'
assert comps['CuttingDust'].get_material(0).get_name()=='M_CoalDust'
assert comps['CoalCutter'].get_editor_property('relative_location').y==61
manager=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.ResourceManager)[0]
grids=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.CableNetworkManager)
if not grids:grids=[spawn(unreal.CableNetworkManager,unreal.Transform())]
if grids:grids[0].refresh_energy_grid()
manager.set_resource_amount(unreal.ResourceType.ENERGY,1000)
if not mine.is_connected_to_power_grid():
    # A real Base Camp power source in the test world, using the shared coverage rules.
    power_source=spawn(unreal.load_class(None,'/Game/BluePrints/BaseModule/BP_BaseModule.BP_BaseModule_C'),unreal.Transform(location=mine.get_actor_location()+unreal.Vector(550,0,0)))
    power_source.set_construction_progress(1)
grids[0].refresh_energy_grid()
pawn=unreal.GameplayStatics.get_player_pawn(world,0)
pawn.set_actor_location(mine.get_actor_location(),False,False)
boom=pawn.get_component_by_class(unreal.SpringArmComponent)
boom.set_editor_property('target_arm_length',1800)
camera=pawn.get_component_by_class(unreal.CameraComponent)
before_rotation=str(comps['CoalCutter'].get_editor_property('relative_rotation'))
before_coal=manager.get_resource_amount(unreal.ResourceType.COAL)
before_payload=str(comps['ConveyorCoal'].get_instance_transform(0,False))
unreal.GameplayStatics.set_global_time_dilation(world,1)
started=time.monotonic();phase=-1;report={'passed':False,'real_gameplay_power':True,'power_source_fixture':True}
def check(delta):
    global phase,started
    if time.monotonic()-started<(12 if phase==0 else .4):return
    try:
        if phase==-1:
            grids[0].refresh_energy_grid()
            assert mine.is_producing_resource(),'No shared power at the test mine'
            phase=0;started=time.monotonic();return
        if phase==0:
            assert manager.get_resource_amount(unreal.ResourceType.COAL)>before_coal
            assert str(comps['CoalCutter'].get_editor_property('relative_rotation'))!=before_rotation
            assert str(comps['ConveyorCoal'].get_instance_transform(0,False))!=before_payload
            assert comps['CuttingChips'].is_visible() and comps['CuttingDust'].is_visible()
            report['produced']=manager.get_resource_amount(unreal.ResourceType.COAL)-before_coal
            report['working_components']={k:{'mesh':comps[k].static_mesh.get_path_name(),'visible':comps[k].is_visible()} for k in ['CoalCutter','ConveyorCoal','CuttingChips','CuttingDust']}
            drone.unassign_from_building();phase=1;started=time.monotonic();return
        assert not comps['CuttingChips'].is_visible() and not comps['CuttingDust'].is_visible()
        assert not mine.is_producing_resource()
        assert drone.assign_to_building(mine,0)
        report['passed']=True
    except Exception as e:report['error']=str(e);unreal.log_error('COAL_VISUAL_TEST_FAILED '+repr(e))
    finally:
        if report.get('passed') or report.get('error'):
            (root/'Saved/CoalVisualTest.json').write_text(json.dumps(report,indent=2))
            unreal.unregister_slate_post_tick_callback(handle)
            unreal.log('COAL_VISUAL_TEST '+json.dumps(report))
handle=unreal.register_slate_post_tick_callback(check)
