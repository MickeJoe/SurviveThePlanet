# Cluster-generated gameplay level

`/Game/WorldGeneration/Maps/L_PlanetClusters` is based on `PlanetLevel`, preserving its gameplay actors, GameMode, surface, lighting, spawn points and hex grid. The original level remains unchanged. Both the game default map and editor startup map point to the new level.

The grid has 37 sectors (`GridRadius=3`), each with a 4000 cm radius. The existing `SectorPopulation` actor now optionally accepts **Authored Sector Template** and **Cluster Variant Library**. These replace only legacy environment dressing; the existing resource and discovery flow remains active. With those references unset, `PlanetLevel` continues using its original generation path.

For each grid sector, startup creates one `PlanetGeneratedSector` using `DA_SectorTemplate_First` and `DA_ClusterVariantLibrary_FullSector`. The seed is `(PopulationSeed XOR (SectorId * 7919)) & 0x7fffffff`, with unsigned arithmetic. It does not depend on actor iteration order or global random state. All six authoritative slot transforms are retained without additional random rotation or mirroring.

Runtime generation happens on the next tick after BeginPlay, using the established population initialization flow. Cluster roots align with the flat planet surface underneath each sector, rather than the debug grid's height offset. All sectors are generated; undiscovered ones remain hidden until the existing discovery event reveals them. Rendering uses per-sector mesh/material ISM groups, not individual runtime rock actors.

To change the world variation, select **Sector Population - Authored Clusters** and change **Seed** before Play. No manual Generate Preview step is required. The authoring maps remain separate from the gameplay map.

The cluster renderer remains visual-only: this change does not introduce rock collision, navigation blocking or footprint-based building restrictions. Resources retain the original map's placement rules, not a new cluster-aware distribution system.

Verification scripts: `create_cluster_game_level.py` checks the original map hash and sector count; `test_cluster_game_pie.py` checks 37 generated sectors, all six populated slots, unique per-sector seeds, no legacy scatter, discovery visibility, the player pawn and deterministic transforms across repeated PIE sessions. Results are saved under `Saved/ClusterGameLevel.json` and `Saved/ClusterGamePIE.json`.

## Editor binary started with -game

`CreateEditorOnlyDefaultSubobject` returns null when `GIsEditor` is false, even in a binary compiled with `WITH_EDITORONLY_DATA`. Both authoring actor constructors now check every optional editor component before configuring it. This prevents the startup crash in `PlanetSectorTemplateActor` and the equivalent latent crash in `PlanetTerrainClusterShapeActor` when class default objects are constructed. Marking an actor editor-only does not prevent its class constructor from running at startup.

`Scripts/test_cluster_game_standalone.py` verifies the default map in a bounded `UnrealEditor -game -nullrhi` run, including generation of all 37 sectors and successful process exit. This is a headless runtime smoke test, not a packaged-build or graphics benchmark.

Verified after the fix: editor startup loads `L_PlanetClusters`; PIE produces 50,565 instances across 37 sectors and 36 distinct variant combinations; a repeated PIE run matches every sector's world-space instance hash. The editor displayed a memory-pressure/video-memory-budget warning during the visual check. Memory/performance tuning remains separate work; it is not the fixed null-subobject crash.
