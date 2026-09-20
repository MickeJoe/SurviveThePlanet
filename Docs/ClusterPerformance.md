# Cluster rendering performance

Project: `C:\UE5\SurviveThePlanet 5.8`, Unreal Engine 5.8.

## Runtime policy

`ASectorPopulation` keeps all sector identities, seeds, discovery and resources.
Only the decorative ISM components have camera-dependent residency. Every 0.1 s,
the local player's actual projection frustum is tested against sector bounds with
a 2500 cm prefetch margin. Updates use real time, independent of the simulation's
pause/speed dilation. Discovered sectors outside that volume for 3 real seconds
release their components. At most 2 sectors regenerate per update, nearest first.
Missing camera data conservatively retains discovered sectors. No actor tick is
added per sector. The population manager throttles its work to 10 Hz; EndPlay
disables its tick.

`APlanetGeneratedSector` reconstructs the same seeded variant/slot transforms on
reload. Instances are populated before component registration, use static mobility,
and do not participate in collision/navigation. Instance groups are keyed by mesh,
material overrides and size tier. Placed mesh bounding-box maximum dimension sets
the tier: below 100 cm culls at 8000 cm and does not cast shadows; below 300 cm
culls at 14000 cm; larger structural meshes have no explicit distance cutoff.
Cull start is 75% of end; smooth fading requires a material that consumes the
instance fade value. Existing materials are not rewritten, so do not assume fading.

The shared mesh/material assets remain referenced by the variant library. This is
render-component residency, not full asset streaming. There are no merged proxy
meshes. Five dense opaque rocks use Nanite instead: Boulder, Cliff Crown, Cliff
Ridge and Scree from Expedition02, plus Rock04 Wedge. Original geometry and materials
are retained; Nanite provides adaptive geometric detail without unique merged
cluster assets. Other meshes remain unchanged. This is not a complete HLOD system.
The corresponding five materials also have their automatically enabled Nanite
usage flags saved, so standalone/cooked loading does not require editor repair.

## Controls and verification

On SectorPopulation, `bManageClusterResidency` disables/enables camera residency;
`bOptimizeClusterRendering` controls static mobility, size culling and tiny-detail
shadows for newly generated components. Disable both before PIE for the original
all-sectors rendering policy. These are diagnostic controls, not saved gameplay state.

- `Scripts/profile_cluster_sectors.py`: run through editor Python console; same
  camera path at 1/7/37 discovered sectors, 8 s warmup + 16 s samples each. Writes
  `Saved/ClusterPerformance.json` and UE CSV profiler captures. Temporarily disables
  editor background CPU throttling. No level is saved. `--baseline` disables the
  runtime performance policies for the test and restores them afterward.
- `Scripts/optimize_cluster_rocks.py`: enables Nanite on the five explicitly listed
  dense opaque rocks and saves only those assets. `--disable` restores their original
  disabled Nanite setting for A/B testing; rerun without arguments to restore the fix.
- `Scripts/audit_cluster_mesh_cost.py`: read-only inventory of geometry, materials,
  Nanite and LOD counts. Vertex counts reported by the static-mesh editor subsystem
  are render/fallback LOD counts, not Nanite source geometry counts.
- `Scripts/summarize_cluster_profile.py`: summarize the latest UE CSV or a supplied
  CSV path, including GPU, game/render thread, draw calls and graphics memory.
- `Scripts/test_cluster_residency.py`: PIE eviction/return test; verifies exact
  transform hash, preserved discovery/resources and a bounded resident subset.
- `Scripts/test_cluster_game_pie.py`: disables residency for exhaustive composition
  validation, independent of camera position.

Frame-time numbers in PIE include editor overhead and are not packaged-game FPS
guarantees. Compare identical viewport dimensions, quality settings and camera paths.
Do not accept an optimization merely because fewer instances are resident: GPU time,
frame-time tails, visual continuity and traversal hitches must also be evaluated.

## Measured 2026-09-20

Same-editor-session A/B, same viewport and camera path, 16 seconds sampled after
8 seconds warmup per discovery count. Control disables both runtime policies and
Nanite on the five rocks. Optimized uses all three changes. CSV frame/GPU timings
are authoritative; Python wall-clock samples are too coarse for precise percentiles.

| Discovered sectors | Control mean frame | Optimized mean frame |
| --- | ---: | ---: |
| 1 | 20.18 ms | 16.67 ms |
| 7 | 28.45 ms | 16.67 ms |
| 37 | 34.76 ms | 16.67 ms |

For 37 sectors, frame-time p95 improved from 88.45 to 16.68 ms; GPU median from
22.76 to 7.93 ms. Optimized is approximately 60 FPS at the current engine limit.
This is a measured local PIE scenario, not a promise for all hardware/resolutions.

Captures under `Saved/Profiling/CSV`:
- Control: `Profile(20260920_193506).csv`
- Optimized: `Profile(20260920_193233).csv`

The initial streaming-only after-capture at 14:35 was INVALID: a simulation-time
timer froze streaming while the game was paused. It must not be used as evidence.
The final code uses real-time throttling, and eviction/return tests verify that
discovery/resources persist and regenerated transforms are identical.

Full-map regression visited all 37 sectors at maximum zoom (6000 cm), preserving
every sector's original transform hash and all 37 resource actors. A separate clean
traversal capture omits hash checks while moving: `Profile(20260920_194148).csv`.
Use the summarizer's `--traversal` mode: exclude the first partial frame only, since
it includes the test setup/hash before CSV capture began. All traversal and GC
frames are retained. Isolated roughly 35–37 ms frames remain; no sustained 15 FPS
drop was observed on this route. This is not a claim of zero hitches.
