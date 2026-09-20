"""Enable adaptive Nanite geometry on the five dense rocks used by clusters.

Preserves source geometry/materials/transforms; does not merge assets or save maps.
The original assets had Nanite disabled. --disable restores that setting for A/B.
"""
import json
import sys
from pathlib import Path
import unreal

project = Path(unreal.Paths.project_dir()).resolve()
assert str(project).lower() == r'C:\UE5\SurviveThePlanet 5.8'.lower()
paths = [
    '/Game/Environment/Expedition02/Meshes/SM_Boulder_02',
    '/Game/Environment/Expedition02/Meshes/SM_Cliff_Crown_02',
    '/Game/Environment/Expedition02/Meshes/SM_Cliff_Ridge_02',
    '/Game/Environment/Expedition02/Meshes/SM_Scree_02',
    '/Game/Environment/Expedition04/Meshes/SM_Rock04_Wedge',
]
subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
enabled = '--disable' not in sys.argv
rows = []
for path in paths:
    mesh = unreal.load_asset(path)
    assert mesh
    for slot in mesh.get_editor_property('static_materials'):
        material = slot.material_interface
        while isinstance(material, unreal.MaterialInstance):
            material = material.get_editor_property('parent')
        assert material.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_OPAQUE, path
    settings = subsystem.get_nanite_settings(mesh)
    before = settings.enabled
    settings.enabled = enabled
    subsystem.set_nanite_settings(mesh, settings, True)
    assert subsystem.get_nanite_settings(mesh).enabled == enabled
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh), path
    rows.append({'mesh': path, 'before_nanite': before, 'nanite': enabled,
                 'render_lod0_vertices': subsystem.get_number_verts(mesh, 0)})
(project/'Saved'/'ClusterRockOptimization.json').write_text(json.dumps(rows, indent=2))
unreal.log('CLUSTER_ROCK_OPTIMIZATION ' + json.dumps(rows))
