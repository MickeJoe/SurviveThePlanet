# Geothermal resources

Aridus has GeothermalSiteFrequency = 0.28 (one site in approximately 28% of non-start sectors) and bGuaranteeStartingGeothermalSite = true. Other planet definitions default to no sites. Generation uses the planet seed and validates ground, mineral spacing and the existing terrain/building clearance. Sites spawn with their sector discovery.

Geothermal construction snaps to a site and must be centered on an available site. The same rule is checked by preview, click placement and the final surface reservation. Other buildings cannot cover a geothermal site. Removing the plant releases the permanent heat source through weak ownership.

The 280 x 260 cm plant uses the existing catalog costs, energy production and drone slots. Blender models, icon, pulsing fissure glow, Niagara steam and a turbine rotor live in /Game/Geothermal. Steam and turbine activity follows operational state; resource steam continues before construction. Existing game speed/pause controls drive component ticks.

Verified in Aridus PIE: starting resource, additional seeded sites, 3x3 footprint, ground/occupied-site rejection, final reservation, ordinary-building protection, power connection, turbine motion and reuse after demolition. Scripts/test_geothermal_runtime.py keeps all spawned fixtures inside the unsaved PIE world.

## Visual presentation

- The compact 280 x 260 cm plant keeps its existing 3x3 footprint and uses wider, depth-offset towers, flange bolts, panel seams, service panels, valves and a turbine shroud.
- Irregular fractured basalt polygons replace the hexagonal tile patch. Open gaps expose a connected glowing bed, with scattered basalt and low mineral crust around the edge.
- M_GeoDetail materials use seamless surface/rock, roughness and normal maps with different metallic responses. The heat material pulses between strengths 8.5 and 9.5. Surface UVs use a consistent 120 cm world-scale projection in Blender.
- NS_GeothermalSteam has 20 sprite particles/s per vent, a 3.2-4.3 s lifetime, turbulent buoyant velocity and growing textured sprites with soft depth fade. User.Wind and User.SteamColor are supplied by the shared effects component. Resource vapor is smaller and less dense; claimed-site vapor is routed through the plant exhausts.
- Four warm work lights respond to available sunlight, fading in below 35% and reaching full intensity at night. Steam color also responds to sunlight. Preview actors do not create effects; unpowered plants stop their steam and rotor.

Authoring: Scripts/create_geothermal_blender.py runs only through Blender's native MCP; Scripts/make_geo_textures.py generates repeatable texture source maps; Scripts/import_geothermal_ue.py imports and assigns assets. Niagara is authored through UE 5.8 native MCP. Materials are reused on repeated imports to avoid destructive material graph deletion in UE 5.8.

Verification: run Scripts/test_geothermal_runtime.py in the editor Python console, then start PIE through native MCP. Scripts/verify_geothermal_visuals.py checks active Niagara, wind overrides, darkness-driven work lights, and captures daytime/nighttime images from the gameplay camera in the unsaved test session.

FBX coordinate convention: Blender X/Z are preserved and Y is mirrored by Unreal import. Native and Blueprint vent/rotor positions use the imported coordinates. The Niagara system loop and emitter loop are both Infinite; a one-shot system loop otherwise stops visible vapor even when its component remains active.
