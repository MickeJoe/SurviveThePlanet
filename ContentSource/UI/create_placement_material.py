import unreal

path = '/Game/UI/Materials'
name = 'M_BuildPlacement'
material = unreal.load_asset(path + '/' + name)
if material is None:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, path, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property('two_sided', True)
    color = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionVectorParameter)
    color.set_editor_property('parameter_name', 'PlacementColor')
    color.set_editor_property('default_value', unreal.LinearColor(0, 0.8, 1, 1))
    opacity = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionScalarParameter)
    opacity.set_editor_property('parameter_name', 'PlacementOpacity')
    opacity.set_editor_property('default_value', 0.65)
    unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
assert material is not None
