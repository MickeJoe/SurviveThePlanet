import unreal,json
from pathlib import Path
taskdir=Path(unreal.Paths.project_saved_dir())
changes=[]
for name in ['DA_PlanetDefinition','DA_Nexaris','DA_Aridus','DA_Borealis']:
    asset=unreal.load_asset('/Game/Data/Planet/'+name);assert asset
    conf=asset.get_editor_property('mission_confidence')
    conf.set_editor_property('decay_per_game_hour',2.5)
    asset.set_editor_property('mission_confidence',conf)
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset,False)
    changes.append(name+': confidence -2.5/hour')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actorsub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for name in ['L_Nexaris','L_Aridus','L_Borealis']:
    assert levels.load_level('/Game/Planet/Maps/'+name)
    points=[a for a in actorsub.get_all_level_actors() if isinstance(a,unreal.DroneSpawnPoint)]
    construction=[a for a in points if a.get_editor_property('drone_type')==unreal.DroneType.CONSTRUCTION]
    assert len(construction)==1,(name,[(a.get_name(),str(a.get_editor_property('drone_type'))) for a in points])
    a=construction[0];a.set_editor_property('spawn_count',2)
    options=a.get_editor_property('spawn_options')
    for option in options:option.set_editor_property('spawn_count',2)
    a.set_editor_property('spawn_options',options)
    assert levels.save_current_level()
    changes.append(name+': 2 construction drones + 1 mining drone')
assert levels.load_level('/Game/Planet/Maps/L_Nexaris')
(taskdir/'balance_round2_authoring.json').write_text(json.dumps(changes,indent=2))
