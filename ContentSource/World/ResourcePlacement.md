# Resource distribution pass

Add PlanetResourcePlacementComponent to the existing HexSectorGrid actor. Assign
DA_Rocky_Sparse or DA_Rocky_Rich from /Game/World/ResourceDistributions, choose a
PlacementSeed and call Generate after RebuildGrid. Generate never rebuilds the
layout or spawns runtime deposits. Both assets work with the same layout.

Rules specify resource and slot types, count/quantity ranges, HQ distance band
(cm), required template dressing tags, footprint radius and edge-to-edge spacing.
Guaranteed rules run first; optional shortages produce warnings. Required shortages
make Result.bSuccess false. A runtime spawner must check that flag before
consuming Result.Deposits; partial results are diagnostic only.

Show Debug plus Debug Rule Id displays cyan occupied, green compatible and red
rejected slots. Result.Slots explains each slot rejection for each rule. This is
an editor/development overlay, not a shipping HUD.

The entire slot envelope must avoid building pockets and drone corridors. Output
includes generation version, layout/placement seeds, resource type, quantity,
sector/slot IDs and resolved positions. Save the resolved result with the run.
No objective, economy or presentation assets are consulted.

Sampling is deterministic and greedy. Authored rule order resolves competition;
failures report constraints instead of silently dropping required deposits.
Clustering and economy-derived distributions are outside this pass.

Default template slots now avoid protected areas. Existing Blueprint/map template
overrides remain authored data: update their ResourceSlots if diagnostics reject
them. Maps are not modified automatically.

## Default-map runtime population

PlanetLevel now contains SectorPopulation, configured by
configure_default_population.py. It plans one finite Iron/Copper/Stone deposit
and at least 48 environment instances per sector, using a fixed seed. Four
formation anchors compose each sector from a landmark cliff, satellite rocks,
pebble fields and dense plant clusters; sparse ground cover fills remaining
gaps. The default 37-sector layout currently produces 37 deposits and 1789
environment instances. Terrain coverage was expanded to cover the outer
sectors. Protected building pockets and drone corridors are excluded from
decoration placement.

The surface uses the warm M_Terrain_05 material and the formation library uses
the same Expedition02/04/05/06 assets as PlanetLevel_RockPreview. Mesh roles are
stored as ordered ranges: landmarks first, then rocks, then plants/ground cover.

Only discovered/established sectors spawn their planned contents. The start
sector spawns immediately; OnSectorStateChanged reveals the others. Repeated
reveal and initialization calls do not duplicate contents. Existing authored
resource actors are preserved. Generated deposits use the existing resource
Blueprint classes and remain visible after sector discovery.

inspect_default_population.py checks the saved map without saving its temporary
actors: initial counts 1 deposit/48 instances, all-sector counts 37/1776, and
unchanged counts after repeated reveal. This is a headless editor-world check,
not a visual or full gameplay test.

Current scope: decoration meshes are visual and have no collision. This does
not create sculpted mountain terrain or add complete run save/load support.
The plan is regenerated from the seed on a new run; depleted quantities and
discovery state are not yet persisted by this system. Broader profile, loadout,
and vertical-slice issues remain separate work.
