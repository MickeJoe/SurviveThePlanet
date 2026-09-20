# Expedition04: modular rock refinement

Canonical project: C:/UE5/SurviveThePlanet 5.8/SurviveThePlanet.uproject
Preview map: /Game/PlanetLevel_RockPreview

Four separate reusable static meshes: Pillar, Slab, Wedge, Foot. Ground-level pivots, metre-scale Blender geometry exported to UE centimetres. Broad clipped fracture faces, bevelled/chipped edges, bedding offsets and baked 1024 BaseColor/Normal textures. Sources: STP_ModularRock04.blend and stp_rocks04.py.

Two old cliff actors E02_Cliff_00 and E02_Cliff_01 are replaced by 34 individually editable instances, grouped in Expedition04/Formation_0 and Formation_1. Same modules form a tall outcrop and a low ridge. Each module can be rotated, scaled and reused. This is an assembly of actors, not a reusable Blueprint prefab yet.

Large modules have convex collision; foot rubble is decorative without collision. Existing terrain reservations are retained. No building-spacing rule is added. No UI or main PlanetLevel edits.

Backup map: /Game/Environment/Expedition04/PlanetLevel_Before04. Build manifest and two UE detail images: Saved/Expedition04. Studio render: ModularRock04_Studio.png. Keep the UE views as visual acceptance evidence; the studio image alone is insufficient.

Art iteration remains: avoid repeated pillar silhouettes, refine colour against existing rocks, and add further fracture variation only where visible at gameplay camera distance. Current mesh density is a modelling prototype (roughly 20k triangles per module); optimize small rubble for larger-scale scattering before expanding use throughout the map. Full reference-image quality is not yet achieved.

Scripts are rerunnable for this stage only: stp_ue04.py replaces actors tagged STP_Expedition04, imports missing assets, and saves the preview. It does not reimport already existing meshes automatically. It loads shared helpers from Expedition02/stp_ue02_build.py without running the old scene builder.

UV correction: unwrap broad fracture faces BEFORE subdivision/displacement; triangulate before normal bake and export. Reimport with imported normals/tangents and flip the normal texture green channel in UE. This resolved fragmented texture islands visible in the first UE import. Use stp_ue04_reimport.py after changed Blender exports.


Verification 2026-09-12: preview reopened and saved; 80 terrain components confirmed. PIE grid path test passed for approaches to 9/9 resource areas using 1x1 footprint. Gameplay04.png shows base/UI and verifies the level runs; the two new formations are outside that initial framing. Formation_A.png and Formation_B.png are the actual UE close-up visual checks after UV correction. Full manual drone collision testing and final reference-quality art approval remain outstanding.

