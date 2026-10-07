import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir());src=root/'ContentSource/PolymerPlant'
tools=unreal.AssetToolsHelpers.get_asset_tools()
for name,folder in [('T_PolymerPlant_BaseColor','/Game/Units/Buildings/PolymerPlant/Textures'),('T_PolymerPlant','/Game/UI/Icons/Buildings')]:
 task=unreal.AssetImportTask();task.filename=str(src/(name+'.png'));task.destination_path=folder;task.destination_name=name;task.replace_existing=True;task.automated=True;task.save=True;tools.import_asset_tasks([task])
m=unreal.load_asset('/Game/Units/Buildings/PolymerPlant/Materials/M_PolymerSurface_v2')
for node in unreal.ObjectIterator(unreal.MaterialExpressionConstant):
 if node.get_outer()!=m:continue
 if abs(node.r-.24)<.001:node.r=.06
 elif abs(node.r-.56)<.001:node.r=.60
unreal.MaterialEditingLibrary.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log('POLYMER_PALETTE_UPDATED')
