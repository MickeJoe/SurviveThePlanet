# Energy connection implementation
Component: UEnergyConnectionComponent. Existing placement state is authoritative.
Preview is owned by the extender ghost and destroyed with it on cancel/tool switch.
Placed extender receives its own connection and retains the parent's source selected in the preview.
Nearest source uses the existing coverage registry, excludes ghosts/incomplete sources and applies 150cm hysteresis.
Placed connections keep their parent while it remains eligible; invalid parent events trigger reconnection excluding descendants, preventing cycles.

Visuals:
- Shared instanced pole mesh, straight double cable mesh deformed into four spline pieces per span, endpoint terminals.
- Approximately 600cm spacing, equal intervals along direct source-to-extender line.
- Terrain traces ground the poles. No automatic detours around rocks in this version.
- Preview green/red follows existing placement validity; final meshes retain authored materials.
- Roof terminal traces the actual roof at its mounting position, avoiding floating at satellite dish height. Power_Ivory has configurable MinimumVisibility=0.08 emissive fill to retain readability under dark lighting.
- No collision/navigation/occupancy/cost/power eligibility changes.
- Sources and configuration use existing coverage/component architecture; no world search or dedicated component Tick.
- Route rebuild only when snapped owner position or registry revision changes.
- Instance/component pools reused; capped at 64 poles / 260 spline pieces per connection.
- Animation and VFX remain the previously proposed next step.

Validation:
- Editor C++ build succeeded.
- Added SurviveThePlanet.Energy.ConnectionPreview regression test (nearest/hysteresis, incomplete source, outside coverage, stationary updates, pool reuse, collision/navigation, placed route).
- The three energy automation tests passed (ConnectionPreview, ExtenderBuilding, CoverageVisualization). Initial editor startup was blocked by Windows Code Integrity; the subsequent final module loaded normally. No security policies were changed.

Final visual validation in PIE: four grounded poles and hanging double cables rendered between Base Camp and extender. Roof terminal mounting height verified at Z=291.76cm, compared with the previous dish-bounds height Z=466.55cm. Temporary inspection actors were removed; no test fixtures were saved in the map.
