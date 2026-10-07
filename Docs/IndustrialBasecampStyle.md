# Industrial buildings: Basecamp style

Polymer Plant and Connector Factory use a shared weathered colonial-industrial finish: warm ivory panels, orange trim, dark mechanical parts, grime in creases, irregular paint wear and subtle surface roughness.

Geometry adds roof panel divisions, reinforcement plates, hex fasteners, corner brackets and maintenance details. Polymer retains its tanks; Connector retains its assembly bay and robot arms.

Updated existing meshes:
- /Game/Units/Buildings/PolymerPlant/Meshes/SM_PolymerPlant
- /Game/Units/Buildings/ConnectorFactory/Meshes/SM_ConnectorFactory

New materials:
- /Game/Units/Buildings/PolymerPlant/Materials/M_PolymerPlant_Basecamp
- /Game/Units/Buildings/ConnectorFactory/Materials/M_ConnectorFactory_Basecamp

Both use a 2048px baked BaseColor atlas and 1024px AO, Roughness and Normal maps. Existing cyan materials and independently animated Polymer parts are preserved.

Building dimensions, ground pivots, Blueprint settings, costs, recipes and placement footprints are unchanged. Polymer is 463.5 x 350 x 249.51 cm; Connector is 470 x 350 x 250 cm.

Authoring:
- Scripts/restyle_industrial_buildings_blender.py implements the shared geometry and material update.
- Scripts/restyle_PolymerPlant_blender.py and Scripts/restyle_ConnectorFactory_blender.py invoke it for each model.
- Scripts/blender_style_request.py reuses the native Blender MCP client with a longer timeout for baking.
- Scripts/import_industrial_basecamp_style.py reimports only meshes, textures and icons and assigns the new materials. It does not reset Blueprint defaults or gameplay data.
- Original source files are retained in ContentSource/_StyleBackups/2026-10-04/.
- Updated editable Blender files and FBX exports remain in ContentSource/PolymerPlant/ and ContentSource/ConnectorFactory/.

Verification:
- Import verifies mesh bounds against their previous values.
- Saved/IndustrialBasecampStyleImport.json records the imported sizes and materials.
- Scripts/test_connector_factory_runtime.py passed after reimport.
- Scripts/test_polymer_runtime.py passed: placement, ground contact, construction, preview gating, production, recipe conservation, animated fans, light/output effects, HUD, starvation, power coverage and restart.
- Output/BasecampStyleComparison.png is an actual PIE screenshot. Polymer Plant in that screenshot was spawned only in the test session; the map was not changed to add another starting building.

