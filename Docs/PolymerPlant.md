# Polymer Plant

Compact white/orange industrial building created in Blender and integrated with the existing catalog and UMG build toolbar.

## Use in game

Select **Industry**, then the **Polymer Plant** icon. Place it on a valid free area using the normal building placement tool. The existing construction-drone queue builds it, and the existing electricity coverage system powers it.

Construction costs: 30 iron, 10 concrete, 2 control chips.

Every 20 seconds of active production: consume 2 coal and 1 water, produce 3 polymer. Electricity demand: 12/minute. Balance values are editable in `DA_PolymerPlant`.

The model occupies 5 × 4 of the project's 100 cm grid cells. Its mesh is approximately 4.7 × 3.6 metres including feet and the short output shelf.

## Assets and implementation

- Blender source: `ContentSource/PolymerPlant/PolymerPlant.blend`
- FBX meshes and rendered icon: `ContentSource/PolymerPlant/`
- Unreal Blueprint: `/Game/BluePrints/Buildings/PolymerPlant/BP_PolymerPlant`
- Building data: `/Game/Data/Buildings/DA_PolymerPlant`
- Meshes and materials: `/Game/Units/Buildings/PolymerPlant/`
- C++ production and runtime animation: `Gameplay/Buildings/PolymerPlant.h/.cpp`
- UMG inventory amount/rate: `/Game/UI/WBP_ResourceDisplay`

Two independently rotating roof fans, a pulsing cyan process light and a moving output block run only during active production. Missing raw materials, incomplete construction, placement previews and missing electricity stop production and effects. Paused production retains partial cycle progress.

The mesh has a ground-level pivot. PolymerPlant traces the actual terrain beneath each placement and seats the visible mesh 0.5 cm into the surface. The grid actor stays at the shared placement reference height; its height does not represent the visible terrain. This also refreshes after a preview or building moves. Fans, output and the process light attach to BuildingMesh and follow its offset.

The refined Blender model adds rounded corner columns, domed vessels, curved process pipes, panel fasteners, fan cages and conveyor rollers. A baked 2048px color atlas and ambient-occlusion texture provide the surface finish. The cyan process light is reduced to avoid washing out the panels.

## Authoring and verification

`Scripts/create_polymer_plant_blender.py` runs through the project's native Blender MCP client. It creates a new scene without deleting other scenes. `Scripts/import_polymer_plant_ue.py` runs inside the canonical UE 5.8 editor through native MCP; it imports assets and registers the building.

`Scripts/test_polymer_runtime.py` checks the catalog, controller-created placement preview, mesh components, visible ground contact within 1 cm, actual electricity coverage, recipe conservation, animation, light/output effects, HUD values, starvation, disconnection and restart in a temporary PIE session. `Scripts/test_polymer_button.py` verifies the actual UMG click binding selects PolymerPlant. Tests change only the temporary PIE world and do not save test buildings or inventory to maps.

UE 5.8 Development Editor build and Git whitespace verification passed.

