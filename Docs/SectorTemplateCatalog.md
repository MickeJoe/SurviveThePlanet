# Sector template catalog

Canonical project: `C:\UE5\SurviveThePlanet 5.8`.

The existing ST_First is retained unchanged in layout. Six additional templates
bring the total to **seven**, using **12 unique footprints** and **36 baked
cluster variants** (three per footprint). No new meshes were imported.

| Folder under `/Game/WorldGeneration/SectorTemplates/` | Starting sector | Slots |
| --- | --- | --- |
| ST_First | Yes | 6 |
| ST_01_OpenBase | Yes | 6 |
| ST_02_BroadPassage | Yes | 6 |
| ST_03_Islands | No | 6 |
| ST_04_ShelteredPocket | No | 5 |
| ST_05_Winding | No | 6 |
| ST_06_ThreeClearings | No | 6 |

Each new folder contains an editable `L_SectorTemplate_*` level and its baked
`DA_SectorTemplate_*` asset. Open the level, edit the shape actors, select the
SectorTemplate actor, then **Bake Sector Template** and **Generate Preview**.
Save the level and data asset after edits. On the data asset, **Can Be Starting
Sector** explicitly opts a template into starting-sector selection. New assets
default to false; baking preserves this setting.

New footprints 07 Crescent, 08 Kidney, 09 Elbow, 10 Winding, 11 Horseshoe and
12 Fan are in `TerrainClusterShapes`. Their editable mesh compositions are in
`ClusterVariants/FullSector/Shape07` through `Shape12`. The original six shapes,
their eighteen variants and the ST_First authoring map are not replaced.

The default map remains `/Game/WorldGeneration/Maps/L_PlanetClusters`, with the
same 37 sectors and 4000 cm center-to-corner radius as PlanetLevel. Its Sector
Population actor references all seven assets in **Authored Sector Templates**.
A nonempty catalog supersedes the old single-template property. Ordinary sectors
may select any catalog entry, including start-eligible ones. Start selection
filters by the flag; an empty eligible set produces a diagnostic, never an unsafe
fallback. Selection is deterministic by population Seed and sector ID, independent
of catalog order or duplicate references. Different seeds can repeat a template.

All placements use baked slot transforms, not random runtime rotation/mirroring.
The new layouts are checked for hex containment and mutually nonoverlapping
footprints. The two new start layouts preserve a central 2100 x 1800 cm clearing.
This is visual authoring, not a replacement for gameplay resource placement or
navigation rules. Full gameplay balancing of all building pockets remains separate.

The same ISM batching, Nanite rock assets, detail culling and camera residency
remain in use. More template variety does not keep all sector meshes resident.

## Reproducibility and checks

- `Scripts/build_sector_catalog.py`: one-time asset recipe; refuses to overwrite
  existing authoring maps. `--templates-only` resumes after shapes were saved,
  but still refuses to replace authored template maps.
- `SurviveThePlanet.PlanetGeneration.AuthoredSectorSelection`: automated C++
  selection tests (1000 seeds, opt-in, empty catalog, order/duplicate stability).
- `Scripts/test_cluster_game_pie.py`: all 37 compositions, all seven templates,
  start eligibility, player startup and repeat-PIE transform determinism.
- `Scripts/test_cluster_residency.py`: eviction/reload, exact transforms across
  all sectors and preservation of exploration/resources. Run `--profile` for a
  traversal capture without expensive per-sector hash checks during profiling.
- Reports are written to `Saved/SectorCatalogPIE.json` and
  `Saved/ClusterResidencyTest.json`.

## Verified 2026-09-21

Editor Development build and the C++ selection automation passed. Two PIE runs
produced identical transforms: 37 sectors, all seven templates, 41,852 instances
when forced fully resident. All 37 sectors passed exact-composition comparison
after normal camera-driven eviction/reload; all 37 resource actors and discovery
state were preserved.

The final build also passed an uncooked `UnrealEditor -game -nullrhi` startup
smoke test (37 sectors, exit code 0). All six authoring maps were rebaked through
the actual editor bake API; slot order/transforms and eligibility flags were
unchanged, and every preview populated its expected slot count.

A separate traversal capture (no hash work during measured frames) visited every
sector at maximum camera zoom-out, with all sectors discovered. Across 4,440
frames: mean 16.673 ms (~60 FPS), p95 16.678 ms, worst 37.517 ms. GPU mean 7.737 ms.
This is an in-editor measurement on this machine, not a packaged-build guarantee;
the isolated worst frame remains a small hitch, not a sustained 15 FPS collapse.
Source: `Saved/Profiling/CSV/Profile(20260921_073155).csv`; the initial setup frame
is excluded by the existing summarizer.
