# Compact resource HUD

Implemented in the canonical UE 5.8 project using WBP_ResourceDisplay.

## Inventory

45 inventory items plus energy. Four tabs: Raw Materials (9), Materials (11),
Components (15, including imported Circuit Boards), Advanced Goods (10).
ResourceCatalog is the shared authoritative metadata for HUD, trade icons,
and build-cost icon fallback. catalog.json is a generated readable snapshot.

Existing enum values are preserved. Iron represents Iron Ore; Copper represents
Copper Ore; Coal represents Carbon; ControlChip represents Circuit Boards.
CopperMetal is the separate refined Copper item. New entries are appended.
Initial stocks stay unchanged; missing Blueprint inventory entries start at zero.
Services and Knowledge are excluded from inventory because the blueprint catalog
does not define them as stockpiled items. This change adds resources and UI;
it does not implement the catalog's proposed production buildings or recipes.

## UI

Two compact rows balanced per category: 5+4, 6+5, 8+7 and 5+5.
Energy is a separate indicator alongside the grid. Icons are 40px, with amount
and production stacked beside them. Names appear only in card tooltips.
Category clicks filter the grid and reopen it if collapsed. The up/down chevron
collapses the grid and energy, leaving only tabs and the toggle. Inventory events
continue to update hidden cards. Full resource names and the Circuit Boards
import information are available in tooltips. Existing production-rate logic,
weather, clock and speed controls are reused. New items show zero production
until a producer exists. The layout scales within the space between the sidebars.
WBP_ObjectiveTracker's width is reduced from 410 to 310 to prevent its progress
counter from overlapping the resource grid.

## Assets

39 transparent 256x256 textures are imported under /Game/UI/Icons/Resources.
Existing ore, stone, concrete, carbon, Circuit Board and energy icons are reused.
Water, steel, polymer and connectors receive dedicated inventory icons, replacing
the previous building illustrations where applicable.
The source atlas and individual PNGs are in ContentSource/Resources.
Generated with the built-in imagegen tool: a transparent 8-column, 5-row atlas
of distinct isometric industrial sci-fi materials and components, ivory/gunmetal
objects with orange/cyan accents, no labels or background. Cell order is encoded
in the source atlas and the individual asset names. The separate PNGs are cropped
and padded atlas cells. WBP texture references include the resources in cooking.

## Verification

Unreal Engine 5.8 Development Editor build succeeds.
Scripts/create_resource_hud.py rebuilds the UMG tree from ResourceCatalog and
imports missing icons. Run in the Unreal editor's Python console.
Scripts/test_resource_hud.py runs in PIE and restores the stocks and UI state it
temporarily changes. It verifies all 46 icon/amount bindings, category counts,
unique grid positions, two-row capacity, collapse/reopen, and hidden updates.
Results: Saved/ResourceHudBuild.json and Saved/ResourceHudRuntimeTest.json.
Real clicks on category and chevron controls were also checked in PIE.
