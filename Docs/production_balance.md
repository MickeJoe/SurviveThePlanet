# Production and player speeds

Material production runs at 3x its authored baseline. One common factor is defined in Source/SurviveThePlanet/Gameplay/Buildings/ProductionBalance.h.
Recipe assets and actor defaults retain their base values; production code and UI convert them to effective durations/rates once.

Applies to all mining resources, water collection, Concrete Plant, Steelworks, Polymer Plant, Connector Plant, Crude Workshop, Uranium Processor and Thermal Processing recipes.
Power generation, reactor fuel cycles and construction times retain their previous balance.
Recipe input/output quantities are unchanged. Faster cycles consume the same recipe more often.

Crude Workshop cycles at 100% drone efficiency: Concrete 20 seconds, Steel 30 seconds, Copper 25 seconds. Assigned drone efficiency continues to scale duration.

Player controls: Pause, 1x, 2x, 4x, 10x. Authored UMG widget names remain compatible with existing bindings; Speed15Button now selects 2x, Speed2Button selects 4x, Speed3Button selects 10x. Developer cheats remain available.

Validation: UE 5.8 Development Editor build passed; live PIE test passed all player speeds, pause/resume, actual workshop production for all three recipes, exact input consumption, four-drone limit, starvation recovery, power recovery, and shared reactor controls. Native basic mining rate getters verified at 30/min per 100% drone (previously 10/min).

Start balance: 3 starting drones (2 construction + 1 mining) on Nexaris, Aridus and Borealis; confidence decay 2.5 points per game hour; uplink grants 40 confidence points, 1 Explorer Drone and 1 additional Construction Drone. Existing circuit board, blueprint and progression rewards remain. Confidence remains capped at 100. Live PIE verified 2 -> 3 construction drones, 0 -> 1 explorer, confidence 20 -> 60 and no duplicate rewards.

Cargo Bay costs: 20 Concrete, 10 Steel, 2 Circuit Boards. Contract UI, offer retrieval, selection and confirmation require a completed Cargo Bay. A preview or unfinished Cargo Bay does not unlock contracts. Uplink alone does not unlock contracts. Live PIE verified the gate before uplink, after uplink, during Cargo Bay construction and after completion; +40 confidence reward and effective actor/catalog construction costs also passed.

The uplink objective now unlocks mission_build_cargo_bay through its unlock_objective reward. Cargo Bay activates automatically, requires one completed CargoBay and grants +1 mission progress once. Existing confidence, blueprint, resource and drone rewards are unchanged.
Drone picker groups by type: the three starting drones show as RX-45 Falcon x2 and MS-35 Beaver x1. Live PIE assigned all three through actual picker buttons to the workshop. Picker title and count tooltips now explain type grouping and availability. No assignment bug was reproduced with three idle starting drones.

Energy demand scales by productive assigned drone count: 1 = 100%, 2 = 180%, 3 = 240%, 4 = 280%. Mixed drone types count equally if they can perform the building work type. Zero drones retains the previous base demand. Implemented once in ABaseBuilding::GetEnergyConsumptionPerMinute; grid budgeting and production UI use the same getter. Existing pause, starvation and derived building idle-demand rules are preserved. Demand stays independent of power availability to avoid blackout/resume oscillation. Native tests and live PIE verified all four levels, removal, mixed starting drone types, UI values and grid totals.
