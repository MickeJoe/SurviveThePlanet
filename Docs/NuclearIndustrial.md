# Nuclear industrial art

Source: `ContentSource/NuclearIndustrial/NuclearIndustrial.blend`.
Models and icons: `/Game/Units/Buildings/NuclearIndustrial`.

`Scripts/create_nuclear_buildings_blender.py` runs through Blender's native MCP.
It reuses the geothermal palette, chamfered primitives, world-scale UVs and FBX
export conventions. Reactor foundation is 450 x 410 cm; processor foundation is
492 x 422 cm, preserving the previous horizontal bounds. Both mesh pivots are at
the ground plane. FBX import mirrors Blender Y.

`Scripts/import_nuclear_buildings_ue.py` runs inside the UE 5.8 editor. It assigns
the existing textured `/Game/Geothermal/M_GeoDetail_*` materials and updates only
the two building definitions' art references, icons and Blueprint mesh components.
Existing meshes remain available. Construction costs, power settings and drone
settings are preserved. The reactor Blueprint now enables BaseBuilding's existing
`ground_mesh_to_surface` terrain trace. The processor keeps its existing animated
Motion component attached to BuildingMesh, with the fan at UE (0, -40, 224) cm.

`Scripts/test_nuclear_industrial_ue.py` runs as editor startup Python, then start
PIE on Aridus through native MCP. It uses temporary, unsaved review fixtures,
measures mesh foundation height against terrain, checks reactor output and fan
motion, and saves day/night screenshots and `Saved/NuclearIndustrialReview.json`.
Do not save the review level. Stop PIE after inspection.
