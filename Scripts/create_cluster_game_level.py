"""Duplicate PlanetLevel, preserving gameplay setup and sector count."""
import hashlib
import json
from pathlib import Path
import unreal

project=Path(unreal.Paths.project_dir()).resolve()
assert str(project).lower()==r'C:\UE5\SurviveThePlanet 5.8'.lower()
source=project/'Content'/'PlanetLevel.umap'
original_hash=hashlib.sha256(source.read_bytes()).hexdigest()
assets=unreal.EditorAssetLibrary
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
destination='/Game/WorldGeneration/Maps/L_PlanetClusters'
assert not assets.does_asset_exist(destination),'New map already exists; refusing to replace it.'
world=unreal.EditorLoadingAndSavingUtils.new_map_from_template('/Game/PlanetLevel',False)
assert world
assert unreal.EditorLoadingAndSavingUtils.save_map(world,destination)
all_actors=actors.get_all_level_actors()
grid=next(a for a in all_actors if isinstance(a,unreal.HexSectorGrid))
grid.rebuild_grid()
baseline=json.loads((project/'Saved'/'PlanetRuntimeInventory.json').read_text())
assert len(grid.get_editor_property('sectors'))==baseline['grid']['count']==37
assert grid.get_editor_property('exploration_sector_radius')==4000
population=next(a for a in all_actors if isinstance(a,unreal.SectorPopulation))
population.set_editor_property('grid',grid)
population.set_editor_property('authored_sector_template',assets.load_asset('/Game/WorldGeneration/SectorTemplates/ST_First/DA_SectorTemplate_First'))
population.set_editor_property('cluster_variant_library',assets.load_asset('/Game/WorldGeneration/ClusterVariants/FullSector/DA_ClusterVariantLibrary_FullSector'))
population.set_editor_property('seed',71237)
population.set_actor_label('Sector Population - Authored Clusters')
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(1400,-5050,6890),unreal.Rotator(pitch=-52,yaw=98,roll=0))
assert levels.save_current_level()
assert hashlib.sha256(source.read_bytes()).hexdigest()==original_hash,'Source map was modified'
(project/'Saved'/'ClusterGameLevel.json').write_text(json.dumps({'map':destination,'sector_count':37,'sector_radius_cm':4000,'base_seed':71237,'original_planet_level_sha256':original_hash,'original_unchanged':True},indent=2))
unreal.log('CLUSTER_GAME_LEVEL_CREATED')
