import unreal
import json
from pathlib import Path
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/PlanetLevel')
result={'actors':[]}
for a in actors.get_all_level_actors():
    result['actors'].append({'label':a.get_actor_label(),'class':a.get_class().get_name(),'location':str(a.get_actor_location())})
    if isinstance(a,unreal.HexSectorGrid):
        a.rebuild_grid()
        result['grid']={'radius':a.get_editor_property('grid_radius'),'sector_radius':a.get_editor_property('exploration_sector_radius'),'count':len(a.get_editor_property('sectors')),'transform':str(a.get_actor_transform())}
    if isinstance(a,unreal.PlanetSurfaceManager):
        result['surface']={p:str(a.get_editor_property(p)) for p in ['chunk_diameter','cells_per_chunk','chunk_meshes','chunk_height_offset']}
(Path(unreal.Paths.project_dir())/'Saved'/'PlanetRuntimeInventory.json').write_text(json.dumps(result,indent=2))
