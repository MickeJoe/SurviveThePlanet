import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
umg=unreal.get_default_object(unreal.UMGToolSet)
bp=unreal.load_asset('/Game/UI/WBP_BuildCost') if unreal.EditorAssetLibrary.does_asset_exist('/Game/UI/WBP_BuildCost') else None
if not bp:
 bp=umg.call_method('CreateWidgetBlueprint',('/Game/UI','WBP_BuildCost',unreal.BuildCostWidget))
assert bp,'Widget Blueprint creation failed'
def add(cls,name,parent=None):
 info=umg.call_method('AddWidget',(bp,cls,name,parent,-1))
 assert info.widget,name
 return info.widget
nodes=list(umg.call_method('GetWidgets',(bp,)).widgets)
if not any(str(n.widget_name)=='CostFrame' for n in nodes):
 for node in nodes:
  if not node.parent and node.widget:umg.call_method('RemoveWidget',(bp,node.widget))
 frame=add(unreal.Border,'CostFrame')
 frame.set_padding(unreal.Margin(1.5,1.5,1.5,1.5))
 frame.set_brush_color(unreal.LinearColor(.38,.29,.12,1))
 background=add(unreal.Border,'CostBackground',frame)
 background.set_padding(unreal.Margin(10,6,10,6))
 background.set_brush_color(unreal.LinearColor(.018,.038,.052,.97))
 stack=add(unreal.VerticalBox,'CostStack',background)
 description=add(unreal.TextBlock,'DescriptionText',stack)
 description.set_text('')
 description.set_visibility(unreal.SlateVisibility.COLLAPSED)
 font=description.get_editor_property('font');font.size=14;description.set_font(font)
 description.slot.set_padding(unreal.Margin(0,0,0,8))
 row=add(unreal.HorizontalBox,'SummaryRow',stack)
 icon=add(unreal.Image,'BuildingIcon',row)
 icon.set_desired_size_override(unreal.Vector2D(38,38))
 icon.slot.set_padding(unreal.Margin(0,0,12,0));icon.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
 costs=add(unreal.HorizontalBox,'CostRow',row)
 costs.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
 status=add(unreal.TextBlock,'StatusText',row)
 status.set_text('✓');font=status.get_editor_property('font');font.size=24;font.typeface_font_name='Bold';status.set_font(font)
 status.set_color_and_opacity(unreal.SlateColor(specified_color=unreal.LinearColor(.25,1,.15,1)))
 status.slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
for name in ['BuildingIcon','CostRow','StatusText','DescriptionText']:
 widgets={str(n.widget_name):n.widget for n in umg.call_method('GetWidgets',(bp,)).widgets}
 umg.call_method('ToggleWidgetAsVariable',(bp,widgets[name],True))
assert umg.call_method('CompileWidgetBlueprint',(bp,))
# Reuse the HUD's authored resource textures, including Connectors.
hud=unreal.load_asset('/Game/UI/WBP_ResourceDisplay')
configs=unreal.get_default_object(hud.generated_class()).get_editor_property('resources')
icons={c.resource_type:c.icon_texture for c in configs if c.icon_texture}
hud_widgets={str(n.widget_name):n.widget for n in umg.call_method('GetWidgets',(hud,)).widgets}
for key,name in [(unreal.ResourceType.ENERGY,'EnergyIcon'),(unreal.ResourceType.IRON,'IronIcon'),(unreal.ResourceType.CONTROL_CHIP,'ControlChipIcon'),(unreal.ResourceType.COPPER,'CopperIcon'),(unreal.ResourceType.STONE,'StoneIcon'),(unreal.ResourceType.WATER,'WaterIcon'),(unreal.ResourceType.CONCRETE,'ConcreteIcon'),(unreal.ResourceType.STEEL,'SteelIcon'),(unreal.ResourceType.COAL,'CoalIcon'),(unreal.ResourceType.POLYMER,'PolymerIcon'),(unreal.ResourceType.CONNECTOR,'ConnectorIcon')]:
 if name in hud_widgets:
  texture=hud_widgets[name].get_editor_property('brush').get_editor_property('resource_object')
  if texture:icons[key]=texture
assert len(icons)==11,icons
unreal.get_default_object(bp.generated_class()).set_editor_property('resource_icons',icons)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp)
unreal.log('BUILD_COST_WIDGET_CREATED '+str(len(icons))+' resource icons')
