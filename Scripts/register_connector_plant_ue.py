"""Register the existing Connector Factory mesh as a buildable Connector Plant."""
import unreal,json
from pathlib import Path
from editor_toolset.toolsets.object import ObjectTools
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r"C:\UE5\SurviveThePlanet 5.8")
lib=unreal.EditorAssetLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools()
mesh=unreal.load_asset("/Game/Units/Buildings/ConnectorFactory/Meshes/SM_ConnectorFactory");assert mesh
icon=unreal.load_asset("/Game/UI/Icons/Buildings/T_ConnectorFactory");assert icon
data=unreal.load_asset("/Game/Data/Buildings/DA_ConnectorPlant") or tools.create_asset("DA_ConnectorPlant","/Game/Data/Buildings",unreal.ConnectorPlantBuildingDataAsset,unreal.DataAssetFactory())
assert isinstance(data,unreal.ConnectorPlantBuildingDataAsset)
for key,value in {
    "display_name":"Connector Plant","description":"Consumes 2 copper and 1 polymer every 20 seconds to produce 3 connectors. Requires 12 electricity/minute.",
    "build_category":unreal.STPBuildCategory.INDUSTRY,"build_tool":unreal.STPBuildTool.CONNECTOR_PLANT,
    "building_type":unreal.STPBuildingType.CONNECTOR_PLANT,"building_tag":"ConnectorPlant","blueprint_id":"ConnectorPlant",
    "building_mesh":mesh,"thumbnail":icon,"toolbar_icon":icon,"toolbar_sort_order":40,
    "blueprint_initially_owned":True,"show_in_build_toolbar":True,"override_energy_settings":True,
    "energy_consumption_per_minute":12.0,"copper_per_cycle":2,"polymer_per_cycle":1,"connectors_per_cycle":3,"cycle_seconds":20.0
}.items():data.set_editor_property(key,value)
polymer=unreal.load_asset("/Game/Data/Buildings/DA_PolymerPlant");assert polymer
data.set_editor_property("construction_costs",list(polymer.get_editor_property("construction_costs")))
bp=unreal.load_asset("/Game/BluePrints/Buildings/ConnectorFactory/BP_ConnectorFactory");assert bp
unreal.BlueprintEditorLibrary.reparent_blueprint(bp,unreal.ConnectorPlant)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
cdo=unreal.get_default_object(bp.generated_class());assert isinstance(cdo,unreal.ConnectorPlant)
for key,value in {"building_data":data,"building_type":unreal.STPBuildingType.CONNECTOR_PLANT,"building_tag":"ConnectorPlant","building_display_name":"Connector Plant","construction_progress":0.0}.items():cdo.set_editor_property(key,value)
for component in cdo.get_components_by_class(unreal.StaticMeshComponent):
    if component.get_name()=="BuildingMesh":
        component.set_static_mesh(mesh)
        component.set_editor_property("relative_rotation",unreal.Rotator(pitch=0,yaw=90,roll=0))
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
data.set_editor_property("building_class",bp.generated_class())
assert lib.save_loaded_asset(data)
assert lib.save_loaded_asset(bp)
catalog=unreal.load_asset("/Game/Data/Buildings/DA_BuildingCatalog");assert catalog
entries=list(catalog.get_editor_property("buildings"))
if data not in entries:entries.append(data)
catalog.set_editor_property("buildings",entries);assert lib.save_loaded_asset(catalog)
# Replace our old display-only startup factory with player construction.
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
removed=[]
for actor in actors.get_all_level_actors():
    if actor.get_actor_label()=="Connector Factory - Starting Site":
        assert isinstance(actor,unreal.BaseModuleSpawnPoint)
        removed.append(actor.get_actor_label())
        assert actors.destroy_actor(actor)
if removed:assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
# Clone the existing Polymer resource card using the project's UMG authoring API.
widget_bp=unreal.load_asset("/Game/UI/WBP_ResourceDisplay")
umg=unreal.get_default_object(unreal.UMGToolSet)
nodes=list(umg.call_method("GetWidgets",(widget_bp,)).widgets)
byname={str(n.widget_name):n for n in nodes if n.widget}
card=byname["PolymerResourceCard"].widget;canvas=byname["ResourceDisplayRoot"].widget
selected=[];seen=set()
for node in nodes:
    if node.widget and (node.widget==card or (node.parent and node.parent.get_name() in seen)):
        selected.append(node);seen.add(node.widget.get_name())
assert selected
def clone_properties(old,new):
    schema=json.loads(ObjectTools.list_properties(old));target=json.loads(ObjectTools.list_properties(new))
    keys=[k for k in schema if k in target and not k.startswith("on") and not k.endswith("Delegate") and k not in ["slot","navigation","toolTipWidget","bindings"]]
    assert ObjectTools.set_properties(new,ObjectTools.get_properties(old,keys))
created={}
for node in selected:
    oldname=str(node.widget_name);name=oldname.replace("Polymer","Connector") if "Polymer" in oldname else "Connector_"+oldname
    parent=canvas if node.widget==card else created[node.parent.get_name()]
    added=byname[name] if name in byname else umg.call_method("AddWidget",(widget_bp,node.widget.get_class(),name,parent,-1))
    widget=added.widget;assert widget,name;created[node.widget.get_name()]=widget
    clone_properties(node.widget,widget)
    if node.widget!=card and node.slot:clone_properties(node.slot,added.slot)
    if name in ["ConnectorIcon","ConnectorAmountText","ConnectorRateText"]:umg.call_method("ToggleWidgetAsVariable",(widget_bp,widget,True))
    if node.widget==card:
        schema=json.loads(ObjectTools.list_properties(added.slot));assert "layoutData" in schema and "bAutoSize" in schema
        values={"layoutData":{"offsets":{"left":-465,"top":92,"right":0,"bottom":0},"anchors":{"minimum":{"x":.5,"y":0},"maximum":{"x":.5,"y":0}},"alignment":{"x":0,"y":0}},"bAutoSize":True}
        assert ObjectTools.set_properties(added.slot,json.dumps(values))
created["PolymerIcon"].set_brush_from_texture(icon,False)
created["PolymerAmountText"].set_text("0");created["PolymerRateText"].set_text("+0.0/min")
created["PolymerResourceCard"].set_tool_tip_text("Connectors")
umg.call_method("CompileWidgetBlueprint",(widget_bp,))
hud_cdo=unreal.get_default_object(widget_bp.generated_class())
configs=list(hud_cdo.get_editor_property("resources"))
if not any(c.resource_type==unreal.ResourceType.CONNECTOR for c in configs):
    config=unreal.ResourceDisplayConfig();config.set_editor_property("resource_type",unreal.ResourceType.CONNECTOR);config.set_editor_property("tooltip","Connectors");config.set_editor_property("icon_texture",icon);configs.append(config)
hud_cdo.set_editor_property("resources",configs)
umg.call_method("CompileWidgetBlueprint",(widget_bp,));assert lib.save_loaded_asset(widget_bp)
report={"passed":True,"data":data.get_path_name(),"blueprint":bp.get_path_name(),"recipe":{"copper":2,"polymer":1,"connectors":3,"seconds":20,"electricity_per_minute":12},"removed_display_spawn_points":removed,"hud":widget_bp.get_path_name(),"footprint":str(cdo.get_grid_footprint())}
(root/"Saved/ConnectorPlantIntegration.json").write_text(json.dumps(report,indent=2))
unreal.log("CONNECTOR_PLANT_INTEGRATED "+json.dumps(report))


