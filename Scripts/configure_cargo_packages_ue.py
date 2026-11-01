"""Author Tier 1 production assets from the shared balance configuration; preserves existing meshes."""
import unreal,json,traceback
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path('C:/UE5/SurviveThePlanet 5.8')
config=json.loads((root/'Config/Objectives.json').read_text())
results=[]
def resource(name):
 return getattr(unreal.ResourceType,{'ControlChip':'CONTROL_CHIP','PurifiedWater':'PURIFIED_WATER','SyntheticFuel':'SYNTHETIC_FUEL'}.get(name,name.upper()))
try:
 for key,settings in config['cargoProduction'].items():
  path='/Game/Data/Buildings/'+('DA_PolymerPlant' if key=='PolymerPlant' else 'BlueprintCatalog/DA_'+key)
  data=unreal.load_asset(path);assert data,path
  cls=data.get_editor_property('building_class');assert cls,key
  bp=unreal.EditorAssetLibrary.load_blueprint_class_asset(cls.get_path_name()) if False else unreal.load_asset(cls.get_path_name().split('.')[0])
  assert bp,key
  if 'recipe' in settings:
   unreal.BlueprintEditorLibrary.reparent_blueprint(bp,unreal.ThermalProcessingBuilding)
  cdo=unreal.get_default_object(bp.generated_class())
  costs=[unreal.ResourceCost(resource=resource(k),cost=v) for k,v in settings['costs'].items()]
  for obj in [data,cdo]:
   obj.set_editor_property('construction_costs',costs)
   obj.set_editor_property('energy_consumption_per_minute',float(settings['energy']))
  data.set_editor_property('override_energy_settings',True)
  if 'recipe' in settings:
   cdo.set_editor_property('building_data',data)
   cdo.set_editor_property('recipe',getattr(unreal.ThermalRecipe,settings['recipe']))
   cdo.set_editor_property('cycle_seconds',float(settings['cycleSeconds']))
   cdo.set_editor_property('output_per_cycle',settings['outputAmount'])
   cdo.set_editor_property('primary_input_per_cycle',2)
   cdo.set_editor_property('purified_water_per_cycle',1)
   for obj in [data,cdo]:
    obj.set_editor_property('max_drone_slots',4)
    obj.set_editor_property('initially_unlocked_drone_slots',4)
    obj.set_editor_property('drone_work_type',unreal.STPDroneWorkType.CONSTRUCTION)
  if key=='PolymerPlant':
   for prop,value in [('coal_per_cycle',2),('water_per_cycle',1),('polymer_per_cycle',3),('cycle_seconds',20.)]:data.set_editor_property(prop,value)
  unreal.BlueprintEditorLibrary.compile_blueprint(bp)
  assert bp.get_editor_property('status')==unreal.BlueprintStatus.BS_UP_TO_DATE,key
  data.set_editor_property('building_class',bp.generated_class())
  assert unreal.EditorAssetLibrary.save_loaded_asset(data,False)
  assert unreal.EditorAssetLibrary.save_loaded_asset(bp,False)
  actual={str(c.resource):c.cost for c in unreal.get_default_object(bp.generated_class()).get_construction_costs()}
  assert actual=={str(resource(k)):v for k,v in settings['costs'].items()},(key,actual)
  results.append({'building':key,'class':bp.generated_class().get_path_name(),'settings':settings,'effectiveCosts':actual})
 result={'passed':True,'assets':results}
except Exception:result={'passed':False,'error':traceback.format_exc(),'assets':results}
(root/'Balance/CargoPackageAssets.json').write_text(json.dumps(result,indent=2))
unreal.log('CARGO_PACKAGE_ASSETS '+json.dumps(result))
unreal.SystemLibrary.quit_editor()
