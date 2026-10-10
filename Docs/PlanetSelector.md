# Planet selector prototype

Canonical project: `C:/UE5/SurviveThePlanet 5.8`, Unreal Engine 5.8.

Play the default map `L_PlanetSelection`. Click a planet (or its name) to inspect its climate, then **LAND & START COLONY**. The existing colony HUD starts paused; choose a speed to run the simulation. Each landing starts a new session.

`WBP_PlanetSelector` is an editable UMG Widget Blueprint. C++ supplies selection, climate details and level travel. Three `UPlanetDefinition` asset instances clone the existing definition so economy, merchant visits and mission tuning are preserved. Three maps instantiate `L_PlanetClusters`; each references its own definition and population seed. The original maps and definition are preserved.

| Planet | Climate | Rotation | Mean temperature | Wet probability |
|---|---|---|---|---|
| Nexaris | Temperate | 24 h | 18 C | 55% |
| Aridus | Arid | 30 h | 38 C | 8% |
| Borealis | Cool maritime | 36 h | 6 C | 80% |

Weather remains deterministic by seed, transitions smoothly and permits genuinely dry periods. Rain suppresses direct sunshine. Values remain in mm/h, m/s and percent for existing gameplay consumers. Solar elevation follows latitude, solar declination and rotation period. The existing HUD clock drives light/phase changes and respects pause, trading pause and simulation speed. The HUD rolls over at each planet's rotation period (24/30/36 hours). The selection screen shows daylight (sun above horizon), combined dawn/dusk (sun between 0 and -6 degrees), and dark night (below -6 degrees) at the landing latitude and season. These periods total one rotation. Building lights respond to natural brightness from solar elevation and weather attenuation, including overcast daylight.

Presentation reuses the level's directional light, skylight and fog, adds rain streaks and windborne motes pooled around the camera pawn, and supplies a directional wind source for compatible vegetation. Rain density tracks rainfall and streak angle follows wind. Sun intensity/color, ambient illumination and fog vary with weather and solar elevation.

This is a climate-profile prototype, not a complete atmospheric simulation: no pressure, orbital seasons, atmospheric chemistry, temperature energy balance, snowfall, cloud simulation, wet surfaces or persistent colonies yet. Planet terrain currently shares the same authored template with different generation seeds. A cold profile suppresses liquid rain; snow requires a future effect. Wind motes are provisional visual cues.

`Scripts/create_planet_assets.py` authors the planet assets and WBP in the editor. It overwrites prototype maps' climate/seed settings and rebuilds the prototype widget; use it intentionally after manual UI changes. `Scripts/prepare_planet_assets.py` produces the simple placeholder textures. Map/UI/data/effect directories are explicitly included in packaging.
