import unreal,time,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();assert world
mines=list(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.CoalMiningMachine));assert mines
mine=mines[0];source=mine.get_resource_source();assert source
for existing in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseDrone):
    if existing.get_assigned_building()==mine: existing.unassign_from_building()
surface=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlanetSurfaceManager)[0]
grids=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.CableNetworkManager)
managers=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.ResourceManager)
statics=unreal.get_default_object(unreal.GameplayStatics)
def spawn(cls,transform):
    actor=statics.call_method('BeginDeferredActorSpawnFromClass',(world,cls,transform,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    statics.call_method('FinishSpawningActor',(actor,transform,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT));return actor
grid=grids[0] if grids else spawn(unreal.CableNetworkManager,unreal.Transform())
manager=managers[0] if managers else spawn(unreal.ResourceManager,unreal.Transform())
placement=surface.get_placement_for_world_location(mine.get_actor_location(),mine.get_grid_footprint())
origin=placement.origin_cell;footprint=mine.get_grid_footprint()
cell=unreal.IntPoint(origin.x+footprint.x,origin.y)
hq_cell=unreal.STPGridCell();hq_cell.x=cell.x+1;hq_cell.y=cell.y
hq=spawn(unreal.BaseBuilding,unreal.Transform(location=surface.get_world_location_for_cell(hq_cell)));hq.set_construction_progress(1)
data=mine.get_building_data()
original_type=unreal.STPBuildingType.MINING_MACHINE
# Transient power test double. The current level has no working connector grid.
# Nothing is saved; normal mining power gates are checked by restoring identity.
data.set_editor_property('building_type',unreal.STPBuildingType.BASE_MODULE)
manager.set_resource_amount(unreal.ResourceType.ENERGY,1000)
drone=spawn(unreal.load_class(None,'/Game/BluePrints/MiningDrone/BP_MiningDrone.BP_MiningDrone_C'),unreal.Transform(location=mine.get_actor_location()+unreal.Vector(0,-250,50)))
assert drone.assign_to_building(mine,0)
grid.refresh_energy_grid();assert mine.is_producing_resource()
assert mine.get_current_output_per_minute()>0
comps={c.get_name():c for c in mine.get_components_by_class(unreal.StaticMeshComponent)}
before_amount=manager.get_resource_amount(unreal.ResourceType.COAL);before_remaining=source.get_remaining_amount()
before_rotation=str(comps['CoalCutter'].get_editor_property('relative_rotation'))
unreal.GameplayStatics.set_global_time_dilation(world,20)
started=time.monotonic();phase=0;report={'passed':False}
def check(delta):
    global phase,started,report
    if time.monotonic()-started < (8 if phase==0 else .4):return
    try:
        if phase==0:
            produced=manager.get_resource_amount(unreal.ResourceType.COAL)-before_amount
            assert produced>=1,'No coal entered inventory'
            assert before_remaining-source.get_remaining_amount()==produced,'Source and inventory must balance'
            assert str(comps['CoalCutter'].get_editor_property('relative_rotation'))!=before_rotation,'Cutter did not rotate'
            assert comps['CuttingChips'].is_visible()
            widget_library=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
            displays=widget_library.call_method('GetAllWidgetsOfClass',(world,unreal.ResourceDisplayWidget,False))
            assert displays,'Resource display missing'
            display=displays[0];assert display.get_editor_property('coal_icon');assert display.get_editor_property('coal_amount_text');assert display.get_editor_property('coal_rate_text')
            assert str(display.get_editor_property('coal_amount_text').get_text())==str(manager.get_resource_amount(unreal.ResourceType.COAL))
            report['produced']=produced
            assert drone.unassign_from_building();assert not mine.is_producing_resource()
            phase=1;started=time.monotonic();return
        if phase==1:
            assert not comps['CuttingChips'].is_visible(),'VFX remain active without drones'
            assert drone.assign_to_building(mine,0)
            data.set_editor_property('building_type',original_type);grid.refresh_energy_grid()
            assert not mine.is_connected_to_power_grid();assert not mine.is_producing_resource()
            phase=2;started=time.monotonic();return
        if phase==2:
            assert not comps['CuttingChips'].is_visible(),'VFX remain active without power'
            data.set_editor_property('building_type',unreal.STPBuildingType.BASE_MODULE);grid.refresh_energy_grid();assert mine.is_producing_resource()
            mine.set_construction_progress(.5);assert not mine.is_producing_resource();mine.set_construction_progress(1)
            source.extract_resource(source.get_remaining_amount());assert not mine.is_producing_resource();assert mine.get_current_output_per_minute()==0
            phase=3;started=time.monotonic();return
        assert not comps['CuttingChips'].is_visible(),'Depleted source leaves VFX active'
        report.update({'passed':True,'power_test_double':True,'normal_level_power_verified':False,'checks':['coal inventory','finite depletion','cutter movement','conveyor instances','chip VFX','drone stop','normal no-power stop','construction stop','depletion stop','UMG coal amount and rate']})
    except Exception as error:
        report['error']=str(error);unreal.log_error('COAL_PRODUCTION_FAILED '+repr(error))
    finally:
        if report.get('passed') or report.get('error'):
            (root/'Saved/CoalProductionTest.json').write_text(json.dumps(report,indent=2));unreal.unregister_slate_post_tick_callback(handle)
            data.set_editor_property('building_type',original_type);unreal.GameplayStatics.set_global_time_dilation(world,.0001)
            unreal.log('COAL_PRODUCTION_RESULT '+json.dumps(report))
handle=unreal.register_slate_post_tick_callback(check)
