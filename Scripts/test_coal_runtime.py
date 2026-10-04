import unreal,json,collections
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
try:
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();assert world
    population=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SectorPopulation)[0]
    grid=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.HexSectorGrid)[0]
    assert not population.get_editor_property('diagnostics'),str(population.get_editor_property('diagnostics'))
    deposits=list(population.get_editor_property('resources').deposits)
    bysector=collections.defaultdict(list)
    for d in deposits:
        bysector[d.sector_id].append(d.resource_type);assert 3000<=d.quantity<=5000
    start=grid.get_editor_property('starting_sector_id');assert set(bysector[start])=={unreal.ResourceType.STONE,unreal.ResourceType.COPPER,unreal.ResourceType.COAL}
    assert len(bysector)==len(grid.get_editor_property('sectors'))
    for sid,types in bysector.items():
        if sid!=start:assert 1<=len(types)<=3 and len(set(types))==len(types)
    for sector in grid.get_editor_property('sectors'):population.reveal_sector(sector.id)
    sources=list(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseResourceSource))
    checked=collections.Counter();statics=unreal.get_default_object(unreal.GameplayStatics)
    def spawn(cls,transform):
        return statics.call_method('BeginDeferredActorSpawnFromClass',(world,cls,transform,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    def finish(actor,transform):statics.call_method('FinishSpawningActor',(actor,transform,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    coal_mine=None
    for source in sources:
        cls=source.get_mine_blueprint();defaults=unreal.get_default_object(cls);transform=defaults.get_placement_transform_for_source(source)
        preview=spawn(cls,transform);preview.set_placement_preview(True);finish(preview,transform)
        assert preview.can_mine_resource_source(source),source.get_name()
        mine=spawn(cls,transform);finish(mine,transform);mine.set_placement_preview(False)
        assert mine.attach_to_resource_source(source),source.get_name()
        assert source.get_reserved_mining_machine()==mine
        assert not preview.can_mine_resource_source(source)
        assert not mine.is_producing_resource();assert mine.get_current_output_per_minute()==0
        mine.set_construction_progress(1)
        assert not mine.is_producing_resource(),'Unstaffed mine must not produce'
        if source.get_resource_type()==unreal.ResourceType.COAL:
            assert isinstance(mine,unreal.CoalMiningMachine)
            assert mine.get_max_drone_slots()==4 and mine.get_unlocked_drone_slots()==1
            assert mine.get_output_per_minute_at100_percent(unreal.ResourceType.COAL)==10
            assert mine.get_output_per_minute_at100_percent(unreal.ResourceType.COPPER)==0
            comps={c.get_name():c for c in mine.get_components_by_class(unreal.StaticMeshComponent)}
            assert comps['CoalCutter'].static_mesh
            assert comps['ConveyorCoal'].get_instance_count()==6
            assert comps['CuttingChips'].get_instance_count()==16
            assert not comps['CuttingChips'].is_visible()
            if coal_mine is None:coal_mine=mine
        checked[str(source.get_resource_type())]+=1;preview.destroy_actor()
    assert coal_mine and len(sources)==len(deposits)
    report={'passed':True,'sectors':len(bysector),'deposits':len(deposits),'start_resources':[str(t) for t in bysector[start]],'checked_mines':dict(checked),'animation_components':True}
    unreal.log('COAL_RUNTIME_PASSED '+json.dumps(report))
except Exception as error:
    report={'passed':False,'error':str(error)};raise
finally:(root/'Saved/CoalRuntimeTest.json').write_text(json.dumps(report,indent=2))
