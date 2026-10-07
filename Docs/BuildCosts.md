# Build costs

Placement uses one compact WBP_BuildCost row above the build toolbar: building icon, total for each required resource and a green check or red cross. Missing amounts are red. The same WBP is used in button tooltips with the existing tooltip text above the cost row.

The controller GetBuildCosts function is authoritative for generic placement preview, payment and the UI. It aggregates duplicate resource entries and excludes nonpositive entries. Mines retain their existing resource-specific construction costs through the active preview.

Only EnergyExtender adds distance costs: ceil(horizontal attachment-point distance in metres * ConnectorsPerMeter). The source comes from the existing connection preview; the final connection copies that same source. ConnectorsPerMeter is editable on the building data asset (initial value 1). Construction costs and connector costs for the same resource are combined. Clicking recalculates at the snapped clicked cell before checking affordability and payment.

## Widget authoring

Scripts/create_build_cost_widget.py runs in the canonical UE 5.8 editor through native MCP Python execution. It creates /Game/UI/WBP_BuildCost, binds the native BuildCostWidget parent and reuses all eleven resource textures from WBP_ResourceDisplay. Run before Play. Runtime presentation is UMG; no Slate UI is constructed.

## Verification

UE 5.8 C++ compilation succeeded. /Game/UI/WBP_BuildCost has been created, compiled and saved with all eleven HUD resource icons. PIE verification in Scripts/test_build_cost_ui.py checks the actual WBP in the toolbar, tooltip instances, extender totals, battery placement totals and hiding after cancel. Results are saved in Saved/BuildCostUITest.json. An actual PIE screenshot is saved under Saved/Screenshots/WindowsEditor. Building icons reuse the toolbar's configured or authored WBP artwork through ApplyIcon.
