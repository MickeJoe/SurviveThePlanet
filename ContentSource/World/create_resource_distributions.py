"""Create authored rocky-planet inputs with UE 5.8 -ExecutePythonScript."""
import unreal

for name, count, quantity in [('DA_Rocky_Sparse', 2, 1200), ('DA_Rocky_Rich', 6, 4000)]:
    path = '/Game/World/ResourceDistributions/' + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        continue
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.PlanetResourceDistribution)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, '/Game/World/ResourceDistributions', unreal.PlanetResourceDistribution, factory)
    rule = unreal.PlanetResourceRule()
    rule.set_editor_property('id', 'Iron')
    rule.set_editor_property('resource_type', unreal.ResourceType.IRON)
    rule.set_editor_property('slot_type', 'Mineral')
    rule.set_editor_property('min_count', count)
    rule.set_editor_property('max_count', count)
    rule.set_editor_property('min_quantity', quantity)
    rule.set_editor_property('max_quantity', quantity)
    asset.set_editor_property('rules', [rule])
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
