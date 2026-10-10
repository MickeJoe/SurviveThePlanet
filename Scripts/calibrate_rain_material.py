"""Make prototype rain readable under the template's fixed exposure."""
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
mat=unreal.load_asset('/Game/Weather/M_RainStreak')
assert mat
mat.set_editor_property('used_with_instanced_static_meshes',True)
unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
color=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,0,0)
color.set_editor_property('constant',unreal.LinearColor(.7,1.0,1.3,1))
alpha=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant,0,150)
alpha.set_editor_property('r',.35)
unreal.MaterialEditingLibrary.connect_material_property(color,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.connect_material_property(alpha,'',unreal.MaterialProperty.MP_OPACITY)
unreal.MaterialEditingLibrary.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
wind=unreal.load_asset('/Game/Weather/M_WindMote')
assert wind
wind.set_editor_property('used_with_instanced_static_meshes',True)
unreal.MaterialEditingLibrary.recompile_material(wind)
unreal.EditorAssetLibrary.save_loaded_asset(wind)
exec(compile((root/'Scripts/test_planet_selector.py').read_text(encoding='utf-8'),str(root/'Scripts/test_planet_selector.py'),'exec'))
