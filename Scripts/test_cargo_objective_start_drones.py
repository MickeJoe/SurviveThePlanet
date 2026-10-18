import unreal,json
from pathlib import Path
taskdir=Path(unreal.Paths.project_saved_dir())
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();assert world
actors=lambda cls:list(unreal.GameplayStatics.get_all_actors_of_class(world,cls))
sub=unreal.get_default_object(unreal.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary'))
objectives=sub.call_method('GetWorldSubsystem',(world,unreal.ObjectiveSubsystem))
mission=sub.call_method('GetWorldSubsystem',(world,unreal.MissionConfidenceSubsystem));mission.set_decay_paused(True)
wl=unreal.get_default_object(unreal.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
drones=actors(unreal.BaseDrone);assert len(drones)==3,len(drones)
assert all(d.is_available_for_assignment() for d in drones)
groups={}
for d in drones:groups.setdefault(str(d.get_drone_display_name()),[]).append(d)
assert sorted(len(g) for g in groups.values())==[1,2]
statics=unreal.get_default_object(unreal.GameplayStatics)
def spawn_obj(cls):
 t=unreal.Transform(location=unreal.Vector(0,0,-10000));a=statics.call_method('BeginDeferredActorSpawnFromClass',(world,cls,t,unreal.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT));return statics.call_method('FinishSpawningActor',(a,t,unreal.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
plant=spawn_obj(unreal.CrudeWorkshop);plant.set_construction_progress(1);plant.set_production_enabled(False)
pc=actors(unreal.SurviveThePlanetPlayerController)[0];pc.set_selected_actor(plant)
info=wl.call_method('GetAllWidgetsOfClass',(world,unreal.BuildingInfoWidget,False))[0]
def widget_in(root,name):return next(o for o in unreal.ObjectIterator(unreal.Widget) if o.get_name()==name and o.get_path_name().startswith(root.get_path_name()))
def picker_grid():
 return next(o for o in unreal.ObjectIterator(unreal.UniformGridPanel) if o.get_name()=='DroneOptionsGrid' and isinstance(o.get_outer().get_outer(),unreal.UserWidget) and o.get_outer().get_outer().is_in_viewport())
slots=widget_in(info,'DroneSlotsBox')
assignment_names=[];initial_counts={}
ordered=sorted(groups,key=lambda n:-len(groups[n]))
for i,name in enumerate([ordered[0],ordered[0],ordered[1]]):
 slots.get_child_at(i).call_method('HandleClicked')
 grid=picker_grid();assert grid.get_children_count()==2,grid.get_children_count()
 cards=[grid.get_child_at(j) for j in range(grid.get_children_count())]
 if i==0:
  initial_counts={str(widget_in(card,'DroneNameText').get_text()):str(widget_in(card,'DroneCountText').get_text()) for card in cards}
  assert sorted(initial_counts.values())==['x1','x2'],initial_counts
 card=next(c for c in cards if str(widget_in(c,'DroneNameText').get_text())==name)
 button=widget_in(card,'SelectButton');assert button.get_is_enabled(),(i,name)
 button.call_method('HandleClicked')
 assert plant.get_assigned_drone_count()==i+1
 assignment_names.append(name)
assert all(plant.get_assigned_drone_at_slot(i) is not None for i in range(3))
for d in drones:plant.unassign_drone(d)
assert all(d.is_available_for_assignment() for d in drones)
before=[str(s.objective_id) for s in objectives.get_active_objectives()]
assert 'mission_build_communication' in before and 'mission_build_cargo_bay' not in before,before
objectives.submit_building_built(unreal.STPBuildingType.COMMUNICATION_MODULE,1)
active=[str(s.objective_id) for s in objectives.get_active_objectives()]
assert 'mission_build_cargo_bay' in active and 'mission_build_communication' not in active,active
tracker=wl.call_method('GetAllWidgetsOfClass',(world,unreal.ObjectiveTrackerWidget,False))[0]
labels=[str(t.get_text()) for t in unreal.ObjectIterator(unreal.TextBlock) if t.get_path_name().startswith(tracker.get_path_name())]
assert 'BUILD CARGO BAY' in labels,labels
progress=mission.get_mission_progress()
objectives.submit_building_built(unreal.STPBuildingType.CARGO_BAY,1)
assert 'mission_build_cargo_bay' not in [str(s.objective_id) for s in objectives.get_active_objectives()]
assert mission.get_mission_progress()==progress+1
objectives.submit_building_built(unreal.STPBuildingType.CARGO_BAY,1)
assert mission.get_mission_progress()==progress+1
(taskdir/'objective_drones_test.json').write_text(json.dumps({'passed':True,'cargo_objective_activated_after_uplink':True,'cargo_objective_visible_in_tracker':True,'cargo_progress_reward_once':1,'start_drones':3,'picker_type_cards':2,'initial_counts':initial_counts,'assigned_via_ui':assignment_names,'all_three_assigned':True},indent=2))
