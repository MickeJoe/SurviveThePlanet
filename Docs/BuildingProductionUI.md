# Building production UI

The existing WBP_BuildingPopup shows resource icons flowing through a cycle timer to the output. Batch quantities are per cycle; continuous outputs are per minute. Recipe values come from the same typed building data assets and native defaults used by gameplay. Real cycle progress is read without advancing the simulation.

Hover a resource tile or icon to see its ResourceCatalog name. Missing inputs have orange quantities. Plug symbols and tooltips show connected, waiting, or disconnected power. Electricity requirements (-/min) and generation (+/min) appear separately. Required consumption is shown even when disconnected, with a tooltip. Water and mining output follows current rain/power/drone state.

DroneAssignmentSection and DroneSlotsBox keep the existing unlocked/locked slots and assignment buttons. Clicking an empty unlocked slot opens drone selection; clicking an assigned slot removes that assignment. The selector anchors next to the visible card.

Visuals live in WBP_BuildingPopup and WBP_FlowResourceCard. Scripts/create_building_flow_ui.py regenerates their layouts from ContentSource/UI/BuildingFlow icons. Production balance is unchanged.
