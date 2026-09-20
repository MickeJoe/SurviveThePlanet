# Expedition07 – Lighting and composition review
Preview only: /Game/PlanetLevel_RockPreview. Backup /Game/Environment/Expedition07/PlanetLevel_Before07.
Sun_KeyLight: intensity 3.2 -> 3.456; pitch -50 -> -43 degrees, yaw -35 retained; source angle 2 degrees. Warm colour nearly unchanged. E02_SoftFill: intensity 1.6 -> 2.5, slightly cooler/neutral fill to make shaded faces readable. No postprocess, UI, gameplay camera, resource or building changes.
Fixed BEFORE/AFTER captures use identical transforms/FOV per pair: Saved/Expedition07/Overview_Before.png, Overview_After.png, Detail_Before.png, Detail_After.png. Plants animate, so their exact poses differ. These are editor scene captures, not gameplay screenshots with buildings.
Composition camera E07_CompositionCamera is saved for repeatable review; this does not replace the player camera. Base clearance check covers newly added E05/E06 decorations within 800 cm of (600,150). None required relocation; composition and passages were maintained, not rebuilt. No added collision or grid blockers.
Both detail images and after overview inspected. More elaborate art, stronger large-scale ground variation and remaining low-detail rock meshes are still outside reference-image final quality.
Script unregisters its callback and ends keep-alive after saving. Rerun uses fixed target light values.

PIE gameplay capture saved as Saved/Expedition07/Gameplay07.png. The capture shows the unchanged WBP layout, base clearance and readable scene lighting. Light pass is complete for this stage; no building spacing rule added.

