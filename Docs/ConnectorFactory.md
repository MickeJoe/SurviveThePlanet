# Connector Plant

Buildable industry building in the canonical UE 5.8 project.

- Build menu: Industry -> Connector Plant, next to Polymer Plant.
- Recipe: 2 copper + 1 polymer -> 3 connectors every 20 game seconds.
- Electricity: 12 energy/minute while operational; uses the existing power grid.
- Construction cost: 30 iron, 10 concrete, 2 control chips, matching Polymer Plant.
- Production requires completed construction, grid coverage, available energy and both inputs. Previews never produce. Interrupted cycles pause and resume.
- HUD: connectors inventory and production rate; copper and polymer rates include this factory's consumption.

Assets:
- Blueprint: /Game/BluePrints/Buildings/ConnectorFactory/BP_ConnectorFactory, parent ConnectorPlant.
- Gameplay data: /Game/Data/Buildings/DA_ConnectorPlant, registered in DA_BuildingCatalog.
- Mesh/materials/textures: /Game/Units/Buildings/ConnectorFactory.
- Blender source and FBX: ContentSource/ConnectorFactory.

The 470 x 350 x 250 cm mesh matches Polymer Plant's scale and uses the Basecamp industrial style. Its 90-degree rotation reserves a 4 x 5 grid footprint. The shared BaseBuilding grounding helper positions the visible foundation against terrain automatically.

The earlier display-only starting factory has been removed from L_PlanetClusters. DA_ConnectorFactory and the original import/runtime scripts remain historical visual authoring resources; use Scripts/register_connector_plant_ue.py for gameplay integration.

Implementation reuses the catalog-driven UMG toolbar, generic placement and construction queue, atomic resource-cost payment, and existing energy grid. ResourceProductionComponent shares material conversion cycles with Polymer Plant.

Verification:
- UE 5.8 Development Editor C++ build succeeded.
- Scripts/test_connector_plant_runtime.py: live Industry click binding, recipe, HUD bindings, placement footprint, construction cost, terrain contact, preview/construction gates, output and input conservation, energy consumption, HUD rate, starvation, coverage and restart.
- Saved/ConnectorPlantRuntimeTest.json records the successful result.
- Scripts/test_polymer_runtime.py passed after moving conversion logic into the shared component; Saved/PolymerRuntimeTest.json records the regression result.


Production visuals: separately exported fan blades rotate above the roof, three connector assemblies travel along the working conveyor, and orange emissive welding sparks and a process light pulse in the open bay. All movement and bursts pause when production stops. The original body mesh no longer contains the animated blades or belt products.

Animation authoring: Scripts/animate_connector_blender.py uses native Blender MCP; Scripts/import_connector_animation_ue.py imports the parts and configures the existing Blueprint. The runtime test verifies movement, visible welding bursts and paused animation during starvation.
