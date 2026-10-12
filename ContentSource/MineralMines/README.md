# Silica Extractor and Rare Mineral Mine

The two existing mining Blueprints and data assets now use detailed colony-style Blender models: painted panelled enclosures, metal headframes, hydraulic sleeves and pistons, pipe flanges, inspection panels, vents, shaft collars and foundation anchors. Silica uses a covered auger and cyclone dust collector; Rare Minerals uses a taller reinforced core drill with a sorting cabinet and sample chute.

Source: `MineralMines.blend`. FBX bodies, drills, fans and transparent icon renders are in this directory. Runtime art assets are in `/Game/Resources/MineralMines`; the existing Blueprints remain in `/Game/Units/Buildings/BlueprintCatalog/SilicaExtractor` and `/RareMineralMine`.

The models reuse the existing geothermal detail materials and textures, plus the shared operational status material. `IndustrialEffectsComponent` adds a `SurfaceMine` mode using the existing mining dust system, drill rotation and axial feed, operating fan, and night worklights. Original Blueprint EventGraph animation nodes were removed so they cannot continue rotating the fan while idle. Placement previews create no operating VFX.

Construction costs, ownership/unlock settings, energy demand, resource compatibility, output rates and drone slots are preserved. Each mine retains four maximum slots, one initially unlocked slot, 10 resource/min per drone at 100% efficiency, and the existing power requirement.

Reproduce with `Scripts/create_mineral_mines_blender.py` through Blender's native MCP, then `Scripts/import_mineral_mines_ue.py` through Unreal's native MCP-controlled Python console. `Scripts/test_mineral_mines_ue.py` registers a PIE test on Nexaris. Import and runtime results are in `Saved/MineralMinesImport.json` and `Saved/MineralMinesReview.json`; game-camera daylight and nighttime captures are `Saved/MineralMinesDay.png` and `Saved/MineralMinesNight.png`.
