import unreal,json,time,traceback
from pathlib import Path
project=Path('C:/UE5/SurviveThePlanet 5.8')
backups={p:p.read_bytes() for p in (project/'Saved/SaveGames').glob('*.sav')}
state={'stage':0,'started':time.monotonic(),'checks':[]}
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
def verify_hover(delta):
    try:
        if time.monotonic()-state['started']>60:raise AssertionError('PIE check timed out')
        world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if state['stage']==0:
            if not world or time.monotonic()-state['started']<6:return
            pc=unreal.GameplayStatics.get_player_controller(world,0);assert pc
            sources=list(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseResourceSource));assert sources
            buildings=list(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseBuilding));assert buildings
            drones=list(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.BaseDrone));assert drones
            grid=unreal.GameplayStatics.get_actor_of_class(world,unreal.HexSectorGrid)
            starting=[s for s in sources if grid.get_sector_at_world_location(s.get_actor_location())==grid.get_editor_property('starting_sector_id')]
            assert {unreal.ResourceType.IRON,unreal.ResourceType.STONE,unreal.ResourceType.COPPER}.issubset({s.get_resource_type() for s in starting})
            resources=unreal.GameplayStatics.get_actor_of_class(world,unreal.ResourceManager)
            assert resources.get_resource_amount(unreal.ResourceType.IRON)==20
            workshop=unreal.load_asset('/Game/Data/Buildings/BlueprintCatalog/DA_CrudeWorkshop')
            costs=workshop.get_editor_property('construction_costs')
            assert len(costs)==1 and costs[0].resource==unreal.ResourceType.IRON and costs[0].cost==40
            assert not resources.can_afford_costs(costs)
            state['checks'].append('Nexaris starts with iron, stone and copper deposits; 20 iron cannot pay workshop cost of 40')
            wl=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
            widget=wl.call_method('Create',(world,unreal.load_asset('/Game/UI/WBP_WorldHoverTooltip').generated_class(),pc))
            widget.add_to_viewport(90)
            for actor,expected in [(buildings[0],str(buildings[0].get_building_display_name())),(drones[0],str(drones[0].get_drone_display_name()))]:
                assert expected
                assert str(unreal.WorldHoverTooltipWidget.get_actor_display_name(actor))==expected
                widget.set_hovered_actor(actor)
                label=next(o for o in unreal.ObjectIterator(unreal.TextBlock) if o.get_name()=='NameText' and o.get_path_name().startswith(widget.get_path_name()))
                assert str(label.get_text())==expected
                assert widget.get_visibility()==unreal.SlateVisibility.HIT_TEST_INVISIBLE
            for source in sources:
                expected=next(str(d.display_name) for d in unreal.ResourceCatalog.get_resource_definitions() if d.resource_type==source.get_resource_type())
                assert str(unreal.WorldHoverTooltipWidget.get_actor_display_name(source))==expected
            state['checks'].append('Real building, drone and all visible resource sources resolve their catalog display names')
            widget.set_hovered_actor(None);assert widget.get_visibility()==unreal.SlateVisibility.COLLAPSED
            state['checks'].append('Authored UMG label displays correct name, ignores hit testing and hides on empty target')
            widget.remove_from_parent()
            unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()
            state['stage']=1;return
        if state['stage']==1:
            if world:return
            for p,b in backups.items():p.write_bytes(b)
            (project/'Saved/WorldHoverTooltipTest.json').write_text(json.dumps(dict(passed=True,checks=state['checks']),indent=2))
            unreal.unregister_slate_post_tick_callback(hover_test_handle)
    except:
        (project/'Saved/WorldHoverTooltipTest.json').write_text(json.dumps(dict(passed=False,checks=state['checks'],error=traceback.format_exc()),indent=2))
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()
        unreal.unregister_slate_post_tick_callback(hover_test_handle)
hover_test_handle=unreal.register_slate_post_tick_callback(verify_hover)
