# Remote Base

Implemented in the canonical UE 5.8 project.

- Infrastructure toolbar entry: `/Game/Data/Buildings/DA_RemoteBase`.
- Gameplay Blueprint: `/Game/Units/Buildings/RemoteBase/BP_RemoteBaseBuilding`, derived from native `ARemoteBase`.
- Reuses the imported Blender meshes, pulse materials, grid placement, construction jobs, blueprint ownership, build cost display and UMG toolbar.
- Fits a 5 x 5 metre footprint and provides 22 metre energy coverage.

## Construction rules

The whole base footprint must be inside one discovered sector. Undiscovered sectors are rejected. A placed Base Camp or Remote Base, including unfinished construction, occupies the sector's only base slot. Placement ghosts do not occupy slots.

The Remote Base opens ordinary construction when its construction job reaches 100%. Removing it returns the sector to Discovered if no other base occupies the sector. Existing buildings are retained; new ordinary construction is blocked. A later power outage stops operation and visual effects but does not revoke the completed sector's establishment.

Ordinary buildings, including mines, require established sectors. Power extenders may be placed in discovered sectors to bridge the network before the new base is completed. Legacy maps without a sector grid retain their existing placement behavior. Procedural resource placement using class templates remains independent of player construction rules.

## Power and cost

The base uses the existing EnergyConnectionComponent and selects the nearest eligible connection point without the extender's source-switch hysteresis. Player placement requires a routable cable to a source connected to the main base power grid. The selected parent and route are carried from preview to construction.

Additional connector cost is rounded up: `ceil(connection distance in metres * ConnectorsPerMeter)`. Distance uses the same building attachment points as extenders. The current data asset uses **1 connector/metre**, inherited from the extender setting, and no additional fixed material costs. Both settings can be changed in DA_RemoteBase. Costs update in the existing build-cost widget and are deducted through the existing placement transaction.

## Verification

- SurviveThePlanetEditor Win64 Development: build succeeded with UE 5.8.
- New SurviveThePlanet.Buildings.RemoteBase test: passed. Covers discovery, duplicate bases, construction completion, removal, footprint boundaries, camp conflict and catalog registration.
- Eleven existing tests passed for energy connections, routing, grid power, coverage, extenders, terrain, placement, resource distribution and UMG toolbar.
- PIE in L_PlanetClusters: clicking the new Infrastructure icon selects REMOTE_BASE and creates BP_RemoteBaseBuilding preview.
- PIE distance-cost check: moving the same preview from X=1000 cm to X=6000 cm increased its connector cost from 1 to 50, with the existing main-base connection point.

Integration can be reproduced with Scripts/register_remote_base_ue.py through the Unreal Python console. The icon is rendered through Blender native MCP with Scripts/render_remote_base_icon.py. These scripts should be run in the canonical project only.
