"""Match the Cargo Bay WBP to the existing right-hand HUD panels."""
import json
from pathlib import Path
import unreal
from editor_toolset.toolsets.object import ObjectTools

root=Path(unreal.Paths.project_dir()).resolve()
assert root==Path(r'C:\UE5\SurviveThePlanet 5.8')
helpers=(root/'Scripts/create_trade_ui.py').read_text(encoding='utf-8')
exec(helpers[:helpers.index('credits=unreal.load_asset')])

def widgets(path):
    bp=unreal.load_asset(path)
    assert bp,path
    return bp,{str(n.widget_name):n.widget for n in umg.call_method('GetWidgets',(bp,)).widgets if n.widget}

mission,mission_nodes=widgets('/Game/UI/Mission/WBP_MissionConfidencePanel')
reference=mission_nodes['PanelBorder']
style=json.loads(ObjectTools.get_properties(reference,['background','brushColor']))
mission_layout=json.loads(ObjectTools.get_properties(reference.slot,['layoutData']))['layoutData']
resources,resource_nodes=widgets('/Game/UI/WBP_ResourceDisplay')
weather_host=resource_nodes['WeatherTimeDisplay']
weather_layout=json.loads(ObjectTools.get_properties(weather_host.slot,['layoutData']))['layoutData']
weather,weather_nodes=widgets('/Game/UI/WBP_WeatherTimeDisplay_V2')
weather_size=next(w for w in weather_nodes.values() if isinstance(w,unreal.SizeBox) and w.get_editor_property('override_height_override'))
weather_height=weather_size.get_editor_property('height_override')
gap=mission_layout['offsets']['top']-(weather_layout['offsets']['top']+weather_height)
assert gap>0,(gap,weather_height)

bp,nodes=widgets('/Game/UI/Trading/WBP_VisitingMerchant')
panel=nodes['MerchantPanel']
props(panel,style)
merchant_layout=json.loads(ObjectTools.get_properties(panel.slot,['layoutData']))['layoutData']
merchant_height=merchant_layout['offsets']['bottom']
layout=json.loads(json.dumps(mission_layout))
layout['offsets']['top']=mission_layout['offsets']['top']+mission_layout['offsets']['bottom']+gap
layout['offsets']['bottom']=merchant_height
props(panel.slot,{'layoutData':layout})
if 'TopAccent' in nodes:
    assert umg.call_method('RemoveWidget',(bp,nodes['TopAccent']))
assert umg.call_method('CompileWidgetBlueprint',(bp,))
assert unreal.EditorAssetLibrary.save_loaded_asset(bp)
actual=json.loads(ObjectTools.get_properties(panel,['background','brushColor']))
assert actual==style
assert layout['offsets']['top']-(mission_layout['offsets']['top']+mission_layout['offsets']['bottom'])==gap
unreal.log('MERCHANT_FRAME_MATCHED '+json.dumps({'gap':gap,'top':layout['offsets']['top'],'corner_radius':style['background']['outlineSettings']['cornerRadii']['x'],'outline_width':style['background']['outlineSettings']['width']}))
