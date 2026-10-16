# Shared production panel and Crude Workshop recipes

`WBP_BuildingPopup` is the shared building panel. It uses reusable authored UMG cards for material flow, recipe choices and drone slots. The flow still adapts to the building's real recipe, power connection, consumption/generation and available drone slots. Start/Pause controls are shown for production buildings; workshop recipe choices are hidden for other buildings. No Repeat control or missing-input banner is used.

Each input card shows global inventory / required amount and a small amber warning triangle when insufficient. These indicators refresh while the panel is open, including after inventory changes. Workshop output shows the actual rate at its assigned drone efficiency. Progress is actual completed work; cycle time is adjusted for staffing.

The production-enabled state belongs to `ABaseBuilding`. Existing factory, mining, water and energy production paths honor that state, and industrial effects follow the production gate. Existing buildings retain their automatic-running default; new Crude Workshops begin paused. Start may be requested with no inputs or no drones. Work waits without consuming partial ingredients and resumes automatically once staffing, power and supplies are available. Pause preserves cycle progress. Recipe changes discard unfinished work and do not consume ingredients, which are paid atomically at completion by the existing resource-production component.

The workshop has exactly four unlocked slots and uses the existing drone assignment picker and Construction working rates, matching the other staffed factories. Throughput scales with the sum of assigned efficiencies. No drone means no production. A fifth assignment is rejected by the existing slot rules.

Provisional recipes per 100% total drone efficiency:

| Recipe | Input | Output | Cycle |
| --- | --- | --- | --- |
| Concrete | 4 Stone | 1 Concrete | 120 seconds |
| Steel | 4 Iron Ore | 1 Steel | 180 seconds |
| Copper | 3 Copper Ore | 1 Copper metal | 150 seconds |

Circuit Boards are excluded from both the UI and the workshop recipe enum. The recipe costs and durations are editable workshop Blueprint defaults for balancing. The workshop retains its 10 iron construction cost and 6 electricity/minute configuration.

Source model and imports remain in `ContentSource/CrudeWorkshop` and `/Game/Units/Buildings/CrudeWorkshop`. The existing catalog definition remains `/Game/Data/Buildings/BlueprintCatalog/DA_CrudeWorkshop`. `Scripts/create_production_panel_ue.py` authors the panel/cards in the canonical UE 5.8 editor. `Scripts/test_production_panel_ue.py` tests real recipe/start/pause/drone picker handlers, repeated production and resource costs, recipe resets, staffing limits, starvation/replenishment, power loss/recovery, closing/reopening and reactor panel adaptation in PIE.
