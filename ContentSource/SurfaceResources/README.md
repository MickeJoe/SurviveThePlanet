# Surface resource deposits

Editable Blender source: `SurfaceResources.blend`. Geometry is authored in centimetres and exported with the existing geothermal FBX/UV helper. Runtime meshes, source Blueprints, and material instances live in `/Game/Resources/SurfaceDeposits`.

| Resource | Planet availability | Chance per other sector | Starting sector |
| --- | --- | --- | --- |
| Silica | All current planets, including the default definition | 65% | One guaranteed |
| Rare Minerals | Nexaris | 20% | One guaranteed |
| Alien Material | Aridus | 14% | One guaranteed |

Occurrence is stored in `SurfaceDeposits` on `/Game/Data/Planet/DA_Aridus`, `DA_Nexaris`, `DA_Borealis`, and `DA_PlanetDefinition`. Each entry contains the existing deposit rule, resource Blueprint, sector probability, starting guarantee, and preferred starting offset. The rule supplies its id, resource, quantity range, radius, and spacing; sector occurrence is controlled by the planet settings. The shipped quantity range is 3,000–5,000 per deposit.

`SectorPopulation` uses the existing terrain-clearance search and discovery lifecycle. Planning uses the planet seed and resource/sector salts; undiscovered deposits do not spawn early, and repeated reveals do not spawn duplicates. Existing uranium generation uses the same generalized planner while retaining its previous seed and settings.

The existing catalog buildings Silica Extractor, Rare Mineral Mine, and Alien Material Harvester now derive from `MiningMachine`, each accepting only its matching source. They reuse resource reservations, construction, power, finite extraction, stock, and drone assignment. Each has four drone slots and the existing mine slot-unlock progression. The initial balance matches existing mines: 10 energy/min while staffed and 10 resource/min per drone at 100% efficiency. The blueprint document specifies the source, energy requirement, and output resource, but no numeric extraction rate.

All material instances share `M_DepositSurface` and the existing geothermal surface textures. Quartz has reduced surface texture/normal influence; the fossil core and fine channels have restrained animated emission. The resource HUD icons are Blender renders of the same deposits.

## Reproduction and verification

- Execute `Scripts/create_surface_resources_blender.py` through `Scripts/blender_native_request.py` and Blender's native MCP.
- Execute `Scripts/import_surface_resources_ue.py` through the native Unreal MCP-controlled Python console. Its result is saved to `Saved/SurfaceResourcesImport.json`.
- Execute `Scripts/test_surface_resources_ue.py` in the editor, setting `SURFACE_TEST_PLANET` to Aridus, Nexaris, or Borealis before starting PIE.
- Runtime reports and gameplay day/night captures are saved as `Saved/SurfaceResources_<Planet>.json` and `Saved/SurfaceResources_<Planet>_Day.png` / `_Night.png`.

The tests cover planet restrictions, starting guarantees, other-sector distribution, finite quantities, discovery idempotence, buildability of every generated source, four assigned mining drones, inventory gains, wrong-source rejection, and idle/depleted production. Borealis also checks its existing uranium generation and mine clearance.
