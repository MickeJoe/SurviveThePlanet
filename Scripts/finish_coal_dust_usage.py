import unreal
material=unreal.load_asset('/Game/Units/Buildings/Coal/Materials/M_CoalDust')
unreal.MaterialEditingLibrary.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material,False)
unreal.log('COAL_DUST_ISM_READY')
