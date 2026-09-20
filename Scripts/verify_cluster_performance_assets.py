"""Verify final editor state and save only the five optimized mesh packages."""
import json
from pathlib import Path
import unreal

project = Path(unreal.Paths.project_dir()).resolve()
assert str(project).lower() == r'C:\UE5\SurviveThePlanet 5.8'.lower()
subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
paths = [
    '/Game/Environment/Expedition02/Meshes/SM_Boulder_02',
    '/Game/Environment/Expedition02/Meshes/SM_Cliff_Crown_02',
    '/Game/Environment/Expedition02/Meshes/SM_Cliff_Ridge_02',
    '/Game/Environment/Expedition02/Meshes/SM_Scree_02',
    '/Game/Environment/Expedition04/Meshes/SM_Rock04_Wedge',
]
for path in paths:
    mesh = unreal.load_asset(path)
    assert subsystem.get_nanite_settings(mesh).enabled, path
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh), path
    # Editor enables the Nanite material usage permutation automatically. Persist
    # that flag too, so standalone/cooked loading does not depend on editor repair.
    for slot in mesh.get_editor_property('static_materials'):
        material = slot.material_interface
        while isinstance(material, unreal.MaterialInstance):
            material = material.get_editor_property('parent')
        assert material.get_editor_property('used_with_nanite')
        assert unreal.EditorAssetLibrary.save_loaded_asset(material)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
population = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SectorPopulation)[0]
assert population.get_editor_property('manage_cluster_residency')
assert population.get_editor_property('optimize_cluster_rendering')
report = {'result': 'passed', 'nanite_meshes': paths, 'runtime_policies_enabled': True,
          'dirty_maps': [p.get_path_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()],
          'dirty_content': [p.get_path_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]}
(project/'Saved'/'ClusterPerformanceAssets.json').write_text(json.dumps(report, indent=2))
unreal.log('CLUSTER_PERFORMANCE_ASSETS ' + json.dumps(report))
