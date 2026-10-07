import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
manager=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.ResourceManager)[0]
controller=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SurviveThePlanetPlayerController)[0]
library=unreal.get_default_object(unreal.load_class(None,"/Script/UMG.WidgetBlueprintLibrary"))
toolbar=library.call_method("GetAllWidgetsOfClass",(world,unreal.BuildToolbarWidget,False))[0]
for res,n in [(unreal.ResourceType.IRON,100),(unreal.ResourceType.CONCRETE,100),(unreal.ResourceType.CONTROL_CHIP,10)]:
 manager.set_resource_amount(res,n)
toolbar.call_method("HandleIndustryCategoryClicked",())
bindings=[b for b in unreal.ObjectIterator(unreal.load_class(None,"/Script/SurviveThePlanet.BuildToolbarClickBinding")) if b.get_outer()==toolbar]
selected=False
for b in bindings:
 b.call_method("Click",())
 if controller.get_active_build_tool()==unreal.STPBuildTool.POLYMER_PLANT:
  selected=True
  break
(root/"Saved/PolymerButtonTest.json").write_text(json.dumps({"passed":selected,"bindings":len(bindings),"active_tool":str(controller.get_active_build_tool())},indent=2))
assert selected,"The toolbar has no live polymer click binding"
controller.set_active_build_tool(unreal.STPBuildTool.NONE)

