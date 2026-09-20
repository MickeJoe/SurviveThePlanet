# Terrain cluster and sector-template authoring

## First authored sector

Open `/Game/WorldGeneration/SectorTemplates/ST_First/L_SectorTemplate_First` in the canonical project `C:\UE5\SurviveThePlanet 5.8`.
The six saved shapes follow the later gameplay reference: irregular bands around a central clearing, with tapered ends and varied widths. They mark future rock/vegetation coverage only. The sector radius is read from the `HexSectorGrid` instance in `/Game/PlanetLevel` (currently 4000 cm).

`Scripts/create_first_sector.py` refreshes this existing authored layout through `correct_first_sector.py`. It backs up the previous WorldGeneration assets under `Saved/AuthoringBackups` before updating them and preserves actor identities. Running it intentionally resets the six footprints and their transforms to this layout. The previous generic preset layout is no longer used by that entry point.

`Saved/FirstSectorPlan.png` is a plan diagram rendered from the reloaded saved polygon coordinates, not an Unreal viewport screenshot; the diagram's fill is for readability. `Saved/FirstSectorVerification.json` contains those coordinates. Editor visualization remains outlines and labels.

This proof of concept stores only low-detail 2D footprints and authored transforms. It does not generate terrain, meshes, resources, vegetation, or runtime sectors.

## Create the first six shape assets

1. In the Content Browser, create a folder such as `Content/WorldGeneration/TerrainClusterShapes`.
2. Right-click, choose **Miscellaneous > Data Asset**, and select `PlanetTerrainClusterShape`.
3. Name the asset, for example `DA_TerrainShape_01_Tapered`.
4. Open it, choose `Shape1_Tapered` in **Preset**, and click **Apply Selected Preset**.
5. Repeat for presets 2 through 6. The presets loosely match the numbered footprints in the original sketch.
6. Edit `ShapeId` and the `Points` array as desired. Points are local XY coordinates in centimeters and the polygon closes automatically.

Keep polygons low-detail (roughly 6-20 points) and order points around the perimeter without crossing edges.

## Create and author a sector map

1. Create a normal empty level, for example `Content/WorldGeneration/SectorTemplates/ST_Test_01/L_SectorTemplate_Test`.
2. Place one `PlanetSectorTemplateActor` at location `(0, 0, 0)`. Keep `SectorRadius` at `4000 cm` to match the hex sectors in `PlanetLevel`; the yellow hexagon is the authored boundary and the red/green arrows mark the local origin and axes.
3. Place six `PlanetTerrainClusterShapeActor` instances.
4. On every shape actor, assign its `Shape` asset and set `OwnerSector` to the placed sector actor. `OwnerSector` is the explicit association used by baking; actor names are ignored.
5. Optionally give each instance a `SlotId`, such as `Cluster_01`. When present, this is also its viewport label.
6. Use the normal move, rotate, and scale tools. The cyan closed spline is the transformed footprint. Arrange the six instances approximately like the supplied sketch, leaving open corridors between them.

Edits to the source shape asset update placed instances. These actors and their visualization components are editor-only and are not present in a packaged game.

## Bake the template

1. Create another **Miscellaneous > Data Asset**, this time selecting `PlanetSectorTemplate`; name it `DA_SectorTemplate_Test`.
2. Select the `PlanetSectorTemplateActor` and assign that asset to `TargetTemplate`.
3. Click **Validate Sector Template**. Warnings are written to the Output Log for missing shapes, polygons with fewer than three points, and footprints extending beyond the sector boundary. A shape actor also has **Validate Shape Actor** for checking an individual instance.
4. Click **Bake Sector Template**.
5. Save the target asset. Its `ClusterSlots` contain the shape references, optional slot IDs, and transforms relative to the sector actor. `SectorRadius` and the derived `6928.203 × 8000 cm` bounding size are copied too.

Running bake again replaces the target asset's slots with the current authored layout. Shapes whose `OwnerSector` points elsewhere are ignored, so multiple sector authoring frames can coexist in a level.

## POC limitations

- The viewport uses closed spline outlines and labels; it does not triangulate or fill the polygons.
- Overlap is allowed and is not checked by this POC.
- Validation reports to the Output Log and does not block baking.
- Presets are starting points, not exact traced geometry.
- Runtime generation and placement are intentionally not implemented.
