# Coal mining

Implemented in the canonical UE 5.8 project.

## Assets and building flow

Blender source: `ContentSource/Coal/Coal.blend`. Static meshes, FBX exports and rendered UI icons are in that directory. UE assets live in `/Game/Units/Buildings/Coal`, with `/Game/BluePrints/CoalMine/BP_CoalMine`, `/Game/BluePrints/Resources/BP_CoalSource` and `/Game/Data/ResourceBuildings/DA_CoalMine`.

The coal mine inherits MiningMachine and uses the existing generic mining tool, source-specific mine selection, placement, construction, reservation, costs and drone assignment. Balance matches copper: 10 iron + 1 control chip, 4 drone slots with 1 unlocked initially, 10 energy/min consumption, and nominal 10 coal/min multiplied by assigned mining efficiency.

Starting sectors contain stone, copper and coal. Other sectors retain 1–3 unique deposits selected from stone/copper/coal/iron. Quantities remain 3000–5000 and the existing spacing, terrain-clearance and mineable-placement rules apply. Coal is appended to the resource enum to preserve existing resource IDs.

CoalMiningMachine animates its cutter, conveyor coal and instanced coal chips only while construction is complete, a usable source remains, mining drones provide efficiency and shared operational power rules permit extraction. Effects stop when production stops. Blender also contains a looping cutter preview action.

The UMG resource bar displays coal inventory and coal/min alongside the existing resources.

## Verification

UE 5.8 Development Editor build succeeded. Coal placement/regression checks passed for coal, stone, copper and iron. PIE validation checked 37 sectors, 69 deposits and their matching mine classes. Production test transferred 19 coal into inventory with equal source depletion, observed cutter movement, chip visibility, bound UMG amount, and stoppage without drones, without power, during construction and after depletion.

The original implementation encountered an incomplete shared energy system. The later visual revision was tested against the updated shared coverage/power rules with temporary Base Camp and grid-manager actors in PIE; no mine identity or production rule was overridden. Those test actors are removed by stopping PIE.

Scripts: create_coal_blender.py runs through the native Blender MCP request helper; import_coal_ue.py runs in the UE editor's Python console. test_coal_runtime.py requires PIE; test_coal_production.py requires its runtime fixtures and never saves its temporary identity override. Saved/Coal*.json contain detailed reports.

An additional attempt to run the dedicated-editor multi-seed test as a null-RHI commandlet could not validate terrain traces for any resource type. This harness did not pass; it is not included in the successful PIE verification above.


## Visual revision

The mine was rebuilt with segmented armor, gasket seams, hydraulic rods and hoses, grille fins, fasteners, wear patches, a larger staggered tooth cutter, a roller conveyor and broken coal around the cutting face. Footprint remains compact (~2.95 x 2.12 m). Source deposit assets and existing gameplay rules were retained.

FBX conversion mirrors Blender's Y coordinate: the cutter belongs at (-25, +61, 35) cm, belt coal at y=-20, and debris/dust travel toward positive Y. Previously the moving parts were placed behind the machine. Native construction and BeginPlay now bind meshes on spawned actors, with native asset references retained for cooking. Cutter teeth, belt coal and chips are more readable at gameplay distance. Soft dust uses a translucent ISM material and disables Nanite for its sphere components. VFX stop when the mine is no longer producing.

The Development Editor build and post-restart PIE visual test passed: inventory increased, cutter rotation and belt transforms changed, cutter/coal/chip/dust meshes were loaded, dust/chips were visible during operation and stopped without a drone. A 24-frame GIF was captured from the actual UE runtime. The screenshot/GIF are close views of the real mesh, not generated concept images.

Reproduction: Scripts/create_coal_blender.py then finish_coal_blender_materials.py via native Blender MCP. Scripts/import_coal_visuals.py in the UE Python console reimports the revised mine parts/icon and runs the verified material finishing scripts; the deposit/icon and coal-rock material edits are retained.


## Faction colors and size

The mine's BuildingMesh default scale is now uniformly 1.25. Its cutter, conveyor coal and operating VFX inherit that scale, while native local animation positions remain unchanged. The Blueprint was saved and reloaded to verify persistence. Approximate visible dimensions are now 3.69 x 2.65 x 1.57 m; the derived grid footprint is 4 x 3 cells. PIE placement validation passed for 69 deposits including 17 coal mines, and all five visual component world scales were verified at 1.25.

Coal mine/cutter mesh slots now reference the existing EnergyExtender Ivory, Orange, Graphite, Steel and Cyan material assets directly. This keeps their faction colors consistent. The Blender scene has a visual scale root of 1.25 and matching faction colors; its source FBX geometry remains at the base size because Unreal applies the component scale. The mine UI icon was rendered and reimported.

Scripts/apply_coal_size_palette.py applies the shared materials and saved Blueprint scale and is called at the end of import_coal_visuals.py. Scripts/apply_coal_faction_blender.py runs through the native Blender MCP after creating/finishing the base model.
