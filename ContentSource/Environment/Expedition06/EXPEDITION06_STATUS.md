# Expedition06 – Local vegetation
Target: /Game/PlanetLevel_RockPreview only. Backup: /Game/Environment/Expedition06/PlanetLevel_Before06.
New Blender sources: low branching shrub (351 polygons) and four-cap disc fungus (552 polygons). Separate reusable meshes, root-level pivots, vertex R wind weights from fixed root to movable tip. No skeletal animation needed for this gentle wind motion.
Adds 64 decorative plant actors in eight pockets around the two Expedition04 formations. Existing E02_Plant actors within 650 cm receive local material overrides with restrained pigment, height-based colour variation and fine mottling. Their geometry is unchanged. Existing opening bloom bases/petals are untouched, preserving their controls.
Wind WPO uses existing Expedition02 movement logic. Weather coupling remains outside this stage. No collision or additional grid restrictions. Main map and WBP UI unchanged.
Review images and build report: Saved/Expedition06. Paired A captures are three seconds apart for wind inspection. Check images and review.json before claiming completion.
The script unregisters its callback and disables keep-alive at completion. Reopen the editor normally afterward, without -ExecutePythonScript.

Visual review completed 2026-09-12: both areas inspected in UE captures; paired same-camera images inspected. New shrub/fungus materials retain vertex-weighted WPO. Fine wind motion is subtle at this distance; weather-driven behaviour is not tested or connected. Existing plant improvements are material overrides, not a geometry redesign. Full target-image art fidelity remains future work. Script exited cleanly.

