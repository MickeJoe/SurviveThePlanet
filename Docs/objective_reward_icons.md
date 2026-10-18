# Compact objective rewards

WBP_ObjectiveRewardStrip owns a 302-unit UMG WrapBox. WBP_ObjectiveRewardIcon is a 46x36 tile with an item/building/drone thumbnail, quantity and optional cyan BP badge. Short rewards use one row; the current uplink's ten rewards use two. No category headings or full text rows consume HUD space.

ObjectiveTrackerWidget reads all six configured reward types, using ResourceCatalog for resource art/names, BaseDrone for drone art/names and BuildingBlueprintSubsystem's existing blueprint lookup for building definitions. The lookup is now public so ownership and reward UI share the same ID rules. Pending rewards and next-objective unlocks each get their own tile.

ObjectiveRewardIconWidget shows the existing WBP_WorldHoverTooltip on actual mouse enter and removes it on leave/destruction. WorldHoverTooltipWidget now accepts display text with an optional wrap width; actor-name tooltips retain their previous behavior. Reward descriptions wrap at 260 units and stay inside the viewport. This avoids reliance on Slate's standard tooltip display during gameplay mouse capture.

TraderPanelWidget already uses the tracker's actual content bottom and therefore follows one/two reward rows without overlap. WBP_ObjectiveTracker retains natural content height and allows child hit testing.

Validation: UE 5.8 Development Editor build passed. Live PIE verified ten distinct icons, two rows, all ten real-cursor hover tooltips, four blueprint badges, single-row Cargo Bay progression and contract placement. Screenshots were visually reviewed with the shared UMG tooltip displayed.
