# Terrain cluster variant POC

**Update:** The sector now uses the complete six-shape library. See [Full-sector workflow](FullSectorClusters.md). The one-shape assets and results below describe the retained original POC.

Canonical project: `C:\UE5\SurviveThePlanet 5.8`.

## Existing systems retained

`PlanetTerrainClusterShape`, `PlanetTerrainClusterShapeActor`, `PlanetSectorTemplate` and its slots are unchanged in responsibility. `PlanetSectorTemplateActor` now offers preview controls. The generated renderer follows the existing `SectorPopulation` mesh/material instancing approach, without invoking its resource, terrain or exploration generation.

## Data and rendering

`PlanetTerrainClusterVariant.h/.cpp` under `Source/SurviveThePlanet/Gameplay/World/Authoring` defines:

- `UPlanetTerrainClusterVariant`: CompatibleShape plus editable mesh, relative transform and material override elements. Rotation/mirroring flags are future metadata, not applied by generation.
- `UPlanetTerrainClusterLibrary`: explicit asset catalog, with compatibility read from variant assets; no shape-name mappings.
- `APlanetGeneratedSector`: one actor holding mesh/material ISM groups. `FRandomStream(Seed)` chooses from compatible assets sorted by asset path. Catalog array order and duplicate entries do not change candidate order. Slot transforms remain authoritative.
- `APlanetTerrainClusterAuthoringActor`: inherits the existing footprint visualization, collects explicit MeshActors and bakes component transforms relative to the authoring frame, preserving material overrides.

Determinism assumes unchanged template slot order, catalog contents, asset paths and variant compositions. A different seed can choose the same variant; it does not guarantee a different result every time.

## Edit the three compositions

Open `/Game/WorldGeneration/ClusterVariants/Shape03/L_ClusterVariants_Shape03`.
Three authoring actors contain A (rock), B (mixed), C (vegetation), each associated with the existing `DA_TerrainShape_03_Long`. Move, rotate or scale their listed StaticMeshActors normally. Add new actors to the root's **Mesh Actors** array. Click **Bake Cluster Variant**, then save the asset. Attachment alone is not membership; the explicit array controls what is baked.

The POC uses existing Expedition02 boulders, scree, rosettes, trumpets and coral, plus Expedition06 disc fungus. No new Blender assets or merged cluster meshes were needed. The compositions contain 19, 32 and 42 editable elements respectively.

## Generate the sector preview

Open `/Game/WorldGeneration/SectorTemplates/ST_First/L_SectorTemplate_First`.
Select `SectorTemplate_First`. The assigned library is `DA_ClusterVariantLibrary_POC` under `ClusterVariants/Shape03`.

1. Set **Preview Seed** (0 = A, 3 = B, 10 = C for the saved POC catalog).
2. Click **Generate Preview**, also used to regenerate.
3. Click **Clear Preview** to remove the generated actor and its ISM groups. Authored shapes remain.

The generated actor is named `GENERATED_SectorPreview`, owned by and attached to the sector authoring actor, and marked editor-only. Inspect its **Selected Variant Ids** and **Diagnostics** to see selection results and missing-shape coverage. Bake the sector template separately after moving authored slots; Generate Preview reads the assigned baked template.

## Verification and limitations

`Scripts/build_cluster_variant_poc.py` created the assets using the actual C++ bake and preview methods. It checks repeated-seed numeric transforms, alternate seed selection of all three variants, preview removal and preservation of all six shape actors. Results are in `Saved/ClusterVariantPOC.json`.

The UE 5.8 editor target compiled and linked successfully. Reopening the saved sector restored all 32 mixed-variant instances (`Saved/ClusterReload.json`). Both authoring maps have editor-only ground and lighting for visual inspection; these are presentation helpers, not generated terrain. The sector radius remains 4000 cm, matching the existing default map. Native Unreal MCP viewport captures are in `Saved/ClusterPOC_Sector.png` and `Saved/ClusterPOC_Variants.png` (left to right: C, B, A).

Only shape 03 has variants as requested: one slot is populated; five remain visibly outlined and report missing compatible variants. There is no automatic substitution between different footprints. Rendering is visual-only with collision disabled. Optional forced-variant slot overrides, undo transactions for preview generation, detailed footprint mesh-bound validation, navigation, streaming and save-game integration are not implemented.

Next step: review these three compositions at the normal game camera distance, then author compatible variants for the remaining five shapes.
