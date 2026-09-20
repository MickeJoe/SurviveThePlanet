# Full-sector cluster compositions

Current project: `C:\UE5\SurviveThePlanet 5.8`.

The existing `L_SectorTemplate_First` now uses `DA_ClusterVariantLibrary_FullSector`, under `/Game/WorldGeneration/ClusterVariants/FullSector`. All six existing footprints have three dense compositions. Shape geometry, sector slots and the 4000 cm sector radius are retained.

Select `SectorTemplate_First`, set **Preview Seed**, and click **Generate Preview**. The saved seed is 4 and produces 1355 instances across all six slots. **Clear Preview** removes only the generated actor and its instances. The original POC library remains available for comparison; it intentionally covers only shape 03 and should not be selected for a complete sector.

## Edit compositions

Each `FullSector/ShapeNN` folder contains `L_ClusterVariants_NN_Dense` and three variant DataAssets: A_Rock, B_Mixed and C_Vegetation. These retain the original actor-based editing and **Bake Cluster Variant** workflow. Edit the mesh actors listed by the authoring root, bake, save the variant, then regenerate the sector.

The new compositions use the existing RockKit large/medium rocks, Expedition02 ridges/crowns/boulders/scree/plants, Expedition04 wedges, Expedition05 debris, and Expedition06 shrubs/fungi. Rotated mesh XY bounds are fitted into each existing footprint. Large anchors, smaller connecting rocks, planted shoulders and ground debris extend along the full shape. Endpoints taper naturally; the open areas between footprints remain clear.

`Scripts/build_full_sector_clusters.py` is a one-time asset-authoring recipe, not runtime scattering. It refuses to overwrite an existing dense authoring map so that later manual edits are preserved. Runtime behavior still uses the existing variant compatibility references, seeded selection, slot transforms and mesh/material ISM grouping. No C++ changes or new Blender assets were required.

## Verification

`Scripts/verify_full_sector_clusters.py` checks full six-slot coverage and repeatable numeric transforms for seeds 0, 3, 4, 10 and 12345; clear/regenerate without removing any authored shape; and exact saved-preview reconstruction after reopening the level. Reports: `Saved/FullSectorClusters.json` and `Saved/FullSectorValidation.json`.

This remains a visual composition workflow. Collision, navigation and build placement restrictions are not implemented by the cluster renderer. Ground and lights in these maps are editor presentation helpers, not changes to the default gameplay map.
