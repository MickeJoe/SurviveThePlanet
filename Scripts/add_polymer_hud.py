import unreal,json
from pathlib import Path
from editor_toolset.toolsets.object import ObjectTools
bp=unreal.load_asset("/Game/UI/WBP_ResourceDisplay");umg=unreal.get_default_object(unreal.UMGToolSet)
nodes=list(umg.call_method("GetWidgets",(bp,)).widgets)
byname={str(n.widget_name):n for n in nodes if n.widget}
def clone_properties(old,new):
 schema=json.loads(ObjectTools.list_properties(old))
 new_schema=json.loads(ObjectTools.list_properties(new))
 keys=[k for k in schema if k in new_schema and not k.startswith("on") and not k.endswith("Delegate") and k not in ["slot","navigation","toolTipWidget","bindings"]]
 values=ObjectTools.get_properties(old,keys)
 assert ObjectTools.set_properties(new,values),(old.get_name(),values)
root=byname["ResourceDisplayRoot"].widget
selected=[];collect=False
for node in nodes:
 if str(node.widget_name)=="SteelResourceCard":collect=True
 if collect:
  if str(node.widget_name)=="WeatherTimeDisplay":break
  if node.widget:selected.append(node)
created={}
if True:
 for node in selected:
  oldname=str(node.widget_name);name=oldname.replace("Steel","Polymer")
  parent=root if oldname=="SteelResourceCard" else created[node.parent.get_name()]
  added=byname[name] if name in byname else umg.call_method("AddWidget",(bp,node.widget.get_class(),name,parent,-1))
  widget=added.widget;assert widget,name;created[oldname]=widget
  clone_properties(node.widget,widget)
  if oldname!="SteelResourceCard" and node.slot:clone_properties(node.slot,added.slot)
  if name in ["PolymerIcon","PolymerAmountText","PolymerRateText"]:umg.call_method("ToggleWidgetAsVariable",(bp,widget,True))
  if oldname=="SteelResourceCard":
   schema=json.loads(ObjectTools.list_properties(added.slot));assert "layoutData" in schema and "bAutoSize" in schema
   vals={"layoutData":{"offsets":{"left":-600,"top":92,"right":0,"bottom":0},"anchors":{"minimum":{"x":.5,"y":0},"maximum":{"x":.5,"y":0}},"alignment":{"x":0,"y":0}},"bAutoSize":True}
   assert ObjectTools.set_properties(added.slot,json.dumps(vals))
 created["SteelIcon"].set_brush_from_texture(unreal.load_asset("/Game/UI/Icons/Buildings/T_PolymerPlant"),False)
 created["SteelAmountText"].set_text("0");created["SteelRateText"].set_text("+0.0/min")
 created["SteelResourceCard"].set_tool_tip_text("Polymer")
umg.call_method("CompileWidgetBlueprint",(bp,))
assert unreal.EditorAssetLibrary.save_loaded_asset(bp)
Path(unreal.Paths.project_dir()).joinpath("Saved/PolymerHUD.json").write_text(json.dumps({"passed":True,"widget":bp.get_path_name()}))


