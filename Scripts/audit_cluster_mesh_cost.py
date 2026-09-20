"""Read-only mesh-cost inventory for the authored cluster library."""
import json
from pathlib import Path
import unreal

library = unreal.load_asset('/Game/WorldGeneration/ClusterVariants/FullSector/DA_ClusterVariantLibrary_FullSector')
subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
meshes = {e.mesh.get_path_name(): e.mesh for v in library.variants for e in v.elements if e.mesh}
rows = []
for name, mesh in sorted(meshes.items()):
    row = {'mesh': name}
    for label, query in (
        ('nanite', lambda: mesh.get_editor_property('nanite_settings').enabled),
        ('lod_count', lambda: subsystem.get_lod_count(mesh)),
        ('lod0_vertices', lambda: subsystem.get_number_verts(mesh, 0)),
        ('materials', lambda: len(mesh.get_editor_property('static_materials'))),
    ):
        try:
            row[label] = query()
        except Exception as error:
            row[label] = str(error)
    rows.append(row)
path = Path(unreal.Paths.project_dir())/'Saved'/'ClusterMeshCost.json'
path.write_text(json.dumps(rows, indent=2))
unreal.log('CLUSTER_MESH_AUDIT ' + str(path))
