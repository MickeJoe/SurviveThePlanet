import unreal,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root=Path('C:/UE5/SurviveThePlanet 5.8')
try:
 bp=unreal.load_asset('/Game/BluePrints/Buildings/STP/BP_STP_NuclearReactor');data=unreal.load_asset('/Game/Data/Buildings/STP/DA_STP_NuclearReactor')
 unreal.BlueprintEditorLibrary.reparent_blueprint(bp,unreal.NuclearReactor)
 cdo=unreal.get_default_object(bp.generated_class())
 for obj in [data,cdo]:
  obj.set_editor_property('max_drone_slots',4);obj.set_editor_property('initially_unlocked_drone_slots',4);obj.set_editor_property('drone_work_type',unreal.STPDroneWorkType.CONSTRUCTION)
 cdo.set_editor_property('fuel_rods_per_cycle',1);cdo.set_editor_property('cooling_units_per_cycle',1);cdo.set_editor_property('cycle_seconds',60)
 unreal.BlueprintEditorLibrary.compile_blueprint(bp);assert bp.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE
 data.set_editor_property('building_class',bp.generated_class());unreal.EditorAssetLibrary.save_loaded_asset(data);unreal.EditorAssetLibrary.save_loaded_asset(bp)
 result={'passed':True}
except Exception:result={'passed':False,'error':traceback.format_exc()}
(root/'Saved/ReactorSetup.json').write_text(json.dumps(result,indent=2))
