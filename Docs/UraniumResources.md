# Uranium resources and industrial effects

Uranium surface deposits use `/Game/Uranium/BP_UraniumResource`, the existing finite resource, discovery and mining systems, and the compact textured bedrock mesh `SM_UraniumSurface`. The formation is approximately 250 × 215 cm and 23 cm high, with muted green mineral crust and faint pulsing mineral fluorescence. Sparse wind-driven dust is visual ambience.

`PlanetDefinition` exposes `UraniumDepositFrequency` (probability of one deposit per non-start sector), `bGuaranteeStartingUraniumDeposit`, and minimum/maximum reserves. Borealis is configured with 0.28 probability, a guaranteed start deposit, and 3,000–5,000 units per deposit. All other definitions, including the template, have zero probability and no start guarantee. Generation is deterministic from planet seed and sector ID. Placement checks surface, sector boundaries, terrain clearance and separation from ordinary deposits. Deposits spawn through existing sector discovery and do not respawn on repeated reveals.

`BP_UraniumMine` uses `AMiningMachine`, accepts only UraniumOre, produces 10 units/minute per 100%-efficient drone, and uses the refined colony-style uranium mine mesh while preserving construction costs and the catalog definition. Drone capacity and initial unlocked slots use the existing standard mine settings. Construction happens through the resource's existing mine action.

`UIndustrialEffectsComponent` pools Niagara components once at BeginPlay. Nuclear Reactor has two cooling plumes and pulsing status material; Uranium Processor has smaller roof ventilation plumes and a rotating roof fan. Effects and fan only run while operational. Resource particles stop when fog, mining replacement or depletion hides the deposit. Placement previews create no particles. Existing reactor generation, processor balance and building drone controls remain unchanged.

Native Blender source: `ContentSource/Uranium/UraniumSurface.blend`. Rebuild scripts: `Scripts/create_uranium_blender.py` (native Blender MCP), `Scripts/import_uranium_ue.py` (native Unreal editor toolset), `Scripts/tune_uranium_dust.py` (native Niagara MCP). Test scripts: `test_uranium_ue.py` and `test_uranium_aridus_ue.py`; reports and day/night captures are saved under `Saved/`. Uranium content is explicitly included in cooking.

## Refined uranium mine

`SM_UraniumMine_Refined` is a compact 390 � 350 cm installation with a covered shaft, reinforced headframe, light ivory service panels, orange safety trim, mechanical braces and a separate drill rotor. It reuses the geothermal Ivory, Orange, Steel, Dark and Lamp materials and the nuclear industry's shared status material. No new material graph is needed.

`BP_UraniumMine` owns `MineDrill` and the existing `IndustrialEffectsComponent` configured as `UraniumMine`. Rotation (150 degrees/second), a 6 cm axial feed stroke and pooled wind-driven dust use `AMiningMachine::IsProducingResource`, so they stop without a productive drone, power or reserves. Four warm unshadowed work lights follow actual sunlight/darkness and operational status. Preview actors create no particles or work lights. The existing four drone slots, one initially unlocked slot, production rate, construction costs and source reservation workflow are preserved.

Blender source: `ContentSource/UraniumMine/UraniumMine.blend`; generation/import scripts: `Scripts/create_uranium_mine_blender.py` and `Scripts/import_uranium_mine_ue.py`. `Scripts/test_uranium_mine_ue.py` checks all generated Borealis uranium positions, construction/reservations, preview suppression, real drone extraction, rotating/feeding drill, night lights, idle shutdown and terrain grounding. Results and captures: `Saved/UraniumMineReview.json`, `Saved/UraniumMineDay.png`, `Saved/UraniumMineNight.png`.


## Nuclear reactor operation

The reactor requires one to four working drones using the existing Construction work rate. Its editable recipe consumes one Fuel Rod and one Cooling Unit per 60 production seconds at 100 percent combined drone efficiency. Both costs are paid atomically by ResourceProductionComponent; the component creates no inventory electricity because CableNetworkManager supplies the grid using the reactor actual production getter. Missing either resource, unfinished construction, placement preview, or no staff produces zero electricity and disables steam. The building info shows both input icons/tooltips, cycle progress, live electricity output and four drone assignments. NS_NuclearSteam shares geothermal steam materials while using its own rising turbulent velocity, pulsed spawn rate and expanding particles. Verified in Saved/ReactorReview.json, with live steam frames Saved/ReactorSteamA.png and Saved/ReactorSteamB.png.
