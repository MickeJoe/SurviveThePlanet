# Energy Extender

Source model: EnergyExtender.blend (Blender 5.2).
Export: SM_EnergyExtender.fbx, metre units converted to centimetres on Unreal import.
Mesh: /Game/Units/Buildings/EnergyExtender/SM_EnergyExtender
Building: /Game/BluePrints/Buildings/EnergyExtender/BP_EnergyExtender
Definition: /Game/Data/Buildings/DA_EnergyExtender
Icon: /Game/UI/Icons/Buildings/T_EnergyExtender

The existing building catalog supplies the ENERGY menu button. The blueprint is
available at game start and initially costs 10 Iron. Change cost and identity in
DA_EnergyExtender.

To tune the 2000 cm coverage radius, open BP_EnergyExtender and select
EnergyCoverage in the upper Components panel, then edit Coverage Radius.
Coverage is shown only while another building is being placed. Ghosts and
unfinished buildings do not supply coverage; completion refreshes the source
without polling. Placement uses the existing terrain, clearance and resource
rules and does not require existing energy coverage.

The new mesh has a ground-centred pivot, approximately 2.9 x 2.9 x 3.265 m bounds,
15332 triangles, five material slots, UVs and generated simple convex collision.
The coverage decal has no collision.

Creation was executed in the running Blender through its official native MCP
extension, in a separate scene that preserved the original scene. Unreal import,
blueprint/material setup and catalog registration used its native MCP.

Verification:
- Unreal Engine 5.8 editor build.
- SurviveThePlanet.Energy.CoverageVisualization
- SurviveThePlanet.Energy.ExtenderBuilding
- SurviveThePlanet.UI.BlueprintToolbar (including the extender button).
