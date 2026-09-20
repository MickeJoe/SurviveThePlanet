import math
import unreal

editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert editor.load_level('/Game/PlanetLevel_RockPreview')
preview_soft_fill = next(a for a in actors.get_all_level_actors() if a.get_actor_label() == 'E02_SoftFill')
soft_fill_rotation = preview_soft_fill.get_actor_rotation()
preview_light = preview_soft_fill.get_components_by_class(unreal.DirectionalLightComponent)[0]
soft_fill_intensity = preview_light.get_editor_property('intensity')
soft_fill_color = preview_light.get_editor_property('light_color')
assert editor.load_level('/Game/PlanetLevel')
all_actors = actors.get_all_level_actors()
grids = [a for a in all_actors if isinstance(a, unreal.HexSectorGrid)]
assert len(grids) == 1, 'Expected one sector grid in default map'
grid = grids[0]
unreal.log('POPULATION_GRID radius=' + str(grid.get_editor_property('exploration_sector_radius')) + ' rings=' + str(grid.get_editor_property('grid_radius')))
# Refresh the authored templates so map instances receive the current native layout.
templates = grid.get_editor_property('sector_templates')
for template in templates:
    index = templates.index(template)
    offset = ((index % 3) - 1) * 90.0
    pockets = []
    for center, extent in [
        ((0, 0), (940, 790)),
        ((-2050, 1250 + offset), (590, 510)),
        ((2050 + offset, 1250), (590, 510)),
        ((0, -2250 - offset), (620, 520))]:
        pocket = unreal.SectorBuildablePocket()
        pocket.set_editor_property('center', unreal.Vector2D(*center))
        pocket.set_editor_property('extent', unreal.Vector2D(*extent))
        pockets.append(pocket)
    template.set_editor_property('buildable_pockets', pockets)
    sockets = []
    corridors = []
    for side in range(6):
        angle = math.radians(30 + side * 60)
        socket = unreal.Vector2D(math.cos(angle) * grid.get_editor_property('exploration_sector_radius'),
                                 math.sin(angle) * grid.get_editor_property('exploration_sector_radius'))
        sockets.append(socket)
        closest = min(pockets, key=lambda p: (p.get_editor_property('center').x - socket.x) ** 2 +
                                              (p.get_editor_property('center').y - socket.y) ** 2)
        corridor = unreal.SectorCorridor()
        corridor.set_editor_property('start', socket)
        corridor.set_editor_property('end', closest.get_editor_property('center'))
        corridor.set_editor_property('width', 300.0)
        corridors.append(corridor)
    for pocket_index in range(1, 4):
        corridor = unreal.SectorCorridor()
        corridor.set_editor_property('start', pockets[0].get_editor_property('center'))
        corridor.set_editor_property('end', pockets[pocket_index].get_editor_property('center'))
        corridor.set_editor_property('width', 240.0)
        corridors.append(corridor)
    template.set_editor_property('connection_sockets', sockets)
    template.set_editor_property('drone_corridors', corridors)
    slots = template.get_editor_property('resource_slots')
    for i, slot in enumerate(slots):
        slot.set_editor_property('location', unreal.Vector2D(2400, 0) if i == 0 else unreal.Vector2D(1200, 2078))
    template.set_editor_property('resource_slots', slots)
grid.set_editor_property('sector_templates', templates)
grid.rebuild_grid()
surfaces = [a for a in all_actors if isinstance(a, unreal.PlanetSurfaceManager)]
assert len(surfaces) == 1
surface = surfaces[0]
chunk_size = surface.get_editor_property('cells_per_chunk') * surface.get_editor_property('tile_spacing')
required_radius = grid.get_editor_property('exploration_sector_radius') * (math.sqrt(3) * grid.get_editor_property('grid_radius') + 1.5)
diameter = 2 * math.ceil(required_radius / chunk_size)
surface.set_editor_property('chunk_diameter', max(surface.get_editor_property('chunk_diameter'), diameter))
surface.set_editor_property('chunk_meshes', [
    unreal.load_asset('/Game/Environment/Expedition03/Meshes/SM_TerrainFlat_03')])
surface.set_editor_property('chunk_material',
    unreal.load_asset('/Game/Environment/Expedition05/Materials/M_Terrain_05'))
# A second directional light competes for forward shading in the game map.
# Keep the preview-map light authored there, and use the existing sky/key setup here.
for soft_fill in [a for a in all_actors if a.get_actor_label() == 'E02_SoftFill']:
    actors.destroy_actor(soft_fill)
# Lift shadow colour through the existing skylight without introducing a second sun.
sky_lights = [a for a in all_actors if isinstance(a, unreal.SkyLight)]
assert len(sky_lights) == 1
sky = sky_lights[0].get_components_by_class(unreal.SkyLightComponent)[0]
sky.set_editor_property('intensity', 1.2)
sky.set_editor_property('indirect_lighting_intensity', 1.15)
sky.set_editor_property('min_occlusion', 0.18)
sky.set_editor_property('occlusion_exponent', 0.75)
sky.set_editor_property('lower_hemisphere_color', unreal.LinearColor(0.09, 0.075, 0.06, 1.0))
existing = [a for a in all_actors if isinstance(a, unreal.SectorPopulation)]
assert len(existing) <= 1
population = existing[0] if existing else actors.spawn_actor_from_class(unreal.SectorPopulation, unreal.Vector())
population.set_actor_label('Sector Population - Seeded Discovery')
population.set_editor_property('grid', grid)
population.set_editor_property('formation_count_per_sector', 4)
population.set_editor_property('landmark_mesh_count', 5)
population.set_editor_property('rock_mesh_count', 4)
population.set_editor_property('terrain_coverage_target', 0.40)
population.set_editor_property('terrain_coverage_variation', 0.08)
population.set_editor_property('start_sector_coverage_scale', 0.85)
decoration_paths = [
    '/Game/Environment/Expedition02/Meshes/SM_Cliff_Crown_02',
    '/Game/Environment/Expedition02/Meshes/SM_Cliff_Wall_02',
    '/Game/Environment/Expedition02/Meshes/SM_Cliff_Ridge_02',
    '/Game/Environment/Expedition04/Meshes/SM_Rock04_Pillar',
    '/Game/Environment/Expedition04/Meshes/SM_Rock04_Wedge',
    '/Game/Environment/Expedition02/Meshes/SM_Boulder_02',
    '/Game/Environment/Expedition02/Meshes/SM_Scree_02',
    '/Game/Environment/Expedition04/Meshes/SM_Rock04_Foot',
    '/Game/Environment/Expedition05/Meshes/SM_Pebbles05_Scatter',
    '/Game/Environment/Expedition02/Meshes/SM_Alien_Rosette_02',
    '/Game/Environment/Expedition02/Meshes/SM_Alien_Coral_02',
    '/Game/Environment/Expedition02/Meshes/SM_Alien_Bloom_Petal_02',
    '/Game/Environment/Expedition02/Meshes/SM_Alien_Trumpets_02',
    '/Game/Environment/Expedition06/Meshes/SM_Alien06_LowShrub',
    '/Game/Environment/Expedition06/Meshes/SM_Alien06_DiscFungus']
population.set_editor_property('decoration_meshes', [unreal.load_asset(p) for p in decoration_paths])
material_paths = [
    ['/Game/Environment/Expedition02/Materials/M_SM_Cliff_Crown_02'],
    ['/Game/Environment/Expedition02/Materials/M_SM_Cliff_Wall_02'],
    ['/Game/Environment/Expedition02/Materials/M_SM_Cliff_Ridge_02'],
    ['/Game/Environment/Expedition04/Materials/M_SM_Rock04_Pillar'],
    ['/Game/Environment/Expedition04/Materials/M_SM_Rock04_Wedge'],
    ['/Game/Environment/Expedition02/Materials/M_SM_Boulder_02'],
    ['/Game/Environment/Expedition02/Materials/M_SM_Scree_02'],
    ['/Game/Environment/Expedition04/Materials/M_SM_Rock04_Foot'],
    ['/Game/Environment/Expedition05/Materials/M_Pebbles_05'],
    ['/Game/Environment/Expedition02/Materials/M_Plant_Gold_Wind',
     '/Game/Environment/Expedition02/Materials/M_Plant_Olive_Wind'],
    ['/Game/Environment/Expedition02/Materials/M_Plant_Coral_Red_Wind',
     '/Game/Environment/Expedition02/Materials/M_Plant_Ember_Tips_Wind'],
    ['/Game/Environment/Expedition02/Materials/M_Plant_Petal_Wind'],
    ['/Game/Environment/Expedition02/Materials/M_Plant_Tube_Wind',
     '/Game/Environment/Expedition02/Materials/M_Plant_Inside_Wind',
     '/Game/Environment/Expedition02/Materials/M_Plant_Olive_Wind'],
    ['/Game/Environment/Expedition06/Materials/M_Plant06_Stem',
     '/Game/Environment/Expedition06/Materials/M_Plant06_Leaves'],
    ['/Game/Environment/Expedition06/Materials/M_Plant06_Stem',
     '/Game/Environment/Expedition06/Materials/M_Plant06_Cap',
     '/Game/Environment/Expedition06/Materials/M_Plant06_Gills']]
material_sets = []
for paths in material_paths:
    material_set = unreal.SectorMeshMaterialSet()
    material_set.set_editor_property('materials', [unreal.load_asset(p) for p in paths])
    material_sets.append(material_set)
population.set_editor_property('decoration_material_sets', material_sets)
population.set_editor_property('deposit_classes', {
    unreal.ResourceType.IRON: unreal.load_class(None, '/Game/BluePrints/Resources/BP_IronSource.BP_IronSource_C'),
    unreal.ResourceType.COPPER: unreal.load_class(None, '/Game/BluePrints/Resources/BP_CopparSource.BP_CopparSource_C'),
    unreal.ResourceType.STONE: unreal.load_class(None, '/Game/BluePrints/Resources/BP_StoneSource.BP_StoneSource_C')})
assert all(population.get_editor_property('decoration_meshes'))
assert len(population.get_editor_property('decoration_material_sets')) == len(decoration_paths)
assert all(population.get_editor_property('deposit_classes').values())
assert surface.get_editor_property('chunk_material')
assert editor.save_current_level()
unreal.log('DEFAULT_POPULATION_CONFIGURED')
